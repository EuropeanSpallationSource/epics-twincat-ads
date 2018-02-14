/*
 */

#include "adsAsynPortDriver.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>

#include "cmd.h"

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <iocsh.h>
#include <initHooks.h>

#include <epicsExport.h>
#include <dbStaticLib.h>
#include <dbAccess.h>

static const char *driverName="adsAsynPortDriver";
static adsAsynPortDriver *adsAsynPortObj;
static long oldTimeStamp=0;
static struct timeval oldTime={0};
static int allowCallbackEpicsState=0;
static initHookState currentEpicsState=initHookAtIocBuild;


static void getEpicsState(initHookState state)
{
  const char* functionName = "adsNotifyCallback";

  if(!adsAsynPortObj){
    printf("%s:%s: ERROR: adsAsynPortObj==NULL\n", driverName, functionName);
    return;
  }

  asynUser *asynTraceUser=adsAsynPortObj->getTraceAsynUser();

  switch(state) {
    case initHookAfterIocRunning:
      allowCallbackEpicsState=1;
      break;
    default:
      break;
  }
  currentEpicsState=state;
  asynPrint(asynTraceUser, ASYN_TRACE_INFO, "%s:%s: EPICS state: %s (%d). Allow ADS callbacks: %s.\n", driverName, functionName,epicsStateToString((int)state),(int)state,allowCallbackEpicsState ? "true" : "false");
}

int initHook(void)
{
  return(initHookRegister(getEpicsState));
}

static void adsNotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
  const char* functionName = "adsNotifyCallback";

  if(!adsAsynPortObj){
    printf("%s:%s: ERROR: adsAsynPortObj==NULL\n", driverName, functionName);
    return;
  }

  asynUser *asynTraceUser=adsAsynPortObj->getTraceAsynUser();
  asynPrint(asynTraceUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);


  if(!allowCallbackEpicsState){
    return;
  }

  const uint8_t* data = reinterpret_cast<const uint8_t*>(pNotification + 1);
  struct timeval newTime;
  gettimeofday(&newTime, NULL);

  asynPrint(asynTraceUser, ASYN_TRACE_INFO,"TIME <%ld.%06ld>\n",(long) newTime.tv_sec, (long) newTime.tv_usec);

  long secs_used=(newTime.tv_sec - oldTime.tv_sec); //avoid overflow by subtracting first
  long micros_used= ((secs_used*1000000) + newTime.tv_usec) - (oldTime.tv_usec);
  oldTime=newTime;

  //Ensure hUser is within range
  if(hUser>(uint32_t)(adsAsynPortObj->getParamTableSize()-1)){
    asynPrint(asynTraceUser, ASYN_TRACE_ERROR, "%s:%s: hUser out of range: %u.\n", driverName, functionName,hUser);
    return;
  }

  //Get paramInfo
  adsParamInfo *paramInfo=adsAsynPortObj->getAdsParamInfo(hUser);
  if(!paramInfo){
    asynPrint(asynTraceUser, ASYN_TRACE_ERROR, "%s:%s: getAdsParamInfo() for hUser %u failed\n", driverName, functionName,hUser);
    return;
  }

  if(!adsAsynPortObj->isCallbackAllowed(paramInfo)){
    return;
  }

  asynPrint(asynTraceUser, ASYN_TRACE_INFO,"Callback for parameter %s (%d).\n",paramInfo->drvInfo,paramInfo->paramIndex);
  asynPrint(asynTraceUser, ASYN_TRACE_INFO,"hUser 0x%x, data size[b]: %d.\n", hUser,pNotification->cbSampleSize);
  asynPrint(asynTraceUser, ASYN_TRACE_INFO,"time stamp [100ns]: %ld, since last plc [ms]: %4.2lf, since last ioc [ms]: %4.2lf.\n",
            pNotification->nTimeStamp,((double)(pNotification->nTimeStamp-oldTimeStamp))/10000.0,(((double)(micros_used))/1000.0));
  oldTimeStamp=pNotification->nTimeStamp;

  //Ensure hUser is equal to parameter index
  if(hUser!=(uint32_t)(paramInfo->paramIndex)){
    asynPrint(asynTraceUser, ASYN_TRACE_ERROR, "%s:%s: hUser not equal to parameter index (%u vs %d).\n", driverName, functionName,hUser,paramInfo->paramIndex);
    return;
  }

  adsAsynPortObj->adsUpdateParameterLock(paramInfo,data,pNotification->cbSampleSize);
}

//Check ads state and, supervise connection (reconnect if needed)
void cyclicThread(void *drvPvt)
{
  adsAsynPortDriver *pPvt = (adsAsynPortDriver *)drvPvt;
  pPvt->cyclicThread();
}

// Constructor for the adsAsynPortDriver class.
adsAsynPortDriver::adsAsynPortDriver(const char *portName,
                                     const char *ipaddr,
                                     const char *amsaddr,
                                     unsigned int amsport,
                                     int paramTableSize,
                                     unsigned int priority,
                                     int autoConnect,
                                     int noProcessEos,
                                     int defaultSampleTimeMS,
                                     int maxDelayTimeMS,
                                     int adsTimeoutMS)
                     :asynPortDriver(portName,
                                     1, /* maxAddr */
                                     paramTableSize,
                                     asynInt32Mask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask | asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask, /* Interface mask */
                                     asynInt32Mask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask | asynInt8ArrayMask | asynInt16ArrayMask | asynInt32ArrayMask,  /* Interrupt mask */
                                     ASYN_CANBLOCK, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
                                     autoConnect, /* Autoconnect */
                                     priority, /* Default priority */
                                     0) /* Default stack size*/
{
  const char* functionName = "adsAsynPortDriver";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  pAdsParamArray_= new adsParamInfo*[paramTableSize];
  memset(pAdsParamArray_,0,sizeof(*pAdsParamArray_));
  adsParamArrayCount_=0;
  paramTableSize_=paramTableSize;
  ipaddr_=strdup(ipaddr);
  amsaddr_=strdup(amsaddr);
  amsportDefault_=amsport;
  priority_=priority;
  autoConnect_=autoConnect;
  noProcessEos_=noProcessEos;
  defaultSampleTimeMS_=defaultSampleTimeMS;
  defaultMaxDelayTimeMS_=maxDelayTimeMS;
  adsTimeoutMS_=adsTimeoutMS;
  connectedAds_=0;
  paramRefreshNeeded_=1;

  //ADS
  adsPort_=0; //handle
  remoteNetId_={0,0,0,0,0,0};
  amsPortList_.clear();

  if(amsportDefault_<=0){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Invalid default AMS port: %d\n", driverName, functionName,amsportDefault_);
    return;
  }

  int nvals = sscanf(amsaddr_, "%hhu.%hhu.%hhu.%hhu.%hhu.%hhu",
                     &remoteNetId_.b[0],
                     &remoteNetId_.b[1],
                     &remoteNetId_.b[2],
                     &remoteNetId_.b[3],
                     &remoteNetId_.b[4],
                     &remoteNetId_.b[5]);

  //Global variables in adsCom.h (for motor record and streamdevice using cmd_eat parser)
  uint8_t  *netId;
  netId=(uint8_t*)&remoteNetId_;

  //Set values to old ascii based api (still used for motor record and streamdevice)
  //setAmsNetId(netId);
  //setAmsPort(amsportDefault_);

  if (nvals != 6) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Invalid AMS address: %s\n", driverName, functionName,amsaddr_);
    return;
  }

  if(paramTableSize_<1){  //If paramTableSize_==1 then only stream device or motor record can use the driver through the "default access" param below.
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Param table size to small: %d\n", driverName, functionName,paramTableSize_);
    return;
  }

  //Add first param for other access (like motor record or stream device).
  int index;
  asynStatus status=createParam("Default access",asynParamNotDefined,&index);
  if(status!=asynSuccess){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: createParam for default access failed.\n", driverName, functionName);
    return;
  }
  adsParamInfo *paramInfo=new adsParamInfo();
  memset(paramInfo,0,sizeof(adsParamInfo));
  paramInfo->recordName=strdup("Any record");
  paramInfo->recordType=strdup("No type");
  paramInfo->scan=strdup("No scan");
  paramInfo->dtyp=strdup("No dtyp");
  paramInfo->inp=strdup("No inp");
  paramInfo->out=strdup("No out");
  paramInfo->drvInfo=strdup("No drvinfo");
  paramInfo->asynType=asynParamNotDefined;
  paramInfo->paramIndex=index;  //also used as hUser for ads callback
  paramInfo->plcAdrStr=strdup("No adr str");
  pAdsParamArray_[0]=paramInfo;
  adsParamArrayCount_++;

  //* Create the thread that computes the waveforms in the background */
  status = (asynStatus)(epicsThreadCreate("adsAsynPortDriverCyclicThread",
                                                     epicsThreadPriorityMedium,
                                                     epicsThreadGetStackSize(epicsThreadStackMedium),
                                                     (EPICSTHREADFUNC)::cyclicThread,this) == NULL);
  if(status){
    printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
    return;
  }
}

