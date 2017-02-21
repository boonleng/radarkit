//
//  RKWaveform.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 2/19/17.
//  Copyright © 2017 Boon Leng Cheong. All rights reserved.
//

#ifndef __RadarKit_Waveform__
#define __RadarKit_Waveform__

#include <RadarKit/RKFoundation.h>

#define RKWaveformDefaultDepth 1024

typedef uint32_t RKWaveformType;
enum RKWaveformType {
    RKWaveformTypeNone,
    RKWaveformTypeSingle,
    RKWaveformTypeFrequencyHopping,
    RKWaveformTypeTimeFrequencyMultiplex
};

typedef struct rk_waveform {
    int             count;                                      // Number of groups
    int             depth;                                      // Maximum number of samples
    RKWaveformType  type;                                       // Various type of waveforms
    double          omega[RKMaxMatchedFilterGroupCount];        // Sub-carrier of RKWaveformTypeFrequencyHopping
    RKComplex       *samples[RKMaxMatchedFilterGroupCount];     // Samples up to amplitude of 1.0
    RKInt16C        *iSamples[RKMaxMatchedFilterGroupCount];    // 16-bit full-scale equivalent of the waveforms
} RKWaveform;

RKWaveform *RKWaveformInitWithCountAndDepth(const int count, const int depth);
RKWaveform *RKWaveformInit(void);
void RKWaveformFree(RKWaveform *);

void RKWaveformMakeHops(RKWaveform *waveform, const double fs, const double bandwidth);

#endif
