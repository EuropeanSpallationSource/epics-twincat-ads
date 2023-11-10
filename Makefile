# Makefile when running gnu make
# If ESS EPICS ENVIRONMENT is set up, Makefile.EEE is used
# Otherwise use Makefile

ADS_FROM_BECKHOFF_SOURCES = \
  ADS/AdsLib/AdsDef.cpp \
  ADS/AdsLib/AdsDevice.cpp \
  ADS/AdsLib/AdsFile.cpp \
  ADS/AdsLib/standalone/AdsLib.cpp \
  ADS/AdsLib/standalone/AmsConnection.cpp \
  ADS/AdsLib/standalone/AmsNetId.cpp \
  ADS/AdsLib/standalone/AmsPort.cpp \
  ADS/AdsLib/standalone/AmsRouter.cpp \
  ADS/AdsLib/Frame.cpp \
  ADS/AdsLib/LicenseAccess.cpp \
  ADS/AdsLib/Log.cpp \
  ADS/AdsLib/standalone/NotificationDispatcher.cpp \
  ADS/AdsLib/RegistryAccess.cpp \
  ADS/AdsLib/RouterAccess.cpp \
  ADS/AdsLib/RTimeAccess.cpp \
  ADS/AdsLib/Sockets.cpp \
  ADS/AdsLib/SymbolAccess.cpp \



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
