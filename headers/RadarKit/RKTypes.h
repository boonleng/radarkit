//
//  RKTypes.h
//  RadarKit
//
//  Created by Boonleng Cheong on 3/17/15.
//

#ifndef __RadarKit_Types__
#define __RadarKit_Types__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <fftw3.h>

#include <RadarKit/RKVersion.h>

//
// Memory Blocks
// Defines the number of slots and gates of each pulse of the RKRadar structure
//
// RKBufferSSlotCount The number of slots for status
// RKBufferCSlotCount The number of slots for config
// RKBufferHSlotCount The number of slots for health JSON string
// RKBufferPSlotCount The number of slots for position buffer
// RKBuffer0SlotCount The number of slots for level-0 pulse storage in the host memory
// RKBuffer1SlotCount The number of slots for level-1 pulse storage in the host memory
// RKBuffer2SlotCount The number of slots for level-2 ray storage in the host memory
// RKBuffer3SlotCount The number of slots for level-3 product storage in the host memory
// RKMaximumControlCount The number of controls (buttons)
// RKMaximumCalibrationCount The number of waveform calibration set
// RKMaximumGateCount The maximum number of gates allocated for each pulse
// RKSIMDAlignSize The minimum alignment size. AVX requires 256 bits = 32 bytes. AVX-512 is on the horizon now.
//

#pragma mark - Constants

#define RKRawDataBuildNo                     6                                 //
#define RKBufferSSlotCount                   10                                // Status
#define RKBufferCSlotCount                   10                                // Config
#define RKBufferHSlotCount                   50                                // Health
#define RKBufferPSlotCount                   1000                              // Positions
#define RKBuffer0SlotCount                   20000                             // Pulse - Level I- - Raw I/Q
#define RKBuffer2SlotCount                   3600                              // Ray - Level II - Moment data
#define RKBuffer3SlotCount                   100                               // Products - Level III - Ready for archive
#define RKMaximumControlCount                128                               // Controls
#define RKMaximumWaveformCalibrationCount    128                               // Waveform calibration
#define RKMaximumGateCount                   262144                            // Must be a multiple of RKSIMDAlignSize
#define RKSIMDAlignSize                      64                                // SSE 16, AVX 32, AVX-512 64

#define RKBaseMomentCount                    10                                // 16 to be the absolute max since productList enum is 32-bit (product + display)
#define RKMaximumLagCount                    5                                 // Number lags of ACF / CCF lag = +/-4 and 0. This should not be changed
#define RKMaximumFilterCount                 8                                 // Maximum filter count within each group. Check RKPulseParameters
#define RKMaximumWaveformCount               22                                // Maximum waveform group count
#define RKWorkerDutyCycleBufferDepth         1000                              //
#define RKMaximumPulsesPerRay                2000                              //
#define RKMaximumRaysPerSweep                1500                              // 1440 is 0.25-deg. This should be plenty
#define RKMaximumPacketSize                  16 * 1024 * 1024                  // Maximum network packet size
#define RKNetworkTimeoutSeconds              20                                //
#define RKNetworkReconnectSeconds            3                                 //
#define RKLagRedThreshold                    0.5                               //
#define RKLagOrangeThreshold                 0.7                               //
#define RKDutyCyleRedThreshold               0.9                               //
#define RKDutyCyleOrangeThreshold            0.8                               //
#define RKStatusBarWidth                     6                                 //
#define RKPulseCountForNoiseMeasurement      200                               //
#define RKProcessorStatusPulseCoreCount      16                                //
#define RKProcessorStatusRingCoreCount       16                                //
#define RKProcessorStatusRayCoreCount        16                                //
#define RKHostMonitorPingInterval            5                                 //
#define RKMaximumProductCount                64                                //
#define RKMaximumIIRFilterTaps               8                                 //
#define RKMaximumPrefixLength                8                                 // String length includes the terminating character!
#define RKMaximumSymbolLength                8                                 // String length includes the terminating character!
#define RKMaximumFileExtensionLength         8                                 // String length includes the terminating character!
#define RKUserParameterCount                 8                                 //

#define RKDefaultDataPath                    "data"
#define RKDataFolderIQ                       "iq"
#define RKDataFolderMoment                   "moment"
#define RKDataFolderHealth                   "health"
#define RKLogFolder                          "log"
#define RKWaveformFolder                     "waveforms"
#define RKFFTWisdomFile                      "radarkit-fft-wisdom"

#define RKNoColor                            "\033[m"
#define RKNoForegroundColor                  "\033[39m"
#define RKNoBackgroundColor                  "\033[49m"
#define RKBaseRedColor                       "\033[91m"
#define RKBaseGreenColor                     "\033[92m"
#define RKBaseYellowColor                    "\033[93m"
#define RKRedColor                           "\033[38;5;196m"
#define RKOrangeColor                        "\033[38;5;208m"
#define RKYellowColor                        "\033[38;5;226m"
#define RKLimeColor                          "\033[38;5;118m"
#define RKGreenColor                         "\033[38;5;46m"
#define RKTealColor                          "\033[38;5;49m"
#define RKIceBlueColor                       "\033[38;5;51m"
#define RKSkyBlueColor                       "\033[38;5;45m"
#define RKBlueColor                          "\033[38;5;27m"
#define RKPurpleColor                        "\033[38;5;99m"
#define RKIndigoColor                        "\033[38;5;201m"
#define RKHotPinkColor                       "\033[38;5;199m"
#define RKDeepPinkColor                      "\033[38;5;197m"
#define RKPinkColor                          "\033[38;5;213m"
#define RKSalmonColor                        "\033[38;5;210m"
#define RKGrayColor                          "\033[38;5;245m"
#define RKPythonColor                        "\033[38;5;226;48;5;24m"
#define RKRadarKitColor                      "\033[38;5;15;48;5;124m"
#define RKMaximumStringLength                4096
#define RKMaximumPathLength                  1024
#define RKMaximumFolderPathLength            768
#define RKMaximumCommandLength               512
#define RKNameLength                         128
#define RKStatusStringLength                 256
#define RKPulseHeaderPaddedSize              256                               // Change this to higher number for post-AVX2 intrinsics
#define RKRayHeaderPaddedSize                128                               // Change this to higher number for post-AVX2 intrinsics
#define RKShortNameLength                    20                                // Short names, e.g., C1, M2, P0, etc. (including color)

#define RKColorDutyCycle(x)  (x > RKDutyCyleRedThreshold ? RKBaseRedColor : (x > RKDutyCyleOrangeThreshold ? RKBaseYellowColor : RKBaseGreenColor))
#define RKColorLag(x)        (x > RKLagRedThreshold      ? RKBaseRedColor : (x > RKLagOrangeThreshold      ? RKBaseYellowColor : RKBaseGreenColor))

#define ITALIC(x)            "\033[3m" x "\033[23m"
#define UNDERLINE(x)         "\033[4m" x "\033[24m"
#define HIGHLIGHT(x)         "\033[38;5;82;48;5;238m" x "\033[0m"
#define UNDERLINE_ITALIC(x)  "\033[3;4m" x "\033[23;24m"

#define CLAMP(x, lo, hi)     MIN(MAX((x), (lo)), (hi))

#define RKFilterAnchorDefault                           {{.length = 1, .maxDataLength = RKMaximumGateCount, .filterGain = 1.0f}}
#define RKFilterAnchorDefaultWithMaxDataLength(x)       {{.length = 1, .maxDataLength = (x), .filterGain = 1.0f}}
#define RKFilterAnchorOfLengthAndMaxDataLength(x, y)    {{.length = (x), .maxDataLength = (y), .filterGain = 1.0f}}

#define RKMarkerScanTypeString(x) \
(((x) & RKMarkerScanTypeMask) == RKMarkerScanTypePPI ? "PPI" : \
(((x) & RKMarkerScanTypeMask) == RKMarkerScanTypeRHI ? "RHI" : \
(((x) & RKMarkerScanTypeMask) == RKMarkerScanTytpePoint ? "SPT" : "UNK")))

#define RKMarkerScanTypeShortString(x) \
(((x) & RKMarkerScanTypeMask) == RKMarkerScanTypePPI ? "P" : \
(((x) & RKMarkerScanTypeMask) == RKMarkerScanTypeRHI ? "R" : \
(((x) & RKMarkerScanTypeMask) == RKMarkerScanTytpePoint ? "S" : "U")))

#pragma mark - Fundamental Types

typedef uint8_t       RKByte;                                                  //
typedef float         RKFloat;                                                 // We can change this to double if we decided one day
typedef ssize_t       RKResult;                                                // Generic return from functions, 0 for no errors and !0 for others.
typedef void *        RKBuffer;                                                //
typedef void *        RKTransceiver;                                           //
typedef void *        RKPedestal;                                              //
typedef void *        RKHealthRelay;                                           //
typedef void *        RKMasterController;                                      //
typedef char          RKName[RKNameLength];                                    // RKName x = char x[RKNameLength]
typedef char          RKShortName[RKShortNameLength];                          // RKShortname x = char x[RKShortNameLength]
typedef char          RKCommand[RKMaximumCommandLength];                       // RKCommand x = char x[RKCommandLength]
typedef uint8_t       RKProductId;                                             // Product identifier
typedef uint64_t      RKIdentifier;                                            // Pulse identifier, ray identifier, config identifier, etc.
typedef const float   RKConst;

