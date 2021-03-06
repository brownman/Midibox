
#  Copyright (c) 2010 Eugene Brazwick

module Reform
  class Animation < Control
    include AnimationContext

    private

      def initialize parent, qtc
        super
        @autostart = !(Animation === parent)
      end

      def duration v
        v = v.val if Milliseconds === v
        @qtc.duration = v
      end

      def appendTo animid
        containing_form[animid].qtc.addAnimation(@qtc)
      end

      def duration ms
        ms = ms.val if Milliseconds === ms
        @qtc.duration = ms
      end

      # time to set immediately after starting (the very first loop)
      # only works with autostart currently
      def startTime ms
        @startTime = Milliseconds === ms ? ms.val : ms
      end

      def currentTime ms
        @qtc.currentTime = Milliseconds === ms ? ms.val : ms
      end

      define_simple_setter :loopCount

      def looping
        @qtc.loopCount = -1
      end

    protected # Animation methods

      attr_writer :autostart

      def autostart value = nil
        return @autostart unless value
        @autostart = value
      end

      def autostart?
        @autostart
      end

    public # Animation methods

      def addTo parent, hash, &block
        parent.addAnimation self, hash, &block
      end

      def addAnimation anim, hash, &block
        super
#         tag "appending #{anim.qtc} to group #@qtc"
        @qtc.addAnimation(anim.qtc)
        anim.autostart = false
      end

      def postSetup
        if @autostart
          @qtc.start
          @qtc.currentTime = @startTime if @startTime
        end
      end

  end #class Animation
end # module Reform
