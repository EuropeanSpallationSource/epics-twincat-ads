TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CXXFLAGS += -std=c++11
USR_CPPFLAGS += -I ../../../ADS/AdsLib/

#=============================
# Build the IOC application

LIBRARY_IOC = adsSupport
# ads.dbd will be created and installed
DBD += adsSupport.dbd

adsSupport_LIBS += asyn

# ads_registerRecordDeviceDriver.cpp derives from ads.dbd
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
  Frame.cpp \


ads_SupportSRCS += ads_registerRecordDeviceDriver.cpp
ads_SupportSRCS += adsCom.cpp 
ads_SupportSRCS += drvAsynAdsPort.cpp
ads_SupportSRCS += cmd.c
ads_SupportSRCS += cmd_EAT.c

ads_SRCS += ${ADSSUPPORTSOURCES}

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


