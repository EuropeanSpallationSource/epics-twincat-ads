TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CXXFLAGS += -std=c++11
USR_CPPFLAGS += -I ../../../ADS/AdsLib/

#=============================
# Build the IOC application

PROD_IOC = ads
# ads.dbd will be created and installed
DBD += ads.dbd

# ads.dbd will be made up from these files:
ads_DBD += base.dbd
ads_DBD += adsController.dbd
# Include dbd files from all support applications:
#ads_DBD += xxx.dbd
ifdef AXIS
ads_DBD += axisSupport.dbd
ads_DBD += EthercatMCSupport.dbd
#ads_DBD += axis.dbd
endif
ads_DBD += asyn.dbd
ads_DBD += drvAsynIPPort.dbd

# Add all the support libraries needed by this IOC
ads_LIBS += asyn
ifdef AXIS
ads_LIBS += axis
ads_LIBS += EthercatMCSupport
endif
#ads_LIBS += AdsLib-Linux.o

# ads_registerRecordDeviceDriver.cpp derives from ads.dbd
# Add source files
ADSSOURCES = \
  AdsDef.cpp \
  AdsLib.cpp \
  AmsConnection.cpp \
  AmsPort.cpp \
  AmsRouter.cpp \
  Log.cpp \
  NotificationDispatcher.cpp \
  Sockets.cpp \
  Frame.cpp \


ads_SRCS += ads_registerRecordDeviceDriver.cpp
ads_SRCS += adsCom.cpp 
ads_SRCS += drvAsynAdsPort.cpp
ads_SRCS += cmd.c
ads_SRCS += cmd_EAT.c
ads_SRCS += ${ADSSOURCES}


# Build the main IOC entry point on workstation OSs.
ads_SRCS_DEFAULT += adsMain.cpp
ads_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#ads_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
ads_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