#if !defined(_Nullable)
#define _Nullable
#endif

#pragma pack(push, 1)

//
// Fundamental unit of a (16-bit) + (16-bit) raw complex IQ sample
//
typedef struct rk_int16c {
    int16_t i;
    int16_t q;
} RKInt16C;

//
// Interleaved complex format. Fundamental unit of a (float) + (float) raw complex IQ sample
//
typedef struct rk_complex {
    RKFloat i;
    RKFloat q;
} RKComplex;

//
// Deinterleaved complex format for vector library
//
typedef struct rk_iqz {
    RKFloat *i;
    RKFloat *q;
} RKIQZ;

//
// A modulo path to describe origin, length and modulo for accessing a circular buffer
//
typedef struct rk_modulo_path {
    uint32_t      origin;
    uint32_t      length;
    uint32_t      modulo;
    uint32_t      planIndex;
} RKModuloPath;

//
// A convenient way to convert bytes into several other types
//
typedef union rk_four_byte {
    struct { RKByte byte[4]; };
    struct { uint8_t u8, u8_2, u8_3, u8_4; };
    struct { int8_t i8, i8_2, i8_3, i8_4; };
    struct { uint16_t u16, u16_2; };
    struct { int16_t i16, i16_2; };
    struct { uint32_t u32; };
    struct { int32_t i32; };
    struct { float f; };
} RKFourByte;

typedef union rk_filter_anchor {
    struct {
        uint32_t      name;                                                    // Just an internal name id
        uint32_t      origin;                                                  // Filter origin to be used with RKWaveform
        uint32_t      length;                                                  // Filter length to be used with RKWaveform
        uint32_t      inputOrigin;                                             // Origin of input
        uint32_t      outputOrigin;                                            // Origin of output
        uint32_t      maxDataLength;                                           // Maximum data to decode
        float         subCarrierFrequency;                                     // For house keeping only, use the waveform->fc for DDC
        float         sensitivityGain;                                         // Sensitivity gain due to longer/efficient waveforms (dB)
        float         filterGain;                                              // Filter gain from the filter coefficients, should be 0.0 (dB)
        float         fullScale;                                               // Scaling factor to get to full scale
        float         lowerBoundFrequency;                                     // For house Keeping only
        float         upperBoundFrequency;                                     // For house Keeping only
    };
    char bytes[64];
} RKFilterAnchor;

typedef RKFilterAnchor RKFilterAnchorGroup[RKMaximumFilterCount];

#pragma mark - Enums

#define RKResultNames \
N(RKResultSuccess) \
N(RKResultTooBig) \
N(RKResultTimeout) \
N(RKResultNullInput) \
N(RKResultEngineNotWired) \
N(RKResultEngineNotActive) \
N(RKResultIncompleteSend) \
N(RKResultIncompleteReceive) \
N(RKResultIncompleteTransceiver) \
N(RKResultIncompletePedestal) \
N(RKResultIncompleteHealthRelay) \
N(RKResultIncompleteControl) \
N(RKResultIncompleteWaveformCalibration) \
N(RKResultIncompleteProductDescription) \
N(RKResultErrorCreatingOperatorRoutine) \
N(RKResultErrorCreatingOperatorCommandRoutine) \
N(RKResultErrorCreatingClientRoutine) \
N(RKResultSDToFDError) \
N(RKResultNoPulseBuffer) \
N(RKResultNoRayBuffer) \
N(RKResultNoPulseCompressionEngine) \
N(RKResultNoPulseRingEngine) \
N(RKResultNoMomentEngine) \
N(RKResultFailedToStartCompressionCore) \
N(RKResultFailedToStartRingCore) \
N(RKResultFailedToStartPulseWatcher) \
N(RKResultFailedToStartRingPulseWatcher) \
N(RKResultFailedToInitiateSemaphore) \
N(RKResultFailedToRetrieveSemaphore) \
N(RKResultFailedToAllocateFFTSpace) \
N(RKResultFailedToAllocateFilter) \
N(RKResultFailedToAllocateDutyCycleBuffer) \
N(RKResultFailedToAllocateScratchSpace) \
N(RKResultFailedToSetWaveform) \
N(RKResultFailedToSetFilter) \
N(RKResultEngineDeactivatedMultipleTimes) \
N(RKResultFailedToStartMomentCore) \
N(RKResultFailedToStartPulseGatherer) \
N(RKResultUnableToChangeCoreCounts) \
N(RKResultFailedToStartPedestalWorker) \
N(RKResultFailedToGetVacantPosition) \
N(RKResultFailedToGetVacantHealth) \
N(RKResultFailedToStartRayGatherer) \
N(RKResultFailedToStartHealthWorker) \
N(RKResultFailedToStartPulseRecorder) \
N(RKResultFailedToStartPedestalMonitor) \
N(RKResultFailedToStartFileManager) \
N(RKResultFailedToStartFileRemover) \
N(RKResultFailedToStartTransceiver) \
N(RKResultFailedToStartPedestal) \
N(RKResultFailedToStartHealthRelay) \
N(RKResultPreferenceFileNotFound) \
N(RKResultPreferenceKeywordNotFound) \
N(RKResultFailedToMeasureNoise) \
N(RKResultFailedToCreateFileRemover) \
N(RKResultFileManagerBufferNotResuable) \
N(RKResultInvalidMomentParameters) \
N(RKResultFailedToCreateUnitWorker) \
N(RKResultFailedToStartHostWatcher) \
N(RKResultFailedToStartHostPinger) \
N(RKResultFailedToExecuteCommand) \
N(RKResultFailedToAddHost) \
N(RKResultFailedToFindProductId) \
N(RKResultFailedToOpenFileForProduct) \
N(RKResultClientNotConnected) \
N(RKResultFileManagerInconsistentFolder) \
N(RKResultFailedToExpandWaveform) \
N(RKResultFailedToOpenFileForWriting) \
N(RKResultRadarNotLive) \
N(RKResultRawDataTypeUndefined) \
N(RKResultNothingToRead) \
N(RKResultNoRadar)

#define N(x) x,
enum RKResult {
    RKResultNames
    RKResultCount,
};
#undef N

enum RKEngineColor {
    RKEngineColorCommandCenter = 10,
    RKEngineColorPulseCompressionEngine = 7,
    RKEngineColorPulseRingFilterEngine = 3,
    RKEngineColorPositionEngine = 4,
    RKEngineColorMomentEngine = 8,
    RKEngineColorHealthEngine = 1,
    RKEngineColorDataRecorder = 9,
    RKEngineColorSweepEngine = 14,
    RKEngineColorHealthLogger = 5,
    RKEngineColorFileManager = 2,
    RKEngineColorTransceiver = 13,
    RKEngineColorPedestalRelayPedzy = 11,
    RKEngineColorHealthRelayTweeta = 0,
    RKEngineColorRadarRelay = 13,
    RKEngineColorHostMonitor = 12,
    RKEngineColorClock = 15,
    RKEngineColorMisc = 16,
    RKEngineColorEngineMonitor = 15,
    RKEngineColorConfig = 6,
    RKEngineColorFFTModule = 15
};

typedef uint32_t RKValueType;
enum RKValueType {
    RKValueTypeBool,
    RKValueTypeInt,
    RKValueTypeLong,
    RKValueTypeInt8,
    RKValueTypeInt16,
    RKValueTypeInt32,
    RKValueTypeInt64,
    RKValueTypeSSize,
    RKValueTypeUInt,
    RKValueTypeULong,
    RKValueTypeUInt8,
    RKValueTypeUInt16,
    RKValueTypeUInt32,
    RKValueTypeUInt64,
    RKValueTypeIntInHex,
    RKValueTypeLongInHex,
    RKValueTypeInt8InHex,
    RKValueTypeInt16InHex,
    RKValueTypeInt32InHex,
    RKValueTypeInt64InHex,
    RKValueTypeSSizeInHex,
    RKValueTypeUIntInHex,
    RKValueTypeULongInHex,
    RKValueTypeUInt8InHex,
    RKValueTypeUInt16InHex,
    RKValueTypeUInt32InHex,
    RKValueTypeUInt64InHex,
    RKValueTypeSize,
    RKValueTypeFloat,
    RKValueTypeDouble,
    RKValueTypeString,
    RKValueTypeNumericString,
    RKValueTYpeFloatMultipliedBy1k,
    RKValueTYpeFloatMultipliedBy1M,
    RKValueTYpeFloatDividedBy1k,
    RKValueTYpeFloatDividedBy1M,
    RKValueTYpeDoubleMultipliedBy1k,
    RKValueTYpeDoubleMultipliedBy1M,
    RKValueTYpeDoubleDividedBy1k,
    RKValueTYpeDoubleDividedBy1M,
    RKValueTypeProductId = RKValueTypeInt8,
    RKValueTypeIdentifier = RKValueTypeUInt64
};

