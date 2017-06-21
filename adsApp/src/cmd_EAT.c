
/**\file
 * \ingroup ads
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "cmd.h"
#include "cmd_EAT.h"
#include "adsCom.h"
#include <inttypes.h>
#include <string.h>


//TODO: Cleanup macros.. should not need different for different types
#define SEND_OK_OR_ERROR_AND_RETURN(function)   \
  do {                                          \
    int iRet=function;                          \
    if(iRet){                                   \
      cmd_buf_printf(buffer,"Error: %d", iRet); \
      return 0;                                 \
    }                                           \
    cmd_buf_printf(buffer,"OK");                \
    return 0;                                   \
  }                                             \
  while(0)

#define SEND_RESULT_OR_ERROR_AND_RETURN_INT(function) \
  do {                                                \
    int iRet=function;                                \
    if(iRet){                                         \
      cmd_buf_printf(buffer,"Error: %d", iRet);       \
      return 0;                                       \
    }                                                 \
    cmd_buf_printf(buffer,"%d", iValue);              \
    return 0;                                         \
  }                                                   \
  while(0)

#define SEND_RESULT_OR_ERROR_AND_RETURN_DOUBLE(function) \
  do {                                                   \
    int iRet=function;                                   \
    if(iRet){                                            \
	cmd_buf_printf(buffer,"Error: %d", iRet);        \
      return 0;                                          \
    }                                                    \
    cmd_buf_printf(buffer,"%lf", fValue);                \
    return 0;                                            \
  }                                                      \
  while(0)

#define SEND_RESULT_OR_ERROR_AND_RETURN_UINT64(function) \
  do {                                                   \
    int iRet=function;                                   \
    if(iRet){                                            \
	cmd_buf_printf(buffer,"Error: %d", iRet);        \
      return 0;                                          \
    }                                                    \
    cmd_buf_printf(buffer,"%"PRIu64"", i64Value);        \
    return 0;                                            \
  }                                                      \
  while(0)

#define IF_ERROR_SEND_ERROR_AND_RETURN(function) \
  do {                                           \
    int iRet=function;                           \
    if(iRet){                                    \
      cmd_buf_printf(buffer,"Error: %d", iRet);  \
      return 0;                                  \
    }                                            \
  }                                              \
  while(0)

void init_axis(int axis_no)
{
  ;
}

static const char * const ADSPORT_equals_str = "ADSPORT=";

/*
  ADSPORT=501/.ADR.16#5001,16#B,2,2=1;
*/
static int motorHandleADS_ADR(const char *arg, uint16_t adsport,ecmcOutputBufferType *buffer)
{
  const char *myarg_1 = NULL;
  unsigned group_no = 0;
  unsigned offset_in_group = 0;
  unsigned len_in_PLC = 0;
  unsigned type_in_PLC = 0;
  int nvals;
  nvals = sscanf(arg, ".ADR.16#%x,16#%x,%u,%u=",
                 &group_no,
                 &offset_in_group,
                 &len_in_PLC,
                 &type_in_PLC);
  LOGINFO6("%s/%s:%d "
           "nvals=%d adsport=%u group_no=0x%x offset_in_group=0x%x len_in_PLC=%u type_in_PLC=%u\n",
           __FILE__, __FUNCTION__, __LINE__,
           nvals,
           adsport,
           group_no,
           offset_in_group,
           len_in_PLC,
           type_in_PLC);

  if (nvals != 4) return __LINE__;

  //WRITE
  myarg_1 = strchr(arg, '=');
  if (myarg_1) {
    myarg_1++; /* Jump over '=' */

    int error=adsWriteByGroupOffset(adsport,(uint32_t)group_no,(uint32_t) offset_in_group,(uint16_t)type_in_PLC,(uint32_t)len_in_PLC,myarg_1,buffer);
    if (error){
      RETURN_ERROR_OR_DIE(buffer,error,"%s/%s:%d myarg_1=%s err_code=%d",
                __FILE__, __FUNCTION__, __LINE__,
                myarg_1,
		error);
	return error;
    }
    cmd_buf_printf(buffer,"OK");
    return 0;
  }

  //READ
  myarg_1 = strchr(arg, '?');
  if (myarg_1) {
    myarg_1++; /* Jump over '?' */
    SYMINFOSTRUCT info;
    memset(&info,0,sizeof(info));
    info.adsDataType=type_in_PLC;
    info.byteSize=len_in_PLC;
    info.idxGroup=group_no;
    info.idxOffset=offset_in_group;

    int error=adsReadByGroupOffset(adsport,&info,buffer);
    if (error){
      RETURN_ERROR_OR_DIE(buffer,error,"%s/%s:%d myarg_1=%s err_code=%d",
                __FILE__, __FUNCTION__, __LINE__,
                myarg_1,
		error);

	return error;
    }
    return 0;
  }
  return __LINE__;
}

