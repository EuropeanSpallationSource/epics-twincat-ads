
#include "adsCom.h"
#include "AdsLib.h"

#include <iostream>
#include <sys/time.h>
#include <string.h>
#include <inttypes.h>

static long adsPort=0; //handle
static uint16_t defaultAmsPort=0; //Port number for module (851 for PLC 1)
static AmsNetId remoteNetId={0,0,0,0,0,0};
//static AmsAddr amsServer={remoteNetId,0};

static int returnVarName=0;

#define RETURN_VAR_NAME_IF_NEEDED                              \
  do {                                                         \
    if(returnVarName){                                         \
      cmd_buf_printf(asciiBuffer,"%s=",info->variableName);    \
    }                                                          \
  }                                                            \
  while(0)                                                     \

extern "C" {
  const char *AdsErrorToString(long error)
  {
    switch (error) {
    case GLOBALERR_TARGET_PORT:
      return "GLOBALERR_TARGET_PORT";
    case GLOBALERR_MISSING_ROUTE:
      return "GLOBALERR_MISSING_ROUTE";
    case GLOBALERR_NO_MEMORY:
      return "GLOBALERR_NO_MEMORY";
    case GLOBALERR_TCP_SEND:
      return "GLOBALERR_TCP_SEND";
    case ADSERR_DEVICE_ERROR:
      return "ADSERR_DEVICE_ERROR";
    case ADSERR_DEVICE_SRVNOTSUPP:
      return "ADSERR_DEVICE_SRVNOTSUPP";
    case ADSERR_DEVICE_INVALIDGRP:
      return "ADSERR_DEVICE_INVALIDGRP";
    case ADSERR_DEVICE_INVALIDOFFSET:
      return "ADSERR_DEVICE_INVALIDOFFSET";
    case ADSERR_DEVICE_INVALIDACCESS:
      return "ADSERR_DEVICE_INVALIDACCESS";
    case ADSERR_DEVICE_INVALIDSIZE:
      return "ADSERR_DEVICE_INVALIDSIZE";
    case ADSERR_DEVICE_INVALIDDATA:
      return "ADSERR_DEVICE_INVALIDDATA";
    case ADSERR_DEVICE_NOTREADY:
      return "ADSERR_DEVICE_NOTREADY";
    case ADSERR_DEVICE_BUSY:
      return "ADSERR_DEVICE_BUSY";
    case ADSERR_DEVICE_INVALIDCONTEXT:
      return "ADSERR_DEVICE_INVALIDCONTEXT";
    case ADSERR_DEVICE_NOMEMORY:
      return "ADSERR_DEVICE_NOMEMORY";
    case ADSERR_DEVICE_INVALIDPARM:
      return "ADSERR_DEVICE_INVALIDPARM";
    case ADSERR_DEVICE_NOTFOUND:
      return "ADSERR_DEVICE_NOTFOUND";
    case ADSERR_DEVICE_SYNTAX:
      return "ADSERR_DEVICE_SYNTAX";
    case ADSERR_DEVICE_INCOMPATIBLE:
      return "ADSERR_DEVICE_INCOMPATIBLE";
    case ADSERR_DEVICE_EXISTS:
      return "ADSERR_DEVICE_EXISTS";
    case ADSERR_DEVICE_SYMBOLNOTFOUND:
      return "ADSERR_DEVICE_SYMBOLNOTFOUND";
    case ADSERR_DEVICE_SYMBOLVERSIONINVALID:
      return "ADSERR_DEVICE_SYMBOLVERSIONINVALID";
    case ADSERR_DEVICE_INVALIDSTATE:
      return "ADSERR_DEVICE_INVALIDSTATE";
    case ADSERR_DEVICE_TRANSMODENOTSUPP:
      return "ADSERR_DEVICE_TRANSMODENOTSUPP";
    case ADSERR_DEVICE_NOTIFYHNDINVALID:
      return "ADSERR_DEVICE_NOTIFYHNDINVALID";
    case ADSERR_DEVICE_CLIENTUNKNOWN:
      return "ADSERR_DEVICE_CLIENTUNKNOWN";
    case ADSERR_DEVICE_NOMOREHDLS:
      return "ADSERR_DEVICE_NOMOREHDLS";
    case ADSERR_DEVICE_INVALIDWATCHSIZE:
      return "ADSERR_DEVICE_INVALIDWATCHSIZE";
    case ADSERR_DEVICE_NOTINIT:
      return "ADSERR_DEVICE_NOTINIT";
    case ADSERR_DEVICE_TIMEOUT:
      return "ADSERR_DEVICE_TIMEOUT";
    case ADSERR_DEVICE_NOINTERFACE:
      return "ADSERR_DEVICE_NOINTERFACE";
    case ADSERR_DEVICE_INVALIDINTERFACE:
      return "ADSERR_DEVICE_INVALIDINTERFACE";
    case ADSERR_DEVICE_INVALIDCLSID:
      return "ADSERR_DEVICE_INVALIDCLSID";
    case ADSERR_DEVICE_INVALIDOBJID:
      return "ADSERR_DEVICE_INVALIDOBJID";
    case ADSERR_DEVICE_PENDING:
      return "ADSERR_DEVICE_PENDING";
    case ADSERR_DEVICE_ABORTED:
      return "ADSERR_DEVICE_ABORTED";
    case ADSERR_DEVICE_WARNING:
      return "ADSERR_DEVICE_WARNING";
    case ADSERR_DEVICE_INVALIDARRAYIDX:
      return "ADSERR_DEVICE_INVALIDARRAYIDX";
    case ADSERR_DEVICE_SYMBOLNOTACTIVE:
      return "ADSERR_DEVICE_SYMBOLNOTACTIVE";
    case ADSERR_DEVICE_ACCESSDENIED:
      return "ADSERR_DEVICE_ACCESSDENIED";
    case ADSERR_DEVICE_LICENSENOTFOUND:
      return "ADSERR_DEVICE_LICENSENOTFOUND";
    case ADSERR_DEVICE_LICENSEEXPIRED:
      return "ADSERR_DEVICE_LICENSEEXPIRED";
    case ADSERR_DEVICE_LICENSEEXCEEDED:
      return "ADSERR_DEVICE_LICENSEEXCEEDED";
    case ADSERR_DEVICE_LICENSEINVALID:
      return "ADSERR_DEVICE_LICENSEINVALID";
    case ADSERR_DEVICE_LICENSESYSTEMID:
      return "ADSERR_DEVICE_LICENSESYSTEMID";
    case ADSERR_DEVICE_LICENSENOTIMELIMIT:
      return "ADSERR_DEVICE_LICENSENOTIMELIMIT";
    case ADSERR_DEVICE_LICENSEFUTUREISSUE:
      return "ADSERR_DEVICE_LICENSEFUTUREISSUE";
    case ADSERR_DEVICE_LICENSETIMETOLONG:
      return "ADSERR_DEVICE_LICENSETIMETOLONG";
    case ADSERR_DEVICE_EXCEPTION:
      return "ADSERR_DEVICE_EXCEPTION";
    case ADSERR_DEVICE_LICENSEDUPLICATED:
      return "ADSERR_DEVICE_LICENSEDUPLICATED";
    case ADSERR_DEVICE_SIGNATUREINVALID:
      return "ADSERR_DEVICE_SIGNATUREINVALID";
    case ADSERR_DEVICE_CERTIFICATEINVALID:
      return "ADSERR_DEVICE_CERTIFICATEINVALID";
    case ADSERR_CLIENT_ERROR:
      return "ADSERR_CLIENT_ERROR";
    case ADSERR_CLIENT_INVALIDPARM:
      return "ADSERR_CLIENT_INVALIDPARM";
    case ADSERR_CLIENT_LISTEMPTY:
      return "ADSERR_CLIENT_LISTEMPTY";
    case ADSERR_CLIENT_VARUSED:
      return "ADSERR_CLIENT_VARUSED";
    case ADSERR_CLIENT_DUPLINVOKEID:
      return "ADSERR_CLIENT_DUPLINVOKEID";
    case ADSERR_CLIENT_SYNCTIMEOUT:
      return "ADSERR_CLIENT_SYNCTIMEOUT";
    case ADSERR_CLIENT_W32ERROR:
      return "ADSERR_CLIENT_W32ERROR";
    case ADSERR_CLIENT_TIMEOUTINVALID:
      return "ADSERR_CLIENT_TIMEOUTINVALID";
    case ADSERR_CLIENT_PORTNOTOPEN:
      return "ADSERR_CLIENT_PORTNOTOPEN";
    case ADSERR_CLIENT_NOAMSADDR:
      return "ADSERR_CLIENT_NOAMSADDR";
    case ADSERR_CLIENT_SYNCINTERNAL:
      return "ADSERR_CLIENT_SYNCINTERNAL";
    case ADSERR_CLIENT_ADDHASH:
      return "ADSERR_CLIENT_ADDHASH";
    case ADSERR_CLIENT_REMOVEHASH:
      return "ADSERR_CLIENT_REMOVEHASH";
    case ADSERR_CLIENT_NOMORESYM:
      return "ADSERR_CLIENT_NOMORESYM";
    case ADSERR_CLIENT_SYNCRESINVALID:
      return "ADSERR_CLIENT_SYNCRESINVALID";
    case ADSERR_CLIENT_SYNCPORTLOCKED:
      return "ADSERR_CLIENT_SYNCPORTLOCKED";
    default:
      return "ADSERR_ERROR_UNKNOWN";
    }
  }
}


