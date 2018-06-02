//
//  RKRadar.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 3/17/15.
//
//

#ifndef __RadarKit_Radar__
#define __RadarKit_Radar__

#include <RadarKit/RKFoundation.h>
#include <RadarKit/RKServer.h>
#include <RadarKit/RKClient.h>
#include <RadarKit/RKClock.h>
#include <RadarKit/RKConfig.h>
#include <RadarKit/RKHealth.h>
#include <RadarKit/RKPosition.h>
#include <RadarKit/RKPulseCompression.h>
#include <RadarKit/RKPulseRingFilter.h>
#include <RadarKit/RKMoment.h>
#include <RadarKit/RKHealthLogger.h>
#include <RadarKit/RKDataRecorder.h>
#include <RadarKit/RKSweep.h>
#include <RadarKit/RKWaveform.h>
#include <RadarKit/RKPreference.h>
#include <RadarKit/RKFileManager.h>
#include <RadarKit/RKRadarRelay.h>
#include <RadarKit/RKHostMonitor.h>

#define xstr(s) str(s)
#define str(s) #s
#define RADAR_VARIABLE_OFFSET(STRING, NAME) \
sprintf(STRING, "                    radar->" xstr(NAME) " @ %ld -> %p\n", (unsigned long)((void *)&radar->NAME - (void *)radar), (unsigned int *)&radar->NAME)

typedef uint32_t RKRadarState;                                     // Everything allocated and live: 0x81ff0555
enum RKRadarState {
    RKRadarStateRayBufferAllocating                  = (1 << 0),   // Data buffers
    RKRadarStateRayBufferInitialized                 = (1 << 1),   //
    RKRadarStateRawIQBufferAllocating                = (1 << 2),   //
    RKRadarStateRawIQBufferInitialized               = (1 << 3),   //
    RKRadarStateStatusBufferAllocating               = (1 << 4),   //
    RKRadarStateStatusBufferInitialized              = (1 << 5),   //
    RKRadarStateConfigBufferAllocating               = (1 << 6),   //
    RKRadarStateConfigBufferInitialized              = (1 << 7),   //
    RKRadarStateHealthBufferAllocating               = (1 << 8),   //
    RKRadarStateHealthBufferInitialized              = (1 << 9),   //
    RKRadarStateHealthNodesAllocating                = (1 << 10),  //
    RKRadarStateHealthNodesInitialized               = (1 << 11),  //
    RKRadarStatePositionBufferAllocating             = (1 << 12),  //
    RKRadarStatePositionBufferInitialized            = (1 << 13),  //
    RKRadarStateControlsInitialized                  = (1 << 14),  //
    RKRadarStateWaveformCalibrationsInitialized      = (1 << 15),  //
    RKRadarStatePulseCompressionEngineInitialized    = (1 << 16),  // Engines
    RKRadarStatePulseRingFilterEngineInitialized     = (1 << 17),  //
    RKRadarStatePositionEngineInitialized            = (1 << 18),  //
    RKRadarStateHealthEngineInitialized              = (1 << 19),  //
    RKRadarStateMomentEngineInitialized              = (1 << 20),  //
    RKRadarStateSweepEngineInitialized               = (1 << 21),  //
    RKRadarStateFileRecorderInitialized              = (1 << 22),  //
    RKRadarStateHealthLoggerInitialized              = (1 << 23),  // Recorders
    RKRadarStateFileManagerInitialized               = (1 << 24),
    RKRadarStateHealthRelayInitialized               = (1 << 25),
    RKRadarStateTransceiverInitialized               = (1 << 26),
    RKRadarStatePedestalInitialized                  = (1 << 27),
    RKRadarStateHostMonitorInitialized               = (1 << 28),
    RKRadarStateRadarRelayInitialized                = (1 << 30),
    RKRadarStateLive                                 = (1 << 31)
};

