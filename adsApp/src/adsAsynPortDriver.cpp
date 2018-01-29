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

#include "adsCom.h"


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
  eventId_ = epicsEventCreate(epicsEventEmpty);
  setCfgData(portName,ipaddr,amsaddr,amsport,priority,autoConnect,noProcessEos);
  //pParamTable_=new asynParamString_t[paramTableSize];
  //paramTableSize_=paramTableSize;
}

void adsAsynPortDriver::report(FILE *fp, int details)
{
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

  int error=adsDisconnect();
  if (error){
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,"adsAsynPortDriver::disconnect: ERROR: Disconnect failed (0x%x).\n",error);
    return asynError;
  }
  return asynSuccess;
}

asynStatus adsAsynPortDriver::connectIt( asynUser *pasynUser)

{
  //epicsMutexLockStatus mutexLockStatus;
  int res;
  int connectOK;

  //(void)pasynUser;
  //mutexLockStatus = epicsMutexLock(adsController_p->mutexId);
  //if (mutexLockStatus != epicsMutexLockOK) {
  //  return asynError;
  //}

  lock();

  //if (adsController_p->connected) {
  //  epicsMutexUnlock(adsController_p->mutexId);
  //  return asynSuccess;
  //}

  /* adsConnect() returns 0 if failed */
  res = adsConnect(ipaddr_,amsaddr_,amsport_);

  connectOK =  res >= 0;
  //if (connectOK) adsController_p->connected = 1;
  //epicsMutexUnlock(adsController_p->mutexId);

  unlock();

  return connectOK ? asynSuccess : asynError;
}

asynStatus adsAsynPortDriver::connect(asynUser *pasynUser)
{
  const char* functionName = "connect";
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s:\n", driverName, functionName);

  asynStatus status = connectIt(pasynUser);
  if (status == asynSuccess){
    //pasynManager->exceptionConnect(pasynUser);
    asynPortDriver::connect(pasynUser);
  }

  return status;
}

