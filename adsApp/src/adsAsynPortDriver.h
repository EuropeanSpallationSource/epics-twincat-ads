#ifndef ADSASYNPORTDRIVER_H_
#define ADSASYNPORTDRIVER_H_

#include "asynPortDriver.h"
#include <epicsEvent.h>
#include <dbCommon.h>
#include <dbBase.h>
#include <dbStaticLib.h>
#include "AdsLib.h"
#include <vector>
#include "adsAsynPortDriverUtils.h"

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
  bool          amsPortConnected;
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
  //Array buffer
  size_t        arrayDataBufferSize;
  void*         arrayDataBuffer;
}adsParamInfo;

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
                    int maxDelayTimeMS,
                    int adsTimeoutMS);

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
  asynStatus refreshParams();
  // ADS methods
  asynStatus adsAddNotificationCallback(adsParamInfo *paramInfo);
  asynStatus adsDelNotificationCallback(adsParamInfo *paramInfo);
  asynStatus adsGetSymInfoByName(adsParamInfo *paramInfo);
  asynStatus adsGetSymHandleByName(adsParamInfo *paramInfo);
  asynStatus adsReleaseSymbolicHandle(adsParamInfo *paramInfo);
  asynStatus adsConnect();
  asynStatus adsDisconnect();
  asynStatus adsWriteParam(adsParamInfo *paramInfo,
                      const void *binaryBuffer,
                      uint32_t bytesToWrite);
  asynStatus adsReadParam(adsParamInfo *paramInfo);
  asynStatus adsReadState(uint16_t *adsState);
  asynStatus adsReadState(uint16_t amsport,
                          uint16_t *adsState);
  asynStatus adsGenericArrayWrite(int paramIndex,
                                  long allowedType,
                                  const void *epicsDataBuffer,
                                  size_t nEpicsBufferBytes);
  asynStatus adsGenericArrayRead(int paramIndex,
                                 long allowedType,
                                 void *epicsDataBuffer,
                                 size_t nEpicsBufferBytes,
                                 size_t *nBytesRead);
  asynStatus updateParamInfoWithPLCInfo(adsParamInfo *paramInfo);

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
  int adsTimeoutMS_;
  //ADS
  long adsPort_; //handle
  AmsNetId remoteNetId_;
  int connected_;
  int paramRefreshNeeded_;
  std::vector<std::uint16_t> amsPortsList_;
};

#endif /* ADSASYNPORTDRIVER_H_ */



