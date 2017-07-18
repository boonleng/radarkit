//
//  RKCommandCenter.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 1/5/17.
//  Copyright © 2017 Boon Leng Cheong. All rights reserved.
//

#ifndef __RadarKit_RKCommandCenter__
#define __RadarKit_RKCommandCenter__

#include <RadarKit/RKRadar.h>

#define RKCommandCenterMaxConnections 32
#define RKCommandCenterMaxRadars       4

typedef struct  rk_user {
    char               login[64];
    RKStream           access;             // Authorized access priviledge
    RKStream           streams;
    RKStream           streamsInProgress;
    double             timeLastOut;
    double             timeLastHealthOut;
    double             timeLastDisplayIQOut;
    double             timeLastIn;
    uint32_t           statusIndex;
    uint32_t           healthIndex;
    uint32_t           rayStatusIndex;
    uint32_t           pulseIndex;
    uint32_t           rayIndex;
    uint32_t           pingCount;
    uint32_t           commandCount;
    uint16_t           pulseDownSamplingRatio;
    uint16_t           rayDownSamplingRatio;
    pthread_mutex_t    mutex;
    char               string[RKMaximumStringLength];
    RKInt16C           samples[2][RKGateCount];
    RKOperator         *serverOperator;
    RKRadar            *radar;
} RKUser;

typedef struct rk_command_center {
    // User set variables
    char               name[RKNameLength];
    int                verbose;
    RKRadar            *radars[RKCommandCenterMaxRadars];
    
    // Program set variables
    bool               relayMode;
    bool               suspendHandler;
    RKServer           *server;
    int                radarCount;
    int                developerInspect;
    RKUser             users[RKCommandCenterMaxConnections];
    RKStream           relayStreams;
    RKStream           relayStreamsLevel2;
    size_t             memoryUsage;
} RKCommandCenter;

RKStream RKStringToFlag(const char *);
int RKFlagToString(char *string, RKStream);

RKCommandCenter *RKCommandCenterInit(void);
void RKCommandCenterFree(RKCommandCenter *);

void RKCommandCenterSetVerbose(RKCommandCenter *, const int);
void RKCommandCenterAddRadar(RKCommandCenter *, RKRadar *);
void RKCommandCenterRemoveRadar(RKCommandCenter *, RKRadar *);

void RKCommandCenterStart(RKCommandCenter *);
void RKCommandCenterStop(RKCommandCenter *);

#endif /* __RadarKit_RKCommandCenter__ */
