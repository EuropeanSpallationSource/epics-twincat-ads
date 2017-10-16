#!/bin/sh
echo "$0 PWD=$PWD"

if ! test -e ADS/AdsLib/AdsLib.h; then
(
	git rm --cached ADS || :
	git submodule deinit ADS || :
	mv ADS ADS.$$ || :
	git submodule add -f https://github.com/Beckhoff/ADS.git ADS || :
	cd ADS &&
	(
			git reset --hard || :
	)
	git add ADS
)
fi
