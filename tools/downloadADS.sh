#!/bin/sh
#echo "$0 PWD=$PWD" "$@"

if ! test -e BeckhoffADS/AdsLib/AdsLib.h; then
(
  git submodule init BeckhoffADS && git submodule update BeckhoffADS
)
fi
action=$1
shift
case "$action" in
clean)
  for src in "$@"; do
    dst=${src##*/}
    rm -f adsApp/src/$dst
done
;;
build)
  >adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak
  for src in "$@"; do
    dst=${src##*/}
		rm -f adsApp/src/$dst
    cp -v $src adsApp/src/$dst
    echo "ADS_FROM_BECKHOFF_SUPPORTSOURCES += $dst" >>adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak
  done
  ;;
*)
  echo >& "$0 [clean|build] <filenames>"
  exit 1
esac

