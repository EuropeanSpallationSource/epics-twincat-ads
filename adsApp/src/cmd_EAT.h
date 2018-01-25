#ifndef CMD_EAT_H
#define CMD_EAT_H

#include "cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

  int cmd_EAT(int argc, const char *argv[], const char *seperator[],adsOutputBufferType *buffer);
  int motorHandleOneArg(const char *myarg_1,adsOutputBufferType *buffer);

#ifdef __cplusplus
}
#endif

#endif /* CMD_EAT_H */
