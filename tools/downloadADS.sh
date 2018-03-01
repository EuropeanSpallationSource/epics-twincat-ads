#!/bin/sh
echo "$0 PWD=$PWD"

if ! test -e ADS/AdsLib/AdsLib.h; then
(
  git submodule init ADS && git submodule update ADS		
)
fi