adsAsynPortDriver::~adsAsynPortDriver()
{
  const char* functionName = "~adsAsynPortDriver";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  free(ipaddr_);
  free(amsaddr_);

  for(int i=0;i<adsParamArrayCount_;i++){
    if(!pAdsParamArray_[i]){
      continue;
    }
    adsDelNotificationCallback(pAdsParamArray_[i],true);  //Block error messages
    adsReleaseSymbolicHandle(pAdsParamArray_[i],true);    //Block error messages
    free(pAdsParamArray_[i]->recordName);
    free(pAdsParamArray_[i]->recordType);
    free(pAdsParamArray_[i]->scan);
    free(pAdsParamArray_[i]->dtyp);
    free(pAdsParamArray_[i]->inp);
    free(pAdsParamArray_[i]->out);
    free(pAdsParamArray_[i]->drvInfo);
    free(pAdsParamArray_[i]->plcAdrStr);
    if(pAdsParamArray_[i]->plcDataIsArray){
      free(pAdsParamArray_[i]->arrayDataBuffer);
    }
    delete pAdsParamArray_[i];
  }
  delete pAdsParamArray_;

  for(amsPortInfo *port : amsPortList_){
    delete port;
  }

}

void adsAsynPortDriver::cyclicThread()
{
  const char* functionName = "cyclicThread";
  double sampleTime=1;
  while (1){
    asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: Sample time [s]= %lf.\n",driverName,functionName,sampleTime);
    uint16_t adsState=0;
    asynStatus stat;

    //Check state of all used ams ports
    if(connectedAds_){
      for(amsPortInfo *port : amsPortList_){
        asynStatus stat=adsReadState(port->amsPort,&adsState);
        bool portConnected=(stat==asynSuccess);
        port->connected=portConnected;
        asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: ADS state: %s (amsPort %d).\n",driverName,functionName,asynStateToString(adsState),port->amsPort);
      }
    }

    if(connectedAds_){
      //Ensure that all amsports are connected (otherwise destroyed when disconnecting)
      bool oneAmsConnectionOK=false;
      for(amsPortInfo *port : amsPortList_){
        oneAmsConnectionOK=oneAmsConnectionOK || port->connected;
        if(port->connected){
          if(!port->paramsOK){
            stat=refreshParams(port->amsPort);
            if(stat==asynSuccess){
              port->paramsOK=1;
            }
            else{
              port->paramsOK=0;
            }
          }
        }
        else{
          port->paramsOK=0;
        }
      }
      //If not atleast one amsPort connection OK then reconnect completely
      if(!oneAmsConnectionOK){
        asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: No amsPort connection OK. Try complete reconnect\n",driverName,functionName);
        connectedAds_=0;
      }
    }
    else{
      //Communication error try to reconnect
      if(autoConnect_){
        lock();
        disconnect(pasynUserSelf);
        connect(pasynUserSelf);
        for(amsPortInfo *port : amsPortList_){
           port->paramsOK=false;
           port->connected=false;
        }
        unlock();
      }
    }

    for(amsPortInfo *port : amsPortList_){
      asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: Amsport %d, connected %d, params OK %d.\n",driverName,functionName,(int)port->amsPort, (int)port->connected,(int)port->paramsOK);
    }

    epicsThreadSleep(sampleTime);
  }
}

void adsAsynPortDriver::report(FILE *fp, int details)
{
  const char* functionName = "report";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if(!fp){
    fprintf(fp,"%s:%s: ERROR: File NULL.\n", driverName, functionName);
    return;
  }

  if (details >= 1) {
    fprintf(fp, "General information:\n");
    fprintf(fp, "  Port:                %s\n",portName);
    fprintf(fp, "  Ip-address:          %s\n",ipaddr_);
    fprintf(fp, "  Ams-address:         %s\n",amsaddr_);
    fprintf(fp, "  Default Ams-port :   %d\n",amsportDefault_);
    fprintf(fp, "  Auto-connect:        %s\n",autoConnect_ ? "true" : "false");
    fprintf(fp, "  Priority:            %d\n",priority_); //Used?
    fprintf(fp, "  ProcessEos:          %s\n",noProcessEos_ ? "false" : "true"); //Inverted
    fprintf(fp, "  Param. table size:   %d\n",paramTableSize_);
    fprintf(fp, "  Param. count:        %d\n",adsParamArrayCount_);
    fprintf(fp, "  NOTE: Several records can be linked to the same parameter.\n");
    fprintf(fp,"\n");
  }
  if(details>=2){
    //print all parameters
    fprintf(fp,"Parameter details:\n");
    for(int i=0; i<adsParamArrayCount_;i++){
      if(!pAdsParamArray_[i]){
        fprintf(fp,"%s:%s: ERROR: Parameter array null at index %d\n", driverName, functionName,i);
        return;
      }
      adsParamInfo *paramInfo=pAdsParamArray_[i];
      fprintf(fp,"  Parameter %d:\n",i);
      if(i==0){
        fprintf(fp,"    Parameter 0 is reserved for Motor Record and Stream Device access (pasynUser->reason==0)!\n");
        fprintf(fp,"\n");
        continue;
      }
      fprintf(fp,"    Param name:                %s\n",paramInfo->drvInfo);
      fprintf(fp,"    Param index:               %d\n",paramInfo->paramIndex);
      fprintf(fp,"    Param type:                %s (%d)\n",asynTypeToString((long)paramInfo->asynType),paramInfo->asynType);
      fprintf(fp,"    Param sample time [ms]:    %lf\n",paramInfo->sampleTimeMS);
      fprintf(fp,"    Param max delay time [ms]: %lf\n",paramInfo->maxDelayTimeMS);
      fprintf(fp,"    Param isIOIntr:            %s\n",paramInfo->isIOIntr ? "true" : "false");
      fprintf(fp,"    Param asyn addr:           %d\n",paramInfo->asynAddr);
      fprintf(fp,"    Param time base:           %s\n",paramInfo->timeBase ? ADS_OPTION_TIMEBASE_PLC : ADS_OPTION_TIMEBASE_EPICS);
      fprintf(fp,"    Param array buffer alloc:  %s\n",paramInfo->arrayDataBuffer ? "true" : "false");
      fprintf(fp,"    Param array buffer size:   %lu\n",paramInfo->arrayDataBufferSize);
      fprintf(fp,"    Plc ams port:              %d\n",paramInfo->amsPort);
      fprintf(fp,"    Plc adr str:               %s\n",paramInfo->plcAdrStr);
      fprintf(fp,"    Plc adr str is ADR cmd:    %s\n",paramInfo->isAdrCommand ? "true" : "false");
      fprintf(fp,"    Plc abs adr valid:         %s\n",paramInfo->plcAbsAdrValid ? "true" : "false");
      fprintf(fp,"    Plc abs adr group:         16#%x\n",paramInfo->plcAbsAdrGroup);
      fprintf(fp,"    Plc abs adr offset:        16#%x\n",paramInfo->plcAbsAdrOffset);
      fprintf(fp,"    Plc data type:             %s\n",adsTypeToString(paramInfo->plcDataType));
      fprintf(fp,"    Plc data type size:        %zu\n",adsTypeSize(paramInfo->plcDataType));
      fprintf(fp,"    Plc data size:             %u\n",paramInfo->plcSize);
      fprintf(fp,"    Plc data is array:         %s\n",paramInfo->plcDataIsArray ? "true" : "false");
      fprintf(fp,"    Plc data type warning:     %s\n",paramInfo->plcDataTypeWarn ? "true" : "false");
      fprintf(fp,"    Ads hCallbackNotify:       %u\n",paramInfo->hCallbackNotify);
      fprintf(fp,"    Ads hSymbHndle:            %u\n",paramInfo->hSymbolicHandle);
      fprintf(fp,"    Ads hSymbHndleValid:       %s\n",paramInfo->bSymbolicHandleValid ? "true" : "false");
      fprintf(fp,"    Record name:               %s\n",paramInfo->recordName);
      fprintf(fp,"    Record type:               %s\n",paramInfo->recordType);
      fprintf(fp,"    Record dtyp:               %s\n",paramInfo->dtyp);
      fprintf(fp,"\n");
    }
  }
}

asynStatus adsAsynPortDriver::disconnect(asynUser *pasynUser)
{
  const char* functionName = "disconnect";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus disconnectStatus=adsDisconnect();
  if (disconnectStatus){
    return asynError;
  }

  return asynPortDriver::disconnect(pasynUser);
}

asynStatus adsAsynPortDriver::refreshParams()
{
  return refreshParams(0);
}

asynStatus adsAsynPortDriver::refreshParams(uint16_t amsPort)
{
  const char* functionName = "refreshParams";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if(connectedAds_){
    if(adsParamArrayCount_>1){
      for(int i=1; i<adsParamArrayCount_;i++){  //Skip first param since used for motorrecord or stream device
        if(!pAdsParamArray_[i]){
          continue;
        }
        adsParamInfo *paramInfo=pAdsParamArray_[i];
        if(amsPort==0 || paramInfo->amsPort==amsPort){
          asynStatus stat=updateParamInfoWithPLCInfo(paramInfo);
          if(stat!=asynSuccess){
            return asynError;
          }
        }
      }
    }
  }
  return asynSuccess;
}

asynStatus adsAsynPortDriver::connect(asynUser *pasynUser)
{
  const char* functionName = "connect";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  bool err=false;
  lock();
  asynStatus stat = adsConnect();

  if (stat!= asynSuccess){
    return asynError;
  }

  if(asynPortDriver::connect(pasynUser)!=asynSuccess){
    return asynError;
  }
  connectedAds_=1;
  unlock();
  return err ? asynError : asynSuccess;
}