typedef struct rk_radar RKRadar;
struct rk_radar {
    //
    // General attributes
    //
    RKName                           name;                           // Name of the engine
    RKRadarDesc                      desc;
    RKRadarState                     state;
    bool                             active;
    size_t                           memoryUsage;
    uint8_t                          processorCount;
    //
    // Buffers
    //
    RKStatus                         *status;                        // Overall RadarKit engine status
    RKConfig                         *configs;
    RKHealth                         *healths;
    RKPosition                       *positions;
    RKBuffer                         pulses;
    RKBuffer                         rays;
    //
    // Anchor indices of the buffers
    //
    uint32_t                         statusIndex;
    uint32_t                         configIndex;
    uint32_t                         healthIndex;
    uint32_t                         positionIndex;
    uint32_t                         pulseIndex;
    uint32_t                         rayIndex;
    //
    // Secondary Health Buffer
    //
    RKHealthNode                     healthNodeCount;
    RKNodalHealth                    *healthNodes;
    //
    //
    // Internal engines
    //
    RKClock                          *pulseClock;
    RKClock                          *positionClock;
    RKHealthEngine                   *healthEngine;
    RKPositionEngine                 *positionEngine;
    RKPulseCompressionEngine         *pulseCompressionEngine;
    RKPulseRingFilterEngine          *pulseRingFilterEngine;
    RKMomentEngine                   *momentEngine;
    RKHealthLogger                   *healthLogger;
    RKSweepEngine                    *sweepEngine;
    RKDataRecorder                   *dataRecorder;
    RKFileManager                    *fileManager;
    RKRadarRelay                     *radarRelay;
    RKHostMonitor                    *hostMonitor;
    //
    // System Inspector
    //
    RKSimpleEngine                   *systemInspector;
    //
    // Internal copies of things
    //
    RKWaveform                       *waveform;
    //
    pthread_mutex_t                  mutex;
    //
    // Hardware protocols for controls
    //
    RKTransceiver                    transceiver;
    RKTransceiver                    (*transceiverInit)(RKRadar *, void *);
    int                              (*transceiverExec)(RKTransceiver, const char *, char *);
    int                              (*transceiverFree)(RKTransceiver);
    void                             *transceiverInitInput;
    char                             transceiverResponse[RKMaximumStringLength];
    //
    RKPedestal                       pedestal;
    RKPedestal                       (*pedestalInit)(RKRadar *, void *);
    int                              (*pedestalExec)(RKPedestal, const char *, char *);
    int                              (*pedestalFree)(RKPedestal);
    void                             *pedestalInitInput;
    char                             pedestalResponse[RKMaximumStringLength];
    //
    RKHealthRelay                    healthRelay;
    RKHealthRelay                    (*healthRelayInit)(RKRadar *, void *);
    int                              (*healthRelayExec)(RKHealthRelay, const char *, char *);
    int                              (*healthRelayFree)(RKHealthRelay);
    void                             *healthRelayInitInput;
    char                             healthRelayResponse[RKMaximumStringLength];
    //
    RKMasterController               masterController;
    int                              (*masterControllerExec)(RKMasterController, const char *, char *);
    //
    // Waveform calibrations
    //
    RKWaveformCalibration            *waveformCalibrations;
    uint32_t                         waveformCalibrationCount;
    //
    // Controls
    //
    RKControl                        *controls;
    uint32_t                         controlCount;
};

//
// Life Cycle
//

RKRadar *RKInitWithDesc(RKRadarDesc);
RKRadar *RKInitQuiet(void);
RKRadar *RKInitLean(void);
RKRadar *RKInitMean(void);
RKRadar *RKInitFull(void);
RKRadar *RKInit(void);
RKRadar *RKInitAsRelay(void);
int RKFree(RKRadar *radar);

//
// Properties
//

// Set the transceiver. Pass in function pointers: init, exec and free
int RKSetTransceiver(RKRadar *,
                     void *initInput,
                     RKTransceiver initRoutine(RKRadar *, void *),
                     int execRoutine(RKTransceiver, const char *, char *),
                     int freeRoutine(RKTransceiver));

// Set the pedestal. Pass in function pointers: init, exec and free
int RKSetPedestal(RKRadar *,
                  void *initInput,
                  RKPedestal initRoutine(RKRadar *, void *),
                  int execRoutine(RKPedestal, const char *, char *),
                  int freeRoutine(RKPedestal));

// Set the health relay. Pass in function pointers: init, exec and free
int RKSetHealthRelay(RKRadar *,
                     void *initInput,
                     RKHealthRelay initRoutine(RKRadar *, void *),
                     int execRoutine(RKHealthRelay, const char *, char *),
                     int freeRoutine(RKHealthRelay));

int RKSetMomentProcessorToMultiLag(RKRadar *, const uint8_t);
int RKSetMomentProcessorToPulsePair(RKRadar *);
int RKSetMomentProcessorToPulsePairHop(RKRadar *);