static void NotifyCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser)
{
    const uint8_t* data = reinterpret_cast<const uint8_t*>(pNotification + 1);
    std::cout << "hUser 0x" << std::hex << hUser <<
        " sample time: " << std::dec << pNotification->nTimeStamp <<
        " sample size: " << std::dec << pNotification->cbSampleSize <<
        " value:";
    for (size_t i = 0; i < pNotification->cbSampleSize; ++i) {
        std::cout << " 0x" << std::dec << (int)data[i];
    }
    std::cout << '\n';
}

uint32_t getHandleByNameExample(std::ostream& out, long port, const AmsAddr& server,
                                const std::string handleName)
{
    uint32_t handle = 0;
    const long handleStatus = AdsSyncReadWriteReqEx2(port,
                                                     &server,
                                                     ADSIGRP_SYM_HNDBYNAME,
                                                     0,
                                                     sizeof(handle),
                                                     &handle,
                                                     handleName.size(),
                                                     handleName.c_str(),
                                                     nullptr);
    if (handleStatus) {
        out << "Create handle for '" << handleName << "' failed with: 0x" << std::hex << handleStatus << '\n';
    }
    return handle;
}

void releaseHandleExample(std::ostream& out, long port, const AmsAddr& server, uint32_t handle)
{
    const long releaseHandle = AdsSyncWriteReqEx(port, &server, ADSIGRP_SYM_RELEASEHND, 0, sizeof(handle), &handle);
    if (releaseHandle) {
        out << "Release handle 0x" << std::hex << handle << "' failed with: 0x" << releaseHandle << '\n';
    }
}

