/*
 * adsParameter.h
 *
 *  Created on: Feb 13, 2018
 *      Author: anderssandstrom
 */

#ifndef SRC_ADSPARAMETER_H_
#define SRC_ADSPARAMETER_H_

#include "asynPortDriver.h"
#include <dbCommon.h>
#include <dbBase.h>
#include <dbStaticLib.h>
#include <dbAccess.h>
#include "AdsLib.h"
#include "adsAsynPortDriverUtils.h"

#define ADS_ADR_COMMAND_PREFIX ".ADR."
#define ADS_OPTION_T_MAX_DLY_MS "T_DLY_MS"
#define ADS_OPTION_T_SAMPLE_RATE_MS "TS_MS"
#define ADS_OPTION_ADSPORT "ADSPORT"

const char*   driverName_="adsParameter";

class adsParameter
{
public:
  adsParameter (const char* asynPortName,
                asynUser *pasynUser,
                uint16_t defaultAmsport,
                double defaultMaxDelayTimeMS,
                double defaultSampleTimeMS);

  ~adsParameter();
  asynStatus setDrvInfo(char* drvInfo);
  asynParamType getAsynType();
  char* getDrvInfo();
  asynStatus setParamIndex(int index);
  asynStatus setAsynAddr(int addr);
  asynStatus updateWithInfoFromPLC(adsSymbolEntry *infoStruct);
  void reportParam(FILE *fp,int details);

private:
  void          initVars();
  asynStatus    validateDrvInfo(const char *drvInfo);
  asynParamType dtypStringToAsynType(char *dtype);
  asynStatus    parsePlcInfofromDrvInfo(const char *drvInfo);
  asynStatus    getRecordInfoFromDrvInfo(const char *drvInfo);

  char          *recordName_;
  char          *recordFieldType_;
  char          *recordFieldDtyp_;
  char          *recordFieldInp_;
  char          *recordFieldOut_;
  char          *drvInfo_;
  asynParamType asynType_;
  int           asynAddr_;
  bool          isIOIntr_;
  double        sampleTimeMS_;  //milli seconds
  double        maxDelayTimeMS_;  //milli seconds
  uint16_t      amsPort_;
  int           paramIndex_;
  bool          plcAbsAdrValid_;  //Symbolic address converted to abs address or .ADR. command parsed
  bool          isAdrCommand_;
  char          *plcAdrStr_;
  uint32_t      plcAbsAdrGroup_;
  uint32_t      plcAbsAdrOffset_;
  uint32_t      plcDataSize_;
  uint32_t      plcDataType_;
  bool          plcDataTypeWarn_;
  bool          plcDataIsArray_;
  //callback information
  uint32_t      hCallbackNotify_;
  bool          bCallbackNotifyValid_;
  uint32_t      hSymbolicHandle_;
  bool          bSymbolicHandleValid_;
  //Array buffer
  size_t        arrayDataBufferSize_;
  void*         arrayDataBuffer_;

  uint16_t      defaultAmsport_;
  double        defaultMaxDelayTimeMS_;
  double        defaultSampleTimeMS_;
  asynUser      *pasynUser_;
  AmsNetId      remoteNetId_;
  char*         asynPortName_;
};

#endif /* SRC_ADSPARAMETER_H_ */
