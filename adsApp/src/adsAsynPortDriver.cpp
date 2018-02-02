/*
 */

#include "adsAsynPortDriver.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "cmd.h"

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <epicsExport.h>
#include <dbStaticLib.h>
#include <dbAccess.h>

#include "adsCom.h"

static void adsNotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
  const uint8_t* data = reinterpret_cast<const uint8_t*>(pNotification + 1);
  printf("hUser 0x%x", hUser);
  printf(" sample time: %ld", pNotification->nTimeStamp);
  printf(" sample size: %d", pNotification->cbSampleSize);
  printf(" value: ");
  for (size_t i = 0; i < pNotification->cbSampleSize; ++i) {
    printf(" 0x%x",(int)data[i]);
  }
}

static const char *driverName="adsAsynPortDriver";

// Constructor for the adsAsynPortDriver class.
adsAsynPortDriver::adsAsynPortDriver(const char *portName,
                                     const char *ipaddr,
                                     const char *amsaddr,
                                     unsigned int amsport,
                                     int paramTableSize,
                                     unsigned int priority,
                                     int autoConnect,
                                     int noProcessEos)
                    :asynPortDriver(portName,
                                   1, /* maxAddr */
                                   paramTableSize,
                                   asynInt32Mask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynEnumMask | asynDrvUserMask | asynOctetMask | asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask, /* Interface mask */
                                   asynInt32Mask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynEnumMask | asynDrvUserMask | asynOctetMask | asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask,  /* Interrupt mask */
                                   ASYN_CANBLOCK, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                                   autoConnect, /* Autoconnect */
                                   priority, /* Default priority */
                                   0) /* Default stack size*/
{
  const char* functionName = "adsAsynPortDriver";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  eventId_ = epicsEventCreate(epicsEventEmpty);
  pAdsParamArray_= new adsParamInfo*[paramTableSize];
  memset(pAdsParamArray_,0,sizeof(*pAdsParamArray_));
  pAdsParamArrayCount_=0;
  paramTableSize_=paramTableSize;
  portName_=portName;
  ipaddr_=ipaddr;
  amsaddr_=amsaddr;
  amsport_=amsport;
  priority_=priority;
  autoConnect_=autoConnect;
  noProcessEos_=noProcessEos;

  //ADS
  adsPort_=0; //handle
  remoteNetId_={0,0,0,0,0,0};

  if(amsport_<=0){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Invalid default AMS port: %d\n", driverName, functionName,amsport_);
  }

  int nvals = sscanf(amsaddr_, "%hhu.%hhu.%hhu.%hhu.%hhu.%hhu",
                     &remoteNetId_.b[0],
                     &remoteNetId_.b[1],
                     &remoteNetId_.b[2],
                     &remoteNetId_.b[3],
                     &remoteNetId_.b[4],
                     &remoteNetId_.b[5]);
  if (nvals != 6) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Invalid AMS address: %s\n", driverName, functionName,amsaddr_);
  }
}

adsAsynPortDriver::~adsAsynPortDriver()
{
  const char* functionName = "~adsAsynPortDriver";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  for(int i=0;i<pAdsParamArrayCount_;i++){
    adsDelNotificationCallback(pAdsParamArray_[i]);

    free(pAdsParamArray_[i]->recordName);
    free(pAdsParamArray_[i]->recordType);
    free(pAdsParamArray_[i]->scan);
    free(pAdsParamArray_[i]->dtyp);
    free(pAdsParamArray_[i]->inp);
    free(pAdsParamArray_[i]->out);
    free(pAdsParamArray_[i]->drvInfo);
    free(pAdsParamArray_[i]->plcSymAdr);
    delete pAdsParamArray_[i];
  }
  delete pAdsParamArray_;
}

void adsAsynPortDriver::report(FILE *fp, int details)
{
  const char* functionName = "report";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if (details >= 1) {
    fprintf(fp, "    Port %s\n",portName);
  }
}

