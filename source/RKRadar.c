//
//  RKRadar.c
//  RadarKit
//
//  Created by Boonleng Cheong on 3/17/15.
//
//

#include <RadarKit/RKRadar.h>

//
//
//

// More struct / function definitions

typedef struct rk_radar_command {
    RKRadar *radar;
    RKCommand command;
} RKRadarCommand;

#pragma mark - Static Functions

typedef uint32_t UIDType;
enum UIDType {
    UIDTypeControl,
    UIDTypeWaveformCalibration
};

static uint32_t getUID(UIDType type) {
    static uint32_t uidControl = 0;
    static uint32_t uidWaveformCalibration = 0;
    switch (type) {
        case UIDTypeControl:
            return uidControl++;
            break;
        case UIDTypeWaveformCalibration:
            return uidWaveformCalibration++;
            break;
        default:
            RKLog("Warning. Unknown UID Type %d.\n", type);
            break;
    }
    return 0;
}

void RKUpdateWaveformCalibration(RKRadar *, const uint8_t, const RKWaveformCalibration *);
void RKUpdateControl(RKRadar *, const uint8_t, const RKControl *);

#pragma mark - Engine Monitor

static size_t RKGetRadarMemoryUsage(RKRadar *radar) {
    int k;
    size_t size;
    size = sizeof(RKRadar);
    size += radar->desc.statusBufferSize;
    size += radar->desc.configBufferSize;
    size += radar->desc.healthBufferSize;
    size += radar->desc.healthNodeBufferSize;
    size += radar->desc.positionBufferSize;
    size += radar->desc.pulseBufferSize;
    size += radar->desc.rayBufferSize;
    size += radar->desc.waveformCalibrationCapacity * sizeof(RKWaveformCalibration);
    size += radar->desc.controlCapacity * sizeof(RKControl);
    // Product buffer is dynamically allocated and reallocated
    radar->desc.productBufferSize = 0;
    for (k = 0; k < radar->desc.productBufferDepth; k++) {
        radar->desc.productBufferSize += radar->products[k].totalBufferSize;
    }
    size += radar->desc.productBufferSize;
    // Memory usage of various internal engines
    size += radar->fileManager->memoryUsage;
    size += radar->hostMonitor->memoryUsage;
    if (radar->desc.initFlags & RKInitFlagPulsePositionCombiner) {
        size += 2 * sizeof(RKClock);
        size += radar->positionEngine->memoryUsage;
    }
    if (radar->desc.initFlags & RKInitFlagSignalProcessor) {
        size += radar->pulseEngine->memoryUsage;
        size += radar->pulseRingFilterEngine->memoryUsage;
        size += radar->momentEngine->memoryUsage;
        size += radar->healthEngine->memoryUsage;
        if (radar->systemInspector) {
            size += radar->systemInspector->memoryUsage;
        }
    } else {
        size += radar->radarRelay->memoryUsage;
    }
    size += radar->healthLogger->memoryUsage;
    size += radar->sweepEngine->memoryUsage;
    size += radar->rawDataRecorder->memoryUsage;
    size += radar->hostMonitor->memoryUsage;
    return size;
}

static void *systemInspectorRunLoop(void *in) {
    RKSimpleEngine *engine = (RKSimpleEngine *)in;
    
    int j, k, s;
    
    bool shown = false;
    
    engine->state |= RKEngineStateWantActive;
    engine->state ^= RKEngineStateActivating;

    RKRadar *radar = (RKRadar *)engine->userResource;

    RKLog("%s Started.   radar = %s\n", engine->name, radar->desc.name);
    
    engine->state ^= RKEngineStateActive;

    double dt;
    uint32_t index;

    RKConfig *config;
    RKHealth *health;
    RKPosition *position0 = NULL, *position1 = NULL;
    RKPulse *pulse0, *pulse1;
    RKRay *ray0, *ray1;
    double positionRate = 0.0, pulseRate = 0.0, rayRate = 0.0;
    double dxduPosition, dxduPulse;

    uint32_t tweetaIndex = radar->desc.initFlags & RKInitFlagSignalProcessor ? radar->healthNodes[RKHealthNodeTweeta].index : 0;

    bool transceiverOkay, pedestalOkay, healthOkay, networkOkay, anyCritical;
    RKStatusEnum networkEnum, transceiverEnum, pedestalEnum, healthEnum;
    char FFTPlanUsage[RKStatusStringLength];
    RKName criticalKey, criticalValue;
    int criticalCount = 0;

    while (engine->state & RKEngineStateWantActive) {
        engine->state |= RKEngineStateSleep1;
        s = 0;
        do {
            if (engine->verbose > 2) {
                RKLog("%s Sleeping %.1f s\n", engine->name, s * 0.05f);
            }
            usleep(25000);
            //RKLog("Warning. radar->status[0].flag = %d   .i = %d\n", radar->status[0].flag, (int)radar->status[0].i);
        } while (++s < 10 && engine->state & RKEngineStateWantActive);
        engine->state ^= RKEngineStateSleep1;
        // Put together a system status
        RKStatus *status = RKGetVacantStatus(radar);
        radar->memoryUsage = RKGetRadarMemoryUsage(radar);
        status->memoryUsage = radar->memoryUsage;
        if (radar->pulseEngine->state & RKEngineStateActive) {
            status->pulseMonitorLag = radar->pulseEngine->lag * 100 / radar->desc.pulseBufferDepth;
            for (k = 0; k < MIN(RKProcessorStatusPulseCoreCount, radar->pulseEngine->coreCount); k++) {
                status->pulseCoreLags[k] = (uint8_t)(99.49f * radar->pulseEngine->workers[k].lag);
                status->pulseCoreUsage[k] = (uint8_t)(99.49f * radar->pulseEngine->workers[k].dutyCycle);
            }
        }
        if (radar->momentEngine->state & RKEngineStateActive) {
            status->rayMonitorLag = radar->momentEngine->lag * 100 / radar->desc.rayBufferDepth;
            for (k = 0; k < MIN(RKProcessorStatusRayCoreCount, radar->momentEngine->coreCount); k++) {
                status->rayCoreLags[k] = (uint8_t)(99.49f * radar->momentEngine->workers[k].lag);
                status->rayCoreUsage[k] = (uint8_t)(99.49f * radar->momentEngine->workers[k].dutyCycle);
            }
        }
        status->recorderLag = radar->rawDataRecorder->lag;
        RKSetStatusReady(radar, status);
        // Show something on the screen when we rotate back to configIndex 6
        if (radar->configIndex == 6) {
            if (!shown) {
                long mem = RKGetMemoryUsage();
                mem *= 1024;
                RKLog("%s %s B   %s B\n", engine->name,
                      RKVariableInString("memoryUsage", &radar->memoryUsage, RKValueTypeSize),
                      RKVariableInString("rusage", &mem, RKValueTypeLong));
                if (radar->desc.initFlags & RKInitFlagPulsePositionCombiner) {
                    dxduPosition = 1.0 / radar->positionClock->dx;
                    dxduPulse = 1.0 / radar->pulseClock->dx;
                    RKLog(">%s %s   %s", engine->name,
                          RKVariableInString("dxduPosition", &dxduPosition, RKValueTypeDouble),
                          RKVariableInString("dxduPulse", &dxduPulse, RKValueTypeDouble));
                }
                RKLog(">%s %s Hz   %s Hz   %s Hz\n",
                      engine->name,
                      RKVariableInString("positionRate", &positionRate, RKValueTypeDouble),
                      RKVariableInString("pulseRate", &pulseRate, RKValueTypeDouble),
                      RKVariableInString("rayRate", &rayRate, RKValueTypeDouble));
                shown = true;
            }
        } else {
            shown = false;
        }

        // Derive the acquisition rate of position, pulse and ray
        // Note, each acquisition rate is computed with its own timeval since they come in bursts, we may get no update in an iteration
        // Basic idea:
        //  - Get the index that is about 1 / N buffer depth behind the current index
        //  - Get the position / pulse / ray of that index
        //  - Get another index that is about another 1 / N buffer depth behind the previous index
        //  - Get the position / pulse / ray of that index
        //  - Compute the number of samples (buffer depth / N) over the period (delta t)

        // Position
        if (radar->positions) {
            index = RKPreviousNModuloS(radar->positionIndex, radar->desc.positionBufferDepth / 8, radar->desc.positionBufferDepth);
            position0 = &radar->positions[index];
            index = RKPreviousNModuloS(index, radar->desc.positionBufferDepth / 8, radar->desc.positionBufferDepth);
            position1 = &radar->positions[index];
            dt = position0->timeDouble - position1->timeDouble;
            if (dt) {
                positionRate = (double)(radar->desc.positionBufferDepth / 8) / dt;
            }
        }

        // Pulse
        if (radar->pulses) {
            index = RKPreviousNModuloS(radar->pulseIndex, radar->desc.pulseBufferDepth / 4, radar->desc.pulseBufferDepth);
            pulse0 = RKGetPulseFromBuffer(radar->pulses, index);
            index = RKPreviousNModuloS(index, radar->desc.pulseBufferDepth / 4, radar->desc.pulseBufferDepth);
            pulse1 = RKGetPulseFromBuffer(radar->pulses, index);
            dt = pulse0->header.timeDouble - pulse1->header.timeDouble;
            if (dt) {
                pulseRate = (double)(radar->desc.pulseBufferDepth / 4) / dt;
            }
        }

        // Ray
        if (radar->rays) {
            index = RKPreviousNModuloS(radar->rayIndex, radar->desc.rayBufferDepth / 8, radar->desc.rayBufferDepth);
            ray0 = RKGetRayFromBuffer(radar->rays, index);
            index = RKPreviousNModuloS(index, radar->desc.rayBufferDepth / 8, radar->desc.rayBufferDepth);
            ray1 = RKGetRayFromBuffer(radar->rays, index);
            dt = ray0->header.startTimeDouble - ray1->header.startTimeDouble;
            if (dt) {
                rayRate = (double)(radar->desc.rayBufferDepth / 8) / dt;
            }
        }

        // Only do this if the radar has a pulse position combiner
        if (radar->positions && radar->desc.initFlags & RKInitFlagPulsePositionCombiner) {
            pedestalOkay = positionRate == 0.0f ? false : true;
            // Position active / standby
            health = RKGetLatestHealthOfNode(radar, RKHealthNodePedestal);
            if (RKFindCondition(health->string, RKStatusEnumTooHigh, false, NULL, NULL) ||
                RKFindCondition(health->string, RKStatusEnumHigh, false, NULL, NULL)) {
                pedestalEnum = RKStatusEnumStandby;
            } else {
                if (RKGetMinorSectorInDegrees(position0->azimuthDegrees, position1->azimuthDegrees) > 0.1f ||
                    RKGetMinorSectorInDegrees(position0->elevationDegrees, position1->elevationDegrees) > 0.1f) {
                    pedestalEnum = RKStatusEnumActive;
                } else {
                    pedestalEnum = RKStatusEnumStandby;
                }
            }
        } else {
            pedestalEnum = RKStatusEnumInvalid;
        }
        
        // Only do this if the radar is a signal processor
        if (radar->desc.initFlags & RKInitFlagSignalProcessor) {
            // General Health
            transceiverOkay = pulseRate == 0.0f ? false : true;
            pedestalOkay = true;
            healthOkay = radar->healthRelay ? (tweetaIndex == radar->healthNodes[RKHealthNodeTweeta].index ? false : true) : false;
            networkOkay = radar->hostMonitor->allReachable ? true : false;
            networkEnum =
            radar->hostMonitor->allReachable ? RKStatusEnumNormal :
            (radar->hostMonitor->anyReachable ? RKStatusEnumStandby :
             (radar->hostMonitor->allKnown ? RKStatusEnumFault : RKStatusEnumUnknown));

            // Transceiver health
            health = RKGetLatestHealthOfNode(radar, RKHealthNodeTransceiver);
            if (RKFindCondition(health->string, RKStatusEnumTooHigh, false, NULL, NULL) ||
                RKFindCondition(health->string, RKStatusEnumHigh, false, NULL, NULL)) {
                transceiverEnum = RKStatusEnumStandby;
            } else {
                transceiverEnum = RKStatusEnumNormal;
            }

            // Tweeta health
            health = RKGetLatestHealthOfNode(radar, RKHealthNodeTweeta);
            if (RKFindCondition(health->string, RKStatusEnumTooHigh, false, NULL, NULL) ||
                RKFindCondition(health->string, RKStatusEnumHigh, false, NULL, NULL)) {
                healthEnum = RKStatusEnumStandby;
            } else {
                healthEnum = RKStatusEnumNormal;
            }

            // Report a health status
            health = RKGetVacantHealth(radar, RKHealthNodeRadarKit);
            if (health) {
                k = sprintf(FFTPlanUsage, "{");
                for (j = 0; j < radar->fftModule->count; j++) {
                    k += sprintf(FFTPlanUsage + k, "%s\"%d\":%d", j > 0 ? "," : "",
                                 radar->fftModule->plans[j].size,
                                 radar->fftModule->plans[j].count);
                }
                k += sprintf(FFTPlanUsage + k, "}");
                if (k > RKStatusStringLength * 3 / 4) {
                    RKLog("Warning. Too little head room in FFTPlanUsage (%d / %d).\n", k, RKStatusStringLength);
                }
                config = RKGetLatestConfig(radar);
                sprintf(health->string, "{"
                        "\"Transceiver\":{\"Value\":%s,\"Enum\":%d}, "
                        "\"Pedestal\":{\"Value\":%s,\"Enum\":%d}, "
                        "\"Health Relay\":{\"Value\":%s,\"Enum\":%d}, "
                        "\"Internet\":{\"Value\":%s,\"Enum\":%d}, "
                        "\"Recorder\":{\"Value\":%s,\"Enum\":%d}, "
                        "\"Ring Filter\":{\"Value\":%s,\"Enum\":%d}, "
                        "\"Processors\":{\"Value\":true,\"Enum\":0}, "
                        "\"Measured PRF\":{\"Value\":\"%s Hz\",\"Enum\":%d}, "
                        "\"Noise\":[%.3f,%.3f], "
                        "\"Position Rate\":{\"Value\":\"%s Hz\",\"Enum\":0}, "
                        "\"rayRate\":%.3f, "
                        "\"FFTPlanUsage\":%s"
                        "}",
                        transceiverOkay ? "true" : "false", transceiverOkay ? transceiverEnum : RKStatusEnumFault,
                        pedestalOkay ? "true" : "false", pedestalOkay ? pedestalEnum : RKStatusEnumFault,
                        healthOkay ? "true" : "false", radar->healthRelay ? (healthOkay ? healthEnum : RKStatusEnumFault) : RKStatusEnumNotWired,
                        networkOkay ? "true" : "false", networkEnum,
                        radar->rawDataRecorder->record ? "true" : "false", radar->rawDataRecorder->record ? RKStatusEnumNormal : RKStatusEnumStandby,
                        radar->pulseRingFilterEngine->useFilter ? "true" : "false", radar->pulseRingFilterEngine->useFilter ? RKStatusEnumNormal : RKStatusEnumStandby,
                        RKIntegerToCommaStyleString((long)round(pulseRate)), fabs(pulseRate - 1.0f / config->prt[0]) * config->prt[0] < 0.1f ? RKStatusEnumNormal : RKStatusEnumStandby,
                        config->noise[0], config->noise[1],
                        RKIntegerToCommaStyleString((long)round(positionRate)), rayRate,
                        FFTPlanUsage
                        );
                RKSetHealthReady(radar, health);
            }

            // Get the latest consolidated health
            health = RKGetLatestHealth(radar);
            anyCritical = RKAnyCritical(health->string, false, criticalKey, criticalValue);
            if (anyCritical) {
                RKLog("Warning. %s is in critical condition (value = %s, count = %d).\n", criticalKey, criticalValue, criticalCount);
                if (criticalCount++ >= 20) {
                    criticalCount = 0;
                    if (pedestalEnum == RKStatusEnumActive) {
                        RKLog("Warning. Suspending radar due to critical %s ...\n", criticalKey);
                        radar->masterControllerExec(radar->masterController, "z", NULL);
                    }
                }
            } else {
                criticalCount = 0;
            }

            for (int i = 0; i < radar->desc.rayBufferDepth; i++) {
                if (radar->momentEngine->momentSource[i].origin >= radar->desc.pulseBufferDepth ||
                    radar->momentEngine->momentSource[i].length > radar->desc.pulseBufferDepth ||
                    radar->momentEngine->momentSource[i].modulo > radar->desc.pulseBufferDepth) {
                    printf("Overflow: --> momentSource[%d] = %d / %d / %d (%d)\n", i,
                           radar->momentEngine->momentSource[i].origin, radar->momentEngine->momentSource[i].length, radar->momentEngine->momentSource[i].modulo, radar->desc.pulseBufferDepth);
                }
            }

        } // if (radar->desc.initFlags & RKInitFlagSignalProcessor) ...
        // Update the indices and time
        tweetaIndex = radar->healthNodes[RKHealthNodeTweeta].index;
    }
    engine->state ^= RKEngineStateActive;
    return NULL;
}

RKSimpleEngine *RKSystemInspector(RKRadar *radar) {
    RKSimpleEngine *engine = (RKSimpleEngine *)malloc(sizeof(RKSimpleEngine));
    if (engine == NULL) {
        RKLog("Error allocating an engine monitor.\n");
        return NULL;
    }
    memset(engine, 0, sizeof(RKSimpleEngine));
    sprintf(engine->name, "%s<SystemInspector>%s",
            rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorEngineMonitor) : "",
            rkGlobalParameters.showColor ? RKNoColor : "");
    engine->state = RKEngineStateAllocated | RKEngineStateProperlyWired | RKEngineStateActivating;
    engine->verbose = radar->desc.initFlags & RKInitFlagVerbose ? 1 : 0;
    engine->memoryUsage = sizeof(RKSimpleEngine);
    engine->userResource = radar;
    RKLog("%s Starting ...\n", engine->name);
    if (pthread_create(&engine->tid, NULL, systemInspectorRunLoop, engine)) {
        RKLog("%s Error creating engine monitor.\n", engine->name);
        free(engine);
        return NULL;
    }
    while (!(engine->state & RKEngineStateWantActive)) {
        usleep(100000);
    }
    return engine;
}

