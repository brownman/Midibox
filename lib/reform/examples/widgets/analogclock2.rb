#!/usr/bin/ruby

require_relative '../../app'

Reform::app {
  canvas {
    scene {
      area -100.0, -100.0, 100.0, 100.0 # topleft - rightbottom
      line { stroke red
        from -100, 0
        to 100, 0
      }
      line { stroke 'steelblue'
        from 0, -100
        to 0, 100
      }
      fill red # changes the default fill of all contained object, even before this
          # declaration (or at least, that is supposed to happen)
          # It is not the 'current' fill, but the 'default' fill.
          # otherwise there would be the temptation to use it in a procedural way;
          # fill red; circle {...} ; fill blue; circle {...};
          # Must be: circle {fill red; ...}; circle {fill blue; ...};
#       tag "calling Scene::duplicate()"
# trace do
      circle { position 0, 0; radius 20; fill blue }
      replicate {
        rotation 20 # degrees, rotation around 0,0
        translation 10.0, 0
        fillhue_rotation 0.25 # 90 degrees
        count 10
        # the resulting matrices are now applied count times on the contained objects
        circle {
          position -50, -50
          radius 25
          # setting a fill here would override the fillhue in duplicate and the fill in scene.
        }
        # it should be possible to nest duplicates properly.
      }
# end
    } # scene
    #antialiasing true   this is the default
    # autofullscale: TODO. View must rescale on resize to the area is in full view (but keep aspectratio)
    title tr('Analog Clock')
    size 400, 400
  }
}