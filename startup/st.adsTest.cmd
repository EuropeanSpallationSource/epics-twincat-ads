
require asyn,4.27.0
require streamdevice,2.7.1
require ads,anderssandstrom

##############################################################################
# Demo file to communicate with TwinCAT over ADS 
# 
# 1. The ams adress of this linux client must be added to the TwinCAT ads router.
#    Systems->routes->add route, use ip of linux plus ".1.1"=> "192.168.88.44.1.1"
# 2. A PLC project with certain variables needs to be executing (see demo project)
# 3. Start with: iocsh st.adsTest.cmd
# 
##############################################################################
############# Configure device (<ASYN PORT>, <IP_of_PLC>,<AMS_of_PLC>,<Default_ADS_Port>,<Not_used>,<Not_used>,<Not_used>):

drvAsynAdsPortConfigure("ADS_1","192.168.88.44","192.168.88.44.1.1",851,0, 0, 0)
asynOctetSetOutputEos("ADS_1", -1, "\n")
asynOctetSetInputEos("ADS_1", -1, "\n")
asynSetTraceMask("ADS_1", -1, 0x41)
asynSetTraceIOMask("ADS_1", -1, 6)
asynSetTraceInfoMask("ADS_1", -1, 15)

##############################################################################
############# Load records (Stream device):

#General 
dbLoadRecords("adsTest.db","P=ADS_IOC:,PORT=ADS_1")

#var streamDebug 1