#pragma mark - Helper Functions

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

void *masterControllerExecuteInBackground(void *in) {
    RKRadarCommand *radarCommand = (RKRadarCommand *)in;
    if (radarCommand->radar->masterController) {
        radarCommand->radar->masterControllerExec(radarCommand->radar->masterController, radarCommand->command, NULL);
    }
    return NULL;
}

#pragma mark - Life Cycle

//
// Initialize a radar object
// Input:
//     RKRadarDesc desc - a description of the properties. See its definition in RKTypes.h
// output:
//     RKRadar *radar - an "object" radar. This is a reference of a radar system.
//
RKRadar *RKInitWithDesc(const RKRadarDesc desc) {
    RKRadar *radar;
    size_t bytes;
    int i, k;

    RKSetUseDailyLog(true);
    if (strlen(desc.dataPath)) {
        RKSetRootFolder(desc.dataPath);
    } else {
        RKSetRootFolder(RKDefaultDataPath);
    }

    RKLog("Initializing ... 0x%08x %s", desc.initFlags, desc.dataPath);

    // Allocate self
    bytes = sizeof(RKRadar);
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&radar, RKSIMDAlignSize, bytes))
    memset(radar, 0, bytes);

    // Get the number of CPUs
    radar->processorCount = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
    RKLog("Number of online CPUs = %ld (HT = 2)\n", radar->processorCount);
    if (radar->processorCount <= 1) {
        RKLog("Assume Number of CPUs = %d was not correctly reported. Override with 4.\n", radar->processorCount);
        radar->processorCount = 4;
    }

    // Set some non-zero variables
    sprintf(radar->name, "%s<MasterController>%s",
            rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(13) : "", rkGlobalParameters.showColor ? RKNoColor : "");
    radar->memoryUsage += bytes;

    // Copy over the input flags and constaint the capacity and depth to hard-coded limits
    radar->desc = desc;
    if (radar->desc.statusBufferDepth > RKBufferSSlotCount) {
        radar->desc.statusBufferDepth = RKBufferSSlotCount;
        RKLog("Info. Status buffer clamped to %s\n", RKIntegerToCommaStyleString(radar->desc.statusBufferDepth));
    } else if (radar->desc.statusBufferDepth == 0) {
        radar->desc.statusBufferDepth = 10;
    }
    if (radar->desc.configBufferDepth > RKBufferCSlotCount) {
        radar->desc.configBufferDepth = RKBufferCSlotCount;
        RKLog("Info. Config buffer clamped to %s\n", RKIntegerToCommaStyleString(radar->desc.configBufferDepth));
    } else if (radar->desc.configBufferDepth == 0) {
        radar->desc.configBufferDepth = 10;
    }
    if (radar->desc.healthBufferDepth > RKBufferHSlotCount) {
        radar->desc.healthBufferDepth = RKBufferHSlotCount;
        RKLog("Info. Health buffer clamped to %s\n", RKIntegerToCommaStyleString(radar->desc.healthBufferDepth));
    } else if (radar->desc.healthBufferDepth == 0) {
        radar->desc.healthBufferDepth = 40;
    }
    if (radar->desc.positionBufferDepth > RKBufferPSlotCount) {
        radar->desc.positionBufferDepth = RKBufferPSlotCount;
        RKLog("Info. Position buffer clamped to %s\n", radar->desc.positionBufferDepth);
    } else if (radar->desc.positionBufferDepth == 0) {
        radar->desc.positionBufferDepth = 500;
    }
    if (radar->desc.pulseBufferDepth > RKBuffer0SlotCount) {
        radar->desc.pulseBufferDepth = RKBuffer0SlotCount;
        RKLog("Info. Pulse buffer clamped to %s\n", RKIntegerToCommaStyleString(radar->desc.pulseBufferDepth));
    } else if (radar->desc.pulseBufferDepth == 0) {
        radar->desc.pulseBufferDepth = 2000;
    }
    if (radar->desc.rayBufferDepth > RKBuffer2SlotCount) {
        radar->desc.rayBufferDepth = RKBuffer2SlotCount;
        RKLog("Info. Ray buffer clamped to %s\n", RKIntegerToCommaStyleString(radar->desc.rayBufferDepth));
    } else if (radar->desc.rayBufferDepth == 0) {
        radar->desc.rayBufferDepth = 720;
    }
    if (radar->desc.productBufferDepth > RKBuffer3SlotCount) {
        radar->desc.productBufferDepth = RKBuffer3SlotCount;
        RKLog("Info. Product buffer clamped to %s\n", radar->desc.productBufferDepth);
    } else if (radar->desc.productBufferDepth == 0) {
        radar->desc.productBufferDepth = 20;
    }
    if (radar->desc.pulseCapacity > RKMaximumGateCount) {
        radar->desc.pulseCapacity = RKMaximumGateCount;
        RKLog("Info. Pulse capacity clamped to %s\n", RKIntegerToCommaStyleString(radar->desc.pulseCapacity));
    } else if (radar->desc.pulseCapacity == 0) {
        radar->desc.pulseCapacity = 1;
    }
    uint32_t stride = radar->desc.pulseToRayRatio * RKSIMDAlignSize;
    radar->desc.pulseCapacity = ((radar->desc.pulseCapacity * sizeof(RKFloat) + stride - 1) / stride) * stride / sizeof(RKFloat);
    if (radar->desc.pulseCapacity != desc.pulseCapacity && radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("Info. Pulse capacity changed from %s to %s (stride = %d RKFloats, %d B)\n",
              RKIntegerToCommaStyleString(desc.pulseCapacity),
              RKIntegerToCommaStyleString(radar->desc.pulseCapacity),
              stride / sizeof(RKFloat), stride);
    }
    if (radar->desc.healthNodeCount == 0 || radar->desc.healthNodeCount > RKHealthNodeCount) {
        radar->desc.healthNodeCount = RKHealthNodeCount;
    }
    if (radar->desc.controlCapacity > RKMaximumControlCount) {
        radar->desc.controlCapacity = RKMaximumControlCount;
        RKLog("Info. Control count limited to %s\n", radar->desc.controlCapacity);
    } else if (radar->desc.controlCapacity == 0) {
        radar->desc.controlCapacity = 64;
    }
    if (radar->desc.waveformCalibrationCapacity > RKMaximumWaveformCalibrationCount) {
        radar->desc.waveformCalibrationCapacity = RKMaximumWaveformCalibrationCount;
        RKLog("Info. Waveform calibration count limited to %s\n", radar->desc.controlCapacity);
    } else if (radar->desc.waveformCalibrationCapacity == 0) {
        radar->desc.waveformCalibrationCapacity = 64;
    }

    // Read in preference file here, override some values
    if (!strlen(radar->desc.name)) {
        sprintf(radar->desc.name, "Radar");
    }
    if (!strlen(radar->desc.filePrefix)) {
        sprintf(radar->desc.filePrefix, "RK");
    }
    RKLog("Radar name = '%s'  prefix = '%s'", radar->desc.name, radar->desc.filePrefix);
    if (strlen(desc.dataPath) == 0) {
        sprintf(radar->desc.dataPath, RKDefaultDataPath);
    }

    // -------------------------------------------------- Buffers --------------------------------------------------
    
    // Status buffer
    if (radar->desc.initFlags & RKInitFlagAllocStatusBuffer) {
        bytes = radar->desc.statusBufferDepth * sizeof(RKStatus);
        if (bytes == 0) {
            RKLog("Error. Zero storage for status buffer?\n");
            exit(EXIT_FAILURE);
        }
        radar->status = (RKStatus *)malloc(bytes);
        if (radar->status == NULL) {
            RKLog("Error. Unable to allocate memory for status buffer");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.statusBufferSize = bytes;
        RKLog("Status buffer occupies %s B  (%s sets)\n",
              RKUIntegerToCommaStyleString(radar->desc.statusBufferSize),
              RKIntegerToCommaStyleString(radar->desc.statusBufferDepth));
        memset(radar->status, 0, bytes);
        for (i = 0; i < radar->desc.statusBufferDepth; i++) {
            radar->status[i].i = -(uint64_t)radar->desc.statusBufferDepth + i;
        }
        radar->state |= RKRadarStateStatusBufferAllocated;
    }

    // Config buffer
    if (radar->desc.initFlags & RKInitFlagAllocConfigBuffer) {
        bytes = radar->desc.configBufferDepth * sizeof(RKConfig);
        if (bytes == 0) {
            RKLog("Error. Zero storage for config buffer?\n");
            exit(EXIT_FAILURE);
        }
        radar->configs = (RKConfig *)malloc(bytes);
        if (radar->configs == NULL) {
            RKLog("Error. Unable to allocate memory for config buffer");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.configBufferSize = bytes;
        RKLog("Config buffer occupies %s B  (%s sets)\n",
              RKUIntegerToCommaStyleString(radar->desc.configBufferSize),
              RKIntegerToCommaStyleString(radar->desc.configBufferDepth));
        memset(radar->configs, 0, bytes);
        for (i = 0; i < radar->desc.configBufferDepth; i++) {
            radar->configs[i].i = -(uint64_t)radar->desc.configBufferDepth + i + 1000;
        }
        radar->state |= RKRadarStateConfigBufferAllocated;
    }
    
    // Health buffer
    if (radar->desc.initFlags & RKInitFlagAllocHealthBuffer) {
        bytes = radar->desc.healthBufferDepth * sizeof(RKHealth);
        if (bytes == 0) {
            RKLog("Error. Zero storage for health buffer?\n");
            exit(EXIT_FAILURE);
        }
        radar->healths = (RKHealth *)malloc(bytes);
        if (radar->healths == NULL) {
            RKLog("Error. Unable to allocate memory for health status");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.healthBufferSize = bytes;
        RKLog("Health buffer occupies %s B  (%s sets)\n",
              RKUIntegerToCommaStyleString(radar->desc.healthBufferSize),
              RKIntegerToCommaStyleString(radar->desc.healthBufferDepth));
        memset(radar->healths, 0, bytes);
        for (i = 0; i < radar->desc.healthBufferDepth; i++) {
            radar->healths[i].i = -(uint64_t)radar->desc.healthBufferDepth + i;
        }
        radar->state |= RKRadarStateHealthBufferAllocated;
    }
    
    // Health nodes
    if (radar->desc.initFlags & RKInitFlagAllocHealthNodes) {
        bytes = radar->desc.healthNodeCount * sizeof(RKNodalHealth);
        if (bytes == 0) {
            RKLog("Error. Zero storage for health nodes?\n");
            exit(EXIT_FAILURE);
        }
        radar->healthNodes = (RKNodalHealth *)malloc(bytes);
        memset(radar->healthNodes, 0, bytes);
        radar->memoryUsage += bytes;
        radar->desc.healthNodeBufferSize = bytes;
        bytes = radar->desc.healthBufferDepth * sizeof(RKHealth);
        if (bytes == 0) {
            RKLog("Error. Zero storage for health node buffers?\n");
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < radar->desc.healthNodeCount; i++) {
            radar->healthNodes[i].healths = (RKHealth *)malloc(bytes);
            memset(radar->healthNodes[i].healths, 0, bytes);
        }
        radar->memoryUsage += radar->desc.healthNodeCount * bytes;
        radar->desc.healthNodeBufferSize += radar->desc.healthNodeCount * bytes;
        RKLog("Nodal-health buffers occupy %s B  (%d nodes x %s sets)\n",
              RKUIntegerToCommaStyleString(radar->desc.healthNodeBufferSize),
              radar->desc.healthNodeCount,
              RKIntegerToCommaStyleString(radar->desc.healthBufferDepth));
        for (k = 0; k < radar->desc.healthNodeCount; k++) {
            for (i = 0; i < radar->desc.healthBufferDepth; i++) {
                radar->healthNodes[k].healths[i].i = -(uint64_t)radar->desc.healthBufferDepth + i;
            }
        }
        radar->state |= RKRadarStateHealthNodesAllocated;
    }
    
    // Position buffer
    if (radar->desc.initFlags & RKInitFlagAllocPositionBuffer) {
        bytes = radar->desc.positionBufferDepth * sizeof(RKPosition);
        if (bytes == 0) {
            RKLog("Error. Zero storage for position buffer?\n");
            exit(EXIT_FAILURE);
        }
        radar->positions = (RKPosition *)malloc(bytes);
        memset(radar->positions, 0, bytes);
        if (radar->positions == NULL) {
            RKLog("Error. Unable to allocate memory for positions.");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.positionBufferSize = bytes;
        RKLog("Position buffer occupies %s B  (%s positions)\n",
              RKIntegerToCommaStyleString(radar->desc.positionBufferSize),
              RKIntegerToCommaStyleString(radar->desc.positionBufferDepth));
        for (i = 0; i < radar->desc.positionBufferDepth; i++) {
            radar->positions[i].i = -(uint64_t)radar->desc.positionBufferDepth + i;
        }
        radar->state |= RKRadarStatePositionBufferAllocated;
    }

    // Pulse (IQ) buffer
    if (radar->desc.initFlags & RKInitFlagAllocRawIQBuffer) {
        bytes = RKPulseBufferAlloc(&radar->pulses, radar->desc.pulseCapacity, radar->desc.pulseBufferDepth);
        if (bytes == 0 || radar->pulses == NULL) {
            RKLog("Error. Unable to allocate memory for I/Q pulses.\n");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.pulseBufferSize = bytes;
        RKLog("Level I buffer occupies %s B  (%s pulses x %s gates)\n",
              RKUIntegerToCommaStyleString(radar->desc.pulseBufferSize),
              RKIntegerToCommaStyleString(radar->desc.pulseBufferDepth),
              RKIntegerToCommaStyleString(radar->desc.pulseCapacity));
        for (i = 0; i < radar->desc.pulseBufferDepth; i++) {
            RKPulse *pulse = RKGetPulseFromBuffer(radar->pulses, i);
            size_t offset = (size_t)pulse->data - (size_t)pulse;
            if (offset != RKPulseHeaderPaddedSize) {
                printf("Unexpected offset = %d != %d\n", (int)offset, RKPulseHeaderPaddedSize);
            }
        }
        radar->state |= RKRadarStateRawIQBufferAllocated;
    }

    // Ray (moment) and product buffers
    if (radar->desc.initFlags & RKInitFlagAllocMomentBuffer) {
        k = ((int)ceilf((float)(radar->desc.pulseCapacity / radar->desc.pulseToRayRatio) * sizeof(RKFloat) / (float)RKSIMDAlignSize)) * RKSIMDAlignSize / sizeof(RKFloat);
        bytes = RKRayBufferAlloc(&radar->rays, k, radar->desc.rayBufferDepth);
        if (bytes == 0 || radar->rays == NULL) {
            RKLog("Error. Unable to allocate memory for rays.\n");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.rayBufferSize = bytes;
        RKLog("Level II buffer occupies %s B  (%s rays x %d moments of %s gates)\n",
              RKUIntegerToCommaStyleString(bytes),
              RKIntegerToCommaStyleString(radar->desc.rayBufferDepth),
              RKBaseMomentCount,
              RKIntegerToCommaStyleString(k));
        radar->state |= RKRadarStateRayBufferAllocated;

        bytes = RKProductBufferAlloc(&radar->products, radar->desc.productBufferDepth, RKMaximumRaysPerSweep, 100);
        if (bytes == 0 || radar->products == NULL) {
            RKLog("Error. Unable to allocate memory for products.\n");
            exit(EXIT_FAILURE);
        }
        radar->memoryUsage += bytes;
        radar->desc.productBufferSize = bytes;
        RKLog("Level III buffer occupies %s B  (%s products x %s cells)\n",
              RKUIntegerToCommaStyleString(radar->desc.productBufferSize),
              RKIntegerToCommaStyleString(radar->desc.productBufferDepth),
              RKIntegerToCommaStyleString(radar->products[0].capacity));
        radar->state |= RKRadarStateProductBufferAllocated;
    }

    // Waveform calibrations
    if (radar->desc.waveformCalibrationCapacity) {
        bytes = radar->desc.waveformCalibrationCapacity * sizeof(RKWaveformCalibration);
        if (bytes == 0) {
            RKLog("Error. Zero storage for waveform calibrations?\n");
            exit(EXIT_FAILURE);
        }
        radar->waveformCalibrations = (RKWaveformCalibration *)malloc(bytes);
        if (radar->waveformCalibrations == NULL) {
            RKLog("Error. Unable to allocate memory for waveform calibrations.\n");
            exit(EXIT_FAILURE);
        }
        memset(radar->waveformCalibrations, 0, bytes);
        for (i = 0; i < radar->desc.waveformCalibrationCapacity; i++) {
            RKWaveformCalibration *calibration = &radar->waveformCalibrations[i];
            calibration->uid = -(uint32_t)radar->desc.waveformCalibrationCapacity + i;
        }
        RKLog("Waveform calibrations occupy %s B (%s units)",
              RKIntegerToCommaStyleString(bytes),
              RKIntegerToCommaStyleString(radar->desc.waveformCalibrationCapacity));
        radar->memoryUsage += bytes;
        radar->state |= RKRadarStateWaveformCalibrationsAllocated;
    }

    // Controls
    if (radar->desc.controlCapacity) {
        bytes = radar->desc.controlCapacity * sizeof(RKControl);
        if (bytes == 0) {
            RKLog("Error. Zero storage for controls?\n");
            exit(EXIT_FAILURE);
        }
        radar->controls = (RKControl *)malloc(bytes);
        if (radar->controls == NULL) {
            RKLog("Error. Unable to allocate memory for controls.\n");
            exit(EXIT_FAILURE);
        }
        memset(radar->controls, 0, bytes);
        for (i = 0; i < radar->desc.controlCapacity; i++) {
            RKControl *control = &radar->controls[i];
            control->uid = -(uint32_t)radar->desc.controlCapacity + i;
        }
        RKLog("Controls occupy %s B (%s units)",
              RKIntegerToCommaStyleString(bytes),
              RKIntegerToCommaStyleString(radar->desc.controlCapacity));
        radar->memoryUsage += bytes;
        radar->state |= RKRadarStateControlsAllocated;
    }
    
    // -------------------------------------------------- Engines --------------------------------------------------

    // File manager
    radar->fileManager = RKFileManagerInit();
    RKFileManagerSetInputOutputBuffer(radar->fileManager, &radar->desc);
    RKLog("File manager occupies %s B\n", RKIntegerToCommaStyleString(radar->fileManager->memoryUsage));
    radar->memoryUsage += radar->fileManager->memoryUsage;
    radar->state |= RKRadarStateFileManagerInitialized;

    // Pulse position combiner marries pulse and position data
    RKName tmpName;
    if (radar->desc.initFlags & RKInitFlagPulsePositionCombiner) {
        // Clocks
        if (radar->desc.pulseSmoothFactor > 0) {
            radar->pulseClock = RKClockInitWithSize(radar->desc.pulseSmoothFactor + 1000, radar->desc.pulseSmoothFactor);
        } else {
            radar->pulseClock = RKClockInit();
        }
        if (radar->desc.pulseTicsPerSecond > 0) {
            RKClockSetDuDx(radar->pulseClock, (double)radar->desc.pulseTicsPerSecond);
        }
        sprintf(tmpName, "%s<TransceiverTime>%s",
                rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorClock) : "", rkGlobalParameters.showColor ? RKNoColor : "");
        RKClockSetName(radar->pulseClock, tmpName);
        radar->memoryUsage += sizeof(RKClock);
        
        if (radar->desc.positionSmoothFactor > 0) {
            radar->positionClock = RKClockInitWithSize(radar->desc.positionSmoothFactor + 1000, radar->desc.positionSmoothFactor);
        } else {
            radar->positionClock = RKClockInit();
        }
        if (radar->desc.positionTicsPerSecond > 0) {
            RKClockSetDuDx(radar->positionClock, (double)radar->desc.positionTicsPerSecond);
        }
        sprintf(tmpName, "%s<AimPedestalTime>%s",
                rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorClock) : "", rkGlobalParameters.showColor ? RKNoColor : "");
        RKClockSetName(radar->positionClock, tmpName);
        RKClockSetOffset(radar->positionClock, -radar->desc.positionLatency);
        RKLog("Position latency = %.3e s\n", radar->desc.positionLatency);
        radar->memoryUsage += sizeof(RKClock);

        // Position engine
        radar->positionEngine = RKPositionEngineInit();
        RKPositionEngineSetInputOutputBuffers(radar->positionEngine, &radar->desc,
                                              radar->positions, &radar->positionIndex,
                                              radar->configs, &radar->configIndex,
                                              radar->pulses, &radar->pulseIndex);
        radar->memoryUsage += radar->positionEngine->memoryUsage;
        radar->state |= RKRadarStatePositionEngineInitialized;
    }

    // Signal processor performs pulse compression, calculates moment, etc.
    if (radar->desc.initFlags & RKInitFlagSignalProcessor) {
        // FFT module
        radar->fftModule = RKFFTModuleInit(radar->desc.pulseCapacity, radar->desc.initFlags & RKInitFlagVerbose ? 1 : 0);
        radar->memoryUsage += sizeof(RKFFTModule);
        
        // Pulse compression engine
        radar->pulseEngine = RKPulseEngineInit();
        RKPulseEngineSetInputOutputBuffers(radar->pulseEngine, &radar->desc,
                                                      radar->configs, &radar->configIndex,
                                                      radar->pulses, &radar->pulseIndex);
        RKPulseEngineSetFFTModule(radar->pulseEngine, radar->fftModule);
        radar->memoryUsage += radar->pulseEngine->memoryUsage;
        radar->state |= RKRadarStatePulseCompressionEngineInitialized;

        // Pulse ring filter engine
        radar->pulseRingFilterEngine = RKPulseRingFilterEngineInit();
        RKPulseRingFilterEngineSetInputOutputBuffers(radar->pulseRingFilterEngine, &radar->desc,
                                                     radar->configs, &radar->configIndex,
                                                     radar->pulses, &radar->pulseIndex);
        radar->memoryUsage += radar->pulseRingFilterEngine->memoryUsage;
        radar->state |= RKRadarStatePulseRingFilterEngineInitialized;

        // Moment engine
        radar->momentEngine = RKMomentEngineInit();
        RKMomentEngineSetInputOutputBuffers(radar->momentEngine, &radar->desc,
                                            radar->configs, &radar->configIndex,
                                            radar->pulses, &radar->pulseIndex,
                                            radar->rays, &radar->rayIndex);
        RKMomentEngineSetFFTModule(radar->momentEngine, radar->fftModule);
        radar->memoryUsage += radar->momentEngine->memoryUsage;
        radar->state |= RKRadarStateMomentEngineInitialized;

        // Health engine
        radar->healthEngine = RKHealthEngineInit();
        RKHealthEngineSetInputOutputBuffers(radar->healthEngine, &radar->desc, radar->healthNodes,
                                            radar->healths, &radar->healthIndex);
        radar->memoryUsage += radar->healthEngine->memoryUsage;
        radar->state |= RKRadarStateHealthEngineInitialized;
    } else {
        // Radar relay
        radar->radarRelay = RKRadarRelayInit();
        RKRadarRelaySetInputOutputBuffers(radar->radarRelay, &radar->desc, radar->fileManager,
                                          radar->status, &radar->statusIndex,
                                          radar->configs, &radar->configIndex,
                                          radar->healths, &radar->healthIndex,
                                          radar->pulses, &radar->pulseIndex,
                                          radar->rays, &radar->rayIndex);
        RKRadarRelaySetVerbose(radar->radarRelay, 2);
        radar->memoryUsage += radar->radarRelay->memoryUsage;
        radar->state |= RKRadarStateRadarRelayInitialized;
    }

    // Health logger
    radar->healthLogger = RKHealthLoggerInit();
    RKHealthLoggerSetInputOutputBuffers(radar->healthLogger, &radar->desc, radar->fileManager,
                                        radar->healths, &radar->healthIndex);
    radar->memoryUsage += radar->healthLogger->memoryUsage;
    radar->state |= RKRadarStateHealthLoggerInitialized;

    // Sweep engine
    radar->sweepEngine = RKSweepEngineInit();
    RKSweepEngineSetInputOutputBuffer(radar->sweepEngine, &radar->desc, radar->fileManager,
                                      radar->configs, &radar->configIndex,
                                      radar->rays, &radar->rayIndex,
                                      radar->products, &radar->productIndex);
    radar->memoryUsage += radar->sweepEngine->memoryUsage;
    radar->state |= RKRadarStateSweepEngineInitialized;

    // Raw data recorder
    radar->rawDataRecorder = RKRawDataRecorderInit();
    RKRawDataRecorderSetInputOutputBuffers(radar->rawDataRecorder, &radar->desc, radar->fileManager,
                                           radar->configs, &radar->configIndex,
                                           radar->pulses, &radar->pulseIndex);
    radar->memoryUsage += radar->rawDataRecorder->memoryUsage;
    radar->state |= RKRadarStateFileRecorderInitialized;

    // Host monitor
    radar->hostMonitor = RKHostMonitorInit();
    radar->memoryUsage += radar->hostMonitor->memoryUsage;
    radar->state |= RKRadarStateHostMonitorInitialized;

    // Other resources
    pthread_mutex_init(&radar->mutex, NULL);
    
    // Recording level 0 - moment and health only
    RKSetRecordingLevel(radar, 0);

    // Give the radar an impulse waveform
    RKSetWaveformToImpulse(radar);
    
    // Total memory usage
    RKLog("Radar initialized. Data buffers occupy %s%s B%s (%s GiB)\n",
          rkGlobalParameters.showColor ? "\033[4m" : "",
          RKIntegerToCommaStyleString(radar->memoryUsage),
          rkGlobalParameters.showColor ? "\033[24m" : "",
          RKFloatToCommaStyleString((double)radar->memoryUsage / 1073741824.0));

    radar->tic++;

    return radar;
}

RKRadar *RKInitQuiet(void) {
    RKRadarDesc desc;
    memset(&desc, 0, sizeof(RKRadarDesc));
    desc.initFlags = RKInitFlagAllocEverythingQuiet | RKInitFlagSignalProcessor;
    desc.pulseCapacity = 4096;
    desc.pulseToRayRatio = 1;
    desc.configBufferDepth = RKBufferCSlotCount;
    desc.healthBufferDepth = RKBufferHSlotCount;
    desc.positionBufferDepth = RKBufferPSlotCount;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

RKRadar *RKInitLean(void) {
    RKRadarDesc desc;
    memset(&desc, 0, sizeof(RKRadarDesc));
    desc.initFlags = RKInitFlagAllocEverything | RKInitFlagSignalProcessor;
    desc.pulseCapacity = 2048;
    desc.pulseToRayRatio = 1;
    desc.configBufferDepth = 10;
    desc.healthBufferDepth = 10;
    desc.positionBufferDepth = 500;
    desc.pulseBufferDepth = 5000;
    desc.rayBufferDepth = 1500;
    return RKInitWithDesc(desc);
}

RKRadar *RKInitMean(void) {
    RKRadarDesc desc;
    memset(&desc, 0, sizeof(RKRadarDesc));
    desc.initFlags = RKInitFlagAllocEverything | RKInitFlagSignalProcessor;
    desc.pulseCapacity = 8192;
    desc.pulseToRayRatio = 2;
    desc.configBufferDepth = RKBufferCSlotCount;
    desc.healthBufferDepth = RKBufferHSlotCount;
    desc.positionBufferDepth = RKBufferPSlotCount;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

//
// Initializes a radar object with all features
// Input:
//     None
// Output:
//     An RKRadar object
//
RKRadar *RKInitFull(void) {
    RKRadarDesc desc;
    memset(&desc, 0, sizeof(RKRadarDesc));
    desc.initFlags = RKInitFlagAllocEverything | RKInitFlagSignalProcessor;
    desc.pulseCapacity = RKMaximumGateCount;
    desc.pulseToRayRatio = 8;
    desc.configBufferDepth = RKBufferCSlotCount;
    desc.healthBufferDepth = RKBufferHSlotCount;
    desc.positionBufferDepth = RKBufferPSlotCount;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

//
// Initializes a radar object with all features
// Input:
//     None
// Output:
//     An RKRadar object
//
RKRadar *RKInit(void) {
    return RKInitLean();
}

//
// Initializes a radar object as a relay
// Input:
//     None
// Output:
//     An RKRadar object
//
RKRadar *RKInitAsRelay(void) {
    RKRadarDesc desc;
    memset(&desc, 0, sizeof(RKRadarDesc));
    desc.initFlags = RKInitFlagAllocRawIQBuffer | RKInitFlagAllocMomentBuffer;
    desc.pulseCapacity = RKMaximumGateCount;
    desc.pulseToRayRatio = 8;
    desc.configBufferDepth = RKBufferCSlotCount;
    desc.healthBufferDepth = RKBufferHSlotCount;
    desc.pulseBufferDepth = RKBuffer0SlotCount;
    desc.rayBufferDepth = RKBuffer2SlotCount;
    return RKInitWithDesc(desc);
}

//
// Free up the radar object
// Input:
//     None
// Output:
//     RKResultSuccess if no errors
//
int RKFree(RKRadar *radar) {
    int i;
    if (radar->active) {
        RKStop(radar);
    }
    // Various RadarKit engines
    if (radar->state & RKRadarStateFileManagerInitialized) {
        RKFileManagerFree(radar->fileManager);
    }
    if (radar->pulseClock) {
        RKClockFree(radar->pulseClock);
        radar->pulseClock = NULL;
    }
    if (radar->positionClock) {
        RKClockFree(radar->positionClock);
        radar->positionClock = NULL;
    }
    if (radar->fftModule) {
        RKFFTModuleFree(radar->fftModule);
        radar->fftModule = NULL;
    }
    if (radar->state & RKRadarStatePulseCompressionEngineInitialized) {
        RKPulseEngineFree(radar->pulseEngine);
        radar->pulseEngine = NULL;
    }
    if (radar->state & RKRadarStatePulseRingFilterEngineInitialized) {
        RKPulseRingFilterEngineFree(radar->pulseRingFilterEngine);
        radar->pulseRingFilterEngine = NULL;
    }
    if (radar->state & RKRadarStatePositionEngineInitialized) {
        RKPositionEngineFree(radar->positionEngine);
        radar->positionEngine = NULL;
    }
    if (radar->state & RKRadarStateMomentEngineInitialized) {
        RKMomentEngineFree(radar->momentEngine);
        radar->momentEngine = NULL;
    }
    if (radar->state & RKRadarStateHealthEngineInitialized) {
        RKHealthEngineFree(radar->healthEngine);
        radar->healthEngine = NULL;
    }
    if (radar->state & RKRadarStateRadarRelayInitialized) {
        RKRadarRelayFree(radar->radarRelay);
        radar->radarRelay = NULL;
    }
    if (radar->state & RKRadarStateHealthLoggerInitialized) {
        RKHealthLoggerFree(radar->healthLogger);
        radar->healthLogger = NULL;
    }
    if (radar->state & RKRadarStateSweepEngineInitialized) {
        RKSweepEngineFree(radar->sweepEngine);
        radar->sweepEngine = NULL;
    }
    if (radar->state & RKRadarStateFileRecorderInitialized) {
        RKRawDataRecorderFree(radar->rawDataRecorder);
        radar->rawDataRecorder = NULL;
    }
    // Transceiver, pedestal & health relay
    if (radar->pedestal) {
        radar->pedestalFree(radar->pedestal);
        radar->pedestal = NULL;
    }
    if (radar->transceiver) {
        radar->transceiverFree(radar->transceiver);
        radar->transceiver = NULL;
    }
    if (radar->healthRelay) {
        radar->healthRelayFree(radar->healthRelay);
        radar->healthRelay = NULL;
    }
    // Internal copies of things
    if (radar->waveform) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Freeing waveform '%s' with %d group%s ...\n",
                  radar->waveform->name, radar->waveform->count, radar->waveform->count > 1 ? "s" : "");
        }
        RKWaveformFree(radar->waveform);
        radar->waveform = NULL;
    }
    if (radar->filter) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Freeing filter ...\n", radar->waveform->name);
        }
        free(radar->filter);
        radar->filter = NULL;
    }
    // Buffers
    RKLog("Freeing radar '%s' ...\n", radar->desc.name);
    if (radar->state & RKRadarStateStatusBufferAllocated) {
        free(radar->status);
    }
    if (radar->state & RKRadarStateConfigBufferAllocated) {
        free(radar->configs);
    }
    if (radar->state & RKRadarStateHealthBufferAllocated) {
        free(radar->healths);
    }
    if (radar->state & RKRadarStateHealthNodesAllocated) {
        free(radar->healthNodes);
    }
    if (radar->state & RKRadarStatePositionBufferAllocated) {
        free(radar->positions);
    }
    if (radar->state & RKRadarStateRawIQBufferAllocated) {
        free(radar->pulses);
    }
    if (radar->state & RKRadarStateRayBufferAllocated) {
        free(radar->rays);
    }
    if (radar->state & RKRadarStateProductBufferAllocated) {
        for (i = 0; i < radar->desc.productBufferDepth; i++) {
            free(radar->products[i].startAzimuth);
            free(radar->products[i].endAzimuth);
            free(radar->products[i].startElevation);
            free(radar->products[i].endElevation);
            free(radar->products[i].data);
        }
        free(radar->products);
    }
    if (radar->state & RKRadarStateControlsAllocated) {
        free(radar->controls);
    }
    if (radar->state & RKRadarStateWaveformCalibrationsAllocated) {
        free(radar->waveformCalibrations);
    }
    // Other resources
    pthread_mutex_destroy(&radar->mutex);
    free(radar);
    RKLog("Done.");
    return RKResultSuccess;
}

#pragma mark - Hardware Hooks

//
// The three major compenents: Digital transceiver, pedestal and general health relay
//
// Input:
//     RKRadar *radar - object of the radar
//     void *input - user defined input structure / object, de-referenced to void * here
//     RKTransceiver/RKPedestal/RKHealthRelay initRoutine(RKRadar *, void *) - init routine
//     int execRoutine(RKTransceiver, const char *, char *) - command execution routine
//     int freeRoutine(RKTransceiver)) - de-allocation routine
// Output:
//     RKResultSuccess if everything was successful
//
int RKSetTransceiver(RKRadar *radar,
                     void *initInput,
                     RKTransceiver initRoutine(RKRadar *, void *),
                     int execRoutine(RKTransceiver, const char *, char *),
                     int freeRoutine(RKTransceiver)) {
    radar->transceiverInitInput = initInput;
    radar->transceiverInit = initRoutine;
    radar->transceiverExec = execRoutine;
    radar->transceiverFree = freeRoutine;
    return RKResultSuccess;
}

int RKSetPedestal(RKRadar *radar,
                  void *initInput,
                  RKPedestal initRoutine(RKRadar *, void *),
                  int execRoutine(RKPedestal, const char *, char *),
                  int freeRoutine(RKPedestal)) {
    radar->pedestalInitInput = initInput;
    radar->pedestalInit = initRoutine;
    radar->pedestalExec = execRoutine;
    radar->pedestalFree = freeRoutine;
    return RKResultSuccess;
}

int RKSetHealthRelay(RKRadar *radar,
                     void *initInput,
                     RKHealthRelay initRoutine(RKRadar *, void *),
                     int execRoutine(RKHealthRelay, const char *, char *),
                     int freeRoutine(RKHealthRelay)) {
    radar->healthRelayInitInput = initInput;
    radar->healthRelayInit = initRoutine;
    radar->healthRelayExec = execRoutine;
    radar->healthRelayFree = freeRoutine;
    return RKResultSuccess;
}

#pragma mark - Before-Live Properties

int RKSetProcessingCoreCounts(RKRadar *radar,
                              const uint8_t pulseEngineCoreCount,
                              const uint8_t pulseRingFilterCoreCount,
                              const uint8_t momentEngineCoreCount) {
    if (radar->pulseEngine == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    if (radar->state & RKRadarStateLive) {
        return RKResultUnableToChangeCoreCounts;
    }
    RKPulseEngineSetCoreCount(radar->pulseEngine, pulseEngineCoreCount);
    RKPulseRingFilterEngineSetCoreCount(radar->pulseRingFilterEngine, pulseRingFilterCoreCount);
    RKMomentEngineSetCoreCount(radar->momentEngine, momentEngineCoreCount);
    return RKResultSuccess;
}

#pragma mark - Properties

//
// Property setters are self-explanatory so minimal descriptions are provided here
//
int RKSetVerbosity(RKRadar *radar, const int verbose) {
    RKLog("Setting verbose level to %d ...\n", verbose);
    if (verbose == 1) {
        radar->desc.initFlags |= RKInitFlagVerbose;
    } else if (verbose == 2) {
        radar->desc.initFlags |= RKInitFlagVeryVerbose;
    } else if (verbose >= 3) {
        radar->desc.initFlags |= RKInitFlagVeryVeryVerbose;
    }
    if (radar->fileManager) {
        RKFileManagerSetVerbose(radar->fileManager, verbose);
    }
    if (radar->hostMonitor) {
        RKHostMonitorSetVerbose(radar->hostMonitor, verbose);
    }
    if (radar->pulseClock) {
        RKClockSetVerbose(radar->pulseClock, verbose);
    }
    if (radar->positionClock) {
        RKClockSetVerbose(radar->positionClock, verbose);
    }
    if (radar->positionEngine) {
        RKPositionEngineSetVerbose(radar->positionEngine, verbose);
    }
    if (radar->pulseEngine) {
        RKPulseEngineSetVerbose(radar->pulseEngine, verbose);
    }
    if (radar->pulseRingFilterEngine) {
        RKPulseRingFilterEngineSetVerbose(radar->pulseRingFilterEngine, verbose);
    }
    if (radar->momentEngine) {
        RKMomentEngineSetVerbose(radar->momentEngine, verbose);
    }
    if (radar->healthEngine) {
        RKHealthEngineSetVerbose(radar->healthEngine, verbose);
    }
    if (radar->radarRelay) {
        RKRadarRelaySetVerbose(radar->radarRelay, verbose);
    }
    if (radar->healthLogger) {
        RKHealthLoggerSetVerbose(radar->healthLogger, verbose);
    }
    if (radar->sweepEngine) {
        RKSweepEngineSetVerbose(radar->sweepEngine, verbose);
    }
    if (radar->rawDataRecorder) {
        RKRawDataRecorderSetVerbose(radar->rawDataRecorder, verbose);
    }
    return RKResultSuccess;
}

int RKSetVerbosityUsingArray(RKRadar *radar, const uint8_t *array) {
    int k;
    for (k = 'a'; k < 'z'; k++) {
        if (array[k] == 0) {
            continue;
        }
        switch (k) {
            case '0':
                RKClockSetVerbose(radar->positionClock, array[k]);
                break;
            case '1':
                RKClockSetVerbose(radar->pulseClock, array[k]);
                break;
            case 'a':
                RKPositionEngineSetVerbose(radar->positionEngine, array[k]);
                break;
            case 'm':
                RKMomentEngineSetVerbose(radar->momentEngine, array[k]);
                break;
            case 'p':
                RKPulseEngineSetVerbose(radar->pulseEngine, array[k]);
                break;
            case 'r':
                RKPulseRingFilterEngineSetVerbose(radar->pulseRingFilterEngine, array[k]);
                break;
            case 's':
                RKSweepEngineSetVerbose(radar->sweepEngine, array[k]);
                break;
            default:
                break;
        }
    }
    return RKResultSuccess;
}

int RKSetDataPath(RKRadar *radar, const char *path) {
    memcpy(radar->desc.dataPath, path, RKMaximumPathLength - 1);
    RKSetRootFolder(path);
    return RKResultSuccess;
}

int RKSetDataUsageLimit(RKRadar *radar, const size_t limit) {
    radar->fileManager->usagelimit = limit;
    RKLog("Usage limit %s B\n", RKIntegerToCommaStyleString(radar->fileManager->usagelimit));
    return RKResultSuccess;
}

int RKSetRecordingLevel(RKRadar *radar, const int level) {
    // Perhaps use a numeric string 100, 110, etc.
    //RKLog("Raw data recording: %s\n", level == 2 ? "Raw from transceiver" : (level == 1 ? "Compressed I/Q" : "No"));
    //RKLog("Product recording: %s\n", level > -1 ? "True" : "False");
    switch (level) {
        case 2:
            RKRawDataRecorderSetRawDataType(radar->rawDataRecorder, RKRawDataTypeFromTransceiver);
            RKRawDataRecorderSetRecord(radar->rawDataRecorder, true);
            RKSweepEngineSetRecord(radar->sweepEngine, true);
            RKHealthLoggerSetRecord(radar->healthLogger, true);
            break;
        case 1:
            RKRawDataRecorderSetRawDataType(radar->rawDataRecorder, RKRawDataTypeAfterMatchedFilter);
            RKRawDataRecorderSetRecord(radar->rawDataRecorder, true);
            RKSweepEngineSetRecord(radar->sweepEngine, true);
            RKHealthLoggerSetRecord(radar->healthLogger, true);
            break;
        default:
            RKRawDataRecorderSetRecord(radar->rawDataRecorder, false);
            RKSweepEngineSetRecord(radar->sweepEngine, true);
            RKHealthLoggerSetRecord(radar->healthLogger, true);
            break;
    }
    return RKResultSuccess;
}

// NOTE: It is possible to call this function as RKSetWaveform(radar, radar->waveform);
int RKSetWaveform(RKRadar *radar, RKWaveform *waveform) {
    if (radar->pulseEngine == NULL) {
        RKLog("Error. No pulse compression engine.\n");
        return RKResultNoPulseCompressionEngine;
    }
    if (waveform == NULL) {
        RKLog("Error. Supplied waveform is NULL\n");
        return RKResultFailedToSetWaveform;
    }
    if (waveform->count > 1 && waveform->filterCounts[0] != waveform->filterCounts[1]) {
        RKLog("Error. Different filter count in different waveform is not supported. (%d, %d)\n", waveform->filterCounts[0], waveform->filterCounts[1]);
        return RKResultFailedToSetWaveform;
    }
    if (waveform->filterCounts[0] <= 0 || waveform->filterCounts[0] > 4) {
        RKLog("Error. Multiplexing = %d filters has not been implemented. Reverting to impulse filter.\n", waveform->filterCounts[0]);
        return RKSetWaveformToImpulse(radar);
    }
    RKWaveform *oldWaveform = radar->waveform;
    RKWaveform *oldWaveformDecimate = radar->waveformDecimate;
    const double pulseWidth = (double)waveform->depth / waveform->fs;
    radar->waveform = RKWaveformCopy(waveform);
    radar->waveformDecimate = RKWaveformCopy(waveform);
    RKWaveformDecimate(radar->waveformDecimate, radar->desc.pulseToRayRatio);
    int j, k, r;
    RKPulseEngineResetFilters(radar->pulseEngine);
    for (k = 0; k < waveform->count; k++) {
        for (j = 0; j < waveform->filterCounts[k]; j++) {
            RKComplex *filter = waveform->samples[k] + waveform->filterAnchors[k][j].origin;
            r = RKPulseEngineSetFilter(radar->pulseEngine,
                                            filter,
                                            waveform->filterAnchors[k][j],
                                            k,
                                            j);
            if (r != RKResultSuccess) {
                return RKResultFailedToSetWaveform;
            }
        }
    }
    // Search for the waveform calibration for this waveform
    RKWaveformCalibration *waveformCalibration = NULL;
    for (k = 0; k < radar->waveformCalibrationCount; k++) {
        if (!strcmp(radar->waveformCalibrations[k].name, waveform->name)) {
            waveformCalibration = &radar->waveformCalibrations[k];
            break;
        }
    }
    // Send the waveform pointers to config buffer
    RKAddConfig(radar,
                RKConfigKeyWaveform, radar->waveform,
                RKConfigKeyWaveformDecimate, radar->waveformDecimate,
                RKConfigKeyWaveformCalibration, waveformCalibration,
                RKConfigKeyPulseWidth, pulseWidth,
                RKConfigKeyNull);
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("Waveform '%s' cached.\n", waveform->name);
    }
    if (oldWaveformDecimate != NULL) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Freeing radar->waveformDecimate cache ...\n");
        }
        RKWaveformFree(oldWaveformDecimate);
    }
    if (oldWaveform != NULL) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Freeing radar->waveform cache ...\n");
        }
        RKWaveformFree(oldWaveform);
    }
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKPulseEngineFilterSummary(radar->pulseEngine);
    }
    if (radar->state & RKRadarStateLive) {
        RKClockReset(radar->pulseClock);
    }
    return RKResultSuccess;
}

