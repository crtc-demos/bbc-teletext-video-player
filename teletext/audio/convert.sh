#!/bin/sh
set -e
# Creates audiodump.wav
#mplayer -ao pcm -vo null "/mnt/bitbucket/jules/MP3s/Dave's Stuff/03 track3.mp3"
sox audiodump.wav --no-dither -c 1 -r 10000 --endian little -2 -u -t raw temp16.raw
./tab temp16.raw track10k
