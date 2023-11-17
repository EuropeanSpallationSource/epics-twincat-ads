TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CXXFLAGS += -std=c++11
USR_CPPFLAGS += -I ../../../BeckhoffADS/AdsLib/
USR_LDFLAGS  += -lpthread

#=============================
# Build the IOC application

LIBRARY_IOC = ads

DBD += ads.dbd

# Add source files
-include $(TOP)/adsApp/src/ADS_FROM_BECKHOFF_SUPPORTSOURCES.mak

ads_SRCS += adsAsynPortDriver.cpp
ads_SRCS += adsAsynPortDriverUtils.cpp
ads_SRCS += ${ADS_FROM_BECKHOFF_SUPPORTSOURCES}

ads_LIBS += asyn
ads_LIBS += $(EPICS_BASE_IOC_LIBS)


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


