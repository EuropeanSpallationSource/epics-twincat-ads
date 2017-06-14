require asyn,4.31
require axis,USER
require ads,USER

epicsEnvSet("MOTOR_PORT",    "$(SM_MOTOR_PORT=MCU1)")

epicsEnvSet("AMSID",         "$(SM_AMSID=5.39.66.76.1.1)")
epicsEnvSet("IPADDR",        "$(SM_IPADDR=127.0.0.1)")
epicsEnvSet("ASYN_PORT",     "$(SM_ASYN_PORT=MC_CPU1)")
#epicsEnvSet("AXISCONFIG",    "cfgFile=../iocBoot/iocEthercatMC/SolAxis-48-1.cfg")
#epicsEnvSet("AXISCONFIG",    "getDebugText=getAxisDebugInfoData(1)")
epicsEnvSet("AXISCONFIG",    "")
epicsEnvSet("PREFIX",        "$(SM_PREFIX=IOC:)")
epicsEnvSet("MOTOR_NAME",    "$(SM_MOTOR_NAME=m1)")
epicsEnvSet("R",             "$(SM_R=m1-)")
epicsEnvSet("AXIS_NO",       "$(SM_AXIS_NO=1)")
epicsEnvSet("DESC",          "$(SM_DESC=motor1)")
epicsEnvSet("EGU",           "$(SM_EGU=mm)")
epicsEnvSet("PREC",          "$(SM_PREC=3)")
epicsEnvSet("VELO",          "$(SM_VELO=10.0)")
epicsEnvSet("JVEL",          "$(SM_JVEL=5.0)")
epicsEnvSet("JAR",           "$(SM_JAR=21)")
epicsEnvSet("ACCL",          "$(SM_ACCL=0.4)")
epicsEnvSet("MRES",          "$(SM_MRES=0.001)")
epicsEnvSet("ERES",          "$(SM_MRES=0.0)")
epicsEnvSet("RDBD",          "$(SM_RDBD=0.1)")
epicsEnvSet("NTMF",          "$(SM_NTMF=3)")
epicsEnvSet("MRES",          "$(SM_MRES=0.01)")
epicsEnvSet("DLLM",          "$(SM_DLLM=15)")
epicsEnvSet("DHLM",          "$(SM_DHLM=170)")

epicsEnvSet("HOMEPROC",      "$(SM_HOMEPROC=1)")
epicsEnvSet("HOMEPOS",       "$(SM_HOMEPOS=0.0)")
epicsEnvSet("HVELTO",        "$(SM_HVELTP=9)")
epicsEnvSet("HVELFRM",       "$(SM_HVELTP=4)")
epicsEnvSet("HOMEACC",       "$(SM_HOMEACC=24)")
epicsEnvSet("HOMEDEC",       "$(SM_HOMEDEC=26)")

epicsEnvSet("SCALRDBD",      "$(RDBD)")

< EthercatMCController.cmd
< EthercatMCAxis.cmd
