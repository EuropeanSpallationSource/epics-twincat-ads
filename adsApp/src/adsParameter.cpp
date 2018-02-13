/*
 * adsParameter.cpp
 *
 *  Created on: Feb 13, 2018
 *      Author: anderssandstrom
 */

#include "adsParameter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


adsParameter::adsParameter (const char* asynPortName,asynUser *pasynUser, uint16_t defaultAmsport,double defaultMaxDelayTimeMS,double defaultSampleTimeMS)
{
  initVars();
  pasynUser_=pasynUser;
  defaultAmsport_=defaultAmsport;
  defaultMaxDelayTimeMS_=defaultMaxDelayTimeMS;
  defaultSampleTimeMS_=defaultSampleTimeMS;
  if(pasynUser_){
    pasynUser->timeout=(defaultMaxDelayTimeMS_*2)/1000;
  }
  asynPortName_=strdup(asynPortName);
}

void adsParameter::initVars()
{
  recordName_=0;
  recordFieldType_=0;
  recordFieldDtyp_=0;
  recordFieldInp_=0;
  recordFieldOut_=0;
  drvInfo_=0;
  asynType_=asynParamNotDefined;
  asynAddr_=0;
  isIOIntr_=false;
  sampleTimeMS_=0;  //milli seconds
  maxDelayTimeMS_=0;  //milli seconds
  amsPort_=defaultAmsport_;
  plcAbsAdrValid_=false;  //Symbolic address converted to abs address or .ADR. command parsed
  isAdrCommand_=false;
  plcAdrStr_=0;
  plcAbsAdrGroup_=0;
  plcAbsAdrOffset_=0;
  plcDataSize_=0;
  plcDataType_=ADST_VOID;
  plcDataTypeWarn_=false;
  plcDataIsArray_=false;
   //callback information
  hCallbackNotify_=0;
  bCallbackNotifyValid_=false;
  hSymbolicHandle_=false;
  bSymbolicHandleValid_=false;
   //Array buffer
  arrayDataBufferSize_=0;;
  arrayDataBuffer_=0;
  paramIndex_=0;
  defaultAmsport_=0;
  defaultMaxDelayTimeMS_=0;
  defaultSampleTimeMS_=0;
  pasynUser_=0;
  remoteNetId_={0,0,0,0,0,0};
}

adsParameter::~adsParameter ()
{
  free(recordName_);
  free(recordFieldType_);
  free(recordFieldDtyp_);
  free(recordFieldInp_);
  free(recordFieldOut_);
  free(drvInfo_);
  free(plcAdrStr_);
  if(plcDataIsArray_){
    free(arrayDataBuffer_);
  }
  free(asynPortName_);
}
asynStatus adsParameter::setDrvInfo(char* drvInfo)
{
  if(validateDrvInfo(drvInfo)!=asynSuccess){
    return asynError;
  }

  asynStatus status=getRecordInfoFromDrvInfo(drvInfo);
  if(status!=asynSuccess){
    return asynError;
  }

  status=parsePlcInfofromDrvInfo(drvInfo);
  if(status!=asynSuccess){
    return asynError;
  }

  pasynUser_->timeout=(maxDelayTimeMS_*2)/1000;

  return asynSuccess;
}

asynParamType adsParameter::getAsynType()
{
  return asynType_;
}

char *adsParameter::getDrvInfo()
{
  return drvInfo_;
}

asynStatus adsParameter::setParamIndex(int index)
{
  paramIndex_=index;
  return asynSuccess;
}

asynStatus adsParameter::setAsynAddr(int addr)
{
  asynAddr_=addr;
  return asynSuccess;
}

asynStatus adsParameter::validateDrvInfo(const char *drvInfo)
{
  const char* functionName = "validateDrvInfo";
  asynPrint(pasynUser_, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName_, functionName,drvInfo);
  if(validateDrvInfo(drvInfo)!=asynSuccess){
    return asynError;
  }

  if(strlen(drvInfo)==0){
    asynPrint(pasynUser_,ASYN_TRACE_ERROR,"Invalid drvInfo string: Length 0 (%s).\n",drvInfo);
    return asynError;
  }

  //Check '?' mark last or '=' last
  const char* read=strrchr(drvInfo,'?');
  if(read){
    if(strlen(read)==1){
      return asynSuccess;
    }
  }

  const char* write=strrchr(drvInfo,'=');
  if(write){
    if(strlen(write)==1){
      return asynSuccess;
    }
  }

  asynPrint(pasynUser_,ASYN_TRACE_ERROR,"Invalid drvInfo string (%s).\n",drvInfo);
  return asynError;
}