typedef uint32_t RKPositionFlagV1;
enum RKPositionFlagV1 {
    RKPositionFlagV1Vacant             = 0,
    RKPositionFlagV1AzimuthEnabled     = 1,            //  0 - EN
    RKPositionFlagV1AzimuthSafety      = (1 << 1),     //  1
    RKPositionFlagV1AzimuthError       = (1 << 2),     //  2
    RKPositionFlagV1AzimuthSweep       = (1 << 8),     //  8
    RKPositionFlagV1AzimuthPoint       = (1 << 9),     //  9
    RKPositionFlagV1AzimuthComplete    = (1 << 10),    // 10
    RKPositionFlagV1ElevationEnabled   = (1 << 16),    //  0 - EN
    RKPositionFlagV1ElevationSafety    = (1 << 17),    //  1
    RKPositionFlagV1ElevationError     = (1 << 18),    //  2
    RKPositionFlagV1ElevationSweep     = (1 << 24),    //  8
    RKPositionFlagV1ElevationPoint     = (1 << 25),    //  9
    RKPositionFlagV1ElevationComplete  = (1 << 26),    // 10
    RKPositionFlagV1ScanActive         = (1 << 28),
    RKPositionFlagV1VCPActive          = (1 << 29),
    RKPositionFlagV1HardwareMask       = 0x3FFFFFFF,
    RKPositionFlagV1Used               = (1 << 30),
    RKPositionFlagV1Ready              = (1 << 31),
    RKPositionFlagV1AzimuthModeMask    = (RKPositionFlagV1AzimuthSweep | RKPositionFlagV1AzimuthPoint),
    RKPositionFlagV1ElevationModeMask  = (RKPositionFlagV1ElevationSweep | RKPositionFlagV1ElevationPoint),
    RKPositionFlagV1ScanModeMask       = (RKPositionFlagV1AzimuthModeMask | RKPositionFlagV1ElevationModeMask)
};

typedef uint32_t RKPositionFlag;
enum RKPositionFlag {
    RKPositionFlagVacant             = 0,
    RKPositionFlagAzimuthEnabled     = 1,            //  0 - EN
    RKPositionFlagAzimuthSafety      = (1 << 1),     //  1
    RKPositionFlagAzimuthError       = (1 << 2),     //  2
    RKPositionFlagAzimuthSweep       = (1 << 8),     //  8
    RKPositionFlagAzimuthPoint       = (1 << 9),     //  9
    RKPositionFlagAzimuthComplete    = (1 << 10),    // 10
    RKPositionFlagElevationEnabled   = (1 << 16),    //  0 - EN
    RKPositionFlagElevationSafety    = (1 << 17),    //  1
    RKPositionFlagElevationError     = (1 << 18),    //  2
    RKPositionFlagElevationSweep     = (1 << 24),    //  8
    RKPositionFlagElevationPoint     = (1 << 25),    //  9
    RKPositionFlagElevationComplete  = (1 << 26),    // 10
    RKPositionFlagScanActive         = (1 << 28),
    RKPositionFlagVCPActive          = (1 << 29),
    RKPositionFlagHardwareMask       = 0x3FFFFFFF,
    RKPositionFlagUsed               = (1 << 30),
    RKPositionFlagReady              = (1 << 31),
    RKPositionFlagAzimuthModeMask    = (RKPositionFlagAzimuthSweep | RKPositionFlagAzimuthPoint),
    RKPositionFlagElevationModeMask  = (RKPositionFlagElevationSweep | RKPositionFlagElevationPoint),
    RKPositionFlagScanModeMask       = (RKPositionFlagAzimuthModeMask | RKPositionFlagElevationModeMask)
};

typedef uint32_t RKHeadingType;
enum RKHeadingType {
    RKHeadingTypeNormal,
    RKHeadingTypeAdd90,
    RKHeadingTypeAdd180,
    RKHeadingTypeAdd270
};

typedef uint32_t RKStatusFlag;
enum RKStatusFlag {
    RKStatusFlagVacant               = 0,
    RKStatusFlagReady                = 1
};

typedef uint32_t RKHealthFlag;
enum RKHealthFlag {
    RKHealthFlagVacant               = 0,
    RKHealthFlagReady                = 1,
    RKHealthFlagUsed                 = (1 << 1)
};

typedef uint32_t RKMarker;
enum RKMarker {
    RKMarkerNull                     = 0,
    RKMarkerSweepMiddle              = 1,                                      //
    RKMarkerSweepBegin               = (1 << 1),                               //  0000 0010
    RKMarkerSweepEnd                 = (1 << 2),                               //  0000 0100
    RKMarkerVolumeBegin              = (1 << 3),                               //  0000 1000
    RKMarkerVolumeEnd                = (1 << 4),                               //  0001 0000
    RKMarkerScanTypeMask             = 0x60,                                   //  0110 0000
    RKMarkerScanTypeUnknown          = (0 << 5),                               //  .00. ....
    RKMarkerScanTypePPI              = (1 << 5),                               //  .01. ....
    RKMarkerScanTypeRHI              = (2 << 5),                               //  .10. ....
    RKMarkerScanTytpePoint           = (3 << 5),                               //  .11. ....
    RKMarkerMemoryManagement         = (1 << 7)                                //  1000 0000
};

//
// Typical status progression:
//
// -> RKPulseStatusVacant
// -> RKPulseStatusHasIQData
// -> RKPulseStatusInspected                                (main thread)
// -> RKPulseStatusCompressed / RKPulseStatusSkipped        (core threads)
// -> RKPulseStatusDownSampled                              (core threads)
// -> RKPulseStatusProcessed                                (core threads)
// -> RKPulseStatusRingInspected                            (main thread)
// -> RKPulseStatusRingFiltered / RKPulseStatusRingSkipped  (main thread consolidates)
// -> RKPulseStatusRingProcessed                            (main thread)
// -> RKPulseStatusHasPosition
// -> RKPulseStatusReadyForMoments
//
typedef uint32_t RKPulseStatus;
enum RKPulseStatus {
    RKPulseStatusVacant              = 0,
    RKPulseStatusHasIQData           = 1,                                      // 0x01
    RKPulseStatusHasPosition         = (1 << 1),                               // 0x02
    RKPulseStatusInspected           = (1 << 2),                               // 0x04
    RKPulseStatusCompressed          = (1 << 3),                               // 0x08
    RKPulseStatusSkipped             = (1 << 4),                               // 0x10
    RKPulseStatusDownSampled         = (1 << 5),                               // 0x20
    RKPulseStatusProcessed           = (1 << 6),                               // 0x40
    RKPulseStatusRingInspected       = (1 << 7),                               // 0x80
    RKPulseStatusRingFiltered        = (1 << 8),
    RKPulseStatusRingSkipped         = (1 << 9),
    RKPulseStatusRingProcessed       = (1 << 10),
    RKPulseStatusReadyForMoments     = (RKPulseStatusProcessed | RKPulseStatusRingProcessed | RKPulseStatusHasPosition),
    RKPulseStatusUsedForMoments      = (1 << 11),
    RKPulseStatusRecorded            = (1 << 12)
};

typedef uint32_t RKRayStatus;
enum RKRayStatus {
    RKRayStatusVacant                = 0,
    RKRayStatusProcessing            = 1,
    RKRayStatusProcessed             = (1 << 1),
    RKRayStatusSkipped               = (1 << 2),
    RKRayStatusReady                 = (1 << 3),
    RKRayStatusStreamed              = (1 << 4),
    RKRayStatusBeingConsumed         = (1 << 5)
};

typedef uint32_t RKInitFlag;
enum RKInitFlag {
    RKInitFlagNone                      = 0,
    RKInitFlagVerbose                   = 0x00000001,
    RKInitFlagVeryVerbose               = 0x00000002,
    RKInitFlagVeryVeryVerbose           = 0x00000004,
    RKInitFlagShowClockOffset           = 0x00000008,
    RKInitFlagManuallyAssignCPU         = 0x00000010,
    RKInitFlagIgnoreGPS                 = 0x00000020,
    RKInitFlagIgnoreHeading             = 0x00000040,
    RKInitFlagReserved4                 = 0x00000080,
    RKInitFlagAllocStatusBuffer         = 0x00000100,                          // 1 << 8
    RKInitFlagAllocConfigBuffer         = 0x00000200,                          // 1 << 9
    RKInitFlagAllocRawIQBuffer          = 0x00000400,                          // 1 << 10
    RKInitFlagAllocPositionBuffer       = 0x00000800,                          // 1 << 11
    RKInitFlagAllocMomentBuffer         = 0x00001000,                          // 1 << 12
    RKInitFlagAllocHealthBuffer         = 0x00002000,                          // 1 << 13
    RKInitFlagAllocHealthNodes          = 0x00004000,                          // 1 << 14
    RKInitFlagReserved1                 = 0x00008000,                          // 1 << 15
    RKInitFlagPulsePositionCombiner     = 0x00010000,                          // 1 << 16
    RKInitFlagSignalProcessor           = 0x00020000,                          // 1 << 17
    RKInitFlagRelay                     = 0x00007703,                          // 37F00(All) - 800(Pos) - 100000(PPC) - 20000(DSP)
    RKInitFlagIQPlayback                = 0x00027701,                          // 37F00(All) - 800(Pos) - 100000(PPC)
    RKInitFlagAllocEverything           = 0x00037F01,
    RKInitFlagAllocEverythingQuiet      = 0x00037F00,
};

