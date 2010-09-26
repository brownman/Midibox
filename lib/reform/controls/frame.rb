
module Reform

  require_relative 'widget'

=begin rdoc

a Frame is a widget that may contain others.

Note I changed 'containing_frame to the Qt 'parent' of the object.
And so 'all_children' becomes simply 'children'.
However, in the previous version a form had 'containing_frame' being the form itself.
=end
  class Frame < Widget
    include ControlContext

    private

    def initialize frame, qtc, autolayout = true
#       tag "CREATING FRAME, qtc=#{qtc}"
      super(frame, qtc)
      # all immediate controls within this panel are in here
      # but also the controls added to Layouts, since Qt layouts do not own them.
      # generate layouts automatically.
      # A layout can also be forced if a widget use a method only valid if its parent
      # is a layout, and the frame is not a layout.
      # in both cases @infused_layout is set.
      @autolayout = autolayout
#       tag "Frame.new EXECUTED"
    end

    # note hash and block are setups for 'control'
    def check_layout control, creator = :formlayout, hash = nil, &block
      unless layout = infused_layout
        # problematic: the creator tends to call postSetup, but it must be delayed
#         tag "missing layout, create a '#{creator}'"
        layout = send(creator, postSetup: false)
#         tag "CREATE IMPLICIT LAYOUT #{self}, qtc=#@qtc, layout=#{layout}, layout.qtc=#{layout.qtc}, creator='#{creator}'"
#         tag "caller = #{caller.join("\n")}"
        @qtc.layout = layout.qtc
        @infused_layout = layout
      end
      if control
#         tag "addWidget #{control} to infused layout #{layout} + SETUP"
        control.parent = layout
        layout.add control, hash, &block
#         tag "add OK"
      end
#       tag "returning layout #{layout}"
      layout
    end

    def added control
      control.parent = self
    end

  public

    attr_accessor :infused_layout

    # array of all controls, widgets, layouts, menus, actions, models etc..
#     attr :autolayout

=begin rdoc
    connect a model (may be nil) to the frame.
    The rules are as follows:
        - if the frame has a connector and that name is also a public method of model,
          with no arguments (a getter therefore) and if model is not nil, then we
          apply the method and use the result to propagate to each control that
          is a direct child
        - if the frame has no name, or basically any other case then the first,
          it will take no action by itself,
          but the connect propagates to each control that is a direct child
=end
    def updateModel aModel, propagation
#       tag "#{self}::updateModel"
#       tag "connector=#{connector}(getter?:#{aModel.getter?(connector)}), aModel = #{aModel}"
      if (cid = connector) && aModel
#         tag "Applying getter #{cid} on model"
        mod = aModel.apply_getter(cid)
        applyModel mod, aModel
        aModel = mod
      end
#       tag "delegate model to children"
      propagateModel aModel, propagation
    end

    def columnCount value
#       tag "#{self}, value=#{value}, induce 'gridlayout'"
      check_layout(nil, :gridlayout).columnCount value
    end

    # override
    def effectiveModel
      return @model if instance_variable_defined?(:@model)
      aModel = parent.effectiveModel
      if (cid = connector) && aModel then aModel.apply_getter(cid) else aModel end
    end

    # override
    def effectiveModel?
      instance_variable_defined?(:@model)
    end

    # override
    def addWidget control, hash, &block
#       tag "#{self}, adding widget #{control}, autolayout=#@autolayout"
      if layout = infused_layout
#         tag "infused layout, add control to layout"
#         control.parent = layout               layout.added will do this
        layout.add(control, hash, &block)
      else
        if @autolayout && layoutcreator = control.auto_layouthint
#           tag "create proper layout, namely : #{layoutcreator}"
          check_layout(control, layoutcreator,  hash, &block)
        else
          super
        end
      end
    end

    def addLayout control, hash, &block
      if layout = infused_layout
        control.parent = layout
        layout.addLayout(control, hash, &block)
      else
#           tag "#{self.class}::addControl. SETTING layout of #@qtc to #{control.qtc}"
        # Qt says the same but it's only a warning
        super # @qtc.layout = control.qtc
      end
    end

    # override
    def postSetup
      super
      if instance_variable_defined?(:@infused_layout)
        @infused_layout.postSetup
        @infused_layout = nil
      end
    end
  end # class Frame

  createInstantiator File.basename(__FILE__, '.rb'), QWidget, Frame

end # Reform