asynStatus adsParameter::getRecordInfoFromDrvInfo(const char *drvInfo)
{
  const char* functionName = "getRecordInfoFromDrvInfo";
  asynPrint(pasynUser_, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName_, functionName,drvInfo);

  bool isInput=false;
  bool isOutput=false;
  amsPort_=defaultAmsport_;
  DBENTRY *pdbentry;
  pdbentry = dbAllocEntry(pdbbase);
  long status = dbFirstRecordType(pdbentry);
  bool recordFound=false;
  if(status) {
    dbFreeEntry(pdbentry);
    return asynError;
  }
  while(!status) {
    recordFieldType_=strdup(dbGetRecordTypeName(pdbentry));
    status = dbFirstRecord(pdbentry);
    while(!status) {
      recordName_=strdup(dbGetRecordName(pdbentry));
      if(!dbIsAlias(pdbentry)){
        status=dbFindField(pdbentry,"INP");
        if(!status){
          recordFieldInp_=strdup(dbGetString(pdbentry));
          isInput=true;
          char port[ADS_MAX_FIELD_CHAR_LENGTH];
          int adr;
          int timeout;
          char currdrvInfo[ADS_MAX_FIELD_CHAR_LENGTH];
          int nvals=sscanf(recordFieldInp_,"@asyn(%[^,],%d,%d)%s",port,&adr,&timeout,currdrvInfo);
          if(nvals==4){
            //Ensure correct port and drvinfo
            if(strcmp(port,asynPortName_)==0 && strcmp(drvInfo,currdrvInfo)==0){
              recordFound=true;  // Correct port and drvinfo!\n");
            }
          }
        }
        else{
          isInput=false;
        }
        status=dbFindField(pdbentry,"OUT");
        if(!status){
          recordFieldOut_=strdup(dbGetString(pdbentry));
          isOutput=true;
          char port[ADS_MAX_FIELD_CHAR_LENGTH];
          int adr;
          int timeout;
          char currdrvInfo[ADS_MAX_FIELD_CHAR_LENGTH];
          int nvals=sscanf(recordFieldOut_,"@asyn(%[^,],%d,%d)%s",port,&adr,&timeout,currdrvInfo);
          if(nvals==4){
            //Ensure correct port and drvinfo
            if(strcmp(port,asynPortName_)==0 && strcmp(drvInfo_,currdrvInfo)==0){
              recordFound=true;  // Correct port and drvinfo!\n");
            }
          }
        }
        else{
          isOutput=false;
        }

        if(recordFound){
          // Correct record found. Collect data from fields
          //DTYP
          status=dbFindField(pdbentry,"DTYP");
          if(!status){
            recordFieldDtyp_=strdup(dbGetString(pdbentry));
            asynType_=dtypStringToAsynType(dbGetString(pdbentry));
          }
          else{
            recordFieldDtyp_=0;
            asynType_=asynParamNotDefined;
          }

          //drvInput (not a field)
          drvInfo_=strdup(drvInfo);
          dbFreeEntry(pdbentry);
          return asynSuccess;  // The correct record was found and the paramInfo structure is filled
        }
        else{
          //Not correct record. Do cleanup.
          if(isInput){
            free(recordFieldInp_);
            recordFieldInp_=0;
          }
          if(isOutput){
            free(recordFieldOut_);
            recordFieldOut_=0;
          }
          drvInfo_=0;
          recordFieldDtyp_=0;
          isInput=false;
          isOutput=false;
        }
      }
      status = dbNextRecord(pdbentry);
      free(recordName_);
      recordName_=0;
    }
    status = dbNextRecordType(pdbentry);
    free(recordFieldType_);
    recordFieldType_=0;
  }
  dbFreeEntry(pdbentry);
  return asynError;
}

