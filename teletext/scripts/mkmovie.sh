#!/bin/sh
mencoder "mf://*.png" -mf fps=29.97 -o output.avi -ovc lavc -lavcopts vcodec=mpeg4