void notificationExample(std::ostream& out, long port, const AmsAddr& server)
{
    const AdsNotificationAttrib attrib = {
        1,
        ADSTRANS_SERVERCYCLE,
        0,
        {4000000}
    };
    uint32_t hNotify;
    uint32_t hUser = 0;

    const long addStatus = AdsSyncAddDeviceNotificationReqEx(port,
                                                             &server,
                                                             0x4020,
                                                             4,
                                                             &attrib,
                                                             &NotifyCallback,
                                                             hUser,
                                                             &hNotify);
    if (addStatus) {
        out << "Add device notification failed with: " << std::dec << addStatus << '\n';
        return;
    }

    std::cout << "Hit ENTER to stop notifications\n";
    std::cin.ignore();

    const long delStatus = AdsSyncDelDeviceNotificationReqEx(port, &server, hNotify);
    if (delStatus) {
        out << "Delete device notification failed with: " << std::dec << delStatus;
        return;
    }
}

void notificationByNameExample(std::ostream& out, long port, const AmsAddr& server)
{
    const AdsNotificationAttrib attrib = {
        1,
        ADSTRANS_SERVERCYCLE,
        0,
        {4000000}
    };
    uint32_t hNotify;
    uint32_t hUser = 0;

    uint32_t handle;

    out << __FUNCTION__ << "():\n";
    handle = getHandleByNameExample(out, port, server, "MAIN.byByte");

    const long addStatus = AdsSyncAddDeviceNotificationReqEx(port,
                                                             &server,
                                                             ADSIGRP_SYM_VALBYHND,
                                                             handle,
                                                             &attrib,
                                                             &NotifyCallback,
                                                             hUser,
                                                             &hNotify);
    if (addStatus) {
        out << "Add device notification failed with: " << std::dec << addStatus << '\n';
        return;
    }

    std::cout << "Hit ENTER to stop by name notifications\n";
    std::cin.ignore();

    const long delStatus = AdsSyncDelDeviceNotificationReqEx(port, &server, hNotify);
    if (delStatus) {
        out << "Delete device notification failed with: " << std::dec << delStatus;
        return;
    }
    releaseHandleExample(out, port, server, handle);
}

void readExample(std::ostream& out, long port, const AmsAddr& server)
{
    uint32_t bytesRead;
    uint32_t buffer;

    out << __FUNCTION__ << "():\n";
    for (size_t i = 0; i < 8; ++i) {
        const long status = AdsSyncReadReqEx2(port, &server, 0x4020, 0, sizeof(buffer), &buffer, &bytesRead);
        if (status) {
            out << "ADS read failed with: " << std::dec << status << '\n';
            return;
        }
        out << "ADS read " << std::dec << bytesRead << " bytes, value: 0x" << std::hex << buffer << '\n';
    }
}

void readByNameExample(std::ostream& out, long port, const AmsAddr& server)
{
    uint32_t bytesRead;
    uint32_t buffer;
    uint32_t handle;

    out << __FUNCTION__ << "():\n";
    handle = getHandleByNameExample(out, port, server, "MAIN.byByte");

    for (size_t i = 0; i < 8; ++i) {
        const long status = AdsSyncReadReqEx2(port,
                                              &server,
                                              ADSIGRP_SYM_VALBYHND,
                                              handle,
                                              sizeof(buffer),
                                              &buffer,
                                              &bytesRead);
        if (status) {
            out << "ADS read failed with: " << std::dec << status << '\n';
            return;
        }
        out << "ADS read " << std::dec << bytesRead << " bytes, value 0x: " << std::hex << buffer << '\n';
    }
    releaseHandleExample(out, port, server, handle);
}

