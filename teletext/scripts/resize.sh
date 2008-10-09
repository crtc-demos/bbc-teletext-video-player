#!/bin/sh
for x in video/*; do
  echo Converting "$x"
  convert "$x" -resize "480x480!" tmp1.png
  ./teletext -x -o tmp2.bmp -a frames.asc tmp1.png
  convert tmp2.bmp $(echo "$x"|sed s:video/:video2/:)
  rm -f tmp1.png tmp2.bmp
done