asynStatus adsAsynPortDriver::validateDrvInfo(const char *drvInfo)
{
  const char* functionName = "validateDrvInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName, functionName,drvInfo);

  if(strlen(drvInfo)==0){
    asynPrint(pasynUserSelf,ASYN_TRACE_ERROR,"Invalid drvInfo string: Length 0 (%s).\n",drvInfo);
    return asynError;
  }

  //Check '?' mark last or '=' last
  const char* read=strrchr(drvInfo,'?');
  if(read){
    if(strlen(read)==1){
      return asynSuccess;
    }
  }

  const char* write=strrchr(drvInfo,'=');
  if(write){
    if(strlen(write)==1){
      return asynSuccess;
    }
  }

  asynPrint(pasynUserSelf,ASYN_TRACE_ERROR,"Invalid drvInfo string (%s).\n",drvInfo);
  return asynError;
}

asynStatus adsAsynPortDriver::drvUserCreate(asynUser *pasynUser,const char *drvInfo,const char **pptypeName,size_t *psize)
{
  const char* functionName = "drvUserCreate";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName, functionName,drvInfo);

  if(validateDrvInfo(drvInfo)!=asynSuccess){
    return asynError;
  }

  int index=0;
  asynStatus status=findParam(drvInfo,&index);
  if(status==asynSuccess){
    asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: Parameter index found at: %d for %s. \n", driverName, functionName,index,drvInfo);
    if(!pAdsParamArray_[index]){
      asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s:pAdsParamArray_[%d]==NULL (drvInfo=%s).", driverName, functionName,index,drvInfo);
      return asynError;
    }
    if(!connectedAds_){
      //try to connect without error handling
      connect(pasynUser);
    }

    if(connectedAds_){
      status =adsReadParam(pAdsParamArray_[index]);
      if(status!=asynSuccess){
        return asynError;
      }
    }
    return asynPortDriver::drvUserCreate(pasynUser,drvInfo,pptypeName,psize);;
  }

  //Ensure space left in param table
  if(adsParamArrayCount_>=paramTableSize_){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: Parameter table full. Parameter with drvInfo %s will be discarded.", driverName, functionName,drvInfo);
    return asynError;
  }

  // Collect data from drvInfo string and recordpasynUser->reason=index;
  adsParamInfo *paramInfo=new adsParamInfo();
  memset(paramInfo,0,sizeof(adsParamInfo));
  paramInfo->sampleTimeMS=defaultSampleTimeMS_;
  paramInfo->maxDelayTimeMS=defaultMaxDelayTimeMS_;

  status=getRecordInfoFromDrvInfo(drvInfo, paramInfo);
  if(status!=asynSuccess){
    return asynError;
  }

  status=createParam(drvInfo,paramInfo->asynType,&index);
  if(status!=asynSuccess){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: createParam() failed.",driverName, functionName);
    return asynError;
  }
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: Parameter created at: %d for %s. \n", driverName, functionName,index,drvInfo);

  paramInfo->paramIndex=index;

  int addr=0;
  status = getAddress(pasynUser, &addr);
  if (status != asynSuccess){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: getAddress() failed.",driverName, functionName);
    return(status);
  }

  paramInfo->asynAddr=addr;

  status=parsePlcInfofromDrvInfo(drvInfo,paramInfo);
  if(status!=asynSuccess){
    return asynError;
  }
  pasynUser->timeout=(paramInfo->maxDelayTimeMS*2)/1000;

  pAdsParamArray_[adsParamArrayCount_]=paramInfo;
  adsParamArrayCount_++;

  if(!connectedAds_){
    //try to connect without error handling
    connect(pasynUser);
  }

  if(connectedAds_){
    status=updateParamInfoWithPLCInfo(paramInfo);
    if(status!=asynSuccess){
      return asynError;
    }
  }
  return asynPortDriver::drvUserCreate(pasynUser,drvInfo,pptypeName,psize); //Assigns pasynUser->reason;
}

asynStatus adsAsynPortDriver::updateParamInfoWithPLCInfo(adsParamInfo *paramInfo)
{
  const char* functionName = "updateParamInfoWithPLCInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: : %s\n", driverName, functionName,paramInfo->drvInfo);

  asynStatus status;

  // Read symbolic information if needed (to get paramInfo->plcSize)
  if(!paramInfo->plcAbsAdrValid){
    status=adsGetSymInfoByName(paramInfo);
    if(status!=asynSuccess){
      return asynError;
    }
  }

  //check if array
  bool isArray=false;
  switch (paramInfo->plcDataType) {
    case ADST_VOID:
      isArray=false;
      break;
    case ADST_STRING:
      isArray=true;  //Special case
      break;
    case ADST_WSTRING:
      isArray=true; //Special case?
      break;
    case ADST_BIGTYPE:
      isArray=false;
      break;
    case ADST_MAXTYPES:
      isArray=false;
      break;
    default:
      isArray=paramInfo->plcSize>adsTypeSize(paramInfo->plcDataType);
      break;
  }
  paramInfo->plcDataIsArray=isArray;

  // Allocate memory for array
  if(isArray){
    lock();
    if(paramInfo->plcSize!=paramInfo->arrayDataBufferSize && paramInfo->arrayDataBuffer){ //new size of array
      free(paramInfo->arrayDataBuffer);
      paramInfo->arrayDataBuffer=NULL;
    }
    if(!paramInfo->arrayDataBuffer){
      paramInfo->arrayDataBuffer = calloc(paramInfo->plcSize,1);
      paramInfo->arrayDataBufferSize=paramInfo->plcSize;
      if(!paramInfo->arrayDataBuffer){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to allocate memory for array data for %s.\n.", driverName, functionName,paramInfo->drvInfo);
        unlock();
        return asynError;
      }
      memset(paramInfo->arrayDataBuffer,0,paramInfo->plcSize);
    }
    unlock();
  }

  //Add callback only for I/O intr
  if(paramInfo->isIOIntr){
    if(paramInfo->bCallbackNotifyValid){
      adsDelNotificationCallback(paramInfo,true);   //try to delete
    }
    //Release symbolic handle if needed
    if(paramInfo->bSymbolicHandleValid){
      adsReleaseSymbolicHandle(paramInfo,true); //try to delete (dont care if fails)
    }

    status=adsAddNotificationCallback(paramInfo);
    if(status!=asynSuccess){
      return asynError;
    }
  }

  //Make first read
  status =adsReadParam(paramInfo);
  if(status!=asynSuccess){
    return asynError;
  }
  return asynSuccess;
}

