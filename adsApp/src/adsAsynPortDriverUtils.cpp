/*
 */

#include "adsAsynPortDriverUtils.h"
#include <string.h>
#include <initHooks.h>


const char *adsErrorToString(long error)
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

const char *adsTypeToString(long type)
{
  switch (type) {
    case ADST_VOID:
      return "ADST_VOID";
    case ADST_INT16:
      return "ADST_INT16";
    case ADST_INT32:
      return "ADST_INT32";
    case ADST_REAL32:
      return "ADST_REAL32";
    case ADST_REAL64:
      return "ADST_REAL64";
    case ADST_INT8:
      return "ADST_INT8";
    case ADST_UINT8:
      return "ADST_UINT8";
    case ADST_UINT16:
      return "ADST_UINT16";
    case ADST_UINT32:
      return "ADST_UINT32";
    case ADST_INT64:
      return "ADST_INT64";
    case ADST_UINT64:
      return "ADST_UINT64";
    case ADST_STRING:
      return "ADST_STRING";
    case ADST_WSTRING:
      return "ADST_WSTRING";
    case ADST_REAL80:
      return "ADST_REAL80";
    case ADST_BIT:
      return "ADST_BIT";
    case ADST_BIGTYPE:
      return "ADST_BIGTYPE";
    case ADST_MAXTYPES:
      return "ADST_MAXTYPES";
    default:
      return "ADS_UNKNOWN_DATATYPE";
  }
}

const char *asynTypeToString(long type)
{
  switch (type) {
    case asynParamInt32:
      return "asynParamInt32";
    case asynParamFloat64:
      return "asynParamFloat64";
    case asynParamUInt32Digital:
      return "asynParamUInt32Digital";
    case asynParamOctet:
      return "asynParamOctet";
    case asynParamInt8Array:
      return "asynParamInt8Array";
    case asynParamInt16Array:
      return "asynParamInt16Array";
    case asynParamInt32Array:
      return "asynParamInt32Array";
    case asynParamFloat32Array:
      return "asynParamFloat32Array";
    case asynParamFloat64Array:
      return "asynParamFloat64Array";
    case asynParamGenericPointer:
      return "asynParamGenericPointer";
    default:
      return "asynUnknownType";
  }
}
const char *asynStateToString(long state)
{
  switch (state) {
     case ADSSTATE_INVALID:
       return "ADSSTATE_INVALID";
       break;
     case ADSSTATE_IDLE:
       return "ADSSTATE_IDLE";
       break;
     case ADSSTATE_RESET:
       return "ADSSTATE_RESET";
       break;
     case ADSSTATE_INIT:
       return "ADSSTATE_INIT";
       break;
     case ADSSTATE_START:
       return "ADSSTATE_START";
       break;
     case ADSSTATE_RUN:
       return "ADSSTATE_RUN";
       break;
     case ADSSTATE_STOP:
       return "ADSSTATE_STOP";
       break;
     case ADSSTATE_SAVECFG:
       return "ADSSTATE_SAVECFG";
       break;
     case ADSSTATE_LOADCFG:
       return "ADSSTATE_LOADCFG";
       break;
     case ADSSTATE_POWERFAILURE:
       return "ADSSTATE_POWERFAILURE";
       break;
     case ADSSTATE_POWERGOOD:
       return "ADSSTATE_POWERGOOD";
       break;
     case ADSSTATE_ERROR:
       return "ADSSTATE_ERROR";
       break;
     case ADSSTATE_SHUTDOWN:
       return "ADSSTATE_SHUTDOWN";
       break;
     case ADSSTATE_SUSPEND:
       return "ADSSTATE_SUSPEND";
       break;
     case ADSSTATE_RESUME:
       return "ADSSTATE_RESUME";
       break;
     case ADSSTATE_CONFIG:
       return "ADSSTATE_CONFIG";
       break;
     case ADSSTATE_RECONFIG:
       return "ADSSTATE_RECONFIG";
       break;
     case ADSSTATE_STOPPING:
       return "ADSSTATE_STOPPING";
       break;
     case ADSSTATE_INCOMPATIBLE:
       return "ADSSTATE_INCOMPATIBLE";
       break;
     case ADSSTATE_EXCEPTION:
       return "ADSSTATE_EXCEPTION";
       break;
     case ADSSTATE_MAXSTATES:
       return "ADSSTATE_MAXSTATES";
       break;

  }
  return "UNKOWN ADSSTATE";
}

