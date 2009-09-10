#!/bin/sh
OUT=$1
CONTRAST=$2
if ! [ "$OUT" ]; then
  echo "Output filename? (xxx.asc)"
  exit 1
fi
if ! [ "$CONTRAST" ]; then
  CONTRAST=15
fi
echo "Using contrast boost of $CONTRAST%"
for x in video/*; do
  echo Converting "$x"
#  convert "$x" -resize "480x500!" -emboss 2 tmp1.png
  convert "$x" -resize "480x500!" tmp1.png
  ./teletext -x -o tmp2.bmp -a "$OUT" tmp1.png
#  convert tmp2.bmp $(echo "$x"|sed s:video/:video2/:)
  rm -f tmp1.png tmp2.bmp
done

