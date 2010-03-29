
require_relative '../rrts'
require_relative 'track'
require 'forwardable'

module RRTS

=begin SOME THOUGHTS

Sending non events to nodes is probably bad.
Instead of sending a track to a chunk we could sent a meta event like
CreateTrackEvent.
=end

  module Node
=begin rdoc
    a Chunk has some meta information and contains a *single* track
    but this may be a CompoundTrack
    A chunk is an enumerable of events, but more importantly, it
    has actual storage for events.

    The following methods delegate to @track:
    - rewind, Reset the track, and/or all subtracks
    - each. Chunks can be treated as eventservers
    - next. Return the next event. The first time called (or after rewind)
            it returns the first event. If the track is a compound track
	    it returns the event with smallest timestamp and priority
    - peek. Return the same event as next but without changing the track position
    - listing. Return a flat array with all contained tracks
=end
    class Chunk < EventsNode
      include Enumerable
      extend Forwardable
      # constant for setting the key
      MAJOR = true
      # constant for setting the key
      # It is possible to change the key using a KeySignatureEvent
      MINOR = false
      private
      # Option used is _split_tracks_ to record each channel on its
      # own track. This does not merge tracks, even if they share a channel
      # All other options are passed on to the track
      # However, a track is only created when the first event is send
      def initialize options = {}
        @split_tracks = true
        for k, v in options
          @split_tracks = v if k == :split_tracks
          # the rest is passed on
        end
        @options = options    # to pass to CompoundTrack
        require_relative '../tempo'
        @tempo = Tempo.new
        @time_signature = nil #  4, 4  do not set it! Otherwise saving/loading MIDI will result in
                              # differences.  Can be interpreted on other level if so required
        @clocks_per_beat = nil
        @key = nil # :C, MAJOR no default here
        @track = nil
        @track_index = {} # hash indexed by key
        # track remains empty until the first event is sent (using <<)
      end
      public

      def to_yaml_properties
        [:@tempo, :@time_signature, :@clocks_per_beat, :@key, :@track]
      end

      # the initial tempo (Tempo instance). Note this is an override from EventsNode
      attr_accessor :tempo

      # the key is a tuple like [:C, MAJOR]
      # the time signature is a tuple like [4,4]
      # clocks_per_beat is typically 24
      # These values may change during recording(!)
      attr_accessor :key, :time_signature, :clocks_per_beat
      # the internal track, normally a CompoundTrack
      attr :track
      # a hash indexed by Track#key. Also a flat list of all tracks.
      # it is maintained as tracks are added using <<.
      attr :track_index

      # Sets the ticks per beat. Tempochanges later on are possible
      # by sending a TempoEvent
      def ticks_per_beat= ppq
        @tempo.ppq = ppq
      end

      # ppq == pulses per quarter, same as ticks per beat
      alias :ppq= :ticks_per_beat=

      # builder compatibilty. It is possible to add events
      # but the argument can also be a track. That track will
      # then be used as the current track. More than one track
      # can be added to the chunk
      def << event
        case event
        when BaseTrack
          @track_index[event.key] = event
          if @track # aready exists
            if CompoundTrack === @track
              @track << event
            else
              @track = CompoundTrack.new(@track, @options)
            end
          elsif @split_tracks
            @track = CompoundTrack.new(event, @options)
          else
            @track = event
          end
          return self
        when TimeSignatureEvent
          @time_signature = event.time_signature
          @clocks_per_beat = event.clocks_per_beat
        when KeySignatureEvent
          @key = event.key_signature
        when TempoEvent
          @tempo.tempo = event.usecs_per_beat
        end
        @track << event
        self
      end

      def_delegators :@track, :each, :rewind, :next, :peek, :listing

      # Are tracks supported, or only events? Always true for Chunk.
      def has_tracks?
        true
      end

      # recreate track.events (after being loaded) as empty array
      def fix_tracks
        @track.fix
        @track_index = {}
#         tag "track=#{@track.inspect}"
#         tag "listing = #{@track.listing}"
        for track in @track.listing(true) # allow empty!
          @track_index[track.key] = track
        end
      end

      # return the designated track from the index. Event should be received
      # from a yaml stream where it lost its track reference.
      def track_for(event)
#         case event.track
#         when BaseTrack then
        @track_index[event.track] or raise RRTSError.new("Track #{event.track} could not be located, keys=#{@track_index.keys.inspect}")
#         else event.track
#         end
      end
    end # class Chunk

  end # Node
end # RRTS