require ads,develop
require stream, 2.8.10
require EthercatMC 3.0.0

##############################################################################
# Demo file to run one motor record axis (or actually axis record). 
# 
#  1. Open TwinCAT test project
#  2. The ams address of this linux client must be added to the TwinCAT ads router.
#     In TwinCAT: Systems->routes->add route, use ip of linux machine plus ".1.1"=> "192.168.88.44.1.1"
#  3. Link axis 1 to hardware and make commisioning.
#  4. Ensure that NC axis 1 is linked to Main.M1Link.Axis
#  5. To run motor both limitswitches need to be linked to switeches or set to 1. (Main.bLimitFwd=1,Main.bLimitBwd=1)
#  6. Download and start plc(s)
#  7. start ioc on linux machine with: iocsh adsMotorRecord.cmd 
#
##############################################################################
############# Configure ads device driver:
# 1. Asyn port name                          :  "ADS_1"
# 2. IP                                      :  "192.168.88.10"
# 3. AMS of plc                              :  "192.168.88.11.1.2"
# 4. Default ams port                        :  851 for plc 1, 852 plc 2 ...
# 5. Parameter table size (max parameters)   :  1000
# 6. priority                                :  0
# 7. disable auto connnect                   :  0 (autoconnect enabled)
# 8. default sample time ms                  :  500
# 9. max delay time ms (buffer time in plc)  :  1000
# 10. ADS command timeout in ms              :  1000  
# 11. default time source (PLC=0,EPICS=1)    :  0 (PLC) NOTE: record TSE field need to be set to -2 for timestamp in asyn ("field(TSE, -2)")
adsAsynPortDriverConfigure("ADS_1","192.168.88.44","192.168.88.44.1.1",851,1000,0,0,50,100,1000,0)

epicsEnvSet(STREAM_PROTOCOL_PATH, ${ads_DB}

asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")
asynSetTraceMask("ADS_1", -1, 0x41)

##############################################################################
############# Configure and load axis record:
EthercatMCCreateController("MCU1", "ADS_1", "32", "200", "1000", "")

epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=ADS_IOC:)")

epicsEnvSet("AXISCONFIG",    "")
epicsEnvSet("EGU",           "mm")
epicsEnvSet("PREC",          "3")
epicsEnvSet("VELO",          "360.0")
epicsEnvSet("JVEL",          "100")
#JAR defaults to VELO/ACCL
epicsEnvSet("JAR",           "0.0")
epicsEnvSet("ACCL",          "1")
epicsEnvSet("MRES",          "0.001")

epicsEnvSet("MOTOR_NAME",    "M1")
epicsEnvSet("R",             "M1-")
epicsEnvSet("DESC",          "Motor 1")
epicsEnvSet("AXIS_NO",       "1")
epicsEnvSet("DLLM",          "$(SM_DLLM=0)")
epicsEnvSet("DHLM",          "$(SM_DHLM=0)")
epicsEnvSet("HOMEPROC",      "$(SM_HOMEPROC=3)")

EthercatMCCreateAxis("MCU1", "${AXIS_NO}", "6", "")
dbLoadRecords("EthercatMC.template", "PREFIX=$(PREFIX), MOTOR_NAME=$(MOTOR_NAME), R=$(R), MOTOR_PORT=$(MOTOR_PORT), ASYN_PORT=$(ASYN_PORT), AXIS_NO=$(AXIS_NO), DESC=$(DESC), PREC=$(PREC), VELO=$(VELO), JVEL=$(JVEL), JAR=$(JAR), ACCL=$(ACCL), MRES=$(MRES), DLLM=$(DLLM), DHLM=$(DHLM), HOMEPROC=$(HOMEPROC)")

##############################################################################
############# Load records Octet interface (Stream device):
dbLoadRecords("adsTestOctet.db","P=ADS_IOC:OCTET:,PORT=ADS_1")

##############################################################################
############# Load records (asyn direct I/O intr):
dbLoadRecords("adsTestAsyn.db","P=ADS_IOC:ASYN:,PORT=ADS_1")

##############################################################################
############# Motor/Axis record error message:
#
# Note: Motor/Axis record will try to read Main.M1.stAxisStatusV2 and use it if accessible. Otherwise fallback on original version ("Main.M1.stAxisStatus"). 
# The following error message will be displayed at startup if "Main.M1.stAxisStatusV2 "is not accessible (this error will not impact the driver):
#  "adsAsynPortDriver:adsGetSymInfoByName: Get symbolic information failed for Main.M1.stAxisStatusV2 with: ADSERR_DEVICE_SYMBOLNOTFOUND (1808)"
#  "adsApp/src/adsAsynPortDriver.cpp/octetCmdHandleInputLine:1237 motorHandleOneArg returned errorcode: 0x3"
#
##############################################################################
############# Usefull commands
#var streamDebug 1
#asynReport(2,"ADS_1")
#asynSetTraceMask("ADS_1", -1, 0xFF)
