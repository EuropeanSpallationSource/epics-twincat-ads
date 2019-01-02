# Makefile when running gnu make
# If ESS EPICS ENVIRONMENT is set up, Makefile.EEE is used
# Otherwise use Makefile

ADSSOURCES = \
  BeckhoffADS/AdsLib/AdsDef.cpp \
  BeckhoffADS/AdsLib/AdsLib.cpp \
  BeckhoffADS/AdsLib/AmsConnection.cpp \
  BeckhoffADS/AdsLib/AmsPort.cpp \
  BeckhoffADS/AdsLib/AmsRouter.cpp \
  BeckhoffADS/AdsLib/Log.cpp \
  BeckhoffADS/AdsLib/NotificationDispatcher.cpp \
  BeckhoffADS/AdsLib/Sockets.cpp \
  BeckhoffADS/AdsLib/Frame.cpp \
  BeckhoffADS/AdsLib/AdsLib.h

# download ADS if needed
build: ${ADSSOURCES} checkws

install: ${ADSSOURCES} checkws

checkws:
	./checkws.sh

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

.PHONY: checkws
