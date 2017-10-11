#!/bin/sh
ADSCOMMIT=35058e7ed6c2666af5ee6f390a170f32b11143
echo "$0 PWD=$PWD"

if test -d ADS; then
  echo "Note: ADS is already cloned"
else
  (
    git submodule update
    (
        cd ADS &&
        (git remote -v | grep Beckhoff) ||
        git remote add Beckhoff https://github.com/Beckhoff/ADS.git
    )
  )
fi
