//
//  RKSweep.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 1/15/17.
//  Copyright © 2017 Boon Leng Cheong. All rights reserved.
//

#ifndef __RadarKit_Sweep__
#define __RadarKit_Sweep__

#define RKSweepScratchSpaceDepth 4

#include <RadarKit/RKFoundation.h>
#include <RadarKit/RKFileManager.h>
#include <netcdf.h>

typedef struct rk_sweep_scratch {
    char                             symbol[8];
    RKName                           name;
    RKName                           unit;
    RKName                           colormap;
    float                            *array1D;
    float                            *array2D;
    char                             filelist[RKMaximumStringLength];              // It's really handleFilesScript + file list
    char                             filename[RKMaximumPathLength];
    char                             summary[RKMaximumStringLength];
    RKRay                            *rays[RKMaximumRaysPerSweep];
    uint32_t                         rayCount;
} RKSweepScratchSpace;

typedef struct rk_sweep_engine RKSweepEngine;

struct rk_sweep_engine {
    // User set variables
    RKName                           name;
    RKRadarDesc                      *radarDescription;
    RKConfig                         *configBuffer;
    uint32_t                         *configIndex;
    RKBuffer                         rayBuffer;
    uint32_t                         *rayIndex;
    uint8_t                          verbose;
    bool                             doNotWrite;
    bool                             hasHandleFilesScript;
    bool                             handleFilesScriptProducesTgz;
    bool                             handleFilesScriptProducesZip;
    char                             handleFilesScript[RKMaximumPathLength];
    RKFileManager                    *fileManager;
    uint32_t                         userProductTimeoutSeconds;

    // Program set variables
    pthread_t                        tidRayGatherer;
    RKSweepScratchSpace              scratchSpaces[RKSweepScratchSpaceDepth];
    uint8_t                          scratchSpaceIndex;
    RKProduct                        products[RKMaximumProductCount];
    pthread_mutex_t                  productMutex;

    // Status / health
    uint32_t                         processedRayIndex;
    char                             statusBuffer[RKBufferSSlotCount][RKMaximumStringLength];
    uint32_t                         statusBufferIndex;
    RKEngineState                    state;
    uint64_t                         tic;
    float                            lag;
    uint32_t                         almostFull;
    size_t                           memoryUsage;
};

RKSweepEngine *RKSweepEngineInit(void);
void RKSweepEngineFree(RKSweepEngine *);

void RKSweepEngineSetVerbose(RKSweepEngine *, const int verbose);
void RKSweepEngineSetInputOutputBuffer(RKSweepEngine *, RKRadarDesc *, RKFileManager *,
                                       RKConfig *configBuffer, uint32_t *configIndex,
                                       RKBuffer rayBuffer,     uint32_t *rayIndex);
void RKSweepEngineSetDoNotWrite(RKSweepEngine *, const bool);
void RKSweepEngineSetProductTimeout(RKSweepEngine *, const uint32_t);
void RKSweepEngineSetHandleFilesScript(RKSweepEngine *engine, const char *script, const bool expectTgz);

int RKSweepEngineStart(RKSweepEngine *);
int RKSweepEngineStop(RKSweepEngine *);

RKProductId RKSweepEngineRegisterProduct(RKSweepEngine *, RKProductDesc);
int RKSweepEngineUnregisterProduct(RKSweepEngine *, RKProductId);
RKFloat *RKSweepEngineGetBufferForProduct(RKSweepEngine *, RKSweep *, RKProductId);
int RKSweepEngineReportProduct(RKSweepEngine *, RKSweep *, RKProductId);

RKSweep *RKSweepCollect(RKSweepEngine *, const uint8_t);
RKSweep *RKSweepRead(const char *);
int RKSweepFree(RKSweep *);

#endif /* RKSweep_h */
