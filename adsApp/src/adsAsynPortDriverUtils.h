
#ifndef ADSASYNPORTDRIVERUTILS_H_
#define ADSASYNPORTDRIVERUTILS_H_

#include "asynPortDriver.h"  //data types
#include "AdsLib.h"          //error codes

#define ADS_MAX_FIELD_CHAR_LENGTH 128
#define ADS_ADR_COMMAND_PREFIX ".ADR."
#define ADS_OPTION_T_MAX_DLY_MS "T_DLY_MS"
#define ADS_OPTION_T_SAMPLE_RATE_MS "TS_MS"
#define ADS_OPTION_TIMEBASE "TIMEBASE"  //PLC or EPICS
#define ADS_OPTION_TIMEBASE_EPICS "EPICS"
#define ADS_OPTION_TIMEBASE_PLC "PLC"
#define ADS_OPTION_ADSPORT "ADSPORT"

//#define ADS_COM_ERROR_INVALID_AMS_PORT 1000
//#define ADS_COM_ERROR_INVALID_AMS_ADDRESS 1001
//#define ADS_COM_ERROR_OPEN_ADS_PORT_FAIL 1002
//#define ADS_COM_ERROR_ADD_ADS_ROUTE_FAIL 1003
//#define ADS_COM_ERROR_INVALID_DATA_TYPE 1004
//#define ADS_COM_ERROR_ADS_READ_BUFFER_INDEX_EXCEEDED_SIZE 1005
//#define ADS_COM_ERROR_BUFFER_TO_EPICS_FULL 1006
#define ADS_COM_ERROR_READ_SYMBOLIC_INFO 1007

#ifndef ASYN_TRACE_INFO
  #define ASYN_TRACE_INFO      0x0040
#endif

typedef enum{
  ADS_TIME_BASE_EPICS=0,
  ADS_TIME_BASE_PLC=1
} ADSTIMEBASE;

typedef struct adsParamInfo{
  char          *recordName;
  char          *recordType;
  char          *scan;
  char          *dtyp;
  char          *inp;
  char          *out;
  char          *drvInfo;
  asynParamType asynType;
  int           asynAddr;
  bool          isIOIntr;
  double        sampleTimeMS;  //milli seconds
  double        maxDelayTimeMS;  //milli seconds
  uint16_t      amsPort;
  int           paramIndex;  //also used as hUser for ads callback
  bool          plcAbsAdrValid;  //Symbolic address converted to abs address or .ADR. command parsed
  bool          isAdrCommand;
  char          *plcAdrStr;
  uint32_t      plcAbsAdrGroup;
  uint32_t      plcAbsAdrOffset;
  uint32_t      plcSize;
  uint32_t      plcDataType;
  bool          plcDataTypeWarn;
  bool          plcDataIsArray;
  uint32_t      hCallbackNotify;
  bool          bCallbackNotifyValid;
  uint32_t      hSymbolicHandle;
  bool          bSymbolicHandleValid;
  size_t        arrayDataBufferSize;
  void*         arrayDataBuffer;
  bool          paramRefreshNeeded;  //Communication broken update handles and callbacks
  //timing
  ADSTIMEBASE   timeBase;
  uint64_t      plcTimeStampRaw;
  epicsTimeStamp plcTimeStamp;
  epicsTimeStamp epicsTimestamp;
}adsParamInfo;

typedef struct amsPortInfo{
  uint16_t amsPort;
  int connected;
  int paramsOK;
}amsPortInfo;

//For info from symbolic name Actually this data type should be in the adslib (but missing)..
typedef struct {
  uint32_t entryLen;
  uint32_t iGroup;
  uint32_t iOffset;
  uint32_t size;
  uint32_t dataType;
  uint32_t flags;
  uint16_t nameLength;
  uint16_t typeLength;
  uint16_t commentLength;
  char  buffer[768]; //256*3, 256 is string size in TwinCAT then 768 is max
  char* variableName;
  char* symDataType;
  char* symComment;
} adsSymbolEntry;

typedef enum{
  ADST_VOID     = 0,
  ADST_INT8     = 16,
  ADST_UINT8    = 17,
  ADST_INT16    = 2,
  ADST_UINT16   = 18,
  ADST_INT32    = 3,
  ADST_UINT32   = 19,
  ADST_INT64    = 20,
  ADST_UINT64   = 21,
  ADST_REAL32   = 4,
  ADST_REAL64   = 5,
  ADST_BIGTYPE  = 65,
  ADST_STRING   = 30,
  ADST_WSTRING  = 31,
  ADST_REAL80   = 32,
  ADST_BIT      = 33,
  ADST_MAXTYPES
} ADSDATATYPEID;

const char *adsErrorToString(long error);
const char *adsTypeToString(long type);
const char *asynTypeToString(long type);
const char *asynStateToString(long state);
const char *epicsStateToString(int state);
size_t adsTypeSize(long type);
asynParamType dtypStringToAsynType(char *dtype);
int windowsToEpicsTimeStamp(uint64_t plcTime, epicsTimeStamp *ts);

#endif /* ADSASYNPORTDRIVERUTILS_H_ */



