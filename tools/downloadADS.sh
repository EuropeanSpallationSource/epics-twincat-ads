#!/bin/sh
echo "$0 PWD=$PWD"

if ! test -e BeckhoffADS/AdsLib/AdsLib.h; then
(
  git submodule init BeckhoffADS && git submodule update BeckhoffADS		
)
fi