asynStatus adsAsynPortDriver::disconnect(asynUser *pasynUser)
{
  const char* functionName = "disconnect";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

//  #if 0
    /* TODO: prevent a reconnect */
//    if (!(tty->flags & FLAG_CONNECT_PER_TRANSACTION) ||
//        (tty->flags & FLAG_SHUTDOWN))
//      pasynManager->exceptionDisconnect(pasynUser);
//  #endif

  asynStatus disconnectStatus=adsDisconnect();
  if (disconnectStatus){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: Disconnect failed..\n", driverName, functionName);
    return asynError;
  }
  return asynSuccess;
}


asynStatus adsAsynPortDriver::connect(asynUser *pasynUser)
{
  const char* functionName = "connect";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  lock();
  asynStatus status = adsConnect();
  unlock();

  if (status == asynSuccess){
    //pasynManager->exceptionConnect(pasynUser);
    asynPortDriver::connect(pasynUser);
  }

  return status;
}

asynStatus adsAsynPortDriver::drvUserCreate(asynUser *pasynUser,const char *drvInfo,const char **pptypeName,size_t *psize)
{
  const char* functionName = "drvUserCreate";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynPortDriver::drvUserCreate(pasynUser,drvInfo,pptypeName,psize);
  //dbAddr  paddr;
  //dbNameToAddr("ADS_IOC::GetFTest3",&paddr);

  //asynPrint(pasynUser, ASYN_TRACE_INFO,"####################################\n");
  //asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: drvInfo=%s.\n", driverName, functionName,drvInfo);
  //const char *data;
  //size_t writeSize=1024;
  //asynStatus status=drvUserGetType(pasynUser,&data,&writeSize);
  //asynPrint(pasynUser, ASYN_TRACE_INFO,"STATUS = %d, data= %s, size=%d\n",status,data,writeSize);

  int index=0;
  asynStatus status=findParam(drvInfo,&index);
  if(status==asynSuccess){
    asynPrint(pasynUser, ASYN_TRACE_INFO, "PARAMETER INDEX FOUND AT: %d for %s. \n",index,drvInfo);
  }
  else{
   // Collect data from drvInfo string and record
    adsParamInfo *paramInfo=new adsParamInfo();
    memset(paramInfo,0,sizeof(adsParamInfo));
    status=getRecordInfoFromDrvInfo(drvInfo, paramInfo);
    if(status==asynSuccess){
      status=createParam(drvInfo,paramInfo->asynType,&index);
      if(status==asynSuccess){
        paramInfo->paramIndex=index;
        status=parsePlcInfofromDrvInfo(drvInfo,paramInfo);
        if(status==asynSuccess){
          pAdsParamArray_[pAdsParamArrayCount_]=paramInfo;
          pAdsParamArrayCount_++;
          //print all parameters
          for(int i=0; i<pAdsParamArrayCount_;i++){
            printParamInfo(pAdsParamArray_[i]);
          }
          asynPrint(pasynUser, ASYN_TRACE_INFO, "PARAMETER CREATED AT: %d for %s.\n",index,drvInfo);
        }
        else{
          asynPrint(pasynUser, ASYN_TRACE_ERROR, "FAILED PARSING OF DRVINFO: %s.\n",drvInfo);
        }
      }
      else{
        asynPrint(pasynUser, ASYN_TRACE_ERROR, "CREATE PARAMETER FAILED for %s.\n",drvInfo);
      }
    }
  }

  //asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->errorMessage=%s.\n",pasynUser->errorMessage);
  //asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->timeout=%lf.\n",pasynUser->timeout);
  //asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->reason=%d.\n",pasynUser->reason);
  //asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->auxStatus=%d.\n",pasynUser->auxStatus);
  //asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->alarmStatus=%d.\n",pasynUser->alarmStatus);
  //asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->alarmSeverity=%d.\n",pasynUser->alarmSeverity);
  //dbDumpRecords();
  //return this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize,pParamTable_, paramTableSize_);
  //pasynManager->report(stdout,1,"ADS_1");

  return asynSuccess;
}

asynParamType adsAsynPortDriver::dtypStringToAsynType(char *dtype)
{
  const char* functionName = "dtypStringToAsynType";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if(strcmp("asynFloat64",dtype)==0){
    return asynParamFloat64;
  }
  if(strcmp("asynInt32",dtype)==0){
    return asynParamInt32;
  }
  return asynParamNotDefined;

  //  asynParamUInt32Digital,
  //  asynParamOctet,
  //  asynParamInt8Array,
  //  asynParamInt16Array,
  //  asynParamInt32Array,
  //  asynParamFloat32Array,
  //  asynParamFloat64Array,
  //  asynParamGenericPointer
}