typedef uint32_t RKBaseMomentList;
enum RKBaseMomentList {
    RKBaseMomentListNone                = 0,                                   // None
    RKBaseMomentListDisplayZ            = 1,                                   // Display Z - Reflectivity dBZ
    RKBaseMomentListDisplayV            = (1 << 1),                            // Display V - Velocity
    RKBaseMomentListDisplayW            = (1 << 2),                            // Display W - Width
    RKBaseMomentListDisplayD            = (1 << 3),                            // Display D - Differential Reflectivity
    RKBaseMomentListDisplayP            = (1 << 4),                            // Display P - PhiDP
    RKBaseMomentListDisplayR            = (1 << 5),                            // Display R - RhoHV
    RKBaseMomentListDisplayK            = (1 << 6),                            // Display K - KDP
    RKBaseMomentListDisplaySh           = (1 << 7),                            // Display Sh - Signal from H channel
    RKBaseMomentListDisplaySv           = (1 << 8),                            // Display Sv - Signal from V channel
    RKBaseMomentListDisplayQ            = (1 << 9),                            // Display SQI - Signal Quality Index
    RKBaseMomentListDisplayZVWDPR       = 0x0000003F,                          // Display All without K, Sh, Sv and Q
    RKBaseMomentListDisplayZVWDPRK      = 0x0000007F,                          // Display All without Sh, Sv and Q
    RKBaseMomentListDisplayZVWDPRKS     = 0x000001FF,                          // Display All without Sh, Sv and Q
    RKBaseMomentListDisplayZVWDPRKSQ    = 0x000003FF,                          // Display All
    RKBaseMomentListDisplayAll          = 0x000003FF,                          // Display All (same as above)
    RKBaseMomentListProductZ            = (1 << 16),                           // Data of Z
    RKBaseMomentListProductV            = (1 << 17),                           // Data of V
    RKBaseMomentListProductW            = (1 << 18),                           // Data of W
    RKBaseMomentListProductD            = (1 << 19),                           // Data of D
    RKBaseMomentListProductP            = (1 << 20),                           // Data of P
    RKBaseMomentListProductR            = (1 << 21),                           // Data of R
    RKBaseMomentListProductK            = (1 << 22),                           // Data of K
    RKBaseMomentListProductSh           = (1 << 23),                           // Data of Sh
    RKBaseMomentListProductSv           = (1 << 24),                           // Data of Sv
    RKBaseMomentListProductQ            = (1 << 25),                           // Data of Q
    RKBaseMomentListProductZVWDPR       = 0x003F0000,                          // Base moment data without K, Sh, Sv and Q
    RKBaseMomentListProductZVWDPRK      = 0x007F0000,                          // Base moment data without Sh, Sv and Q
    RKBaseMomentListProductZVWDPRKS     = 0x01FF0000,                          // All data without Q
    RKBaseMomentListProductZVWDPRKSQ    = 0x03FF0000,                          // All data
    RKBaseMomentListProductAll          = 0x03FF0000                           // All data (same as above)
};

typedef uint32_t RKBaseMomentIndex;
enum RKBaseMomentIndex {
    RKBaseMomentIndexZ,
    RKBaseMomentIndexV,
    RKBaseMomentIndexW,
    RKBaseMomentIndexD,
    RKBaseMomentIndexP,
    RKBaseMomentIndexR,
    RKBaseMomentIndexK,
    RKBaseMomentIndexSh,
    RKBaseMomentIndexSv,
    RKBaseMomentIndexQ,
    RKBaseMomentIndexZv,                                                       // No longer used
    RKBaseMomentIndexVv,                                                       // No longer used
    RKBaseMomentIndexWv,                                                       // No longer used
    RKBaseMomentIndexCount
};

typedef uint32_t RKProductType;
enum RKProductType {
    RKProductTypeUnknown             = 0,                                      // Unspecified
    RKProductTypeCellMatch           = (1),                                    //
    RKProductTypePPI                 = (1 << 1),                               //
    RKProductTypeCAPPI               = (1 << 2)                                //
};

typedef uint32_t RKConfigKey;
enum RKConfigKey {
    RKConfigKeyNull,
    RKConfigKeySweepElevation,
    RKConfigKeySweepAzimuth,
    RKConfigKeyPositionMarker,
    RKConfigKeyPRT,
    RKConfigKeyPRF,
    RKConfigKeyDualPRF,
    RKConfigKeyPulseGateCount,
    RKConfigKeyPulseGateSize,
    RKConfigKeyPulseWidth,
    RKConfigKeyWaveform,
    RKConfigKeyWaveformDecimate,
    RKConfigKeyWaveformId,
    RKConfigKeyWaveformName,
    RKConfigKeySystemNoise,
    RKConfigKeySystemZCal,
    RKConfigKeySystemDCal,
    RKConfigKeySystemPCal,
    RKConfigKeyWaveformCalibration,
    RKConfigKeySNRThreshold,
    RKConfigKeySQIThreshold,
    RKConfigKeyVCPDefinition,
    RKConfigKeyRingFilterGateCount,
    RKConfigKeyTransitionGateCount,
    RKConfigUserIntegerParameters,
    RKConfigUserFloatParameters,
    RKConfigKeyTotalNumberOfKeys
};

typedef uint8_t RKHealthNode;
enum RKHealthNode {
    RKHealthNodeRadarKit,
    RKHealthNodeTransceiver,
    RKHealthNodePedestal,
    RKHealthNodeTweeta,
    RKHealthNodeUser1,
    RKHealthNodeUser2,
    RKHealthNodeUser3,
    RKHealthNodeUser4,
    RKHealthNodeUser5,
    RKHealthNodeUser6,
    RKHealthNodeUser7,
    RKHealthNodeUser8,
    RKHealthNodeCount,
    RKHealthNodeInvalid = (RKHealthNode) - 1
};

typedef uint8_t RKScriptProperty;
enum RKScriptProperty {
    RKScriptPropertyNull            = 0,
    RKScriptPropertyProduceZip      = (1),
    RKScriptPropertyProduceTgz      = (1 << 1),
    RKScriptPropertyProduceTarXz    = (1 << 2),
    RKScriptPropertyProduceArchive  = (RKScriptPropertyProduceZip | RKScriptPropertyProduceTgz | RKScriptPropertyProduceTarXz),
    RKScriptPropertyRemoveNCFiles   = (1 << 3)
};

//
// Typical progression:
//
// EngineInit         RKEngineStateAllocated
//
// EngineSetX
// EngineSetY
// EngineSetZ         RKEngineStateProperlyWired
//
// EngineStart        RKEngineStateWantActive
//                    RKEngineStateActivating
//                    RKEngineStateSleep0
//                    RKEngineStateSleep0 -
//                    RKEngineStateActive
//                    RKEngineStateChildAllocated
//                    RKEngineStateChildProperlyWired
//                    RKEngineStateChildActivating
//                    RKEngineStateChildActive
//
// EngineStop         RKEngineStateWantActive -
//                    RKEngineStateDeactivating
//                    RKEngineStateChildDeactivating
//                    RKEngineStateChildActive -
//                    RKEngineStateActive -
//                    (RKEngineStateChildAllocated | RKEngineStateChildProperlyWired | RKEngineStateAllocated | RKEngineStateProperlyWired)
//
typedef uint32_t RKEngineState;
enum RKEngineState {
    RKEngineStateNull                            = 0,                          // Nothing
    RKEngineStateSleep0                          = 1,                          // Usually for a wait just outside of the main while loop
    RKEngineStateSleep1                          = (1 << 1),                   // Stage 1 wait - usually waiting for pulse
    RKEngineStateSleep2                          = (1 << 2),                   // Stage 2 wait
    RKEngineStateSleep3                          = (1 << 3),                   // Stage 3 wait
    RKEngineStateSleepMask                       = 0x0000000F,                 //
    RKEngineStateWritingFile                     = (1 << 4),                   // Generating an output file
    RKEngineStateMemoryChange                    = (1 << 5),                   // Some required pointers are being changed
    RKEngineStateSuspended                       = (1 << 6),                   // All indices stop increasing
    RKEngineStateBusyMask                        = 0x000000F0,                 //
    RKEngineStateReserved                        = (1 << 7),                   //
    RKEngineStateAllocated                       = (1 << 8),                   // Resources have been allocated
    RKEngineStateProperlyWired                   = (1 << 9),                   // All required pointers are properly wired up
    RKEngineStateActivating                      = (1 << 10),                  // The main run loop is being activated
    RKEngineStateDeactivating                    = (1 << 11),                  // The main run loop is being deactivated
    RKEngineStateActive                          = (1 << 13),                  // The engine is active
    RKEngineStateWantActive                      = (1 << 15),                  // The engine is set to want active
    RKEngineStateMainMask                        = 0x0000FF00,                 //
    RKEngineStateChildAllocated                  = (1 << 16),                  // The child resources have been allocated
    RKEngineStateChildProperlyWired              = (1 << 17),                  // Probably not used
    RKEngineStateChildActivating                 = (1 << 18),                  // The children are being activated
    RKEngineStateChildDeactivating               = (1 << 19),                  // The children are being deactivated
    RKEngineStateChildActive                     = (1 << 20),                  // The children are active
    RKEngineStateChildMask                       = 0x001F0000                  //
};

