

# Note: I found there is also QFileSystemModel that serves a different
# purpose but probably could become a part of this one. Meaning:
# you could pass it as 'qtc'. That way a FileSystem could be immediately
# displayed in a QTreeView for example.
module Reform
  class FileSystem < AbstractModel
    private
      def initialize parent, qtc = nil
        if Hash === parent
          super()
          setup(parent)
        else
          if Hash === qtc
            super(parent)
            hash = qtc
          else
            hash = {}
          end
#           if parent && parent.model?
#             @root = parent.root
#             @keypath = ?????
#           else
#             @root = self
#           end
          setup(hash)
        end
      end

      def setup hash
        @dirname = Dir::getwd
        @filename = nil
        @itemname = tr('an item')
        reset_captions
        @file = nil
        @reg = { }
        hash.each do |k, v|
          case k
          when :dirname then @dirname = v
          when :filename then @filename = v
          when :itemname then @itemname = v
          when :pattern, :filter then @pattern = v
          when :open_caption then @open_caption = v
          when :save_caption then @save_caption = v
          when :saveas_caption then @saveas_caption = v
          when :register then @reg.merge!(v)
          else
            raise ArgumentError, "Bad arg '#{k}'"
          end
        end
      end

      def dirname value = nil
        return @dirname unless value
        @dirname = value
        @file = nil
      end

      def filename value = nil
        return @filename unless value
        @filename = value
        @file = nil
      end

      # If the opened file matches some pattern, the klass is used to load it by calling
      # klass::load.
      # The result must be a Model. If it is not Structure is wrapped around it.
      def register pattern, klass
        @reg[pattern] = klass
      end

      def postSetup
        @filename and load
      end

      def reset_captions
        @open_caption = @save_caption = @saveas_caption = nil
      end

      def load
        return unless @filename
        for pattern, klass in @reg
          if @filename =~ pattern
            model = klass::load(path)
            unless model.respond_to?(:model?) && model.model?
              model = Structure.new(value: model, keypath: [])
            end
            return @file = model
          end
        end
      end

      def splitPath(filename)
        [File.dirname(filename), File.basename(filename)]
      end

      def store aPath
        dirname, filename = splitPath(aPath)
        for pattern, klass in @reg
          if filename =~ pattern
            klass::store(@file, aPath)
            @dirname, @filename = dirname, filename
            return
          end
        end
        raise Error, tr("No storage class registered for '%s'") % filename
      end

    public

      def path
        "#@dirname#{@dirname[-1] == '/' || @filename && @filename[0] == '/' ? '' : '/'}#@filename"
      end

#       attr_accessor :dirname, :filename

      # for internal use only!!
      attr_writer :dirname, :filename

      # the default is 'an item', setting it resets all captions !!
      def itemname value = nil
        return @itemname unless value
        @itemname = value
        reset_captions
      end

      # set the pattern as expected by FileDialog. Use semicolon as separator.
      # Example:        pattern '*.png;*.jpg'
      def pattern value = nil
        return @pattern unless value
        @pattern = value
      end

      alias :filter :pattern            # this is how FileDialog calls it

      # use this to set the whole caption for the file-open dialog
      # do not set itemname afterwards, that will erase it.
      def open_caption value = nil
        if value
          @open_caption = value
        else
          @open_caption || tr('Pick %s') % @itemname
        end
      end

      def save_caption value = nil
        if value
          @save_caption = value
        else
          @save_caption || tr('Save %s') % @itemname
        end
      end

      def saveas_caption value = nil
        if value
          @saveas_caption = value
        else
          @saveas_caption || @save_caption || tr('Save %s') % @itemname
        end
      end

      # set the path to load a file
      def path= filename
        return if !filename || filename.empty?
        pickup_tran do |tran|
#           tag "pickup_tran, sender = #{tran.sender}"
          org_dirname, org_filename = @dirname, @filename
          org_file = @file
          @dirname, @filename = splitPath(filename)
          unless tran.aborted?
            tran.addPropertyChange :dirname, org_dirname
            tran.addPropertyChange :filename, org_filename
            tran.addDependencyChange :path
            tran.addPropertyChange :file, org_file
#             tag "tran.changed_keys = #{tran.changed_keys.inspect}"
          end
          load # if this fails, then @dirname and @filename are left UNCHANGED automagically!!
#           tag "Did load, @file = #{@file.value.inspect}, propagate change"
        end
      end

        # shows FileDialog to open a file and assign the contents to _file_
      def open parent = nil
#         tag "apply_setter path, sender = #{parent}"
        apply_setter(:path, Qt::FileDialog.getOpenFileName(parent && parent.qtc, open_caption, @dirname, @pattern),
                     parent)
      end

      # returns the contents of the file. It should be a model
      def file
        @file || load
      end

      # this should save the model to disk again. Meant as 'save as'
      def saveas parent = nil
        pth = Qt::FileDialog.getSaveFileName(parent && parent.qtc, saveas_caption, path, @pattern) or return
        store(pth)
      end
  end

  createInstantiator File.basename(__FILE__, '.rb'), nil, FileSystem

end