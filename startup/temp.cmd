require ads,anderssandstrom
require streamdevice,2.7.7
require axis,10.1.5

##############################################################################
# Demo file to run one axis motor record (or actually axis record). 
# 
# 1. The ams adress of this linux client must be added to the TwinCAT ads router.
#    Systems->routes->add route, use ip of linux plus ".1.1"=> "192.168.88.44.1.1"
# 2. A PLC project with an instance of FB_DriveVirtual, (in "Main.M1"), must be 
#    loaded in the PLC. See demo twincat project. 
# 3. Start with: iocsh st.adsTestAxisRecord.cmd
# 
##############################################################################
############# Configure device (<ASYN PORT>, <IP_of_PLC>,<AMS_of_PLC>,<Default_ADS_Port>,<Not_used>,<Not_used>,<Not_used>):
## Configure devices
#drvAsynAdsPortConfigure("ADS_1","192.168.88.60","5.40.216.206.1.1",851,0, 0,0)
adsAsynPortDriverConfigure("ADS_1","192.168.88.60","5.40.216.206.1.1",851,1000, 0, 0,0)

asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")

asynSetTraceMask("ADS_1", -1, 0x41)
asynSetTraceIOMask("ADS_1", -1, 0xFF)
asynSetTraceInfoMask("ADS_1", -1, 15)

#############################################################################
############# Load records (Stream device):
#dbLoadRecords("adsTest.db","P=ADS_IOC:,PORT=ADS_1")
dbLoadRecords("adsTestAsyn.db","P=ADS_IOC:,PORT=ADS_1")

#var streamDebug 1

