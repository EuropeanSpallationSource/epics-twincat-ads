# Makefile when running gnu make
# If ESS EPICS ENVIRONMENT is set up, Makefile.EEE is used
# Otherwise use Makefile

ADS_FROM_BECKHOFF_SOURCES = \
  BeckhoffADS/AdsLib/AdsDef.cpp \
  BeckhoffADS/AdsLib/AdsDevice.cpp \
  BeckhoffADS/AdsLib/AdsFile.cpp \
  BeckhoffADS/AdsLib/standalone/AdsLib.cpp \
  BeckhoffADS/AdsLib/standalone/AmsConnection.cpp \
  BeckhoffADS/AdsLib/standalone/AmsNetId.cpp \
  BeckhoffADS/AdsLib/standalone/AmsPort.cpp \
  BeckhoffADS/AdsLib/standalone/AmsRouter.cpp \
  BeckhoffADS/AdsLib/Frame.cpp \
  BeckhoffADS/AdsLib/LicenseAccess.cpp \
  BeckhoffADS/AdsLib/Log.cpp \
  BeckhoffADS/AdsLib/standalone/NotificationDispatcher.cpp \
  BeckhoffADS/AdsLib/RegistryAccess.cpp \
  BeckhoffADS/AdsLib/RouterAccess.cpp \
  BeckhoffADS/AdsLib/RTimeAccess.cpp \
  BeckhoffADS/AdsLib/Sockets.cpp \
  BeckhoffADS/AdsLib/SymbolAccess.cpp \



# download ADS if needed
build: adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak checkws

install: adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak checkws

clean: cleanadssources

checkws:
	./checkws.sh

cleanadssources:
	${PWD}/tools/downloadADS.sh clean ${ADS_FROM_BECKHOFF_SOURCES}


adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak: Makefile $(ADS_FROM_BECKHOFF_SOURCES)
	${PWD}/tools/downloadADS.sh build ${ADS_FROM_BECKHOFF_SOURCES}

ifdef EPICS_ENV_PATH
ifeq ($(EPICS_MODULES_PATH),/opt/epics/modules)
ifeq ($(EPICS_BASES_PATH),/opt/epics/bases)
include Makefile.EEE
else
include Makefile.epics
endif
else
include Makefile.epics
endif
else
include Makefile.epics
endif

.PHONY: checkws