//
// Sets the waveform from a pre-defined file that specifies the digital samples for an
// arbitrary waveform generator.
//
int RKSetWaveformByFilename(RKRadar *radar, const char *filename) {
    if (radar->pulseEngine == NULL) {
        RKLog("Error. No pulse compression engine.\n");
        return RKResultNoPulseCompressionEngine;
    }
    // Load in the waveform
    RKWaveform *waveform = RKWaveformInitFromFile(filename);
    return RKSetWaveform(radar, waveform);
}

int RKSetWaveformToImpulse(RKRadar *radar) {
    if (radar->pulseEngine == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    RKPulseEngineSetFilterToImpulse(radar->pulseEngine);
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKPulseEngineFilterSummary(radar->pulseEngine);
    }
    RKWaveform *waveform = RKWaveformInitAsImpulse();
    return RKSetWaveform(radar, waveform);;
}

//int RKSetWaveformTo121(RKRadar *radar) {
//    if (radar->pulseEngine == NULL) {
//        return RKResultNoPulseCompressionEngine;
//    }
//    RKPulseEngineSetFilterTo121(radar->pulseEngine);
//    if (radar->desc.initFlags & RKInitFlagVerbose) {
//        RKPulseEngineFilterSummary(radar->pulseEngine);
//    }
//    return RKResultSuccess;
//}

