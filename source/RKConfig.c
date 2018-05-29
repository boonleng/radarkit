//
//  RKConfig.c
//  RadarKit
//
//  Created by Boon Leng Cheong on 1/16/17.
//  Copyright © 2017 Boon Leng Cheong. All rights reserved.
//

#include <RadarKit/RKConfig.h>

void RKConfigAdvanceEllipsis(RKConfig *configs, uint32_t *configIndex, uint32_t configBufferDepth, ...) {
    va_list args;
    va_start(args, configBufferDepth);
    return RKConfigAdvance(configs, configIndex, configBufferDepth, args);
}

void RKConfigAdvance(RKConfig *configs, uint32_t *configIndex, uint32_t configBufferDepth, va_list args) {
    uint32_t  j, k;
    char      *string;
    char      stringBuffer[RKMaxFilterCount][RKNameLength];

    for (k = 0; k < RKMaxFilterCount; k++) {
        memset(stringBuffer[k], 0, RKNameLength * sizeof(char));
    }
    
    // Use exclusive access here to prevent multiple processes trying to change RKConfig too quickly
    pthread_mutex_lock(&rkGlobalParameters.mutex);

//    RKConfig *newConfig = &configs[RKNextModuloS(*configIndex, configBufferDepth)];
//    RKConfig *oldConfig = &configs[*configIndex];

    RKConfig *newConfig = &configs[*configIndex];
    RKConfig *oldConfig = &configs[RKPreviousModuloS(*configIndex, configBufferDepth)];

    const uint64_t configId = newConfig->i + configBufferDepth;
    
    //RKLog("--- RKConfigAdvance()   Id = %llu ---\n", configId);

    uint32_t filterCount;
    RKFilterAnchor *filterAnchor;
    RKWaveformCalibration *waveformCal;
    RKFloat (*ZCal)[2];

    // Copy everything
    memcpy(newConfig, oldConfig, sizeof(RKConfig));

    uint32_t key = va_arg(args, RKConfigKey);

    // Modify the values based on the supplied keys
    while (key != RKConfigKeyNull) {
        switch (key) {
            case RKConfigKeySweepElevation:
                newConfig->sweepElevation = (float)va_arg(args, double);
                break;
            case RKConfigKeySweepAzimuth:
                newConfig->sweepAzimuth = (float)va_arg(args, double);
                break;
            case RKConfigKeyPositionMarker:
                newConfig->startMarker = va_arg(args, RKMarker);
				sprintf(stringBuffer[0], "New Sweep   EL %.2f°   AZ %.2f°  %s   filterCount = %d",
                        newConfig->sweepElevation,
                        newConfig->sweepAzimuth,
                        newConfig->startMarker & RKMarkerPPIScan ? "PPI" : (newConfig->startMarker & RKMarkerRHIScan ? "RHI" : "UNK"),
                        newConfig->filterCount);
                break;
            case RKConfigKeyPRF:
                newConfig->prf[0] = va_arg(args, uint32_t);
                sprintf(stringBuffer[0], "PRF = %s Hz", RKIntegerToCommaStyleString(newConfig->prf[0]));
                break;
            case RKConfigKeyDualPRF:
                newConfig->prf[0] = va_arg(args, uint32_t);
                newConfig->prf[1] = va_arg(args, uint32_t);
				sprintf(stringBuffer[0], "Dual PRF = %s / %s Hz", RKIntegerToCommaStyleString(newConfig->prf[0]), RKIntegerToCommaStyleString(newConfig->prf[1]));
                break;
            case RKConfigKeyGateCount:
                newConfig->gateCount[0] = va_arg(args, uint32_t);
				sprintf(stringBuffer[0], "GateCount = %s", RKIntegerToCommaStyleString(newConfig->gateCount[0]));
                break;
            case RKConfigKeyWaveformId:
                // ???
                //RKParseCommaDelimitedValues(newConfig->waveformId, RKValueTypeUInt32, RKMaxFilterCount, string);
                break;
            case RKConfigKeyVCPDefinition:
                string = va_arg(args, char *);
                if (string == NULL) {
                    sprintf(stringBuffer[0], "VCP = (NULL)\n");
                } else {
                    sprintf(stringBuffer[0], "VCP = %s\n", string);
                    strncpy(newConfig->vcpDefinition, string, RKNameLength - 1);
                }
                break;
            case RKConfigKeyNoise:
                newConfig->noise[0] = (RKFloat)va_arg(args, double);
                newConfig->noise[1] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "Noise = %.2f %.2f ADU^2", newConfig->noise[0], newConfig->noise[1]);
                break;
            case RKConfigKeySystemZCal:
                newConfig->systemZCal[0] = (RKFloat)va_arg(args, double);
                newConfig->systemZCal[1] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "SystemZCal = %.2f %.2f dB", newConfig->systemZCal[0], newConfig->systemZCal[1]);
                break;
			case RKConfigKeySystemDCal:
				newConfig->systemDCal = (RKFloat)va_arg(args, double);
				sprintf(stringBuffer[0], "SystemDCal = %.2f dB", newConfig->systemDCal);
				break;
            case RKConfigKeyZCal:
                newConfig->ZCal[0][0] = (RKFloat)va_arg(args, double);
                newConfig->ZCal[1][0] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "ZCal = %.2f %.2f dB", newConfig->ZCal[0][0], newConfig->ZCal[1][0]);
                break;
            case RKConfigKeyDCal:
                newConfig->DCal[0] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "DCal = %.2f dB", newConfig->DCal[0]);
                break;
            case RKConfigKeyPCal:
                newConfig->PCal[0] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "PCal = %.2f rad", newConfig->PCal[0]);
                break;
            case RKConfigKeyZCal2:
                newConfig->ZCal[0][1] = (RKFloat)va_arg(args, double);
                newConfig->ZCal[1][1] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "ZCal[2] = %.2f %.2f %.2f %.2f dB", newConfig->ZCal[0][0], newConfig->ZCal[0][1], newConfig->ZCal[1][0], newConfig->ZCal[1][1]);
                break;
            case RKConfigKeyDCal2:
                newConfig->DCal[1] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "DCal[2] = %.2f %.2f dB", newConfig->DCal[0], newConfig->DCal[1]);
                break;
            case RKConfigKeyPCal2:
                newConfig->PCal[1] = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "PCal[2] = %.2f %.2f rad", newConfig->PCal[0], newConfig->PCal[1]);
                break;
            case RKConfigKeyWaveformCalibration:
                waveformCal = (RKWaveformCalibration *)va_arg(args, void *);
                for (j = 0; j < waveformCal->count; j++) {
                    newConfig->ZCal[j][0] = waveformCal->ZCal[j][0];
                    newConfig->ZCal[j][1] = waveformCal->ZCal[j][1];
                    newConfig->DCal[j] = waveformCal->DCal[j];
                    newConfig->PCal[j] = waveformCal->PCal[j];
                }
                break;
            case RKConfigKeyZCals:
                // Calibration constants in [filterIndex][H/V] specified as N, ZCal[0][H], ZCal[0][V], ZCal[1][H], ZCal[1][V], ..., ZCal[N-1][H], ZCal[N-1][V]
                filterCount = va_arg(args, int);
                ZCal = (RKFloat(*)[2])va_arg(args, void *);
                if (filterCount == 0 || ZCal == NULL) {
                    break;
                }
                for (j = 0; j < filterCount; j++) {
                    newConfig->ZCal[j][0] = ZCal[j][0];
                    newConfig->ZCal[j][1] = ZCal[j][1];
                    sprintf(stringBuffer[j], "ZCal[%d] = (%.2f, %.2f) dB", j, newConfig->ZCal[j][0], newConfig->ZCal[j][1]);
                }
                break;
            case RKConfigKeySNRThreshold:
                newConfig->SNRThreshold = (RKFloat)va_arg(args, double);
                sprintf(stringBuffer[0], "SNRThreshold = %.2f dB", newConfig->SNRThreshold);
                break;
            case RKConfigKeyWaveform:
                strncpy(newConfig->waveform, va_arg(args, char *), RKNameLength - 1);
                sprintf(stringBuffer[0], "Waveform = '%s'", newConfig->waveform);
                break;
            case RKConfigKeyFilterCount:
                newConfig->filterCount = (uint8_t)va_arg(args, int);
                sprintf(stringBuffer[0], "Filter Count = %d", newConfig->filterCount);
                break;
            case RKConfigKeyFilterAnchor:
                memcpy(&newConfig->filterAnchors[0], va_arg(args, void *), sizeof(RKFilterAnchor));
                sprintf(stringBuffer[0], "Filter1 @ i:%d, o:%d, d:%d   %.2f dB",
                        newConfig->filterAnchors[0].inputOrigin,
                        newConfig->filterAnchors[0].outputOrigin,
                        newConfig->filterAnchors[0].maxDataLength,
                        newConfig->filterAnchors[0].sensitivityGain);
                break;
            case RKConfigKeyFilterAnchor2:
                memcpy(&newConfig->filterAnchors[1], va_arg(args, void *), sizeof(RKFilterAnchor));
                sprintf(stringBuffer[0], "Filter2 @ i:%d, o:%d, d:%d   %.2f dB",
                        newConfig->filterAnchors[1].inputOrigin,
                        newConfig->filterAnchors[1].outputOrigin,
                        newConfig->filterAnchors[1].maxDataLength,
                        newConfig->filterAnchors[1].sensitivityGain);
                break;
            case RKConfigKeyFilterAnchors:
                filterCount = va_arg(args, int);
                filterAnchor = (RKFilterAnchor *)va_arg(args, void *);
                if (filterCount == 0 || filterAnchor == NULL) {
                    break;
                }
                memcpy(newConfig->filterAnchors, filterAnchor, filterCount * sizeof(RKFilterAnchor));
                for (j = 0; j < filterCount; j++) {
                    sprintf(stringBuffer[j], "Filter[%d] @ i:%d, o:%d, d:%d   %.2f dB",
                            j,
                            newConfig->filterAnchors[j].inputOrigin,
                            newConfig->filterAnchors[j].outputOrigin,
                            newConfig->filterAnchors[j].maxDataLength,
                            newConfig->filterAnchors[j].sensitivityGain);
                    
                }
                break;
            default:
                sprintf(stringBuffer[0], "Key %d not understood.", key);
                break;
        }
        for (k = 0; k < RKMaxFilterCount; k++) {
            if (strlen(stringBuffer[k])) {
                RKLog("%s<ParameterKeeper>%s C%02d %s   ConfigId = %s\n",
                      rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorConfig) : "",
                      rkGlobalParameters.showColor ? RKNoColor : "",
                      *configIndex,
                      stringBuffer[k],
                      RKIntegerToCommaStyleString(configId));
                stringBuffer[k][0] = '\0';
            }
        }
        // Get the next key
        key = va_arg(args, RKConfigKey);
    }

    va_end(args);

    // Update the identifier and the buffer index
    newConfig->i = configId;
    *configIndex = RKNextModuloS(*configIndex, configBufferDepth);
    
    pthread_mutex_unlock(&rkGlobalParameters.mutex);
}

RKConfig *RKConfigWithId(RKConfig *configs, uint32_t configBufferDepth, uint64_t id) {
    int k = configBufferDepth;
    while (k > 0) {
        k--;
        if (configs[k].i == id) {
            return &configs[k];
        }
    }
    return NULL;
}