int motorHandleOneArg(const char *myarg_1,ecmcOutputBufferType *buffer)
{
  //const char *myarg = myarg_1;
  int err_code;

  LOGINFO4("INPUT TO motorHandleOnearg: %s \n",myarg_1);

  uint16_t adsport=0; //should actually be called amsport ( 851 for first plc as default) ...

  /* ADSPORT= */
  if (!strncmp(myarg_1, ADSPORT_equals_str, strlen(ADSPORT_equals_str))) {
    myarg_1 += strlen(ADSPORT_equals_str);
    int nvals=sscanf(myarg_1,"%" SCNu16,&adsport);
    if(nvals!=1){
      RETURN_ERROR_OR_DIE(buffer,__LINE__,"%s/%s:%d myarg_1=%s err_code=%d: ADS port parse error",
                      __FILE__, __FUNCTION__, __LINE__,
                      myarg_1,
                      __LINE__);
    }
    myarg_1=strchr(myarg_1, '/');
    myarg_1++;
  }

  /*.ADR.*/
  char *adr=strstr(myarg_1, ".ADR.");
  if(adr) {
    myarg_1 = adr;

    err_code = motorHandleADS_ADR(myarg_1,adsport,buffer);
    if (err_code == -1) return 0;
    if (err_code == 0) {
      return 0;
    }
    RETURN_ERROR_OR_DIE(buffer,err_code,"%s/%s:%d myarg_1=%s err_code=%d",
                  __FILE__, __FUNCTION__, __LINE__,
                  myarg_1,
                  err_code);
  }

  /*int Cfg.SetEnableFuncCallDiag(int nEnable);*/
  int iValue=0;
  int nvals = sscanf(myarg_1, "Cfg.SetEnableFuncCallDiag(%d)",&iValue);
  if (nvals == 1) {
    if(iValue){
      debug_print_flags|= 1<<4; //Fourth bit set (for use LOGINFO4)
    }
    else{
      debug_print_flags &= ~(1<<4); //Fourth bit reset (for use LOGINFO4)
    }
    return 0;
  }


  char variableName[255];
  memset(&variableName,0,sizeof(variableName));

  //symbolic write
  adr=strchr(myarg_1, '=');
  if(adr)
  {
    //Copy variable name
    strncpy(variableName,myarg_1,adr-myarg_1);
    adr++; //Jump over '='
    err_code = adsWriteByName(adsport,variableName,adr,buffer);
    if (err_code) {
      RETURN_ERROR_OR_DIE(buffer,err_code,"%s/%s:%d myarg_1=%s err_code=%d",
	                  __FILE__, __FUNCTION__, __LINE__,
	                  myarg_1,
	                  err_code);
    }
    cmd_buf_printf(buffer,"OK");
    return 0;
  }

  //symbolic read
  adr = strchr(myarg_1, '?');
  if (adr)
  {
    //Copy variable name
    strncpy(variableName,myarg_1,adr-myarg_1);
    variableName[adr-myarg_1]=0;
    err_code = adsReadByName(adsport,variableName,buffer);
    return err_code;
  }
  /*  if we come here, it is a bad command */
  cmd_buf_printf(buffer,"Error bad command");
  return 0;
}

int cmd_EAT(int argc, const char *argv[], const char *sepv[],ecmcOutputBufferType *buffer)
{
  const char *myargline = (argc > 0) ? argv[0] : "";
  int i;
  if (PRINT_STDOUT_BIT6())
  {
    const char *myarg[6];
    myarg[0] = myargline;
    myarg[1] = (argc >= 1) ? argv[1] : "";
    myarg[2] = (argc >= 2) ? argv[2] : "";
    myarg[3] = (argc >= 3) ? argv[3] : "";
    myarg[4] = (argc >= 4) ? argv[4] : "";
    myarg[5] = (argc >= 5) ? argv[5] : "";
    LOGINFO6("%s/%s:%d argc=%d "
             "myargline=\"%s\" myarg[1]=\"%s\" myarg[2]=\"%s\" myarg[3]=\"%s\" myarg[4]=\"%s\" myarg[5]=\"%s\"\n",
             __FILE__, __FUNCTION__, __LINE__,
             argc,  myargline,
             myarg[1], myarg[2], myarg[3], myarg[4], myarg[5]);
  }
  for (i = 1; i <= argc; i++) {
    int errorCode=motorHandleOneArg(argv[i],buffer);
    if(errorCode){
      RETURN_ERROR_OR_DIE(buffer,errorCode, "%s/%s:%d motorHandleOneArg returned errorcode: 0x%x\n",
	                __FILE__, __FUNCTION__, __LINE__,
                          errorCode);
    }
    cmd_buf_printf(buffer,"%s", sepv[i]);
    if (PRINT_STDOUT_BIT6()) {
      LOGINFO6("%s/%s:%d i=%d "
               "argv[%d]=%s, sepv[%d]=\"",
               __FILE__, __FUNCTION__, __LINE__,
               argc, i, argv[i], i);
      cmd_dump_to_std(sepv[i], strlen(sepv[i]));
      LOGINFO6("\"\n");
    }
  } /* while argc > 0 */
  return 0;
}
/******************************************************************************/
