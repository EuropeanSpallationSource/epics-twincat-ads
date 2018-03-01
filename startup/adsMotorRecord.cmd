require axis,10.1.5
require ads,anderssandstrom

##############################################################################
# Demo file to run one axis motor record (or actually axis record). 
# 
# 1. The ams adress of this linux client must be added to the TwinCAT ads router.
#    Systems->routes->add route, use ip of linux plus ".1.1"=> "192.168.88.44.1.1"
##############################################################################
## Configure devices
# 1. Asyn port name                         : "ADS_1"
# 2. IP                                     : "192.168.88.10"
# 3. AMS of plc                             : "192.168.88.11.1.2"
# 4. Default ams port                       : 851 for plc 1, 852 plc 2 ...
# 5. Parameter table size (max parameters)  : 1000 example
# 6. priority                               : 0
# 7. disable auto connnect                  : 0 (autoconnect enabled)
# 8. noProcessEOS                           : 0
# 9. default sample time ms                 : 500
# 10. max delay time ms (buffer time in plc): 1000
# 11. ADS command timeout in ms             : 1000  
# 12. default time source (PLC=0,EPICS=1).  : 0 (PLC) NOTE: record TSE field need to be set to -2 for timestamp in asyn ("field(TSE, -2)")

adsAsynPortDriverConfigure("ADS_1","192.168.88.44","192.168.88.44.1.1",851,1000, 0, 0,0,50,100,1000,0)

asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")

asynSetTraceMask("ADS_1", -1, 0xFF)

##############################################################################
############# Configure and load axis record:

#IS THIS CORRECT TO USE ASYN PORT FROM ABOVE??? NEED to test
EthercatMCCreateController("MCU1", "ADS_1", "32", "200", "1000", "")

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

##############################################################################
############# Load records (Stream device):
#dbLoadRecords("adsTest.db","P=ADS_IOC:,PORT=ADS_1")

##############################################################################
############# Load records (asyn direct):
#dbLoadRecords("adsTestAsynSlim.db","P=ADS_IOC:,PORT=ADS_1")

#var streamDebug 1

#asynReport(2,"ADS_1")