asynStatus adsAsynPortDriver::getRecordInfoFromDrvInfo(const char *drvInfo,adsParamInfo *paramInfo)
{
  const char* functionName = "getRecordInfoFromDrvInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName, functionName,drvInfo);
  bool isInput=false;
  bool isOutput=false;
  paramInfo->amsPort=amsportDefault_;
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
          isInput=true;
          char port[ADS_MAX_FIELD_CHAR_LENGTH];
          int adr;
          int timeout;
          char currdrvInfo[ADS_MAX_FIELD_CHAR_LENGTH];
          int nvals=sscanf(paramInfo->inp,"@asyn(%[^,],%d,%d)%s",port,&adr,&timeout,currdrvInfo);
          if(nvals==4){
            //Ensure correct port and drvinfo
            if(strcmp(port,portName)==0 && strcmp(drvInfo,currdrvInfo)==0){
              recordFound=true;  // Correct port and drvinfo!\n");
            }
          }
        }
        else{
          isInput=false;
        }
        status=dbFindField(pdbentry,"OUT");
        if(!status){
          paramInfo->out=strdup(dbGetString(pdbentry));
          isOutput=true;
          char port[ADS_MAX_FIELD_CHAR_LENGTH];
          int adr;
          int timeout;
          char currdrvInfo[ADS_MAX_FIELD_CHAR_LENGTH];
          int nvals=sscanf(paramInfo->out,"@asyn(%[^,],%d,%d)%s",port,&adr,&timeout,currdrvInfo);
          if(nvals==4){
            //Ensure correct port and drvinfo
            if(strcmp(port,portName)==0 && strcmp(drvInfo,currdrvInfo)==0){
              recordFound=true;  // Correct port and drvinfo!\n");
            }
          }
        }
        else{
          isOutput=false;
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

          //drvInput (not a field)
          paramInfo->drvInfo=strdup(drvInfo);
          dbFreeEntry(pdbentry);
          return asynSuccess;  // The correct record was found and the paramInfo structure is filled
        }
        else{
          //Not correct record. Do cleanup.
          if(isInput){
            free(paramInfo->inp);
            paramInfo->inp=0;
          }
          if(isOutput){
            free(paramInfo->out);
            paramInfo->out=0;
          }
          paramInfo->drvInfo=0;
          paramInfo->scan=0;
          paramInfo->dtyp=0;
          isInput=false;
          isOutput=false;
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

asynStatus adsAsynPortDriver::parsePlcInfofromDrvInfo(const char* drvInfo,adsParamInfo *paramInfo)
{
  const char* functionName = "parsePlcInfofromDrvInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName, functionName,drvInfo);

  //Check if input or output
  const char* temp=strrchr(drvInfo,'?');
  if(temp){
    if(strlen(temp)==1){
      paramInfo->isIOIntr=true; //All inputs will be created I/O intr
    }
    else{ //Must be '=' in end
      paramInfo->isIOIntr=false;
    }
  }
  else{ //Must be '=' in end
    paramInfo->isIOIntr=false;
  }

  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: drvInfo %s is %s\n", driverName, functionName,drvInfo,paramInfo->isIOIntr ? "I/O Intr (end with ?)": " not I/O Intr (end with =)");

  //take part after last "/" if option or complete string..
  char buffer[ADS_MAX_FIELD_CHAR_LENGTH];
  //See if option (find last '/')
  const char *drvInfoEnd=strrchr(drvInfo,'/');
  if(drvInfoEnd){ // found '/'
    int nvals=sscanf(drvInfoEnd,"/%s",buffer);
    if(nvals==1){
      paramInfo->plcAdrStr=strdup(buffer);
      paramInfo->plcAdrStr[strlen(paramInfo->plcAdrStr)-1]=0; //Strip ? or = from end
    }
    else{
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse PLC address string from drvInfo (%s)\n", driverName, functionName,drvInfo);
      return asynError;
    }
  }
  else{  //No options
    paramInfo->plcAdrStr=strdup(drvInfo);  //Symbolic or .ADR.
    paramInfo->plcAdrStr[strlen(paramInfo->plcAdrStr)-1]=0; //Strip ? or = from end
  }

  //Check if .ADR. command
  const char *option=ADS_ADR_COMMAND_PREFIX;
  paramInfo->plcAbsAdrValid=false;
  paramInfo->isAdrCommand=false;
  const char *isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("16#%x,16#%x,%u,%u"))){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s command from drvInfo (%s). String to short.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
    paramInfo->isAdrCommand=true;
    int nvals;
    nvals = sscanf(isThere+strlen(option),"16#%x,16#%x,%u,%u",
             &paramInfo->plcAbsAdrGroup,
             &paramInfo->plcAbsAdrOffset,
             &paramInfo->plcSize,
             &paramInfo->plcDataType);

    if(nvals==4){
      paramInfo->plcAbsAdrValid=true;
    }
    else{
      paramInfo->plcAbsAdrValid=false;
      paramInfo->plcAbsAdrGroup=-1;
      paramInfo->plcAbsAdrOffset=-1;
      paramInfo->plcSize=-1;
      paramInfo->plcDataType=-1;
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s command from drvInfo (%s). Wrong format.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
  }

  //Check if ADS_OPTION_T_MAX_DLY_MS option
  option=ADS_OPTION_T_MAX_DLY_MS;
  isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("=0/"))){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    nvals = sscanf(isThere+strlen(option),"=%lf/",&paramInfo->maxDelayTimeMS);

    if(nvals!=1){
      paramInfo->maxDelayTimeMS=defaultMaxDelayTimeMS_;
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
  }

  //Check if ADS_OPTION_T_SAMPLE_RATE_MS option
  option=ADS_OPTION_T_SAMPLE_RATE_MS;
  paramInfo->sampleTimeMS=defaultSampleTimeMS_;
  isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("=0/"))){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    nvals = sscanf(isThere+strlen(option),"=%lf/",&paramInfo->sampleTimeMS);

    if(nvals!=1){
      paramInfo->sampleTimeMS=defaultSampleTimeMS_;
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
  }

  //Check if ADS_OPTION_TIMEBASE option
  option=ADS_OPTION_TIMEBASE;
  paramInfo->timeBase=ADS_TIME_BASE_EPICS;
  isThere=strstr(drvInfo,option);
  if(isThere){
    int minLen=strlen(ADS_OPTION_TIMEBASE_PLC);
    int epicsLen=strlen(ADS_OPTION_TIMEBASE_EPICS);
    if(epicsLen<minLen){
      minLen=epicsLen;
    }
    if(strlen(isThere)<(strlen(option)+strlen("=/")+minLen)){ //Allowed "PLC" or "EPICS"
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    nvals = sscanf(isThere+strlen(option),"=%[^/]/",buffer);
    if(nvals!=1){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
    //Defaults to EPICS so only need to check PLC
    if(strcmp(ADS_OPTION_TIMEBASE_PLC,buffer)==0){
      paramInfo->timeBase=ADS_TIME_BASE_PLC;
    }
  }

  //Check if ADS_OPTION_ADSPORT option
  option=ADS_OPTION_ADSPORT;
  paramInfo->amsPort=amsportDefault_;
  isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("=0/"))){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    int val;
    nvals = sscanf(isThere+strlen(option),"=%d/",&val);
    if(nvals==1){
      paramInfo->amsPort=(uint16_t)val;
    }
    else{
      paramInfo->amsPort=amsportDefault_;
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName, functionName,option,drvInfo);
      return asynError;
    }
  }

  //See if new amsPort, then update list
  bool newAmsPort=true;
  for(amsPortInfo *port : amsPortList_){
    if(port->amsPort==paramInfo->amsPort){
      newAmsPort=false;
    }
  }
  if(newAmsPort){
    try{
      amsPortInfo *newPort=new amsPortInfo();
      newPort->amsPort=paramInfo->amsPort;
      newPort->connected=0;
      amsPortList_.push_back(newPort);
      asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: Added new amsPort to amsPortList: %d .\n", driverName, functionName,(int)paramInfo->amsPort);
    }
    catch(std::exception &e)
    {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Failed to add new amsPort to list for pareameter %s. Exception: %s.\n", driverName, functionName,drvInfo,e.what());
      return asynError;
    }
  }

  return asynSuccess;
}

bool adsAsynPortDriver::isCallbackAllowed(adsParamInfo *paramInfo)
{
  return isCallbackAllowed(paramInfo->amsPort);
}

bool adsAsynPortDriver::isCallbackAllowed(uint16_t amsPort)
{
  const char* functionName = "isCallbackAllowed";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: amsPort:%u\n", driverName, functionName,amsPort);

  for(amsPortInfo *port : amsPortList_){
    if(port->amsPort==amsPort)
     return port->paramsOK;
  }
  return false;
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

asynStatus adsAsynPortDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  const char* functionName = "writeInt32";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);
  adsParamInfo *paramInfo;
  int paramIndex = pasynUser->reason;

  if(!pAdsParamArray_[paramIndex]){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: pAdsParamArray NULL\n", driverName, functionName);
    return asynError;
  }
  paramInfo=pAdsParamArray_[paramIndex];

  uint8_t buffer[8]; //largest datatype is 8bytes
  uint32_t maxBytesToWrite=0;
  // Convert epicsInt32 to plctype if possible..
  switch(paramInfo->plcDataType){
    case ADST_INT8:
      int8_t *ADST_INT8Var;
      ADST_INT8Var=((int8_t*)buffer);
      *ADST_INT8Var=(int8_t)value;
      maxBytesToWrite=1;
      break;
    case ADST_INT16:
      int16_t *ADST_INT16Var;
      ADST_INT16Var=((int16_t*)buffer);
      *ADST_INT16Var=(int16_t)value;
      maxBytesToWrite=2;
      break;
    case ADST_INT32:
      int32_t *ADST_INT32Var;
      ADST_INT32Var=((int32_t*)buffer);
      *ADST_INT32Var=(int32_t)value;
      maxBytesToWrite=4;
      break;
    case ADST_INT64:
      int64_t *ADST_INT64Var;
      ADST_INT64Var=((int64_t*)buffer);
      *ADST_INT64Var=(int64_t)value;
      maxBytesToWrite=8;
      break;
    case ADST_UINT8:
      uint8_t *ADST_UINT8Var;
      ADST_UINT8Var=((uint8_t*)buffer);
      *ADST_UINT8Var=(uint8_t)value;
      maxBytesToWrite=1;
      break;
    case ADST_UINT16:
      uint16_t *ADST_UINT16Var;
      ADST_UINT16Var=((uint16_t*)buffer);
      *ADST_UINT16Var=(uint16_t)value;
      maxBytesToWrite=2;
      break;
    case ADST_UINT32:
      uint32_t *ADST_UINT32Var;
      ADST_UINT32Var=((uint32_t*)buffer);
      *ADST_UINT32Var=(uint32_t)value;
      maxBytesToWrite=4;
      break;
    case ADST_UINT64:
      uint64_t *ADST_UINT64Var;
      ADST_UINT64Var=((uint64_t*)buffer);
      *ADST_UINT64Var=(uint64_t)value;
      maxBytesToWrite=8;
      break;
    case ADST_REAL32:
      float *ADST_REAL32Var;
      ADST_REAL32Var=((float*)buffer);
      *ADST_REAL32Var=(float)value;
      maxBytesToWrite=4;
      break;
    case ADST_REAL64:
      double *ADST_REAL64Var;
      ADST_REAL64Var=((double*)buffer);
      *ADST_REAL64Var=(double)value;
      maxBytesToWrite=8;
      break;
    case ADST_BIT:
      buffer[0]=value>0;
      maxBytesToWrite=1;
      break;
    default:
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Data types not compatible (epicsInt32 and %s). Write canceled.\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType));
      return asynError;
      break;
  }

  // Warning. Risk of loss of data..
  if(sizeof(value)>maxBytesToWrite || sizeof(value)>paramInfo->plcSize){
    asynPrint(pasynUserSelf, ASYN_TRACE_WARNING, "%s:%s: WARNING. EPICS datatype size larger than PLC datatype size (%ld vs %d bytes).\n", driverName,functionName,sizeof(value),paramInfo->plcDataType);
    paramInfo->plcDataTypeWarn=true;
  }

  //Ensure that PLC datatype and number of bytes to write match
  if(maxBytesToWrite!=paramInfo->plcSize || maxBytesToWrite==0){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Data types size missmatch (%s and %d bytes). Write canceled.\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),maxBytesToWrite);
    return asynError;
  }

  //Do the write
  if(adsWriteParam(paramInfo,(const void *)buffer,maxBytesToWrite)!=asynSuccess){
    return asynError;
  }

 return asynPortDriver::writeInt32(pasynUser, value);
}

