#!/bin/sh
ADSCOMMIT=c64a050d880034435023bb095495c6f05c0fa18a
echo "$0 PWD=$PWD"

if test -d ADS; then
	echo "Note: ADS is already cloned"
else
	(
		git clone https://github.com/Beckhoff/ADS.git ADS.tmp &&
 		cd ADS.tmp &&
		git checkout $ADSCOMMIT &&
		cd .. &&
		mv ADS.tmp/ ADS/
	)
fi
