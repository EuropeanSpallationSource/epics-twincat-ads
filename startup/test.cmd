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
# 12. default time source (PLC=0,EPICS=0).  : 0 (PLC) NOTE: record TSE field need to be set to -2 for timestamp in asyn ("field(TSE, -2)")

adsAsynPortDriverConfigure("ADS_1","192.168.88.44","192.168.88.44.1.1",851,1000, 0, 0,0,50,100,1000,0)

asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")


asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")

asynSetTraceMask("ADS_1", -1, 0x01)

#############################################################################

dbLoadRecords("adsTestAsyn.db","P=ADS_IOC:,PORT=ADS_1")

#asynReport(2,"ADS_1")