asynStatus adsAsynPortDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  const char* functionName = "writeFloat64";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

   adsParamInfo *paramInfo;
  int paramIndex = pasynUser->reason;

  if(!pAdsParamArray_[paramIndex]){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: pAdsParamArray NULL\n", driverName, functionName);
    return asynError;
  }
  paramInfo=pAdsParamArray_[paramIndex];

  uint8_t buffer[8]; //largest datatype is 8bytes
  uint32_t maxBytesToWrite=0;
  // Convert epicsFloat64 to plctype if possible..
  switch(paramInfo->plcDataType){
    case ADST_INT8:
      int8_t *ADST_INT8Var;
      ADST_INT8Var=((int8_t*)buffer);
      *ADST_INT8Var=(int8_t)value;
      maxBytesToWrite=1;
      break;
    case ADST_INT16:
      int16_t *ADST_INT16Var;
      ADST_INT16Var=((int16_t*)buffer);
      *ADST_INT16Var=(int16_t)value;
      maxBytesToWrite=2;
      break;
    case ADST_INT32:
      int32_t *ADST_INT32Var;
      ADST_INT32Var=((int32_t*)buffer);
      *ADST_INT32Var=(int32_t)value;
      maxBytesToWrite=4;
      break;
    case ADST_INT64:
      int64_t *ADST_INT64Var;
      ADST_INT64Var=((int64_t*)buffer);
      *ADST_INT64Var=(int64_t)value;
      maxBytesToWrite=8;
      break;
    case ADST_UINT8:
      uint8_t *ADST_UINT8Var;
      ADST_UINT8Var=((uint8_t*)buffer);
      *ADST_UINT8Var=(uint8_t)value;
      maxBytesToWrite=1;
      break;
    case ADST_UINT16:
      uint16_t *ADST_UINT16Var;
      ADST_UINT16Var=((uint16_t*)buffer);
      *ADST_UINT16Var=(uint16_t)value;
      maxBytesToWrite=2;
      break;
    case ADST_UINT32:
      uint32_t *ADST_UINT32Var;
      ADST_UINT32Var=((uint32_t*)buffer);
      *ADST_UINT32Var=(uint32_t)value;
      maxBytesToWrite=4;
      break;
    case ADST_UINT64:
      uint64_t *ADST_UINT64Var;
      ADST_UINT64Var=((uint64_t*)buffer);
      *ADST_UINT64Var=(uint64_t)value;
      maxBytesToWrite=8;
      break;
    case ADST_REAL32:
      float *ADST_REAL32Var;
      ADST_REAL32Var=((float*)buffer);
      *ADST_REAL32Var=(float)value;
      maxBytesToWrite=4;
      break;
    case ADST_REAL64:
      double *ADST_REAL64Var;
      ADST_REAL64Var=((double*)buffer);
      *ADST_REAL64Var=(double)value;
      maxBytesToWrite=8;
      break;
    case ADST_BIT:
      buffer[0]=value>0;
      maxBytesToWrite=1;
      break;
    default:
      asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: Data types not compatible (epicsInt32 and %s). Write canceled.\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType));
      return asynError;
      break;
  }

  // Warning. Risk of loss of data..
  if(sizeof(value)>maxBytesToWrite || sizeof(value)>paramInfo->plcSize){
    asynPrint(pasynUser, ASYN_TRACE_WARNING, "%s:%s: WARNING. EPICS datatype size larger than PLC datatype size (%ld vs %d bytes).\n", driverName,functionName,sizeof(value),paramInfo->plcDataType);
    paramInfo->plcDataTypeWarn=true;
  }

  //Ensure that PLC datatype and number of bytes to write match
  if(maxBytesToWrite!=paramInfo->plcSize || maxBytesToWrite==0){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: Data types size mismatch (%s and %d bytes). Write canceled.\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),maxBytesToWrite);
    return asynError;
  }

  //Do the write
  if(adsWriteParam(paramInfo,(const void *)buffer,maxBytesToWrite)!=asynSuccess){
    return asynError;
  }

  return asynPortDriver::writeFloat64(pasynUser,value);
}

asynStatus adsAsynPortDriver::adsGenericArrayRead(int paramIndex,long allowedType,void *epicsDataBuffer,size_t nEpicsBufferBytes,size_t *nBytesRead)
{
  const char* functionName = "adsGenericArrayRead";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);


  if(!pAdsParamArray_[paramIndex]){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: pAdsParamArray NULL\n", driverName, functionName);
    return asynError;
  }

  adsParamInfo *paramInfo=pAdsParamArray_[paramIndex];

  //Only support same datatype as in PLC
  if(paramInfo->plcDataType!=allowedType){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Data types not compatible. Read canceled.\n", driverName, functionName);
    return asynError;
  }

  size_t bytesToWrite=nEpicsBufferBytes;
  if(paramInfo->plcSize<nEpicsBufferBytes){
    bytesToWrite=paramInfo->plcSize;
  }

  if(!paramInfo->arrayDataBuffer || !epicsDataBuffer){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Buffer(s) NULL. Read canceled.\n", driverName, functionName);
    return asynError;
  }

  memcpy(epicsDataBuffer,paramInfo->arrayDataBuffer,bytesToWrite);
  *nBytesRead=bytesToWrite;

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsGenericArrayWrite(int paramIndex,long allowedType,const void *data,size_t nEpicsBufferBytes)
{
  const char* functionName = "adsGenericArrayWrite";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);


  if(!pAdsParamArray_[paramIndex]){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: pAdsParamArray NULL\n", driverName, functionName);
    return asynError;
  }

  adsParamInfo *paramInfo=pAdsParamArray_[paramIndex];

  //Only support same datatype as in PLC
  if(paramInfo->plcDataType!=allowedType){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Data types not compatible. Write canceled.\n", driverName, functionName);
    return asynError;
  }

  size_t bytesToWrite=nEpicsBufferBytes;
  if(paramInfo->plcSize<nEpicsBufferBytes){
    bytesToWrite=paramInfo->plcSize;
  }

  //Write to ADS
  asynStatus stat=adsWriteParam(paramInfo,data,bytesToWrite);
  if(stat!=asynSuccess){
    return asynError;
  }

  //copy data to buffer;
  if(paramInfo->arrayDataBuffer){
    memcpy(paramInfo->arrayDataBuffer,data,bytesToWrite);
  }
  return asynSuccess;
}

asynStatus adsAsynPortDriver::readInt8Array(asynUser *pasynUser,epicsInt8 *value,size_t nElements,size_t *nIn)
{
  const char* functionName = "readInt8Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if(!pAdsParamArray_[pasynUser->reason]){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: pAdsParamArray NULL\n", driverName, functionName);
    return asynError;
  }

  long allowedType=ADST_INT8;

  //Also allow string as int8array (special case)
  if(pAdsParamArray_[pasynUser->reason]->plcDataType==ADST_STRING){
    allowedType=ADST_STRING;
  }

  size_t nBytesRead=0;
  asynStatus stat=adsGenericArrayRead(pasynUser->reason, allowedType,(void *)value,nElements*sizeof(epicsInt8),&nBytesRead);
  if(stat!=asynSuccess){
    return asynError;
  }
  *nIn=nBytesRead/sizeof(epicsInt8);
  return asynSuccess;
}

asynStatus adsAsynPortDriver::writeInt8Array(asynUser *pasynUser, epicsInt8 *value,size_t nElements)
{
  const char* functionName = "writeInt8Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);


  long allowedType=ADST_INT8;


  if(!pAdsParamArray_[pasynUser->reason]){
    asynPrint(pasynUser, ASYN_TRACE_ERROR, "%s:%s: pAdsParamArray NULL\n", driverName, functionName);
    return asynError;
  }

  //Also allow string as int8array (special case)
  if(pAdsParamArray_[pasynUser->reason]->plcDataType==ADST_STRING){
    allowedType=ADST_STRING;
  }

  return adsGenericArrayWrite(pasynUser->reason,allowedType,(const void *)value,nElements*sizeof(epicsInt8));
}

asynStatus adsAsynPortDriver::readInt16Array(asynUser *pasynUser,epicsInt16 *value,size_t nElements,size_t *nIn)
{
  const char* functionName = "readInt16Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_INT16;

  size_t nBytesRead=0;
  asynStatus stat=adsGenericArrayRead(pasynUser->reason, allowedType,(void *)value,nElements*sizeof(epicsInt16),&nBytesRead);
  if(stat!=asynSuccess){
    return asynError;
  }
  *nIn=nBytesRead/sizeof(epicsInt16);
  return asynSuccess;
}


asynStatus adsAsynPortDriver::writeInt16Array(asynUser *pasynUser, epicsInt16 *value,size_t nElements)
{
  const char* functionName = "writeInt16Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_INT16;

  return adsGenericArrayWrite(pasynUser->reason,allowedType,(const void *)value,nElements*sizeof(epicsInt16));
}