asynStatus adsAsynPortDriver::getRecordInfoFromDrvInfo(const char *drvInfo,adsParamInfo *paramInfo)
{
  const char* functionName = "getRecordInfoFromDrvInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  paramInfo->amsPort=amsport_;
  DBENTRY *pdbentry;
  pdbentry = dbAllocEntry(pdbbase);
  long status = dbFirstRecordType(pdbentry);
  bool recordFound=false;
  if(status) {
    dbFreeEntry(pdbentry);
    return asynError;
  }
  while(!status) {
    paramInfo->recordType=strdup(dbGetRecordTypeName(pdbentry));
    status = dbFirstRecord(pdbentry);
    while(!status) {
      paramInfo->recordName=strdup(dbGetRecordName(pdbentry));
      if(!dbIsAlias(pdbentry)){
        status=dbFindField(pdbentry,"INP");
        if(!status){
          paramInfo->inp=strdup(dbGetString(pdbentry));
          paramInfo->isInput=true;
          char port[MAX_FIELD_CHAR_LENGTH];
          int adr;
          int timeout;
          char currdrvInfo[MAX_FIELD_CHAR_LENGTH];
          int nvals=sscanf(paramInfo->inp,"@asyn(%[^,],%d,%d)%s",port,&adr,&timeout,currdrvInfo);
          if(nvals==4){
            //Ensure correct port and drvinfo
            if(strcmp(port,portName)==0 && strcmp(drvInfo,currdrvInfo)==0){
              recordFound=true;  // Correct port and drvinfo!\n");
            }
          }
        }
        else{
          paramInfo->isInput=false;
        }
        status=dbFindField(pdbentry,"OUT");
        if(!status){
          paramInfo->out=strdup(dbGetString(pdbentry));
          paramInfo->isOutput=true;
          char port[MAX_FIELD_CHAR_LENGTH];
          int adr;
          int timeout;
          char currdrvInfo[MAX_FIELD_CHAR_LENGTH];
          int nvals=sscanf(paramInfo->out,"@asyn(%[^,],%d,%d)%s",port,&adr,&timeout,currdrvInfo);
          if(nvals==4){
            //Ensure correct port and drvinfo
            if(strcmp(port,portName)==0 && strcmp(drvInfo,currdrvInfo)==0){
              recordFound=true;  // Correct port and drvinfo!\n");
            }
          }
        }
        else{
          paramInfo->isOutput=false;
        }

        if(recordFound){
          // Correct record found. Collect data from fields
          //DTYP
          status=dbFindField(pdbentry,"DTYP");
          if(!status){
            paramInfo->dtyp=strdup(dbGetString(pdbentry));
            paramInfo->asynType=dtypStringToAsynType(dbGetString(pdbentry));
          }
          else{
            paramInfo->dtyp=0;
            paramInfo->asynType=asynParamNotDefined;
          }
          //SCAN
          status=dbFindField(pdbentry,"SCAN");
          if(!status){
            paramInfo->scan=strdup(dbGetString(pdbentry));
          }
          else{
            paramInfo->scan=0;
          }
          //drvInput (not a field)
          paramInfo->drvInfo=strdup(drvInfo);
          dbFreeEntry(pdbentry);
          return asynSuccess;  // The correct record was found and the paramInfo structure is filled
        }
        else{
          //Not correct record. Do cleanup.
          if(paramInfo->isInput){
            free(paramInfo->inp);
            paramInfo->inp=0;
          }
          if(paramInfo->isOutput){
            free(paramInfo->out);
            paramInfo->out=0;
          }
          paramInfo->drvInfo=0;
          paramInfo->scan=0;
          paramInfo->dtyp=0;
          paramInfo->isInput=false;
          paramInfo->isOutput=false;
        }
      }
      status = dbNextRecord(pdbentry);
      free(paramInfo->recordName);
      paramInfo->recordName=0;
    }
    status = dbNextRecordType(pdbentry);
    free(paramInfo->recordType);
    paramInfo->recordType=0;
  }
  dbFreeEntry(pdbentry);
  return asynError;
}

