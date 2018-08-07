//
//  RKFoundation.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 3/17/15.
//
//

#ifndef __RadarKit_Foundation__
#define __RadarKit_Foundation__

#include <RadarKit/RKTypes.h>
#include <RadarKit/RKMisc.h>

#define RKDefaultLogfile                 "messages.log"
#define RKEOL                            "\r\n"

// Compute the next/previous N-stride location with size S
#define RKNextNModuloS(i, N, S)          ((i) >= (S) - (N) ? (i) + (N) - (S) : (i) + (N))
#define RKPreviousNModuloS(i, N, S)      ((i) < (N) ? (S) + (i) - (N) : (i) - (N))

// Compute the next/previous location with size S
#define RKNextModuloS(i, S)              ((i) == (S) - 1 ? 0 : (i) + 1)
#define RKPreviousModuloS(i, S)          ((i) == 0 ? (S) - 1 : (i) - 1)

//
// Z in [-32.0    ... 95.5]           Zi = (Z) x 2 + 64
// V in [-16.0    ... 15.875]         Vi = (V) x 8 + 128
// W in [ -0.05   ...  5.0]           Wi = (W) x 20 + 0
// D in [-10.0    ... 15.5]           Di = (D) x 10 + 100
// P in [  -PI    ... PI*(N-1)/N]     Pi = (P) x 128/M_PI + 128
// R in [  0.0    ... 1.079]
// K in [ -0.1*PI ... 0.1*PI*(N-1)/N] Ki = (K) x 1280/M_PI + 128
//
//
// Z  in [CLR  -31.50  ... 95.5]            Zi = (Z) * 2 + 64
// V  in [CLR  -15.875 ... 15.875]          Vi = (V) * 8 + 128
// W  in [CLR    0.00  ... 12.70]           Wi = (W) * 20 + 1
// D  in [CLR   -9.90  ... 15.5]            Di = (D) * 10 + 100
// P  in [CLR    -PI   ... +PI]             Pi = (P) * 255/(2*M_PI) + 128.5
// R  in [CLR   0.033  ... 1.079]
// K  in [CLR -0.1*PI  ... +0.1*PI]         Ki = (K) * 255/(20*M_PI) + 128.5
// VE in [CLR   -63.5  ... 63.5]            Vi = (V) * 2 + 128
//

#define RKZLHMAC  { lhma[0] = -32.0f;     lhma[1] = 95.5f;       lhma[2] = 2.0f;      lhma[3] =  64.0f; }  //
#define RKVLHMAC  { lhma[0] = -16.0f;     lhma[1] = 15.875f;     lhma[2] = 8.0f;      lhma[3] = 128.0f; }  //
#define RKWLHMAC  { lhma[0] = -0.05f;     lhma[1] = 12.70f;      lhma[2] = 20.0f;     lhma[3] =   1.0f; }  //
#define RKDLHMAC  { lhma[0] = -10.0f;     lhma[1] = 15.5f;       lhma[2] = 10.0f;     lhma[3] = 100.0f; }  //
#define RKPLHMAC  { lhma[0] = -3.16623f;  lhma[1] = 3.11695f;    lhma[2] = 40.5845f;  lhma[3] = 128.5f; }  //  pi - 2*pi/255
#define RKKLHMAC  { lhma[0] = -0.558508f; lhma[1] = 0.55414249f; lhma[2] = 229.1831f; lhma[3] = 128.0f; }  //  -0.2*pi + 0.4*pi/255
#define RKRLHMAC  { lhma[0] = 0.0f;       lhma[1] = 1.079f;      lhma[2] = 1.0f;      lhma[3] =   0.0f; }  //
#define RKV2LHMAC { lhma[0] = -32.0f;     lhma[1] = 31.75f;      lhma[2] = 4.0f;      lhma[3] = 128.0f; }  //
#define RKV3LHMAC { lhma[0] = -64.0f;     lhma[1] = 63.5f;       lhma[2] = 2.0f;      lhma[3] = 128.0f; }  //

#define RKRho2Uint8(r)    (r > 0.93f ? roundf((r - 0.93f) * 1000.0f) + 106.0f : (r > 0.7f ? roundf((r - 0.7f) * 300.0f) + 37.0f : roundf(r * 52.8571f)))

#define RKSingleWrapTo2PI(x)   ((x) < -M_PI ? ((x) + 2.0f * M_PI) : ((x) >= M_PI ? ((x) - 2.0f * M_PI) : (x)))

typedef struct RKGlobalParameterStruct {
    char             program[32];                                // Name of the program in log
    char             logfile[RKMaximumPathLength];               // Name of the log file. This is ignored when dailyLog = true
    char             rootDataFolder[RKMaximumPathLength - 80];   // Root folder where iq, moment health and log files are stored
    bool             dailyLog;                                   // Daily mode where log file is /{rootDataFolder}/log/YYYYMMDD.log
    bool             showColor;                                  // Show colors
    pthread_mutex_t  mutex;                                      // Mutual exclusive access
    FILE             *stream;                                    // Secondary output stream, can be NULL
} RKGlobalParamters;