asynStatus adsAsynPortDriver::readInt32Array(asynUser *pasynUser,epicsInt32 *value,size_t nElements,size_t *nIn)
{
  const char* functionName = "readInt32Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_INT32;

  size_t nBytesRead=0;
  asynStatus stat=adsGenericArrayRead(pasynUser->reason, allowedType,(void *)value,nElements*sizeof(epicsInt32),&nBytesRead);
  if(stat!=asynSuccess){
    return asynError;
  }
  *nIn=nBytesRead/sizeof(epicsInt32);
  return asynSuccess;
}

asynStatus adsAsynPortDriver::writeInt32Array(asynUser *pasynUser, epicsInt32 *value,size_t nElements)
{
  const char* functionName = "writeInt32Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_INT32;

  return adsGenericArrayWrite(pasynUser->reason,allowedType,(const void *)value,nElements*sizeof(epicsInt32));
}

asynStatus adsAsynPortDriver::readFloat32Array(asynUser *pasynUser,epicsFloat32 *value,size_t nElements,size_t *nIn)
{
  const char* functionName = "readFloat32Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_REAL32;

  size_t nBytesRead=0;
  asynStatus stat=adsGenericArrayRead(pasynUser->reason, allowedType,(void *)value,nElements*sizeof(epicsFloat32),&nBytesRead);
  if(stat!=asynSuccess){
    return asynError;
  }
  *nIn=nBytesRead/sizeof(epicsFloat32);
  return asynSuccess;
}

asynStatus adsAsynPortDriver::writeFloat32Array(asynUser *pasynUser,epicsFloat32 *value,size_t nElements)
{
  const char* functionName = "writeFloat32Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_REAL32;
  return adsGenericArrayWrite(pasynUser->reason,allowedType,(const void *)value,nElements*sizeof(epicsFloat32));
}

asynStatus adsAsynPortDriver::readFloat64Array(asynUser *pasynUser,epicsFloat64 *value,size_t nElements,size_t *nIn)
{
  const char* functionName = "readFloat64Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_REAL64;

  size_t nBytesRead=0;
  asynStatus stat=adsGenericArrayRead(pasynUser->reason, allowedType,(void *)value,nElements*sizeof(epicsFloat64),&nBytesRead);
  if(stat!=asynSuccess){
    return asynError;
  }
  *nIn=nBytesRead/sizeof(epicsFloat64);
  return asynSuccess;
}

asynStatus adsAsynPortDriver::writeFloat64Array(asynUser *pasynUser,epicsFloat64 *value,size_t nElements)
{
  const char* functionName = "writeFloat64Array";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  long allowedType=ADST_REAL64;
  return adsGenericArrayWrite(pasynUser->reason,allowedType,(const void *)value,nElements*nElements*sizeof(epicsFloat64));
}

asynUser *adsAsynPortDriver::getTraceAsynUser()
{
  const char* functionName = "getTraceAsynUser";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  return pasynUserSelf;
}

asynStatus adsAsynPortDriver::adsGetSymHandleByName(adsParamInfo *paramInfo)
{
  const char* functionName = "adsGetSymHandleByName";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  //NOTE: MUST CHECK THAT handleByName folllows if adr is moved in PLC (after compile) otherwise a '
  // rtriggering of all notificatios are needed...

  uint32_t symbolHandle=0;

  const long handleStatus = AdsSyncReadWriteReqEx2(adsPort_,
                                                   &amsServer,
                                                   ADSIGRP_SYM_HNDBYNAME,
                                                   0,
                                                   sizeof(paramInfo->hSymbolicHandle),
                                                   &symbolHandle,
                                                   strlen(paramInfo->plcAdrStr),
                                                   paramInfo->plcAdrStr,
                                                   nullptr);
  if (handleStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Create handle for %s failed with: %s (%ld)\n", driverName, functionName,paramInfo->plcAdrStr,adsErrorToString(handleStatus),handleStatus);
    return asynError;
  }

  //Add handle succeded
  paramInfo->hSymbolicHandle=symbolHandle;
  paramInfo->bSymbolicHandleValid=true;

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsAddNotificationCallback(adsParamInfo *paramInfo)
{
  const char* functionName = "adsAddNotificationCallback";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  uint32_t group=0;
  uint32_t offset=0;

  paramInfo->bCallbackNotifyValid=false;

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  if(paramInfo->isAdrCommand){// Abs access (ADR command)
    if(!paramInfo->plcAbsAdrValid){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Absolute address in paramInfo not valid.\n", driverName, functionName);
      return asynError;
    }

    group=paramInfo->plcAbsAdrGroup;
    offset=paramInfo->plcAbsAdrOffset;
  }
  else{ // Symbolic access

    // Read symbolic information if needed (to get paramInfo->plcSize)
    if(!paramInfo->plcAbsAdrValid){
      asynStatus statusInfo=adsGetSymInfoByName(paramInfo);
      if(statusInfo!=asynSuccess){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymInfoByName failed.\n", driverName, functionName);
        return asynError;
      }
    }

    // Get symbolic handle if needed
    if(!paramInfo->bSymbolicHandleValid){
      asynStatus statusHandle=adsGetSymHandleByName(paramInfo);
      if(statusHandle!=asynSuccess){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymHandleByName failed.\n", driverName, functionName);
        return asynError;
      }
    }

    group=ADSIGRP_SYM_VALBYHND;  //Access via symbolic handle stored in paramInfo->hSymbolicHandle
    offset=paramInfo->hSymbolicHandle;
  }

  AdsNotificationAttrib attrib;
  /** Length of the data that is to be passed to the callback function. */
  attrib.cbLength=paramInfo->plcSize;
  /**
  * ADSTRANS_SERVERCYCLE: The notification's callback function is invoked cyclically.
  * ADSTRANS_SERVERONCHA: The notification's callback function is only invoked when the value changes.
  */
  attrib.nTransMode=ADSTRANS_SERVERONCHA;  //Add option
  /** The notification's callback function is invoked at the latest when this time has elapsed. The unit is 100 ns. */
  attrib.nMaxDelay=(uint32_t)(paramInfo->maxDelayTimeMS*10000); // 100ms
  /** The ADS server checks whether the variable has changed after this time interval. The unit is 100 ns. */
  attrib.nCycleTime=(uint32_t)(paramInfo->sampleTimeMS*10000);

  uint32_t hNotify=0;
  long addStatus = AdsSyncAddDeviceNotificationReqEx(adsPort_,
                                                     &amsServer,
                                                     group,
                                                     offset,
                                                     &attrib,
                                                     &adsNotifyCallback,
                                                     (uint32_t)paramInfo->paramIndex,
                                                     &hNotify);

  if (addStatus){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Add device notification failed with: %s (%ld)\n", driverName, functionName,adsErrorToString(addStatus),addStatus);
    return asynError;
  }

  //Add was successfull
  paramInfo->hCallbackNotify=hNotify;
  paramInfo->bCallbackNotifyValid=true;

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsDelNotificationCallback(adsParamInfo *paramInfo)
{
  return adsDelNotificationCallback(paramInfo,false);
}
asynStatus adsAsynPortDriver::adsDelNotificationCallback(adsParamInfo *paramInfo,bool blockErrorMsg)
{
  const char* functionName = "delNotificationCallback";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  const long delStatus = AdsSyncDelDeviceNotificationReqEx(adsPort_, &amsServer,paramInfo->hCallbackNotify);
  if (delStatus && !blockErrorMsg){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Delete device notification failed with: %s (%ld)\n", driverName, functionName,adsErrorToString(delStatus),delStatus);
    return asynError;
  }

  paramInfo->bCallbackNotifyValid=false;
  paramInfo->hCallbackNotify=-1;

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsGetSymInfoByName(adsParamInfo *paramInfo)
{
  const char* functionName = "getSymInfoByName";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);
  adsSymbolEntry infoStruct;

  uint32_t bytesRead=0;
  AmsAddr amsServer;

  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  const long infoStatus = AdsSyncReadWriteReqEx2(adsPort_,
                                             &amsServer,
                                             ADSIGRP_SYM_INFOBYNAMEEX,
                                             0,
                                             sizeof(adsSymbolEntry),
                                             &infoStruct,
                                             strlen(paramInfo->plcAdrStr),
                                             paramInfo->plcAdrStr,
                                             &bytesRead);

  if (infoStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Get symbolic information failed for %s with: %s (%ld)\n", driverName, functionName,paramInfo->plcAdrStr,adsErrorToString(infoStatus),infoStatus);
    return asynError;
  }

  infoStruct.variableName = infoStruct.buffer;

  if(infoStruct.nameLength>=sizeof(infoStruct.buffer)-1){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Missalignment of type in AdsSyncReadWriteReqEx2 return struct for %s: 0x%x\n", driverName, functionName,paramInfo->plcAdrStr,ADS_COM_ERROR_READ_SYMBOLIC_INFO);
    return asynError;
  }
  infoStruct.symDataType = infoStruct.buffer+infoStruct.nameLength+1;

  if(infoStruct.nameLength + infoStruct.typeLength+2>=(uint16_t)(sizeof(infoStruct.buffer)-1)){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Missalignment of comment in AdsSyncReadWriteReqEx2 return struct for %s: 0x%x\n", driverName, functionName,paramInfo->plcAdrStr,ADS_COM_ERROR_READ_SYMBOLIC_INFO);
  }
  infoStruct.symComment= infoStruct.symDataType+infoStruct.typeLength+1;

  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Symbolic information\n");
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"SymEntrylength: %d\n",infoStruct.entryLen);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"idxGroup: 0x%x\n",infoStruct.iGroup);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"idxOffset: 0x%x\n",infoStruct.iOffset);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"ByteSize: %d\n",infoStruct.size);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"adsDataType: %d\n",infoStruct.dataType);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Flags: %d\n",infoStruct.flags);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Name length: %d\n",infoStruct.nameLength);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Type length: %d\n",infoStruct.typeLength);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Type length: %d\n",infoStruct.commentLength);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Variable name: %s\n",infoStruct.variableName);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Data type: %s\n",infoStruct.symDataType);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"Comment: %s\n",infoStruct.symComment);

  //fill data in structure
  paramInfo->plcAbsAdrGroup=infoStruct.iGroup; //However hopefully this adress should never be used (use symbol intstead since safer if memory moves in plc)..
  paramInfo->plcAbsAdrOffset=infoStruct.iOffset; // -"- -"-
  paramInfo->plcSize=infoStruct.size;  //Needed also for symbolic access
  paramInfo->plcDataType=infoStruct.dataType;
  paramInfo->plcAbsAdrValid=true;

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsConnect()
{
  const char* functionName = "adsConnect";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  // add local route to your ADS Master
  const long addRouteStatus =AdsAddRoute(remoteNetId_, ipaddr_);
  if (addRouteStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Adding ADS route failed with: %s (%ld).\n", driverName, functionName,adsErrorToString(addRouteStatus),addRouteStatus);
    return asynError;
  }

  // open a new ADS port
  adsPort_ = AdsPortOpenEx();
  if (!adsPort_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s:Open ADS port failed.\n", driverName, functionName);
    return asynError;
  }

  // Update timeout
  uint32_t defaultTimeout=0;
  long status=AdsSyncGetTimeoutEx(adsPort_,&defaultTimeout);
  if(status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: AdsSyncGetTimeoutEx failed with: %s (%ld).\n", driverName, functionName,adsErrorToString(status),status);
    return asynError;
  }

  status=AdsSyncSetTimeoutEx(adsPort_,(uint32_t)adsTimeoutMS_);
  if(status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: AdsSyncSetTimeoutEx failed with: %s (%ld).\n", driverName, functionName,adsErrorToString(status),status);
    return asynError;
  }

  asynPrint(pasynUserSelf, ASYN_TRACE_INFO,"%s:%s: Update ADS sync time out from %u to %u.\n", driverName, functionName,defaultTimeout,(uint32_t)adsTimeoutMS_);

  //setAdsPort(adsPort_); //Global variable in adsCom.h used to get motor record and stream device com to work.

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsDisconnect()
{
  const char* functionName = "adsDisconnect";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if(adsPort_){ //only disconnect if connected
    const long closeStatus = AdsPortCloseEx(adsPort_);
    if (closeStatus) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Close ADS port failed with: %s (%ld)\n", driverName, functionName,adsErrorToString(closeStatus),closeStatus);
      return asynError;
    }

    AdsDelRoute(remoteNetId_);
  }

  adsPort_=0;
  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsReleaseSymbolicHandle(adsParamInfo *paramInfo)
{
  return adsReleaseSymbolicHandle(paramInfo, false);
}
asynStatus adsAsynPortDriver::adsReleaseSymbolicHandle(adsParamInfo *paramInfo, bool blockErrorMsg)
{
  const char* functionName = "adsReleaseHandle";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  paramInfo->bSymbolicHandleValid=false;

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  const long releaseStatus = AdsSyncWriteReqEx(adsPort_, &amsServer, ADSIGRP_SYM_RELEASEHND, 0, sizeof(paramInfo->hSymbolicHandle), &paramInfo->hSymbolicHandle);
  if (releaseStatus && !blockErrorMsg) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Release of handle 0x%x failed with: %s (%ld)\n", driverName, functionName,paramInfo->hSymbolicHandle,adsErrorToString(releaseStatus),releaseStatus);
    return asynError;
  }

  paramInfo->hSymbolicHandle=-1;
  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsWriteParam(adsParamInfo *paramInfo,const void *binaryBuffer,uint32_t bytesToWrite)
{
  const char* functionName = "adsWriteParam";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  // Calculate consumed time by this method
  struct timeval start, end;
  long secs_used,micros_used;
  gettimeofday(&start, NULL);

  uint32_t group=0;
  uint32_t offset=0;

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  if(paramInfo->isAdrCommand){// Abs access (ADR command)
    if(!paramInfo->plcAbsAdrValid){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Absolute address in paramInfo not valid.\n", driverName, functionName);
      return asynError;
    }

    group=paramInfo->plcAbsAdrGroup;
    offset=paramInfo->plcAbsAdrOffset;
  }
  else{ // Symbolic access

    // Read symbolic information if needed (to get paramInfo->plcSize)
    if(!paramInfo->plcAbsAdrValid){
      asynStatus statusInfo=adsGetSymInfoByName(paramInfo);
      if(statusInfo==asynError){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymInfoByName failed.\n", driverName, functionName);
        return asynError;
      }
    }

    // Get symbolic handle if needed
    if(!paramInfo->bSymbolicHandleValid){
      asynStatus statusHandle=adsGetSymHandleByName(paramInfo);
      if(statusHandle!=asynSuccess){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymHandleByName failed.\n", driverName, functionName);
        return asynError;
      }
    }

    group=ADSIGRP_SYM_VALBYHND;  //Access via symbolic handle stored in paramInfo->hSymbolicHandle
    offset=paramInfo->hSymbolicHandle;
  }

  long writeStatus= AdsSyncWriteReqEx(adsPort_,
                                      &amsServer,
                                      group,
                                      offset,
                                      paramInfo->plcSize,
                                      binaryBuffer);

  if (writeStatus) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: ADS write failed with: %s (%ld)\n", driverName, functionName,adsErrorToString(writeStatus),writeStatus);
    return asynError;
  }

  gettimeofday(&end, NULL);
  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: ADS write: micros used: %ld\n", driverName, functionName,micros_used);

  return asynSuccess;
}

