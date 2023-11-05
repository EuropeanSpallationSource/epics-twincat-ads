# Makefile when running gnu make
# If ESS EPICS ENVIRONMENT is set up, Makefile.EEE is used
# Otherwise use Makefile

ADS_FROM_BECKHOFF_SOURCES = \
  BeckhoffADS/AdsLib/AdsDef.cpp \
  BeckhoffADS/AdsLib/AdsLib.cpp \
  BeckhoffADS/AdsLib/AmsConnection.cpp \
  BeckhoffADS/AdsLib/AmsPort.cpp \
  BeckhoffADS/AdsLib/AmsRouter.cpp \
  BeckhoffADS/AdsLib/Log.cpp \
  BeckhoffADS/AdsLib/NotificationDispatcher.cpp \
  BeckhoffADS/AdsLib/Sockets.cpp \
  BeckhoffADS/AdsLib/Frame.cpp \



# download ADS if needed
build: adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak checkws

install: adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak checkws


checkws:
	./checkws.sh

adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak: Makefile $(ADS_FROM_BECKHOFF_SOURCES)
	${PWD}/tools/downloadADS.sh build ${ADS_FROM_BECKHOFF_SOURCES}


${ADS_FROM_BECKHOFF_SOURCES}:
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
