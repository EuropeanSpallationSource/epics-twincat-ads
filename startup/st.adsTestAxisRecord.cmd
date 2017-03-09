require asyn,4.27.0
require streamdevice
require axis,10.0.7
require ads,anderssandstrom

## Configure devices
drvAsynAdsPortConfigure("ADS_1","192.168.88.44","192.168.88.44.1.1",851,0, 0, 0)
#drvAsynAdsPortConfigure("ADS_2","192.168.88.41","192.168.88.41.1.1",852,0, 0, 0)
asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")

#asynSetTraceMask("ADS_1", -1, 0xFF)
##asynSetTraceMask("ADS_1", -1, 0x48)
asynSetTraceMask("ADS_1", -1, 0x41)

asynSetTraceIOMask("ADS_1", -1, 2)
asynSetTraceIOMask("ADS_1", -1, 6)

asynSetTraceInfoMask("ADS_1", -1, 15)

#IS THIS CORRECT TO USE ASYN PORT FROM ABOVE?
EthercatMCCreateController("MCU1", "ADS_1", "32", "200", "1000", "")
#-----------------------

############################################################

epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=IOC2:)")

#values for motor xl

epicsEnvSet("AXISCONFIG",    "")
epicsEnvSet("EGU",           "mm")
epicsEnvSet("PREC",          "3")
epicsEnvSet("VELO",          "360.0")
epicsEnvSet("JVEL",          "100")
#JAR defaults to VELO/ACCL
epicsEnvSet("JAR",           "0.0")
epicsEnvSet("ACCL",          "1")
epicsEnvSet("MRES",          "0.001")

epicsEnvSet("MOTOR_NAME",    "xl")
epicsEnvSet("R",             "xl-")
epicsEnvSet("DESC",          "Left Blade")
epicsEnvSet("AXIS_NO",       "1")
epicsEnvSet("DLLM",          "$(SM_DLLM=0)")
epicsEnvSet("DHLM",          "$(SM_DHLM=0)")
epicsEnvSet("HOMEPROC",      "$(SM_HOMEPROC=3)")

EthercatMCCreateAxis("MCU1", "${AXIS_NO}", "6", "")
dbLoadRecords("EthercatMC.template", "PREFIX=$(PREFIX), MOTOR_NAME=$(MOTOR_NAME), R=$(R), MOTOR_PORT=$(MOTOR_PORT), ASYN_PORT=$(ASYN_PORT), AXIS_NO=$(AXIS_NO), DESC=$(DESC), PREC=$(PREC), VELO=$(VELO), JVEL=$(JVEL), JAR=$(JAR), ACCL=$(ACCL), MRES=$(MRES), DLLM=$(DLLM), DHLM=$(DHLM), HOMEPROC=$(HOMEPROC)")

#Stream device
#epicsEnvSet "P" "$(P=I:)" 
#epicsEnvSet "R" "$(R=Test)" 

#General 
#dbLoadTemplate("adsGeneral.substitutions")

#One motion axis status	
#dbLoadTemplate("DUT_AxisStatus.substitutions")
#dbLoadTemplate("FB_DriveVirtual.substitutions")

#var streamDebug 1

