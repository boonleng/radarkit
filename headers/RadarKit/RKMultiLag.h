//
//  RKMultiLag.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 1/2/17.
//  Copyright (c) 2017 Boon Leng Cheong. All rights reserved.
//

#ifndef __RadarKit_RKMultiLag__
#define __RadarKit_RKMultiLag__

#include <RadarKit/RKFoundation.h>
#include <RadarKit/RKDSP.h>

int RKMultiLag(RKScratch *, RKPulse **, const uint16_t, const char *);

#endif /* defined(__RadarKit_RKMultiLag__) */