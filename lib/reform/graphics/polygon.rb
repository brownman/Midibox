
# Copyright (c) 2010 Eugene Brazwick

module Reform

  require_relative '../graphicsitem'

  class Polygon < GraphicsItem
  private

    # every element should be a tuple.
    def points *pts
#       tag "pts=#{pts.inspect}"
      poly = Qt::PolygonF.new(pts.map {|x, y| Qt::PointF.new(x, y) })
#       for x, y in pts do
#         p = Qt::PointF.new(x, y)
#         tag "p=#{p.inspect}"
#         poly.append p
#       end
#       tag "#poly = #{poly.count}"           # STACK OVERFLOW?
      @qtc.polygon = poly
    end

  public

#     def self.new_qt_implementor(qt_implementor_class, parent, qparent)
#       poly = qt_implementor_class.new(qparent)
#       poly.pen, poly.brush = parent.pen, parent.brush
#       poly
#     end

    # @qtc does NOT implement 'objectName'!!! This is rather onfortunate
    def name aName = nil
      if aName
        @objectName = aName.to_s
        parent.registerName aName, self
      else
        @objectName
      end
    end


  end # Polygon

  createInstantiator File.basename(__FILE__, '.rb'), Qt::GraphicsPolygonItem, Polygon

end # Reform