//
//  RKTest.h
//  RadarKit
//
//  Created by Boonleng Cheong on 3/18/15.
//  Copyright (c) 2015 Boonleng Cheong. All rights reserved.
//

#ifndef __RadarKit_Test__
#define __RadarKit_Test__

#include <RadarKit/RKRadar.h>

#define RKTestWaveformCacheCount 2

typedef int RKTestFlag;
enum RKTestFlag {
    RKTestFlagNone         = 0,
    RKTestFlagVerbose      = 1,
    RKTestFlagShowResults  = 1 << 1
};

typedef int RKTestSIMDFlag;
enum RKTestSIMDFlag {
    RKTestSIMDFlagNull                       = 0,
    RKTestSIMDFlagShowNumbers                = 1,
    RKTestSIMDFlagPerformanceTestArithmetic  = 1 << 1,
    RKTestSIMDFlagPerformanceTestConversion  = 1 << 2,
    RKTestSIMDFlagPerformanceTestAll         = RKTestSIMDFlagPerformanceTestArithmetic | RKTestSIMDFlagPerformanceTestConversion
};

typedef int RKTestPedestalScanMode;
enum RKTestPedestalScanMode {
    RKTestPedestalScanModeNull,
    RKTestPedestalScanModePPI,
    RKTestPedestalScanModeRHI,
    RKTestPedestalScanModeBadPedestal
};

typedef struct rk_test_transceiver {
    RKName         name;
    int            verbose;
    int            sleepInterval;
    int            gateCapacity;
    int            gateCount;
    float          gateSizeMeters;
    long           counter;
    double         fs;
    double         prt;
    RKByte         sprt;
    RKWaveform     *waveformCache[RKTestWaveformCacheCount];
    unsigned int   waveformCacheIndex;
	RKCommand      customCommand;
    pthread_t      tidRunLoop;
    RKEngineState  state;
    RKRadar        *radar;
    size_t         memoryUsage;
    bool           simFault;
    bool           transmitting;
    int            chunkSize;
    double         periodEven;
    double         periodOdd;
    long           ticEven;
    long           ticOdd;
    char           playbackFolder[RKMaximumFolderPathLength];
    RKFileHeader   fileHeaderCache;
    RKPulseHeader  pulseHeaderCache;
    RKByte         dump[1024 * 1024];
} RKTestTransceiver;

typedef struct rk_test_pedestal {
    RKName         name;
    int            verbose;
    long           counter;
    int            scanMode;
    int            commandCount;
    float          scanElevation;
    float          scanAzimuth;
    float          speedAzimuth;
    float          speedElevation;
    float          rhiElevationStart;
    float          rhiElevationEnd;
    pthread_t      tidRunLoop;
    RKEngineState  state;
    RKRadar        *radar;
    size_t         memoryUsage;
} RKTestPedestal;

typedef struct rk_test_health_relay {
    RKName         name;
    int            verbose;
    long           counter;
    pthread_t      tidRunLoop;
    RKEngineState  state;
    RKRadar        *radar;
    size_t         memoryUsage;
} RKTestHealthRelay;

#pragma mark - Test By Number

char *RKTestByNumberDescription(const int);
void RKTestByNumber(const int, const void *);

#pragma mark - Basic Tests

void RKTestTerminalColors(void);
void RKTestPrettyStrings(void);
void RKTestModuloMath(void);
void RKTestParseCommaDelimitedValues(void);
void RKTestParseJSONString(void);
void RKTestFileManager(void);
void RKTestPreferenceReading(void);
void RKTestCountFiles(void);
void RKTestFileMonitor(void);
void RKTestHostMonitor(void);
void RKTestInitializingRadar(void);
void RKTestTemperatureToStatus(void);
void RKTestGetCountry(void);
void RKTestBufferOverviewText(const char *);
void RKTestHealthOverviewText(const char *);
void RKTestSweepRead(const char *);
void RKTestProductRead(const char *);
void RKTestProductWrite(void);
void RKTestReviseLogicalValues(void);
void RKTestReadIQ(const char *);
void RKTestPreparePath(void);

#pragma mark - DSP Tests

void RKTestSIMD(const RKTestSIMDFlag);
void RKTestWindow(void);
void RKTestHilbertTransform(void);
void RKTestWriteFFTWisdom(void);
void RKTestRingFilterShowCoefficients(void);
void RKTestRamp(void);

#pragma mark - Waveform Tests

void RKTestMakeHops(void);
void RKTestWaveformTFM(void);
void RKTestWaveformWrite(void);
void RKTestWaveformDownsampling(void);
void RKTestWaveformShowProperties(void);
void RKTestWaveformShowUserWaveformProperties(const char *filename);

#pragma mark - Radar Signal Processing

void RKTestPulseCompression(RKTestFlag);
void RKTestOneRay(int method(RKScratch *, RKPulse **, const uint16_t), const int);
void RKTestOneRaySpectra(int method(RKScratch *, RKPulse **, const uint16_t), const int lag);

#pragma mark - Performance Tests

void RKTestPulseCompressionSpeed(void);
void RKTestMomentProcessorSpeed(void);
void RKTestCacheWrite(void);

#pragma mark - Transceiver Emulator

RKTransceiver RKTestTransceiverInit(RKRadar *, void *);
int RKTestTransceiverExec(RKTransceiver, const char *, char *);
int RKTestTransceiverFree(RKTransceiver);

#pragma mark - Pedestal Emulator

RKPedestal RKTestPedestalInit(RKRadar *, void *);
int RKTestPedestalExec(RKPedestal, const char *, char *);
int RKTestPedestalFree(RKPedestal);

#pragma mark - Health Relay Emulator

RKHealthRelay RKTestHealthRelayInit(RKRadar *, void *);
int RKTestHealthRelayExec(RKHealthRelay, const char *, char *);
int RKTestHealthRelayFree(RKHealthRelay);

#pragma mark -

void RKTestCommandQueue(void);
void RKTestSingleCommand(void);
void RKTestExperiment(void);

#endif /* defined(__RadarKit_RKFile__) */