#pragma mark -

// Some states of the radar
int RKSetVerbosity(RKRadar *, const int);
int RKSetVerbosityUsingArray(RKRadar *, const uint8_t *);
int RKSetDataPath(RKRadar *, const char *);
int RKSetDataUsageLimit(RKRadar *, const size_t limit);
int RKSetDoNotWrite(RKRadar *, const bool doNotWrite);
int RKSetDataRecorder(RKRadar *, const bool record);
int RKToggleDataRecorder(RKRadar *);

// Some operating parameters
int RKSetWaveform(RKRadar *, RKWaveform *);
int RKSetWaveformByFilename(RKRadar *, const char *);
int RKSetWaveformToImpulse(RKRadar *);
int RKSetWaveformTo121(RKRadar *);
int RKSetProcessingCoreCounts(RKRadar *,
                              const unsigned int pulseCompressionCoreCount,
                              const unsigned int momentProcessorCoreCount);
int RKSetPRF(RKRadar *, const uint32_t prf);
uint32_t RKGetPulseCapacity(RKRadar *);

// If there is a tic count from firmware, use it as clean reference for time derivation
void RKSetPulseTicsPerSeconds(RKRadar *, const double);
void RKSetPositionTicsPerSeconds(RKRadar *, const double);

//
// Interactions
//

// State
int RKGoLive(RKRadar *);
int RKWaitWhileActive(RKRadar *);
int RKStart(RKRadar *);
int RKStop(RKRadar *);
int RKSoftRestart(RKRadar *);
int RKResetClocks(RKRadar *);
int RKExecuteCommand(RKRadar *, const char *, char *);
void RKPerformMasterTaskInBackground(RKRadar *, const char *);

// Tasks
void RKMeasureNoise(RKRadar *);
void RKSetSNRThreshold(RKRadar *, const RKFloat);

// Status
RKStatus *RKGetVacantStatus(RKRadar *);
void RKSetStatusReady(RKRadar *, RKStatus *);

// Configs
void RKAddConfig(RKRadar *radar, ...);
RKConfig *RKGetLatestConfig(RKRadar *radar);

// Healths
RKHealthNode RKRequestHealthNode(RKRadar *);
RKHealth *RKGetVacantHealth(RKRadar *, const RKHealthNode);
void RKSetHealthReady(RKRadar *, RKHealth *);
RKHealth *RKGetLatestHealth(RKRadar *);
RKHealth *RKGetLatestHealthOfNode(RKRadar *, const RKHealthNode);
int RKGetEnumFromLatestHealth(RKRadar *, const char *);

// Positions
RKPosition *RKGetVacantPosition(RKRadar *);
void RKSetPositionReady(RKRadar *, RKPosition *);
RKPosition *RKGetLatestPosition(RKRadar *);
float RKGetPositionUpdateRate(RKRadar *);

// Pulses
RKPulse *RKGetVacantPulse(RKRadar *);
void RKSetPulseHasData(RKRadar *, RKPulse *);
void RKSetPulseReady(RKRadar *, RKPulse *);
RKPulse *RKGetLatestPulse(RKRadar *);

// Rays
RKRay *RKGetVacantRay(RKRadar *);
void RKSetRayReady(RKRadar *, RKRay *);

// Waveform Calibrations
void RKAddWaveformCalibration(RKRadar *, const RKWaveformCalibration *);
void RKUpdateWaveformCalibration(RKRadar *, const uint8_t, const RKWaveformCalibration *);
void RKClearWaveformCalibrations(RKRadar *);
void RKConcludeWaveformCalibrations(RKRadar *);

// Controls
void RKAddControl(RKRadar *, const RKControl *);
void RKAddControlAsLabelAndCommand(RKRadar *radar, const char *label, const char *command);
void RKUpdateControl(RKRadar *, const uint8_t, const RKControl *);
void RKClearControls(RKRadar *);
void RKConcludeControls(RKRadar *);

// Absolute address value query
void RKGetRegisterValue(RKRadar *, void *value, const unsigned long registerOffset, size_t size);
void RKSetRegisterValue(RKRadar *, void *value, const unsigned long registerOffset, size_t size);
void RKShowOffsets(RKRadar *, char *);
int RKBufferOverview(RKRadar *, char *, const RKOverviewFlag);

#endif /* defined(__RadarKit_RKRadar__) */
