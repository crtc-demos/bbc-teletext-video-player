#!/bin/sh
set -e
xa play.a65 -o play
rm -rf dimg
mkdir dimg
cp track8k track8k.inf dimg
cp play play.inf dimg
cp outsqz outsqz.inf dimg
cp "!boot" "!boot.inf" dimg
../../adfs/adfs play.adl dimg
