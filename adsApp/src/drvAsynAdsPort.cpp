/**********************************************************************
 * Asyn device support for the ADS
 **********************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <osiUnistd.h>
#include <osiSock.h>
/*
 * cmd.h before all EPICS stuff, otherwise
   __attribute__((format (printf,1,2)))
   will not work
*/

#include "adsCom.h"

#include <cantProceed.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsAssert.h>
#include <epicsExit.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <osiUnistd.h>

#include <epicsExport.h>


#include "asynDriver.h"
#include "asynOctet.h"
#include "asynInterposeCom.h"
#include "asynInterposeEos.h"
#include "cmd.h"

/*
 * This structure holds the hardware-specific information for a single
 * asyn link.
 */
typedef struct {
  char              *portName;
  asynUser          *pasynUser;        /* Not currently used */

  epicsMutexId       mutexId;
  asynInterface      common;
  asynInterface      octet;
  int                connected;
  char *ipaddr;
  char *amsaddr;
  unsigned int amsport;
} adsController_t;



/*
 * Close a connection
 */
static void
closeConnection(asynUser *pasynUser,adsController_t *adsController_p, const char *why, int error)
{
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s close connection (%s,0x%x)\n",
            adsController_p->portName, why, error);
  adsDisconnect();
  adsController_p->connected = 0;
#if 0
  /* TODO: prevent a reconnect */
  if (!(tty->flags & FLAG_CONNECT_PER_TRANSACTION) ||
      (tty->flags & FLAG_SHUTDOWN))
    pasynManager->exceptionDisconnect(pasynUser);
#endif

}

/*Beginning of asynCommon methods*/
/*
 * Report link parameters
 */
static void
asynCommonReport(void *drvPvt, FILE *fp, int details)
{
  adsController_t *adsController_p = (adsController_t *)drvPvt;

  assert(adsController_p);
  if (details >= 1) {
    fprintf(fp, "    Port %s\n",
            adsController_p->portName);
  }
}

/*
 * Clean up a connection on exit
 */
static void
cleanup (void *arg)
{
  ;
}

/*
 * Create a link
 */
static asynStatus
connectIt(void *drvPvt, asynUser *pasynUser)

{
  adsController_t *adsController_p = (adsController_t *)drvPvt;
  epicsMutexLockStatus mutexLockStatus;

  int res;
  int connectOK;

  (void)pasynUser;
  mutexLockStatus = epicsMutexLock(adsController_p->mutexId);
  if (mutexLockStatus != epicsMutexLockOK) {
    return asynError;
  }

  if (adsController_p->connected) {
    epicsMutexUnlock(adsController_p->mutexId);
    return asynSuccess;
  }
  /* adsConnect() returns 0 if failed */
  res = adsConnect(adsController_p->ipaddr,adsController_p->amsaddr,
                   adsController_p->amsport);
  connectOK =  res >= 0;
  if (connectOK) adsController_p->connected = 1;
  epicsMutexUnlock(adsController_p->mutexId);

  return connectOK ? asynSuccess : asynError;
}

static asynStatus
asynCommonConnect(void *drvPvt, asynUser *pasynUser)
{
  asynStatus status = connectIt(drvPvt, pasynUser);
  if (status == asynSuccess)
    pasynManager->exceptionConnect(pasynUser);
  return status;
}

static asynStatus
asynCommonDisconnect(void *drvPvt, asynUser *pasynUser)
{
  adsController_t *adsController_p = (adsController_t *)drvPvt;

  assert(adsController_p);
  printf("DDDDDDDDD %s/%s:%d\n",
         __FILE__, __FUNCTION__, __LINE__);
  closeConnection(pasynUser, adsController_p, "asynCommonDisconnect", 0);
  adsDisconnect();
  return asynSuccess;
}

/* asynOctet methods */
static asynStatus writeIt(void *drvPvt, asynUser *pasynUser,
                          const char *data,
                          size_t numchars,
                          size_t *nbytesTransfered)
{
  adsController_t *adsController_p = (adsController_t *)drvPvt;
  size_t thisWrite = 0;
  asynStatus status = asynError;
  int error;
  epicsMutexLockStatus mutexLockStatus;

  assert(adsController_p);
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s write.\n", adsController_p->portName);
  asynPrintIO(pasynUser, ASYN_TRACEIO_DRIVER, data, numchars,
              "%s write %lu\n",
              adsController_p->portName,
              (unsigned long)numchars);
  *nbytesTransfered = 0;

  if (numchars == 0) {
    return asynSuccess;
  }
  mutexLockStatus = epicsMutexLock(adsController_p->mutexId);
  if (mutexLockStatus != epicsMutexLockOK) return(asynError);
  error = CMDwriteIt(data, numchars);
  if (error) {
    closeConnection(pasynUser, adsController_p, "WriteIt", error);
  } else {
    thisWrite = numchars;
    *nbytesTransfered = thisWrite;
    status = asynSuccess;
  }
  epicsMutexUnlock(adsController_p->mutexId);

  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s wrote %lu return %s.\n",
            adsController_p->portName,
            (unsigned long)*nbytesTransfered,
            pasynManager->strStatus(status));
  return status;
}