int adsAsynPortDriver::getAmsPortFromDrvInfo(const char* drvInfo)
{
  const char* functionName = "getAmsPortFromDrvInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  //check if "ADSPORT" option in drvInfo string
  char plcAdr[MAX_FIELD_CHAR_LENGTH];
  int amsPortLocal=0;
  int nvals=sscanf(drvInfo,"ADSPORT=%d/%s",&amsPortLocal,plcAdr);
  if(nvals==2){
    return amsPortLocal;
  }
  return amsport_;  //No ADSPORT option found => return default port
}

asynStatus adsAsynPortDriver::parsePlcInfofromDrvInfo(const char* drvInfo,adsParamInfo *paramInfo)
{
  const char* functionName = "parsePlcInfofromDrvInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  bool err=false;
  //take part after last "/" if option or complete string.. How to handle .adr.??
  char plcAdrLocal[MAX_FIELD_CHAR_LENGTH];
  //See if option (find last '/')
  const char *drvInfoEnd=strrchr(drvInfo,'/');
  if(drvInfoEnd){ // found '/'
    int nvals=sscanf(drvInfoEnd,"/%s",plcAdrLocal);
    if(nvals==1){
      paramInfo->plcSymAdr=strdup(plcAdrLocal);
    }
    else{
      err=true;
    }
  }

  //Check if .ADR. command
  paramInfo->plcAbsAdrValid=false;
  paramInfo->isAdrCommand=false;
  const char *adrStr=strstr(drvInfo,ADR_COMMAND_PREFIX);
  if(adrStr){
    paramInfo->isAdrCommand=true;
    int nvals;
    nvals = sscanf(adrStr, ".ADR.16#%x,16#%x,%u,%u",
             &paramInfo->plcGroup,
             &paramInfo->plcOffsetInGroup,
             &paramInfo->plcSize,
             &paramInfo->plcDataType);

    if(nvals==4){
      paramInfo->plcAbsAdrValid=true;
    }
    else{
      err=true;
    }
  }
  //Look for AMSPORT option
  paramInfo->amsPort=getAmsPortFromDrvInfo(drvInfo);

  if(err){
    return asynError;
  }
  else{
    return asynSuccess;
  }
}

void adsAsynPortDriver::printParamInfo(adsParamInfo *paramInfo)
{
  const char* functionName = "printParamInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  printf("########################################\n");
  printf("  Record name:        %s\n",paramInfo->recordName);
  printf("  Record type:        %s\n",paramInfo->recordType);
  printf("  Record dataType:    %s\n",paramInfo->dtyp);
  printf("  Record asynType:    %d\n",paramInfo->asynType);
  printf("  Record scan:        %s\n",paramInfo->scan);
  printf("  Record inp:         %s\n",paramInfo->inp);
  printf("  Record out:         %s\n",paramInfo->out);
  printf("  Record isInput:     %d\n",paramInfo->isInput);
  printf("  Record isOutput:    %d\n",paramInfo->isOutput);
  printf("  Param index:        %d\n",paramInfo->paramIndex);
  printf("  Param drvInfo:      %s\n",paramInfo->drvInfo);
  printf("  Plc SymAdr:         %s\n",paramInfo->plcSymAdr);
  printf("  Plc Ams Port:       %d\n",paramInfo->amsPort);
  printf("  Plc SymAdrIsAdrCmd: %d\n",paramInfo->isAdrCommand);
  printf("  Plc AbsAdrValid:    %d\n",paramInfo->plcAbsAdrValid);
  printf("  Plc GroupNum:       16#%x\n",paramInfo->plcGroup);
  printf("  Plc OffsetInGroup:  16#%x\n",paramInfo->plcOffsetInGroup);
  printf("  Plc DataTypeSize:   %u\n",paramInfo->plcSize);
  printf("  Plc DataType:       %u\n",paramInfo->plcDataType);
  printf("########################################\n");
}

