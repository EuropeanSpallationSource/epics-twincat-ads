#ifndef ADSCOM_H
#define ADSCOM_H

#define __STDC_FORMAT_MACROS

#include "cmd.h"

#include <inttypes.h>
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_SIZE 1024
#define DUT_AXIS_STATUS "DUT_AxisStatus_v0_01"

#define ADS_COM_ERROR_INVALID_AMS_PORT 1000
#define ADS_COM_ERROR_INVALID_AMS_ADDRESS 1001
#define ADS_COM_ERROR_OPEN_ADS_PORT_FAIL 1002
//#define ADS_COM_ERROR_ADD_ADS_ROUTE_FAIL 1003
#define ADS_COM_ERROR_INVALID_DATA_TYPE 1004
#define ADS_COM_ERROR_ADS_READ_BUFFER_INDEX_EXCEEDED_SIZE 1005
#define ADS_COM_ERROR_BUFFER_TO_EPICS_FULL 1006
#define ADS_COM_ERROR_READ_SYMBOLIC_INFO 1007

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

typedef struct {
  uint32_t symEntryLen;
  uint32_t idxGroup;
  uint32_t idxOffset;
  uint32_t byteSize;
  uint16_t adsDataType;
  uint32_t dummy1; //why??
  uint32_t dummy2; //why??
  uint16_t dummy3; //why??
  char  buffer[768]; //256*3, 256 is string size in TwinCAT
  char* variableName;
  char* symDataType;
  char* symComment;
} SYMINFOSTRUCT;

typedef struct {
  char bEnable;
  char bReset;
  char bExecute;
  uint16_t nCommand;
  uint16_t nCmdData;
  double fVelocity;
  double fPosition;
  double fAcceleration;
  double fDeceleration;
  char bJogFwd;
  char bJogBwd;
  char bLimitFwd;
  char bLimitBwd;
  double fOverride;
  char bHomeSensor;
  char bEnabled;
  char bError;
  uint32_t nErrorId;
  double fActVelocity;
  double fActPosition;
  double fActDiff;
  char bHomed;
  char bBusy;
} STAXISSTATUSSTRUCT;
int setAmsNetId(uint8_t  *netId);
int setAdsPort(long  adsPort);
int setAmsPort(uint16_t  amsPort);
long adsReadByName(uint16_t amsPort,const char *variableAddr,adsOutputBufferType *outBuffer);
int adsReadByGroupOffset(uint16_t amsPort,SYMINFOSTRUCT *info, adsOutputBufferType *outBuffer);
int adsWriteByName(uint16_t amsPort,const char *variableAddr,char *asciiValueToWrite,adsOutputBufferType *outBuffer);
int adsWriteByGroupOffset(uint16_t amsPort,uint32_t group, uint32_t offset,uint16_t dataType,uint32_t dataSize,const char *asciiValueToWrite,adsOutputBufferType *asciiResponseBuffer);
//int adsConnect(const char *ipaddr,const char *amsaddr, int amsport);
//int adsDisconnect();

#ifdef __cplusplus
}
#endif

#endif /* ADSCOM_H */
