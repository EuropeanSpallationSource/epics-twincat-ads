EXCLUDE_VERSIONS=3.14.12.5

PROJECT=ads

include ${EPICS_ENV_PATH}/module.Makefile

USR_DEPENDENCIES = asyn,4.31.0


USR_CXXFLAGS += -std=c++11

# Temporally removed to speed up 
EXCLUDE_ARCHS += eldk

SOURCES = \
  adsApp/src/drvAsynAdsPort.cpp \
  adsApp/src/adsCom.cpp \
  adsApp/src/cmd.c \
  adsApp/src/cmd_EAT.c \
  adsApp/src/AdsDef.cpp \
  adsApp/src/AdsLib.cpp \
  adsApp/src/AmsConnection.cpp \
  adsApp/src/AmsPort.cpp \
  adsApp/src/AmsRouter.cpp \
  adsApp/src/Frame.cpp \
  adsApp/src/Log.cpp \
  adsApp/src/NotificationDispatcher.cpp \
  adsApp/src/Sockets.cpp \


TEMPLATES = \
  adsApp/Db/adsGeneral.template \
  adsApp/Db/DUT_AxisStatus_v0_01.template\
  adsApp/Db/FB_DriveVirtual_v1_01.template\
  adsApp/Db/adsTest.db\