typedef uint32_t RKStatusEnum;
enum RKStatusEnum {
    RKStatusEnumUnknown                          = -3,                         //
    RKStatusEnumOld                              = -3,                         //
    RKStatusEnumInvalid                          = -2,                         //
    RKStatusEnumTooLow                           = -2,                         //
    RKStatusEnumLow                              = -1,                         //
    RKStatusEnumNormal                           =  0,                         //
    RKStatusEnumActive                           =  0,                         //
    RKStatusEnumHigh                             =  1,                         //
    RKStatusEnumStandby                          =  1,                         //
    RKStatusEnumInactive                         =  1,                         //
    RKStatusEnumOutOfRange                       =  1,                         //
    RKStatusEnumTooHigh                          =  2,                         //
    RKStatusEnumNotOperational                   =  2,                         //
    RKStatusEnumOff                              =  2,                         //
    RKStatusEnumFault                            =  2,                         //
    RKStatusEnumNotWired                         =  3,                         //
    RKStatusEnumCritical                         =  4                          // This would the status we may shutdown the radar. Co-incidently, red = 0x4
};

typedef uint32_t RKFileType;
enum RKFileType {
    RKFileTypeIQ,
    RKFileTypeMoment,
    RKFileTypeHealth,
    RKFileTypeLog,
    RKFileTypeCount
};

typedef uint64_t RKStream;
enum RKStream {
    RKStreamNull                                 = 0,                          //
    RKStreamStatusMask                           = 0x0F,                       // Values 0-15 in the lowest 4 bits (exclusive mode)
    RKStreamStatusPositions                      = 1,                          //
    RKStreamStatusPulses                         = 2,                          //
    RKStreamStatusRays                           = 3,                          //
    RKStreamStatusIngest                         = 4,                          // Ingest and processing lag
    RKStreamStatusEngines                        = 5,                          // State of Engines
    RKStreamStatusBuffers                        = 6,                          // Buffer overview
    RKStreamASCIIArtZ                            = 7,                          // Are you ASCII me?
    RKStreamASCIIArtHealth                       = 8,                          // Health in ASCII art
    RKStreamStatusAll                            = 0xFF,                       //
    RKStreamHealthInJSON                         = (1 << 5),                   // Health in JSON
    RKStreamStatusEngineBinary                   = (1 << 6),                   // WIP: Status of various engines
    RKStreamStatusProcessorStatus                = (1 << 7),                   // WIP: Consolidated binary from of the system status
    RKStreamDisplayIQ                            = (1 << 8),                   // Low rate IQ (sub-smpled)
    RKStreamDisplayIQFiltered                    = (1 << 9),                   // Filtered IQ (usually matched filter is applied)
    RKStreamProductIQ                            = (1 << 10),                  // Full rate IQ
    RKStreamProductIQFiltered                    = (1 << 11),                  // Full rate filtered IQ
    RKStreamControl                              = (1 << 15),                  // Controls
    RKStreamScopeStuff                           = 0x0000000000000300ULL,      //
    RKStreamDisplayZ                             = (1 << 16),                  // Display: Z = 0x00010000
    RKStreamDisplayV                             = (1 << 17),                  //
    RKStreamDisplayW                             = (1 << 18),                  //
    RKStreamDisplayD                             = (1 << 19),                  //
    RKStreamDisplayP                             = (1 << 20),                  //
    RKStreamDisplayR                             = (1 << 21),                  //
    RKStreamDisplayK                             = (1 << 22),                  //
    RKStreamDisplaySh                            = (1 << 23),                  //
    RKStreamDisplaySv                            = (1 << 24),                  //
    RKStreamDisplayQ                             = (1 << 25),                  //
    RKStreamDisplayZVWDPRKS                      = 0x0000000001FF0000ULL,      //
    RKStreamDisplayAll                           = 0x0000000003FF0000ULL,      //
    RKStreamProductZ                             = (1ULL << 32),               // Products by ray
    RKStreamProductV                             = (1ULL << 33),               //
    RKStreamProductW                             = (1ULL << 34),               //
    RKStreamProductD                             = (1ULL << 35),               //
    RKStreamProductP                             = (1ULL << 36),               //
    RKStreamProductR                             = (1ULL << 37),               //
    RKStreamProductK                             = (1ULL << 38),               //
    RKStreamProductSh                            = (1ULL << 39),               //
    RKStreamProductSv                            = (1ULL << 40),               //
    RKStreamProductQ                             = (1ULL << 41),               //
    RKStreamProductZVWDPRKS                      = 0x000001FF00000000ULL,      //
    RKStreamProductAll                           = 0x000003FF00000000ULL,      //
    RKStreamSweepZ                               = (1ULL << 48),               // Products by sweep
    RKStreamSweepV                               = (1ULL << 49),               //
    RKStreamSweepW                               = (1ULL << 50),               //
    RKStreamSweepD                               = (1ULL << 51),               //
    RKStreamSweepP                               = (1ULL << 52),               //
    RKStreamSweepR                               = (1ULL << 53),               //
    RKStreamSweepK                               = (1ULL << 54),               //
    RKStreamSweepSh                              = (1ULL << 55),               //
    RKStreamSweepSv                              = (1ULL << 56),               //
    RKStreamSweepQ                               = (1ULL << 57),               //
    RKStreamSweepZVWDPRKS                        = 0x01FF000000000000ULL,      //
    RKStreamSweepAll                             = 0x03FF000000000000ULL,      //
    RKStreamAlmostEverything                     = 0x03FF03FF03FFF000ULL,      // Don't use this.
    RKStreamStatusTerminalChange                 = 0x0400000000000000ULL       // Change terminal size
};

typedef uint8_t RKHostStatus;
enum RKHostStatus {
    RKHostStatusUnknown,
    RKHostStatusUnreachable,
    RKHostStatusPartiallyReachable,
    RKHostStatusReachableUnusual,
    RKHostStatusReachable
};

typedef uint32_t RKProductStatus;
enum RKProductStatus {
    RKProductStatusVacant                        = 0,                          //
    RKProductStatusActive                        = (1 << 0),                   // This slot has been registered
    RKProductStatusBusy                          = (1 << 1),                   // Waiting for processing node
    RKProductStatusSkipped                       = (1 << 2),                   //
    RKProductStatusSleep0                        = (1 << 4),                   // Sleep stage 0 -
    RKProductStatusSleep1                        = (1 << 5),                   // Sleep stage 1 -
    RKProductStatusSleep2                        = (1 << 6),                   // Sleep stage 2 -
    RKProductStatusSleep3                        = (1 << 7)                    // Sleep stage 3 -
};

typedef uint32_t RKTextPreferences;
enum RKTextPreferences {
    RKTextPreferencesNone                        = 0,                          //
    RKTextPreferencesShowColor                   = 1,                          // Use escape sequence for colors
    RKTextPreferencesDrawBackground              = (1 << 1),                   // Repaint the background
    RKTextPreferencesWindowSizeMask              = (7 << 2),                   // Forced windwow size
    RKTextPreferencesWindowSize80x25             = (0 << 2),                   //
    RKTextPreferencesWindowSize80x40             = (1 << 2),                   //
    RKTextPreferencesWindowSize80x50             = (2 << 2),                   //
    RKTextPreferencesWindowSize120x40            = (3 << 2),                   //
    RKTextPreferencesWindowSize120x50            = (4 << 2),                   //
    RKTextPreferencesWindowSize120x80            = (5 << 2),                   //
    RKTextPreferencesShowDebuggingMessage        = (1 << 7)                    //
};

typedef uint32_t RKWaveformType;
enum RKWaveformType {
    RKWaveformTypeNone                           = 0,                          //
    RKWaveformTypeIsComplex                      = 1,                          // Complex form usually represents baseband
    RKWaveformTypeSingleTone                     = (1 << 1),                   // The traditional single frequency waveform
    RKWaveformTypeFrequencyHopping               = (1 << 2),                   //
    RKWaveformTypeLinearFrequencyModulation      = (1 << 3),                   //
    RKWaveformTypeTimeFrequencyMultiplexing      = (1 << 4),                   //
    RKWaveformTypeFromFile                       = (1 << 5),                   //
    RKWaveformTypeFlatAnchors                    = (1 << 6),                   // Frequency hopping has multiple waveforms but the anchors are identical
    RKWaveformTypeFrequencyHoppingChirp          = (1 << 7)                    //
};

