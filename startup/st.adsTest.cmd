require asyn
require streamdevice
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

#-----------------------

############################################################


#Stream device
#epicsEnvSet "P" "$(P=I:)" 
#epicsEnvSet "R" "$(R=Test)" 

#General 
dbLoadTemplate("adsGeneral.substitutions")

#One motion axis status	
dbLoadTemplate("DUT_AxisStatus.substitutions")
dbLoadTemplate("FB_DriveVirtual.substitutions")

#var streamDebug 1