void readStateExample(std::ostream& out, long port, const AmsAddr& server)
{
    uint16_t adsState;
    uint16_t devState;

    const long status = AdsSyncReadStateReqEx(port, &server, &adsState, &devState);
    if (status) {
        out << "ADS read failed with: " << std::dec << status << '\n';
        return;
    }
    out << "ADS state: " << std::dec << adsState << " devState: " << std::dec << devState << '\n';
}

void runExample()
{
    static const AmsNetId remoteNetId { 192, 168, 88, 44, 1, 1 };
    static const char remoteIpV4[] = "192.168.88.44";

    // add local route to your EtherCAT Master
    if (AdsAddRoute(remoteNetId, remoteIpV4)) {
	std::cout<< "Adding ADS route failed, did you specified valid addresses?\n";
        return;
    }

    // open a new ADS port
    const long port = AdsPortOpenEx();
    if (!port) {
	std::cout << "Open ADS port failed\n";
        return;
    }

    const AmsAddr remote { remoteNetId, AMSPORT_R0_PLC_TC3 };
    notificationExample(std::cout, port, remote);
    notificationByNameExample(std::cout, port, remote);
    readExample(std::cout, port, remote);
    readByNameExample(std::cout, port, remote);
    readStateExample(std::cout, port, remote);

    const long closeStatus = AdsPortCloseEx(port);
    if (closeStatus) {
	std::cout << "Close ADS port failed with: " << std::dec << closeStatus << '\n';
    }

#ifdef _WIN32
    // WORKAROUND: On Win7 std::thread::join() called in destructors
    //             of static objects might wait forever...
    AdsDelRoute(remoteNetId);
#endif
}

void reset()
{
  adsPort=0;
  defaultAmsPort=0;
  remoteNetId={0,0,0,0,0,0};
}

int adsConnect(const char *ipaddr,const char *amsaddr, int amsport)
{
  defaultAmsPort=amsport;

  if(defaultAmsPort<=0){
    std::cout<< "Invalid AMS port:" << defaultAmsPort << " .Did you specify a valid ams port number?\n";
    return ADS_COM_ERROR_INVALID_AMS_PORT;
  }
  int nvals = sscanf(amsaddr, "%hhu.%hhu.%hhu.%hhu.%hhu.%hhu",
		     &remoteNetId.b[0],
		     &remoteNetId.b[1],
		     &remoteNetId.b[2],
		     &remoteNetId.b[3],
		     &remoteNetId.b[4],
		     &remoteNetId.b[5]);
  if (nvals != 6) {
    LOGERR("Invalid AMS address: %s. Did you specify a valid address? Error number: %d\n", amsaddr,ADS_COM_ERROR_INVALID_AMS_ADDRESS);
    reset();
    return ADS_COM_ERROR_INVALID_AMS_ADDRESS;
  }

  // add local route to your EtherCAT Master
  int error=AdsAddRoute(remoteNetId, ipaddr);
  if (error) {
    LOGERR("Adding ADS route failed, did you specified valid addresses? Error number: %d\n",error);
    reset();
    return error;
  }

  // open a new ADS port
  adsPort = AdsPortOpenEx();
  if (!adsPort) {
    LOGERR("Open ADS port failed. Error number: %d\n",ADS_COM_ERROR_OPEN_ADS_PORT_FAIL);
    reset();
    return ADS_COM_ERROR_OPEN_ADS_PORT_FAIL;
  }

  return 0;
}

int adsDisconnect()
{
  const long closeStatus = AdsPortCloseEx(adsPort);
   if (closeStatus) {
     LOGERR("Close ADS port failed with error code: %ld\n",closeStatus);
   }

#ifdef _WIN32
   // WORKAROUND: On Win7 std::thread::join() called in destructors
   //             of static objects might wait forever...
   AdsDelRoute(remoteNetId);
#endif
  reset();
  return 0;

}

