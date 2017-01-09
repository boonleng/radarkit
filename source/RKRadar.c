//
//  RKRadar.c
//  RadarKit
//
//  Created by Boon Leng Cheong on 3/17/15.
//
//

#include <RadarKit/RKRadar.h>

//
//
//

#pragma mark - Helper Functions

RKTransceiver backgroundTransceiverInit(void *in) {
    RKRadar *radar = (RKRadar *)in;
    return radar->transceiverInit(radar, radar->transceiverInitInput);
}

RKPedestal backgroundPedestalInit(void *in) {
    RKRadar *radar = (RKRadar *)in;
    return radar->pedestalInit(radar, radar->pedestalInitInput);
}

void *radarCoPilot(void *in) {
    RKRadar *radar = (RKRadar *)in;
    RKMomentEngine *productGenerator = radar->momentEngine;
    int k = 0;
    int s = 0;

    RKLog("CoPilot started.  %d\n", productGenerator->rayStatusBufferIndex);
    
    while (radar->active) {
        s = 0;
        while (k == productGenerator->rayStatusBufferIndex && radar->active) {
            usleep(1000);
            if (++s % 200 == 0) {
                RKLog("Nothing ...\n");
            }
        }
        RKLog(productGenerator->rayStatusBuffer[k]);
        k = RKNextModuloS(k, RKBufferSSlotCount);
    }
    return NULL;
}

#pragma mark - Life Cycle