asynStatus adsParameter::parsePlcInfofromDrvInfo(const char* drvInfo)
{
  const char* functionName = "parsePlcInfofromDrvInfo";
  asynPrint(pasynUser_, ASYN_TRACE_INFO, "%s:%s: drvInfo: %s\n", driverName_, functionName,drvInfo);

  //Check if input or output
  const char* temp=strrchr(drvInfo,'?');
  if(temp){
    if(strlen(temp)==1){
      isIOIntr_=true; //All inputs will be created I/O intr
    }
    else{ //Must be '=' in end
      isIOIntr_=false;
    }
  }
  else{ //Must be '=' in end
    isIOIntr_=false;
  }

  asynPrint(pasynUser_, ASYN_TRACE_INFO, "%s:%s: drvInfo %s is %s\n", driverName_, functionName,drvInfo,isIOIntr_ ? "I/O Intr (end with ?)": " not I/O Intr (end with =)");

  //take part after last "/" if option or complete string..
  char plcAdrLocal[ADS_MAX_FIELD_CHAR_LENGTH];
  //See if option (find last '/')
  const char *drvInfoEnd=strrchr(drvInfo,'/');
  if(drvInfoEnd){ // found '/'
    int nvals=sscanf(drvInfoEnd,"/%s",plcAdrLocal);
    if(nvals==1){
      plcAdrStr_=strdup(plcAdrLocal);
      plcAdrStr_[strlen(plcAdrStr_)-1]=0; //Strip ? or = from end
    }
    else{
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse PLC address string from drvInfo (%s)\n", driverName_, functionName,drvInfo);
      return asynError;
    }
  }
  else{  //No options
    plcAdrStr_=strdup(drvInfo);  //Symbolic or .ADR.
    plcAdrStr_[strlen(plcAdrStr_)-1]=0; //Strip ? or = from end
  }

  //Check if .ADR. command
  const char *option=ADS_ADR_COMMAND_PREFIX;
  plcAbsAdrValid_=false;
  isAdrCommand_=false;
  const char *isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("16#%x,16#%x,%u,%u"))){
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s command from drvInfo (%s). String to short.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
    isAdrCommand_=true;
    int nvals;
    nvals = sscanf(isThere+strlen(option),"16#%x,16#%x,%u,%u",
             &plcAbsAdrGroup_,
             &plcAbsAdrOffset_,
             &plcDataSize_,
             &plcDataType_);

    if(nvals==4){
      plcAbsAdrValid_=true;
    }
    else{
      plcAbsAdrValid_=false;
      plcAbsAdrGroup_=-1;
      plcAbsAdrOffset_=-1;
      plcDataSize_=-1;
      plcDataType_=-1;
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s command from drvInfo (%s). Wrong format.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
  }

  //Check if ADS_OPTION_T_MAX_DLY_MS option
  option=ADS_OPTION_T_MAX_DLY_MS;
  isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("=0/"))){
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    nvals = sscanf(isThere+strlen(option),"=%lf/",
             &maxDelayTimeMS_);

    if(nvals!=1){
      maxDelayTimeMS_=defaultMaxDelayTimeMS_;
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
  }

  //Check if ADS_OPTION_T_SAMPLE_RATE_MS option
  option=ADS_OPTION_T_SAMPLE_RATE_MS;
  sampleTimeMS_=defaultSampleTimeMS_;
  isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("=0/"))){
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    nvals = sscanf(isThere+strlen(option),"=%lf/",
             &sampleTimeMS_);

    if(nvals!=1){
      sampleTimeMS_=defaultSampleTimeMS_;
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
  }

  //Check if ADS_OPTION_ADSPORT option
  option=ADS_OPTION_ADSPORT;
  amsPort_=defaultAmsport_;
  isThere=strstr(drvInfo,option);
  if(isThere){
    if(strlen(isThere)<(strlen(option)+strlen("=0/"))){
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). String to short.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
    int nvals;
    int val;
    nvals = sscanf(isThere+strlen(option),"=%d/",
             &val);
    if(nvals==1){
      amsPort_=(uint16_t)val;
    }
    else{
      amsPort_=defaultAmsport_;
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to parse %s option from drvInfo (%s). Wrong format.\n", driverName_, functionName,option,drvInfo);
      return asynError;
    }
  }

  //See if new amsPort_, then update list
/*  bool newAmsPort=true;
  for(uint16_t port : amsPortsList_){
    if(port==amsPort_){
      newAmsPort=false;
    }
  }
  if(newAmsPort){
    try{
      amsPortsList_.push_back(amsPort_);
      asynPrint(pasynUser_, ASYN_TRACE_INFO, "%s:%s: Added new amsPort to amsPortList: %d .\n", driverName_, functionName,(int)amsPort_);
    }
    catch(std::exception &e)
    {
      asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to add new amsPort to list for parameter %s. Exception: %s.\n", driverName_, functionName,drvInfo,e.what());
      return asynError;
    }
  }*/

  return asynSuccess;
}

