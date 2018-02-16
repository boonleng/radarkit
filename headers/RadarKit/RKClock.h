//
//  RKClock.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 1/5/17.
//  Copyright © 2017 Boon Leng Cheong. All rights reserved.
//

#ifndef __RadarKit_Clock_h__
#define __RadarKit_Clock_h__

#include <RadarKit/RKFoundation.h>

// A clock derived from counter and request time
#define RKClockDefaultBufferDepth        2000
#define RKClockDefaultStride             1000
#define RKClockAWhile                    300.0

typedef struct rk_clock {
    // User set parameters
    double           offsetSeconds;
    char             name[RKNameLength];
    int              verbose;
    bool             autoSync;
    bool             hasWisdom;                   // User provided dudt
	bool             infoShown;                   // Show b value
    bool             highPrecision;               // High precision mode
    bool             useInternalReference;        // Use internal reference u
    uint32_t         size;                        // User changeable depth
	uint32_t         block;                       // Block size of data during burst transfers
    uint32_t         stride;                      // Size to compute average
    uint64_t         tic;                         // An internal tic in case user doesn't obey the rule

    // Program set parameters
    struct timeval   *tBuffer;                    // The time which a request was made (dirty)
    double           *xBuffer;                    // A double representation of timeval (dirty)
    double           *uBuffer;                    // Driving reference (clean)
    double           *yBuffer;                    // Predicted time
    double           *zBuffer;                    // History of dx/du
    
    double           a;                           // Major coefficient
    double           b;                           // Minor coefficient
    
    uint32_t         index;
    uint64_t         count;
    double           initDay;
    double           initTime;
    double           latestTime;
    double           typicalPeriod;
    double           x0;
    double           u0;
    double           dx;
    double           sum_x0;
    double           sum_u0;
    
} RKClock;

RKClock *RKClockInitWithSize(const uint32_t, const uint32_t);
RKClock *RKClockInitWitName(const char *);
RKClock *RKClockInit(void);
void RKClockFree(RKClock *);

void RKClockSetName(RKClock *, const char *);
void RKClockSetVerbose(RKClock *, const int);
void RKClockSetManualSync(RKClock *);
void RKClockSetOffset(RKClock *, const double);
void RKClockSetDxDu(RKClock *, const double);
void RKClockSetDuDx(RKClock *, const double);
void RKClockSetHighPrecision(RKClock *, const bool);

//void RKClockSync(RKClock *clock, const double u);

double RKClockGetTime(RKClock *, const double, struct timeval *);

void RKClockReset(RKClock *);

#endif /* __RadarKit_RKClock_h__ */
