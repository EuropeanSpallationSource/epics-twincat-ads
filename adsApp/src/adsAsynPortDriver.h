
#ifndef ADSASYNPORTDRIVER_H_
#define ADSASYNPORTDRIVER_H_

#include "asynPortDriver.h"
#include <epicsEvent.h>
#include <dbCommon.h>
#include <dbBase.h>
#include <dbStaticLib.h>
#include "AdsLib.h"

#define MAX_FIELD_CHAR_LENGTH 128
#define ADR_COMMAND_PREFIX ".ADR."

typedef struct adsParamInfo{
  char          *recordName;
  char          *recordType;
  char          *scan;
  char          *dtyp;
  char          *inp;
  char          *out;
  char          *drvInfo;
  asynParamType asynType;
  bool          isInput;
  bool          isOutput;
  uint16_t      amsPort;
  int           paramIndex;  //aslo used as hUser for ads callback
  bool          plcAbsAdrValid;  //Symbolic address converted to abs address or .ADR. command parsed
  bool          isAdrCommand;
  char          *plcSymAdr;
  uint32_t      plcGroup;
  uint32_t      plcOffsetInGroup;
  uint32_t      plcSize;
  uint32_t      plcDataType;
  //callback information
  uint32_t      hCallbackNotify;
  uint32_t      hSymbolicHandle;
  bool          hSymbolicHandleValid;
}adsParamInfo;

//For info from symbolic name Actually this data type should be in the adslib (but missing)..
typedef struct {
  uint32_t entryLen;
  uint32_t iGroup;
  uint32_t iOffset;
  uint32_t size;
  uint32_t dataType;
  uint32_t flags;
  uint16_t nameLength; //why??
  uint16_t typeLength; //why??
  uint16_t commentLength; //why??
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
                    int noProcessEos);
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
  virtual asynStatus readFloat64(asynUser *pasynUser,
                                 epicsFloat64 *value);
  virtual asynStatus readInt8Array(asynUser *pasynUser,
                                   epicsInt8 *value,
                                   size_t nElements,
                                   size_t *nIn);
  virtual asynStatus readInt16Array(asynUser *pasynUser,
                                    epicsInt16 *value,
                                    size_t nElements,
                                    size_t *nIn);
  virtual asynStatus readInt32Array(asynUser *pasynUser,
                                    epicsInt32 *value,
                                    size_t nElements,
                                    size_t *nIn);
  virtual asynStatus readFloat32Array(asynUser *pasynUser,
                                      epicsFloat32 *value,
                                      size_t nElements,
                                      size_t *nIn);
  virtual asynStatus readFloat64Array(asynUser *pasynUser,
                                      epicsFloat64 *value,
                                      size_t nElements,
                                      size_t *nIn);
  asynUser *getTraceAsynUser();
protected:

private:
  //Asyn and EPICS stuff
  asynStatus getRecordInfoFromDrvInfo(const char *drvInfo,
                                      adsParamInfo *paramInfo);
  asynStatus parsePlcInfofromDrvInfo(const char* drvInfo,
                                     adsParamInfo *paramInfo);
  asynParamType dtypStringToAsynType(char *dtype);
  int getAmsPortFromDrvInfo(const char* drvInfo);
  void printParamInfo(adsParamInfo *paramInfo);
  // ADS methods
  asynStatus adsAddNotificationCallback(adsParamInfo *paramInfo);
  asynStatus adsDelNotificationCallback(adsParamInfo *paramInfo);
  asynStatus adsGetSymInfoByName(adsParamInfo *paramInfo);
  asynStatus adsGetSymHandleByName(adsParamInfo *paramInfo);
  asynStatus adsReleaseSymbolicHandle(adsParamInfo *paramInfo);
  asynStatus adsConnect();
  asynStatus adsDisconnect();
  asynStatus adsWrite(adsParamInfo *paramInfo,
                      const void *binaryBuffer);
  epicsEventId eventId_;
  const char *portName_;
  const char *ipaddr_;
  const char *amsaddr_;
  uint16_t amsport_;
  unsigned int priority_;
  int autoConnect_;
  int noProcessEos_;
  adsParamInfo **pAdsParamArray_;
  int pAdsParamArrayCount_;
  int paramTableSize_;

  //ADS
  long adsPort_; //handle
  AmsNetId remoteNetId_;
};

#endif /* ADSASYNPORTDRIVER_H_ */



