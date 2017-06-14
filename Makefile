# Makefile when running gnu make
# If ESS EPICS ENVIRONMENT is set up, Makefile.EEE is used
# Otherwise use Makefile

ADSSOURCES = \
  ADS/AdsLib/AdsDef.cpp \
  ADS/AdsLib/AdsLib.cpp \
  ADS/AdsLib/AmsConnection.cpp \
  ADS/AdsLib/AmsPort.cpp \
  ADS/AdsLib/AmsRouter.cpp \
  ADS/AdsLib/Log.cpp \
  ADS/AdsLib/NotificationDispatcher.cpp \
  ADS/AdsLib/Sockets.cpp \
  ADS/AdsLib/Frame.cpp \

# download ADS if needed
build: ${ADSSOURCES}

${ADSSOURCES}:
	${PWD}/tools/downloadADS.sh


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

