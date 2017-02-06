//
//  RKFile.c
//  RadarKit
//
//  Created by Boon Leng Cheong on 3/18/15.
//  Copyright (c) 2015 Boon Leng Cheong. All rights reserved.
//

#include <RadarKit/RKFile.h>

#pragma mark - Internal Functions

void RKFileEngineSetCacheSize(RKFileEngine *engine, uint32_t size) {
    if (engine->cacheSize == size) {
        return;
    }
    if (engine->cache != NULL) {
        free(engine->cache);
        engine->memoryUsage -= engine->cacheSize;
    }
    engine->cacheSize = size;
    if (posix_memalign((void **)&engine->cache, RKSIMDAlignSize, engine->cacheSize)) {
        RKLog("%s Error. Unable to allocate cache.", engine->name);
        exit(EXIT_FAILURE);
    }
    engine->memoryUsage += engine->cacheSize;
}

uint32_t RKFileEngineCacheWrite(RKFileEngine *engine, const void *payload, const uint32_t size) {
    if (size == 0) {
        return 0;
    }
    uint32_t remainingSize = size;
    uint32_t lastChunkSize = 0;
    uint32_t writtenSize = 0;
    //
    // Method:
    //
    // If the remainder of cache is less than then payload size, copy the whatever that fits, called it lastChunkSize
    // Then, the last part of the payload (starting lastChunkSize) should go into the cache. Otherwise, just
    // write out the remainig payload entirely, leaving the cache empty.
    //
    if (engine->cacheWriteIndex + remainingSize >= engine->cacheSize) {
        lastChunkSize = engine->cacheSize - engine->cacheWriteIndex;
        memcpy(engine->cache + engine->cacheWriteIndex, payload, lastChunkSize);
        remainingSize = size - lastChunkSize;
        writtenSize = (uint32_t)write(engine->fd, engine->cache, engine->cacheSize);
        if (writtenSize != engine->cacheSize) {
            RKLog("%s Error in write().   writtenSize = %s\n", RKIntegerToCommaStyleString((long)writtenSize));
        }
        engine->cacheWriteIndex = 0;
        if (remainingSize >= engine->cacheSize) {
            writtenSize += (uint32_t)write(engine->fd, (char *)(payload + lastChunkSize), remainingSize);
            return writtenSize;
        }
    }
    memcpy(engine->cache + engine->cacheWriteIndex, payload + lastChunkSize, remainingSize);
    engine->cacheWriteIndex += remainingSize;
    return writtenSize;
}

uint32_t RKFileEngineCacheFlush(RKFileEngine *engine) {
    if (engine->cacheWriteIndex == 0) {
        return 0;
    }
    return (uint32_t)write(engine->fd, engine->cache, engine->cacheWriteIndex);
}

#pragma mark - Implementation

void RKFileEngineUpdateStatusString(RKFileEngine *engine) {
    int i;
    char *string;

    // Status string
    string = engine->statusBuffer[engine->statusBufferIndex];

    // Always terminate the end of string buffer
    string[RKMaximumStringLength - 1] = '\0';
    string[RKMaximumStringLength - 2] = '#';

    // Use RKStatusBarWidth characters to draw a bar
    i = *engine->pulseIndex * (RKStatusBarWidth + 1) / engine->pulseBufferDepth;
    memset(string, 'F', i);
    memset(string + i, '.', RKStatusBarWidth - i);

    // Engine lag
    i = RKStatusBarWidth + snprintf(string + RKStatusBarWidth, RKMaximumStringLength - RKStatusBarWidth, " | %s%02.0f%s",
                                    rkGlobalParameters.showColor ? RKColorLag(engine->lag) : "",
                                    99.9f * engine->lag,
                                    rkGlobalParameters.showColor ? RKNoColor : "");
    engine->statusBufferIndex = RKNextModuloS(engine->statusBufferIndex, RKBufferSSlotCount);
}

