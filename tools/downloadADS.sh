#!/bin/sh
ADSCOMMIT=35058e7ed6c2666af5ee6f390a170f32b11143
echo "$0 PWD=$PWD"

if test -d ADS; then
	echo "Note: ADS is already cloned"
else
	(
		git clone https://github.com/EuropeanSpallationSource/ADS.git ADS.tmp &&
 		cd ADS.tmp &&
		git checkout $ADSCOMMIT &&
		cd .. &&
		mv ADS.tmp/ ADS/
	)
fi