extern RKGlobalParamters rkGlobalParameters;
extern const char * const rkResultStrings[];

#pragma mark - Common Functions

// Log
int RKLog(const char *, ...);

// Presentation
void RKSetWantColor(const bool);
void RKSetWantScreenOutput(const bool);
void RKSetUseDailyLog(const bool);
int RKSetProgramName(const char *);
int RKSetRootFolder(const char *);
int RKSetLogfile(const char *);
int RKSetLogfileToDefault(void);

// Filename / string
bool RKGetSymbolFromFilename(const char *filename, char *symbol);

// Common numeric output
void RKShowTypeSizes(void);
void RKShowVecFloat(const char *name, const float *p, const int n);
void RKShowVecIQZ(const char *name, const RKIQZ *p, const int n);
void RKShowArray(const RKFloat *data, const char *letter, const int width, const int height);
char *RKStringFromValue(const void *value, RKValueType type);
char *RKVariableInString(const char *name, const void *value, RKValueType type);

// Clearing buffer
void RKZeroOutFloat(RKFloat *data, const uint32_t capacity);
void RKZeroOutIQZ(RKIQZ *data, const uint32_t capacity);
void RKZeroTailFloat(RKFloat *data, const uint32_t capacity, const uint32_t origin);
void RKZeroTailIQZ(RKIQZ *data, const uint32_t capacity, const uint32_t origin);

// Pulse
size_t RKPulseBufferAlloc(RKBuffer *, const uint32_t capacity, const uint32_t pulseCount);
void RKPulseBufferFree(RKBuffer);
RKPulse *RKGetPulse(RKBuffer, const uint32_t pulseIndex);
RKInt16C *RKGetInt16CDataFromPulse(RKPulse *, const uint32_t channelIndex);
RKComplex *RKGetComplexDataFromPulse(RKPulse *, const uint32_t channelIndex);
RKIQZ RKGetSplitComplexDataFromPulse(RKPulse *, const uint32_t channelIndex);
int RKClearPulseBuffer(RKBuffer, const uint32_t pulseCount);

// Ray
size_t RKRayBufferAlloc(RKBuffer *, const uint32_t capacity, const uint32_t rayCount);
void RKRayBufferFree(RKBuffer);
RKRay *RKGetRay(RKBuffer, const uint32_t rayIndex);
uint8_t *RKGetUInt8DataFromRay(RKRay *, const uint32_t productIndex);
float *RKGetFloatDataFromRay(RKRay *, const uint32_t productIndex);
int RKClearRayBuffer(RKBuffer buffer, const uint32_t rayCount);

// Scratch space for moment processors
size_t RKScratchAlloc(RKScratch **space, const uint32_t capacity, const uint8_t lagCount, const bool);
void RKScratchFree(RKScratch *);

// Standalone file monitor (one file per thread)
RKFileMonitor *RKFileMonitorInit(const char *filename, void (*)(void *), void *);
int RKFileMonitorFree(RKFileMonitor *);

// Stream symbols / binary
RKStream RKStreamFromString(const char *);
char *RKStringOfStream(RKStream);
int RKStringFromStream(char *, RKStream);
int RKGetNextProductDescription(char *symbol, char *name, char *unit, char *colormap, RKBaseMomentIndex *, RKBaseMomentList *);

// Parser, enum, strings
size_t RKParseCommaDelimitedValues(void *, RKValueType , const size_t, const char *);
size_t RKParseNumericArray(void *, RKValueType, const size_t, const char *);
void RKParseQuotedStrings(const char *source, ...);
void RKMakeJSONStringFromControls(char *, RKControl *, uint32_t count);
RKStatusEnum RKValueToEnum(RKConst value, RKConst tlo, RKConst lo, RKConst nlo, RKConst nhi, RKConst hi, RKConst thi);
RKStatusEnum RKStatusFromTemperatureForCE(RKConst value);
RKStatusEnum RKStatusFromTemperatureForIE(RKConst value);
RKStatusEnum  RKStatusFromTemperatureForComputers(RKConst value);
bool RKFindCondition(const char *, const RKStatusEnum, const bool, char *firstKey, char *firstValue);
bool RKAnyCritical(const char *, const bool, char *firstKey, char *firstValue);
int RKParseProductDescription(RKProductDesc *, const char *);
RKProductId RKProductIdFromString(const char *);
RKIdentifier RKIdentifierFromString(const char *);

// Simple engine
int RKSimpleEngineFree(RKSimpleEngine *);

#endif /* defined(__RadarKit_RKFoundation__) */
