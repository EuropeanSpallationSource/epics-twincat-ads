TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CXXFLAGS += -std=c++11
USR_CPPFLAGS += -I ../../../ADS/AdsLib/

#=============================
# Build the IOC application

PROD_IOC = adsExApp
# ads.dbd will be created and installed
DBD += adsExApp.dbd

# ads.dbd will be made up from these files:
adsExApp_DBD += base.dbd
adsExApp_DBD += adsController.dbd
# Include dbd files from all support applications:
#ads_DBD += xxx.dbd
ifdef MOTOR
adsExApp_DBD  += axisSupport.dbd
adsExApp_DBD  += EthercatMCSupport.dbd
adsExApp_LIBS += motor
adsExApp_LIBS += EthercatMCSupport
endif

adssEx_DBD += asyn.dbd
adssEx_DBD += drvAsynIPPort.dbd

# Build the main IOC entry point on workstation OSs.
adsExApp_SRCS_DEFAULT += adsMain.cpp
adsExApp_SRCS_vxWorks += -nil-

# Finally link to the EPICS Base libraries
adsExApp_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