typedef uint32_t RKEventType;
enum RKEventType {
    RKEventTypeNull,                                                           //
    RKEventTypeRaySweepBegin,                                                  //
    RKEventTypeRaySweepEnd                                                     //
};

typedef uint8_t RKFilterType;
enum RKFilterType {
    RKFilterTypeNull,                                                          // No filter
    RKFilterTypeElliptical1,                                                   // Elliptical filter, high pass at 0.1 rad / sample
    RKFilterTypeElliptical2,                                                   // Elliptical filter, high pass at 0.2 rad / sample
    RKFilterTypeElliptical3,                                                   // Elliptical filter, high pass at 0.3 rad / sample
    RKFilterTypeElliptical4,                                                   // Elliptical filter, high pass at 0.4 rad / sample
    RKFilterTypeImpulse,                                                       // Impulse at n = 0
    RKFilterTypeCount,                                                         // The count of built-in filters
    RKFilterTypeUserDefined,
    RKFilterTypeTest1
};

typedef uint8_t RKRawDataType;
enum RKRawDataType {
    RKRawDataTypeNull,                                                         // No recording
    RKRawDataTypeFromTransceiver,                                              // Raw straight from the digital transceiver (RKInt16C)
    RKRawDataTypeAfterMatchedFilter                                            // The I/Q samples after pulse compression (RKFloat)
};

#pragma mark - Structure Definitions

//
// A general description of a radar.
// Most parameters are used for initialization.
// Some may be overriden even when the radar is live.
//
typedef struct rk_radar_desc {
    RKInitFlag           initFlags;                                            // Initialization. See RKInitFlag enum.
    uint32_t             pulseCapacity;                                        // Capacity of a pulse, i.e., maximum number of range gates
    uint16_t             pulseToRayRatio;                                      // The down-sampling ratio of range gates from pulses to rays
    uint16_t             doNotUse;                                             //
    uint32_t             healthNodeCount;                                      // Number of user health node count
    uint32_t             healthBufferDepth;                                    // Depth of the cosolidated health buffer
    uint32_t             statusBufferDepth;                                    // Depth of the status buffer (RKStatus)
    uint32_t             configBufferDepth;                                    // Depth of the operational configuration parameters
    uint32_t             positionBufferDepth;                                  // Depth of the position readings
    uint32_t             pulseBufferDepth;                                     // Depth of the pulse buffer
    uint32_t             rayBufferDepth;                                       // Depth of the ray buffer
    uint32_t             productBufferDepth;                                   // Depth of the product buffer
    uint32_t             controlCapacity;                                      // Number of control buttons
    uint32_t             waveformCalibrationCapacity;                          // Number of waveform specific calibrations
    size_t               healthNodeBufferSize;                                 // Buffer size (B)
    size_t               healthBufferSize;                                     // Buffer size (B)
    size_t               statusBufferSize;                                     // Buffer size (B)
    size_t               configBufferSize;                                     // Buffer size (B)
    size_t               positionBufferSize;                                   // Buffer size (B)
    size_t               pulseBufferSize;                                      // Buffer size (B)
    size_t               rayBufferSize;                                        // Buffer size (B)
    size_t               productBufferSize;                                    // Buffer size (B)
    uint32_t             pulseSmoothFactor;                                    // Pulse rate (Hz)
    uint32_t             pulseTicsPerSecond;                                   // Pulse tics per second (normally 10e6)
    uint32_t             positionSmoothFactor;                                 // Position rate (Hz)
    uint32_t             positionTicsPerSecond;                                // Position tics per second
    double               positionLatency;                                      // Position latency (s)
    double               latitude;                                             // Latitude (degrees)
    double               longitude;                                            // Longitude (degrees)
    float                heading;                                              // Radar heading
    float                radarHeight;                                          // Radar height from ground (m)
    float                wavelength;                                           // Radar wavelength (m)
    RKName               name;                                                 // Radar name
    char                 filePrefix[RKMaximumPrefixLength];                    // Prefix of output files
    char                 dataPath[RKMaximumFolderPathLength];                  // Root path for the data files
} RKRadarDesc;

typedef struct rk_waveform {
    RKName               name;                                                 // Waveform name in plain string
    RKWaveformType       type;                                                 // Various type of waveforms
    uint8_t              count;                                                // Number of groups
    int                  depth;                                                // Maximum number of samples
    double               fc;                                                   // Carrier frequency (Hz)
    double               fs;                                                   // Sampling frequency (Hz)
    uint8_t              filterCounts[RKMaximumWaveformCount];                 // Number of filters (see filterAnchors)
    RKFilterAnchorGroup  filterAnchors[RKMaximumWaveformCount];                // Filter anchors of each sub-waveform for de-multiplexing
    RKComplex            *samples[RKMaximumWaveformCount];                     // Samples up to amplitude of 1.0
    RKInt16C             *iSamples[RKMaximumWaveformCount];                    // 16-bit full-scale equivalence of the waveforms
} RKWaveform;

typedef union rk_wave_file_header {
    struct {
        uint8_t         count;                                                 // Count of groups
        uint32_t        depth;                                                 // Waveform depth
        RKWaveformType  type;                                                  // Waveform type
        RKName          name;                                                  // Waveform name
        double          fc;                                                    // Carrier frequency
        double          fs;                                                    // Sampling frequency
        uint8_t         filterCounts[RKMaximumWaveformCount];                  // Number of filters (see filterAnchors)
    };
    char bytes[512];
} RKWaveFileGlobalHeader;

typedef struct rk_waveform_cal {
    uint32_t             uid;                                                  // A unique identifier
    RKName               name;                                                 // A string description
    uint8_t              count;                                                // The number of tones in this waveform
    RKFloat              ZCal[RKMaximumFilterCount][2];                        // Calibration factor for individual tone
    RKFloat              DCal[RKMaximumFilterCount];                           // Calibration factor for individual tone
    RKFloat              PCal[RKMaximumFilterCount];                           // Calibration factor for individual tone
} RKWaveformCalibration;

typedef struct rk_waveform_response {
    uint32_t             count;                                                // Number of combinations (at most 3 for now)
    uint32_t             length;                                               // Length of each filter
    RKFloat              **amplitudeResponse;                                  // An array of amplitudes of [count][length] (dB)
    RKFloat              **phaseResponse;                                      // An array of phases of [count][length] (radians)
} RKWaveformResponse;

//
// A running configuration buffer (version 1, see below for updated version)
//
typedef struct rk_config_v1 {
    RKIdentifier         i;                                                    // Identity counter
    float                sweepElevation;                                       // Sweep elevation angle (degrees)
    float                sweepAzimuth;                                         // Sweep azimuth angle (degrees)
    RKMarker             startMarker;                                          // Marker of the latest start ray
    uint8_t              filterCount;                                          // Number of filters
    RKFilterAnchor       filterAnchors[RKMaximumFilterCount];                  // Filter anchors at ray level
    RKFloat              prt[RKMaximumFilterCount];                            // Pulse repetition time (s)
    RKFloat              pw[RKMaximumFilterCount];                             // Pulse width (s)
    uint32_t             pulseGateCount;                                       // Number of range gates
    RKFloat              pulseGateSize;                                        // Size of range gate (m)
    uint32_t             pulseRingFilterGateCount;                             // Number of range gates to apply ring filter
    uint32_t             waveformId[RKMaximumFilterCount];                     // Transmit waveform
    RKFloat              noise[2];                                             // Noise floor (ADU)
    RKFloat              systemZCal[2];                                        // System-wide Z calibration (dB)
    RKFloat              systemDCal;                                           // System-wide ZDR calibration (dB)
    RKFloat              systemPCal;                                           // System-wide phase calibration (rad)
    RKFloat              ZCal[RKMaximumFilterCount][2];                        // Waveform Z calibration (dB)
    RKFloat              DCal[RKMaximumFilterCount];                           // Waveform ZDR calibration (dB)
    RKFloat              PCal[RKMaximumFilterCount];                           // Waveform phase calibration (rad)
    RKFloat              SNRThreshold;                                         // Censor SNR (dB)
    RKFloat              SQIThreshold;                                         // Censor SQI
    RKName               waveform;                                             // Waveform name
    char                 vcpDefinition[RKMaximumCommandLength];                // Volume coverage pattern
} RKConfigV1;

