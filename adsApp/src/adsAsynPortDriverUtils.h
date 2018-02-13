
#ifndef ADSASYNPORTDRIVERUTILS_H_
#define ADSASYNPORTDRIVERUTILS_H_

#include "asynPortDriver.h"
#include "AdsLib.h"
#include "adsCom.h"

#define ADS_MAX_FIELD_CHAR_LENGTH 128
#define ADS_ADR_COMMAND_PREFIX ".ADR."
#define ADS_OPTION_T_MAX_DLY_MS "T_DLY_MS"
#define ADS_OPTION_T_SAMPLE_RATE_MS "TS_MS"
#define ADS_OPTION_ADSPORT "ADSPORT"

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

const char *adsErrorToString(long error);
const char *adsTypeToString(long type);
const char *asynTypeToString(long type);
const char *asynStateToString(long state);
size_t adsTypeSize(long type);

#endif /* ADSASYNPORTDRIVERUTILS_H_ */



