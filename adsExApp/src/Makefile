TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================


#=============================
# Build the IOC application

PROD_IOC = adsExApp
# ads.dbd will be created and installed
DBD += adsExApp.dbd

# ads.dbd will be made up from these files:
adsExApp_DBD += base.dbd
adsExApp_DBD += asyn.dbd
adsExApp_DBD += adsSupport.dbd

# Build the main IOC entry point on workstation OSs.
adsExApp_SRCS += adsExApp_registerRecordDeviceDriver.cpp
adsExApp_SRCS_DEFAULT += adsMain.cpp
adsExApp_SRCS_vxWorks += -nil-

adsExApp_LIBS += asyn
adsExApp_LIBS += adsSupport
# Finally link to the EPICS Base libraries
adsExApp_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