asynStatus adsAsynPortDriver::readOctet(asynUser *pasynUser, char *value, size_t maxChars,size_t *nActual, int *eomReason)
{
  const char* functionName = "readOctet";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  size_t thisRead = 0;
  int reason = 0;
  asynStatus status = asynSuccess;

  *value = '\0';
  lock();
  int error=CMDreadIt(value, maxChars);
  if (error) {
    status = asynError;
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, CMDreadIt failed (0x%x).\n",
              driverName, functionName, error);
    unlock();
    return asynError;
  }

  thisRead = strlen(value);
  *nActual = thisRead;

  /* May be not enough space ? */
  if (thisRead > maxChars-1) {
    reason |= ASYN_EOM_CNT;
  }
  else{
    reason |= ASYN_EOM_EOS;
  }

  if (thisRead == 0 && pasynUser->timeout == 0){
    status = asynTimeout;
  }

  if (eomReason){
    *eomReason = reason;
  }

  *nActual = thisRead;
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
             "%s thisRead=%lu data=\"%s\"\n",
             portName,
             (unsigned long)thisRead, value);
  unlock();
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:%s\n", driverName, functionName,value);

  return status;
}

asynStatus adsAsynPortDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,size_t *nActual)
{
  const char* functionName = "writeOctet";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: %s\n", driverName, functionName,value);

  size_t thisWrite = 0;
  asynStatus status = asynError;

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s write.\n", /*ecmcController_p->*/portName);
  asynPrintIO(pasynUser, ASYN_TRACEIO_DRIVER, value, maxChars,
              "%s write %lu\n",
              portName,
              (unsigned long)maxChars);
  *nActual = 0;

  if (maxChars == 0){
    return asynSuccess;
  }
  //lock();
  if (!(CMDwriteIt(value, maxChars))) {
    thisWrite = maxChars;
    *nActual = thisWrite;
    status = asynSuccess;
  }
  //unlock();
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s wrote %lu return %s.\n",
            portName,
            (unsigned long)*nActual,
            pasynManager->strStatus(status));
  return status;
}

asynStatus adsAsynPortDriver::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
  const char* functionName = "readFloat64";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  int function = pasynUser->reason;
  const char *paramName;
  getParamName(function, &paramName);
  *value=101.01999;

  asynPrint(pasynUser, ASYN_TRACE_INFO, "readFloat64: %s, %d\n",paramName,function);
  asynStatus status = asynSuccess;
  return status;

}

asynStatus adsAsynPortDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  const char* functionName = "writeInt32";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynStatus adsAsynPortDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  const char* functionName = "writeFloat64";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynUser *adsAsynPortDriver::getTraceAsynUser()
{
  const char* functionName = "getTraceAsynUser";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  return pasynUserSelf;
}