RKRadar *RKInitWithDesc(const RKRadarInitDesc desc) {
    RKRadar *radar;
    size_t bytes;

    if (desc.initFlags & RKInitFlagVerbose) {
        RKLog("Initializing ... 0x%x", desc.initFlags);
    }
    // Allocate self
    bytes = sizeof(RKRadar);
    if (posix_memalign((void **)&radar, RKSIMDAlignSize, bytes)) {
        fprintf(stderr, "Error allocation memory for radar.\n");
        return NULL;
    }
    memset(radar, 0, bytes);

    // Set some non-zero variables
    strcpy(radar->name, "PX-10k");
    radar->state |= RKRadarStateBaseAllocated;
    radar->active = true;
    radar->memoryUsage += bytes;

    // Copy over the input flags and constaint the capacity and depth to hard-coded limits
    radar->desc = desc;
    if (radar->desc.pulseBufferDepth > RKBuffer0SlotCount) {
        radar->desc.pulseBufferDepth = RKBuffer0SlotCount;
    }
    if (radar->desc.pulseCapacity > RKGateCount) {
        radar->desc.pulseCapacity = RKGateCount;
    }
    if (radar->desc.rayBufferDepth > RKBuffer2SlotCount) {
        radar->desc.rayBufferDepth = RKBuffer2SlotCount;
    }

    // Config buffer
    radar->state |= RKRadarStateConfigBufferAllocating;
    bytes = RKBufferCSlotCount * sizeof(RKOperatingParameters);
    radar->parameters = (RKOperatingParameters *)malloc(bytes);
    if (radar->parameters == NULL) {
        RKLog("Error. Unable to allocate memory for pulse parameters");
        return NULL;
    }
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("Config buffer occupies %s B\n", RKIntegerToCommaStyleString(bytes));
    }
    memset(radar->parameters, 0, bytes);
    radar->memoryUsage += bytes;
    radar->state ^= RKRadarStateConfigBufferAllocating;
    radar->state |= RKRadarStateConfigBufferIntialized;

    // IQ buffer
    if (radar->desc.initFlags & RKInitFlagAllocRawIQBuffer) {
        radar->state |= RKRadarStateRawIQBufferAllocating;
        bytes = RKPulseBufferAlloc(&radar->pulses, radar->desc.pulseCapacity, radar->desc.pulseBufferDepth);
        if (bytes == 0 || radar->pulses == NULL) {
            RKLog("Error. Unable to allocate memory for I/Q pulses");
            return NULL;
        }
        if (radar->desc.initFlags & RKInitFlagVerbose) {
            RKLog("Level I buffer occupies %s B  (%s gates x %s pulses)\n",
                  RKIntegerToCommaStyleString(bytes),
                  RKIntegerToCommaStyleString(radar->desc.pulseCapacity),
                  RKIntegerToCommaStyleString(radar->desc.pulseBufferDepth));
        }
        for (int i = 0; i < radar->desc.pulseBufferDepth; i++) {
            RKPulse *pulse = RKGetPulse(radar->pulses, i);
            size_t offset = (size_t)pulse->data - (size_t)pulse;
            if (offset != 256) {
                printf("Unexpected offset = %d != 256\n", (int)offset);
            }
        }
        radar->memoryUsage += bytes;
        radar->state ^= RKRadarStateRawIQBufferAllocating;
        radar->state |= RKRadarStateRawIQBufferInitialized;
    }

    // Moment bufer
    if (radar->desc.initFlags & RKInitFlagAllocMomentBuffer) {
        radar->state |= RKRadarStateRayBufferAllocating;
        bytes = RKRayBufferAlloc(&radar->rays, radar->desc.pulseCapacity / radar->desc.pulseToRayRatio, radar->desc.rayBufferDepth);
        if (radar->desc.initFlags & RKInitFlagVerbose) {
            RKLog("Level II buffer occupies %s B  (%d products of %s gates x %s rays)\n",
                  RKIntegerToCommaStyleString(bytes),
                  RKMaxProductCount,
                  RKIntegerToCommaStyleString(radar->desc.pulseCapacity / radar->desc.pulseToRayRatio),
                  RKIntegerToCommaStyleString(radar->desc.rayBufferDepth));
        }
        radar->memoryUsage += bytes;
        radar->state ^= RKRadarStateRayBufferAllocating;
        radar->state |= RKRadarStateRayBufferInitialized;
    }

    // Clock
    radar->clock = RKClockInitWithSize(5000, 1000);
    RKClockSetName(radar->clock, "<pulseClock>");
    radar->memoryUsage += sizeof(RKClock);
    
    // Pulse compression engine
    radar->pulseCompressionEngine = RKPulseCompressionEngineInit();
    RKPulseCompressionEngineSetInputOutputBuffers(radar->pulseCompressionEngine,
                                                  radar->pulses, &radar->pulseIndex, radar->desc.pulseBufferDepth);
    radar->memoryUsage += sizeof(RKPulseCompressionEngine);
    radar->state |= RKRadarStatePulseCompressionEngineInitialized;

    // Position engine
    radar->positionEngine = RKPositionEngineInit();
    RKPositionEngineSetInputOutputBuffers(radar->positionEngine,
                                          radar->pulses, &radar->pulseIndex, radar->desc.pulseBufferDepth);
    radar->memoryUsage += sizeof(RKPositionEngine);
    radar->state |= RKRadarStatePositionEngineInitialized;
    
    // Moment engine
    radar->momentEngine = RKMomentEngineInit();
    RKMomentEngineSetInputOutputBuffers(radar->momentEngine,
                                        radar->pulses, &radar->pulseIndex, radar->desc.pulseBufferDepth,
                                        radar->rays, &radar->rayIndex, radar->desc.rayBufferDepth);
    radar->memoryUsage += sizeof(RKMomentEngine);
    radar->state |= RKRadarStateMomentEngineInitialized;

    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("Radar initialized. Data buffers occupy %s B (%s GiB)\n",
              RKIntegerToCommaStyleString(radar->memoryUsage),
              RKFloatToCommaStyleString(1.0e-9f * radar->memoryUsage));
    }

    return radar;
}

