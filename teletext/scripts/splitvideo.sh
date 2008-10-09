#!/bin/sh
if ! [ "$1" ]; then
  exit 1
fi
mplayer  -nosound "$1" -vo png:z=1