asynStatus adsAsynPortDriver::adsReadParam(adsParamInfo *paramInfo)
{

  const char* functionName = "adsReadParam";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  uint32_t group=0;
  uint32_t offset=0;

  AmsAddr amsServer;
  if(paramInfo->amsPort<=0){  //Invalid amsPort try to fallback on default
    amsServer={remoteNetId_,amsportDefault_};
  }
  else{
    amsServer={remoteNetId_,paramInfo->amsPort};
  }

  if(paramInfo->isAdrCommand){// Abs access (ADR command)
    if(!paramInfo->plcAbsAdrValid){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Absolute address in paramInfo not valid.\n", driverName, functionName);
      return asynError;
    }

    group=paramInfo->plcAbsAdrGroup;
    offset=paramInfo->plcAbsAdrOffset;
  }
  else{ // Symbolic access

    // Read symbolic information if needed (to get paramInfo->plcSize)
    if(!paramInfo->plcAbsAdrValid){
      asynStatus statusInfo=adsGetSymInfoByName(paramInfo);
      if(statusInfo==asynError){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymInfoByName failed.\n", driverName, functionName);
        return asynError;
      }
    }

    // Get symbolic handle if needed
    if(!paramInfo->bSymbolicHandleValid){
      asynStatus statusHandle=adsGetSymHandleByName(paramInfo);
      if(statusHandle!=asynSuccess){
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: adsGetSymHandleByName failed.\n", driverName, functionName);
        return asynError;
      }
    }

    group=ADSIGRP_SYM_VALBYHND;  //Access via symbolic handle stored in paramInfo->hSymbolicHandle
    offset=paramInfo->hSymbolicHandle;
  }

  char *data=new char[paramInfo->plcSize];
  uint32_t bytesRead=0;
  const long statusRead = AdsSyncReadReqEx2(adsPort_,
                                     &amsServer,
                                     group,
                                     offset,
                                     paramInfo->plcSize,
                                     (void *)data,
                                     &bytesRead);

  if(statusRead){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: AdsSyncReadReqEx2 failed: %s (%lu).\n", driverName, functionName,adsErrorToString(statusRead),statusRead);
    return asynError;
  }

  if(bytesRead!=paramInfo->plcSize){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Read bytes differ from parameter plc size (%u vs %u).\n", driverName, functionName,bytesRead,paramInfo->plcSize);
    return asynError;
  }

  return adsUpdateParameter(paramInfo,(const void *)data,bytesRead);
}

asynStatus adsAsynPortDriver::adsReadState(uint16_t *adsState)
{
  return adsReadState(amsportDefault_,adsState);
}

asynStatus adsAsynPortDriver::adsReadState(uint16_t amsport,uint16_t *adsState)
{

  const char* functionName = "adsReadState";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  AmsAddr amsServer={remoteNetId_,amsport};

  uint16_t devState;

  const long status = AdsSyncReadStateReqEx(adsPort_, &amsServer, adsState, &devState);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: ADS read state failed with: %s (%ld)\n",driverName, functionName,adsErrorToString(status),status);
    return asynError;
  }

  return asynSuccess;
}

int adsAsynPortDriver::getParamTableSize()
{
  return paramTableSize_;
}

