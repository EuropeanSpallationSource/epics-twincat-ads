TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CXXFLAGS += -std=c++11
USR_CPPFLAGS += -I ../../../ADS/AdsLib/
USR_LDFLAGS  += -lpthread

#=============================
# Build the IOC application

LIBRARY_IOC = adsSupport

DBD += adsSupport.dbd

# Add source files
ADSSUPPORTSOURCES = \
  AdsDef.cpp \
  AdsLib.cpp \
  AmsConnection.cpp \
  AmsPort.cpp \
  AmsRouter.cpp \
  Log.cpp \
  NotificationDispatcher.cpp \
  Sockets.cpp \
  Frame.cpp


adsSupport_SRCS += adsAsynPortDriver.cpp
adsSupport_SRCS += adsAsynPortDriverUtils.cpp
adsSupport_SRCS += ${ADSSUPPORTSOURCES}

adsSupport_LIBS += asyn
adsSupport_LIBS += $(EPICS_BASE_IOC_LIBS)


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