int RKSetPRF(RKRadar *radar, const uint32_t prf) {
    RKAddConfig(radar, RKConfigKeyPRF, prf, RKConfigKeyNull);
    return RKResultSuccess;
}

uint32_t RKGetPulseCapacity(RKRadar *radar) {
    if (radar->pulses == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    RKPulse *pulse = RKGetPulseFromBuffer(radar->pulses, 0);
    return pulse->header.capacity;
}

void RKSetPulseTicsPerSeconds(RKRadar *radar, const double delta) {
    RKClockSetDxDu(radar->pulseClock, 1.0 / delta);
}

void RKSetPositionTicsPerSeconds(RKRadar *radar, const double delta) {
    RKClockSetDxDu(radar->positionClock, 1.0 / delta);
}

int RKSetMomentCalibrator(RKRadar *radar, void (*calibrator)(RKScratch *, RKConfig *)) {
    if (radar->momentEngine == NULL) {
        return RKResultNoMomentEngine;
    }
    radar->momentEngine->calibrator = calibrator;
    return RKResultSuccess;
}

int RKSetPulseCompressor(RKRadar *radar, void (*compressor)(RKCompressionScratch *)) {
    if (radar->pulseEngine == NULL) {
        return RKResultNoPulseCompressionEngine;
    }
    radar->pulseEngine->compressor = compressor;
    return RKResultSuccess;
}

int RKSetMomentProcessorToMultiLag(RKRadar *radar, const uint8_t lagChoice) {
    if (radar->momentEngine == NULL) {
        return RKResultNoMomentEngine;
    }
    if (radar->momentEngine->processor == &RKMultiLag && radar->momentEngine->userLagChoice == lagChoice) {
        return RKResultSuccess;
    }
    radar->momentEngine->processor = &RKMultiLag;
    radar->momentEngine->processorLagCount = RKMaximumLagCount;
    if (lagChoice < 0 || lagChoice > 4) {
        RKLog("Error. Invalid lag choice (%d) for multi-lag method.\n", lagChoice);
        return RKResultInvalidMomentParameters;
    }
    radar->momentEngine->userLagChoice = lagChoice;
    RKLog("Moment processor set to %sMultilag %d%s",
          rkGlobalParameters.showColor ? "\033[4m" : "",
          lagChoice,
          rkGlobalParameters.showColor ? "\033[24m" : "");
    return RKResultSuccess;
}

int RKSetMomentProcessorToPulsePair(RKRadar *radar) {
    if (radar->momentEngine == NULL) {
        return RKResultNoMomentEngine;
    }
    radar->momentEngine->processor = &RKPulsePair;
    radar->momentEngine->processorLagCount = 3;
    RKLog("Moment processor set to %sPulse Pair%s",
          rkGlobalParameters.showColor ? "\033[4m" : "",
          rkGlobalParameters.showColor ? "\033[24m" : "");
    return RKResultSuccess;
}

int RKSetMomentProcessorToPulsePairHop(RKRadar *radar) {
    if (radar->momentEngine == NULL) {
        return RKResultNoMomentEngine;
    }
    radar->momentEngine->processor = &RKPulsePairHop;
    radar->momentEngine->processorLagCount = 2;
    RKLog("Moment processor set to %sPulse Pair for Frequency Hopping%s\n",
          rkGlobalParameters.showColor ? "\033[4m" : "",
          rkGlobalParameters.showColor ? "\033[24m" : "");
    if (radar->pulseRingFilterEngine->useFilter &&
        radar->pulseRingFilterEngine->filter.type >= RKFilterTypeElliptical1 &&
        radar->pulseRingFilterEngine->filter.type <= RKFilterTypeElliptical4) {
        RKLog("Ring filter incompatible with Pulse Pair for Frequency Hopping, disabling ...");
        RKPulseRingFilterEngineDisableFilter(radar->pulseRingFilterEngine);
    }
    return RKResultSuccess;
}

int RKSetMomentProcessorToPulsePairStaggeredPRT(RKRadar *radar) {
    if (radar->momentEngine == NULL) {
        return RKResultNoMomentEngine;
    }
    radar->momentEngine->processor = &RKPulsePairStaggeredPRT;
    radar->momentEngine->processorLagCount = 2;
    RKLog("Warning. Moment processor set to %sPulse Pair for Staggered PRT%s (Not Implemented)\n",
          rkGlobalParameters.showColor ? "\033[4m" : "",
          rkGlobalParameters.showColor ? "\033[24m" : "");
    return RKResultSuccess;
}

int RKSetMomentProcessorToSpectralMoment(RKRadar *radar) {
    if (radar->momentEngine == NULL) {
        return RKResultNoMomentEngine;
    }
    radar->momentEngine->processor = &RKSpectralMoment;
    RKLog("Moment processor set to %sSpectral Moment%s\n",
          rkGlobalParameters.showColor ? "\033[4m" : "",
          rkGlobalParameters.showColor ? "\033[24m" : "");
    return RKResultSuccess;
}

int RKSetProductRecorder(RKRadar *radar, int (*productRecorder)(RKProduct *, const char *)) {
    RKSweepEngineSetProductRecorder(radar->sweepEngine, productRecorder);
    return RKResultSuccess;
}

int RKSetPulseRingFilterByType(RKRadar *radar, RKFilterType type, const uint32_t gateCount) {
    RKIIRFilter *filter = (RKIIRFilter *)malloc(sizeof(RKIIRFilter));
    if (filter == NULL) {
        RKLog("Error. Unable to allocate a filter.\n");
        return RKResultFailedToSetFilter;
    }
    RKGetFilterCoefficients(filter, type);
    RKSetPulseRingFilter(radar, filter, gateCount);
    free(filter);
    return RKResultSuccess;
}

// gateCount = 0 means no change from existing setting
int RKSetPulseRingFilter(RKRadar *radar, RKIIRFilter *filter, const uint32_t gateCount) {
    if (filter == NULL) {
        return RKResultFailedToSetFilter;
    }
    RKIIRFilter *oldFilter = radar->filter;
    radar->filter = (RKIIRFilter *)malloc(sizeof(RKIIRFilter));
    if (radar->filter == NULL) {
        RKLog("Error. Unable to allocate filter.\n");
        return RKResultFailedToSetFilter;
    }
    memcpy(radar->filter, filter, sizeof(RKIIRFilter));
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("Filter '%s' cached.\n", filter->name);
    }
    if (oldFilter != NULL) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Freeing RKIIRFilter cache ...\n");
        }
        free(oldFilter);
    }
    if (gateCount) {
        RKConfig *config = RKGetLatestConfig(radar);
        if (config->ringFilterGateCount != gateCount) {
            //RKLog("pulseRingFilterGateCount (%d) %d -> %d\n", config->i, config->pulseRingFilterGateCount, gateCount);
            RKAddConfig(radar, RKConfigKeyRingFilterGateCount, gateCount, RKConfigKeyNull);
        }
    }
    RKPulseRingFilterEngineSetFilter(radar->pulseRingFilterEngine, filter);
    RKPulseRingFilterEngineEnableFilter(radar->pulseRingFilterEngine);
    if (radar->state & RKRadarStateLive) {
        RKPulseRingFilterEngineShowFilterSummary(radar->pulseRingFilterEngine);
    } else {
        RKLog("Initial Pulse ring filter set to %s%s%s\n",
              rkGlobalParameters.showColor ? "\033[4m" : "",
              filter->name,
              rkGlobalParameters.showColor ? "\033[24m" : "");
    }
    return RKResultSuccess;
}

#pragma mark - Interaction / State Change

