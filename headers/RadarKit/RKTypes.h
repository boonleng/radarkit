//
//  RKTypes.h
//  RadarKit
//
//  Created by Boon Leng Cheong on 3/17/15.
//
//

#ifndef __RadarKit_RKTypes__
#define __RadarKit_RKTypes__

#include <RadarKit/RKFoundation.h>

typedef uint8_t   RKboolean;
typedef int8_t    RKbyte;
typedef uint64_t  RKenum;

/// Fundamental unit of a (16-bit) + (16-bit) raw complex IQ sample
typedef struct RKInt16 {
    int16_t i;
    int16_t q;
} RKInt16;

/// Fundamental unit of a (float) + (float) raw complex IQ sample
typedef struct RKFloat {
    float i;
    float q;
} RKFloat;


/*!
 @typedef RKPulseHeader
 @brief Fundamental unit of the pulse header that is designed to be SIMD alignment friendly.
 Changing this structure would have a major impacts on the workingness of the entiire framework.
 It is imperative to maintain the SIMD alignment whenever new fields are added.
 @param s                      Pulse Status
 @param i                      Pulse Identity
 @param n                      Pulse Network Counter
 @param marker                 Volume / sweep / radial marker
 @param pulseWidthSampleCount  Pulse width in number of samples
 @param timeSec                UNIX time, seconds since Epoch 1970
 @param timeUSec               Microsecond part of the time
 @param timeDouble             Double representation of the time
 @param az                     Azimuth raw reading
 @param el                     Elevation raw reading
 @param prfHz                  PRF (Pulse Repetition Frequency) in Hz since last pulse
 @param prfIdx                 PRF index for multi-PRF operations
 @param gateCount              Number of usable gates in this pulse
 @param azimuthBinIndex        Azimuth bin index
 @param gateSizeMeters         Gate size in meters
 @param azimuthDegrees         Azimuth in degrees
 @param elevationDegrees       Elevation in degrees
 @param vazDps                 Velocity of azimuth in degrees per second
 @param velDps                 Velocity of azimuth in degrees per second
 */
typedef struct RKPulseHeader {
    uint32_t    s;
    uint32_t    i;
    uint32_t    n;
    uint16_t    marker;
    uint16_t    pulseWidthSampleCount;
    
    uint32_t    timeSec;
    uint32_t    timeUSec;
    double      timeDouble;
    
    uint16_t    az;
    uint16_t    el;
    uint16_t    prfHz;
    uint16_t    prfIdx;
    uint16_t    gateCount;
    uint16_t    azimuthBinIndex;
    float       gateSizeMeters;
    
    float       azimuthDegrees;
    float       elevationDegrees;
    float       vazDps;
    float       velDps;
} RKPulseHeader;

typedef struct RKInt16Pulse {
    RKPulseHeader  header;
    RKInt16        X[2][RKGateCount];
} RKInt16Pulse;

typedef struct RKFloatPulse {
    RKPulseHeader  header;
    RKFloat        X[2][RKGateCount];
} RKFloatPulse;

/*!
 @typedef RKRayHeader
 @brief Fundamental unit of the ray header.
 @param s                      Pulse Status
 @param i                      Pulse Identity
 @param n                      Pulse Network Counter
 @param marker                 Volume / sweep / radial marker
 @param reserved1              For future...
 @param startTimeSec           Start time of the ray in UNIX time
 @param startTimeUSec          Start time's microsecond portion
 @param startTimeD             Start time in double representation
 @param endTimeSec             End time of the ray in UNIX time
 @param endTimeUSec            End time's microsecond portion
 @param endTimeD               End time in double representation
 */
typedef struct RKRayHeader {
    uint32_t       s;
    uint32_t       i;
    uint32_t       n;
    uint16_t       marker;
    uint16_t       reserved1;
    
    uint32_t       startTimeSec;
    uint32_t       startTimeUSec;
    double         startTimeD;
    
    uint32_t       endTimeSec;
    uint32_t       endTimeUSec;
    double         endTimeD;
} RKRayHeader;

typedef struct RKInt16Ray {
    RKRayHeader    header;
    RKInt16        data[RKGateCount];
} RKInt16Ray;

/*!
 * @typedef RKRadar
 * @brief The big structure that holds all the necessary buffers
 * @param rawPulse
 */
typedef struct RKRadar {
    //
    // Buffers aligned to SIMD requirements
    //
    RKInt16Pulse    *rawPulses;
    RKFloatPulse    *compressedPulses;
    RKFloatPulse    *filteredCompressedPulses;
    RKInt16Ray      *rays;
    //
    //
    //
    RKenum          initFlags;
    RKboolean       rayBuffersInitialized;
    RKboolean       pulseBuffersInitialized;
    //
    //
    //
    uint32_t        memoryUsage;
    
} RKRadar;


#endif /* defined(__RadarKit_RKTypes__) */
