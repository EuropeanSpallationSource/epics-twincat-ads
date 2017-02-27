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

  asynInterface      common;
  asynInterface      octet;
} adsController_t;


const char *portName_;
const char *ipaddr_;
const char *amsaddr_;
unsigned int amsport_;
unsigned int priority_;
int noAutoConnect_;
int noProcessEos_;

/*
 * Close a connection
 */
static void
closeConnection(asynUser *pasynUser,adsController_t *adsController_p)
{
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s close connection\n",
            adsController_p->portName);
  adsDisconnect();
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
  (void)drvPvt;
  (void)pasynUser;
  if(adsConnect(ipaddr_,amsaddr_,amsport_)<0)
    return asynError;
  else
    return asynSuccess;
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
  closeConnection(pasynUser, adsController_p);
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

  assert(adsController_p);
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s write.\n", adsController_p->portName);
  asynPrintIO(pasynUser, ASYN_TRACEIO_DRIVER, data, numchars,
              "%s write %lu\n",
              adsController_p->portName,
              (unsigned long)numchars);
  *nbytesTransfered = 0;

  if (numchars == 0){
    return asynSuccess;
  }
  if (!(CMDwriteIt(data, numchars))) {
    thisWrite = numchars;
    *nbytesTransfered = thisWrite;
    status = asynSuccess;
  }

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

  assert(adsController_p);

  /*
   * Feed what writeIt() gave us into the MCU
   */
  *data = '\0';
  if (CMDreadIt(data, maxchars)) status = asynError;
  if (status == asynSuccess) {
    thisRead = strlen(data);
    *nbytesTransfered = thisRead;
    /* May be not enough space ? */
    if (thisRead > maxchars -1)  reason |= ASYN_EOM_CNT;

    if (gotEom) *gotEom = reason;

    if (thisRead == 0 && pasynUser->timeout == 0){
      status = asynTimeout;
    }
  }
  *nbytesTransfered = thisRead;
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s thisRead=%lu data=\"%s\"\n",
            adsController_p->portName,
            (unsigned long)thisRead, data);
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
  portName_=portName;
  ipaddr_=ipaddr;
  amsaddr_=amsaddr;
  amsport_=amsport;
  priority_=priority;
  noAutoConnect_=noAutoConnect;
  noProcessEos_=noProcessEos;


  adsController_t *adsController_p;
  asynInterface *pasynInterface;
  asynStatus status;
  size_t nbytes;
  void *allocp;
  asynOctet *pasynOctet;
  printf("%s/%s:%d port=%s ipaddr=%s amsaddr=%s priority=%u noAutoConnect=%d noProcessEos=%d\n",
         __FILE__, __FUNCTION__, __LINE__,
         portName ? portName : "",
         ipaddr,
	 amsaddr,
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