//
// Set the radar to go live. All the engines will be ignited with this function, going into infinite
// loops waiting for incoming samples or events, which come from network connections.
// Input:
//     RKRadar *radar - object of the radar
// Output:
//     Always RKResultSuccess
//
int RKGoLive(RKRadar *radar) {
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("RKGoLive()   %s\n", RKVariableInString("radar->tic", &radar->tic, RKValueTypeUInt64));
    }
    pthread_mutex_lock(&radar->mutex);
    radar->tic++;

    // Start the engines
    RKFileManagerStart(radar->fileManager);
    RKHostMonitorStart(radar->hostMonitor);
    if (radar->desc.initFlags & RKInitFlagPulsePositionCombiner) {
        RKPositionEngineStart(radar->positionEngine);
    }
    if (radar->desc.initFlags & RKInitFlagSignalProcessor) {
        if (radar->desc.initFlags & RKInitFlagManuallyAssignCPU) {
            // Main thread uses 1 CPU. Start the others from 1.
            uint8_t o = 1;
            if (o + radar->pulseEngine->coreCount + radar->momentEngine->coreCount > radar->processorCount) {
                RKLog("Info. Not enough physical cores (%d / %d). Core counts will be adjusted.\n",
                      radar->pulseEngine->coreCount + radar->momentEngine->coreCount, radar->processorCount);
                RKPulseEngineSetCoreCount(radar->pulseEngine, MAX(1, radar->processorCount / 2));
                RKPulseRingFilterEngineSetCoreCount(radar->pulseRingFilterEngine, MAX(1, radar->processorCount / 2));
                RKMomentEngineSetCoreCount(radar->momentEngine, MAX(1, radar->processorCount / 2 - 1));
                RKMomentEngineSetCoreOrigin(radar->momentEngine, o + radar->pulseEngine->coreCount);
            }
            // For now, pulse compression and ring filter engines both share the same cores
            RKPulseEngineSetCoreOrigin(radar->pulseEngine, o);
            RKPulseRingFilterEngineSetCoreOrigin(radar->pulseRingFilterEngine, o);
        }
        // Now, we start the engines
        RKPulseEngineStart(radar->pulseEngine);
        RKPulseRingFilterEngineStart(radar->pulseRingFilterEngine);
        RKMomentEngineStart(radar->momentEngine);
        RKHealthEngineStart(radar->healthEngine);
        // After all the engines started, we monitor them. This engine should be stopped before stopping the engines.
        radar->systemInspector = RKSystemInspector(radar);
    } else {
        RKRadarRelayStart(radar->radarRelay);
    }
    RKRawDataRecorderStart(radar->rawDataRecorder);
    RKHealthLoggerStart(radar->healthLogger);
    RKSweepEngineStart(radar->sweepEngine);

    // Get the post-allocated memory
    radar->memoryUsage = RKGetRadarMemoryUsage(radar);

    // Add a dummy config to get things started if there hasn't been one from the user
    if (radar->configIndex == 0) {
        RKAddConfig(radar,
                    RKConfigKeyPRF, 1000,
                    RKConfigKeySystemNoise, 0.1, 0.1,
                    RKConfigKeySystemZCal, -27.0, -27.0,
                    RKConfigKeySystemDCal, -0.01,
                    RKConfigKeySystemPCal, 0.01,
                    RKConfigKeySNRThreshold, 0.1,
                    RKConfigKeySQIThreshold, 0.2,
                    RKConfigKeyTransitionGateCount, 5,
                    RKConfigKeyRingFilterGateCount, 1000,
                    RKConfigKeyNull);
    }

    // Now we declare the radar is live
    radar->state |= RKRadarStateLive;
    
    // Health Relay
    if (radar->healthRelayInit != NULL) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Initializing health relay ...\n");
        }
        if (radar->healthRelayFree == NULL || radar->healthRelayExec == NULL) {
            RKLog("Error. Health relay incomplete.\n");
            pthread_mutex_unlock(&radar->mutex);
            RKStop(radar);
            return RKResultIncompleteHealthRelay;
        }
        radar->healthRelay = radar->healthRelayInit(radar, radar->healthRelayInitInput);
        if (radar->healthRelay == NULL) {
            RKLog("Error. Unable to start the health relay.\n");
            pthread_mutex_unlock(&radar->mutex);
            RKStop(radar);
            return RKResultFailedToStartHealthRelay;
        }
        radar->state |= RKRadarStateHealthRelayInitialized;
    }
    
    // Pedestal
    if (radar->pedestalInit != NULL) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Initializing pedestal ...\n");
        }
        if (radar->pedestalFree == NULL || radar->pedestalExec == NULL) {
            RKLog("Error. Pedestal incomplete\n.");
            pthread_mutex_unlock(&radar->mutex);
            RKStop(radar);
            return RKResultIncompletePedestal;
        }
        radar->pedestal = radar->pedestalInit(radar, radar->pedestalInitInput);
        if (radar->pedestal == NULL) {
            RKLog("Error. Unable to start the pedestal.\n");
            pthread_mutex_unlock(&radar->mutex);
            RKStop(radar);
            return RKResultFailedToStartPedestal;
        }
        radar->state |= RKRadarStatePedestalInitialized;
    }

    // Transceiver
    if (radar->transceiverInit != NULL) {
        if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
            RKLog("Initializing transceiver ...\n");
        }
        if (radar->transceiverFree == NULL || radar->transceiverExec == NULL) {
            RKLog("Error. Transceiver incomplete.\n");
            pthread_mutex_unlock(&radar->mutex);
            RKStop(radar);
            return RKResultIncompleteTransceiver;
        }
        radar->transceiver = radar->transceiverInit(radar, radar->transceiverInitInput);
        if (radar->transceiver == NULL) {
            RKLog("Error. Unable to start the transceiver.\n");
            pthread_mutex_unlock(&radar->mutex);
            RKStop(radar);
            return RKResultFailedToStartTransceiver;
        }
        radar->state |= RKRadarStateTransceiverInitialized;
    }

    // For now, the transceiver is the master controller
    if (radar->masterController == NULL) {
        radar->masterController = radar->transceiver;
        radar->masterControllerExec = radar->transceiverExec;
    }

    // Show the udpated memory usage
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("Radar live. All data buffers occupy %s%s B%s (%s GiB)\n",
              rkGlobalParameters.showColor ? "\033[4m" : "",
              RKIntegerToCommaStyleString(radar->memoryUsage),
              rkGlobalParameters.showColor ? "\033[24m" : "",
              RKFloatToCommaStyleString((double)radar->memoryUsage / 1073741824.0));
    }

    // Now we declare the radar active
    radar->active = true;
    radar->tic++;
    pthread_mutex_unlock(&radar->mutex);
    return RKResultSuccess;
}

int RKStart(RKRadar *radar) {
    RKGoLive(radar);
    RKWaitWhileActive(radar);
    return RKResultSuccess;
}

//
// Wait indefinitely until the radar engines are killed
// Input:
//     RKRadar *radar - the radar object of the system
// Output:
//     None
//
int RKWaitWhileActive(RKRadar *radar) {
    char buffer[RKMaximumStringLength];
    bool isForeground = tcgetpgrp(STDIN_FILENO) == getpgrp();
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("Process is in %s", isForeground ? "foreground" : "background");
    }
    while (radar->active) {
        if (isForeground) {
            fgets(buffer, sizeof(buffer), stdin);
            if (feof(stdin) && radar->tic > 2) {
                fprintf(stderr, "\n");
                if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
                    RKLog("EOF (Ctrl-D) detected.\n");
                }
                RKStop(radar);
            }
        }
        usleep(100000);
        radar->tic++;
    }
    return RKResultSuccess;
}

//
// Stops the engines. The radar engines will all be killed.
// Input:
//     RKRadar object of the radar
// Ouput:
//     Always RKResultSuccess
//
int RKStop(RKRadar *radar) {
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("RKStop()   %s", RKVariableInString("radar->tic", &radar->tic, RKValueTypeUInt64));
    }
    int s = 0;
    float t;
    do {
        if (++s % 40 == 0) {
            t = s * 0.025f;
            RKLog("Waiting for %s < 2 ... %s s\n",
                  RKVariableInString("radar->tic", &radar->tic, RKValueTypeUInt64),
                  RKVariableInString("t", &t, RKValueTypeFloat));
        }
        usleep(25000);
    } while (radar->tic < 2);
    pthread_mutex_lock(&radar->mutex);
    if (!(radar->state & RKRadarStateLive)) {
        RKLog("Radar is not live.\n");
        pthread_mutex_unlock(&radar->mutex);
        return RKResultEngineDeactivatedMultipleTimes;
    }
    radar->state &= ~RKRadarStateLive;
    if (radar->systemInspector) {
        RKSimpleEngineFree(radar->systemInspector);
    }
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("Radar state = 0x%08x\n", radar->state);
    }
    if (radar->state & RKRadarStatePedestalInitialized) {
        if (radar->pedestalExec != NULL) {
            if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
                RKLog("Sending 'disconnect' to pedestal ...\n");
            }
            radar->pedestalExec(radar->pedestal, "disconnect", radar->pedestalResponse);
        }
        radar->state ^= RKRadarStatePedestalInitialized;
    }
    if (radar->state & RKRadarStateTransceiverInitialized) {
        if (radar->transceiverExec != NULL) {
            if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
                RKLog("Sending 'disconnect' to transceiver ...\n");
            }
            radar->transceiverExec(radar->transceiver, "disconnect", radar->transceiverResponse);
        }
        radar->state ^= RKRadarStateTransceiverInitialized;
    }
    if (radar->state & RKRadarStateHealthRelayInitialized) {
        if (radar->healthRelayExec != NULL) {
            if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
                RKLog("Sending 'disconnect' to health relay ...\n");
            }
            radar->healthRelayExec(radar->healthRelay, "disconnect", radar->healthRelayResponse);
        }
        radar->state ^= RKRadarStateHealthRelayInitialized;
    }
    if (radar->state & RKRadarStateFileManagerInitialized) {
        RKFileManagerStop(radar->fileManager);
        radar->state ^= RKRadarStateFileManagerInitialized;
    }
    if (radar->state & RKRadarStateHostMonitorInitialized) {
        RKHostMonitorStop(radar->hostMonitor);
        radar->state ^= RKRadarStateHostMonitorInitialized;
    }
    if (radar->state & RKRadarStatePulseCompressionEngineInitialized) {
        RKPulseEngineStop(radar->pulseEngine);
        radar->state ^= RKRadarStatePulseCompressionEngineInitialized;
    }
    if (radar->state & RKRadarStatePulseRingFilterEngineInitialized) {
        RKPulseRingFilterEngineStop(radar->pulseRingFilterEngine);
        radar->state ^= RKRadarStatePulseRingFilterEngineInitialized;
    }
    if (radar->state & RKRadarStatePositionEngineInitialized) {
        RKPositionEngineStop(radar->positionEngine);
        radar->state ^= RKRadarStatePositionEngineInitialized;
    }
    if (radar->state & RKRadarStateMomentEngineInitialized) {
        RKMomentEngineStop(radar->momentEngine);
        radar->state ^= RKRadarStateMomentEngineInitialized;
    }
    if (radar->state & RKRadarStateHealthEngineInitialized) {
        RKHealthEngineStop(radar->healthEngine);
        radar->state ^= RKRadarStateHealthEngineInitialized;
    }
    if (radar->state & RKRadarStateHealthLoggerInitialized) {
        RKHealthLoggerStop(radar->healthLogger);
        radar->state ^= RKRadarStateHealthLoggerInitialized;
    }
    if (radar->state & RKRadarStateRadarRelayInitialized) {
        RKRadarRelayStop(radar->radarRelay);
        radar->state ^= RKRadarStateRadarRelayInitialized;
    }
    if (radar->state & RKRadarStateFileRecorderInitialized) {
        RKRawDataRecorderStop(radar->rawDataRecorder);
        radar->state ^= RKRadarStateFileRecorderInitialized;
    }
    if (radar->state & RKRadarStateSweepEngineInitialized) {
        RKSweepEngineStop(radar->sweepEngine);
        radar->state ^= RKRadarStateSweepEngineInitialized;
    }
    if (radar->desc.initFlags & RKInitFlagVeryVeryVerbose) {
        RKLog("Radar state = 0x%x\n", radar->state);
    }
    radar->active = false;
    pthread_mutex_unlock(&radar->mutex);
    return RKResultSuccess;
}

int RKSoftRestart(RKRadar *radar) {
    int i, k;
    size_t bytes;
    if (!(radar->state & RKRadarStateLive)) {
        RKLog("Error. Radar not live. Unable to restart.\n");
        return RKResultRadarNotLive;
    }
    RKLog("Stopping internal engines ...\n");

    radar->state ^= RKRadarStateLive;

    // Stop the inspector
    RKSimpleEngineFree(radar->systemInspector);

    // Stop all data acquisition and DSP-related engines
    RKSweepEngineStop(radar->sweepEngine);
    RKRawDataRecorderStop(radar->rawDataRecorder);
    RKHealthLoggerStop(radar->healthLogger);
    RKHealthEngineStop(radar->healthEngine);
    RKMomentEngineStop(radar->momentEngine);
    RKPositionEngineStop(radar->positionEngine);
    RKPulseRingFilterEngineStop(radar->pulseRingFilterEngine);
    RKPulseEngineStop(radar->pulseEngine);

    // Reset all major indices
    radar->statusIndex = 0;
    radar->healthIndex = 0;
    radar->positionIndex = 0;
    radar->pulseIndex = 0;
    radar->rayIndex = 0;
    for (k = 0; k < radar->desc.healthNodeCount; k++) {
        radar->healthNodes[k].index = 0;
    }

    // Clear out some data buffers
    bytes = radar->desc.statusBufferDepth * sizeof(RKStatus);
    memset(radar->status, 0, bytes);
    for (i = 0; i < radar->desc.statusBufferDepth; i++) {
        radar->status[i].i = -(uint64_t)radar->desc.statusBufferDepth + i;
    }
    bytes = radar->desc.healthBufferDepth * sizeof(RKHealth);
    memset(radar->healths, 0, bytes);
    for (i = 0; i < radar->desc.healthBufferDepth; i++) {
        radar->healths[i].i = -(uint64_t)radar->desc.healthBufferDepth + i;
    }
    for (k = 0; k < radar->desc.healthNodeCount; k++) {
        memset(radar->healthNodes[k].healths, 0, bytes);
        for (i = 0; i < radar->desc.healthBufferDepth; i++) {
            radar->healthNodes[k].healths[i].i = -(uint64_t)radar->desc.healthBufferDepth + i;
        }
    }
    bytes = radar->desc.positionBufferDepth * sizeof(RKPosition);
    memset(radar->positions, 0, bytes);
    for (i = 0; i < radar->desc.positionBufferDepth; i++) {
        radar->positions[i].i = -(uint64_t)radar->desc.positionBufferDepth + i;
    }
    RKClearPulseBuffer(radar->pulses, radar->desc.pulseBufferDepth);
    RKClearRayBuffer(radar->rays, radar->desc.rayBufferDepth);

    // Restore the waveform
    RKSetWaveform(radar, radar->waveform);

    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("Starting internal engines ... %d / %d / %d\n", radar->pulseIndex, radar->rayIndex, radar->healthIndex);
    }

    // Start them again
    RKPulseEngineStart(radar->pulseEngine);
    RKPulseRingFilterEngineStart(radar->pulseRingFilterEngine);
    RKPositionEngineStart(radar->positionEngine);
    RKMomentEngineStart(radar->momentEngine);
    RKHealthEngineStart(radar->healthEngine);
    RKHealthLoggerStart(radar->healthLogger);
    RKRawDataRecorderStart(radar->rawDataRecorder);
    RKSweepEngineStart(radar->sweepEngine);

    // Start the inspector
    radar->systemInspector = RKSystemInspector(radar);

    // Show the udpated memory usage
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("Radar live. All data buffers occupy %s%s B%s (%s GiB)\n",
              rkGlobalParameters.showColor ? "\033[4m" : "",
              RKUIntegerToCommaStyleString(radar->memoryUsage),
              rkGlobalParameters.showColor ? "\033[24m" : "",
              RKFloatToCommaStyleString((double)radar->memoryUsage / 1073741824.0));
    }

    radar->state ^= RKRadarStateLive;

    return RKResultSuccess;
}

int RKResetClocks(RKRadar *radar) {
    RKClockReset(radar->pulseClock);
    RKClockReset(radar->positionClock);
    return RKResultSuccess;
}