long getSymInfoByName(uint16_t amsPort,const char* variableAddr,SYMINFOSTRUCT *info)
{
  //TODO: Some information is corrupt in the return of this function (all stings). However the needed data is OK...
  AmsAddr amsServer;
  uint32_t bytesRead=0;
  if(amsPort<=0){
    amsServer={remoteNetId,defaultAmsPort};
  }
  else{
    amsServer={remoteNetId,amsPort};
  }

  const long status = AdsSyncReadWriteReqEx2(adsPort,
                                             &amsServer,
					     ADSIGRP_SYM_INFOBYNAMEEX,
                                             0,
                                             sizeof(SYMINFOSTRUCT),
                                             info,
					     strlen(variableAddr),
					     variableAddr,
					     &bytesRead);

  LOGINFO4("Total bytes read: %d\n",bytesRead);

  info->variableName = info->buffer;
  unsigned int offset=strlen(info->variableName)+1;
  if(offset>=sizeof(info->buffer)){
    return ADS_COM_ERROR_READ_SYMBOLIC_INFO;
  }
  info->symDataType = info->buffer+offset;
  unsigned int offset2=strlen(info->symDataType)+1;
  if(offset+offset2>=sizeof(info->buffer)){
    return ADS_COM_ERROR_READ_SYMBOLIC_INFO;
  }
  info->symComment= info->symDataType+offset2;

  if (status) {
    LOGERR("Read of symbol info for %s failed with: %s (0x%lx)\n",variableAddr,
           AdsErrorToString(status), status);
    return status;
  }


  LOGINFO4("Buffer Dump raw: \n");
  if(debug_print_flags & (1<<4)){
    int n=sizeof(SYMINFOSTRUCT), i =0;
    unsigned char* byte_array = (unsigned char*)info;

    while (i < n){
	LOGINFO4("%02X",(unsigned)byte_array[i]);
      i++;
    }

    LOGINFO4("\nBuffer Dump ascii: \n");
    i =0;
    while (i < n)
    {
	LOGINFO4("%c",(unsigned)byte_array[i]);
      i++;
    }
    LOGINFO4("\n");
  }

  LOGINFO4("Symbolic information\n");
  LOGINFO4("SymEntrylength: %d\n",info->symEntryLen);
  LOGINFO4("idxGroup: 0x%x\n",info->idxGroup);
  LOGINFO4("idxOffset: 0x%x\n",info->idxOffset);
  LOGINFO4("adsDataType: %d\n",info->adsDataType);
  LOGINFO4("ByteSize: %d\n",info->byteSize);
  LOGINFO4("Dummy1: %d\n",info->dummy1);
  LOGINFO4("Dummy2: %d\n",info->dummy2);
  LOGINFO4("Dummy3: %d\n",info->dummy3);
  LOGINFO4("Variable name: %s\n",info->variableName);
  LOGINFO4("Data type: %s\n",info->symDataType);
  LOGINFO4("Comment: %s\n",info->symComment);

  return 0;
}