//
// A running configuration buffer
//
typedef union rk_config {
    struct {
        RKIdentifier         i;                                                // Identity counter
        float                sweepElevation;                                   // Sweep elevation angle (degrees)
        float                sweepAzimuth;                                     // Sweep azimuth angle (degrees)
        RKMarker             startMarker;                                      // Marker of the latest start ray
        RKFloat              prt[RKMaximumFilterCount];                        // Pulse repetition time (s)
        RKFloat              pw[RKMaximumFilterCount];                         // Pulse width (s)
        uint32_t             pulseGateCount;                                   // Number of range gates
        RKFloat              pulseGateSize;                                    // Size of range gate (m)
        uint32_t             transitionGateCount;                              // Transition gate count
        uint32_t             ringFilterGateCount;                              // Number of range gates to apply ring filter
        uint32_t             waveformId[RKMaximumFilterCount];                 // Transmit waveform
        RKFloat              noise[2];                                         // Noise floor (ADU)
        RKFloat              systemZCal[2];                                    // System-wide Z calibration (dB)
        RKFloat              systemDCal;                                       // System-wide ZDR calibration (dB)
        RKFloat              systemPCal;                                       // System-wide phase calibration (rad)
        RKFloat              ZCal[RKMaximumFilterCount][2];                    // Waveform Z calibration (dB)
        RKFloat              DCal[RKMaximumFilterCount];                       // Waveform ZDR calibration (dB)
        RKFloat              PCal[RKMaximumFilterCount];                       // Waveform phase calibration (rad)
        RKFloat              SNRThreshold;                                     // Censor SNR (dB)
        RKFloat              SQIThreshold;                                     // Censor SQI
        RKName               waveformName;                                     // Waveform name
        RKWaveform           *waveform;                                        // Reference to the waveform storage
        RKWaveform           *waveformDecimate;                                // Reference to the waveform storage in Level-II sampling rate
        uint32_t             userIntegerParameters[RKUserParameterCount];      // User integer parameters (not yet)
        float                userFloatParameters[RKUserParameterCount];        // User float parameters (not yet)
        char                 vcpDefinition[RKMaximumCommandLength];            // Volume coverage pattern
    };
    RKByte               bytes[1024];
} RKConfig;

//
// Consolidated health buffer
//
typedef union rk_heath {
    struct {
        RKIdentifier        i;                                                 // Identity counter
        RKHealthFlag        flag;                                              // Health flag
        struct timeval      time;                                              // Time in struct timeval
        double              timeDouble;                                        // Time in double
        char                string[RKMaximumStringLength];                     // Health string
    };
    RKByte               *bytes;
} RKHealth;

//
// Individual health buffer
//
typedef struct rk_nodal_health {
    RKHealth             *healths;                                             // Pointer (8 byte for 64-bit systems)
    uint32_t             index;                                                // Index (4 byte)
    bool                 active;                                               // Active flag (1 byte)
} RKNodalHealth;

//
// Raw position reported from a pedestal
//
typedef union rk_position {
    struct {
        RKIdentifier        i;                                                 // Counter
        uint64_t            tic;                                               // Time tic
        RKFourByte          rawElevation;                                      // Raw elevation readout
        RKFourByte          rawAzimuth;                                        // Raw azimuth readout
        RKFourByte          rawElevationVelocity;                              // Raw velocity of elevation readout
        RKFourByte          rawAzimuthVelocity;                                // Raw velocity of azimuth readout
        RKFourByte          rawElevationStatus;                                // Raw status of elevation readout
        RKFourByte          rawAzimuthStatus;                                  // Raw status of azimuth readout
        uint8_t             queueSize;                                         // Queue size of the readout buffer
        uint8_t             elevationMode;                                     // Positioning mode of elevation
        uint8_t             azimuthMode;                                       // Positioning mode of azimuth
        uint8_t             sequence;                                          // DEBUG command sequence
        RKPositionFlag      flag;                                              // Position flag
        float               elevationDegrees;                                  // Decoded elevation
        float               azimuthDegrees;                                    // Decoded elevation
        float               elevationVelocityDegreesPerSecond;                 // Decoded velocity of elevation
        float               azimuthVelocityDegreesPerSecond;                   // Decoded velocity of azimuth
        float               elevationCounter;                                  // Progress counter (of target) of the elevation
        float               elevationTarget;                                   // Targeted progress counter of the elevation
        float               azimuthCounter;                                    // Progress counter (of target) of the azimuth
        float               azimuthTarget;                                     // Targeted progress counter of the azimuth
        float               sweepElevationDegrees;                             // Set elevation for current sweep
        float               sweepAzimuthDegrees;                               // Set azimuth for current sweep
        struct timeval      time;                                              // Time in struct timeval
        double              timeDouble;                                        // Time in double;
    };
    RKByte               bytes[128];
} RKPosition;

//
// Pulse header
//
typedef struct rk_pulse_header {
    RKIdentifier        i;                                                     // Identity counter
    RKIdentifier        n;                                                     // Network counter, may be useful to indicate packet loss
    uint64_t            t;                                                     // A clean clock-related tic count
    RKPulseStatus       s;                                                     // Status flag
    uint32_t            capacity;                                              // Allocated capacity
    uint32_t            gateCount;                                             // Number of range gates
    uint32_t            downSampledGateCount;                                  // Number of range gates after down-sampling
    RKMarker            marker;                                                // Position Marker
    uint32_t            pulseWidthSampleCount;                                 // Pulsewidth
    struct timeval      time;                                                  // UNIX time in seconds since 1970/1/1 12:00am
    double              timeDouble;                                            // Time in double representation
    RKFourByte          rawAzimuth;                                            // Raw azimuth reading, which may take up to 4 bytes
    RKFourByte          rawElevation;                                          // Raw elevation reading, which may take up to 4 bytes
    uint16_t            configIndex;                                           // Operating configuration index
    uint16_t            configSubIndex;                                        // Operating configuration sub-index
    uint16_t            azimuthBinIndex;                                       // Ray bin
    float               gateSizeMeters;                                        // Size of range gates
    float               elevationDegrees;                                      // Elevation in degrees
    float               azimuthDegrees;                                        // Azimuth in degrees
    float               elevationVelocityDegreesPerSecond;                     // Velocity of elevation in degrees / second
    float               azimuthVelocityDegreesPerSecond;                       // Velocity of azimuth in degrees / second
} RKPulseHeader;

//
// Pulse parameters for matched filters (pulseEngineCore)
//
typedef struct rk_pulse_parameters {
    uint32_t             gid;                                                  //
    uint32_t             filterCounts[2];                                      //
    uint32_t             planIndices[2][RKMaximumFilterCount];                 //
    uint32_t             planSizes[2][RKMaximumFilterCount];                   //
} RKPulseParameters;

//
// Pulse
//
// - RKPulse struct is padded to a SIMD alignment
//
typedef struct rk_pulse {
    union {
        struct {
            RKPulseHeader        header;                                       //
            RKPulseParameters    parameters;                                   //
        };                                                                     //
        RKByte               headerBytes[RKPulseHeaderPaddedSize];             //
    };                                                                         //
    RKByte               data[0];                                              //
} RKPulse;

//
// Ray header
//
typedef struct rk_ray_header {
    uint32_t             capacity;                                             // Capacity
    RKRayStatus          s;                                                    // Ray status
    RKIdentifier         i;                                                    // Ray indentity
    RKIdentifier         n;                                                    // Ray network counter
    RKMarker             marker;                                               // Volume / sweep / radial marker
    RKBaseMomentList     baseMomentList;                                       // 16-bit MSB for products + 16-bit LSB for display
    uint16_t             configIndex;                                          // Operating configuration index
    uint16_t             configSubIndex;                                       // Operating configuration sub-index
    uint16_t             gateCount;                                            // Gate count of the ray
    uint16_t             pulseCount;                                           //
    float                gateSizeMeters;                                       // Size of range gates
    float                sweepElevation;                                       // Sweep elevation for PPI
    float                sweepAzimuth;                                         // Sweep azimuth for RHI
    struct timeval       startTime;                                            // Start time of the ray in UNIX time
    double               startTimeDouble;                                      // Start time in double representation
    float                startAzimuth;                                         // End time in double representation
    float                startElevation;                                       //
    struct timeval       endTime;                                              // End time of the ray in UNIX time
    double               endTimeDouble;                                        //
    float                endAzimuth;                                           //
    float                endElevation;                                         //
    uint8_t              fftOrder;                                             // The order of FFT (2^N) = plan index of FFTModule
    uint8_t              reserved1;                                            //
    uint8_t              reserved2;                                            //
    uint8_t              reserved3;                                            //
} RKRayHeader;

//
// Ray
//
// - RKRay struct is padded to a SIMD alignment
//
typedef struct rk_ray {
    union {
        RKRayHeader      header;
        RKByte           headerBytes[RKRayHeaderPaddedSize];
    };
    RKByte           data[0];
} RKRay;

//
// Sweep header
//
typedef struct rk_sweep_header {
    uint32_t             rayCount;                                             // Number of rays
    uint32_t             gateCount;                                            // Number of range gates
    time_t               startTime;                                            // Start time of the sweep
    time_t               endTime;                                              // End time of the sweep
    RKBaseMomentList     baseMomentList;                                       // List of available products
    float                gateSizeMeters;                                       // Gate size in meters
    bool                 isPPI;                                                //
    bool                 isRHI;                                                //
    bool                 external;                                             // Data is external buffer, reference by *rays[]
    RKRadarDesc          desc;                                                 //
    RKConfig             config;                                               //
    char                 filename[RKMaximumPathLength - 80];                   // Propose filename without symbol and extension, XX-20180520-112233
} RKSweepHeader;

//
// Sweep
//
typedef struct rk_sweep {
    RKSweepHeader        header;
    RKBuffer             rayBuffer;
    RKRay                *rays[RKMaximumRaysPerSweep];
} RKSweep;

