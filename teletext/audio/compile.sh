#!/bin/sh
set -e
xa aplay.a65 -o aplay
../../adfs/adfs aplay.adl .
