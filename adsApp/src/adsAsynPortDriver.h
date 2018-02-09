
#ifndef ADSASYNPORTDRIVER_H_
#define ADSASYNPORTDRIVER_H_

#include "asynPortDriver.h"
#include <epicsEvent.h>
#include <dbCommon.h>
#include <dbBase.h>
#include <dbStaticLib.h>
#include "AdsLib.h"

#define ADS_MAX_FIELD_CHAR_LENGTH 128
#define ADS_ADR_COMMAND_PREFIX ".ADR."
#define ADS_OPTION_T_MAX_DLY_MS "T_DLY_MS"
#define ADS_OPTION_T_SAMPLE_RATE_MS "TS_MS"
#define ADS_OPTION_ADSPORT "ADSPORT"

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
  //bool          isInput;
  //bool          isOutput;
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
  //callback information
  uint32_t      hCallbackNotify;
  bool          bCallbackNotifyValid;
  uint32_t      hSymbolicHandle;
  bool          bSymbolicHandleValid;
  void*         arrayDataBuffer;
}adsParamInfo;

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

class adsAsynPortDriver : public asynPortDriver {
public:
  adsAsynPortDriver(const char *portName,
                    const char *ipaddr,
                    const char *amsaddr,
                    unsigned int amsport,
                    int paramTableSize,
                    unsigned int priority,
                    int autoConnect,
                    int noProcessEos,
                    int defaultSampleTimeMS,
                    int maxDelayTimeMS);

  virtual ~adsAsynPortDriver();
  virtual void report(FILE *fp, int details);
  virtual asynStatus disconnect(asynUser *pasynUser);
  virtual asynStatus connect(asynUser *pasynUser);
  virtual asynStatus drvUserCreate(asynUser *pasynUser,
                                   const char *drvInfo,
                                   const char **pptypeName,
                                   size_t *psize);
  virtual asynStatus writeOctet(asynUser *pasynUser,
                                const char *value,
                                size_t maxChars,
                                size_t *nActual);
  virtual asynStatus readOctet(asynUser *pasynUser,
                               char *value,
                               size_t maxChars,
                               size_t *nActual,
                               int *eomReason);
  virtual asynStatus writeInt32(asynUser *pasynUser,
                                epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser,
                                  epicsFloat64 value);
  virtual asynStatus readInt8Array(asynUser *pasynUser,
                                   epicsInt8 *value,
                                   size_t nElements,
                                   size_t *nIn);
  virtual asynStatus writeInt8Array(asynUser *pasynUser,
                                    epicsInt8 *value,
                                    size_t nElements);
  virtual asynStatus readInt16Array(asynUser *pasynUser,
                                    epicsInt16 *value,
                                    size_t nElements,
                                    size_t *nIn);
  virtual asynStatus writeInt16Array(asynUser *pasynUser,
                                     epicsInt16 *value,
                                     size_t nElements);
  virtual asynStatus readInt32Array(asynUser *pasynUser,
                                    epicsInt32 *value,
                                    size_t nElements,
                                    size_t *nIn);
  virtual asynStatus writeInt32Array(asynUser *pasynUser,
                                     epicsInt32 *value,
                                     size_t nElements);
  virtual asynStatus readFloat32Array(asynUser *pasynUser,
                                      epicsFloat32 *value,
                                      size_t nElements,
                                      size_t *nIn);
  virtual asynStatus writeFloat32Array(asynUser *pasynUser,
                                       epicsFloat32 *value,
                                       size_t nElements);
  virtual asynStatus readFloat64Array(asynUser *pasynUser,
                                      epicsFloat64 *value,
                                      size_t nElements,
                                      size_t *nIn);
  virtual asynStatus writeFloat64Array(asynUser *pasynUser,
                                       epicsFloat64 *value,
                                       size_t nElements);
  asynUser *getTraceAsynUser();
  int getParamTableSize();
  adsParamInfo *getAdsParamInfo(int index);
  asynStatus adsUpdateParameter(adsParamInfo* paramInfo,
                                 const void *data,
                                 size_t dataSize);
  void cyclicThread();
protected:

private:
  //Asyn and EPICS methods
  asynStatus addParam(asynUser *pasynUser,const char *drvInfo,int *index);
  asynStatus validateDrvInfo(const char *drvInfo);
  asynStatus getRecordInfoFromDrvInfo(const char *drvInfo,
                                      adsParamInfo *paramInfo);
  asynStatus parsePlcInfofromDrvInfo(const char* drvInfo,
                                     adsParamInfo *paramInfo);
  asynParamType dtypStringToAsynType(char *dtype);
  asynStatus reconnect();
  // ADS methods
  asynStatus adsAddNotificationCallback(adsParamInfo *paramInfo);
  asynStatus adsDelNotificationCallback(adsParamInfo *paramInfo);
  asynStatus adsGetSymInfoByName(adsParamInfo *paramInfo);
  asynStatus adsGetSymHandleByName(adsParamInfo *paramInfo);
  asynStatus adsReleaseSymbolicHandle(adsParamInfo *paramInfo);
  asynStatus adsConnect();
  asynStatus adsAddRoute();
  asynStatus adsDisconnect();
  asynStatus adsWrite(adsParamInfo *paramInfo,
                      const void *binaryBuffer,
                      uint32_t bytesToWrite);
  asynStatus adsRead(adsParamInfo *paramInfo);
  asynStatus adsReadState(uint16_t *adsState);
  asynStatus adsGenericArrayWrite(int paramIndex,
                                  long allowedType,
                                  const void *epicsDataBuffer,
                                  size_t nEpicsBufferBytes);
  asynStatus adsGenericArrayRead(int paramIndex,
                                 long allowedType,
                                 void *epicsDataBuffer,
                                 size_t nEpicsBufferBytes,
                                 size_t *nBytesRead);
  //Static methods
  static const char *adsErrorToString(long error);
  static const char *adsTypeToString(long type);
  static const char *asynTypeToString(long type);
  static const char *asynStateToString(long state);
  static size_t adsTypeSize(long type);

  //Variables
  char *ipaddr_;
  char *amsaddr_;
  uint16_t amsportDefault_;
  unsigned int priority_;
  int autoConnect_;
  int noProcessEos_;
  adsParamInfo **pAdsParamArray_;
  int adsParamArrayCount_;
  int paramTableSize_;
  int defaultSampleTimeMS_;
  int defaultMaxDelayTimeMS_;
  //ADS
  long adsPort_; //handle
  AmsNetId remoteNetId_;
};

#endif /* ADSASYNPORTDRIVER_H_ */