adsParamInfo *adsAsynPortDriver::getAdsParamInfo(int index)
{
  const char* functionName = "getAdsParamInfo";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s: Get paramInfo for index: %d\n", driverName, functionName,index);

  if(index<adsParamArrayCount_){
    return pAdsParamArray_[index];
  }
  else{
    return NULL;
  }
}

asynStatus adsAsynPortDriver::adsUpdateParameterLock(adsParamInfo* paramInfo,const void *data,size_t dataSize)
{
  lock();
  asynStatus stat=adsUpdateParameter(paramInfo,data,dataSize);
  unlock();
  return stat;
}

asynStatus adsAsynPortDriver::adsUpdateParameter(adsParamInfo* paramInfo,const void *data,size_t dataSize)
{

  const char* functionName = "adsUpdateParameter";
  asynPrint(pasynUserSelf, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  if(!paramInfo){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: paramInfo NULL.\n", driverName, functionName);
    return asynError;
  }

  if(!data){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: data NULL.\n", driverName, functionName);
    return asynError;
  }

  //Write size used for arrays
  size_t writeSize=dataSize;
  if(paramInfo->plcSize<dataSize){
    writeSize=paramInfo->plcSize;
  }

  //Update time stamp (later take timestamp from callback instead somehow)
  if(updateTimeStamp()!=asynSuccess){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: updateTimeStamp() failed.\n", driverName, functionName);
    return asynError;
  }

  asynStatus ret=asynError;

  //Check if array
  if(paramInfo->plcDataIsArray){
    if(!paramInfo->arrayDataBuffer){
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Array but buffer is NULL.\n", driverName, functionName);
      return asynError;
    }
    //Copy data to param buffer
    memcpy(paramInfo->arrayDataBuffer,data,writeSize);
  }

  switch(paramInfo->plcDataType){
    case ADST_INT8:
      int8_t *ADST_INT8Var;
      ADST_INT8Var=((int8_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_INT8Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_INT8Var));
          break;
        case asynParamInt8Array:
          ret=doCallbacksInt8Array((epicsInt8 *)paramInfo->arrayDataBuffer,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_INT16:
      int16_t *ADST_INT16Var;
      ADST_INT16Var=((int16_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_INT16Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_INT16Var));
          break;
        case asynParamInt16Array:
          ret=doCallbacksInt16Array((epicsInt16 *)paramInfo->arrayDataBuffer,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;
    case ADST_INT32:
      int32_t *ADST_INT32Var;
      ADST_INT32Var=((int32_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_INT32Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_INT32Var));
          break;
        case asynParamInt32Array:
          ret=doCallbacksInt32Array((epicsInt32 *)paramInfo->arrayDataBuffer,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_INT64:
      int64_t *ADST_INT64Var;
      ADST_INT64Var=((int64_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_INT64Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_INT64Var));
          break;
        // No 64 bit int array callback type (also no 64bit int in EPICS)
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_UINT8:
      uint8_t *ADST_UINT8Var;
      ADST_UINT8Var=((uint8_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_UINT8Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_UINT8Var));
          break;
        // Arrays of unsigned not supported
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_UINT16:
      uint16_t *ADST_UINT16Var;
      ADST_UINT16Var=((uint16_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_UINT16Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_UINT16Var));
          break;
        // Arrays of unsigned not supported
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_UINT32:
      uint32_t *ADST_UINT32Var;
      ADST_UINT32Var=((uint32_t*)data);
      //Asyn types
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_UINT32Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_UINT32Var));
          break;
        // Arrays of unsigned not supported
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_UINT64:
      uint64_t *ADST_UINT64Var;
      ADST_UINT64Var=((uint64_t*)data);
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_UINT64Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_UINT64Var));
          break;
        // Arrays of unsigned not supported
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_REAL32:
      float *ADST_REAL32Var;
      ADST_REAL32Var=((float*)data);
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_REAL32Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_REAL32Var));
          break;
        case asynParamFloat32Array:
          ret=doCallbacksFloat32Array((epicsFloat32 *)paramInfo->arrayDataBuffer,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_REAL64:
      double *ADST_REAL64Var;
      ADST_REAL64Var=((double*)data);

      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_REAL64Var));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_REAL64Var));
          break;
        case asynParamFloat64Array:
          ret=doCallbacksFloat64Array((epicsFloat64 *)paramInfo->arrayDataBuffer,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_BIT:
      int8_t *ADST_BitVar;
      ADST_BitVar=((int8_t*)data);
      switch(paramInfo->asynType){
        case asynParamInt32:
          ret=setIntegerParam(paramInfo->paramIndex,(int)(*ADST_BitVar));
          break;
        case asynParamFloat64:
          ret=setDoubleParam(paramInfo->paramIndex,(double)(*ADST_BitVar));
          break;
        case asynParamInt8Array:
          ret=doCallbacksInt8Array((epicsInt8 *) ADST_BitVar,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
      }
      break;

    case ADST_STRING:
      int8_t *ADST_STRINGVar;
      ADST_STRINGVar=((int8_t*)data);
      switch(paramInfo->asynType){
        case asynParamInt8Array:
          ret=doCallbacksInt8Array((epicsInt8 *) ADST_STRINGVar,writeSize, paramInfo->paramIndex,paramInfo->asynAddr);
          break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
          return asynError;
          break;
        }
      break;

    default:
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: Type combination not supported. PLC type = %s, ASYN type= %s\n", driverName, functionName,adsTypeToString(paramInfo->plcDataType),asynTypeToString(paramInfo->asynType));
      return asynError;
      break;
  }

  if(ret!=asynSuccess){
    return ret;
  }

  ret=callParamCallbacks();
  if(ret!=asynSuccess){
    return ret;
  }

  return asynSuccess;
}

/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

  //static initHookFunction myHookFunction;


  asynUser *pPrintOutAsynUser;

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
                             int noProcessEos,
                             int defaultSampleTimeMS,
                             int maxDelayTimeMS,
                             int adsTimeoutMS)
  {

    if (!portName) {
      printf("adsAsynPortDriverConfigure bad portName: %s\n",
             portName ? portName : "");
      return -1;
    }
    if (!ipaddr) {
      printf("adsAsynPortDriverConfigure bad ipaddr: %s\n",ipaddr ? ipaddr : "");
      return -1;
    }
    if (!amsaddr) {
      printf("adsAsynPortDriverConfigure bad amsaddr: %s\n",amsaddr ? amsaddr : "");
      return -1;
    }
    if (!defaultSampleTimeMS) {
      printf("adsAsynPortDriverConfigure bad defaultSampleTimeMS: %dms. Standard value of 100ms will be used.\n",defaultSampleTimeMS);
      defaultSampleTimeMS=100;
    }

    if (!maxDelayTimeMS) {
      printf("adsAsynPortDriverConfigure bad maxDelayTimeMS: %dms. Standard value of 500ms will be used.\n",maxDelayTimeMS);
      maxDelayTimeMS=500;
    }

    if (!adsTimeoutMS) {
      printf("adsAsynPortDriverConfigure bad adsTimeoutMS: %dms. Standard value of 2000ms will be used.\n",adsTimeoutMS);
      adsTimeoutMS=2000;
    }

    adsAsynPortObj=new adsAsynPortDriver(portName,
                                         ipaddr,
                                         amsaddr,
                                         amsport,
                                         asynParamTableSize,
                                         priority,
                                         noAutoConnect==0,
                                         noProcessEos,
                                         defaultSampleTimeMS,
                                         maxDelayTimeMS,
                                         adsTimeoutMS);
    if(adsAsynPortObj){
      asynUser *traceUser= adsAsynPortObj->getTraceAsynUser();
      if(!traceUser){
        printf("adsAsynPortDriverConfigure: ERROR: Failed to retrieve asynUser for trace. \n");
        return (asynError);
      }
      pPrintOutAsynUser=traceUser;
    }

    initHook();

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
  static const iocshArg adsAsynPortDriverConfigureArg8 = { "default sample time ms",iocshArgInt};
  static const iocshArg adsAsynPortDriverConfigureArg9 = { "max delay time ms",iocshArgInt};
  static const iocshArg adsAsynPortDriverConfigureArg10 = { "ADS communication timeout ms",iocshArgInt};
  static const iocshArg *adsAsynPortDriverConfigureArgs[] = {
    &adsAsynPortDriverConfigureArg0, &adsAsynPortDriverConfigureArg1,
    &adsAsynPortDriverConfigureArg2, &adsAsynPortDriverConfigureArg3,
    &adsAsynPortDriverConfigureArg4, &adsAsynPortDriverConfigureArg5,
    &adsAsynPortDriverConfigureArg6, &adsAsynPortDriverConfigureArg7,
    &adsAsynPortDriverConfigureArg8,&adsAsynPortDriverConfigureArg9,
    &adsAsynPortDriverConfigureArg10};

  static const iocshFuncDef adsAsynPortDriverConfigureFuncDef =
    {"adsAsynPortDriverConfigure",11,adsAsynPortDriverConfigureArgs};

  static void adsAsynPortDriverConfigureCallFunc(const iocshArgBuf *args)
  {
    adsAsynPortDriverConfigure(args[0].sval,args[1].sval,args[2].sval,args[3].ival, args[4].ival, args[5].ival,args[6].ival,args[7].ival,args[8].ival,args[9].ival,args[10].ival);
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
