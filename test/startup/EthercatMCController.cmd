drvAsynAdsPortConfigure("$(ASYN_PORT)","$(IPADDR)","$(AMSID)",851,0, 0, 0)

asynOctetSetOutputEos("$(ASYN_PORT)", -1, ";\n")
asynOctetSetInputEos("$(ASYN_PORT)", -1, ";\n")
EthercatMCCreateController("$(MOTOR_PORT)", "$(ASYN_PORT)", "32", "200", "1000")


#asynDriver.h:
#define ASYN_TRACE_ERROR     0x0001
#define ASYN_TRACEIO_DEVICE  0x0002
#define ASYN_TRACEIO_FILTER  0x0004
#define ASYN_TRACEIO_DRIVER  0x0008
#define ASYN_TRACE_FLOW      0x0010
#define ASYN_TRACE_WARNING   0x0020
#define ASYN_TRACE_INFO      0x0040
asynSetTraceMask("$(ASYN_PORT)", -1, 0x41)
asynSetTraceMask("$(ASYN_PORT)", -1, 0xFF)


asynSetTraceIOMask("$(ASYN_PORT)", -1, 2)

# Bit 2: file/line
# Bit 3: thread
# Bit 0: Time
# Bit 1: Port
asynSetTraceInfoMask("$(ASYN_PORT)", -1, 3)