//
// File header of raw I/Q data
//
typedef union rk_file_header_v1 {
    struct {                                                                   // Up to buildNo 5
        RKName               preface;                                          //
        uint32_t             buildNo;                                          //
        RKRadarDesc          desc;                                             //
        RKConfigV1           config;                                           //
        RKRawDataType        dataType;                                         //
    };                                                                         //
    RKByte               bytes[4096];                                          //
} RKFileHeaderV1;

typedef union rk_file_header {
    struct {
        RKName               preface;                                          // 128 B
        uint32_t             buildNo;                                          //   4 B
        RKRawDataType        dataType;                                         //   1 B
        uint8_t              reserved[123];                                    // 123 B = 256 B
        RKRadarDesc          desc;                                             //         1072 B
        RKConfig             config;                                           //         1600 B
    };                                                                         //
    RKByte               bytes[4096];                                          //
} RKFileHeader;

//
// Preference entry
//
typedef struct rk_preferene_object {
    char                 keyword[RKNameLength];                                //
    char                 valueString[RKMaximumStringLength];                   //
    bool                 isNumeric;                                            //
    bool                 isValid;                                              //
    int                  numericCount;                                         //
    char                 subStrings[4][RKNameLength];                          //
    double               doubleValues[4];                                      //
    bool                 boolValues[4];                                        //
} RKPreferenceObject;

//
// Control
//
typedef struct rk_control {
    uint32_t             uid;                                                  // A unique identifier
    uint8_t              state;                                                // Some internal state for house keeping
    uint8_t              level;                                                // Root level controls are for top interface
    char                 label[RKNameLength];                                  // Label up to RKNameLength
    char                 command[RKMaximumCommandLength];                      // Control command
} RKControl;

//
// Status
//
// - This can be a supported feature reported back from client
//
typedef struct rk_status {
    RKIdentifier         i;
    RKStatusFlag         flag;
    size_t               memoryUsage;
    uint8_t              pulseMonitorLag;
    uint8_t              pulseSkipCount;
    uint8_t              pulseCoreLags[RKProcessorStatusPulseCoreCount];
    uint8_t              pulseCoreUsage[RKProcessorStatusPulseCoreCount];
    uint8_t              ringMonitorLag;
    uint8_t              ringSkipCount;
    uint8_t              ringCoreLags[RKProcessorStatusRingCoreCount];
    uint8_t              ringCoreUsage[RKProcessorStatusRingCoreCount];
    uint8_t              rayMonitorLag;
    uint8_t              raySkipCount;
    uint8_t              rayCoreLags[RKProcessorStatusRayCoreCount];
    uint8_t              rayCoreUsage[RKProcessorStatusRayCoreCount];
    uint8_t              recorderLag;
} RKStatus;

//
// Simple engine
//
// - File monitor, etc.
//
typedef struct rk_simple_engine {
    RKName               name;                                                 // Engine name
    uint8_t              verbose;                                              //
    pthread_t            tid;                                                  //
    RKEngineState        state;                                                //
    uint32_t             memoryUsage;                                          //
    void                 *userResource;                                        // User defined resource
} RKSimpleEngine;

//
// File monitor
//
typedef struct rk_file_monitor {                                               // Simple engine template
    RKName               name;                                                 // Engine name
    uint8_t              verbose;                                              //
    pthread_t            tid;                                                  //
    RKEngineState        state;                                                //
    uint32_t             memoryUsage;                                          //
    char                 filename[RKMaximumPathLength];                        // User defined file to monitor
    void                 (*callbackRoutine)(void *);                           // User defined callback function
    void                 *userResource;                                        // User defined resource
} RKFileMonitor;

//
// User product from other processing nodes
//

typedef union rk_product_desc {                                                // A 1-KB struct that describes a product
    struct {                                                                   //
        uint32_t             key;                                              // A unique key to identify the product routine
        RKName               name;                                             // Name of the product
        RKName               unit;                                             // Unit of the product
        RKName               colormap;                                         // Colormap of the product for the UI
        char                 symbol[8];                                        // Product symbol
        RKBaseMomentIndex    index;                                            // Base moment index
        RKProductType        type;                                             // RKProductType
        uint32_t             pieceCount;                                       // Count of piece-wise function that maps data to color index
        RKFloat              w[16];                                            // Data to color index weight (piece-wise function)
        RKFloat              b[16];                                            // Data to color index bias (piece-wise function)
        RKFloat              l[16];                                            // The lower bound of each piece
        RKFloat              mininimumValue;                                   // Minimum value
        RKFloat              maximumValue;                                     // Maximum value
    };
    RKByte bytes[1024];
} RKProductDesc;

typedef union rk_product_header {
    struct {                                                                   // A 1-KB struct that contains meta data
        RKName               radarName;                                        // Radar name
        double               latitude;                                         // Latitude (degrees)
        double               longitude;                                        // Longitude (degrees)
        float                heading;                                          // Radar heading
        float                radarHeight;                                      // Radar height from ground (m)
        float                wavelength;                                       // Radar wavelength (m)
        float                sweepElevation;                                   // Sweep elevation angle (degrees)
        float                sweepAzimuth;                                     // Sweep azimuth angle (degrees)
        uint32_t             rayCount;                                         // Number of rays
        uint32_t             gateCount;                                        // Number of range gates
        float                gateSizeMeters;                                   // Gate size in meters
        time_t               startTime;                                        // Start time of the sweep
        time_t               endTime;                                          // End time of the sweep
        bool                 isPPI;                                            // PPI indicator
        bool                 isRHI;                                            // RHI indicator
        RKFloat              prt[RKMaximumFilterCount];                        // Pulse repetition time (s)
        RKFloat              pw[RKMaximumFilterCount];                         // Pulse width (s)
        RKFloat              noise[2];                                         // Noise floor (ADU)
        RKFloat              systemZCal[2];                                    // System-wide Z calibration (dB)
        RKFloat              systemDCal;                                       // System-wide ZDR calibration (dB)
        RKFloat              systemPCal;                                       // System-wide phase calibration (rad)
        RKFloat              ZCal[RKMaximumFilterCount][2];                    // Waveform Z calibration (dB)
        RKFloat              DCal[RKMaximumFilterCount];                       // Waveform ZDR calibration (dB)
        RKFloat              PCal[RKMaximumFilterCount];                       // Waveform phase calibration (rad)
        RKFloat              SNRThreshold;                                     // Censor SNR (dB)
        RKFloat              SQIThreshold;                                     // Censor SQI
        RKName               waveformName;                                     // Waveform name
        char                 vcpDefinition[RKMaximumCommandLength];            // Volume coverage pattern
        char                 suggestedFilename[RKMaximumPathLength];           // RadarKit suggested fullpath filename
    };
    RKByte bytes[2048];
} RKProductHeader;

typedef struct rk_product {                                                    // A description of user product
    RKIdentifier         i;                                                    // Product counter to be synchronized with RKConfig->i
    RKProductId          pid;                                                  // Product identifier from RKProductRegister()
    RKProductDesc        desc;                                                 // Description
    RKProductStatus      flag;                                                 // Various state
    RKProductHeader      header;                                               // Product header
    uint32_t             capacity;                                             // Number of RKFloat elements in blocks of array
    uint32_t             totalBufferSize;                                      // Total buffer size of this struct
    RKFloat              *startAzimuth;                                        // Start azimuth of each ray
    RKFloat              *endAzimuth;                                          // End azimuth of each ray
    RKFloat              *startElevation;                                      // Start elevation of each ray
    RKFloat              *endElevation;                                        // End elevation of each ray
    RKFloat              *data;                                                // Flattened array of user product
} RKProduct;

typedef struct rk_product_collection {
    uint32_t             count;                                                // Number of products
    RKProduct            *products;                                            // Products
} RKProductCollection;

typedef struct rk_iir_filter {
    RKName               name;                                                 // String description of the filter
    RKFilterType         type;                                                 // Built-in type
    uint32_t             bLength;                                              // Length of b's
    uint32_t             aLength;                                              // Length of a's
    RKComplex            B[RKMaximumIIRFilterTaps];                            // Coefficient b's
    RKComplex            A[RKMaximumIIRFilterTaps];                            // Coefficient a's
} RKIIRFilter;

typedef struct rk_task {
    RKCommand            command;                                              // A ocmmand string for RKRadarExecute()
    double               timeout;                                              // Maximum time for completion
    RKEventType          endingEvent;                                          // Ending event that indicates completion of the task
} RKTask;

typedef struct rk_command_queue {
    uint8_t              head;                                                 // Head index (for push)
    uint8_t              tail;                                                 // Tail index (for pop)
    uint8_t              count;                                                // Number of elements that can be popped
    uint8_t              depth;                                                // Buffer depth
    bool                 nonblocking;                                          // Non-blocking operations
    RKCommand            *buffer;                                              // Buffer
    pthread_mutex_t      lock;                                                 // Mutually exclusive access lock
    uint32_t             tic;                                                  // Deposit count
    uint32_t             toc;                                                  // Withdrawal count
} RKCommandQueue;

#pragma pack(pop)

#endif /* defined(__RadarKit_Types__) */