void *pulseRecorder(void *in) {
    RKFileEngine *engine = (RKFileEngine *)in;

    int j, k, s;

    struct timeval t0, t1;

    RKPulse *pulse;
    RKConfig *config;

    RKLog("%s Started.   mem = %s B   pulseIndex = %d\n", engine->name, RKIntegerToCommaStyleString(engine->memoryUsage), *engine->pulseIndex);

    gettimeofday(&t1, 0); t1.tv_sec -= 1;

    engine->state = RKFileEngineStateActive;

    j = 0;
    k = 0;
    while (engine->state == RKFileEngineStateActive) {
        // The pulse
        pulse = RKGetPulse(engine->pulseBuffer, k);
        // Wait until the buffer is advanced
        s = 0;
        while (k == *engine->pulseIndex && engine->state & RKFileEngineStateActive) {
            usleep(10000);
            if (++s % 100 == 0 && engine->verbose > 1) {
                RKLog("%s sleep 1/%.1f s   k = %d   pulseIndex = %d   header.s = 0x%02x\n",
                      engine->name, (float)s * 0.01f, k, *engine->pulseIndex, pulse->header.s);
            }
        }
        // Wait until the pulse is completely processed
        while (!(pulse->header.s & RKPulseStatusProcessed) && engine->state == RKFileEngineStateActive) {
            usleep(1000);
            if (++s % 100 == 0 && engine->verbose > 1) {
                RKLog("%s sleep 2/%.1f s   k = %d   pulseIndex = %d   header.s = 0x%02x\n",
                      engine->name, (float)s * 0.01f, k , *engine->pulseIndex, pulse->header.s);
            }
        }
        if (engine->state != RKFileEngineStateActive) {
            break;
        }
        // Lag of the engine
        engine->lag = fmodf(((float)*engine->pulseIndex + engine->pulseBufferDepth - k) / engine->pulseBufferDepth, 1.0f);
        if (!isfinite(engine->lag)) {
            RKLog("%s %d + %d - %d = %d",
                  engine->name, *engine->pulseIndex, engine->pulseBufferDepth, k, *engine->pulseIndex + engine->pulseBufferDepth - k, engine->lag);
        }

        // Assess the configIndex
        if (j != pulse->header.configIndex) {
            j = pulse->header.configIndex;
            config = &engine->configBuffer[pulse->header.configIndex];

            // Close the current file
            RKLog("Closing file ...\n");
        }

        // Actual cache and write happen here.
        

        // Log a message if it has been a while
        gettimeofday(&t0, NULL);
        if (RKTimevalDiff(t0, t1) > 0.05) {
            t1 = t0;
            RKFileEngineUpdateStatusString(engine);
        }

        // Update pulseIndex for the next watch
        k = RKNextModuloS(k, engine->pulseBufferDepth);
    }
    return NULL;
}

#pragma mark - Life Cycle

RKFileEngine *RKFileEngineInit(void) {
    RKFileEngine *engine = (RKFileEngine *)malloc(sizeof(RKFileEngine));
    if (engine == NULL) {
        RKLog("Error. Unable to allocate RKFileEngine.\r");
        exit(EXIT_FAILURE);
    }
    memset(engine, 0, sizeof(RKFileEngine));
    sprintf(engine->name, "%s<RawDataRecorder>%s",
            rkGlobalParameters.showColor ? RKGetBackgroundColor() : "", rkGlobalParameters.showColor ? RKNoColor : "");
    RKFileEngineSetCacheSize(engine, 32 * 1024 * 1024);
    engine->state = RKFileEngineStateAllocated;
    engine->memoryUsage = sizeof(RKFileEngine) + engine->cacheSize;
    return engine;
}

void RKFileEngineFree(RKFileEngine *engine) {
    if (engine->state == RKFileEngineStateActive) {
        RKFileEngineStop(engine);
    }
    free(engine->cache);
    free(engine);
}

#pragma mark - Properties

void RKFileEngineSetVerbose(RKFileEngine *engine, const int verbose) {
    engine->verbose = verbose;
}

void RKFileEngineSetInputOutputBuffers(RKFileEngine *engine,
                                       RKConfig *configBuffer, uint32_t *configIndex, const uint32_t configBufferDepth,
                                       RKBuffer pulseBuffer,   uint32_t *pulseIndex,  const uint32_t pulseBufferDepth) {
    engine->configBuffer      = configBuffer;
    engine->configIndex       = configIndex;
    engine->configBufferDepth = configBufferDepth;
    engine->pulseBuffer       = pulseBuffer;
    engine->pulseIndex        = pulseIndex;
    engine->pulseBufferDepth  = pulseBufferDepth;
}

#pragma mark - Interactions

int RKFileEngineStart(RKFileEngine *engine) {
    engine->state = RKFileEngineStateActivating;
    if (pthread_create(&engine->tidPulseRecorder, NULL, pulseRecorder, engine) != 0) {
        RKLog("%s Error. Failed to start.\n", engine->name);
        return RKResultFailedToStartPulseRecorder;
    }
    while (engine->state < RKFileEngineStateActive) {
        usleep(10000);
    }
    return RKResultSuccess;
}

int RKFileEngineStop(RKFileEngine *engine) {
    if (engine->state == RKFileEngineStateActive) {
        engine->state = RKFileEngineStateDeactivating;
        pthread_join(engine->tidPulseRecorder, NULL);
    } else {
        return RKResultEngineDeactivatedMultipleTimes;
    }
    engine->state = RKFileEngineStateSleep;
    return RKResultSuccess;
}

char *RKFileEngineStatusString(RKFileEngine *engine) {
    return engine->statusBuffer[RKPreviousModuloS(engine->statusBufferIndex, RKBufferSSlotCount)];
}