int RKExecuteCommand(RKRadar *radar, const char *commandString, char * _Nullable string) {
    int k;
    char sval1[RKMaximumStringLength];
    char sval2[RKMaximumStringLength];
    memset(sval1, 0, sizeof(sval1));
    memset(sval2, 0, sizeof(sval2));
    double fval1 = 0.0;
    double fval2 = 0.0;
    RKFilterType filterType;
    uint32_t u32;
    int ival;

    RKConfig *config = RKGetLatestConfig(radar);

    if (radar->desc.initFlags & RKInitFlagSignalProcessor) {
        k = 0;
        while (!(radar->state & RKRadarStateLive)) {
            usleep(100000);
            if (++k % 10 == 0 && radar->desc.initFlags & RKInitFlagVeryVerbose) {
                RKLog("Sleep 1/%.1f s   radar->state = 0x%04x\n", (float)k * 0.1f, radar->state);
            }
        }
        // Process the command
        // Commands 'a', 'c', 'dr', 's', 'u', 'x', 'z' will never arrive here
        switch (commandString[0]) {
            case 'd':
                // DSP related
                switch (commandString[commandString[1] == ' ' ? 2 : 1]) {
                    case 'c':
                        RKClearPulseBuffer(radar->pulses, radar->desc.pulseBufferDepth);
                        RKClearRayBuffer(radar->rays, radar->desc.rayBufferDepth);
                        if (string) {
                            sprintf(string, "ACK. Buffers cleared." RKEOL);
                        }
                        break;
                    case 'f':
                        // 'df' - DSP filter
                        k = sscanf(&commandString[2], "%d %u", &ival, &u32);
                        if (k == 2) {
                            filterType = ival;
                            RKLog("type = %u   u32 = %u\n", filterType, u32);
                            RKSetPulseRingFilterByType(radar, filterType, u32);
                            if (string) {
                                sprintf(string, "ACK. Ring filter %u gateCount %s." RKEOL, filterType, RKIntegerToCommaStyleString(u32));
                            }
                        } else if (k == 1) {
                            filterType = ival;
                            if ((RKFilterType)filterType == RKFilterTypeNull) {
                                RKPulseRingFilterEngineDisableFilter(radar->pulseRingFilterEngine);
                                if (string) {
                                    sprintf(string, "ACK. Ring filter disabled." RKEOL);
                                }
                            } else {
                                RKSetPulseRingFilterByType(radar, filterType, 0);
                                if (string) {
                                    sprintf(string, "ACK. Ring filter %u." RKEOL, filterType);
                                }
                            }
                        } else {
                            RKPulseRingFilterEngineDisableFilter(radar->pulseRingFilterEngine);
                            if (string) {
                                sprintf(string, "ACK. Ring filter disabled." RKEOL);
                            }
                        }
                        break;
                    case 'm':
                        // 'dm' - DSP moment method
                        k = sscanf(&commandString[2], "%d", &ival);
                        if (k == 1) {
                            switch (ival) {
                                case 1:
                                    RKSetMomentProcessorToPulsePairHop(radar);
                                    if (string) {
                                        sprintf(string, "ACK. Moment processor set to PulsePairHop." RKEOL);
                                    }
                                    break;
                                case 2:
                                    RKSetMomentProcessorToMultiLag(radar, 2);
                                    if (string) {
                                        sprintf(string, "ACK. Moment processor set to MultiLag-2." RKEOL);
                                    }
                                    break;
                                case 3:
                                    RKSetMomentProcessorToMultiLag(radar, 3);
                                    if (string) {
                                        sprintf(string, "ACK. Moment processor set to MultiLag-3." RKEOL);
                                    }
                                    break;
                                case 4:
                                    RKSetMomentProcessorToMultiLag(radar, 4);
                                    if (string) {
                                        sprintf(string, "ACK. Moment processor set to MultiLag-3." RKEOL);
                                    }
                                    break;
                                case 5:
                                    RKSetMomentProcessorToSpectralMoment(radar);
                                    if (string) {
                                        sprintf(string, "ACK. Moment processor set to SpectralMoment." RKEOL);
                                    }
                                    break;
                                default:
                                    RKSetMomentProcessorToPulsePair(radar);
                                    if (string) {
                                        sprintf(string, "ACK. Moment processor set to PulsePair." RKEOL);
                                    }
                                    break;
                            }
                        } else if (string) {
                            sprintf(string, "ACK. Current moment processor is %s" RKEOL,
                                    radar->momentEngine->processor == &RKPulsePair ? "PulsePair" :
                                    (radar->momentEngine->processor == &RKPulsePairHop ? "PulsePairHop" :
                                     (radar->momentEngine->processor == &RKMultiLag && radar->momentEngine->userLagChoice == 2 ? "MultiLag-2" :
                                      (radar->momentEngine->processor == &RKMultiLag && radar->momentEngine->userLagChoice == 3 ? "MultiLag-3" :
                                       (radar->momentEngine->processor == &RKMultiLag && radar->momentEngine->userLagChoice == 4 ? "MultiLag-4" :
                                        (radar->momentEngine->processor == &RKSpectralMoment ? "SpectralMoment" : "Unknown"))))));
                        }
                        break;
                    case 'n':
                        // 'dn' - DSP noise override
                        k = sscanf(&commandString[2], "%lf %lf", &fval1, &fval2);
                        if (k == 2) {
                            RKAddConfig(radar, RKConfigKeySystemNoise, fval1, fval2, RKConfigKeyNull);
                            if (string) {
                                sprintf(string, "ACK. Noise set to %.4f, %.4f" RKEOL, fval1, fval2);
                            }
                        } else if (k == -1 && string) {
                            sprintf(string, "ACK. Current noise is %.4f %.4f" RKEOL, config->noise[0], config->noise[1]);
                        } else if (string) {
                            sprintf(string, "NAK. Must have two paramters  (k = %d)." RKEOL, k);
                        }
                        break;
                    case 'N':
                        // 'dN' - DSP noise override in dB
                        break;
                    case 'r':
                        // 'dr' - Restart DSP engines
                        RKSoftRestart(radar);
                        if (string) {
                            sprintf(string, "ACK. Soft restart executed." RKEOL);
                        }
                        break;
                    case 'q':
                        k = sscanf(&commandString[2], "%lf", &fval1);
                        if (k == 1) {
                            RKAddConfig(radar, RKConfigKeySQIThreshold, fval1, RKConfigKeyNull);
                            if (string) {
                                sprintf(string, "ACK. SQI threshold set to %.2f" RKEOL, fval1);
                            }
                        } else if (string) {
                            sprintf(string, "ACK. Current SQI threshold is %.2f" RKEOL, config->SQIThreshold);
                        }
                        break;
                    case 's':
                        // 'ds' - DSP threshold in SNR dB
                    case 't':
                        // 'dt' - DSP threshold in SNR dB
                        k = sscanf(&commandString[2], "%lf", &fval1);
                        if (k == 1) {
                            RKAddConfig(radar, RKConfigKeySNRThreshold, fval1, RKConfigKeyNull);
                            if (string) {
                                sprintf(string, "ACK. SNR threshold set to %.2f dB" RKEOL, fval1);
                            }
                        } else if (string) {
                            sprintf(string, "ACK. Current SNR threshold is %.2f dB" RKEOL, config->SNRThreshold);
                        }
                        break;
                    default:
                        break;
                }
                break;
                
            case 'h':
                if (strlen(commandString) == 1 || !strncmp(commandString, "help", 4)) {
                    if (string == NULL) {
                        RKLog("Trying to get help without providing string? Tell my father.\n");
                        break;
                    }
                    k = sprintf(string,
                                "Help\n"
                                "====\n"
                                "\n"
                                HIGHLIGHT("a") " [USERNAME] [ENCRYPTED_PASSWORD] - Authenticate\n"
                                "\n"
                                HIGHLIGHT("d") " [COMMAND] [PARAMETER] - DSP parameters,\n"
                                "    where command can be one of the following:\n"
                                "        f - ground clutter filter\n"
                                "            - 0 - No ground clutter filter\n"
                                "            - 1 - Ground clutter filter Elliptical @ +/- 0.1 rad/sample\n"
                                "            - 2 - Ground clutter filter Elliptical @ +/- 0.2 rad/sample\n"
                                "            - 3 - Ground clutter filter Elliptical @ +/- 0.3 rad/sample\n"
                                "            - 4 - Ground clutter filter Elliptical @ +/- 0.4 rad/sample\n"
                                "            NOTE: Depending on the PRT, the actual filter velocity can be obtained\n"
                                "                  by scaling the discrete filter frequency (omega) in rad/sample to\n"
                                "                  filter velocity in m/s as:\n"
                                "                  velocity = omega / (2 * PI) * va\n"
                                "        r - restart internal engines\n"
                                "        t - threshold to censor using VALUE in dB\n"
                                "\n"
                                "    e.g.,\n"
                                "        dr - restarts internal engines\n"
                                "        dn 0.1 - overrides the noise with 0.1 ADU\n"
                                "        dt 0 - sets the threshold to censor at 0 dB\n"
                                "        df 1 - sets the ground clutter filter to Elliptical @ +/- 0.1 rad/sample\n"
                                "\n"
                                HIGHLIGHT("s") " [VALUE] - Get various data streams\n"
                                "    where [VALUE] can be one of the following:\n"
                                "        0 - Position update (the latest)\n"
                                "        1 - Pulse updates (the latest)\n"
                                "        2 - Product generation (ray by ray update)\n"
                                "        3 - Overall view of the system load\n"
                                "        4 - Various engine states\n"
                                "        5 - Buffer overview\n"
                                "        6 - Range time plot of reflectivity\n"
                                "            (Modes 0 - 6 are exclusive, i.e., one at a time),\n"
                                "    or any combination of any of the following:\n"
                                "        z - Display stream of Z reflectivity\n"
                                "        v - Display stream of V velocity\n"
                                "        w - Display stream of W width\n"
                                "        d - Display stream of D differential reflectivity\n"
                                "        p - Display stream of P PhiDP differential phase\n"
                                "        r - Display stream of R RhoHV cross-correlation coefficient\n"
                                "        k - Display stream of K KDP specific phase\n"
                                "        s - Display stream of Sh signal power in dBm\n"
                                "\n"
                                "    e.g.,\n"
                                "        s 0 - streams the pedestal raw data\n"
                                "        s 2 - streams the product summary header\n"
                                "        s 3 - streams the overall system status\n"
                                "        s zvwd - streams Z, V, W and D\n"
                                "\n"
                                HIGHLIGHT("r") " - Toggle in between start and stop recording I/Q data\n"
                                "\n"
                                HIGHLIGHT("v") " - Sets a simple VCP (coming soon)\n"
                                "    e.g.,\n"
                                "        v 2:2:20 180 - a volume at EL 2° to 20° at 2° steps, AZ slew at 180°/s\n"
                                "\n"
                                HIGHLIGHT("b") " - Simulate a push-button event from a physical hardware button\n"
                                "\n"
                                HIGHLIGHT("y") " - Everything goes, default waveform and VCP\n"
                                "\n"
                                HIGHLIGHT("z") " - Everything stops\n"
                                "\n");
                    
                    k += sprintf(string + k,
                                 HIGHLIGHT("t") " - " UNDERLINE_ITALIC("Transceiver") " commands, everything that starts with 't' goes to the transceiver\n"
                                 "    module in a concatenated form, e.g., 't help' -> 'help' to the transceiver.\n\n");
                    if (radar->transceiver) {
                        radar->transceiverExec(radar->transceiver, "help", sval1);
                        RKStripTail(sval1);
                        k += RKIndentCopy(string + k, sval1, 5);
                        k += sprintf(string + k, "\n\n");
                    } else {
                        k += sprintf(string + k, "    INFO: Transceiver not set.\n");
                    }
                    printf("%s\n", sval1);

                    k += sprintf(string + k,
                                 HIGHLIGHT("p") " - " UNDERLINE_ITALIC ("Pedestal") " commands, everything that starts with 'p' goes to the pedestal module\n"
                                 "    in a concatenated form, e.g., 'p help' -> 'help' to the pedestal.\n\n");
                    if (radar->pedestal) {
                        radar->pedestalExec(radar->pedestal, "help", sval1);
                        RKStripTail(sval1);
                        k += RKIndentCopy(string + k, sval1, 5);
                        k += sprintf(string + k, "\n\n");
                    } else {
                        k += sprintf(string + k, "    INFO: Pedestal not set.\n");
                    }
                    
                    k += sprintf(string + k,
                                 HIGHLIGHT("h") " - " UNDERLINE_ITALIC ("Health Relay") " commands, everything that starts with 'h' goes to the health relay\n"
                                 "    module in a concatenated form, e.g., 'p help' -> 'help' to the health relay.\n\n");
                    if (radar->healthRelay) {
                        radar->healthRelayExec(radar->healthRelay, "help", sval1);
                        RKStripTail(sval1);
                        k += RKIndentCopy(string + k, sval1, 5);
                        k += sprintf(string + k, "\n\n");
                    } else {
                        k += sprintf(string + k, "    INFO: Health Relay not set.\n");
                    }
                    
                    sprintf(string + k, "\n== (%s) ==" RKEOL, RKIntegerToCommaStyleString(k));
                    
                } else {
                
                    // Forward to health relay
                    if (strlen(commandString) < 2) {
                        if (string) {
                            sprintf(string, "NAK. Empty command to health relay. Ignoring ..." RKEOL);
                        }
                        //RKOperatorSendCommandResponse(O, string);
                        break;
                    }
                    k = 0;
                    do {
                        k++;
                    } while (commandString[k] == ' ');
                    radar->healthRelayExec(radar->healthRelay, commandString + k, string);
                }
                break;
                
            case 'p':
                // Pass everything to pedestal
                if (strlen(commandString) < 2) {
                    if (string) {
                        sprintf(string, "NAK. Empty command to pedestal. Ignoring ..." RKEOL);
                    }
                    break;
                }
                k = 0;
                do {
                    k++;
                } while (commandString[k] == ' ');
                radar->pedestalExec(radar->pedestal, commandString + k, string);
                break;

            case 't':
                // Pass everything to transceiver
                if (strlen(commandString) < 2) {
                    if (string) {
                        sprintf(string, "NAK. Empty command to transceiver. Ignoring ..." RKEOL);
                    }
                    break;
                }
                k = 0;
                do {
                    k++;
                } while (commandString[k] == ' ');
                radar->transceiverExec(radar->transceiver, commandString + k, string);
                break;

            case 'r':
                radar->rawDataRecorder->record = !radar->rawDataRecorder->record;
                if (strlen(commandString) > 1) {
                    k = 0;
                    do {
                        k++;
                    } while (commandString[k] == ' ');
                    if (commandString[k] == '0') {
                        radar->rawDataRecorder->rawDataType = RKRawDataTypeFromTransceiver;
                        radar->rawDataRecorder->record = true;
                    } else if (commandString[k] == '1') {
                        radar->rawDataRecorder->rawDataType = RKRawDataTypeAfterMatchedFilter;
                        radar->rawDataRecorder->record = true;
                    } else {
                        radar->rawDataRecorder->rawDataType = RKRawDataTypeNull;
                        radar->rawDataRecorder->record = false;
                    }
                }
                if (string) {
                    sprintf(string, "ACK. Recorder is %s." RKEOL,
                            radar->rawDataRecorder->record == false ? "set to standby" :
                            (radar->rawDataRecorder->rawDataType == RKRawDataTypeFromTransceiver ? "recording raw I/Q" :
                             (radar->rawDataRecorder->rawDataType == RKRawDataTypeAfterMatchedFilter ? "recording I/Q" : "in unknown state")));
                }
                break;
                
            case 'b':  // Button event
            case 'y':  // Start everything
            case 'z':  // Stop everything
                // Passed to the master controller
                if (radar->masterController == NULL) {
                    if (string) {
                        sprintf(string, "NAK. Not ready." RKEOL);
                    }
                } else {
                    radar->masterControllerExec(radar->masterController, commandString, string);
                }
                break;
                
            default:
                if (string) {
                    snprintf(string, RKMaximumStringLength - 1, "NAK. Unknown command '%s'." RKEOL, commandString);
                }
                break;
        } // switch ...
    }
    return RKResultSuccess;
}

//
// Performs a function in the background.
// This function is intended for handling commands that may recurse to itself. Instead of causing a block
// call, this function make the functional call as a separate call eliminating the blocking behavior.
// Input:
//     RKRadar *radar - object of the radar
//     char *command - a string of the command
// Ouput:
//     None
//
void RKPerformMasterTaskInBackground(RKRadar *radar, const char *command) {
    if (radar->masterController == NULL) {
        RKLog("Master controller is not set.\n");
        return;
    }
    RKRadarCommand taskInput;
    taskInput.radar = radar;
    strncpy(taskInput.command, command, RKMaximumCommandLength - 1);
    pthread_t tid;
    pthread_create(&tid, NULL, &masterControllerExecuteInBackground, &taskInput);
}

// General

//
// Measure noise from the latest 200 pulses
// Input:
//     RKRadar *radar - object of the radar
// Ouput:
//     None
//
void RKMeasureNoise(RKRadar *radar) {
    int k = 0;
    RKFloat noise[2];
    RKFloat noiseAverage[2] = {0.0f, 0.0f};
    uint32_t index = RKPreviousModuloS(radar->pulseIndex, radar->desc.pulseBufferDepth);
    RKPulse *pulse = RKGetPulseFromBuffer(radar->pulses, index);
    while (!(pulse->header.s & RKPulseStatusCompressed) && k++ < radar->desc.pulseBufferDepth) {
        index = RKPreviousModuloS(index, radar->desc.pulseBufferDepth);
        pulse = RKGetPulseFromBuffer(radar->pulses, index);
    }
    // Avoid data before this offset to exclude the transient effects right after transmit pulse
    int origin = 0;
    for (k = 0; k < radar->pulseEngine->filterCounts[0]; k++) {
        origin += radar->pulseEngine->filterAnchors[0][k].length;
    }
    // Add another tail (fill pulse width)
    if (k > 0) {
        origin += 2 * radar->pulseEngine->filterAnchors[0][k - 1].length;
    }
    // Account for down-sampling stride in PulseCompressionEngine
    origin /= MAX(1, radar->desc.pulseToRayRatio);
    for (k = 0; k < RKPulseCountForNoiseMeasurement; k++) {
        index = RKPreviousModuloS(index, radar->desc.pulseBufferDepth);
        pulse = RKGetPulseFromBuffer(radar->pulses, index);
        RKMeasureNoiseFromPulse(noise, pulse, origin);
        noiseAverage[0] += noise[0];
        noiseAverage[1] += noise[1];
    }
    noiseAverage[0] /= (RKFloat)k;
    noiseAverage[1] /= (RKFloat)k;
    if (!isfinite(noiseAverage[0])) {
        noiseAverage[0] = 0.001f;
    }
    if (!isfinite(noiseAverage[1])) {
        noiseAverage[1] = 0.001f;
    }
    RKAddConfig(radar, RKConfigKeySystemNoise, noiseAverage[0], noiseAverage[1], RKConfigKeyNull);
}

void RKSetSNRThreshold(RKRadar *radar, const RKFloat threshold) {
    RKAddConfig(radar, RKConfigKeySNRThreshold, threshold, RKConfigKeyNull);
}

#pragma mark - Status

//
// Get a vacant slot to fill in system status
// This method is usually used by the RadarKit internally to report
// system status.
// Input:
//     RKRadar *radar - object of the radar
// Output:
//     None
//
RKStatus *RKGetVacantStatus(RKRadar *radar) {
    RKStatus *status = &radar->status[radar->statusIndex];
    if (status->flag != RKStatusFlagVacant) {
        RKLog("Warning. radar->status[%d] should be vacant.\n", radar->statusIndex);
    }
    if (radar->state & RKRadarStateLive) {
        status->i += radar->desc.statusBufferDepth;
        radar->statusIndex = RKNextModuloS(radar->statusIndex, radar->desc.statusBufferDepth);
        radar->status[radar->statusIndex].flag = RKStatusFlagVacant;
    }
    return status;
}

void RKSetStatusReady(RKRadar *radar, RKStatus *status) {
    if (radar->state & RKRadarStateLive) {
        status->flag |= RKStatusFlagReady;
    }
}

#pragma mark - Configs

//
// Add a configuration to change the operational setting.
// This is an internally used function, shouldn't be used by
// user program.
// Input:
//     RKConfigKey key - the key that describes what comes next
//     value(s) - values of the key
// Output:
//     None
// Note:
//     The last key must be RKConfigKeyNull
// Example:
//     RKConfigAdd(radar, RKConfigKeyPRF, 1000, RKConfigNull) to set PRF
//     RKConfigAdd(radar, RKConfigKeySystemNoise, 0.3, 0.2, RKConfigNull) to set noise
//
// Users normally don't have to deal with these
//
void RKAddConfig(RKRadar *radar, ...) {
    va_list args;
    va_start(args, radar);
    if (radar->desc.initFlags & RKInitFlagVeryVerbose) {
        RKLog("RKAddConfig() ...\n");
    }
    return RKConfigAdvance(radar->configs, &radar->configIndex, radar->desc.configBufferDepth, args);
}

