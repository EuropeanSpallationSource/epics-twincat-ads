TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = ads
# ads.dbd will be created and installed
DBD += ads.dbd

# ads.dbd will be made up from these files:
ads_DBD += base.dbd

# Include dbd files from all support applications:
#ads_DBD += xxx.dbd
ads_DBD += asyn.dbd
ads_DBD += drvAsynIPPort.dbd

# Add source files
ads_SRCS += drvAsynAdsPort.cpp
ads_SRCS += cmd.c

# Add all the support libraries needed by this IOC
ads_LIBS += asyn
ads_LIBS += AdsLib-Linux.o

# ads_registerRecordDeviceDriver.cpp derives from ads.dbd
ads_SRCS += ads_registerRecordDeviceDriver.cpp

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


