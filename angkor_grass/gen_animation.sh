#!/bin/bash

magick -delay 12.5 -loop 0 'save/*.png' -set dispose background +repage save/8fps.gif