int binary2ascii(void *binaryBuffer, uint32_t binaryBufferSize, SYMINFOSTRUCT *info,
		 ecmcOutputBufferType *asciiBuffer)
{
  uint32_t bytesProcessed=0;
  int cycles=0;
  int error=0;
  int bytesPerDataPoint=0;
  while(bytesProcessed<info->byteSize && !error){
    //write comma for arrays
    if(bytesProcessed!=0){
      cmd_buf_printf(asciiBuffer,",");
    }
    switch(info->adsDataType){
      case ADST_INT8:
	RETURN_VAR_NAME_IF_NEEDED;
        int8_t *ADST_INT8Var;
        ADST_INT8Var=((int8_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%hhd",*ADST_INT8Var);
        LOGINFO4("Binary 2 ASCII ADST_INT8, value: %d\n", *ADST_INT8Var);
        bytesPerDataPoint=1;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_INT16:
	RETURN_VAR_NAME_IF_NEEDED;
        int16_t *ADST_INT16Var;
        ADST_INT16Var=((int16_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%d",*ADST_INT16Var);
        LOGINFO4("Binary 2 ASCII ADST_INT16, value: %d\n", *ADST_INT16Var);
        bytesPerDataPoint=2;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_INT32:
	RETURN_VAR_NAME_IF_NEEDED;
        int32_t *ADST_INT32Var;
        ADST_INT32Var=((int32_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%d",*ADST_INT32Var);
        LOGINFO4("Binary 2 ASCII ADST_INT32, value: %d\n", *ADST_INT32Var);
        bytesPerDataPoint=4;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_INT64:
	RETURN_VAR_NAME_IF_NEEDED;
        int64_t *ADST_INT64Var;
        ADST_INT64Var=((int64_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"% PRId64",*ADST_INT64Var);
        LOGINFO4("Binary 2 ASCII ADST_INT64, value: %" PRId64 "\n", *ADST_INT64Var);
        bytesPerDataPoint=8;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT8:
	RETURN_VAR_NAME_IF_NEEDED;
        uint8_t *ADST_UINT8Var;
        ADST_UINT8Var=((uint8_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%hhu",*ADST_UINT8Var);
        LOGINFO4("Binary 2 ASCII ADST_UINT8, value: %d\n", *ADST_UINT8Var);
        bytesPerDataPoint=1;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT16:
	RETURN_VAR_NAME_IF_NEEDED;
        uint16_t *ADST_UINT16Var;
        ADST_UINT16Var=((uint16_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%d",*ADST_UINT16Var);
        LOGINFO4("Binary 2 ASCII ADST_UINT16, value: %d\n", *ADST_UINT16Var);
        bytesPerDataPoint=2;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT32:
	RETURN_VAR_NAME_IF_NEEDED;
        uint32_t *ADST_UINT32Var;
        ADST_UINT32Var=((uint32_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%d",*ADST_UINT32Var);
        LOGINFO4("Binary 2 ASCII ADST_UINT32, value: %d\n", *ADST_UINT32Var);
        bytesPerDataPoint=4;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT64:
	RETURN_VAR_NAME_IF_NEEDED;
        uint64_t *ADST_UINT64Var;
        ADST_UINT64Var=((uint64_t*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"% PRIu64",*ADST_UINT64Var);
        LOGINFO4("Binary 2 ASCII ADST_UINT64, value: %" PRIu64 "\n", *ADST_UINT64Var);
        bytesPerDataPoint=8;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_REAL32:
	RETURN_VAR_NAME_IF_NEEDED;
        float *ADST_REAL32Var;
        ADST_REAL32Var=((float*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%f",*ADST_REAL32Var);
        LOGINFO4("Binary 2 ASCII ADST_REAL32, value: %lf\n", *ADST_REAL32Var);
        bytesPerDataPoint=4;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_REAL64:
	RETURN_VAR_NAME_IF_NEEDED;
        double *ADST_REAL64Var;
        ADST_REAL64Var=((double*)binaryBuffer)+cycles;
        cmd_buf_printf(asciiBuffer,"%lf",*ADST_REAL64Var);
        LOGINFO4("Binary 2 ASCII ADST_REAL64, value: %lf\n", *ADST_REAL64Var);
        bytesPerDataPoint=8;
        bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_BIT:
	RETURN_VAR_NAME_IF_NEEDED;
        char *charVar;
        charVar=((char*)binaryBuffer)+cycles;
        if(*charVar==1){
          cmd_buf_printf(asciiBuffer,"1");
        }
        else{
          cmd_buf_printf(asciiBuffer,"0");
        }
        bytesPerDataPoint=1;//TODO: Check if each bit takes one byte or actually only one bit?!
        bytesProcessed+=bytesPerDataPoint;
        LOGINFO4("Binary 2 ASCII ADST_BIT, value: %c\n", *charVar);
        break;
      case ADST_STRING:
	RETURN_VAR_NAME_IF_NEEDED;
  	char *ADST_STRINGVar;
	ADST_STRINGVar = (char*)binaryBuffer;
        cmd_buf_printf(asciiBuffer,"%s",ADST_STRINGVar);
        LOGINFO4("Binary 2 ASCII ADST_STRING, value: %s\n", ADST_STRINGVar);
        bytesProcessed=info->byteSize;
	break;
      case ADST_BIGTYPE:
        if(strcmp(info->symDataType,DUT_AXIS_STATUS)==0){
          //RETURN_VAR_NAME_IF_NEEDED;
          cmd_buf_printf(asciiBuffer,"%s=",info->variableName); //Always output variable name for stAxisStatus
          STAXISSTATUSSTRUCT * stAxisData;
          stAxisData=(STAXISSTATUSSTRUCT*)binaryBuffer;

          if(stAxisData->bEnable){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bReset){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bExecute){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          cmd_buf_printf(asciiBuffer,"%d,", stAxisData->nCommand);
          cmd_buf_printf(asciiBuffer,"%d,", stAxisData->nCmdData);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fVelocity);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fPosition);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fAcceleration);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fDeceleration);
          if(stAxisData->bJogFwd){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bJogBwd){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bLimitFwd){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bLimitBwd){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fOverride);
          if(stAxisData->bHomeSensor){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bEnabled){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bError){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          cmd_buf_printf(asciiBuffer,"%d,", stAxisData->nErrorId);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fActVelocity);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fActPosition);
          cmd_buf_printf(asciiBuffer,"%lf,", stAxisData->fActDiff);
          if(stAxisData->bHomed){
            cmd_buf_printf(asciiBuffer,"1,");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0,");
          }
          if(stAxisData->bBusy){
            cmd_buf_printf(asciiBuffer,"1;");
          }
          else{
            cmd_buf_printf(asciiBuffer,"0;");
          }
          LOGINFO4("Binary 2 ASCII ADST_BIGTYPE, type: %s\n", info->symDataType);
          bytesProcessed=info->byteSize;
          break; //end DUT_AXIS_STATUS
        }
        break;
      default:
        error=ADS_COM_ERROR_INVALID_DATA_TYPE;
        LOGERR("Data type %s (%d) not implemented. Error: %d\n", info->symDataType, info->adsDataType,error);
        bytesPerDataPoint=0;
        break;
    }
    cycles++;

    if(binaryBufferSize<bytesProcessed+bytesPerDataPoint){
      error=ADS_COM_ERROR_ADS_READ_BUFFER_INDEX_EXCEEDED_SIZE;
      LOGERR("Buffer size exceeded. Error: %d\n",error);
    }
    if((asciiBuffer->bufferSize-asciiBuffer->bytesUsed)<20){
      error=ADS_COM_ERROR_BUFFER_TO_EPICS_FULL;
      LOGERR("Buffer size exceeded. Error: %d\n",error);
    }
  }
  return error;
}

int ascii2binary(const char *asciiBuffer,uint16_t dataType,void *binaryBuffer, uint32_t binaryBufferSize, uint32_t *bytesProcessed)
{
  int cycles=0;
  int error=0;
  int bytesPerDataPoint=0;
  int converted = 0 ;

  LOGINFO4("ascii2binary buffer: %s, datatype: %d\n", asciiBuffer ,dataType);
  do
  {
    switch(dataType){
      case ADST_INT8:
        int8_t *ADST_INT8Var;
        ADST_INT8Var=((int8_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNd8, (ADST_INT8Var));
        bytesPerDataPoint=1;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_INT16:
        int16_t *ADST_INT16Var;
        ADST_INT16Var=((int16_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNd16, ADST_INT16Var);
        bytesPerDataPoint=2;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_INT32:
        int32_t *ADST_INT32Var;
        ADST_INT32Var=((int32_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNd32, (ADST_INT32Var));
        bytesPerDataPoint=4;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_INT64:
        int64_t *ADST_INT64Var;
        ADST_INT64Var=((int64_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNd64, (ADST_INT64Var));
        bytesPerDataPoint=8;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT8:
        uint8_t *ADST_UINT8Var;
        ADST_UINT8Var=((uint8_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNu8, (ADST_UINT8Var));
        bytesPerDataPoint=1;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT16:
        uint16_t *ADST_UINT16Var;
        ADST_UINT16Var=((uint16_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNu16, (ADST_UINT16Var));
        bytesPerDataPoint=2;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT32:
        uint32_t *ADST_UINT32Var;
        ADST_UINT32Var=((uint32_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNu32, (ADST_UINT32Var));
        bytesPerDataPoint=4;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_UINT64:
        uint64_t *ADST_UINT64Var;
        ADST_UINT64Var=((uint64_t*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%" SCNu64, (ADST_UINT64Var));
        bytesPerDataPoint=8;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_REAL32:
        float *ADST_REAL32Var;
        ADST_REAL32Var=((float*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%f", (ADST_REAL32Var));
        bytesPerDataPoint=4;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_REAL64:
        double *ADST_REAL64Var;
        ADST_REAL64Var=((double*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%lf", (ADST_REAL64Var));
        bytesPerDataPoint=8;
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_BIT:
        char *charVar;
        charVar=((char*)binaryBuffer)+cycles;
        converted = sscanf( asciiBuffer, "%hhu", (charVar));
        bytesPerDataPoint=1; //TODO: Check if each bit takes one byte or actually only one bit?!
        *bytesProcessed+=bytesPerDataPoint;
        break;
      case ADST_STRING:
	char *stringVar;
	stringVar=((char*)binaryBuffer);
        converted = sscanf( asciiBuffer, "%s", stringVar);
        bytesPerDataPoint=1; //TODO: Check if each bit takes one byte or actually only one bit?!
        *bytesProcessed=binaryBufferSize;
	break;
      default:
        std::cout  << "Data type: " << std::dec << dataType << " not implemented" << '\n';
        error=ADS_COM_ERROR_INVALID_DATA_TYPE;
        bytesPerDataPoint=0;
        break;
    }
    if(binaryBufferSize<*bytesProcessed+bytesPerDataPoint){
      error=ADS_COM_ERROR_ADS_READ_BUFFER_INDEX_EXCEEDED_SIZE;
      LOGERR("ascii2binary buffer size exceeded. Error: %d\n", error);
    }

    cycles++;
    asciiBuffer= strchr( asciiBuffer, ',' ) ;
    if(asciiBuffer){
	asciiBuffer++;
    }

  } while(asciiBuffer !=NULL  && converted != 0 && !error);
  return error;
}

long adsReadByName(uint16_t amsPort,const char *variableAddr,ecmcOutputBufferType *outBuffer)
{
  SYMINFOSTRUCT info;
  memset(&info,0,sizeof(info));
  AmsAddr amsServer;
  if(amsPort<=0){
    amsServer={remoteNetId,defaultAmsPort};
  }
  else{
    amsServer={remoteNetId,amsPort};
  }

  LOGINFO4("%s(): Variable Name:%s\n", __FUNCTION__,variableAddr);

  struct timeval start, end;
  long secs_used,micros_used;
  gettimeofday(&start, NULL);

  long errorCode=getSymInfoByName(amsPort,variableAddr,&info);
  switch (errorCode) {
    case ADSERR_DEVICE_SYMBOLNOTFOUND:
      cmd_buf_printf(outBuffer, "%s (0x%lx)",
                     AdsErrorToString(errorCode), errorCode);
      return 0;
  case ADSERR_CLIENT_SYNCTIMEOUT:
    /* retry once */
    errorCode = getSymInfoByName(amsPort,variableAddr,&info);
    if (errorCode) {
      return errorCode;
    }
    break;
  case ADSERR_CLIENT_PORTNOTOPEN:
    cmd_buf_printf(outBuffer, "%s (0x%lx)",
                   AdsErrorToString(errorCode), errorCode);
    return errorCode;
  case 0:
      break;
  default:
    cmd_buf_printf(outBuffer, "%s (0x%lx)",
                   AdsErrorToString(errorCode), errorCode);
    return 0;
  }
  int status=adsReadByGroupOffset(amsPort,&info,outBuffer);
  gettimeofday(&end, NULL);
  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
  LOGINFO4("Name: micros_used: %ld\n",micros_used);

  return status;
}

int adsReadByGroupOffset(uint16_t amsPort,SYMINFOSTRUCT *info, ecmcOutputBufferType *outBuffer)
{
  uint32_t bytesRead;
  uint8_t adsReadBuffer[BUFFER_SIZE];

  AmsAddr amsServer;
  if(amsPort<=0){
    amsServer={remoteNetId,defaultAmsPort};
  }
  else{
    amsServer={remoteNetId,amsPort};
  }

  int dataSize=info->byteSize;
  if(info->byteSize>BUFFER_SIZE){
    dataSize=BUFFER_SIZE;
  }

  LOGINFO4("%s():\n",__FUNCTION__);

  struct timeval start, end;
  long secs_used,micros_used;
  gettimeofday(&start, NULL);

  int error = AdsSyncReadReqEx2(adsPort, &amsServer, info->idxGroup,info->idxOffset,dataSize, &adsReadBuffer, &bytesRead);
  if (error) {
    LOGERR("%s(): ADS read failed with: %d\n",__FUNCTION__,error);
    return error;
  }

  gettimeofday(&end, NULL);
  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
  LOGINFO4("Grp,Offset: micros_used: %ld\n",micros_used);

  error=binary2ascii(&adsReadBuffer,BUFFER_SIZE,info,outBuffer);
  if (error) {
    LOGERR("%s(): ADS read failed with: %d\n",__FUNCTION__,error);
    return error;
  }

  return 0;
}

int adsWriteByName(uint16_t amsPort,const char *variableAddr,char *asciiValueToWrite,ecmcOutputBufferType *outBuffer)
{
  SYMINFOSTRUCT info;

  AmsAddr amsServer;
  if(amsPort<=0){
    amsServer={remoteNetId,defaultAmsPort};
  }
  else{
    amsServer={remoteNetId,amsPort};
  }

  LOGINFO4("%s(): variable: %s value to write:%s\n",__FUNCTION__,variableAddr,asciiValueToWrite);

  struct timeval start, end;
  long secs_used,micros_used;
  gettimeofday(&start, NULL);

  int errorCode=getSymInfoByName(amsPort,variableAddr,&info);
  if(errorCode){
    return errorCode;
  }

  int status=adsWriteByGroupOffset(amsPort,info.idxGroup,info.idxOffset,info.adsDataType,info.byteSize,asciiValueToWrite,outBuffer);

  gettimeofday(&end, NULL);
  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
  LOGINFO4("Name: micros_used: %ld\n",micros_used);

  if (status) {
    return status;
  }

  return 0;
}

int adsWriteByGroupOffset(uint16_t amsPort,uint32_t group, uint32_t offset,uint16_t dataType,uint32_t dataSize, const char *asciiValueToWrite,ecmcOutputBufferType *asciiResponseBuffer)
{
  uint32_t bytesToWrite=0;
  uint8_t binaryBuffer[BUFFER_SIZE];
  AmsAddr amsServer;

  LOGINFO4("%s(): group: %x offset: %x datatype: %d dataSize: %d  value: %s\n",__FUNCTION__, group, offset,dataType,dataSize, asciiValueToWrite );

  if(amsPort<=0){
    amsServer={remoteNetId,defaultAmsPort};
  }
  else{
    amsServer={remoteNetId,amsPort};
  }
  int error=ascii2binary(asciiValueToWrite,dataType,&binaryBuffer,BUFFER_SIZE,&bytesToWrite);
  if(error){
    cmd_buf_printf(asciiResponseBuffer,"Error: %x", error);
    LOGERR("%s(): ASCII to binary conversion failed: %d\n",__FUNCTION__,error);
    return error;
  }
  if(bytesToWrite>dataSize){
    bytesToWrite=dataSize;
  }

  struct timeval start, end;
  long secs_used,micros_used;
  gettimeofday(&start, NULL);

  error = AdsSyncWriteReqEx(adsPort, &amsServer, group, offset, bytesToWrite, &binaryBuffer);

  if (error) {
    LOGERR("%s(): ADS write failed with: %d\n",__FUNCTION__,error);
    return error;
  }

  gettimeofday(&end, NULL);
  secs_used=(end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
  micros_used= ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
  LOGINFO4("adsWriteByGroupOffset: micros_used: %ld\n",micros_used);

  return 0;
}