asynStatus adsAsynPortDriver::readInt8Array(asynUser *pasynUser, epicsInt8 *value,size_t nElements, size_t *nIn)
{
  const char* functionName = "readInt8Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynStatus adsAsynPortDriver::readInt16Array(asynUser *pasynUser, epicsInt16 *value,size_t nElements, size_t *nIn)
{
  const char* functionName = "readInt16Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynStatus adsAsynPortDriver::readInt32Array(asynUser *pasynUser, epicsInt32 *value,size_t nElements, size_t *nIn)
{
  const char* functionName = "readInt32Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynStatus adsAsynPortDriver::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,size_t nElements, size_t *nIn)
{
  const char* functionName = "readFloat32Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynStatus adsAsynPortDriver::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value,size_t nElements, size_t *nIn)
{
  const char* functionName = "readFloat64Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = asynSuccess;
  return status;
}

asynStatus adsAsynPortDriver::adsAddNotificationCallback(adsParamInfo *paramInfo)
{
  const char* functionName = "addNotificationCallbackAbsAdr";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  uint32_t hNotify=0;

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsport_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  long addStatus=0;

  // Absolute access via group offset
  if(paramInfo->isAdrCommand){

    if(!paramInfo->plcAbsAdrValid){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Absolute address in paramInfo not valid.\n", driverName, functionName);
      return asynError;
    }

    AdsNotificationAttrib attrib;
    /** Length of the data that is to be passed to the callback function. */
    attrib.cbLength=paramInfo->plcSize;
    /**
    * ADSTRANS_SERVERCYCLE: The notification's callback function is invoked cyclically.
    * ADSTRANS_SERVERONCHA: The notification's callback function is only invoked when the value changes.
    */
    attrib.nTransMode=ADSTRANS_SERVERONCHA;
    /** The notification's callback function is invoked at the latest when this time has elapsed. The unit is 100 ns. */
    attrib.nMaxDelay=10000000; // 1s
    /** The ADS server checks whether the variable has changed after this time interval. The unit is 100 ns. */
    attrib.dwChangeFilter=1000000; //100ms

    addStatus = AdsSyncAddDeviceNotificationReqEx(adsPort_,
                                                             &amsServer,
                                                             paramInfo->plcGroup,
                                                             paramInfo->plcOffsetInGroup,
                                                             &attrib,
                                                             &adsNotifyCallback,
                                                             (uint32_t)paramInfo->paramIndex,
                                                             &hNotify);

  }
  else{ // Access via symbolic varaiable name

    AdsSymbolEntry infoStruct;
    //get information about symbol (only to get size)
    asynStatus statusInfo=adsGetSymInfoByName(paramInfo->amsPort,paramInfo->plcSymAdr,&infoStruct);

    if(statusInfo==asynError){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymInfoByName failed.\n", driverName, functionName);
      return asynError;
    }
    //fill data in structure
    paramInfo->plcGroup=infoStruct.iGroup;
    paramInfo->plcOffsetInGroup=infoStruct.iOffset;
    paramInfo->plcSize=infoStruct.size;
    paramInfo->plcDataType=infoStruct.dataType;
    paramInfo->plcAbsAdrValid=true;  //However hopefully this adress should never be used (use symbol intstead since safer if memory moves in plc)..

    //NOTE: MUST CHECK THAT handleByName folllows if adr is moved in PLC (after compile) otherwise a '
    // rtriggering of all notificatios are needed...

    uint32_t symbolHandle=0;

    const long handleStatus = AdsSyncReadWriteReqEx2(adsPort_,
                                                     &amsServer,
                                                     ADSIGRP_SYM_HNDBYNAME,
                                                     0,
                                                     sizeof(paramInfo->hSymbolicHandle),
                                                     &symbolHandle,
                                                     strlen(paramInfo->plcSymAdr),
                                                     paramInfo->plcSymAdr,
                                                     nullptr);
    if (handleStatus) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Create handle for %s failed with: %ld\n", driverName, functionName,paramInfo->plcSymAdr,handleStatus);
      return asynError;
    }

    //Add handle succeded
    paramInfo->hSymbolicHandle=symbolHandle;

    AdsNotificationAttrib attrib;
    /** Length of the data that is to be passed to the callback function. */
    attrib.cbLength=paramInfo->plcSize;
    /**
    * ADSTRANS_SERVERCYCLE: The notification's callback function is invoked cyclically.
    * ADSTRANS_SERVERONCHA: The notification's callback function is only invoked when the value changes.
    */
    attrib.nTransMode=ADSTRANS_SERVERONCHA;
    /** The notification's callback function is invoked at the latest when this time has elapsed. The unit is 100 ns. */
    attrib.nMaxDelay=10000000; // 1s
    /** The ADS server checks whether the variable has changed after this time interval. The unit is 100 ns. */
    attrib.dwChangeFilter=1000000; //100ms
    addStatus = AdsSyncAddDeviceNotificationReqEx(adsPort_,
                                                             &amsServer,
                                                             ADSIGRP_SYM_VALBYHND,
                                                             paramInfo->hSymbolicHandle,
                                                             &attrib,
                                                             &adsNotifyCallback,
                                                             (uint32_t)paramInfo->paramIndex,
                                                             &hNotify);
  }

  if (addStatus){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Add device notification failed with: %ld\n", driverName, functionName,addStatus);
    return asynError;
  }

  //Add was successfull
  paramInfo->hCallbackNotify=hNotify;

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsDelNotificationCallback(adsParamInfo *paramInfo)
{
  const char* functionName = "delNotificationCallback";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsport_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  const long delStatus = AdsSyncDelDeviceNotificationReqEx(adsPort_, &amsServer,paramInfo->hCallbackNotify);
  if (delStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Delete device notification failed with: %ld\n", driverName, functionName,delStatus);
    return asynError;
  }

  //Release symbolic handle if needed
  if(!paramInfo->isAdrCommand){
    asynStatus releaseStatus=adsReleaseSymbolicHandle(paramInfo);
    if(releaseStatus!=asynSuccess){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Release symbolic handle failed\n", driverName, functionName);
      return asynError;
    }
  }
  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsGetSymInfoByName(uint16_t amsport,char *variableName,AdsSymbolEntry *infoStruct)
{
  const char* functionName = "getSymInfoByName";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  uint32_t bytesRead=0;
  AmsAddr amsServer;

  if(amsport<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsport_};
  }
  else{
    amsServer={remoteNetId_,amsport};
  }

  const long infoStatus = AdsSyncReadWriteReqEx2(adsPort_,
                                             &amsServer,
                                             ADSIGRP_SYM_INFOBYNAMEEX,
                                             0,
                                             sizeof(AdsSymbolEntry),
                                             infoStruct,
                                             strlen(variableName),
                                             variableName,
                                             &bytesRead);

  if (infoStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Get symbolic information failed for %s: %ld\n", driverName, functionName,variableName,infoStatus);
    return asynError;
  }

  infoStruct->variableName = infoStruct->buffer;

  if(infoStruct->nameLength>=sizeof(infoStruct->buffer)-1){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Missalignment of type in AdsSyncReadWriteReqEx2 return struct for %s: 0x%x\n", driverName, functionName,variableName,ADS_COM_ERROR_READ_SYMBOLIC_INFO);
    return asynError;
  }
  infoStruct->symDataType = infoStruct->buffer+infoStruct->nameLength+1;

  if(infoStruct->nameLength + infoStruct->typeLength+2>=(uint16_t)(sizeof(infoStruct->buffer)-1)){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Missalignment of comment in AdsSyncReadWriteReqEx2 return struct for %s: 0x%x\n", driverName, functionName,variableName,ADS_COM_ERROR_READ_SYMBOLIC_INFO);
  }
  infoStruct->symComment= infoStruct->symDataType+infoStruct->typeLength+1;

  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Symbolic information\n");
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"SymEntrylength: %d\n",infoStruct->entryLen);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"idxGroup: 0x%x\n",infoStruct->iGroup);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"idxOffset: 0x%x\n",infoStruct->iOffset);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"ByteSize: %d\n",infoStruct->size);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"adsDataType: %d\n",infoStruct->dataType);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Flags: %d\n",infoStruct->flags);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Name length: %d\n",infoStruct->nameLength);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Type length: %d\n",infoStruct->typeLength);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Type length: %d\n",infoStruct->commentLength);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Variable name: %s\n",infoStruct->variableName);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Data type: %s\n",infoStruct->symDataType);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Comment: %s\n",infoStruct->symComment);

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsConnect()
{
  const char* functionName = "adsConnect";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  // add local route to your ADS Master
   const long addRouteStatus =AdsAddRoute(remoteNetId_, ipaddr_);
  if (addRouteStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Adding ADS route failed with code: %ld.\n", driverName, functionName,addRouteStatus);
    return asynError;
  }

  // open a new ADS port
  adsPort_ = AdsPortOpenEx();
  if (!adsPort_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s:Open ADS port failed.\n", driverName, functionName);
    return asynError;
  }

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsDisconnect()
{
  const char* functionName = "adsDisconnect";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  const long closeStatus = AdsPortCloseEx(adsPort_);
  if (closeStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Close ADS port failed with error code: %ld\n", driverName, functionName,closeStatus);
    return asynError;
  }

#ifdef _WIN32
   // WORKAROUND: On Win7 std::thread::join() called in destructors
   //             of static objects might wait forever...
   AdsDelRoute(remoteNetId_);
#endif

  adsPort_=0;
  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsReleaseSymbolicHandle(adsParamInfo *paramInfo)
{
  const char* functionName = "adsReleaseHandle";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsport_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  const long releaseStatus = AdsSyncWriteReqEx(adsPort_, &amsServer, ADSIGRP_SYM_RELEASEHND, 0, sizeof(paramInfo->hSymbolicHandle), &paramInfo->hSymbolicHandle);
  if (releaseStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Release of handle 0x%x failed with: %ld\n", driverName, functionName,paramInfo->hSymbolicHandle,releaseStatus);
    return asynError;
  }
  return asynSuccess;
}


/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

  asynUser *pPrintOutAsynUser;

  static adsAsynPortDriver *adsAsynPortObj;
  /*
   * Configure and register
   */
  epicsShareFunc int
  adsAsynPortDriverConfigure(const char *portName,
                             const char *ipaddr,
                             const char *amsaddr,
                             unsigned int amsport,
                             unsigned int asynParamTableSize,
                             unsigned int priority,
                             int noAutoConnect,
                             int noProcessEos)
  {

    if (!portName) {
      printf("drvAsynAdsPortConfigure bad portName: %s\n",
             portName ? portName : "");
      return -1;
    }
    if (!ipaddr) {
      printf("drvAsynAdsPortConfigure bad ipaddr: %s\n",ipaddr ? ipaddr : "");
      return -1;
    }
    if (!amsaddr) {
      printf("drvAsynAdsPortConfigure bad amsaddr: %s\n",amsaddr ? amsaddr : "");
      return -1;
    }

    adsAsynPortObj=new adsAsynPortDriver(portName,ipaddr,amsaddr,amsport,asynParamTableSize,priority,noAutoConnect==0,noProcessEos);
    if(adsAsynPortObj){
      asynUser *traceUser= adsAsynPortObj->getTraceAsynUser();
      if(!traceUser){
        printf("adsAsynPortDriverConfigure: ERROR: Failed to retrieve asynUser for trace. \n");
        return (asynError);
      }
      pPrintOutAsynUser=traceUser;
      //adsAsynPortObj->connect(traceUser);
    }

    return asynSuccess;
  }

  /*
   * IOC shell command registration
   */
  static const iocshArg adsAsynPortDriverConfigureArg0 = { "port name",iocshArgString};
  static const iocshArg adsAsynPortDriverConfigureArg1 = { "ip-addr",iocshArgString};
  static const iocshArg adsAsynPortDriverConfigureArg2 = { "ams-addr",iocshArgString};
  static const iocshArg adsAsynPortDriverConfigureArg3 = { "default-ams-port",iocshArgInt};
  static const iocshArg adsAsynPortDriverConfigureArg4 = { "asyn param table size",iocshArgInt};
  static const iocshArg adsAsynPortDriverConfigureArg5 = { "priority",iocshArgInt};
  static const iocshArg adsAsynPortDriverConfigureArg6 = { "disable auto-connect",iocshArgInt};
  static const iocshArg adsAsynPortDriverConfigureArg7 = { "noProcessEos",iocshArgInt};
  static const iocshArg *adsAsynPortDriverConfigureArgs[] = {
    &adsAsynPortDriverConfigureArg0, &adsAsynPortDriverConfigureArg1,
    &adsAsynPortDriverConfigureArg2, &adsAsynPortDriverConfigureArg3,
    &adsAsynPortDriverConfigureArg4, &adsAsynPortDriverConfigureArg5,
    &adsAsynPortDriverConfigureArg6, &adsAsynPortDriverConfigureArg7};

  static const iocshFuncDef adsAsynPortDriverConfigureFuncDef =
    {"adsAsynPortDriverConfigure",8,adsAsynPortDriverConfigureArgs};

  static void adsAsynPortDriverConfigureCallFunc(const iocshArgBuf *args)
  {
    adsAsynPortDriverConfigure(args[0].sval,args[1].sval,args[2].sval,args[3].ival, args[4].ival, args[5].ival,args[6].ival,args[7].ival);
  }

  /*
   * This routine is called before multitasking has started, so there's
   * no race condition in the test/set of firstTime.
   */

  static void adsAsynPortDriverRegister(void)
  {
    iocshRegister(&adsAsynPortDriverConfigureFuncDef,adsAsynPortDriverConfigureCallFunc);
  }

  epicsExportRegistrar(adsAsynPortDriverRegister);
}