asynStatus adsParameter::updateWithInfoFromPLC(adsSymbolEntry *infoStruct)
{
  const char* functionName = "readParamInfoFromPLC";
  asynPrint(pasynUser_, ASYN_TRACE_INFO, "%s:%s: : %s\n", driverName_, functionName,drvInfo_);


  infoStruct->variableName = infoStruct->buffer;

  if(infoStruct->nameLength>=sizeof(infoStruct->buffer)-1){
    asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Missalignment of type in AdsSyncReadWriteReqEx2 return struct for %s: 0x%x\n", driverName_, functionName,plcAdrStr_,ADS_COM_ERROR_READ_SYMBOLIC_INFO);
    return asynError;
  }
  infoStruct->symDataType = infoStruct->buffer+infoStruct->nameLength+1;

  if(infoStruct->nameLength + infoStruct->typeLength+2>=(uint16_t)(sizeof(infoStruct->buffer)-1)){
    asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Missalignment of comment in AdsSyncReadWriteReqEx2 return struct for %s: 0x%x\n", driverName_, functionName,plcAdrStr_,ADS_COM_ERROR_READ_SYMBOLIC_INFO);
  }
  infoStruct->symComment= infoStruct->symDataType+infoStruct->typeLength+1;


  //fill data in structure
  plcAbsAdrGroup_=infoStruct->iGroup; //However hopefully this adress should never be used (use symbol intstead since safer if memory moves in plc)..
  plcAbsAdrOffset_=infoStruct->iOffset; // -"- -"-
  plcDataSize_=infoStruct->size;  //Needed also for symbolic access
  plcDataType_=infoStruct->dataType;
  plcAbsAdrValid_=true;

  //check if array
  bool isArray=false;
  switch (plcDataType_) {
    case ADST_VOID:
      isArray=false;
      break;
    case ADST_STRING:
      isArray=true;  //Special case
      break;
    case ADST_WSTRING:
      isArray=true; //Special case?
      break;
    case ADST_BIGTYPE:
      isArray=false;
      break;
    case ADST_MAXTYPES:
      isArray=false;
      break;
    default:
      isArray=plcDataSize_>adsTypeSize(plcDataType_);
      break;
  }
  plcDataIsArray_=isArray;

  // Allocate memory for array
  if(isArray){
    if(plcDataSize_!=arrayDataBufferSize_ && arrayDataBuffer_){ //new size of array
      free(arrayDataBuffer_);
      arrayDataBuffer_=NULL;
    }
    if(!arrayDataBuffer_){
      arrayDataBuffer_= calloc(plcDataSize_,1);
      arrayDataBufferSize_=plcDataSize_;
      if(!arrayDataBuffer_){
        asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s:%s: Failed to allocate memory for array data for %s.\n.", driverName_, functionName,drvInfo_);
        return asynError;
      }
      memset(arrayDataBuffer_,0,plcDataSize_);
    }
  }
  return asynSuccess;
}

void adsParameter::reportParam(FILE *fp,int details)
{
  fprintf(fp,"    Param name:                %s\n",drvInfo_);
  fprintf(fp,"    Param index:               %d\n",paramIndex_);
  fprintf(fp,"    Param type:                %s (%d)\n",asynTypeToString((long)asynType_),asynType_);
  fprintf(fp,"    Param sample time [ms]:    %lf\n",sampleTimeMS_);
  fprintf(fp,"    Param max delay time [ms]: %lf\n",maxDelayTimeMS_);
  fprintf(fp,"    Param isIOIntr:            %s\n",isIOIntr_ ? "true" : "false");
  fprintf(fp,"    Param asyn addr:           %d\n",asynAddr_);
  fprintf(fp,"    Param array buffer alloc:  %s\n",arrayDataBuffer_ ? "true" : "false");
  fprintf(fp,"    Param array buffer size:   %lu\n",arrayDataBufferSize_);
  fprintf(fp,"    Plc ams port:              %d\n",amsPort_);
  fprintf(fp,"    Plc adr str:               %s\n",plcAdrStr_);
  fprintf(fp,"    Plc adr str is ADR cmd:    %s\n",isAdrCommand_ ? "true" : "false");
  fprintf(fp,"    Plc abs adr valid:         %s\n",plcAbsAdrValid_ ? "true" : "false");
  fprintf(fp,"    Plc abs adr group:         16#%x\n",plcAbsAdrGroup_);
  fprintf(fp,"    Plc abs adr offset:        16#%x\n",plcAbsAdrOffset_);
  fprintf(fp,"    Plc data type:             %s\n",adsTypeToString(plcDataType_));
  fprintf(fp,"    Plc data type size:        %lu\n",adsTypeSize(plcDataType_));
  fprintf(fp,"    Plc data size:             %u\n",plcDataSize_);
  fprintf(fp,"    Plc data is array:         %s\n",plcDataIsArray_ ? "true" : "false");
  fprintf(fp,"    Plc data type warning:     %s\n",plcDataTypeWarn_ ? "true" : "false");
  fprintf(fp,"    Ads hCallbackNotify:       %u\n",hCallbackNotify_);
  fprintf(fp,"    Ads hSymbHndle:            %u\n",hSymbolicHandle_);
  fprintf(fp,"    Ads hSymbHndleValid:       %s\n",bSymbolicHandleValid_ ? "true" : "false");
  fprintf(fp,"    Record name:               %s\n",recordName_);
  fprintf(fp,"    Record type:               %s\n",recordFieldType_);
  fprintf(fp,"    Record dtyp:               %s\n",recordFieldDtyp_);
}
