#!/bin/sh
set -e
# Creates audiodump.wav
mplayer -ao pcm -vo null tron.wmv
sox audiodump.wav -c 1 -r 12500 --endian little -2 -u -t raw temp16.raw
./tab temp16.raw noisebits
