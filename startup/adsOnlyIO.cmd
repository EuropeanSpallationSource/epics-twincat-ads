require axis,10.1.5
require ads,anderssandstrom

##############################################################################
# Demo file to link an EPICS IOC with some I/O in a TwinCAT plc 
# 
#  1. Open TwinCAT test project
#  2. The ams adress of this linux client must be added to the TwinCAT ads router.
#     In TwinCAT: Systems->routes->add route, use ip of linux machine plus ".1.1"=> "192.168.88.44.1.1"
#  3. Download and start plc(s)
#  4. start ioc on linux machine with: iocsh adsOnlyIO.cmd 
#
##############################################################################
############# Configure ads device driver:
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
asynSetTraceMask("ADS_1", -1, 0x01)

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