asynStatus adsAsynPortDriver::drvUserCreate(asynUser *pasynUser,const char *drvInfo,const char **pptypeName,size_t *psize)
{
  const char* functionName = "drvUserCreate";

  asynPortDriver::drvUserCreate(pasynUser,drvInfo,pptypeName,psize);

  //pasynUser->drvUser->


  //asynStandardInterfaces *pInterfaces = this->pasynPortDriver->getAsynStdInterfaces();
  asynStandardInterfaces *pIF = getAsynStdInterfaces();
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: pIF->float64  %s !!\n", driverName, functionName,pIF->float64.interfaceType);
  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: pIF->int32  %s!!\n", driverName, functionName,pIF->int32.interfaceType);


  //devPvtCommon *pPvt = (devPvtCommon *)pasynUser->userPvt;  // VERY UGLY CODE.. NEED TO GET RECORD INFO vi name?
  //dbCommon *pr = (dbCommon *)pPvt->pr;

  //DBENTRY *pdbentry = dbAllocEntry(pdbbase);
  //asynStatus status = dbFindRecord(pdbentry, pr->name);
  //if (status) {
  //  asynPrint(pPvt->pasynUser, ASYN_TRACE_ERROR,"%s %s::%s error finding record\n",pr->name, driverName, functionName);
  //  return asynError;
  //}

  //dbCommon *pr = (dbCommon *)pasynUser->userPvt->pr;

  dbAddr  paddr;
  dbNameToAddr("ADS_IOC::GetFTest3",&paddr);



  //asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: RECORDNAME: %s. HEPP\n", driverName, functionName,pr->name);
  asynPrint(pasynUser, ASYN_TRACE_INFO,"####################################\n");

  asynPrint(pasynUser, ASYN_TRACE_INFO, "%s:%s: drvInfo=%s. HEPP\n", driverName, functionName,drvInfo);

  asynInterface *pasynInterface = pasynManager->findInterface(pasynUser,asynCommonType,1);

  asynPrint(pasynUser, ASYN_TRACE_INFO,"%s pinterface %p drvPvt %p\n",pasynInterface->interfaceType, pasynInterface->pinterface,pasynInterface->drvPvt);

  //asynCommon *pasynCommon = (asynCommon *)pasynInterface->pinterface;
  //void *drvPvt = pasynInterface->drvPvt;
  //int isConnected;

  const char *data;
  size_t writeSize=1024;

  asynStatus status=drvUserGetType(pasynUser,&data,&writeSize);

  asynPrint(pasynUser, ASYN_TRACE_INFO,"STATUS = %d, data= %s, size=%d\n",status,data,writeSize);


//char *errorMessage;
//int errorMessageSize;
///* timeout must be set by the user */
//double timeout; /* Timeout for I/O operations*/
//void *userPvt;
//void *userData;
///* The following is for use by driver */
//void *drvUser;
///* The following is normally set by driver via asynDrvUser->create() */
//int reason;
//epicsTimeStamp timestamp;
///* The following are for additional information from method calls */
//int auxStatus; /* For auxillary status*/
//int alarmStatus; /* Typically for EPICS record alarm status */
//int alarmSeverity; /* Typically for EPICS record alarm severity */

  int yesNo=-1;
  pasynManager->isConnected(pasynUser, &yesNo);

  asynPrint(pasynUser, ASYN_TRACE_INFO, "HHHHHHHHHHHHHHHHHHHHHH CONNECTED: %d\n",yesNo);
  int index=0;
  status=findParam(drvInfo,&index);
  if(status==asynSuccess){
    asynPrint(pasynUser, ASYN_TRACE_INFO, "PAARMETER INDEX FOUND AT: %d for %s. \n",index,drvInfo);
  }
  else{
    asynPrint(pasynUser, ASYN_TRACE_INFO, "PAARMETER INDEX NOT FOUND for %s.\n",drvInfo);
    status=createParam(drvInfo,asynParamFloat64,&index);
    if(status==asynSuccess){
      asynPrint(pasynUser, ASYN_TRACE_INFO, "PARAMETER CREATED AT: %d for %s. \n",index,drvInfo);
    }
    else{
      asynPrint(pasynUser, ASYN_TRACE_INFO, "CREATE PARAMETER FAILED for %s.\n",drvInfo);
    }
  }

  asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->errorMessage=%s.\n",pasynUser->errorMessage);
  asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->timeout=%lf.\n",pasynUser->timeout);
  asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->reason=%d.\n",pasynUser->reason);
  asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->auxStatus=%d.\n",pasynUser->auxStatus);
  asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->alarmStatus=%d.\n",pasynUser->alarmStatus);
  asynPrint(pasynUser, ASYN_TRACE_INFO, "pasynUser->alarmSeverity=%d.\n",pasynUser->alarmSeverity);
  dbDumpRecords();
  //return this->drvUserCreateParam(pasynUser, drvInfo, pptypeName, psize,pParamTable_, paramTableSize_);
  //pasynManager->report(stdout,1,"ADS_1");
  return asynSuccess;
}
void adsAsynPortDriver::dbDumpRecords()
{
  DBENTRY *pdbentry;
  long status;

  pdbentry = dbAllocEntry(pdbBase_);
  status = dbFirstRecordType(pdbentry);
  if(status) {printf("No␣record␣descriptions\n");return;}
  while(!status) {
    printf("record␣type:␣%s",dbGetRecordTypeName(pdbentry));
    status = dbFirstRecord(pdbentry);
    if(status) printf("␣␣No␣Records\n");
      while(!status) {
       if(dbIsAlias(pdbentry))
         printf("\n␣␣Alias:%s\n",dbGetRecordName(pdbentry));
       else {
         printf("\n␣␣Record:%s\n",dbGetRecordName(pdbentry));
         status = dbFirstField(pdbentry,TRUE);
         if(status) printf("␣␣␣␣No␣Fields\n");
           while(!status) {
             printf("␣␣␣␣%s:␣%s",dbGetFieldName(pdbentry),
             dbGetString(pdbentry));
             status=dbNextField(pdbentry,TRUE);
           }
         }
         status = dbNextRecord(pdbentry);
      }
    status = dbNextRecordType(pdbentry);
  }
  printf("End␣of␣all␣Records\n");
  dbFreeEntry(pdbentry);
}

asynStatus adsAsynPortDriver::readOctet(asynUser *pasynUser, char *value, size_t maxChars,size_t *nActual, int *eomReason)
{
  const char* functionName = "readOctet";
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

asynStatus adsAsynPortDriver::setCfgData(const char *portName,
                                         const char *ipaddr,
                                         const char *amsaddr,
                                         unsigned int amsport,
                                         unsigned int priority,
                                         int noAutoConnect,
                                         int noProcessEos)
{
  portName_=portName;
  ipaddr_=ipaddr;
  amsaddr_=amsaddr;
  amsport_=amsport;
  priority_=priority;
  noAutoConnect_=noAutoConnect;
  noProcessEos_=noProcessEos;
  return asynSuccess;
}

void adsAsynPortDriver::setDbBase(DBBASE *pdbbase)
{
  pdbBase_=pdbbase;
}

/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

  extern DBBASE *pdbBase;
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
      adsAsynPortObj->connect(traceUser);
      adsAsynPortObj->setDbBase(pdbBase);

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