RKConfig *RKGetLatestConfig(RKRadar *radar) {
    return &radar->configs[RKPreviousModuloS(radar->configIndex, radar->desc.configBufferDepth)];
}

#pragma mark - Healths

//
// Request a health reporting node.
// Input:
//     RKRadar radar - the radar
// Output:
//     An RKHealthNode to identifity that node
//
RKHealthNode RKRequestHealthNode(RKRadar *radar) {
    RKHealthNode node = RKHealthNodeUser1 + radar->healthNodeCount;
    if (node == RKHealthNodeCount) {
        RKLog("Error. No more health node available.\n");
        node = (RKHealthNode)-1;
    }
    radar->healthNodeCount++;
    return node;
}

//
// Get a health slot for a specific node.
// Input:
//     RKRadar radar - the radar
//     RKHealthNode node - the node to report the health later
// Output
//     A vacant slot to fill in health information
//
RKHealth *RKGetVacantHealth(RKRadar *radar, const RKHealthNode node) {
    if (radar->healthEngine == NULL) {
        RKLog("Error. Health engine has not started.\n");
        //exit(EXIT_FAILURE);
        return NULL;
    }
    radar->healthNodes[node].active = true;
    uint32_t index = radar->healthNodes[node].index;
    RKHealth *health = &radar->healthNodes[node].healths[index];
    if (radar->state & RKRadarStateLive) {
        health->i += radar->desc.healthBufferDepth;
        index = RKNextModuloS(index, radar->desc.healthBufferDepth);
        radar->healthNodes[node].healths[index].flag = RKHealthFlagVacant;
        radar->healthNodes[node].healths[index].string[0] = '\0';
        radar->healthNodes[node].index = index;
    }
    return health;
}

void RKSetHealthReady(RKRadar *radar, RKHealth *health) {
    gettimeofday(&health->time, NULL);
    health->timeDouble = (double)health->time.tv_sec + 1.0e-6 * (double)health->time.tv_usec;
    health->flag = RKHealthFlagReady;
}

RKHealth *RKGetLatestHealth(RKRadar *radar) {
    uint32_t index = RKPreviousModuloS(radar->healthIndex, radar->desc.healthBufferDepth);
    if (!(radar->healths[index].flag & RKHealthFlagReady)) {
        index = RKPreviousModuloS(index, radar->desc.healthBufferDepth);
    }
    return &radar->healths[index];
}

RKHealth *RKGetLatestHealthOfNode(RKRadar *radar, const RKHealthNode node) {
    uint32_t index = RKPreviousModuloS(radar->healthNodes[node].index, radar->desc.healthBufferDepth);
    if (!(radar->healthNodes[node].healths[index].flag & RKHealthFlagReady)) {
        index = RKPreviousModuloS(index, radar->desc.healthBufferDepth);
    }
    return &radar->healthNodes[node].healths[index];
}

RKStatusEnum RKGetEnumFromLatestHealth(RKRadar *radar, const char *keyword) {
    RKHealth *health = RKGetLatestHealth(radar);
    char *stringObject = RKGetValueOfKey(health->string, keyword);
    if (stringObject) {
        char *stringEnum = RKGetValueOfKey(stringObject, "enum");
        if (stringEnum) {
            return (RKStatusEnum)atoi(stringEnum);
        }
    }
    return RKStatusEnumInvalid;
}

#pragma mark - Positions

//
// Get a vacant slot to fill in position data from the pedestal.
// Input:
//     RKRadar *radar - object of the radar
// Output:
//     None
//
RKPosition *RKGetVacantPosition(RKRadar *radar) {
    if (radar->positionEngine == NULL) {
        RKLog("Error. Pedestal engine has not started.\n");
        return NULL;
    }
    RKPosition *position = &radar->positions[radar->positionIndex];
    if (position->flag != RKPositionFlagVacant) {
        position->flag = RKPositionFlagVacant;
        if (radar->state & RKRadarStateLive) {
            RKLog("Warning. Unexpected position flag.\n");
        }
    }
    if (radar->state & RKRadarStateLive) {
        position->i += radar->desc.positionBufferDepth;
    }
    radar->positions[RKNextNModuloS(radar->positionIndex, radar->desc.positionBufferDepth / 8, radar->desc.positionBufferDepth)].flag = RKPositionFlagVacant;
    return position;
}

void RKSetPositionReady(RKRadar *radar, RKPosition *position) {
    if (position->flag & ~RKPositionFlagHardwareMask) {
        RKLog("Error. Ingested a position with a flag (0x%08x) outside of allowable value.\n", position->flag);
    }
    position->timeDouble = RKClockGetTime(radar->positionClock, (double)position->tic, &position->time);
    if ((radar->desc.initFlags & RKInitFlagShowClockOffset) && (position->tic % 5 == 0)) {
        struct timeval t;
        gettimeofday(&t, NULL);
        printf("\033[38;5;222mposition\033[0m @ %+14.4f %+14.4f            x0 = %10.4f  u0 = %10.0f  dx/du = %.3e  k = %u\n", position->timeDouble,
               position->timeDouble - ((double)t.tv_sec + 1.0e-6 * (double)t.tv_usec - radar->positionClock->initDay),
               radar->positionClock->x0, radar->positionClock->u0, radar->positionClock->dx, radar->positionClock->index);
    }
    position->flag |= RKPositionFlagReady;
    radar->positionIndex = RKNextModuloS(radar->positionIndex, radar->desc.positionBufferDepth);
    return;
}

RKPosition *RKGetLatestPosition(RKRadar *radar) {
    uint32_t index = RKPreviousModuloS(radar->positionIndex, radar->desc.positionBufferDepth);
    RKPosition *position = &radar->positions[index];
    int k = 0;
    while (!(position->flag & RKPositionFlagReady) && k++ < radar->desc.positionBufferDepth) {
        index = RKPreviousModuloS(index, radar->desc.pulseBufferDepth);
        position = &radar->positions[index];
    }
    return position;
}

float RKGetPositionUpdateRate(RKRadar *radar) {
    uint32_t n = radar->desc.positionBufferDepth / 2;
    uint32_t i = RKPreviousModuloS(radar->positionIndex, radar->desc.positionBufferDepth);
    uint32_t o = RKPreviousNModuloS(radar->positionIndex, n, radar->desc.positionBufferDepth);
    return (float)n / (radar->positions[i].timeDouble - radar->positions[o].timeDouble);
}

#pragma mark - Pulses

//
// Get a vacant slot to fill in pulse data from the digital transceiver.
// Input:
//     RKRadar *radar - object of the radar
// Output:
//     None
//
RKPulse *RKGetVacantPulse(RKRadar *radar) {
    if (radar->pulses == NULL) {
        RKLog("Error. Buffer for raw pulses has not been allocated.\n");
        exit(EXIT_FAILURE);
    }
    // Set the 1/8-old pulse vacant
    RKPulse *pulse = RKGetPulseFromBuffer(radar->pulses, RKNextNModuloS(radar->pulseIndex, radar->desc.pulseBufferDepth >> 3, radar->desc.pulseBufferDepth));
    pulse->header.s = RKPulseStatusVacant;
    // Current pulse
    pulse = RKGetPulseFromBuffer(radar->pulses, radar->pulseIndex);
    pulse->header.s = RKPulseStatusVacant;
    pulse->header.timeDouble = 0.0;
    pulse->header.time.tv_sec = 0;
    pulse->header.time.tv_usec = 0;
    if (radar->state & RKRadarStateLive) {
        pulse->header.i += radar->desc.pulseBufferDepth;
        radar->pulseIndex = RKNextModuloS(radar->pulseIndex, radar->desc.pulseBufferDepth);
    }
    return pulse;
}

void RKSetPulseHasData(RKRadar *radar, RKPulse *pulse) {
    if (pulse->header.i == 0) {
        RKClockReset(radar->pulseClock);
    }
    if (pulse->header.timeDouble == 0.0 && pulse->header.time.tv_sec == 0) {
        pulse->header.timeDouble = RKClockGetTime(radar->pulseClock, (double)pulse->header.t, &pulse->header.time);
    }
    if (pulse->header.gateCount > pulse->header.capacity) {
        RKLog("Error. gateCount = %s > capacity = %s\n",
              RKIntegerToCommaStyleString(pulse->header.gateCount), RKIntegerToCommaStyleString(pulse->header.capacity));
        pulse->header.gateCount = pulse->header.capacity;
    }
    if ((radar->desc.initFlags & RKInitFlagShowClockOffset) && (pulse->header.i % 100 == 0)) {
        struct timeval t;
        gettimeofday(&t, NULL);
        printf("           %+14.4f %+14.4f @ \033[38;5;118mpulse\033[0m    x0 = %10.4f  u0 = %10.0f  dx/du = %.3e\n", pulse->header.timeDouble,
               pulse->header.timeDouble - ((double)t.tv_sec + 1.0e-6 * (double)t.tv_usec - radar->pulseClock->initDay),
               radar->pulseClock->x0, radar->pulseClock->u0, radar->pulseClock->dx);
    }
    if (radar->state & RKRadarStateLive) {
        pulse->header.s = RKPulseStatusHasIQData;
    }
    return;
}

void RKSetPulseReady(RKRadar *radar, RKPulse *pulse) {
    if (radar->state & RKRadarStateLive) {
        pulse->header.s = RKPulseStatusHasIQData | RKPulseStatusHasPosition;
    }
}

RKPulse *RKGetLatestPulse(RKRadar *radar) {
    uint32_t index = RKPreviousModuloS(radar->pulseIndex, radar->desc.pulseBufferDepth);
    RKPulse *pulse = RKGetPulseFromBuffer(radar->pulses, index);
    int k = 0;
    while (!(pulse->header.s & RKPulseStatusCompressed) && k++ < radar->desc.pulseBufferDepth) {
        index = RKPreviousModuloS(index, radar->desc.pulseBufferDepth);
        pulse = RKGetPulseFromBuffer(radar->pulses, index);
    }
    return pulse;
}

#pragma mark - Rays

//
// Get a vacant slot to fill in ray data.
// MomentEngine doesn't rely on this function.
// This function may be used for a relay to fill the buffer.
// Input:
//     RKRadar *radar - object of the radar
// Output:
//     None
//
RKRay *RKGetVacantRay(RKRadar *radar) {
    if (radar->rays == NULL) {
        RKLog("Error. Buffer for rays has not been allocated.\n");
        exit(EXIT_FAILURE);
    }
    RKRay *ray = RKGetRayFromBuffer(radar->rays, radar->rayIndex);
    ray->header.s = RKRayStatusVacant;
    ray->header.startTime.tv_sec = 0;
    ray->header.startTime.tv_usec = 0;
    ray->header.endTime.tv_sec = 0;
    ray->header.endTime.tv_usec = 0;
    if (radar->state & RKRadarStateLive) {
        ray->header.i += radar->desc.rayBufferDepth;
        radar->rayIndex = RKNextModuloS(radar->rayIndex, radar->desc.rayBufferDepth);
    }
    return ray;
}

void RKSetRayReady(RKRadar *radar, RKRay *ray) {
    if (radar->state & RKRadarStateLive) {
        ray->header.s |= RKRayStatusReady;
    }
}

RKRay *RKGetLatestRay(RKRadar *radar) {
    uint32_t index = RKPreviousModuloS(radar->rayIndex, radar->desc.rayBufferDepth);
    RKRay *ray = RKGetRayFromBuffer(radar->rays, index);
    int k = 0;
    while (!(ray->header.s & RKRayStatusReady) && k++ < radar->desc.rayBufferDepth) {
        index = RKPreviousModuloS(index, radar->desc.rayBufferDepth);
        ray = RKGetRayFromBuffer(radar->rays, index);
    }
    return ray;
}

RKRay *RKGetLatestRayIndex(RKRadar *radar, uint32_t *index) {
    *index = RKPreviousModuloS(radar->rayIndex, radar->desc.rayBufferDepth);
    RKRay *ray = RKGetRayFromBuffer(radar->rays, *index);
    int k = 0;
    while (!(ray->header.s & RKRayStatusReady) && k++ < radar->desc.rayBufferDepth) {
        *index = RKPreviousModuloS(*index, radar->desc.rayBufferDepth);
        ray = RKGetRayFromBuffer(radar->rays, *index);
    }
    if (k == radar->desc.rayBufferDepth) {
        ray = NULL;
    }
    return ray;
}

#pragma mark - Waveform Calibrations

void RKUpdateWaveformCalibration(RKRadar *radar, const uint8_t index, const RKWaveformCalibration *calibration) {
    if (index >= radar->desc.waveformCalibrationCapacity) {
        RKLog("Error. Unable to update waveform calibration.\n");
        return;
    }
    RKWaveformCalibration *target = &radar->waveformCalibrations[index];
    memcpy(target, calibration, sizeof(RKWaveformCalibration));
}

void RKAddWaveformCalibration(RKRadar *radar, const RKWaveformCalibration *calibration) {
    uint8_t index = radar->waveformCalibrationCount++;
    if (index >= radar->desc.waveformCalibrationCapacity) {
        RKLog("Error. Cannot add anymore waveform calibration.\n");
        return;
    }
    RKUpdateWaveformCalibration(radar, index, calibration);
}

void RKClearWaveformCalibrations(RKRadar *radar) {
    radar->waveformCalibrationCount = 0;
}

void RKConcludeWaveformCalibrations(RKRadar *radar) {
    int k;
    for (k = 0; k < radar->waveformCalibrationCount; k++) {
        RKWaveformCalibration *waveformCalibration = &radar->waveformCalibrations[k];
        waveformCalibration->uid = getUID(UIDTypeWaveformCalibration);
    }
}

#pragma mark - Controls

void RKUpdateControl(RKRadar *radar, const uint8_t index, const RKControl *control) {
    if (index >= radar->desc.controlCapacity) {
        RKLog("Error. Control index is out of bound.\n");
        return;
    }
    RKControl *target = &radar->controls[index];
    strncpy(target->label, control->label, RKNameLength - 1);
    strncpy(target->command, control->command, RKMaximumCommandLength - 1);
}

void RKAddControl(RKRadar *radar, const RKControl *control) {
    uint8_t index = radar->controlCount++;
    if (index >= radar->desc.controlCapacity) {
        RKLog("Error. Cannot add anymore controls.\n");
        return;
    }
    RKUpdateControl(radar, index, control);
}

void RKAddControlAsLabelAndCommand(RKRadar *radar, const char *label, const char *command) {
    uint8_t index = radar->controlCount++;
    if (index >= radar->desc.controlCapacity) {
        RKLog("Error. Cannot add anymore controls.\n");
        return;
    }
    RKControl *target = &radar->controls[index];
    strncpy(target->label, label, RKNameLength - 1);
    strncpy(target->command, command, RKMaximumCommandLength - 1);
}

void RKClearControls(RKRadar *radar) {
    int k;
    radar->controlCount = 0;
    for (k = 0; k < radar->desc.controlCapacity; k++) {
        RKControl *control = &radar->controls[k];
        memset(control->label, 0, RKNameLength);
        memset(control->command, 0, RKMaximumCommandLength);
    }
}

void RKConcludeControls(RKRadar *radar) {
    int k;
    for (k = 0; k < radar->controlCount; k++) {
        RKControl *control = &radar->controls[k];
        control->uid = getUID(UIDTypeControl);
    }
}

#pragma mark - Developer Access

void RKGetRegisterValue(RKRadar *radar, void *value, const unsigned long registerOffset, size_t size) {
    memcpy(value, (void *)radar + registerOffset, size);
}

void RKSetRegisterValue(RKRadar *radar, void *value, const unsigned long registerOffset, size_t size) {
    memcpy((void *)radar + registerOffset, value, size);
}

void RKShowOffsets(RKRadar *radar, char *text) {
    int k = 0;
    char *buffer = (char *)malloc(RKMaximumStringLength);
    if (buffer == NULL) {
        fprintf(stderr, "Error. Unable to allocate memory.\n");
        return;
    }
    k += RADAR_VARIABLE_OFFSET(buffer + k, active);
    k += RADAR_VARIABLE_OFFSET(buffer + k, statusIndex);
    k += RADAR_VARIABLE_OFFSET(buffer + k, configIndex);
    k += RADAR_VARIABLE_OFFSET(buffer + k, healthIndex);
    k += RADAR_VARIABLE_OFFSET(buffer + k, positionIndex);
    k += RADAR_VARIABLE_OFFSET(buffer + k, pulseIndex);
    k += RADAR_VARIABLE_OFFSET(buffer + k, rayIndex);
    k += RADAR_VARIABLE_OFFSET(buffer + k, rawDataRecorder->record);
    k += RADAR_VARIABLE_OFFSET(buffer + k, positionEngine);
    k += RADAR_VARIABLE_OFFSET(buffer + k, pulseEngine->verbose);
    k += RADAR_VARIABLE_OFFSET(buffer + k, momentEngine);
    if (text == NULL) {
        printf("%s", buffer);
    } else {
        memcpy(text, buffer, k + 1);
    }
    free(buffer);
}