size_t adsTypeSize(long type)
{
  switch (type) {
    case ADST_VOID:
      return 0;
    case ADST_INT16:
      return 2;
    case ADST_INT32:
      return 4;
    case ADST_REAL32:
      return 4;
    case ADST_REAL64:
      return 8;
    case ADST_INT8:
      return 1;
    case ADST_UINT8:
      return 1;
    case ADST_UINT16:
      return 2;
    case ADST_UINT32:
      return 4;
    case ADST_INT64:
      return 8;
    case ADST_UINT64:
      return 8;
    case ADST_STRING:
      return 1;  //Array of char
    case ADST_WSTRING:
      return -1;
    case ADST_REAL80:
      return 10;
    case ADST_BIT:
      return 1;
    case ADST_BIGTYPE:
      return -1;
    case ADST_MAXTYPES:
      return -1;
    default:
      return -1;
  }
}

asynParamType dtypStringToAsynType(char *dtype)
{
  if(strcmp("asynFloat64",dtype)==0){
    return asynParamFloat64;
  }
  if(strcmp("asynInt32",dtype)==0){
    return asynParamInt32;
  }
  if(strcmp("asynInt8ArrayIn",dtype)==0 || strcmp("asynInt8ArrayOut",dtype)==0){
    return asynParamInt8Array;
  }
  if(strcmp("asynInt16ArrayIn",dtype)==0 || strcmp("asynInt16ArrayOut",dtype)==0){
    return asynParamInt16Array;
  }
  if(strcmp("asynInt32ArrayIn",dtype)==0 || strcmp("asynInt32ArrayOut",dtype)==0){
    return asynParamInt32Array;
  }
  if(strcmp("asynFloat32ArrayIn",dtype)==0 || strcmp("asynFloat32ArrayOut",dtype)==0){
    return asynParamFloat32Array;
  }
  if(strcmp("asynFloat64ArrayIn",dtype)==0 || strcmp("asynFloat64ArrayOut",dtype)==0){
    return asynParamFloat64Array;
  }
  //  asynParamUInt32Digital,
  //  asynParamOctet,
  //  asynParamGenericPointer

  return asynParamNotDefined;
}

const char* epicsStateToString(int state)
{
  switch(state) {
    case initHookAtIocBuild:
      return "initHookAtIocBuild";
      break;
    case initHookAtBeginning:
      return "initHookAtBeginning";
      break;
    case initHookAfterCallbackInit:
      return "initHookAfterCallbackInit";
      break;
    case initHookAfterCaLinkInit:
      return "initHookAfterCaLinkInit";
      break;
    case initHookAfterInitDrvSup:
      return "initHookAfterInitDrvSup";
      break;
    case initHookAfterInitRecSup:
      return "initHookAfterInitRecSup";
      break;
    case initHookAfterInitDevSup:
      return "initHookAfterInitDevSup";
      break;
    case initHookAfterInitDatabase:
      return "initHookAfterInitDatabase";
      break;
    case initHookAfterFinishDevSup:
      return "initHookAfterFinishDevSup";
      break;
    case initHookAfterScanInit:
      return "initHookAfterScanInit";
      break;
    case initHookAfterInitialProcess:
      return "initHookAfterInitialProcess";
      break;
    case initHookAfterIocBuilt:
      return "initHookAfterIocBuilt";
      break;
    case initHookAtIocRun:
      return "initHookAtIocRun";
      break;
    case initHookAfterDatabaseRunning:
      return "initHookAfterDatabaseRunning";
      break;
    case initHookAfterCaServerRunning:
      return "initHookAfterCaServerRunning";
      break;
    case initHookAfterIocRunning:
      return "initHookAfterIocRunning";
      break;
    case initHookAtIocPause:
      return "initHookAtIocPause";
      break;
    case initHookAfterCaServerPaused:
      return "initHookAfterCaServerPaused";
      break;
    case initHookAfterDatabasePaused:
      return "initHookAfterDatabasePaused";
      break;
    case initHookAfterIocPaused:
      return "initHookAfterIocPaused";
      break;
     case initHookAfterInterruptAccept:
      return "initHookAfterInterruptAccept";
      break;
    default:
      return "Unknown state";
      break;
  }
  return "Unknown state";
}