RKRadar *RKInitQuiet(void) {
    RKRadarInitDesc desc;
    desc.initFlags = RKInitFlagAllocEverythingQuiet;
    desc.pulseCapacity = RKGateCount;
    desc.pulseToRayRatio = 1;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

RKRadar *RKInitLean(void) {
    RKRadarInitDesc desc;
    desc.initFlags = RKInitFlagAllocEverything;
    desc.pulseCapacity = 2048;
    desc.pulseToRayRatio = 1;
    desc.pulseBufferDepth = 10000;
    desc.rayBufferDepth = 2000;
    return RKInitWithDesc(desc);
}

RKRadar *RKInitMean(void) {
    RKRadarInitDesc desc;
    desc.initFlags = RKInitFlagAllocEverything;
    desc.pulseCapacity = RKGateCount / 2;
    desc.pulseToRayRatio = 2;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

RKRadar *RKInitFull(void) {
    RKRadarInitDesc desc;
    desc.initFlags = RKInitFlagAllocEverything;
    desc.pulseCapacity = RKGateCount;
    desc.pulseToRayRatio = 1;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

RKRadar *RKInit(void) {
    return RKInitFull();
}

int RKFree(RKRadar *radar) {
    if (radar->active) {
        RKStop(radar);
    }
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("Freeing radar ...\n");
    }
    RKClockFree(radar->clock);
    RKMomentEngineFree(radar->momentEngine);
    RKPositionEngineFree(radar->positionEngine);
    RKPulseCompressionEngineFree(radar->pulseCompressionEngine);
    while (radar->state & RKRadarStateRawIQBufferAllocating) {
        usleep(1000);
    }
    if (radar->state & RKRadarStateRawIQBufferInitialized) {
        free(radar->pulses);
    }
    while (radar->state & RKRadarStateRayBufferAllocating) {
        usleep(1000);
    }
    if (radar->state & RKRadarStateRayBufferInitialized) {
        free(radar->rays);
    }
    free(radar);
    return EXIT_SUCCESS;
}

#pragma mark - Hardware Hooks

int RKSetTransceiver(RKRadar *radar, RKTransceiver init(RKRadar *, void *), void *initInput) {
    radar->transceiverInit = init;
    radar->transceiverInitInput = initInput;
    return RKResultNoError;
}

int RKSetPedestal(RKRadar *radar, RKPedestal init(RKRadar *, void *), void *initInput) {
    radar->pedestalInit = init;
    radar->pedestalInitInput = initInput;
    return RKResultNoError;
}

#pragma mark - Properties

int RKSetVerbose(RKRadar *radar, const int verbose) {
    if (verbose) {
        RKLog("Setting verbose level to %d ...\n", verbose);
    }
    if (verbose == 1) {
        radar->desc.initFlags |= RKInitFlagVerbose;
    } else if (verbose == 2) {
        radar->desc.initFlags |= RKInitFlagVeryVerbose;
    } else if (verbose >= 3) {
        radar->desc.initFlags |= RKInitFlagVeryVeryVerbose;
    }
    RKClockSetVerbose(radar->clock, verbose);
    RKPulseCompressionEngineSetVerbose(radar->pulseCompressionEngine, verbose);
    RKPositionEngineSetVerbose(radar->positionEngine, verbose);
    RKMomentEngineSetVerbose(radar->momentEngine, verbose);
    return RKResultNoError;
}

int RKSetDeveloperMode(RKRadar *radar) {
    RKMomentEngineSetDeveloperMode(radar->momentEngine);
    return RKResultNoError;
}

//
// NOTE: Function incomplete, need to define file format
// ingest the samples, convert, etc.
//
int RKSetWaveform(RKRadar *radar, const char *filename, const int group, const int maxDataLength) {
    if (radar->pulseCompressionEngine == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    // Load in the waveform
    // Call a transceiver delegate function to fill in the DAC
    RKComplex filter[] = {{1.0f, 0.0f}};
    return RKPulseCompressionSetFilter(radar->pulseCompressionEngine, filter, 1, 0, maxDataLength, group, 0);
}

int RKSetWaveformToImpulse(RKRadar *radar) {
    if (radar->pulseCompressionEngine == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    return RKPulseCompressionSetFilterToImpulse(radar->pulseCompressionEngine);
}

int RKSetWaveformTo121(RKRadar *radar) {
    if (radar->pulseCompressionEngine == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    return RKPulseCompressionSetFilterTo121(radar->pulseCompressionEngine);
}

int RKSetProcessingCoreCounts(RKRadar *radar,
                              const unsigned int pulseCompressionCoreCount,
                              const unsigned int momentProcessorCoreCount) {
    if (radar->state & RKRadarStateLive) {
        return RKResultUnableToChangeCoreCounts;
    }
    RKPulseCompressionEngineSetCoreCount(radar->pulseCompressionEngine, pulseCompressionCoreCount);
    RKMomentEngineSetCoreCount(radar->momentEngine, momentProcessorCoreCount);
    return RKResultNoError;
}

int RKSetPRF(RKRadar *radar, const float prf) {
    return RKResultNoError;
}

uint32_t RKGetPulseCapacity(RKRadar *radar) {
    if (radar->pulses == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    RKPulse *pulse = RKGetPulse(radar->pulses, 0);
    return pulse->header.capacity;
}

void RKSetPulseTicsPerSeconds(RKRadar *radar, const double delta) {
    RKClockSetDxDu(radar->clock, 1.0 / delta);
}

void RKSetPositionTicsPerSeconds(RKRadar *radar, const double delta) {
    RKClockSetDxDu(radar->positionEngine->clock, 1.0 / delta);
}

#pragma mark - Interaction / State Change

int RKGoLive(RKRadar *radar) {
    RKPulseCompressionEngineStart(radar->pulseCompressionEngine);
    RKPositionEngineStart(radar->positionEngine);
    RKMomentEngineStart(radar->momentEngine);

    // Pedestal
    if (radar->pedestalInit != NULL) {
        if (radar->desc.initFlags & RKInitFlagVerbose) {
            RKLog("Initializing pedestal ...");
        }
        pthread_create(&radar->pedestalThreadId, NULL, backgroundPedestalInit, radar);
        while (radar->positionEngine->clock->count < 10) {
            usleep(1000);
        }
    }

    // Transceiver
    if (radar->transceiverInit != NULL) {
        if (radar->desc.initFlags & RKInitFlagVerbose) {
            RKLog("Initializing transceiver ...");
        }
        pthread_create(&radar->transceiverThreadId, NULL, backgroundTransceiverInit, radar);
    }
    radar->state |= RKRadarStateLive;

    // Launch a co-pilot to monitor status of various engines
//    if (radar->momentEngine != NULL && radar->desc.initFlags & RKInitFlagVerbose) {
//        RKLog("Initializing status monitor ...");
//        pthread_create(&radar->monitorThreadId, NULL, radarCoPilot, radar);
//    }
    
    return 0;
}

int RKWaitWhileActive(RKRadar *radar) {
    while (radar->active) {
        usleep(100000);
    }
    return 0;
}

int RKStop(RKRadar *radar) {
    radar->active = false;
    if (radar->state & RKRadarStatePulseCompressionEngineInitialized) {
        // Expect <pulseWatcher> stopped
        RKPulseCompressionEngineStop(radar->pulseCompressionEngine);
        radar->state ^= RKRadarStatePulseCompressionEngineInitialized;
    }
    if (radar->state & RKRadarStatePositionEngineInitialized) {
        // Expect <pulseTagger> stopped
        RKPositionEngineStop(radar->positionEngine);
        radar->state ^= RKRadarStatePositionEngineInitialized;
    }
    if (radar->state & RKRadarStateMomentEngineInitialized) {
        // Expect <pulseGatherer> stopped
        RKMomentEngineStop(radar->momentEngine);
        radar->state ^= RKRadarStateMomentEngineInitialized;
    }
    if (radar->transceiverInit != NULL) {
        if (radar->transceiverExec != NULL) {
            radar->transceiverExec(radar->transceiver, "stop");
        }
        pthread_join(radar->transceiverThreadId, NULL);
    }
    if (radar->pedestalInit != NULL) {
        if (radar->pedestalExec != NULL) {
            radar->pedestalExec(radar->pedestal, "stop");
        }
        pthread_join(radar->pedestalThreadId, NULL);
    }
    return 0;
}

#pragma mark -
#pragma mark Pulses and Rays

RKPulse *RKGetVacantPulse(RKRadar *radar) {
    if (radar->pulses == NULL) {
        RKLog("Error. Buffer for raw pulses has not been allocated.\n");
        exit(EXIT_FAILURE);
    }
    RKPulse *pulse = RKGetPulse(radar->pulses, radar->pulseIndex);
    pulse->header.s = RKPulseStatusVacant;
    pulse->header.i += radar->desc.pulseBufferDepth;
    pulse->header.timeDouble = 0.0;
    pulse->header.time.tv_sec = 0;
    pulse->header.time.tv_usec = 0;
    return pulse;
}

void RKSetPulseHasData(RKRadar *radar, RKPulse *pulse) {
    if (pulse->header.timeDouble == 0.0 && pulse->header.time.tv_sec == 0) {
        pulse->header.timeDouble = RKClockGetTime(radar->clock, (double)pulse->header.t, &pulse->header.time);
    }
    pulse->header.s = RKPulseStatusHasIQData;
    radar->pulseIndex = RKNextModuloS(radar->pulseIndex, radar->desc.pulseBufferDepth);
}

void RKSetPulseReady(RKRadar *radar, RKPulse *pulse) {
    pulse->header.s = RKPulseStatusHasIQData | RKPulseStatusHasPosition;
}

RKPosition *RKGetVacantPosition(RKRadar *radar) {
    if (radar->positionEngine == NULL) {
        RKLog("Error. Pedestal engine has not started.\n");
        exit(EXIT_FAILURE);
    }
    return RKPositionEngineGetVacantPosition(radar->positionEngine);
}

void RKSetPositionReady(RKRadar *radar, RKPosition *position) {
    return RKPositionEngineSetPositionReady(radar->positionEngine, position);
}