static asynStatus readIt(void *drvPvt,
                         asynUser *pasynUser,
                         char *data,
                         size_t maxchars,
                         size_t *nbytesTransfered,
                         int *gotEom)
{
  adsController_t *adsController_p = (adsController_t *)drvPvt;
  size_t thisRead = 0;
  int reason = 0;
  asynStatus status = asynSuccess;
  epicsMutexLockStatus mutexLockStatus;
  long readStatus;

  assert(adsController_p);

  /*
   * Feed what writeIt() gave us into the MCU
   */
  *data = '\0';
  mutexLockStatus = epicsMutexLock(adsController_p->mutexId);
  if (mutexLockStatus != epicsMutexLockOK) return(asynError);
  readStatus = CMDreadIt(data, maxchars);
  epicsMutexUnlock(adsController_p->mutexId);
  if (readStatus) {
    status = asynError;
    asynPrint(pasynUser, ASYN_TRACE_ERROR|ASYN_TRACEIO_DRIVER,
              "%s readStatus=0x%lx\n",
              adsController_p->portName,
              readStatus);
  } else {
    thisRead = strlen(data);
    *nbytesTransfered = thisRead;
    /* May be not enough space ? */
    if (thisRead > maxchars -1)  reason |= ASYN_EOM_CNT;

    if (gotEom) *gotEom = reason;

    if (thisRead == 0 && pasynUser->timeout == 0){
      status = asynTimeout;
    }
    *nbytesTransfered = thisRead;
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s thisRead=%lu data=\"%s\"\n",
              adsController_p->portName,
              (unsigned long)thisRead, data);
  }
  return status;
}

/*
 * Flush pending input
 */
static asynStatus
flushIt(void *drvPvt,asynUser *pasynUser)
{
  adsController_t *adsController_p = (adsController_t *)drvPvt;

  assert(adsController_p);
  asynPrint(pasynUser, ASYN_TRACE_FLOW, "%s flush\n",
            adsController_p->portName);
  return asynSuccess;
}

/*
 * Clean up a adsController_pController
 */
static void
adsController_pCleanup(adsController_t *adsController_p)
{
  if (adsController_p) {
    free(adsController_p->portName);
    free(adsController_p->ipaddr);
    free(adsController_p->amsaddr);
    free(adsController_p);
  }
}

/*
 * asynCommon methods
 */
static const struct asynCommon drvAsynAdsPortAsynCommon = {
  asynCommonReport,
  asynCommonConnect,
  asynCommonDisconnect
};

/*
 * Configure and register
 */
