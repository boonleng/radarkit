//
//  RKRadar.h
//  RadarKit
//
//  Created by Boonleng Cheong on 3/17/15.
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
#include <RadarKit/RKCalibrator.h>
#include <RadarKit/RKPositionEngine.h>
#include <RadarKit/RKPulseEngine.h>
#include <RadarKit/RKPulseRingFilter.h>
#include <RadarKit/RKMomentEngine.h>
#include <RadarKit/RKHealthLogger.h>
#include <RadarKit/RKRawDataRecorder.h>
#include <RadarKit/RKSweepEngine.h>
#include <RadarKit/RKProduct.h>
#include <RadarKit/RKWaveform.h>
#include <RadarKit/RKPreference.h>
#include <RadarKit/RKFileManager.h>
#include <RadarKit/RKRadarRelay.h>
#include <RadarKit/RKHostMonitor.h>

#define xstr(s) str(s)
#define str(s) #s
#define RADAR_VARIABLE_OFFSET(STRING, NAME) \
sprintf(STRING, "                    radar->" xstr(NAME) " @ %ld -> %p\n", (unsigned long)((void *)&radar->NAME - (void *)radar), (unsigned int *)&radar->NAME)

typedef uint32_t RKRadarState;
enum RKRadarState {
    RKRadarStateRayBufferAllocated                   = (1 << 0),   // Data buffers
    RKRadarStateRawIQBufferAllocated                 = (1 << 1),   //
    RKRadarStateStatusBufferAllocated                = (1 << 2),   //
    RKRadarStateConfigBufferAllocated                = (1 << 3),   //
    RKRadarStateHealthBufferAllocated                = (1 << 4),   //
    RKRadarStateHealthNodesAllocated                 = (1 << 5),   //
    RKRadarStatePositionBufferAllocated              = (1 << 6),   //
    RKRadarStateWaveformCalibrationsAllocated        = (1 << 7),   //
    RKRadarStateControlsAllocated                    = (1 << 8),   //
    RKRadarStateProductBufferAllocated               = (1 << 9),   //
    RKRadarStateRserverd1                            = (1 << 10),  //
    RKRadarStateRserverd2                            = (1 << 11),  //
    RKRadarStateRserverd3                            = (1 << 12),  //
    RKRadarStateRserverd4                            = (1 << 13),  //
    RKRadarStateRserverd5                            = (1 << 14),  //
    RKRadarStateRserverd6                            = (1 << 15),  //
    RKRadarStatePulseCompressionEngineInitialized    = (1 << 16),  // Engines
    RKRadarStatePulseRingFilterEngineInitialized     = (1 << 17),  //
    RKRadarStatePositionEngineInitialized            = (1 << 18),  //
    RKRadarStateHealthEngineInitialized              = (1 << 19),  //
    RKRadarStateMomentEngineInitialized              = (1 << 20),  //
    RKRadarStateSweepEngineInitialized               = (1 << 21),  //
    RKRadarStateFileRecorderInitialized              = (1 << 22),  //
    RKRadarStateHealthLoggerInitialized              = (1 << 23),  //
    RKRadarStateFileManagerInitialized               = (1 << 24),  //
    RKRadarStateHealthRelayInitialized               = (1 << 25),  //
    RKRadarStateTransceiverInitialized               = (1 << 26),  //
    RKRadarStatePedestalInitialized                  = (1 << 27),  //
    RKRadarStateHostMonitorInitialized               = (1 << 28),  //
    RKRadarStateReserved7                            = (1 << 29),  //
    RKRadarStateRadarRelayInitialized                = (1 << 30),  //
    RKRadarStateLive                                 = (1 << 31)   //
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
    uint64_t                         tic;
    pthread_mutex_t                  mutex;
    //
    // Buffers
    //
    RKStatus                         *status;                        // Overall RadarKit engine status
    RKConfig                         *configs;
    RKHealth                         *healths;
    RKPosition                       *positions;
    RKBuffer                         pulses;
    RKBuffer                         rays;
    RKProduct                        *products;
    //
    // Anchor indices of the buffers
    //
    uint32_t                         statusIndex;
    uint32_t                         configIndex;
    uint32_t                         healthIndex;
    uint32_t                         positionIndex;
    uint32_t                         pulseIndex;
    uint32_t                         rayIndex;
    uint32_t                         productIndex;
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
    RKFFTModule                      *fftModule;
    RKHealthEngine                   *healthEngine;
    RKPositionEngine                 *positionEngine;
    RKPulseEngine                    *pulseEngine;
    RKPulseRingFilterEngine          *pulseRingFilterEngine;
    RKMomentEngine                   *momentEngine;
    RKRawDataRecorder                *rawDataRecorder;
    RKHealthLogger                   *healthLogger;
    RKSweepEngine                    *sweepEngine;
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
    RKWaveform                       *waveformDecimate;
    RKIIRFilter                      *filter;
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
// Hardware Hooks
//
#pragma mark - Hardware Hooks

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

//
// Properties
//
#pragma mark - Before-Live Properties

// These can only be set before the radar goes live
int RKSetProcessingCoreCounts(RKRadar *, const uint8_t, const uint8_t, const uint8_t);

#pragma mark - Properties

// Some states of the radar
int RKSetVerbosity(RKRadar *, const int);
int RKSetVerbosityUsingArray(RKRadar *, const uint8_t *);
int RKSetDataPath(RKRadar *, const char *);
int RKSetDataUsageLimit(RKRadar *, const size_t limit);
int RKSetRecordingLevel(RKRadar *, const int);

// Some operating parameters
int RKSetWaveform(RKRadar *, RKWaveform *);
int RKSetWaveformByFilename(RKRadar *, const char *);
int RKSetWaveformToImpulse(RKRadar *);
int RKSetPRF(RKRadar *, const uint32_t);
uint32_t RKGetPulseCapacity(RKRadar *);

// If there is a tic count from firmware, use it as clean reference for time derivation
void RKSetPulseTicsPerSeconds(RKRadar *, const double);
void RKSetPositionTicsPerSeconds(RKRadar *, const double);

// Pulse compressor
int RKSetPulseCompressor(RKRadar *radar, void (*compressor)(RKCompressionScratch *));

// Moment calibrator
int RKSetMomentCalibrator(RKRadar *radar, void (*calibrator)(RKScratch *, RKConfig *));

// Moment processor
int RKSetMomentProcessorToMultiLag(RKRadar *, const uint8_t);
int RKSetMomentProcessorToPulsePair(RKRadar *);
int RKSetMomentProcessorToPulsePairHop(RKRadar *);
int RKSetMomentProcessorToPulsePairStaggeredPRT(RKRadar *);
int RKSetMomentProcessorToSpectralMoment(RKRadar *);

// Moment recorder (RadarKit uses netcdf by default)
int RKSetProductRecorder(RKRadar *radar, int (*productRecorder)(RKProduct *, const char *));

// Pulse ring filter (FIR / IIR ground clutter filter)
int RKSetPulseRingFilterByType(RKRadar *, RKFilterType, const uint32_t);
int RKSetPulseRingFilter(RKRadar *, RKIIRFilter *, const uint32_t);

//
// Interactions
//
#pragma mark - Interaction / State Change

// State
int RKGoLive(RKRadar *);                                                                           // Go live
int RKWaitWhileActive(RKRadar *);                                                                  // Wait
int RKStart(RKRadar *);                                                                            // Start the radar (RKGoLive and RKWaitWhileActive)
int RKStop(RKRadar *);                                                                             // Stop the radar
int RKSoftRestart(RKRadar *);                                                                      // Restart the DSP related engines (pulse compression, moment calculation, sweep gathering, etc.)
int RKResetClocks(RKRadar *);                                                                      // Reset the internal clock tracking mechanism
int RKExecuteCommand(RKRadar *, const char *, char * _Nullable);                                   // Execute a command and wait for feedback (blocking)
void RKPerformMasterTaskInBackground(RKRadar *, const char *);                                     // Send a command to the master controller in the background (non-blocking)

// General
void RKMeasureNoise(RKRadar *);                                                                    // Ask RadarKit to measure noise from the latest pulses
void RKSetSNRThreshold(RKRadar *, const RKFloat);                                                  // Set the censoring SNR threshold

// Status
RKStatus *RKGetVacantStatus(RKRadar *);                                                            // Don't worry about this. This is managed by systemInspector
void RKSetStatusReady(RKRadar *, RKStatus *);                                                      // Don't worry about this. This is managed by systemInspector

// Configs
void RKAddConfig(RKRadar *radar, ...);                                                             // Inform RadarKit about certain slow-changing parameters, e.g., PRF, waveform, etc.
RKConfig *RKGetLatestConfig(RKRadar *radar);                                                       // Get the latest configuration from the radar

// Healths
RKHealthNode RKRequestHealthNode(RKRadar *);
RKHealth *RKGetVacantHealth(RKRadar *, const RKHealthNode);                                        // Get a vacant slot for storing position data
void RKSetHealthReady(RKRadar *, RKHealth *);                                                      // Declare the health is ready
RKHealth *RKGetLatestHealth(RKRadar *);                                                            // Get the latest consolidated health from the radar
RKHealth *RKGetLatestHealthOfNode(RKRadar *, const RKHealthNode);                                  // Get the latest health of a node from the radar
RKStatusEnum RKGetEnumFromLatestHealth(RKRadar *, const char *);                                   // Get the RKStatusEnum of a specific keyword from the latest consolidated health

// Positions
RKPosition *RKGetVacantPosition(RKRadar *);                                                        // Get a vacant slot for storing position data
void RKSetPositionReady(RKRadar *, RKPosition *);                                                  // Declare the position is ready
RKPosition *RKGetLatestPosition(RKRadar *);                                                        // Get the latest position from the radar
float RKGetPositionUpdateRate(RKRadar *);                                                          // Get the position report rate

// Pulses
RKPulse *RKGetVacantPulse(RKRadar *);                                                              // Get a vacant slot for storing pulse data
void RKSetPulseHasData(RKRadar *, RKPulse *);                                                      // Declare the pulse has 16-bit I/Q data. Let RadarKit tag the position
void RKSetPulseReady(RKRadar *, RKPulse *);                                                        // Declare the pulse has 16-bit I/Q data and position, the pulse is ready for moment processing
RKPulse *RKGetLatestPulse(RKRadar *);                                                              // Get the latest pulse from the radar

// Rays
RKRay *RKGetVacantRay(RKRadar *);                                                                  // Get a vacant slot for storing ray data
void RKSetRayReady(RKRadar *, RKRay *);                                                            // Declare the ray is ready
RKRay *RKGetLatestRay(RKRadar *);                                                                  // Get the latest ray from the radar
RKRay *RKGetLatestRayIndex(RKRadar *, uint32_t *);                                                 // Get the latest ray index from the radar

// Waveform calibrations
void RKAddWaveformCalibration(RKRadar *, const RKWaveformCalibration *);                           // Add a waveform specific calibration
void RKClearWaveformCalibrations(RKRadar *);                                                       // Clear all waveform calibrations
void RKConcludeWaveformCalibrations(RKRadar *);                                                    // Declare waveform calibration setup complete

// Controls
void RKAddControl(RKRadar *, const RKControl *);                                                   // Add control through an RKControl struct
void RKAddControlAsLabelAndCommand(RKRadar *, const char *label, const char *command);             // Add control through specifying a label and command string
void RKClearControls(RKRadar *);                                                                   // Clear all controls
void RKConcludeControls(RKRadar *);                                                                // Declare control setup complete

#pragma mark - Developer Access

// Absolute address value query
void RKGetRegisterValue(RKRadar *, void *value, const unsigned long offset, size_t size);           // Does not work like the way I expected just yet
void RKSetRegisterValue(RKRadar *, void *value, const unsigned long offset, size_t size);           // Does not work like the way I expected just yet
void RKShowOffsets(RKRadar *, char *);                                                              // Does not work like the way I expected just yet

// ASCII Art
int RKBufferOverview(char *, RKRadar *, const RKTextPreferences);                                   // Do you ASCII? :)
int RKHealthOverview(char *, const char *, const RKTextPreferences);

#endif /* defined(__RadarKit_RKRadar__) */
