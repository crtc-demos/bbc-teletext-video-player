#!/bin/sh
IN=$1
CONTRAST=$2
if ! [ "$CONTRAST" ]; then
  CONTRAST=15
fi
echo "Using contrast boost of $CONTRAST%"
echo Converting "$IN"
#  convert "$IN" -resize "480x500!" -emboss 2 tmp1.png
#convert "$IN" -resize "480x500!" -contrast-stretch $CONTRAST% tmp1.png
convert "$IN" -resize "480x500!" tmp1.png
./teletext -o tmp2.bmp tmp1.png
#convert tmp2.bmp $(echo "$x"|sed s:video/:video2/:)
rm -f tmp1.png tmp2.bmp
