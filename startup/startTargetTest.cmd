require ads,dev

##############################################################################
############# Configure ads device driver:
# 1.  Asyn port name                         :  "TARGET_ADS"
# 2.  IP                                     :  "192.168.88.200"
# 3.  AMS of plc                             :  "5.46.8.38.1.1"
# 4.  Default ams port                       :  851 for plc 1, 852 plc 2 ...
# 5.  Parameter table size (max parameters)  :  1000
# 6.  priority                               :  0
# 7.  disable auto connnect                  :  0 (autoconnect enabled)
# 8.  default sample time ms                 :  500
# 9.  max delay time ms (buffer time in plc) :  1000
# 10. ADS command timeout in ms              :  1000  
# 11. default time source (PLC=0,EPICS=1)    :  0 (PLC) NOTE: record TSE field need to be set to -2 for timestamp in asyn ("field(TSE, -2)")

adsAsynPortDriverConfigure("TARGET_ADS","192.168.88.200","5.46.8.38.1.1",852,1000,0,0,50,100,5000,0)

asynOctetSetOutputEos("TARGET_ADS", -1, "\n")
asynOctetSetInputEos("TARGET_ADS", -1, "\n")
asynSetTraceMask("TARGET_ADS", -1, 0x41)

##############################################################################
############# Load records (asyn direct I/O intr):
dbLoadRecords("target.db","P=ADS_IOC:ASYN:,PORT=TARGET_ADS")

##############################################################################
############# Usefull commands
#var streamDebug 1
#asynReport(2,"TARGET_ADS")
#asynSetTraceMask("TARGET_ADS", -1, 0xFF)
