#!/bin/sh
set -e
# Creates audiodump.wav
mplayer -ao pcm -vo null "../Pitchshifter - Dead Battery.mp4"
sox audiodump.wav -c 1 -r 8000 --endian little -2 -u -t raw temp16.raw
./tab temp16.raw pitch8k
