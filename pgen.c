#include <RadarKit.h>

int main(int argc, const char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Please supply a filename.\n");
        return EXIT_FAILURE;
    }
    
    char filename[1024];
    struct timeval s, e;

    strcpy(filename, argv[1]);
    
    RKSetWantScreenOutput(true);

    RKLog("Opening file %s ...", filename);
//    int k = 0;
//    gettimeofday(&s, NULL);
//    for (int i = 0; i < 500; i++) {
//        printf("Trial #%04d   Filename = %s\n", i, filename);
//        RKProductCollection *collection = RKProductCollectionInitWithFilename(filename);
//        k += collection->count;
//        RKProductCollectionFree(collection);
//    }
//    gettimeofday(&e, NULL);
//    double dt = RKTimevalDiff(e, s);
//    RKLog("Elapsed time = %.3f s   (%s files / sec)\n", dt, RKFloatToCommaStyleString((double)k / dt));
    
    gettimeofday(&s, NULL);
    RKFileHeader *header = (RKFileHeader *)malloc(sizeof(RKFileHeader));
    
    FILE *fid = fopen(filename, "r");
    if (fid == NULL) {
        RKLog("Error. Unable to open file %s", filename);
    }
    
    fread(header, sizeof(RKFileHeader), 1, fid);
    
    printf("header:\n");
    printf("->%s\n", RKVariableInString("desc.name", &header->desc.name, RKValueTypeString));
    printf("->desc.name = '%s'\n", header->desc.name);
    printf("\n");
    printf("->desc.latitude = %.6f\n", header->desc.latitude);
    printf("->desc.longitude = %.6f\n", header->desc.longitude);
    printf("\n");
    printf("->config.waveform = '%s'\n", header->config.waveform);
    printf("\n");
    printf("->desc.pulseCapacity = %u\n", header->desc.pulseCapacity);
    printf("->desc.pulseToRayRatio = %u\n", header->desc.pulseToRayRatio);
    printf("\n");
    printf("->dataType = '%s'\n",
           header->dataType == RKRawDataTypeFromTransceiver ? "Raw" :
           (header->dataType == RKRawDataTypeAfterMatchedFilter ? "Compressed" : "Unknown"));

    RKBuffer pulseBuffer;
    RKBuffer rayBuffer;
    RKProduct *products;
    size_t bytes;
    
    int i0;
    int i1 = 0;
    int count = 0;
    
    bytes = RKPulseBufferAlloc(&pulseBuffer, header->desc.pulseCapacity, RKMaximumPulsesPerRay);
    if (bytes == 0 || pulseBuffer == NULL) {
        RKLog("Error. Unable to allocate memory for I/Q pulses.\n");
        exit(EXIT_FAILURE);
    }
    RKLog("Pulse buffer occupies %s B  (%s pulses x %s gates)\n",
          RKUIntegerToCommaStyleString(bytes),
          RKIntegerToCommaStyleString(RKMaximumPulsesPerRay),
          RKIntegerToCommaStyleString(header->desc.pulseCapacity));

    RKMarker marker = header->config.startMarker;
    
    printf("sweep.Elevation = %.2f\n", header->config.sweepElevation);
    printf("marker = %04x / %04x\n", marker, RKMarkerScanTypePPI);

    int k;
    int m = 0;
    for (k = 0; k < 40; k++) {
        RKPulse *pulse = RKGetPulseFromBuffer(pulseBuffer, k);

        fread(&pulse->header, sizeof(RKPulseHeader), 1, fid);
        fread(RKGetComplexDataFromPulse(pulse, 0), pulse->header.downSampledGateCount * sizeof(RKComplex), 1, fid);
        fread(RKGetComplexDataFromPulse(pulse, 1), pulse->header.downSampledGateCount * sizeof(RKComplex), 1, fid);

        if ((marker & RKMarkerScanTypeMask) == RKMarkerScanTypePPI) {
            i0 = (int)floorf(pulse->header.azimuthDegrees);
        } else if ((marker & RKMarkerScanTypeMask) == RKMarkerScanTypeRHI) {
            i0 = (int)floorf(pulse->header.elevationDegrees);
        } else {
            i0 = 360 * (int)floorf(pulse->header.elevationDegrees - 0.25f) + (int)floorf(pulse->header.azimuthDegrees);
        }

        m = 0;
        if (i1 != i0 || count == RKMaximumPulsesPerRay) {
            i1 = i0;
            m = 1;
        }
        printf("pulse (EL %.2f, AZ %.2f) %s x %.2fm %d/%d %d %s\n",
               pulse->header.elevationDegrees, pulse->header.azimuthDegrees,
               RKIntegerToCommaStyleString(pulse->header.downSampledGateCount),
               pulse->header.gateSizeMeters * header->desc.pulseToRayRatio,
               i0, pulse->header.azimuthBinIndex,
               pulse->header.marker,
               m ? "*" : ""
               );
    }

    fclose(fid);
    free(header);

    gettimeofday(&e, NULL);
    double dt = RKTimevalDiff(e, s);
    RKLog("Elapsed time = %.3f s\n", dt);

    return EXIT_SUCCESS;
}