epicsShareFunc int
drvAsynAdsPortConfigure(const char *portName,
			const char *ipaddr,
			const char *amsaddr,
			unsigned int amsport,
                        unsigned int priority,
                        int noAutoConnect,
                        int noProcessEos)
{
  adsController_t *adsController_p;
  asynInterface *pasynInterface;
  asynStatus status;
  size_t nbytes;
  void *allocp;
  asynOctet *pasynOctet;
  printf("%s/%s:%d asynport=%s ipaddr=%s amsaddr=%s amsport=%u priority=%u noAutoConnect=%d noProcessEos=%d\n",
         __FILE__, __FUNCTION__, __LINE__,
         portName ? portName : "",
         ipaddr,
	 amsaddr,
         amsport,
         priority,
         noAutoConnect,
         noProcessEos);

  if (!portName) {
    printf("drvAsynAdsPortConfigure bad parameter %s\n",
           portName ? portName : "");
    return -1;
  }

  /* Create a driver  */
  nbytes = sizeof(adsController_t) + sizeof(asynOctet);
  allocp = callocMustSucceed(1, nbytes,
                             "drvAsynAdsPortConfigure()");
  adsController_p = (adsController_t *)allocp;

  adsController_p->ipaddr = epicsStrDup(ipaddr);
  adsController_p->amsaddr = epicsStrDup(amsaddr);
  adsController_p->amsport = amsport;

  adsController_p->mutexId = epicsMutexCreate();
  if (!adsController_p->mutexId) {
    printf("ERROR: epicsMutexCreate failure %s\n", portName);
    return -1;
  }

  pasynOctet = (asynOctet *)(adsController_p+1);
  adsController_p->portName = epicsStrDup(portName);

  /* Link with higher level routines */
  allocp = callocMustSucceed(2, sizeof *pasynInterface,
                             "drvAsynAdsPortConfigure");

  pasynInterface = (asynInterface *)allocp;
  adsController_p->common.interfaceType = asynCommonType;
  adsController_p->common.pinterface  = (void *)&drvAsynAdsPortAsynCommon;
  adsController_p->common.drvPvt = adsController_p;
  if (pasynManager->registerPort(adsController_p->portName,
                                 ASYN_CANBLOCK,
                                 !noAutoConnect,
                                 priority,
                                 0) != asynSuccess) {
    printf("drvAsynAdsPortConfigure: Can't register port %s\n",
           portName);
    adsController_pCleanup(adsController_p);
    return -1;
  }
  /* Register interface */
  status = pasynManager->registerInterface(adsController_p->portName,
                                           &adsController_p->common);
  if(status != asynSuccess) {
    printf("drvAsynAdsPortConfigure: Can't register interface %s\n",
           portName);
    adsController_pCleanup(adsController_p);
    return -1;
  }
  pasynOctet->read = readIt;
  pasynOctet->write = writeIt;
  pasynOctet->flush = flushIt;
  adsController_p->octet.interfaceType = asynOctetType;
  adsController_p->octet.pinterface  = pasynOctet;
  adsController_p->octet.drvPvt = adsController_p;
  status = pasynOctetBase->initialize(adsController_p->portName,
                                      &adsController_p->octet, 0, 0, 1);
  printf("XXXXXXX %s/%s:%d %s\n",
         __FILE__, __FUNCTION__, __LINE__,
         portName ? portName : "");
  if(status != asynSuccess) {
    printf("EEEEEEEEE drvAsynAdsPortConfigure: pasynOctetBase->initialize failed.\n");
    adsController_pCleanup(adsController_p);
    return -1;
  }

  asynInterposeEosConfig(adsController_p->portName, -1, 1, 1);

  adsController_p->pasynUser = pasynManager->createAsynUser(0,0);
  status = pasynManager->connectDevice(adsController_p->pasynUser,
                                       adsController_p->portName, -1);
  if(status != asynSuccess) {
    printf("connectDevice failed %s\n",
           adsController_p->pasynUser->errorMessage);
    cleanup(adsController_p);
    return -1;
  }
  /*
   * Register for cleanup
   */
  epicsAtExit(cleanup, adsController_p);

  //Open ads/ams connection
  return 0;
}

/*
 * IOC shell command registration
 */
static const iocshArg drvAsynAdsPortConfigureArg0 = { "port name",iocshArgString};
static const iocshArg drvAsynAdsPortConfigureArg1 = { "ip-addr",iocshArgString};
static const iocshArg drvAsynAdsPortConfigureArg2 = { "ams-addr",iocshArgString};
static const iocshArg drvAsynAdsPortConfigureArg3 = { "default-ams-port",iocshArgInt};
static const iocshArg drvAsynAdsPortConfigureArg4 = { "priority",iocshArgInt};
static const iocshArg drvAsynAdsPortConfigureArg5 = { "disable auto-connect",iocshArgInt};
static const iocshArg drvAsynAdsPortConfigureArg6 = { "noProcessEos",iocshArgInt};
static const iocshArg *drvAsynAdsPortConfigureArgs[] = {
  &drvAsynAdsPortConfigureArg0, &drvAsynAdsPortConfigureArg1,
  &drvAsynAdsPortConfigureArg2, &drvAsynAdsPortConfigureArg3,
  &drvAsynAdsPortConfigureArg4, &drvAsynAdsPortConfigureArg5,
  &drvAsynAdsPortConfigureArg6};
static const iocshFuncDef drvAsynAdsPortConfigureFuncDef =
  {"drvAsynAdsPortConfigure",7,drvAsynAdsPortConfigureArgs};

static void drvAsynAdsPortConfigureCallFunc(const iocshArgBuf *args)
{
  drvAsynAdsPortConfigure(args[0].sval,args[1].sval,args[2].sval,args[3].ival, args[4].ival, args[5].ival,args[6].ival);
}

/*
 * This routine is called before multitasking has started, so there's
 * no race condition in the test/set of firstTime.
 */
static void
drvAsynAdsPortRegisterCommands(void)
{
  iocshRegister(&drvAsynAdsPortConfigureFuncDef,drvAsynAdsPortConfigureCallFunc);
}

extern "C" {
  epicsExportRegistrar(drvAsynAdsPortRegisterCommands);
}