int RKBufferOverview(RKRadar *radar, char *text, const RKTextPreferences flag) {
    static int slice, positionStride = 1, pulseStride = 1, rayStride = 1, healthStride = 1;
    int i, j, k, m = 0, n = 0;
    char *c;
    size_t s;
    RKRay *ray;
    RKPulse *pulse;
    RKPosition *position;
    
    // Symbols and corresponding colors
    const char m0 = '.';  const char c0[] = RKRedColor;
    const char m1 = '|';  const char c1[] = RKYellowColor;
    const char m2 = ':';  const char c2[] = RKGreenColor;
    const char m3 = 'o';  const char c3[] = RKBlueColor;
    const char m4 = '-';  const char c4[] = RKBlueColor;
    
    int w = MAX(4, (int)log10(MAX(radar->desc.pulseBufferDepth, radar->desc.rayBufferDepth)) + 1);

    static struct winsize terminalSize = {.ws_col = 0, .ws_row = 0};
    
    if (flag & RKTextPreferencesDrawBackground) {
        // General address format goes like this: [color reset] [new line] %04d-%04d
        char format[64];
        sprintf(format, "%%0%dd-%%0%dd\n", w, w);
        
        // Check the terminal width
        switch (flag & RKTextPreferencesWindowSizeMask) {
            case RKTextPreferencesWindowSize80x25:
                terminalSize.ws_col = 80;
                terminalSize.ws_row = 25;
                break;
            case RKTextPreferencesWindowSize80x40:
                terminalSize.ws_col = 80;
                terminalSize.ws_row = 40;
                break;
            case RKTextPreferencesWindowSize80x50:
                terminalSize.ws_col = 80;
                terminalSize.ws_row = 50;
                break;
            case RKTextPreferencesWindowSize120x40:
                terminalSize.ws_col = 110;
                terminalSize.ws_row = 40;
                break;
            case RKTextPreferencesWindowSize120x50:
                terminalSize.ws_col = 110;
                terminalSize.ws_row = 50;
                break;
            case RKTextPreferencesWindowSize120x80:
                terminalSize.ws_col = 110;
                terminalSize.ws_row = 80;
                break;
            default:
                ioctl(0, TIOCGWINSZ, &terminalSize);
                if (terminalSize.ws_col == 0) {
                    terminalSize.ws_col = 110;
                } else {
                    if (terminalSize.ws_col >= 120) {
                        terminalSize.ws_col -= 10;
                    }
                }
                if (terminalSize.ws_row == 0) {
                    terminalSize.ws_row = 80;
                }
                RKLog("Using window size %d x %d\n", terminalSize.ws_col, terminalSize.ws_row);
                break;
        }
        
        // Background of buffers: digits occupy 2 numbers (2 x w), '-', some ' '(front), some ' '(back), then pick the optimal 5.
        slice = (terminalSize.ws_col - 2 * w - 3 + 4) / 5 * 5;

        if (flag & RKTextPreferencesShowDebuggingMessage) {
            RKLog("%s   %s", RKVariableInString("slice", &slice, RKValueTypeInt), RKVariableInString("w", &w, RKValueTypeInt));
        }

        // Clear screen, go to the origin
        m = sprintf(text, "\033[1;1H\033[2J");
        
        // Position
        if (radar->positions) {
            c = RKIntegerToCommaStyleString(radar->desc.positionBufferSize);
            s = strlen(c);
            if (terminalSize.ws_row > 25) {
                m += sprintf(text + m, "Position Buffer (%s B)\n", c);
                memset(text + m, '-', s + 20);
                m += s + 20;
                *(text + m++) = '\n';
                n += 4;
            } else {
                m += sprintf(text + m, "\033[4mPosition Buffer (%s B)\033[24m\n", c);
                n += 3;
            }
            k = slice * MAX(1, (terminalSize.ws_row - 16) / 8);
            positionStride = MAX(1, (radar->desc.positionBufferDepth + k - 1) / k);
            //RKLog("%s", RKVariableInString("positionStride", &positionStride, RKValueTypeInt));
            for (j = 0, k = 0; j < 30 && k < radar->desc.positionBufferDepth; j++) {
                m += sprintf(text + m, format, k, MIN(k + positionStride * slice, radar->desc.positionBufferDepth));
                k += positionStride * slice;
                n++;
            }
        } else {
            n++;
        }
        
        // Pulse buffer
        c = RKIntegerToCommaStyleString(radar->desc.pulseBufferSize);
        s = strlen(c);
        if (terminalSize.ws_row > 25) {
            m += sprintf(text + m, "\033[%d;1HPulse Buffer (%s B)\n", n, c);
            memset(text + m, '-', s + 17);
            m += s + 17;
            *(text + m++) = '\n';
            n += 3;
        } else {
            m += sprintf(text + m, "\033[%d;1H\033[4mPulse Buffer (%s B)\033[24m\n", n, c);
            n += 2;
        }
        k = slice * (terminalSize.ws_row - 16) / 2;
        pulseStride = MAX(1, (radar->desc.pulseBufferDepth + k - 1) / k);
        for (j = 0, k = 0; j < 30 && k < radar->desc.pulseBufferDepth; j++) {
            m += sprintf(text + m, format, k, MIN(k + pulseStride * slice, radar->desc.pulseBufferDepth));
            k += pulseStride * slice;
            n++;
        }

        // Ray buffer
        c = RKIntegerToCommaStyleString(radar->desc.rayBufferSize);
        s = strlen(c);
        if (terminalSize.ws_row > 25) {
            m += sprintf(text + m, "\033[%d;1HRay Buffer (%s B)\n", n, c);
            memset(text + m, '-', s + 15);
            m += s + 15;
            *(text + m++) = '\n';
            n += 3;
        } else {
            m += sprintf(text + m, "\033[%d;1H\033[4mRay Buffer (%s B)\033[24m\n", n, c);
            n += 2;
        }
        k = slice * (terminalSize.ws_row - 16) / 2;
        rayStride = MAX(1, (radar->desc.rayBufferDepth + k - 1) / k);
        for (j = 0, k = 0; j < 30 && k < radar->desc.rayBufferDepth; j++) {
            m += sprintf(text + m, format, k, MIN(k + rayStride * slice, radar->desc.rayBufferDepth));
            k += rayStride * slice;
            n++;
        }

        // Health buffers
        c = RKIntegerToCommaStyleString(radar->desc.healthNodeBufferSize);
        s = strlen(c);
        if (terminalSize.ws_row > 25) {
            m += sprintf(text + m, "\033[%d;1HHealth Buffers (%s B)\n", n, c);
            memset(text + m, '-', s + 19);
            m += s + 19;
            *(text + m++) = '\n';
            n += 3;
        } else {
            m += sprintf(text + m, "\033[%d;1H\033[4mHealth Buffers (%s B)\033[24m\n", n, c);
            n += 2;
        }
        k = (terminalSize.ws_col / 2 - 2 * w - 6 + 4) / 5 * 5;
        healthStride = MAX(1, (radar->desc.healthBufferDepth + k - 1) / k);
        for (k = 0; k < MIN(4, RKHealthNodeCount); k++) {
            m += sprintf(text + m, "%3s: 0-%d\n",
                         k == RKHealthNodeRadarKit ? "RKI" :
                         (k == RKHealthNodeTransceiver ? "TRX" :
                          (k == RKHealthNodePedestal ? "PED" :
                           (k == RKHealthNodeTweeta ? "TWT" :
                            (k == RKHealthNodeUser1 ? "US1" :
                             (k == RKHealthNodeUser2 ? "US2" :
                              (k == RKHealthNodeUser3 ? "US3" :
                               (k == RKHealthNodeUser1 ? "US4" : "UNK"))))))),
                         radar->desc.healthBufferDepth);
            n++;
        }
        n = n - k - 1;
        for (; k < MIN(8, RKHealthNodeCount); k++) {
            m += sprintf(text + m, "\033[%d;%dH", n, terminalSize.ws_col - 2 * w - 2 - radar->desc.healthBufferDepth / healthStride);
            m += sprintf(text + m, "%3s: 0-%d\n",
                            k == RKHealthNodeUser1 ? "US1" :
                             (k == RKHealthNodeUser2 ? "US2" :
                              (k == RKHealthNodeUser3 ? "US3" :
                               (k == RKHealthNodeUser1 ? "US4" : "UNK"))),
                         radar->desc.healthBufferDepth);
            n++;
        }
        n++;
        
        if (flag & RKTextPreferencesShowColor) {
            m += sprintf(text + m,
                         "\033[%d;%dH"
                         "%s%c" RKNoColor " Vacant    %s%c" RKNoColor " Has Data    %s%c" RKNoColor " Shared    %s%c" RKNoColor " Used\n",
                         n, 2 * w + 3 + MAX(0, (terminalSize.ws_col - 80) / 2), c0, m0, c1, m1, c2, m2, c3, m3);
        } else {
            m += sprintf(text + m,
                         "\033[%d;%dH%c Vacant    %c Has Data    %c Shared    %c Used\n",
                         n, 2 * w + 3 + MAX(0, (terminalSize.ws_col - 80) / 2), m0, m1, m2, m3);
        }
    }

    // Use w for two address end points plus the other characters
    w = 2 * w + 3;

    uint32_t s0, s1;
    if (radar->positions) {
        n = 2;
        if (terminalSize.ws_row > 25) {
            n++;
        }
        k = 0;
        for (j = 0; j < 30 && k < radar->desc.positionBufferDepth; j++) {
            // Move the cursor
            m += sprintf(text + m, "\033[%d;%dH", n, w);
            s1 = (uint32_t)-1;
            for (i = 0; i < slice && k < radar->desc.positionBufferDepth; i++, k += positionStride) {
                position = &radar->positions[k];
                s0 = position->flag;
                if (flag & RKTextPreferencesShowColor) {
                    if (s0 & RKPositionFlagUsed) {
                        if (s0 == s1) {
                            *(text + m++) = m3;
                        } else {
                            m += sprintf(text + m, "%s%c", c3, m3);
                        }
                    } else if (s0 & RKPositionFlagReady) {
                        if (s0 == s1) {
                            *(text + m++) = m1;
                        } else {
                            m += sprintf(text + m, "%s%c", c1, m1);
                        }
                    } else {
                        if (s0 == s1) {
                            *(text + m++) = m0;
                        } else {
                            m += sprintf(text + m, "%s%c", c0, m0);
                        }
                    }
                    s1 = s0;
                } else {
                    *(text + m++) = s0 & RKPositionFlagUsed ? m3 : (s0 & RKPositionFlagReady ? m1 : m0);
                }
            }
            n++;
        }
    } else {
        n = 0;
    }

    n += 2;
    if (terminalSize.ws_row > 25) {
        n++;
    }
    k = 0;
    for (j = 0; j < 30 && k < radar->desc.pulseBufferDepth; j++) {
        // Move the cursor
        m += sprintf(text + m, "\033[%d;%dH", n, w);
        s1 = (uint32_t)-1;
        for (i = 0; i < slice && k < radar->desc.pulseBufferDepth; i++, k += pulseStride) {
            pulse = RKGetPulseFromBuffer(radar->pulses, k);
            s0 = pulse->header.s;
            if (flag & RKTextPreferencesShowColor) {
                if (s0 & RKPulseStatusRecorded) {
                    if (s0 == s1) {
                        *(text + m++) = m4;
                    } else {
                        m += sprintf(text + m, "%s%c", c4, m4);
                    }
                } else if (s0 & RKPulseStatusUsedForMoments) {
                    if (s0 == s1) {
                        *(text + m++) = m3;
                    } else {
                        m += sprintf(text + m, "%s%c", c3, m3);
                    }
                } else if (s0 & RKPulseStatusRingProcessed) {
                    if (pulse->header.s == s1) {
                        *(text + m++) = m2;
                    } else {
                        m += sprintf(text + m, "%s%c", c2, m2);
                    }
                } else if (s0 & RKPulseStatusHasIQData) {
                    if (pulse->header.s == s1) {
                        *(text + m++) = m1;
                    } else {
                        m += sprintf(text + m, "%s%c", c1, m1);
                    }
                } else {
                    if (s0 == s1) {
                        *(text + m++) = m0;
                    } else {
                        m += sprintf(text + m, "%s%c", c0, m0);
                    }
                }
                s1 = s0;
            } else {
                *(text + m++) = s0 & RKPulseStatusUsedForMoments ? m3 : (s0 & RKPulseStatusRingProcessed ? m2 : (s0 & RKPulseStatusHasIQData ? m1 : m0));
            }
        }
        n++;
    }
    
    n += 2;
    if (terminalSize.ws_row > 25) {
        n++;
    }
    k = 0;
    for (j = 0; j < 30 && k < radar->desc.rayBufferDepth; j++) {
        m += sprintf(text + m, "\033[%d;%dH", n, w);
        s1 = (uint32_t)-1;
        for (i = 0; i < slice && k < radar->desc.rayBufferDepth; i++, k += rayStride) {
            ray = RKGetRayFromBuffer(radar->rays, k);
            s0 = ray->header.s;
            if (flag & RKTextPreferencesShowColor) {
                if (s0 & RKRayStatusBeingConsumed) {
                    if (s0 == s1) {
                        *(text + m++) = m3;
                    } else {
                        m += sprintf(text + m, "%s%c", c3, m3);
                    }
                } else if (s0 & RKRayStatusStreamed) {
                    if (ray->header.s == s1) {
                        *(text + m++) = m2;
                    } else {
                        m += sprintf(text + m, "%s%c", c2, m2);
                    }
                } else if (s0 & RKRayStatusReady) {
                    if (ray->header.s == s1) {
                        *(text + m++) = m1;
                    } else {
                        m += sprintf(text + m, "%s%c", c1, m1);
                    }
                } else {
                    if (s0 == s1) {
                        *(text + m++) = m0;
                    } else {
                        m += sprintf(text + m, "%s%c", c0, m0);
                    }
                }
            } else {
                *(text + m++) = s0 & RKRayStatusBeingConsumed ? m3 : (s0 & RKRayStatusStreamed ? m2 : (s0 & RKRayStatusReady ? m1 : m0));
            }
            s1 = s0;
        }
        n++;
    }

    n += 2;
    if (terminalSize.ws_row > 25) {
        n++;
    }
    for (j = 0; j < MIN(4, RKHealthNodeCount); j++) {
        m += sprintf(text + m, "\033[%d;%dH", n, w);
        s1 = (uint32_t)-1;
        for (i = 0, k = 0; i < slice && k < radar->desc.healthBufferDepth; i++, k += healthStride) {
            RKHealth *health = &radar->healthNodes[j].healths[k];
            s0 = health->flag;
            if (flag & RKTextPreferencesShowColor) {
                if (s0 & RKHealthFlagUsed) {
                    if (s0 == s1) {
                        *(text + m++) = m3;
                    } else {
                        m += sprintf(text + m, "%s%c", c3, m3);
                    }
                } else if (s0 & RKHealthFlagReady) {
                    if (s0 == s1) {
                        *(text + m++) = m1;
                    } else {
                        m += sprintf(text + m, "%s%c", c1, m1);
                    }
                } else {
                    if (s0 == s1) {
                        *(text + m++) = m0;
                    } else {
                        m += sprintf(text + m, "%s%c", c0, m0);
                    }
                }
            } else {
                *(text + m++) = s0 & RKHealthFlagReady ? m1 : m0;
            }
            s1 = s0;
        }
        n++;
    }
    n -= j;
    for (; j < MIN(8, RKHealthNodeCount); j++) {
        m += sprintf(text + m, "\033[%d;%dH", n, terminalSize.ws_col - radar->desc.healthBufferDepth / healthStride + 1);
        s1 = (uint32_t)-1;
        for (i = 0, k = 0; i < slice && k < radar->desc.healthBufferDepth; i++, k += healthStride) {
            RKHealth *health = &radar->healthNodes[j].healths[k];
            s0 = health->flag;
            if (flag & RKTextPreferencesShowColor) {
                if (s0 & RKHealthFlagUsed) {
                    if (s0 == s1) {
                        *(text + m++) = m3;
                    } else {
                        m += sprintf(text + m, "%s%c", c3, m3);
                    }
                } else if (s0 & RKHealthFlagReady) {
                    if (s0 == s1) {
                        *(text + m++) = m1;
                    } else {
                        m += sprintf(text + m, "%s%c", c1, m1);
                    }
                } else {
                    if (s0 == s1) {
                        *(text + m++) = m0;
                    } else {
                        m += sprintf(text + m, "%s%c", c0, m0);
                    }
                }
            } else {
                *(text + m++) = s0 & RKHealthFlagReady ? m1 : m0;
            }
            s1 = s0;
        }
        n++;
    }

    n++;
    c = RKIntegerToCommaStyleString(m);
    s = strlen(c);
    m += sprintf(text + m, "\033[0m\033[%d;%dH== (%s) ==" RKEOL, n, (int)(terminalSize.ws_col - s - 7), c);
    *(text + m) = '\0';
    return m;
}

char *RKGetNextKeyValue(char *json, char *key, char *value) {
    char *c = json;
    char *e = c + strlen(c);
    char *ks, *ke, *vs, *ve;
    // Forward until the next non-space character
    while (*c == ' ' && c < e) {
        c++;
    }
    if (c == e) {
        *key = '\0';
        return NULL;
    }
    // Find the key
    ks = c;
    switch (*ks) {
        case '"':
            ks++;
            ke = strchr(ks, '"');
            break;
        case '\'':
            ks++;
            ke = strchr(ks, '\'');
            break;
        default:
            ke = strchr(ks, ':');
            ke--;
            break;
    }
    memcpy(key, ks, ke - ks);
    key[ke - ks] = '\0';
    // Find the delimiter ':'
    c = strchr(ke, ':');
    if (c == NULL) {
        fprintf(stderr, "Incomplete string?\n");
        return NULL;
    }
    c++;
    // Find the next non-space character
    // Forward until the next non-space character
    while (*c == ' ' && c < e) {
        c++;
    }
    if (c == e) {
        *value = '\0';
        return NULL;
    }
    // If this is bracketed piece, find the start and end
    vs = c++;
    switch (*vs) {
        case '"':
            ve = strchr(c, '"');
            break;
        case '[':
            ve = strchr(c, ']');
            break;
        case '{':
            ve = strchr(c, '}');
            break;
        case '\'':
            ve = strchr(c, '\'');
            break;
        default:
            ve = strchr(c, ',');
            if (ve == NULL) {
                ve = e;
            }
            ve--;
            break;
    }
    memcpy(value, vs, ve - vs + 1);
    value[ve - vs + 1] = '\0';
    if (ve == e - 1) {
        c = NULL;
    } else {
        c = ve + 1;
        while (c < e && (*c == ',' || *c == ' ')) {
            c++;
        }
    }
    return c;
}

int RKHealthOverview(const char *json, char *text, const RKTextPreferences flag) {
    int m;
    char *c, *e;
    
    //static struct winsize terminalSize = {.ws_col = 0, .ws_row = 0};

    char key[RKNameLength];
    char value[RKStatusStringLength];
    char string[RKMaximumStringLength];

    // Make a local copy for manipulation
    strcpy(string, json);
    
    //if (flag & RKTextPreferencesDrawBackground) {
    c = string;
    e = c + strlen(c);
    if (*c == '{') {
        c++;
    }
    if (e > c) {
        *(e - 1) = '\0';
    }
    while (c != NULL) {
        c = RKGetNextKeyValue(c, key, value);
        printf("key '%s'   value '%s'  c = %c\n", key, value, c == NULL ? 0 : *c);
    }

    m = sprintf(text, "Hello World\n");
    
    return m;
}
