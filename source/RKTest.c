//
//  RKTest.c
//  RadarKit
//
//  Created by Boonleng Cheong on 10/25/16.
//  Copyright (c) 2016 Boonleng Cheong. All rights reserved.
//

#include <RadarKit/RKTest.h>
#include <getopt.h>

#define RKFMT                               "%5d"
#define RKSIMD_TEST_DESC_FORMAT             "%65s"
#define RKSIMD_TEST_TIME_FORMAT             "%0.4f"
#define RKSIMD_TEST_RESULT(clr, str, res)   clr ? \
    printf(RKSIMD_TEST_DESC_FORMAT " : %s.\033[0m\n", str, res ? "\033[32msuccessful" : "\033[31mfailed") : \
    printf(RKSIMD_TEST_DESC_FORMAT " : %s.\n", str, res ? "successful" : "failed");
#define OXSTR(x)                       x ? "\033[32mo\033[0m" : "\033[31mx\033[0m"
#define PEDESTAL_SAMPLING_TIME         0.01
#define HEALTH_RELAY_SAMPLING_TIME     0.1

#define TEST_RESULT(clr, str, res)   clr ? \
printf("%s %s\033[0m\n", str, res ? "\033[32mokay" : "\033[31mtoo high") : \
printf("%s %s\n", str, res ? "okay" : "too high");

// Make some private functions available

int prepareScratch(RKScratch *);
int makeRayFromScratch(RKScratch *, RKRay *);
size_t RKScratchAlloc(RKScratch **space, const uint32_t capacity, const uint8_t lagCount, const uint8_t fftOrder, const bool);
void RKScratchFree(RKScratch *);

#pragma mark - Static Functions

static void RKTestCallback(void *in) {
    RKFileMonitor *engine = (RKFileMonitor *)in;
    RKLog("%s I am a callback function.\n", engine->name);
}

// Compare two filenames alphabetically
static int string_cmp_by_filename(const void *a, const void *b) {
    return strcmp((char *)a, (char *)b);
}

#pragma mark - Test Wrapper and Help Text

char *RKTestByNumberDescription(const int indent) {
    static char text[4096];
    char helpText[] =
    " 0 - Show types\n"
    " 1 - Show colors\n"
    " 2 - Pretty strings\n"
    " 3 - Modulo-math macros\n"
    " 4 - Parse comma delimited values\n"
    " 5 - Parse values in a JSON string\n"
    " 6 - File manager - RKFileManagerInit()\n"
    " 7 - Read the preference file - RKPreferenceInit()\n"
    " 8 - Count files using RKCountFilesInPath()\n"
    " 9 - File monitor module - RKFileMonitor()\n"
    "10 - Internet monitor module - RKHostMonitorInit()\n"
    "11 - Initialize a simple radar system - RKRadarInit()\n"
    "12 - Convert a temperature reading to status\n"
    "13 - Get a country name from position\n"
    "14 - Generating text for buffer overview\n"
    "15 - Read a .nc file using RKSweepRead(); -T15 FILENAME\n"
    "16 - Read a .nc file using RKProductRead(); -T16 FILENAME\n"
    "17 - Read .nc files using RKProductCollectionInitWithFilename(); -T17 FILENAME\n"
    "18 - Write a .nc file using RKProductFileWriterNC()\n"
    "19 - RKTestReviseLogicalValues()\n"
    "20 - Reading a .rkc file; -T20 FILENAME\n"
    "21 - RKCommandQueue unit test\n"
    "\n"
    "30 - SIMD quick test\n"
    "31 - SIMD test with numbers shown\n"
    "32 - Show window types\n"
    "33 - Hilbert transform\n"
    "34 - Optimize FFT performance and generate an fft-wisdom file\n"
    "35 - Show ring filter coefficients\n"
    "36 - Show ramp types\n"
    "\n"
    "40 - Make a frequency hopping sequence\n"
    "41 - Make a TFM waveform\n"
    "42 - Generate a waveform file\n"
    "43 - Test waveform down-sampling\n"
    "44 - Test showing built-in waveform properties\n"
    "45 - Test showing waveform properties; -T45 WAVEFORM_FILE\n"
    "\n"
    "50 - Pulse compression using simple cases\n"
    "51 - Calculating one ray using the Pulse Pair method\n"
    "52 - Calculating one ray using the Pulse Pair Hop method\n"
    "53 - Calculating one ray using the Multi-Lag method with L = 2\n"
    "54 - Calculating one ray using the Multt-Lag method with L = 3\n"
    "55 - Calculating one ray using the Multi-Lag method with L = 4\n"
    "56 - Calculating one ray using the Spectral Moment method (** WIP **)\n"
    "\n"
    "60 - Measure the speed of SIMD calculations\n"
    "61 - Measure the speed of pulse compression\n"
    "62 - Measure the speed of various moment methods\n"
    "63 - Measure the speed of cached write\n";
    RKIndentCopy(text, helpText, indent);
    if (strlen(text) > 3000) {
        fprintf(stderr, "Warning. Approaching limit. (%lu)\n", strlen(text));
    }
    return text;
}

void RKTestByNumber(const int number, const void *arg) {
    RKSetWantScreenOutput(true);
    switch (number) {
        case 0:
            RKShowTypeSizes();
            printf("\n");
            RKNetworkShowPacketTypeNumbers();
            break;
        case 1:
            RKTestTerminalColors();
            break;
        case 2:
            RKTestPrettyStrings();
            break;
        case 3:
            RKTestModuloMath();
            break;
        case 4:
            RKTestParseCommaDelimitedValues();
            break;
        case 5:
            RKTestParseJSONString();
            break;
        case 6:
            RKTestFileManager();
            break;
        case 7:
            RKTestPreferenceReading();
            break;
        case 8:
            RKTestCountFiles();
            break;
        case 9:
            RKTestFileMonitor();
            break;
        case 10:
            RKTestHostMonitor();
            break;
        case 11:
            RKTestInitializingRadar();
            break;
        case 12:
            RKTestTemperatureToStatus();
            break;
        case 13:
            RKTestGetCountry();
            break;
        case 14:
            RKTestBufferOverviewText((char *)arg);
            break;
        case 15:
            if (arg == NULL) {
                RKLog("No filename given.\n");
                exit(EXIT_FAILURE);
            }
            RKTestSweepRead((char *)arg);
            break;
        case 16:
            if (arg == NULL) {
                RKLog("No filename given.\n");
                exit(EXIT_FAILURE);
            }
            RKTestProductRead((char *)arg);
            break;
        case 17:
            if (arg == NULL) {
                RKLog("No filename given.\n");
                exit(EXIT_FAILURE);
            }
            RKProductCollectionInitWithFilename((char *)arg);
            break;
        case 18:
            RKTestProductWrite();
            break;
        case 19:
            RKTestReviseLogicalValues();
            break;
        case 20:
            if (arg == NULL) {
                RKLog("No filename given.\n");
                exit(EXIT_FAILURE);
            }
            RKTestReadIQ((char *)arg);
            break;
        case 21:
            RKTestPreparePath();
            break;
        case 22:
            RKTestHealthOverviewText((char *)arg);
            break;
        case 30:
            RKTestSIMD(RKTestSIMDFlagNull);
            break;
        case 31:
            RKTestSIMD(RKTestSIMDFlagShowNumbers);
            break;
        case 32:
            RKTestWindow();
            break;
        case 33:
            RKTestHilbertTransform();
            break;
        case 34:
            RKTestWriteFFTWisdom();
            break;
        case 35:
            RKTestRingFilterShowCoefficients();
            break;
        case 36:
            RKTestCommandQueue();
            break;
        case 37:
            RKTestRamp();
            break;
        case 40:
            RKTestMakeHops();
            break;
        case 41:
            RKTestWaveformTFM();
            break;
        case 42:
            RKTestWaveformWrite();
            break;
        case 43:
            RKTestWaveformDownsampling();
            break;
        case 44:
            RKTestWaveformShowProperties();
            break;
        case 45:
            if (arg == NULL) {
                RKLog("No filename given.\n");
                exit(EXIT_FAILURE);
            }
            RKTestWaveformShowUserWaveformProperties((char *)arg);
            break;
        case 50:
            RKTestPulseCompression(RKTestFlagVerbose | RKTestFlagShowResults);
            break;
        case 51:
            RKTestOneRay(RKPulsePair, 0);
            break;
        case 52:
            RKTestOneRay(RKPulsePairHop, 0);
            break;
        case 53:
            RKTestOneRay(RKMultiLag, 2);
            break;
        case 54:
            RKTestOneRay(RKMultiLag, 3);
            break;
        case 55:
            RKTestOneRay(RKMultiLag, 4);
            break;
        case 56:
            RKTestOneRay(RKSpectralMoment, 0);
            break;
        case 57:
            RKTestOneRaySpectra(RKSpectralMoment, 0);
            break;
        case 60:
            RKTestSIMD(RKTestSIMDFlagPerformanceTestAll);
            break;
        case 61:
            RKTestPulseCompressionSpeed();
            break;
        case 62:
            RKTestMomentProcessorSpeed();
            break;
        case 63:
            RKTestCacheWrite();
            break;
        case 99:
            RKTestExperiment();
            break;
        default:
            RKLog("Test %d is invalid.\n", number);
            break;
    }
}

#pragma mark - Fundamental Functions

void RKTestTerminalColors(void) {
    SHOW_FUNCTION_NAME
    for (int k = 0; k < 17; k++) {
        printf("%s<BackgroundColor %2d>%s    %s<Color %2d>%s\n", RKGetBackgroundColorOfIndex(k), k, RKNoColor, RKGetColorOfIndex(k), k, RKNoColor);
    }
    printf("\n");
    int c;
    for (int i = 0; i < 6; i++) {
        for (int k = 0; k < 6; k++) {
            for (int j = 0; j < 6; j++) {
                c = k * 100 + j * 10 + i;
                printf("%s %03d %s", RKGetBackgroundColorOfCubeIndex(c), c, RKNoColor);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void RKTestPrettyStrings(void) {
    SHOW_FUNCTION_NAME
    int k;
    RKFloat f;
    printf("\n");
    for (k = 1020; k < (1 << 30); k += k ^ 2) {
        printf("k = %11d -> %s\n", k, RKIntegerToCommaStyleString(k));
    }
    f = +INFINITY;  printf("f = %11.2f -> %s\n", f, RKFloatToCommaStyleString(f));
    f = -INFINITY;  printf("f = %11.2f -> %s\n", f, RKFloatToCommaStyleString(f));
    f = NAN;        printf("f = %11.2f -> %s\n", f, RKFloatToCommaStyleString(f));
    f = 1000.0f;    printf("f = %11.3f -> %s\n", f, RKFloatToCommaStyleString(f));
    f = 20000.0f;   printf("f = %11.3f -> %s\n", f, RKFloatToCommaStyleString(f));
    f = 400000.0f;  printf("f = %11.3f -> %s\n", f, RKFloatToCommaStyleString(f));
    f = 1234567.8f; printf("f = %11.3f -> %s\n", f, RKFloatToCommaStyleString(f));
    printf("\n");
    char *c;
    bool tf = true; printf("%s\n", RKVariableInString("tf", &tf, RKValueTypeBool));
    int8_t i8 = 100; printf("%s\n", RKVariableInString("i8", &i8, RKValueTypeInt8));
    uint8_t u8 = 100; printf("%s\n", RKVariableInString("u8", &u8, RKValueTypeUInt8));
    int16_t i16 = 100; printf("%s\n", RKVariableInString("i16", &i16, RKValueTypeInt16));
    uint16_t u16 = 100; printf("%s\n", RKVariableInString("u16", &u16, RKValueTypeUInt16));
    int32_t i32 = 100; printf("%s\n", RKVariableInString("i32", &i32, RKValueTypeInt32));
    uint64_t i64 = 1234567;
    c = RKVariableInString("u64", RKIntegerToCommaStyleString(i64), RKValueTypeNumericString);
    printf("%s (len = %zu)\n", c, strlen(c));
    char name[] = "RadarKit";
    c = RKVariableInString("name", name, RKValueTypeString);
    printf("%s (len = %zu)\n", c, strlen(c));
    i32 = 0x303f;
    c = RKVariableInString("mask", &i32, RKValueTypeInt32InHex);
    printf("%s (len = %zu)\n", c, strlen(c));
}

void RKTestModuloMath(void) {
    SHOW_FUNCTION_NAME
    int k;
    const int N = 4;

    RKLog("Test with SlotCount = %d, N = %d\n", RKBuffer0SlotCount, N);
    k = 0;                      RKLog("k = " RKFMT " --> Next N = " RKFMT "\n", k, RKNextNModuloS(k, N, RKBuffer0SlotCount));
    k = RKBuffer0SlotCount - 4; RKLog("k = " RKFMT " --> Next N = " RKFMT "\n", k, RKNextNModuloS(k, N, RKBuffer0SlotCount));
    k = RKBuffer0SlotCount - 3; RKLog("k = " RKFMT " --> Next N = " RKFMT "\n", k, RKNextNModuloS(k, N, RKBuffer0SlotCount));
    k = RKBuffer0SlotCount - 2; RKLog("k = " RKFMT " --> Next N = " RKFMT "\n", k, RKNextNModuloS(k, N, RKBuffer0SlotCount));
    k = RKBuffer0SlotCount - 1; RKLog("k = " RKFMT " --> Next N = " RKFMT "\n", k, RKNextNModuloS(k, N, RKBuffer0SlotCount));
    k = RKBuffer0SlotCount - 1; RKLog("k = " RKFMT " --> Prev N = " RKFMT "\n", k, RKPreviousNModuloS(k, N, RKBuffer0SlotCount));
    k = 0;                      RKLog("k = " RKFMT " --> Prev N = " RKFMT "\n", k, RKPreviousNModuloS(k, N, RKBuffer0SlotCount));
    k = 1;                      RKLog("k = " RKFMT " --> Prev N = " RKFMT "\n", k, RKPreviousNModuloS(k, N, RKBuffer0SlotCount));
    k = 2;                      RKLog("k = " RKFMT " --> Prev N = " RKFMT "\n", k, RKPreviousNModuloS(k, N, RKBuffer0SlotCount));
    k = 3;                      RKLog("k = " RKFMT " --> Prev N = " RKFMT "\n", k, RKPreviousNModuloS(k, N, RKBuffer0SlotCount));
    k = 4;                      RKLog("k = " RKFMT " --> Prev N = " RKFMT "\n", k, RKPreviousNModuloS(k, N, RKBuffer0SlotCount));
    k = 4899;                   RKLog("k = " RKFMT " --> Next N = " RKFMT "\n", k, RKNextNModuloS(k, 100 - 1, RKBuffer0SlotCount));

    struct timeval t0, t1;
    gettimeofday(&t1, NULL); t1.tv_sec -= 1;
    gettimeofday(&t0, NULL);
    if (RKTimevalDiff(t0, t1) < 0.1) {
        RKLog("First iteraction failed.\n");
    } else {
        RKLog("First iteraction is as expected.\n");
    }
}

void RKTestParseCommaDelimitedValues(void) {
    SHOW_FUNCTION_NAME
    char string[] = "1200,2000, 3000,  5000";
    float v[4];
    int32_t i[4];
    RKParseCommaDelimitedValues(v, RKValueTypeFloat, 4, string);
    RKLog("%s -> %.2f %.2f %.2f %.2f\n", string, v[0], v[1], v[2], v[3]);
    RKParseCommaDelimitedValues(i, RKValueTypeInt32, 4, string);
    RKLog("%s -> %d %d %d %d\n", string, i[0], i[1], i[2], i[3]);
}

void RKTestParseJSONString(void) {
    SHOW_FUNCTION_NAME
    char string[] = "{"
    "\"Transceiver\":{\"Value\":true,\"Enum\":0}, "
    "\"Pedestal\":{\"Value\":true,\"Enum\":0}, "
    "\"Health Relay\":{\"Value\":true,\"Enum\":0}, "
    "\"Network\":{\"Value\":true,\"Enum\":0}, "
    "\"Recorder (Coming Soon)\":{\"Value\":true,\"Enum\":3}, "
    "\"10-MHz Clock\":{\"Value\":true,\"Enum\":0}, "
    "\"DAC PLL\":{\"Value\":true,\"Enum\":0}, "
    "\"FPGA Temp\":{\"Value\":\"69.3degC\",\"Enum\":0}, "
    "\"Core Volt\":{\"Value\":\"1.00 V\",\"Enum\":4}, "
    "\"Aux. Volt\":{\"Value\":\"2.469 V\",\"Enum\":0}, "
    "\"XMC Volt\":{\"Value\":\"11.649 V\",\"Enum\":0}, "
    "\"XMC 3p3\":{\"Value\":\"3.250 V\",\"Enum\":0}, "
    "\"PRF\":{\"Value\":\"5,008 Hz\",\"Enum\":0,\"Target\":\"5,000 Hz\"}, "
    "\"Transmit H\":{\"Value\":\"69.706 dBm\",\"Enum\":0,\"MaxIndex\":2,\"Max\":\"-1.877 dBm\",\"Min\":\"-2.945 dBm\"}, "
    "\"Transmit V\":{\"Value\":\"69.297 dBm\",\"Enum\":0,\"MaxIndex\":2,\"Max\":\"-2.225 dBm\",\"Min\":\"-3.076 dBm\"}, "
    "\"DAC QI\":{\"Value\":\"0.913\",\"Enum\":0}, "
    "\"Waveform\":{\"Value\":\"h4011\",\"Enum\":0}, "
    "\"UnderOver\":[0,-897570], "
    "\"Lags\":[-139171212,-139171220,-159052813], \"NULL\":[149970], "
    "\"Pedestal AZ Interlock\":{\"Value\":true,\"Enum\":0}, "
    "\"Pedestal EL Interlock\":{\"Value\":true,\"Enum\":0}, "
    "\"VCP Active\":{\"Value\":true,\"Enum\":0}, "
    "\"Pedestal AZ Position\":{\"Value\":\"26.21 deg\",\"Enum\":0}, "
    "\"Pedestal EL Position\":{\"Value\":\"2.97 deg\",\"Enum\":0}, "
    "\"TWT Power\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Warmed Up\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT High Voltage\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Full Power\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT VSWR\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Duty Cycle\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Fans\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Interlock\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Faults Clear\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Cathode Voltage\":{\"Value\":\"-21.54 kV\",\"Enum\":0}, "
    "\"TWT Body Current\":{\"Value\":\"0.09 A\",\"Enum\":0}, "
    "\"GPS Valid\":{\"Value\":true,\"Enum\":0}, "
    "\"GPS Latitude\":{\"Value\":\"35.1812820\",\"Enum\":0}, "
    "\"GPS Longitude\":{\"Value\":\"-97.4373016\",\"Enum\":0}, "
    "\"GPS Heading\":{\"Value\":\"88.0 deg\", \"Enum\":0}, "
    "\"Ground Speed\":{\"Value\":\"0.30 km/h\", \"Enum\":0}, "
    "\"Platform Pitch\":{\"Value\":\"-0.23 deg\",\"Enum\":0}, "
    "\"Platform Roll\":{\"Value\":\"0.04 deg\",\"Enum\":0}, "
    "\"I2C Chip\":{\"Value\":\"30.50 degC\",\"Enum\":0}, "
    "\"Event\":\"none\", \"Log Time\":1493410480"
    "}";
    printf("%s (%d characters)\n\n", string, (int)strlen(string));

    // Test getting a specific key
    printf("Getting specific key:\n");
    printf("---------------------\n");
    char *stringObject, *stringValue, *stringEnum;
    if ((stringObject = RKGetValueOfKey(string, "latitude")) != NULL) {
        printf("stringObject = '%s'\n", stringObject);
        stringValue = RKGetValueOfKey(stringObject, "value");
        stringEnum = RKGetValueOfKey(stringObject, "enum");
        if (stringValue != NULL && stringEnum != NULL && atoi(stringEnum) == RKStatusEnumNormal) {
            printf("latitude = %.7f\n", atof(stringValue));
        }
    }
    printf("\n");
    if ((stringObject = RKGetValueOfKey(string, "longitude")) != NULL) {
        printf("stringObject = '%s'\n", stringObject);
        stringValue = RKGetValueOfKey(stringObject, "value");
        stringEnum = RKGetValueOfKey(stringObject, "enum");
        if (stringValue != NULL && stringEnum != NULL && atoi(stringEnum) == RKStatusEnumNormal) {
            printf("longitude = %.7f\n", atof(stringValue));
        }
    }

    printf("\n");
    printf("Getting all keys:\n");
    printf("-----------------\n");
    char criticalKey[RKNameLength];
    char criticalValue[RKNameLength];
    bool anyCritical = RKAnyCritical(string, true, criticalKey, criticalValue);
    printf("anyCritical = %s%s%s%s%s\n",
           rkGlobalParameters.showColor ? "\033[38;5;207m" : "",
           anyCritical ? "true" : "false",
           rkGlobalParameters.showColor ? RKNoColor : "",
           anyCritical ? " --> " : "",
           anyCritical ? criticalKey : ""
           );

    printf("\n");
    char strObj[] = "0";
    stringObject = strObj;
    printf("stringObject = '%s'\n", stringObject);
    stringValue = RKGetValueOfKey(stringObject, "value");
    stringEnum = RKGetValueOfKey(stringObject, "enum");
    printf("stringValue = %s\n", stringValue == NULL ? "(NULL)" : stringValue);
    printf("stringEnum = %s\n", stringEnum == NULL ? "(NULL)" : stringEnum);
    if (stringValue != NULL && stringEnum != NULL && atoi(stringEnum) == RKStatusEnumNormal) {
        printf("longitude = %.7f\n", atof(stringValue));
    }

    printf("\n===\n\n");
    
    int k;
    size_t s;
    const int N = 8;
    float *nums = (float *)malloc(N * sizeof(float));
    sprintf(string, "{'name':'U', 'PieceCount': 1, 'b':-32, 'w':[0.5, 0.6]}");
    printf("%s (%d characters)\n\n", string, (int)strlen(string));

    stringObject = RKGetValueOfKey(string, "name");
    printf("name = %s\n", stringObject);
    stringObject = RKGetValueOfKey(string, "PieceCount");
    printf("Piece Count = %s\n", stringObject);

    stringObject = RKGetValueOfKey(string, "b");
    printf("b = %s -->", stringObject);
    s = RKParseNumericArray(nums, RKValueTypeFloat, N, stringObject);
    for (k = 0; k < s; k++) {
        printf(" %.2f", nums[k]);
    }
    printf(" (count = %zu)\n", s);

    stringObject = RKGetValueOfKey(string, "w");
    printf("w = %s --> ", stringObject);
    s = RKParseNumericArray(nums, RKValueTypeFloat, N, stringObject);
    for (k = 0; k < s; k++) {
        printf(" %.2f", nums[k]);
    }
    printf(" (count = %zu)\n", s);

    free(nums);

    printf("\n===\n\n");

    sprintf(string, "{"
            "\"Health\":{\"Value\":true,\"Enum\":1}, "
            "\"Transceiver\":{\"Value\":true,\"Enum\":1}, "
            "\"GPS Latitude\":{\"Value\":\"+35.0 deg\",\"Enum\":1}, "
            "\"GPS Longitude\":{\"Value\":\"-97.0 deg\",\"Enum\":1}, "
            "}");
    printf("string = %s\n", string);
    RKReplaceEnumOfKey(string, "Health", RKStatusEnumOld);
    printf("string = %s\n", string);
    RKReplaceAllValuesOfKey(string, "Enum", RKStatusEnumOld);
    printf("string = %s\n", string);

    printf("\n===\n\n");
    sprintf(string, "{"
            "\"Health\":{\"Value\":\"true\",\"Enum\":1}, "
            "\"Transceiver\":{\"Value\":\"False\",\"Enum\":1}, "
            "}");
    printf("string = %s\n", string);
    RKReviseLogicalValues(string);
    printf("string = %s\n", string);
}

void RKTestFileManager(void) {
    SHOW_FUNCTION_NAME
    int e, j, k, s;
    RKName filename;
    struct timeval tm;
    gettimeofday(&tm, NULL);
    time_t startTime = tm.tv_sec;
    float es[] = {2.0f, 4.0f, 6.0f, 8.0f};
    const int ne = sizeof(es) / sizeof(float);
    RKName ss[] = {"Z", "V", "D", "R"};
    const int ns = sizeof(ss) / sizeof(RKName);
    FILE *fid;
    size_t filesize = 1024 * 1024;
    char *payload = (char *)malloc(filesize);
    if (payload == NULL) {
        fprintf(stderr, "Unable to allocate payload.\n");
        exit(EXIT_FAILURE);
    }
    
    const char dataPath[] = "data";
    RKFileManager *fileManager = RKFileManagerInit();
    RKFileManagerSetPathToMonitor(fileManager, dataPath);
    RKFileManagerSetDiskUsageLimit(fileManager, 200 * 1024 * 1024);
    RKFileManagerSetVerbose(fileManager, 3);
    
    RKLog("Warning. All data in moment folder will be erased!\n");
    printf("Press Enter to continue ... 'n' to keep moment files, or Ctrl-C to terminate.\n");
    
    j = getchar();
    
    //printf("j = %d = %c\n", j, j);
    if (j != 'n' && j != 'N') {
        if (system("rm -rf data/moment") == -1) {
            RKLog("Error. Failed during system().\n");
        }
    }

    RKFileManagerStart(fileManager);
    
    e = 0;
    for (j = 0; j < 50; j++) {
        startTime += 3000;
        k = sprintf(filename, "%s/%s/", dataPath, RKDataFolderMoment);
        k += strftime(filename + k, 9, "%Y%m%d", gmtime(&startTime));
        k += sprintf(filename + k, "/RK-");
        k += strftime(filename + k, 16, "%Y%m%d-%H%M%S", gmtime(&startTime));
        k += sprintf(filename + k, "-E%.1f", es[e]);
        
        for (s = 0; s < ns; s++) {
            sprintf(filename + k, "-%s.nc", ss[s]);
            
            RKPreparePath(filename);
            
            fid = fopen(filename, "w");
            if (fid == NULL) {
                fprintf(stderr, "Unable to create file.\n");
                exit(EXIT_FAILURE);
            }
            fwrite(payload, filesize, 1, fid);
            fclose(fid);
            
            RKFileManagerAddFile(fileManager, filename, RKFileTypeMoment);
        }
        e = RKNextModuloS(e, ne);
        usleep(500000);
    }
    
    RKFileManagerFree(fileManager);
}

void RKTestPreferenceReading(void) {
    SHOW_FUNCTION_NAME
    int k;
    RKName name;
    double numbers[4];
    RKControl control;
    RKWaveformCalibration cali;
    RKPreference *preference = RKPreferenceInit();;
    RKPreferenceObject *object = NULL;
    object = RKPreferenceFindKeyword(preference, "PedzyHost");
    if (object) {
        printf("pedzy host = %s\n", object->valueString);
    }
    object = RKPreferenceFindKeyword(preference, "TweetaHost");
    if (object) {
        printf("tweeta host = %s\n", object->valueString);
    }
    RKPreferenceGetValueOfKeyword(preference, 1, "Name", name, RKParameterTypeString, RKNameLength);
    RKPreferenceGetValueOfKeyword(preference, 1, "FilePrefix", name, RKParameterTypeString, RKNameLength);
    RKPreferenceGetValueOfKeyword(preference, 1, "Latitude", numbers, RKParameterTypeDouble, 1);
    RKPreferenceGetValueOfKeyword(preference, 1, "Longitude", numbers, RKParameterTypeDouble, 1);
    RKPreferenceGetValueOfKeyword(preference, 1, "SystemZCal", numbers, RKParameterTypeDouble, 2);
    RKPreferenceGetValueOfKeyword(preference, 1, "SystemDCal", numbers, RKParameterTypeDouble, 1);
    k = 0;
    while (RKPreferenceGetValueOfKeyword(preference, 1, "Shortcut", &control, RKParameterTypeControl, 0) == RKResultSuccess) {
        k++;
    }
    RKLog(">Preference.Shortcut count = %d\n", k);
    k = 0;
    while (RKPreferenceGetValueOfKeyword(preference, 1, "WaveformCal", &cali, RKParameterTypeWaveformCalibration, 0) == RKResultSuccess) {
        k++;
    }
    RKLog(">Preference.WaveformCal count = %d\n", k);
    RKPreferenceFree(preference);
}

void RKTestCountFiles(void) {
    SHOW_FUNCTION_NAME
    const char *folder = "data";
    long count = RKCountFilesInPath(folder);
    printf("%ld files in %s\n", count, folder);
}

void RKTestFileMonitor(void) {
    SHOW_FUNCTION_NAME
    const char *file = "pref.conf";
    RKFileMonitor *mon = RKFileMonitorInit(file, &RKTestCallback, NULL);
    RKLog("Touching file %s ...\n", file);
    char command[strlen(file) + 10];
    sprintf(command, "touch %s", file);
    int k = system(command);
    if (k) {
        RKLog("Error. Failed using system() -> %d   errno = %d\n", k, errno);
    }
    sleep(2);
    RKFileMonitorFree(mon);
}

void RKTestHostMonitor(void) {
    SHOW_FUNCTION_NAME
    RKHostMonitor *o = RKHostMonitorInit();
    if (o == NULL) {
        fprintf(stderr, "Unable to allocate a Host Monitor.\n");
        return;
    }
    RKHostMonitorSetVerbose(o, 2);
    RKHostMonitorAddHost(o, "www.amazon.com");
    RKHostMonitorStart(o);
    sleep(RKHostMonitorPingInterval * 3 + RKHostMonitorPingInterval - 1);
    RKHostMonitorFree(o);
}

void RKTestInitializingRadar(void) {
    SHOW_FUNCTION_NAME
    RKRadar *aRadar = RKInitLean();
    RKShowOffsets(aRadar, NULL);
    RKFree(aRadar);
}

void RKTestTemperatureToStatus(void) {
    SHOW_FUNCTION_NAME
    int k;
    float values[] = {-51.0f, -12.0f, -4.5f, 0.5f, 30.0f, 72.5f, 83.0f, 90.5f};
    for (k = 0; k < sizeof(values) / sizeof(float); k++) {
        printf("value = %7.2f -> %d\n", values[k], RKStatusFromTemperatureForCE(values[k]));
    }
}

void RKTestGetCountry(void) {
    SHOW_FUNCTION_NAME
    int k;
    double latitude;
    double longitude;
    char *country;
    bool correct;

    double coords[][2] = {
        { 35.222567, -97.439478},  // United States
        { 36.391592, 127.031428},  // South Korea
        {-11.024860, -75.279694},  // Peru
        { 34.756300, 135.615895}   // Japan
    };
    
    RKName answers[] = {
        "United States",
        "South Korea",
        "Peru",
        "Japan"
    };

    for (k = 0; k < sizeof(coords) / sizeof(coords[0]); k++) {
        latitude = coords[k][0];
        longitude = coords[k][1];
        country = RKCountryFromPosition(latitude, longitude);
        correct = !strcmp(answers[k], country);
        printf("%d. (%10.6f, %10.6f) --> %s (%s%s%s)\n", k, latitude, longitude, country,
               correct ? (rkGlobalParameters.showColor ? RKGreenColor : "") : (rkGlobalParameters.showColor ? RKRedColor : ""),
               correct ? "Correct" : "Incorrect",
               rkGlobalParameters.showColor ? RKNoColor : ""
               );
    }
}

void RKTestBufferOverviewText(const char *options) {
    SHOW_FUNCTION_NAME
    RKRadar *radar = RKInitLean();
    char *text = (char *)malloc(8192);
    RKTextPreferences textPreferences = RKTextPreferencesShowColor | RKTextPreferencesDrawBackground | RKTextPreferencesWindowSize120x80;
    if (options) {
        textPreferences = (RKTextPreferences)strtol(options, NULL, 16);
        RKLog("%s options = %s -> 0x%02x\n", __FUNCTION__, options, textPreferences);
        RKLog(">%s %s %s\n",
              textPreferences & RKTextPreferencesShowColor ? "RKTextPreferencesShowColor" : "",
              textPreferences & RKTextPreferencesDrawBackground ? "RKTextPreferencesDrawBackground" : "",
              ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize120x80 ? "RKTextPreferencesWindowSize120x80" :
               ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize120x50 ? "RKTextPreferencesWindowSize120x50" :
                ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize80x50 ? "RKTextPreferencesWindowSize80x50" :
                 ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize80x40 ? "RKTextPreferencesWindowSize80x50" :
                  ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize80x25 ? "RKTextPreferencesWindowSize80x25" : ""))))));
    }
    RKBufferOverview(radar, text, textPreferences);
    printf("%s\n", text);
    RKFree(radar);
    free(text);
}

void RKTestHealthOverviewText(const char *options) {
    SHOW_FUNCTION_NAME
    char string[] = "{"
    "\"Transceiver\":{\"Value\":true,\"Enum\":0}, "
    "\"Pedestal\":{\"Value\":true,\"Enum\":0}, "
    "\"Health Relay\":{\"Value\":true,\"Enum\":0}, "
    "\"Network\":{\"Value\":true,\"Enum\":0}, "
    "\"Recorder (Coming Soon)\":{\"Value\":true,\"Enum\":3}, "
    "\"10-MHz Clock\":{\"Value\":true,\"Enum\":0}, "
    "\"DAC PLL\":{\"Value\":true,\"Enum\":0}, "
    "\"FPGA Temp\":{\"Value\":\"69.3degC\",\"Enum\":0}, "
    "\"Core Volt\":{\"Value\":\"1.00 V\",\"Enum\":4}, "
    "\"Aux. Volt\":{\"Value\":\"2.469 V\",\"Enum\":0}, "
    "\"XMC Volt\":{\"Value\":\"11.649 V\",\"Enum\":0}, "
    "\"XMC 3p3\":{\"Value\":\"3.250 V\",\"Enum\":0}, "
    "\"PRF\":{\"Value\":\"5,008 Hz\",\"Enum\":0,\"Target\":\"5,000 Hz\"}, "
    "\"Transmit H\":{\"Value\":\"69.706 dBm\",\"Enum\":0,\"MaxIndex\":2,\"Max\":\"-1.877 dBm\",\"Min\":\"-2.945 dBm\"}, "
    "\"Transmit V\":{\"Value\":\"69.297 dBm\",\"Enum\":0,\"MaxIndex\":2,\"Max\":\"-2.225 dBm\",\"Min\":\"-3.076 dBm\"}, "
    "\"DAC QI\":{\"Value\":\"0.913\",\"Enum\":0}, "
    "\"Waveform\":{\"Value\":\"h4011\",\"Enum\":0}, "
    "\"UnderOver\":[0,-897570], "
    "\"Lags\":[-139171212,-139171220,-159052813], \"NULL\":[149970], "
    "\"Pedestal AZ Interlock\":{\"Value\":true,\"Enum\":0}, "
    "\"Pedestal EL Interlock\":{\"Value\":true,\"Enum\":0}, "
    "\"VCP Active\":{\"Value\":true,\"Enum\":0}, "
    "\"Pedestal AZ Position\":{\"Value\":\"26.21 deg\",\"Enum\":0}, "
    "\"Pedestal EL Position\":{\"Value\":\"2.97 deg\",\"Enum\":0}, "
    "\"TWT Power\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Warmed Up\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT High Voltage\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Full Power\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT VSWR\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Duty Cycle\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Fans\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Interlock\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Faults Clear\":{\"Value\":true,\"Enum\":0}, "
    "\"TWT Cathode Voltage\":{\"Value\":\"-21.54 kV\",\"Enum\":0}, "
    "\"TWT Body Current\":{\"Value\":\"0.09 A\",\"Enum\":0}, "
    "\"GPS Valid\":{\"Value\":true,\"Enum\":0}, "
    "\"GPS Latitude\":{\"Value\":\"35.1812820\",\"Enum\":0}, "
    "\"GPS Longitude\":{\"Value\":\"-97.4373016\",\"Enum\":0}, "
    "\"GPS Heading\":{\"Value\":\"88.0 deg\", \"Enum\":0}, "
    "\"Ground Speed\":{\"Value\":\"0.30 km/h\", \"Enum\":0}, "
    "\"Platform Pitch\":{\"Value\":\"-0.23 deg\",\"Enum\":0}, "
    "\"Platform Roll\":{\"Value\":\"0.04 deg\",\"Enum\":0}, "
    "\"I2C Chip\":{\"Value\":\"30.50 degC\",\"Enum\":0}, "
    "\"Event\":\"none\", \"Log Time\":1493410480"
    "}";
    printf("%s\n", string);
    char *text = (char *)malloc(8192);
    RKTextPreferences textPreferences = RKTextPreferencesShowColor | RKTextPreferencesDrawBackground | RKTextPreferencesWindowSize120x80;
    if (options) {
        textPreferences = (RKTextPreferences)strtol(options, NULL, 16);
        RKLog("%s options = %s -> 0x%02x\n", __FUNCTION__, options, textPreferences);
        RKLog(">%s %s %s\n",
              textPreferences & RKTextPreferencesShowColor ? "RKTextPreferencesShowColor" : "",
              textPreferences & RKTextPreferencesDrawBackground ? "RKTextPreferencesDrawBackground" : "",
              ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize120x80 ? "RKTextPreferencesWindowSize120x80" :
               ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize120x50 ? "RKTextPreferencesWindowSize120x50" :
                ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize80x50 ? "RKTextPreferencesWindowSize80x50" :
                 ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize80x40 ? "RKTextPreferencesWindowSize80x50" :
                  ((textPreferences & RKTextPreferencesWindowSizeMask) == RKTextPreferencesWindowSize80x25 ? "RKTextPreferencesWindowSize80x25" : ""))))));
    }
    RKHealthOverview(string, text, textPreferences);
    printf("%s\n", text);
    free(text);
}

void RKTestSweepRead(const char *file) {
    SHOW_FUNCTION_NAME
    RKSweep *sweep = RKSweepFileRead(file);
    if (sweep) {
        RKSweepFree(sweep);
    }
}

void RKTestProductRead(const char *file) {
    SHOW_FUNCTION_NAME
    RKProduct *product = RKProductFileReaderNC(file, true);
    if (product) {
        RKProductFree(product);
    }
}

void RKTestProductWrite(void) {
    SHOW_FUNCTION_NAME
    int g, k;
    float *v;
    RKProduct *product;
    RKProductBufferAlloc(&product, 1, 360, 8);
    product->desc.type = RKProductTypePPI;
    sprintf(product->desc.name, "Reflectivity");
    sprintf(product->desc.unit, "dBZ");
    sprintf(product->desc.colormap, "Reflectivity");
    sprintf(product->header.radarName, "RadarKit");
    product->header.latitude = 35.23682;
    product->header.longitude = -97.46381;
    product->header.heading = 0.0f;
    product->header.wavelength = 0.0314f;
    product->header.sweepElevation = 2.4f;
    product->header.rayCount = 360;
    product->header.gateCount = 8;
    product->header.gateSizeMeters = 7500.0f;
    product->header.prt[0] = 1.0e-3f;
    product->header.isPPI = true;
    product->header.startTime = 201443696;
    product->header.endTime = 201443696 + 10;
    float az = 90.0f;
    for (k = 0; k < 360; k++) {
        product->startAzimuth[k] = az;
        if (az >= 359.0f) {
            az = az - 359.0f;
        } else {
            az += 1.0f;
        }
        product->endAzimuth[k] = az;
        product->startElevation[k] = 2.4f;
        product->endElevation[k] = 2.4f;
    }
    v = product->data;
    for (k = 0; k < 360; k++) {
        for (g = 0; g < 8; g++) {
            *v++ = NAN;
        }
    }
    RKProductFileWriterNC(product, "blank.nc");
    v = product->data;
    for (k = 0; k < 360; k++) {
        for (g = 0; g < 8; g++) {
            *v++ = (float)(k % 15) * 5.0f - 5.0f;
        }
    }
    RKProductFileWriterNC(product, "rainbow.nc");
    RKProductBufferFree(product, 1);
}

void RKTestReviseLogicalValues(void) {
    SHOW_FUNCTION_NAME
    char string[] = "{"
    "\"Transceiver\":{\"Value\":\"TRUE\", \"Enum\":0}, "
    "\"Pedestal\":{\"Value\":\"True\", \"Enum\":0}, "
    "\"Health Relay\":{\"Value\":\"true\", \"Enum\":0}, "
    "\"Pedestal\":\"True\", "
    "}";
    printf("string = %s\n", string);
    RKReviseLogicalValues(string);
    printf("  -> %s\n", string);
}

void RKTestReadIQ(const char *filename) {
    SHOW_FUNCTION_NAME
    int i, j, k, p = 0;
    size_t tr;
    time_t startTime;
    size_t readsize, bytes, mem = 0;
    char timestr[32];
    long filesize = 0;
    uint32_t u32;
    RKBuffer pulseBuffer;

    RKLog("Opening file %s ...\n", filename);
    FILE *fid = fopen(filename, "r");
    if (fid == NULL) {
        fprintf(stderr, "Error opening file %s\n", filename);
        return;
    }
    if (fseek(fid, 0L, SEEK_END)) {
        RKLog("Error. Unable to tell the file size.\n");
    } else {
        filesize = ftell(fid);
        RKLog("File size = %s B\n", RKUIntegerToCommaStyleString(filesize));
    }
    rewind(fid);

    RKFileHeader *fileHeader = (RKFileHeader *)malloc(sizeof(RKFileHeader));
    RKConfig *config = &fileHeader->config;
    RKWaveform *waveform = NULL;

    readsize = fread(fileHeader, sizeof(RKFileHeader), 1, fid);
    if (fileHeader->buildNo <= 4) {
        RKLog("Error. Sorry but I wasn't programmed to read this. Ask my father.\n");
        return;
    } else if (fileHeader->buildNo == 5) {
        rewind(fid);
        RKFileHeaderV1 *fileHeaderV1 = (RKFileHeaderV1 *)malloc(sizeof(RKFileHeaderV1));
        fread(fileHeaderV1, sizeof(RKFileHeaderV1), 1, fid);
        fileHeader->dataType = fileHeaderV1->dataType;
        fileHeader->desc = fileHeaderV1->desc;
        memcpy(&fileHeader->config, &fileHeaderV1->config, sizeof(RKConfigV1));
        fileHeader->config.waveform = NULL;
        fileHeader->config.waveformDecimate = NULL;
        free(fileHeaderV1);
    } else {
        waveform = RKWaveformReadFromReference(fid);
        fileHeader->config.waveform = waveform;
        fileHeader->config.waveformDecimate = RKWaveformCopy(waveform);
        RKWaveformDecimate(fileHeader->config.waveformDecimate, fileHeader->desc.pulseToRayRatio);
    }
    RKLog(">fileHeader.preface = '%s'   buildNo = %d\n", fileHeader->preface, fileHeader->buildNo);
    RKLog(">fileHeader.dataType = '%s'\n",
           fileHeader->dataType == RKRawDataTypeFromTransceiver ? "Raw" :
           (fileHeader->dataType == RKRawDataTypeAfterMatchedFilter ? "Compressed" : "Unknown"));
    RKLog(">desc.name = '%s'\n", fileHeader->desc.name);
    RKLog(">desc.latitude, longitude = %.6f, %.6f\n", fileHeader->desc.latitude, fileHeader->desc.longitude);
    RKLog(">desc.pulseCapacity = %s\n", RKIntegerToCommaStyleString(fileHeader->desc.pulseCapacity));
    RKLog(">desc.pulseToRayRatio = %u\n", fileHeader->desc.pulseToRayRatio);
    RKLog(">desc.productBufferDepth = %u\n", fileHeader->desc.productBufferDepth);
    RKLog(">desc.wavelength = %.4f m\n", fileHeader->desc.wavelength);
    RKLog(">config.sweepElevation = %.2f deg\n", config->sweepElevation);
    RKLog(">config.prt = %.3f ms (PRF = %s Hz)\n",
          1.0e3f * config->prt[0],
          RKIntegerToCommaStyleString((int)roundf(1.0f / config->prt[0])));
    RKLog(">config.pw = %.2f us\n", 1.0e-6 * config->pw[0]);
    RKLog(">config.pulseGateCount = %s --> %s\n",
          RKIntegerToCommaStyleString(config->pulseGateCount),
          RKIntegerToCommaStyleString(config->pulseGateCount / fileHeader->desc.pulseToRayRatio));
    RKLog(">config.pulseGateSize = %.3f m --> %.3f m\n",
          config->pulseGateSize,
          config->pulseGateSize * fileHeader->desc.pulseToRayRatio);
    RKLog(">config.noise = %.2f, %.2f ADU^2\n", config->noise[0], config->noise[1]);
    RKLog(">config.systemZCal = %.2f, %.2f ADU^2\n", config->systemZCal[0], config->systemZCal[1]);
    RKLog(">config.systemDCal = %.2f dB\n", config->systemDCal);
    RKLog(">config.systemPCal = %.2f deg\n", config->systemPCal);
    RKLog(">config.SNRThreshold = %.2f dB\n", config->SNRThreshold);
    RKLog(">config.SQIThreshold = %.2f\n", config->SQIThreshold);
    RKLog(">config.waveformName = '%s'\n", config->waveformName);
    if (waveform) {
        RKWaveformSummary(waveform);
    }

    // Ray capacity always respects pulseCapcity / pulseToRayRatio and SIMDAlignSize
    const uint32_t rayCapacity = ((uint32_t)ceilf((float)fileHeader->desc.pulseCapacity / fileHeader->desc.pulseToRayRatio / (float)RKSIMDAlignSize)) * RKSIMDAlignSize;
    if (fileHeader->dataType == RKRawDataTypeFromTransceiver) {
        u32 = fileHeader->desc.pulseCapacity;
    } else if (fileHeader->dataType == RKRawDataTypeAfterMatchedFilter) {
        u32 = (uint32_t)ceilf((float)rayCapacity * sizeof(int16_t) / RKSIMDAlignSize) * RKSIMDAlignSize / sizeof(int16_t);
    } else {
        RKLog("Error. Unable to handle dataType %d", fileHeader->dataType);
        exit(EXIT_FAILURE);
    }
    const uint32_t pulseCapacity = u32;
    bytes = RKPulseBufferAlloc(&pulseBuffer, pulseCapacity, 1);
    if (bytes == 0 || pulseBuffer == NULL) {
        RKLog("Error. Unable to allocate memory for I/Q pulses.\n");
        exit(EXIT_FAILURE);
    }
    mem += bytes;
    RKLog("Pulse buffer occupies %s B  (%s pulses x %s gates)\n",
          RKUIntegerToCommaStyleString(bytes),
          RKIntegerToCommaStyleString(RKMaximumPulsesPerRay),
          RKIntegerToCommaStyleString(pulseCapacity));

    char sweepBeginMarker[20] = "S", sweepEndMarker[20] = "E";
    if (rkGlobalParameters.showColor) {
        sprintf(sweepBeginMarker, "%sS%s", RKGetColorOfIndex(3), RKNoColor);
        sprintf(sweepEndMarker, "%sE%s", RKGetColorOfIndex(2), RKNoColor);
    }

    for (k = 0; k < RKRawDataRecorderDefaultMaximumRecorderDepth; k++) {
        RKPulse *pulse = RKGetPulseFromBuffer(pulseBuffer, 0);
        // Pulse header
        readsize = fread(&pulse->header, sizeof(RKPulseHeader), 1, fid);
        if (readsize != 1) {
            break;
        }
        // Restore pulse capacity variable since we are not using whatever that was recorded in the file (radar)
        pulse->header.capacity = pulseCapacity;
        if (pulse->header.downSampledGateCount > pulseCapacity) {
            printf("Error. Pulse contains %s gates / %s capacity allocated.\n",
                   RKIntegerToCommaStyleString(pulse->header.downSampledGateCount),
                   RKIntegerToCommaStyleString(pulseCapacity));
        }
        startTime = pulse->header.time.tv_sec;
        tr = strftime(timestr, 24, "%F %T", gmtime(&startTime));
        tr += sprintf(timestr + tr, ".%06d", (int)pulse->header.time.tv_usec);
        // Pulse payload of H and V data into channels 0 and 1, respectively. Also, copy to split-complex storage
        for (j = 0; j < 2; j++) {
            RKComplex *x = RKGetComplexDataFromPulse(pulse, j);
            RKIQZ z = RKGetSplitComplexDataFromPulse(pulse, j);
            readsize = fread(x, sizeof(RKComplex), pulse->header.downSampledGateCount, fid);
            if (readsize != pulse->header.downSampledGateCount || readsize > pulseCapacity) {
                printf("Error. This should not happen.  readsize = %s != %s || > %s\n",
                       RKIntegerToCommaStyleString(readsize),
                       RKIntegerToCommaStyleString(pulse->header.downSampledGateCount),
                       RKIntegerToCommaStyleString(pulseCapacity));
            }
            for (i = 0; i < pulse->header.downSampledGateCount; i++) {
                z.i[i] = x[i].i;
                z.q[i] = x[i].q;
            }
        }
        if (p % 100 == 0) {
            printf("p:%06d/%06" PRIu64 " %s  E%5.2f, A%6.2f  %s x %.1f m\n", p, pulse->header.i, timestr,
                   pulse->header.elevationDegrees, pulse->header.azimuthDegrees,
                   RKIntegerToCommaStyleString(pulse->header.downSampledGateCount),
                   pulse->header.gateSizeMeters * fileHeader->desc.pulseToRayRatio);
        }
        p++;
    }
    printf("fpos = %s / %s   k = %s\n",
          RKUIntegerToCommaStyleString(ftell(fid)), RKUIntegerToCommaStyleString(filesize),
          RKIntegerToCommaStyleString(k));
    if (ftell(fid) != filesize) {
        printf("Warning. There is leftover in the file.");
    }

    fclose(fid);
    if (waveform) {
        RKWaveformFree(waveform);
    }
    free(fileHeader);
}

void RKTestPreparePath(void) {
    int k;
    time_t tt;
    char daystr[32], timestr[32];
    char filename[1024];
    time(&tt);
    for (k = 0; k < 30; k++) {
        strftime(daystr, 31, "%Y%m%d", localtime(&tt));
        strftime(timestr, 31, "%H%M%S", localtime(&tt));
        sprintf(filename, "data/iq/%s/PX-%s-%s-E1.0-Z.nc", daystr, daystr, timestr);
        printf("tt = %zu  -->  %s %s -->  %s\n", tt, daystr, timestr, filename);
        RKPreparePath(filename);
        tt += 86400;
    }
}

#pragma mark -

void RKTestSIMD(const RKTestSIMDFlag flag) {
    SHOW_FUNCTION_NAME
    RKSIMD_show_info();

    int i;
    const int n = RKSIMDAlignSize / sizeof(RKFloat) * 2;

    for (i = 1; i <= 8; i++) {
        RKSIMD_show_count(i);
    }

    // PKIQZ struct variables are usually allocated somewhere else
    RKIQZ s, d, c;

    // In a local context, they are usually in reference form
    RKIQZ *src = &s;
    RKIQZ *dst = &d;
    RKIQZ *cpy = &c;
    RKComplex *cs;
    RKComplex *cd;
    RKComplex *cc;

    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&src->i, RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKFloat)))
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&src->q, RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKFloat)))
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&dst->i, RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKFloat)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&dst->q, RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKFloat)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&cpy->i, RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKFloat)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&cpy->q, RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKFloat)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&cs,     RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKComplex)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&cd,     RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKComplex)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&cc,     RKSIMDAlignSize, RKMaximumGateCount * sizeof(RKComplex)));

    const RKFloat tiny = 1.0e-3f;
    bool good;
    bool all_good = true;

    //

    for (i = 0; i < n; i++) {
        src->i[i] = (RKFloat)i;
        src->q[i] = (RKFloat)-i;
    }

    RKSIMD_zcpy(src, dst, n);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        good = src->i[i] == dst->i[i] && src->q[i] == dst->q[i];
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("src[%2d] = %9.2f%+9.2fi   dst[%2d] = %9.2f%+9.2fi\n", i, src->i[i], src->q[i], i, dst->i[i], dst->q[i]);
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "Complex Vector Copy -  zcpy", all_good);

    //

    memset(dst->i, 0, n * sizeof(RKFloat));
    memset(dst->q, 0, n * sizeof(RKFloat));

    RKSIMD_zscl(src, 3.0f, dst, n);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, 3-3i, 6-6i, 9-9i, ...
        good = fabsf(dst->i[i] - 3.0f * i) < tiny && fabs(dst->q[i] + 3.0f * i) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%9.2f%+9.2fi x 3.0 -> %9.2f%+9.2fi  %s\n", src->i[i], src->q[i], dst->i[i], dst->q[i], OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "Complex Vector Scaling by a Float -  zscl", all_good);

    //

    RKSIMD_zadd(src, src, dst, n);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, 2-2i, 3-3i, 4-4i, ...
        good = fabsf(dst->i[i] - (float)(2 * i)) < tiny && fabs(dst->q[i] - (float)(-2 * i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%9.2f%+9.2fi ++ -> %9.2f%+9.2fi  %s\n", src->i[i], src->q[i], dst->i[i], dst->q[i], OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "Complex Vector Addition -  zadd", all_good);

    //

    RKSIMD_zcpy(src, dst, n);
    RKSIMD_izadd(src, dst, n);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (int i = 0; i < n; i++) {
        // Answers should be 0, 2-2i, 3-3i, 4-4i, ...
        good = fabsf(dst->i[i] - (float)(2 * i)) < tiny && fabs(dst->q[i] - (float)(-2 * i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%9.2f%+9.2fi ++ -> %9.2f%+9.2fi  %s\n", src->i[i], src->q[i], dst->i[i], dst->q[i], OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "In-place Complex Vector Addition - izadd", all_good);

    //

    RKSIMD_zmul(src, src, dst, n, false);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, -2i, -8i, -18i, ...
        good = fabsf(dst->i[i]) < tiny && fabs(dst->q[i] - (float)(-2 * i * i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%9.2f%+9.2fi ** -> %9.2f%+9.2fi  %s\n", src->i[i], src->q[i], dst->i[i], dst->q[i], OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "Complex Vector Multiplication -  zmul", all_good);

    //

    RKSIMD_zcpy(src, dst, n);
    RKSIMD_izmul(src, dst, n, false);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, -2i, -8i, -18i, ...
        good = fabsf(dst->i[i]) < tiny && fabs(dst->q[i] - (float)(-2 * i * i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%9.2f%+9.2fi ** -> %9.2f%+9.2fi  %s\n", src->i[i], src->q[i], dst->i[i], dst->q[i], OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "In-place Complex Vector Multiplication - izmul", all_good);

    //

    // Populate some numbers
    for (i = 0; i < n; i++) {
        cs[i].i = (RKFloat)i;
        cs[i].q = (RKFloat)(-i);
        cd[i].i = (RKFloat)(i + 1);
        cd[i].q = (RKFloat)(-i);
    }
    memcpy(cc, cd, n * sizeof(RKComplex));

    RKSIMD_iymul(cs, cd, n);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, 1-3i, 2-10i, 3-21i, ...
        good = fabsf(cd[i].i - (RKFloat)i) < tiny && fabsf(cd[i].q - (RKFloat)(-2 * i * i - i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%+9.2f%+9.2fi * %+9.2f%+9.2fi = %+9.2f%+9.2fi  %s\n", cs[i].i, cs[i].q, cc[i].i, cc[i].q, cd[i].i, cd[i].q, OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "In-place Deinterleaved Complex Vector Multiplication - iymul", all_good);

    //

    // Populate some numbers
    for (i = 0; i < n; i++) {
        cs[i].i = (RKFloat)i;
        cs[i].q = (RKFloat)(-i);
        cd[i].i = (RKFloat)(i + 1);
        cd[i].q = (RKFloat)(-i);
    }
    memcpy(cc, cd, n * sizeof(RKComplex));

    RKSIMD_iymul2(cs, cd, n, false);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, 1-3i, 2-10i, 3-21i, ...
        good = fabsf(cd[i].i - (RKFloat)i) < tiny && fabsf(cd[i].q - (RKFloat)(-2 * i * i - i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%+9.2f%+9.2fi * %+9.2f%+9.2fi = %+9.2f%+9.2fi  %s\n", cs[i].i, cs[i].q, cc[i].i, cc[i].q, cd[i].i, cd[i].q, OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "In-place Deinterleaved Complex Vector Multiplication - iymul2", all_good);

    //

    for (i = 0; i < n; i++) {
        cc[i].i = (RKFloat)i;
        cc[i].q = (RKFloat)(-i);
    }
    RKSIMD_Complex2IQZ(cc, src, n);
    for (i = 0; i < n; i++) {
        cc[i].i = (RKFloat)(i + 1);
        cc[i].q = (RKFloat)(-i);
    }
    RKSIMD_Complex2IQZ(cc, dst, n);
    memcpy(cpy->i, dst->i, n * sizeof(RKFloat));
    memcpy(cpy->q, dst->q, n * sizeof(RKFloat));
    RKSIMD_izmul(src, dst, n, false);
    RKSIMD_IQZ2Complex(dst, cc, n);
    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0, 1-3i, 2-10i, 3-21i, ...
        good = fabsf(cc[i].i - (RKFloat)i) < tiny && fabsf(cc[i].q - (RKFloat)(-2 * i * i - i)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%+9.2f%+9.2fi * %+9.2f%+9.2fi = %+9.2f%+9.2fi  %s\n", src->i[i], src->q[i], cpy->i[i], cpy->q[i], dst->i[i], dst->q[i], OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "Deinterleave, Multiply Using iymul, and Interleave", all_good);

    //

    RKInt16C *is = (RKInt16C *)src->i;

    for (i = 0; i < n; i++) {
        is[i].i = (i % 2 == 0 ? 1 : -1);
        is[i].q = (i - n / 2) * (i % 2 == 0 ? -1 : 1);
    }
    memset(cd, 0, n * sizeof(RKComplex));

    RKSIMD_Int2Complex(is, cd, n);

    if (flag & RKTestSIMDFlagShowNumbers) {
        printf("====\n");
    }
    all_good = true;
    for (i = 0; i < n; i++) {
        // Answers should be 0-16i, 1-15i, 2-14i, 3-131i, ...
        good = fabsf(cd[i].i - (RKFloat)(i % 2 == 0 ? 1 : -1)) < tiny && fabsf(cd[i].q - (RKFloat)(i - n / 2) * (i % 2 == 0 ? -1 : 1)) < tiny;
        if (flag & RKTestSIMDFlagShowNumbers) {
            printf("%+3d%+3di -> %+5.1f%+5.1fi  %s\n", is[i].i, is[i].q, cd[i].i, cd[i].q, OXSTR(good));
        }
        all_good &= good;
    }
    RKSIMD_TEST_RESULT(rkGlobalParameters.showColor, "Conversion from i16 to float", all_good);

    if (flag & RKTestSIMDFlagPerformanceTestAll) {
        printf("\n==== Performance Test ====\n\n");
        printf("Using %s gates\n", RKIntegerToCommaStyleString(RKMaximumGateCount));

        int k;
        const int m = 20000;
        struct timeval t1, t2;

        if (flag & RKTestSIMDFlagPerformanceTestArithmetic) {
            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_zmul(src, src, dst, RKMaximumGateCount, false);
            }
            gettimeofday(&t2, NULL);
            printf("Regular SIMD multiplication time for %dK loops = %.3f s\n", m / 1000, RKTimevalDiff(t2, t1));

            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_izmul(src, dst, RKMaximumGateCount, false);
            }
            gettimeofday(&t2, NULL);
            printf("In-place SIMD multiplication time for %dK loops = %.3f s\n", m / 1000, RKTimevalDiff(t2, t1));

            printf("Vectorized Complex Multiplication (%dK loops):\n", m / 1000);
            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_iymul_reg(cs, cd, RKMaximumGateCount);
            }
            gettimeofday(&t2, NULL);
            printf("              reg: " RKSIMD_TEST_TIME_FORMAT " ms (Compiler Optimized -O2)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));

            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_iymul(cs, cd, RKMaximumGateCount);
            }
            gettimeofday(&t2, NULL);
            printf("            iymul: " RKSIMD_TEST_TIME_FORMAT " ms (Normal interleaved I/Q)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));

            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_izmul((RKIQZ *)src, (RKIQZ *)dst, RKMaximumGateCount, false);
            }
            gettimeofday(&t2, NULL);
            printf("            izmul: " RKSIMD_TEST_TIME_FORMAT " ms (Deinterleaved I/Q)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));

            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_Complex2IQZ(cc, src, RKMaximumGateCount);
                RKSIMD_izmul((RKIQZ *)src, (RKIQZ *)dst, RKMaximumGateCount, false);
                RKSIMD_IQZ2Complex(dst, cc, RKMaximumGateCount);
            }
            gettimeofday(&t2, NULL);
            printf("    E + izmul + D: " RKSIMD_TEST_TIME_FORMAT " ms (D, Multiply, I)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));
        }

        if (flag & RKTestSIMDFlagPerformanceTestConversion) {
            printf("Copy (%dK loops):\n", m / 1000);
            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                memcpy(src->i, dst->i, RKMaximumGateCount * sizeof(RKFloat));
                memcpy(src->q, dst->q, RKMaximumGateCount * sizeof(RKFloat));
            }
            gettimeofday(&t2, NULL);
            printf("       memcpy x 2: " RKSIMD_TEST_TIME_FORMAT " ms (Compiler Optimized -O2)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));

            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_zcpy(src, dst, RKMaximumGateCount);
            }
            gettimeofday(&t2, NULL);
            printf("             zcpy: " RKSIMD_TEST_TIME_FORMAT " ms (SIMD)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));

            printf("Conversions (%dK loops):\n", m / 1000);
            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_Int2Complex_reg(is, cd, RKMaximumGateCount);
            }
            gettimeofday(&t2, NULL);
            printf("              reg: " RKSIMD_TEST_TIME_FORMAT " ms (Compiler Optimized -O2)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));

            gettimeofday(&t1, NULL);
            for (k = 0; k < m; k++) {
                RKSIMD_Int2Complex(is, cd, RKMaximumGateCount);
            }
            gettimeofday(&t2, NULL);
            printf("      cvtepi32_ps: " RKSIMD_TEST_TIME_FORMAT " ms (SIMD)\n", 1.0e3 / m * RKTimevalDiff(t2, t1));
        }

        printf("\n==========================\n");
    }

    free(src->i);
    free(src->q);
    free(dst->i);
    free(dst->q);
    free(cpy->i);
    free(cpy->q);
    free(cs);
    free(cd);
    free(cc);
}


void RKTestWindow(void) {
    int k;
    int n = 6;
    double param;
    RKFloat *window = (RKFloat *)malloc(n * sizeof(RKFloat));

    printf("=================\nWindow Functions:\n=================\n\n");

    printf("Hamming:\n");
    RKWindowMake(window, RKWindowTypeHamming, n);
    for (k = 0; k < n; k++) {
        printf("w[%d] = %.4f\n", k, window[k]);
    }
    printf("\n");

    param = 0.5;
    printf("Kaiser @ %.4f:\n", param);
    RKWindowMake(window, RKWindowTypeKaiser, n, param);
    for (k = 0; k < n; k++) {
        printf("w[%d] = %.4f\n", k, window[k]);
    }
    printf("\n");

    param = 0.8;
    printf("Trapezoid @ %.4f:\n", param);
    RKWindowMake(window, RKWindowTypeTrapezoid, n, param);
    for (k = 0; k < n; k++) {
        printf("w[%d] = %.4f\n", k, window[k]);
    }
    printf("\n");

    param = 0.5;
    printf("Tukey @ %.4f:\n", param);
    RKWindowMake(window, RKWindowTypeTukey, n, param);
    for (k = 0; k < n; k++) {
        printf("w[%d] = %.4f\n", k, window[k]);
    }
    printf("\n");

    free(window);
}

void RKTestHilbertTransform(void) {
    SHOW_FUNCTION_NAME
    int i;
    const int n = 10;
    RKFloat a = 0.0f;
    RKFloat *x = (RKFloat *)malloc(n * sizeof(RKFloat));
    RKComplex *y = (RKComplex *)malloc(n * sizeof(RKComplex));

    // Simple example of Xr = [1 2 3 4] like hilbert() in MATLAB
    memset(x, 0, n * sizeof(RKFloat));
    x[0] = 1.0f;
    x[1] = 2.0f;
    x[2] = 3.0f;
    x[3] = 4.0f;
    RKHilbertTransform(x, y, 4);

    printf("\n");
    printf("Example 1:\n");
    printf("----------\n");
    printf("\nX =\n\n");
    for (i = 0; i < 4; i++) {
        printf("    [ %7.4f ]\n", x[i]);
    }
    printf("\nH =\n\n");
    for (i = 0; i < 4; i++) {
        printf("    [ %7.4f %s %7.4fi ]\n", y[i].i, y[i].q < 0 ? "-" : "+", y[i].q < 0.0 ? -y[i].q : y[i].q);
    }

    // Another example to illustrate the noise gain is more or less preserved.
    for (i = 0; i < n; i++) {
        x[i] = cosf(0.1f * 2.0f * M_PI * (float)i + 0.2);
    }
    RKHilbertTransform(x, y, n);
    for (i = 0; i < n; i++) {
        a += y[i].i * y[i].i + y[i].q * y[i].q;
    }

    printf("\n");
    printf("Example 2:\n");
    printf("----------\n");
    printf("\nX =\n\n");
    for (i = 0; i < n; i++) {
        printf("    [ %7.4f ]\n", x[i]);
    }
    printf("\nH =\n\n");
    for (i = 0; i < n; i++) {
        printf("    [ %7.4f %s %7.4fi ]\n", y[i].i, y[i].q < 0 ? "-" : "+", y[i].q < 0.0 ? -y[i].q : y[i].q);
    }
    printf("\na = %.4f\n\n", a);
    free(x);
    free(y);
}

void RKTestWriteFFTWisdom(void) {
    SHOW_FUNCTION_NAME
    fftwf_plan plan;
    fftwf_complex *in, *out;
    int nfft = 1 << (int)ceilf(log2f((float)RKMaximumGateCount));
    in = fftwf_malloc(nfft * sizeof(fftwf_complex));
    out = fftwf_malloc(nfft * sizeof(fftwf_complex));
    RKLog("Generating FFT wisdom ...\n");
    while (nfft > 2) {
        RKLog("NFFT %s\n", RKIntegerToCommaStyleString(nfft));
        plan = fftwf_plan_dft_1d(nfft, in, in, FFTW_FORWARD, FFTW_MEASURE);
        fftwf_destroy_plan(plan);
        plan = fftwf_plan_dft_1d(nfft, in, out, FFTW_FORWARD, FFTW_MEASURE);
        fftwf_destroy_plan(plan);
        plan = fftwf_plan_dft_1d(nfft, out, out, FFTW_BACKWARD, FFTW_MEASURE);
        fftwf_destroy_plan(plan);
        plan = fftwf_plan_dft_1d(nfft, out, in, FFTW_BACKWARD, FFTW_MEASURE);
        fftwf_destroy_plan(plan);
        nfft >>= 1;
    }
    fftwf_free(in);
    fftwf_free(out);
    RKLog("Exporting FFT wisdom ...\n");
    fftwf_export_wisdom_to_filename(RKFFTWisdomFile);
    RKLog("Done.\n");
}

void RKTestRingFilterShowCoefficients(void) {
    int i, k;
    RKFilterType type;
    RKIIRFilter *filter = (RKIIRFilter *)malloc(sizeof(RKIIRFilter));
    char *string = (char *)malloc(1024);
    if (filter == NULL || string == NULL) {
        fprintf(stderr, "Error allocation memory.\n");
        exit(EXIT_FAILURE);
    }
    for (type = RKFilterTypeNull; type < RKFilterTypeCount; type++) {
        RKGetFilterCoefficients(filter, type);
        RKLog("%s:\n", filter->name);
        i = sprintf(string, "b = [");
        for (k = 0; k < filter->bLength; k++) {
            i += sprintf(string + i, "%s%.4f", k > 0 ? ", " : "", filter->B[k].i);
        }
        sprintf(string + i, "]");
        RKLog(">%s", string);
        i = sprintf(string, "a = [");
        for (k = 0; k < filter->aLength; k++) {
            i += sprintf(string + i, "%s%.4f", k > 0 ? ", " : "", filter->A[k].i);
        }
        sprintf(string + i, "]");
        RKLog(">%s", string);
    }
    free(string);
    free(filter);
}

void RKTestRamp(void) {
    int k;
    int n = 6;
    double param;
    RKFloat *ramp = (RKFloat *)malloc(n * sizeof(RKFloat));

    printf("=================\nRamp Functions:\n=================\n\n");

    printf("Zeros:\n");
    RKRampMake(ramp, RKRampTypeZeros, n);
    for (k = 0; k < n; k++) {
        printf("r[%d] = %.4f\n", k, ramp[k]);
    }
    printf("\n");

    param = 0.5;
    printf("Step @ %.4f:\n", param);
    RKRampMake(ramp, RKRampTypeStep, n, param);
    for (k = 0; k < n; k++) {
        printf("r[%d] = %.4f\n", k, ramp[k]);
    }
    printf("\n");

    printf("Linear:\n");
    RKRampMake(ramp, RKRampTypeLinear, n);
    for (k = 0; k < n; k++) {
        printf("r[%d] = %.4f\n", k, ramp[k]);
    }
    printf("\n");

    printf("Raised Cosine:\n");
    RKRampMake(ramp, RKRampTypeRaisedCosine, n);
    for (k = 0; k < n; k++) {
        printf("r[%d] = %.4f\n", k, ramp[k]);
    }
    printf("\n");

    free(ramp);
}

#pragma mark - Waveform Tests

void RKTestMakeHops(void) {
    SHOW_FUNCTION_NAME
    for (int k = 3; k < 13; k++) {
        printf(UNDERLINE("%d Hops:\n"), k);
        RKBestStrideOfHops(k, true);
        printf("\n");
    }
}

void RKTestWaveformTFM(void) {
    SHOW_FUNCTION_NAME
    const char filename[] = "waveforms/test-tfm.rkwav";
    RKWaveform *waveform = RKWaveformInitAsFakeTimeFrequencyMultiplexing();
    RKWaveformSummary(waveform);
    RKWaveformWriteFile(waveform, filename);
    RKWaveformFree(waveform);
}

void RKTestWaveformWrite(void) {
    SHOW_FUNCTION_NAME
    RKWaveform *waveform = RKWaveformInitWithCountAndDepth(14, 100);
    RKWaveformFrequencyHops(waveform, 20.0e6, 0.0, 16.0e6);

    char filename[160];
    snprintf(filename, 159, "waveforms/%s.rkwav", waveform->name);
    RKLog("Creating waveform file '%s' ...\n", filename);
    RKWaveformWriteFile(waveform, filename);

    RKLog("Reading waveform file '%s' ...\n", filename);
    RKWaveform *loadedWaveform = RKWaveformInitFromFile(filename);
    RKWaveformSummary(loadedWaveform);

    RKWaveformFree(waveform);
    RKWaveformFree(loadedWaveform);

    printf("Remove the waveform file? (y/n) [y]:");
    
    int j = getchar();
    if (j == 10 || j == 'y' || j == 'Y') {
        RKLog("Removing waveform file ...\n");
        if (remove(filename)) {
            RKLog("Error removing file '%s'.\n", filename);
        }
    }
    RKLog("Done.\n");
}

void RKTestWaveformDownsampling(void) {
    SHOW_FUNCTION_NAME
    RKWaveform *waveform = RKWaveformInitAsTimeFrequencyMultiplexing(160.0e6, 0.0, 4.0e6, 50.0e-6);
    RKWaveformSummary(waveform);
    
    RKWaveformDecimate(waveform, 32);
    RKWaveformSummary(waveform);
}

void RKTestWaveformShowProperties(void) {
    SHOW_FUNCTION_NAME
    RKWaveform *waveform;
    
    waveform = RKWaveformInitAsSingleTone(160.0e6, 1.0e6, 1.0e-6);
    RKWaveformSummary(waveform);

    printf("\n");

    RKWaveformDownConvert(waveform);
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);

    printf("\n");

    waveform = RKWaveformInitAsLinearFrequencyModulation(160.0e6, 50.0e6, 2.0e-6, 1.0e6);
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);

    printf("\n");

    waveform = RKWaveformInitAsLinearFrequencyModulation(160.0e6, 50.0e6, 4.0e-6, 2.0e6);
    RKWaveformDownConvert(waveform);
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);

    printf("\n");
    
    waveform = RKWaveformInitAsFrequencyHops(200.0e6, 0.0, 1.0e-6, 20.0e6, 5);
    RKWaveformSummary(waveform);
    
    printf("\n");
    
    RKWaveformDecimate(waveform, 4);
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);
    
    printf("\n");
    
    waveform = RKWaveformInitAsFrequencyHoppingChirp(200.0e6, 0.0, 25.0e6, 1.6e-6, 5);
    RKWaveformSummary(waveform);
    
    printf("\n");
    
    RKWaveformDecimate(waveform, 4);
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);

    if (!RKFilenameExists("waveforms/ofm.rkwav")) {
        return;
    }

    printf("\n");

    waveform = RKWaveformInitFromFile("waveforms/ofm.rkwav");
    RKWaveformSummary(waveform);

    printf("\n");

    RKWaveformDownConvert(waveform);
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);

    if (!RKFilenameExists("waveforms/barker03.rkwav")) {
        return;
    }

    printf("\n");

    waveform = RKWaveformInitFromFile("waveforms/barker03.rkwav");
    RKWaveformSummary(waveform);
    RKWaveformFree(waveform);
}

void RKTestWaveformShowUserWaveformProperties(const char *filename) {
    SHOW_FUNCTION_NAME
    RKWaveform *waveform = RKWaveformInitFromFile(filename);
    RKWaveformSummary(waveform);
    
    printf("\n");

    RKWaveformDecimate(waveform, 8);
    RKWaveformSummary(waveform);
    
    printf("\n");
    
    RKWaveformDecimate(waveform, 15);
    RKWaveformSummary(waveform);

    RKWaveformFree(waveform);
}

#pragma mark - Radar Signal Processing

void RKTestPulseCompression(RKTestFlag flag) {
    SHOW_FUNCTION_NAME
    int j, k;
    RKPulse *pulse;
    RKInt16C *X;
    RKComplex *F;
    RKComplex *Y;
    RKIQZ Z;

    RKRadar *radar = RKInitLean();
    RKSetProcessingCoreCounts(radar, 2, 1, 1);

    // Increases verbosity if set
    if (flag & RKTestFlagVerbose) {
        RKSetVerbosity(radar, 1);
    }

    RKGoLive(radar);

    // Filter #2
    RKComplex filter2[] = {{1.0f, 1.0f}};
    RKFilterAnchor anchor2 = RKFilterAnchorDefaultWithMaxDataLength(8);

    // Filter #3
    RKComplex filter3[] = {{1.0f, 0.0f}, {0.0f, 1.0f}, {-1.0f, 0.0f}, {0.0f, -1.0f}};
    RKFilterAnchor anchor3 = RKFilterAnchorOfLengthAndMaxDataLength(4, 8);

    for (k = 0; k < 4; k++) {
        switch (k) {
            default:
                // Default is impulse [1];
                RKPulseEngineSetFilterToImpulse(radar->pulseEngine);
                break;
            case 1:
                // Two-tap running average [1, 1]
                RKPulseEngineSetFilterTo11(radar->pulseEngine);
                break;
            case 2:
                // Change filter to filter #2: [1 + 1i]
                RKPulseEngineSetFilter(radar->pulseEngine, filter2, anchor2, 0, 0);
                break;
            case 3:
                // Change filter to filter #3
                RKPulseEngineSetFilter(radar->pulseEngine, filter3, anchor3, 0, 0);
                break;
        }

        pulse = RKGetVacantPulse(radar);
        pulse->header.gateCount = 6;

        X = RKGetInt16CDataFromPulse(pulse, 0);
        memset(X, 0, pulse->header.gateCount * sizeof(RKInt16C));
        X[0].i = 1;
        X[0].q = 0;
        X[1].i = 2;
        X[1].q = 0;
        X[2].i = 4;
        X[2].q = 0;
        X[3].i = 2;
        X[3].q = 0;
        X[4].i = 1;
        X[4].q = 0;
        RKSetPulseReady(radar, pulse);

        while (radar->state & RKRadarStateLive && (pulse->header.s & RKPulseStatusCompressed) == 0) {
            usleep(1000);
        }

        F = radar->pulseEngine->filters[0][0];
        Y = RKGetComplexDataFromPulse(pulse, 0);
        Z = RKGetSplitComplexDataFromPulse(pulse, 0);

        if (flag & RKTestFlagShowResults) {
            printf("\033[4mTest %d:\n\033[24m", k);
            printf("X =               F =                    Y =                        Z =\n");
            for (j = 0; j < 8; j++) {
                printf("    [ %2d %s %2di ]      [ %5.2f %s %5.2fi ]      [ %6.2f %s %6.2fi ]      [ %6.2f %s %6.2fi ]\n",
                       X[j].i, X[j].q < 0 ? "-" : "+", abs(X[j].q),
                       F[j].i, F[j].q < 0.0f ? "-" : "+", fabs(F[j].q),
                       Y[j].i, Y[j].q < 0.0f ? "-" : "+", fabs(Y[j].q),
                       Z.i[j], Z.q[j] < 0.0f ? "-" : "+", fabs(Z.q[j]));
            }
            printf("\n");
        }
    } // for (k = 0; k < 3; ...)

    RKFree(radar);
}

void RKTestOneRay(int method(RKScratch *, RKPulse **, const uint16_t), const int lag) {
    SHOW_FUNCTION_NAME
    int k, p, n, g;
    RKScratch *space;
    RKFFTModule *fftModule;
    RKBuffer pulseBuffer;
    const int gateCount = 6;
    const int pulseCount = 10;
    const int pulseCapacity = 64;

    RKLog("Allocating buffers ...\n");

    RKScratchAlloc(&space, pulseCapacity, RKMaximumLagCount, (uint8_t)ceilf(log2f((float)pulseCount)), true);
    fftModule = RKFFTModuleInit(pulseCapacity, 0);
    space->fftModule = fftModule;
    space->gateCount = gateCount;
    space->gateSizeMeters = 30.0f;

    RKPulseBufferAlloc(&pulseBuffer, pulseCapacity, pulseCount);
    RKPulse *pulses[pulseCount];
    
    for (k = 0; k < pulseCount; k++) {
        RKPulse *pulse = RKGetPulseFromBuffer(pulseBuffer, k);
        pulse->header.t = k;
        pulse->header.i = (uint64_t)(-1) - pulseCount + k;
        pulse->header.gateCount = gateCount;
        pulse->header.downSampledGateCount = gateCount;
        // Fill in the data...
        for (p = 0; p < 2; p++) {
            RKIQZ X = RKGetSplitComplexDataFromPulse(pulse, p);
            RKComplex *Y = RKGetComplexDataFromPulse(pulse, p);

            // Some seemingly random pattern for testing
            n = pulse->header.i % 3 * (pulse->header.i % 2 ? 1 : -1) + p;
            for (g = 0; g < gateCount; g++) {
                if (g % 2 == 0) {
                    X.i[g] = (int16_t)((g * n) + p);
                    X.q[g] = (int16_t)((n - 2) * (g - 1));
                } else {
                    X.i[g] = (int16_t)(-g * (n - 1));
                    X.q[g] = (int16_t)((g * p) + n);
                }
                Y[g].i = X.i[g];
                Y[g].q = X.q[g];
            }
        }
        pulses[k] = pulse;
    }

    //printf("pcal[0] = %.2f\n", space->pcal[0]);

    if (method == RKPulsePairHop) {
        RKLog("Info. Pulse Pair for Frequency Hopping.\n");
    } else if (method == RKPulsePair) {
        RKLog("Info. Pulse Pair.\n");
    } else if (method == RKMultiLag) {
        space->userLagChoice = lag;
        space->velocityFactor = 1.0f;
        space->widthFactor = 1.0f;
        RKLog("Info. Multilag (N = %d).\n", space->userLagChoice);
    } else if (method == RKSpectralMoment) {
        RKLog("Warning. Spectral Moment is incomplete.\n");
    } else {
        RKLog("Warning. Unknown method.\n");
        method = RKPulsePair;
    }
    method(space, pulses, pulseCount);

    // Some known results
    RKFloat err;
    RKName str;

    // Results for pulse-pair, pulse-pair for hops, multilag for lags 2, 3, and 4
    RKFloat D[][7] = {
        { 1.7216, -2.5106, -1.9448,  -1.3558, -0.2018,  -0.9616},  // pulse-pair
        { 2.2780,  2.6324,  2.7621,   2.3824,  3.1231,   2.3561},  // pulse-pair for hops
        { 4.3376, -7.4963, -7.8030, -11.6505, -1.1906, -11.4542},  // multilag 2
        { 2.7106, -8.4965, -7.8061,  -9.1933, -0.7019,  -8.4546},  // multilag 3
        { 3.7372, -4.2926, -4.1635,  -6.0751, -0.7788,  -5.9091},  // multilag 4
        { 0.0000,  0.0000,  0.0000,   0.0000,  0.0000,   0.0000}   // spectral moment
    };
    RKFloat P[][7] = {
        { 0.4856, -0.4533, -0.4636, -0.5404, -0.4298, -0.5248},
        { 0.4155, -0.8567, -0.7188, -0.7400, -0.4405, -0.6962},
        { 0.4856, -0.4533, -0.4636, -0.5404, -0.4298, -0.5248},
        { 0.4856, -0.4533, -0.4636, -0.5404, -0.4298, -0.5248},
        { 0.4856, -0.4533, -0.4636, -0.5404, -0.4298, -0.5248},
        { 0.0000,  0.0000,  0.0000,  0.0000,  0.0000,  0.0000}
    };
    RKFloat R[][7] = {
        {0.9024, 0.7063, 0.8050, 0.7045, 0.8717, 0.7079},
        {0.9858, 0.8144, 0.8593, 0.9104, 0.9476, 0.9236},
        {1.8119, 2.5319, 2.9437, 6.7856, 2.6919, 8.4917},
        {1.0677, 1.1674, 1.3540, 2.2399, 1.3389, 2.6234},
        {1.3820, 1.4968, 1.6693, 2.4468, 1.6047, 2.7012},
        {0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000}
    };

    // Select the row for the correct answer
    int row = 0;
    if (method == RKPulsePairHop) {
        row = 1;
    } else if (method == RKMultiLag && lag >=2 && lag <= 4) {
        row = lag;
    }

    // Error of ZDR
    err = 0.0;
    for (k = 0; k < gateCount; k++) {
        err += D[row][k] - space->ZDR[k];
    }
    err /= (RKFloat)gateCount;
    sprintf(str, "Delta ZDR = %.4e", err);
    TEST_RESULT(rkGlobalParameters.showColor, str, fabsf(err) < 1.0e-4);

    // Error of PhiDP
    err = 0.0;
    for (k = 0; k < gateCount; k++) {
        err += P[row][k] - space->PhiDP[k];
    }
    err /= (RKFloat)gateCount;
    sprintf(str, "Delta PhiDP = %.4e", err);
    TEST_RESULT(rkGlobalParameters.showColor, str, fabsf(err) < 1.0e-4);

    // Error of RhoHV
    err = 0.0;
    for (k = 0; k < gateCount; k++) {
        err += R[row][k] - space->RhoHV[k];
    }
    err /= (RKFloat)gateCount;
    sprintf(str, "Delta RhoHV = %.4e", err);
    TEST_RESULT(rkGlobalParameters.showColor, str, fabsf(err) < 1.0e-4);

    RKLog("Deallocating buffers ...\n");

    RKScratchFree(space);
    RKFFTModuleFree(fftModule);
    RKPulseBufferFree(pulseBuffer);
}

void RKTestOneRaySpectra(int method(RKScratch *, RKPulse **, const uint16_t), const int lag) {
    const float spec[][80] = {
        {0.0394, 0.0153, 0.0034, 0.0249, 0.0389, 0.0334, 0.0041, 0.0247, 0.0387, 0.0309, 0.0192, 0.0062, 0.0239, 0.0317, 0.0461, -0.0014, 0.0450, 0.0026, 0.0116, 0.0184, 0.0348, 0.0103, 0.0443, 0.0048, 0.0362, -0.0035, 0.0124, 0.0067, 0.0327, 0.0158, 0.0441, 0.0442, -0.0034, 0.0303, 0.0326, 0.0243, 0.0274, 0.0273, 0.0256, 0.0023, 0.0404, 0.0393, 0.0420, 0.0282, 0.0144, 0.0911, 0.2050, 0.4137, 0.4217, 0.4863, 0.2174, 0.0581, 0.0545, 0.0188, 0.0480, 0.0339, 0.0441, 0.0350, 0.0194, 0.0089, 0.0220, 0.0508, 0.0281, 0.0306, 0.0326, 0.0354, 0.0213, 0.0463, 0.0251, 0.0293, 0.0427, 0.0194, 0.0112, 0.0175, 0.0439, 0.0369, 0.0255, 0.0194, -0.0013, 0.0376},
        {0.0188, -0.0023, -0.0020, 0.0234, 0.0203, 0.0337, 0.0154, 0.0401, 0.0272, 0.0533, 0.0353, 0.0240, 0.0080, 0.0184, 0.0250, 0.0248, 0.0410, 0.0198, 0.0271, 0.0095, 0.0392, 0.0269, 0.0015, 0.0183, 0.0322, 0.0401, 0.0296, 0.0433, -0.0030, 0.0249, 0.0063, 0.0312, 0.0121, 0.0349, 0.0388, 0.0374, 0.0236, 0.0147, 0.0236, 0.0312, 0.0310, 0.0110, 0.0125, 0.0625, 0.1467, 0.2478, 0.5613, 0.4319, 0.3899, 0.1563, 0.0545, 0.0208, 0.0217, 0.0273, 0.0159, 0.0193, 0.0452, 0.0403, 0.0331, 0.0291, 0.0008, 0.0090, 0.0315, 0.0394, 0.0477, 0.0420, 0.0279, 0.0081, 0.0384, 0.0277, 0.0408, 0.0409, -0.0015, 0.0408, 0.0131, 0.0288, 0.0477, 0.0328, 0.0446, 0.0028},
        {0.0486, 0.0044, 0.0191, 0.0424, 0.0165, 0.0440, 0.0216, 0.0271, 0.0461, 0.0417, 0.0395, 0.0275, 0.0264, 0.0321, 0.0432, 0.0157, 0.0164, 0.0056, 0.0330, 0.0488, 0.0301, 0.0052, 0.0010, 0.0216, 0.0133, 0.0133, 0.0436, -0.0009, 0.0135, 0.0209, 0.0339, 0.0285, 0.0429, 0.0172, 0.0290, 0.0167, 0.0209, 0.0427, 0.0135, 0.0452, 0.0284, 0.0548, 0.1148, 0.2334, 0.4289, 0.3897, 0.6166, 0.2957, 0.1797, 0.0565, 0.0310, 0.0144, 0.0275, 0.0500, 0.0488, 0.0017, 0.0093, 0.0288, 0.0181, 0.0309, 0.0216, 0.0218, 0.0388, 0.0035, 0.0149, 0.0517, 0.0337, 0.0308, 0.0215, 0.0394, 0.0144, 0.0471, 0.0131, 0.0215, 0.0286, 0.0196, 0.0445, 0.0258, 0.0087, 0.0256},
        {0.0332, 0.0111, 0.0035, 0.0199, 0.0441, 0.0507, 0.0163, 0.0332, 0.0067, 0.0236, 0.0046, 0.0416, 0.0260, 0.0282, 0.0071, 0.0162, 0.0180, 0.0033, 0.0310, 0.0111, 0.0202, 0.0112, 0.0484, 0.0090, 0.0061, 0.0154, 0.0172, 0.0308, 0.0496, 0.0437, 0.0257, 0.0410, 0.0162, 0.0053, 0.0305, 0.0233, 0.0254, 0.0379, 0.0405, 0.0328, 0.0993, 0.1840, 0.3994, 0.5495, 0.5981, 0.3567, 0.2871, 0.1970, 0.0915, 0.0184, 0.0403, 0.0251, 0.0337, 0.0243, 0.0469, 0.0183, 0.0501, 0.0448, 0.0155, 0.0050, 0.0097, 0.0029, 0.0430, 0.0352, 0.0333, 0.0091, 0.0311, 0.0164, -0.0029, 0.0276, 0.0038, 0.0462, 0.0152, 0.0533, 0.0419, 0.0247, 0.0066, 0.0034, 0.0194, 0.0065},
        {0.0094, 0.0218, 0.0343, 0.0476, 0.0206, 0.0063, 0.0498, 0.0104, 0.0212, 0.0472, 0.0391, 0.0075, 0.0268, 0.0452, 0.0248, 0.0002, 0.0438, 0.0242, 0.0059, 0.0222, 0.0041, 0.0286, 0.0338, 0.0051, 0.0349, 0.0088, 0.0339, 0.0162, 0.0279, 0.0385, 0.0183, 0.0371, 0.0341, 0.0330, 0.0486, 0.0550, 0.0149, 0.0690, 0.0423, 0.1583, 0.3137, 0.4177, 0.5366, 0.5777, 0.4715, 0.3648, 0.1569, 0.0710, 0.0681, 0.0307, 0.0080, 0.0109, 0.0432, 0.0233, 0.0275, 0.0226, 0.0161, 0.0384, 0.0514, 0.0466, 0.0340, 0.0107, 0.0016, 0.0120, 0.0110, 0.0344, 0.0042, 0.0051, 0.0518, 0.0066, 0.0328, 0.0196, 0.0274, 0.0354, 0.0256, 0.0352, 0.0122, 0.0165, 0.0268, 0.0351},
        {0.0105, 0.0475, 0.0353, 0.0426, 0.0140, 0.0443, 0.0267, 0.0338, 0.0249, 0.0213, 0.0103, 0.0369, 0.0438, 0.0199, 0.0479, 0.0086, 0.0425, 0.0205, 0.0335, 0.0211, 0.0473, 0.0081, 0.0056, -0.0009, 0.0383, 0.0344, 0.0033, 0.0038, 0.0037, 0.0421, 0.0273, 0.0448, 0.0205, 0.0106, 0.0342, 0.0410, 0.0688, 0.0848, 0.1459, 0.2851, 0.5326, 0.5496, 0.4331, 0.4200, 0.2563, 0.1731, 0.1257, 0.0575, 0.0402, 0.0347, 0.0445, 0.0192, 0.0325, 0.0383, 0.0162, 0.0149, 0.0453, 0.0369, 0.0220, 0.0386, 0.0063, 0.0452, 0.0364, 0.0241, 0.0340, 0.0144, 0.0303, 0.0233, 0.0052, 0.0291, 0.0382, 0.0111, 0.0060, 0.0055, 0.0040, 0.0155, 0.0151, 0.0088, 0.0502, 0.0476},
        {0.0080, 0.0036, 0.0116, 0.0485, 0.0482, 0.0245, 0.0454, 0.0302, 0.0169, 0.0187, 0.0075, 0.0403, 0.0078, 0.0241, 0.0228, 0.0461, 0.0384, 0.0131, 0.0314, 0.0287, 0.0210, 0.0214, 0.0516, 0.0404, 0.0052, 0.0310, 0.0165, 0.0102, 0.0120, 0.0357, 0.0302, 0.0149, 0.0153, 0.0228, 0.0493, 0.0870, 0.1522, 0.2388, 0.2554, 0.3913, 0.6099, 0.4106, 0.4201, 0.3598, 0.1878, 0.1122, 0.0884, 0.0265, 0.0398, 0.0414, 0.0134, 0.0118, 0.0285, 0.0391, 0.0221, 0.0314, 0.0531, 0.0349, 0.0098, 0.0087, 0.0096, 0.0204, 0.0440, 0.0212, 0.0417, 0.0063, 0.0084, 0.0434, 0.0008, 0.0497, 0.0154, 0.0405, 0.0177, 0.0277, 0.0244, 0.0462, 0.0090, 0.0376, 0.0070, 0.0047},
        {0.0224, 0.0153, 0.0354, 0.0256, 0.0408, 0.0413, 0.0488, 0.0330, 0.0373, 0.0411, 0.0420, 0.0228, 0.0017, 0.0155, 0.0328, 0.0368, 0.0169, 0.0072, 0.0004, 0.0262, 0.0192, 0.0100, 0.0172, 0.0225, 0.0301, 0.0546, 0.0449, 0.0388, 0.0223, -0.0024, 0.0108, 0.0191, 0.0605, 0.0749, 0.1103, 0.2169, 0.2569, 0.3168, 0.5121, 0.6360, 0.4757, 0.5976, 0.3825, 0.2321, 0.1945, 0.1110, 0.0619, 0.0311, 0.0017, 0.0427, 0.0399, 0.0091, 0.0450, 0.0219, -0.0017, 0.0148, 0.0121, 0.0011, 0.0281, 0.0436, 0.0268, 0.0309, 0.0257, 0.0087, 0.0320, 0.0267, 0.0226, 0.0043, 0.0247, 0.0027, 0.0271, 0.0216, 0.0135, 0.0065, 0.0333, -0.0044, 0.0071, 0.0354, 0.0015, 0.0349},
        {0.0008, 0.0253, 0.0429, 0.0327, 0.0086, -0.0046, 0.0235, 0.0167, 0.0134, 0.0004, 0.0328, 0.0341, 0.0327, 0.0100, 0.0088, 0.0352, 0.0091, 0.0400, 0.0011, 0.0297, 0.0513, 0.0049, 0.0377, 0.0048, 0.0310, 0.0343, 0.0464, 0.0140, 0.0086, 0.0084, 0.0448, 0.0395, 0.0531, 0.1212, 0.1546, 0.2953, 0.4170, 0.4494, 0.4253, 0.5414, 0.5359, 0.3419, 0.2837, 0.2030, 0.1698, 0.0484, 0.0253, 0.0635, 0.0012, 0.0122, 0.0389, 0.0347, 0.0399, 0.0051, 0.0125, 0.0106, 0.0161, 0.0052, 0.0126, 0.0049, 0.0089, 0.0024, 0.0149, 0.0376, 0.0055, 0.0435, 0.0155, 0.0144, 0.0359, 0.0253, 0.0394, 0.0188, 0.0472, 0.0336, 0.0308, 0.0379, 0.0178, 0.0213, 0.0028, 0.0064},
        {0.0216, 0.0460, 0.0224, 0.0419, 0.0097, 0.0451, 0.0272, 0.0204, 0.0119, 0.0529, 0.0393, 0.0163, 0.0226, 0.0453, 0.0066, 0.0357, 0.0134, 0.0330, 0.0496, 0.0222, -0.0035, 0.0516, 0.0392, 0.0102, 0.0398, 0.0370, 0.0326, 0.0315, 0.0249, 0.0239, 0.0513, 0.0298, 0.1089, 0.1460, 0.2571, 0.3449, 0.4509, 0.4980, 0.4590, 0.5749, 0.4229, 0.4207, 0.2769, 0.2029, 0.1074, 0.0747, 0.0525, 0.0504, 0.0238, 0.0476, 0.0109, 0.0020, 0.0477, 0.0336, -0.0002, 0.0286, 0.0256, 0.0074, 0.0426, 0.0180, 0.0128, 0.0126, 0.0318, 0.0196, 0.0325, 0.0227, 0.0431, 0.0475, 0.0486, 0.0441, 0.0250, 0.0050, 0.0270, 0.0004, 0.0149, 0.0249, 0.0460, 0.0062, 0.0130, 0.0243},
        {0.0298, 0.0287, 0.0329, 0.0039, 0.0168, 0.0228, 0.0539, 0.0398, 0.0445, 0.0274, 0.0094, 0.0062, 0.0119, 0.0087, 0.0093, 0.0387, 0.0158, 0.0140, 0.0312, 0.0481, 0.0338, 0.0505, 0.0344, -0.0014, 0.0284, 0.0231, 0.0345, 0.0258, 0.0322, 0.0506, 0.0726, 0.1152, 0.1776, 0.1926, 0.2766, 0.3875, 0.4967, 0.5848, 0.4748, 0.4094, 0.4403, 0.3963, 0.3354, 0.1950, 0.1292, 0.0597, 0.0420, 0.0542, 0.0247, 0.0230, 0.0559, 0.0265, 0.0141, 0.0364, 0.0219, 0.0390, 0.0494, 0.0517, 0.0313, 0.0327, 0.0171, 0.0029, 0.0288, 0.0303, 0.0249, 0.0427, 0.0120, 0.0356, 0.0055, 0.0366, 0.0033, 0.0411, 0.0199, 0.0228, 0.0124, 0.0314, 0.0549, 0.0418, 0.0321, 0.0185},
        {0.0118, 0.0272, 0.0131, 0.0150, 0.0462, 0.0390, 0.0137, 0.0402, 0.0144, 0.0395, 0.0231, 0.0418, 0.0390, 0.0336, 0.0020, 0.0372, 0.0412, 0.0128, 0.0211, 0.0046, 0.0260, 0.0280, 0.0122, 0.0121, 0.0132, 0.0118, 0.0134, 0.0567, 0.0391, 0.0628, 0.1241, 0.0967, 0.2273, 0.2828, 0.3781, 0.4179, 0.5619, 0.6227, 0.3956, 0.3533, 0.4831, 0.4263, 0.1978, 0.2114, 0.1315, 0.0483, 0.0382, 0.0595, 0.0454, 0.0466, 0.0183, 0.0133, 0.0290, 0.0217, 0.0399, 0.0269, 0.0236, 0.0281, 0.0282, 0.0112, 0.0308, 0.0419, 0.0189, 0.0221, 0.0046, -0.0042, 0.0204, 0.0325, 0.0199, 0.0355, 0.0348, 0.0076, 0.0492, 0.0163, 0.0067, 0.0089, 0.0404, 0.0130, 0.0165, 0.0416},
        {0.0009, 0.0064, 0.0094, 0.0109, 0.0380, 0.0452, 0.0252, 0.0553, 0.0283, 0.0028, 0.0417, 0.0322, 0.0303, 0.0342, 0.0051, -0.0007, 0.0289, 0.0352, 0.0330, 0.0115, 0.0307, 0.0118, 0.0284, 0.0375, 0.0076, 0.0163, 0.0366, 0.0367, 0.0372, 0.0701, 0.1265, 0.1575, 0.2208, 0.2767, 0.4364, 0.5593, 0.4407, 0.4451, 0.4616, 0.4925, 0.4676, 0.2992, 0.2501, 0.1987, 0.1051, 0.0809, 0.0757, 0.0466, 0.0501, 0.0555, 0.0131, 0.0088, 0.0150, 0.0378, 0.0367, 0.0326, 0.0039, 0.0176, 0.0424, 0.0431, 0.0119, 0.0494, 0.0069, 0.0297, 0.0510, 0.0354, 0.0299, 0.0286, 0.0304, 0.0295, 0.0180, 0.0052, 0.0475, 0.0360, 0.0398, 0.0477, 0.0133, 0.0199, 0.0109, 0.0131},
        {0.0383, 0.0075, 0.0288, 0.0071, 0.0064, 0.0126, 0.0069, 0.0066, 0.0104, 0.0310, 0.0291, 0.0286, 0.0042, 0.0413, 0.0213, 0.0076, 0.0242, 0.0117, 0.0536, 0.0108, 0.0317, 0.0457, 0.0385, 0.0152, 0.0283, 0.0196, 0.0453, 0.0334, 0.0646, 0.0571, 0.1139, 0.1190, 0.2059, 0.2363, 0.3570, 0.4499, 0.5289, 0.6148, 0.4074, 0.6051, 0.3132, 0.4101, 0.2823, 0.1999, 0.1799, 0.1248, 0.0577, 0.0821, 0.0668, 0.0422, 0.0106, 0.0117, 0.0005, 0.0223, 0.0027, -0.0012, 0.0138, 0.0452, 0.0101, 0.0510, 0.0255, -0.0022, 0.0396, 0.0404, 0.0516, 0.0056, 0.0181, 0.0129, 0.0318, 0.0373, 0.0431, 0.0452, 0.0421, 0.0161, 0.0102, 0.0140, 0.0288, 0.0445, 0.0043, 0.0159},
        {0.0371, 0.0408, 0.0270, 0.0035, 0.0314, 0.0255, 0.0237, 0.0340, 0.0509, 0.0216, 0.0270, 0.0280, 0.0166, 0.0346, 0.0236, 0.0524, 0.0268, 0.0142, 0.0280, 0.0348, 0.0120, 0.0145, 0.0475, 0.0275, 0.0271, 0.0318, 0.0333, 0.0692, 0.0671, 0.0875, 0.1373, 0.1366, 0.2802, 0.2232, 0.3529, 0.3908, 0.5209, 0.4393, 0.5125, 0.6195, 0.3562, 0.4462, 0.2828, 0.2917, 0.1705, 0.1288, 0.0602, 0.0588, 0.0528, 0.0164, 0.0502, 0.0362, 0.0227, 0.0440, 0.0070, 0.0434, 0.0222, 0.0365, 0.0469, 0.0404, 0.0104, -0.0011, 0.0212, 0.0421, 0.0373, 0.0054, 0.0431, 0.0365, 0.0400, 0.0183, 0.0183, 0.0071, 0.0032, 0.0447, 0.0004, 0.0050, 0.0278, 0.0448, 0.0232, 0.0134},
        {0.0212, 0.0373, 0.0442, 0.0418, 0.0387, 0.0338, 0.0096, 0.0123, 0.0252, 0.0084, 0.0450, 0.0125, 0.0420, 0.0522, 0.0202, 0.0218, 0.0073, 0.0152, 0.0121, 0.0208, 0.0112, 0.0201, 0.0450, 0.0501, 0.0490, 0.0159, 0.0631, 0.0637, 0.0651, 0.0904, 0.1224, 0.1565, 0.1920, 0.3225, 0.4125, 0.4061, 0.4249, 0.5859, 0.4615, 0.4295, 0.6094, 0.5100, 0.3091, 0.2847, 0.2199, 0.1918, 0.1412, 0.1249, 0.0564, 0.0754, 0.0200, 0.0364, 0.0299, -0.0013, 0.0209, 0.0487, 0.0286, 0.0192, 0.0418, 0.0333, 0.0533, 0.0063, 0.0038, 0.0297, 0.0452, 0.0212, 0.0287, 0.0150, -0.0012, 0.0397, 0.0510, 0.0530, 0.0065, 0.0084, 0.0379, 0.0131, -0.0011, 0.0026, 0.0026, 0.0237},
        {0.0425, 0.0099, -0.0057, 0.0345, 0.0236, 0.0148, 0.0273, 0.0181, 0.0227, 0.0383, 0.0386, 0.0482, 0.0509, 0.0272, 0.0291, 0.0318, 0.0126, 0.0422, -0.0037, 0.0484, 0.0332, 0.0227, 0.0343, 0.0304, 0.0110, 0.0251, 0.0531, 0.0615, 0.0646, 0.1018, 0.1120, 0.1506, 0.1685, 0.3139, 0.3292, 0.4285, 0.4287, 0.4727, 0.4928, 0.6235, 0.6112, 0.5343, 0.4945, 0.3122, 0.2848, 0.3074, 0.1847, 0.1237, 0.1146, 0.0605, 0.0330, 0.0145, 0.0244, 0.0173, 0.0104, 0.0480, 0.0110, 0.0506, 0.0402, 0.0192, 0.0184, 0.0370, 0.0196, 0.0424, 0.0158, 0.0275, 0.0303, 0.0068, -0.0026, 0.0062, 0.0428, 0.0104, 0.0175, 0.0022, 0.0483, 0.0388, 0.0041, 0.0539, 0.0156, 0.0131},
        {0.0224, 0.0324, 0.0198, 0.0171, 0.0260, 0.0214, 0.0235, 0.0043, 0.0433, 0.0237, 0.0381, 0.0093, 0.0207, 0.0505, 0.0397, 0.0339, 0.0394, 0.0385, 0.0321, 0.0364, 0.0447, 0.0135, 0.0311, 0.0325, 0.0395, 0.0505, 0.0302, 0.0554, 0.0642, 0.0697, 0.1088, 0.1626, 0.1632, 0.3130, 0.3696, 0.3261, 0.3484, 0.4500, 0.5733, 0.4464, 0.6247, 0.4051, 0.4185, 0.3727, 0.3582, 0.3309, 0.2517, 0.1805, 0.1276, 0.1114, 0.0773, 0.0905, 0.0444, 0.0281, 0.0399, 0.0387, 0.0018, 0.0075, 0.0458, 0.0393, 0.0475, 0.0300, 0.0177, 0.0010, 0.0088, 0.0157, 0.0389, 0.0005, 0.0390, 0.0043, 0.0345, 0.0024, 0.0264, 0.0317, 0.0410, -0.0001, 0.0100, 0.0256, 0.0422, 0.0252},
        {0.0175, 0.0278, 0.0444, 0.0341, 0.0417, 0.0085, 0.0197, 0.0378, 0.0239, 0.0334, 0.0223, 0.0514, 0.0335, 0.0074, 0.0182, 0.0401, 0.0284, 0.0228, 0.0326, 0.0434, 0.0457, 0.0310, 0.0283, 0.0198, 0.0454, 0.0485, 0.0240, 0.0161, 0.0322, 0.0734, 0.1070, 0.1373, 0.1666, 0.2232, 0.2922, 0.2344, 0.4480, 0.3084, 0.4058, 0.5796, 0.6127, 0.5760, 0.5252, 0.3782, 0.3942, 0.4378, 0.2924, 0.2634, 0.1592, 0.1799, 0.0928, 0.0884, 0.0847, 0.0598, 0.0310, 0.0393, 0.0346, 0.0476, 0.0137, 0.0077, 0.0273, 0.0493, 0.0358, 0.0156, 0.0366, 0.0277, 0.0428, 0.0300, 0.0331, 0.0043, 0.0061, 0.0227, 0.0231, 0.0061, 0.0195, -0.0023, 0.0098, 0.0124, 0.0220, 0.0306},
        {0.0345, 0.0174, 0.0177, 0.0088, 0.0480, 0.0302, 0.0062, 0.0096, 0.0434, -0.0004, 0.0173, 0.0485, 0.0102, 0.0315, 0.0428, 0.0145, 0.0281, 0.0358, 0.0048, 0.0271, 0.0052, 0.0275, 0.0163, 0.0337, 0.0257, 0.0143, 0.0427, 0.0484, 0.0504, 0.0560, 0.0973, 0.1157, 0.1782, 0.1896, 0.1850, 0.2755, 0.2676, 0.3889, 0.3725, 0.5491, 0.5367, 0.4912, 0.4488, 0.5070, 0.3819, 0.4742, 0.3091, 0.2603, 0.2123, 0.1891, 0.1804, 0.1472, 0.0880, 0.0933, 0.0436, 0.0499, 0.0194, 0.0268, 0.0514, 0.0246, 0.0168, 0.0268, 0.0514, 0.0379, 0.0442, 0.0039, 0.0271, 0.0119, 0.0141, 0.0247, 0.0076, 0.0243, 0.0348, 0.0324, 0.0252, 0.0390, 0.0360, 0.0068, 0.0251, 0.0492},
        {0.0239, 0.0327, -0.0047, 0.0012, 0.0061, 0.0334, 0.0291, 0.0257, 0.0245, 0.0524, 0.0002, 0.0545, 0.0500, 0.0369, 0.0321, 0.0151, 0.0034, 0.0316, 0.0368, 0.0056, 0.0029, 0.0307, 0.0233, 0.0462, 0.0390, 0.0074, 0.0496, 0.0326, 0.0384, 0.0510, 0.0541, 0.0731, 0.1094, 0.1479, 0.2028, 0.2078, 0.2919, 0.3638, 0.4540, 0.4518, 0.3665, 0.4244, 0.6123, 0.5305, 0.4867, 0.5866, 0.4385, 0.4104, 0.2812, 0.2597, 0.1955, 0.1924, 0.1268, 0.1384, 0.0801, 0.0356, 0.0654, 0.0233, 0.0513, 0.0064, 0.0309, 0.0324, 0.0239, 0.0165, 0.0143, 0.0215, 0.0216, 0.0506, 0.0090, 0.0286, 0.0207, 0.0264, 0.0056, -0.0052, 0.0363, 0.0531, 0.0153, -0.0046, 0.0372, 0.0324},
        {-0.0004, 0.0411, 0.0120, 0.0374, 0.0281, 0.0389, 0.0043, 0.0534, 0.0213, 0.0041, 0.0082, 0.0221, 0.0425, 0.0115, 0.0008, 0.0187, 0.0112, 0.0043, 0.0289, 0.0188, 0.0159, 0.0223, 0.0273, 0.0513, 0.0443, 0.0404, 0.0454, 0.0333, 0.0108, 0.0299, 0.0448, 0.0960, 0.1213, 0.0789, 0.1341, 0.2082, 0.2618, 0.3069, 0.2803, 0.4295, 0.4641, 0.4188, 0.4170, 0.5637, 0.4738, 0.5908, 0.6038, 0.5155, 0.5243, 0.3030, 0.3558, 0.1852, 0.1923, 0.1069, 0.1094, 0.1197, 0.0658, 0.0514, 0.0505, 0.0300, 0.0019, 0.0129, 0.0080, -0.0017, 0.0311, 0.0183, 0.0481, 0.0046, 0.0267, 0.0515, 0.0060, 0.0448, 0.0067, 0.0521, 0.0287, -0.0008, 0.0109, 0.0054, 0.0098, 0.0376},
        {0.0345, 0.0346, 0.0173, 0.0202, 0.0459, 0.0172, 0.0474, 0.0172, 0.0033, 0.0170, 0.0404, 0.0176, 0.0546, 0.0381, 0.0514, 0.0471, 0.0370, 0.0058, 0.0271, 0.0045, 0.0357, 0.0216, 0.0004, 0.0165, 0.0127, 0.0316, 0.0344, 0.0415, 0.0225, 0.0662, 0.0589, 0.0553, 0.0648, 0.0758, 0.1278, 0.1975, 0.1669, 0.2479, 0.1953, 0.2487, 0.4360, 0.4980, 0.5276, 0.5510, 0.5346, 0.5823, 0.6425, 0.4396, 0.4866, 0.4877, 0.4531, 0.2752, 0.2865, 0.2228, 0.2114, 0.1049, 0.1512, 0.0601, 0.0747, 0.0581, 0.0317, 0.0261, 0.0517, 0.0408, 0.0423, 0.0191, 0.0079, 0.0277, 0.0288, 0.0077, 0.0320, 0.0135, 0.0506, 0.0157, 0.0311, 0.0274, 0.0157, 0.0462, 0.0501, 0.0435},
        {0.0458, 0.0428, 0.0337, 0.0475, -0.0003, 0.0492, 0.0019, 0.0087, 0.0155, 0.0025, 0.0223, 0.0429, 0.0230, 0.0132, 0.0182, 0.0178, 0.0437, 0.0227, 0.0178, 0.0236, 0.0516, 0.0295, 0.0178, 0.0415, 0.0375, 0.0154, 0.0111, 0.0066, 0.0280, 0.0326, 0.0388, 0.0346, 0.0810, 0.0567, 0.0874, 0.1208, 0.1540, 0.1624, 0.2853, 0.2649, 0.3753, 0.2609, 0.3709, 0.5770, 0.3955, 0.4335, 0.6330, 0.4980, 0.6073, 0.6200, 0.3798, 0.4591, 0.3371, 0.3312, 0.3154, 0.1791, 0.1578, 0.1359, 0.0976, 0.0860, 0.0781, 0.0502, 0.0695, 0.0206, 0.0518, 0.0571, 0.0437, 0.0524, 0.0167, 0.0448, 0.0473, 0.0362, 0.0134, 0.0248, 0.0215, 0.0337, 0.0392, 0.0387, 0.0315, 0.0428},
        {0.0009, 0.0402, 0.0365, 0.0140, 0.0076, 0.0414, 0.0320, 0.0423, 0.0038, 0.0248, 0.0350, 0.0227, 0.0108, 0.0322, 0.0389, 0.0081, 0.0387, 0.0080, 0.0386, 0.0282, 0.0384, 0.0242, 0.0428, 0.0185, 0.0045, 0.0093, 0.0101, -0.0007, 0.0315, 0.0497, 0.0221, 0.0613, 0.0443, 0.0783, 0.0938, 0.0853, 0.1301, 0.1713, 0.1786, 0.2503, 0.2962, 0.3530, 0.2883, 0.3118, 0.3929, 0.6001, 0.4595, 0.6244, 0.6263, 0.4161, 0.3630, 0.5652, 0.4582, 0.3900, 0.2689, 0.3324, 0.2002, 0.2386, 0.2060, 0.1195, 0.1216, 0.0970, 0.0884, 0.0342, 0.0805, 0.0460, 0.0222, 0.0246, 0.0470, 0.0155, 0.0132, 0.0238, 0.0438, 0.0132, 0.0197, 0.0503, 0.0131, 0.0204, 0.0205, 0.0214},
        {-0.0024, 0.0408, 0.0121, 0.0409, 0.0354, -0.0020, 0.0078, 0.0305, 0.0130, 0.0217, 0.0073, 0.0378, 0.0504, 0.0363, 0.0364, 0.0329, 0.0100, 0.0132, 0.0444, 0.0344, 0.0242, 0.0315, 0.0313, 0.0380, 0.0424, 0.0220, 0.0160, 0.0012, 0.0140, 0.0340, 0.0462, 0.0635, 0.0246, 0.0683, 0.0741, 0.0829, 0.0718, 0.1117, 0.1080, 0.1924, 0.2603, 0.2718, 0.3067, 0.4205, 0.3527, 0.3901, 0.4720, 0.4780, 0.5090, 0.6326, 0.4511, 0.4768, 0.5113, 0.3832, 0.4045, 0.3376, 0.2913, 0.3128, 0.2083, 0.2017, 0.1096, 0.1363, 0.0940, 0.0851, 0.0757, 0.0586, 0.0299, 0.0239, 0.0533, 0.0432, 0.0179, 0.0491, 0.0202, 0.0092, 0.0146, -0.0014, 0.0281, 0.0141, -0.0019, 0.0430},
        {0.0082, 0.0229, 0.0311, 0.0494, 0.0223, 0.0136, 0.0210, 0.0113, 0.0006, 0.0214, 0.0170, 0.0208, 0.0197, 0.0285, 0.0145, 0.0086, 0.0430, 0.0238, 0.0386, 0.0528, 0.0128, 0.0231, 0.0297, 0.0098, 0.0097, 0.0039, 0.0258, 0.0518, 0.0202, 0.0274, 0.0450, 0.0446, 0.0310, 0.0359, 0.0576, 0.0756, 0.0907, 0.1047, 0.1418, 0.1676, 0.2216, 0.2217, 0.2533, 0.2480, 0.3247, 0.4000, 0.4434, 0.4189, 0.5780, 0.6372, 0.4527, 0.4197, 0.4442, 0.5883, 0.5687, 0.3679, 0.3436, 0.3225, 0.2116, 0.2729, 0.2853, 0.1857, 0.1653, 0.1046, 0.0641, 0.0883, 0.0323, 0.0492, 0.0312, 0.0544, 0.0380, 0.0265, 0.0172, 0.0551, 0.0179, 0.0438, 0.0449, 0.0264, 0.0210, 0.0197},
        {0.0438, 0.0401, 0.0434, 0.0256, 0.0455, 0.0130, 0.0231, 0.0291, 0.0176, -0.0052, 0.0171, 0.0179, 0.0380, 0.0512, 0.0447, 0.0366, 0.0014, 0.0131, 0.0193, 0.0142, 0.0166, 0.0161, 0.0271, 0.0006, 0.0160, 0.0038, 0.0244, 0.0281, 0.0217, 0.0534, 0.0443, 0.0136, 0.0270, 0.0413, 0.0714, 0.0852, 0.0973, 0.0819, 0.0919, 0.1014, 0.1005, 0.1957, 0.2116, 0.2491, 0.2622, 0.3246, 0.3624, 0.4606, 0.3916, 0.5925, 0.4701, 0.5230, 0.5480, 0.5935, 0.4627, 0.5024, 0.4987, 0.3719, 0.3471, 0.3515, 0.3638, 0.2862, 0.1584, 0.1733, 0.1303, 0.0915, 0.0893, 0.0691, 0.0651, 0.0316, 0.0451, 0.0280, 0.0388, 0.0314, 0.0330, 0.0358, 0.0384, 0.0092, 0.0394, 0.0156},
        {0.0129, 0.0428, 0.0109, 0.0197, 0.0072, 0.0322, 0.0055, 0.0256, 0.0080, 0.0050, 0.0377, 0.0184, 0.0246, 0.0378, 0.0145, 0.0280, 0.0249, 0.0471, 0.0487, 0.0288, 0.0221, 0.0068, 0.0109, 0.0359, 0.0461, 0.0242, 0.0192, 0.0371, 0.0445, 0.0348, 0.0458, 0.0406, 0.0488, 0.0208, 0.0293, 0.0198, 0.0667, 0.0719, 0.0828, 0.0934, 0.1052, 0.1181, 0.2090, 0.1741, 0.3215, 0.3449, 0.4293, 0.3234, 0.3363, 0.3939, 0.5635, 0.5365, 0.5751, 0.6283, 0.4423, 0.5016, 0.3453, 0.4169, 0.4463, 0.3947, 0.2451, 0.2824, 0.3100, 0.2047, 0.1746, 0.1899, 0.1340, 0.1404, 0.0856, 0.0736, 0.0616, 0.0770, 0.0365, 0.0634, 0.0356, 0.0468, 0.0146, 0.0221, 0.0115, 0.0396},
        {0.0286, 0.0174, 0.0170, 0.0222, 0.0407, 0.0170, 0.0091, 0.0426, 0.0411, 0.0156, 0.0106, -0.0012, 0.0279, 0.0433, 0.0112, 0.0354, 0.0172, 0.0153, 0.0474, 0.0157, 0.0508, 0.0147, 0.0320, -0.0017, 0.0441, 0.0100, 0.0355, 0.0111, 0.0178, 0.0075, 0.0305, 0.0363, 0.0539, 0.0486, 0.0133, 0.0456, 0.0449, 0.0486, 0.0437, 0.0562, 0.1035, 0.0907, 0.2013, 0.1914, 0.2114, 0.2627, 0.3838, 0.3770, 0.3058, 0.3933, 0.3777, 0.5624, 0.5082, 0.4891, 0.6233, 0.4376, 0.4845, 0.5430, 0.3973, 0.3359, 0.4798, 0.4040, 0.2934, 0.2771, 0.2266, 0.1969, 0.1794, 0.1423, 0.1017, 0.0875, 0.0814, 0.0615, 0.0385, 0.0171, 0.0329, 0.0436, 0.0252, 0.0208, 0.0313, 0.0062},
        {0.0345, 0.0391, 0.0315, 0.0394, 0.0230, 0.0323, 0.0456, 0.0341, 0.0494, 0.0215, 0.0246, 0.0216, 0.0342, 0.0212, 0.0080, 0.0216, 0.0154, 0.0223, 0.0473, 0.0298, 0.0400, 0.0039, 0.0245, 0.0051, 0.0011, 0.0472, 0.0263, 0.0496, 0.0095, 0.0050, 0.0477, 0.0243, 0.0268, 0.0060, 0.0417, 0.0423, 0.0446, 0.0697, 0.0549, 0.0712, 0.0831, 0.1253, 0.1563, 0.1645, 0.2409, 0.1758, 0.2678, 0.2797, 0.4292, 0.4143, 0.4818, 0.3448, 0.4840, 0.6259, 0.4537, 0.6564, 0.4252, 0.4754, 0.4315, 0.3938, 0.5007, 0.3909, 0.3731, 0.3322, 0.2906, 0.3225, 0.1878, 0.1550, 0.1442, 0.1705, 0.0857, 0.0590, 0.0598, 0.0656, 0.0443, 0.0556, 0.0380, 0.0236, 0.0629, 0.0167},
        {0.0413, 0.0480, 0.0409, 0.0106, 0.0447, 0.0371, 0.0427, 0.0070, 0.0337, 0.0373, 0.0407, 0.0132, 0.0320, 0.0127, 0.0343, 0.0433, -0.0025, 0.0218, 0.0430, 0.0148, 0.0272, 0.0084, 0.0366, -0.0010, 0.0335, 0.0473, 0.0367, 0.0165, 0.0070, 0.0446, 0.0473, 0.0415, 0.0283, 0.0195, 0.0444, 0.0600, 0.0512, 0.0736, 0.0450, 0.0730, 0.0744, 0.1210, 0.0895, 0.1377, 0.1127, 0.2558, 0.2159, 0.2376, 0.3149, 0.3203, 0.4318, 0.5227, 0.3884, 0.4714, 0.4661, 0.5232, 0.4997, 0.5692, 0.4173, 0.3596, 0.4911, 0.4052, 0.4166, 0.3277, 0.4029, 0.3776, 0.2848, 0.1756, 0.1673, 0.1780, 0.1084, 0.1317, 0.0962, 0.0846, 0.0728, 0.0608, 0.0349, 0.0489, 0.0661, 0.0153},
        {0.0572, 0.0310, 0.0409, 0.0181, 0.0178, 0.0137, 0.0519, 0.0158, 0.0292, 0.0140, 0.0240, 0.0456, 0.0377, 0.0211, 0.0160, 0.0123, 0.0109, 0.0295, 0.0251, 0.0132, 0.0376, 0.0390, 0.0376, 0.0095, 0.0203, -0.0001, 0.0003, 0.0167, 0.0436, 0.0475, 0.0373, 0.0377, 0.0105, 0.0423, 0.0380, 0.0440, 0.0529, 0.0360, 0.0732, 0.0595, 0.0668, 0.1135, 0.1153, 0.1050, 0.1753, 0.1872, 0.2526, 0.2440, 0.3208, 0.3844, 0.4469, 0.4568, 0.3520, 0.4713, 0.5259, 0.5244, 0.4245, 0.5993, 0.4610, 0.4203, 0.4912, 0.3875, 0.5275, 0.4058, 0.4341, 0.3180, 0.3389, 0.1768, 0.2481, 0.1823, 0.1643, 0.1335, 0.1544, 0.0772, 0.0868, 0.0814, 0.0789, 0.0671, 0.0491, 0.0443},
        {0.0289, 0.0341, 0.0263, 0.0152, 0.0359, 0.0022, 0.0075, 0.0006, 0.0325, 0.0454, 0.0053, 0.0061, 0.0210, 0.0458, 0.0051, 0.0452, 0.0518, 0.0197, -0.0014, 0.0428, 0.0252, 0.0300, 0.0279, 0.0399, 0.0105, 0.0320, 0.0221, 0.0519, 0.0473, 0.0144, 0.0078, 0.0010, 0.0249, 0.0290, 0.0363, 0.0593, 0.0521, 0.0479, 0.0371, 0.0942, 0.0893, 0.0687, 0.1240, 0.1535, 0.1778, 0.2166, 0.2709, 0.2728, 0.2940, 0.3167, 0.3402, 0.3854, 0.5152, 0.3976, 0.4089, 0.4890, 0.4080, 0.4706, 0.4689, 0.5712, 0.5724, 0.3814, 0.4972, 0.5308, 0.3874, 0.3336, 0.2400, 0.2450, 0.3103, 0.1980, 0.2378, 0.2159, 0.1293, 0.1269, 0.0679, 0.1011, 0.0752, 0.0442, 0.0531, 0.0348},
        {0.0757, 0.0559, 0.0651, 0.0105, 0.0175, 0.0183, 0.0204, 0.0463, 0.0419, 0.0433, 0.0232, 0.0062, 0.0332, 0.0032, 0.0313, 0.0304, 0.0355, 0.0214, 0.0125, 0.0345, 0.0392, 0.0349, 0.0337, 0.0033, 0.0350, 0.0402, 0.0143, 0.0078, 0.0278, 0.0248, 0.0329, 0.0443, 0.0162, 0.0225, 0.0495, 0.0673, 0.0643, 0.0352, 0.0714, 0.0856, 0.0427, 0.0760, 0.1459, 0.1287, 0.2010, 0.2285, 0.2042, 0.2860, 0.2365, 0.2487, 0.2761, 0.4467, 0.5212, 0.3620, 0.4343, 0.4457, 0.3863, 0.4533, 0.4692, 0.6433, 0.4261, 0.4679, 0.4108, 0.3736, 0.3940, 0.3271, 0.3677, 0.2948, 0.2521, 0.2087, 0.2465, 0.2376, 0.1697, 0.1145, 0.1622, 0.1175, 0.0905, 0.0398, 0.0627, 0.0271},
        {0.0271, 0.0222, 0.0237, 0.0328, 0.0133, 0.0249, 0.0452, 0.0107, 0.0051, 0.0243, 0.0461, 0.0236, 0.0472, 0.0083, 0.0186, 0.0438, 0.0407, 0.0039, 0.0069, 0.0292, 0.0326, 0.0149, 0.0086, 0.0162, 0.0455, 0.0027, 0.0093, 0.0276, 0.0343, 0.0090, 0.0321, 0.0332, 0.0304, 0.0479, 0.0533, 0.0547, 0.0618, 0.0487, 0.0529, 0.0414, 0.0830, 0.1065, 0.0885, 0.1414, 0.1512, 0.1860, 0.2183, 0.2012, 0.1929, 0.3453, 0.2795, 0.3989, 0.3906, 0.4152, 0.5123, 0.3925, 0.6067, 0.6145, 0.4596, 0.4590, 0.5084, 0.4864, 0.5290, 0.4377, 0.4993, 0.3756, 0.2898, 0.2917, 0.3729, 0.2306, 0.2490, 0.2216, 0.1653, 0.1621, 0.0983, 0.1170, 0.1105, 0.0853, 0.0751, 0.0458},
        {0.0817, 0.0570, 0.0507, 0.0580, 0.0550, 0.0585, 0.0359, 0.0153, 0.0102, 0.0467, 0.0091, 0.0389, 0.0114, 0.0181, 0.0363, 0.0316, 0.0196, 0.0493, -0.0023, 0.0500, -0.0029, 0.0395, 0.0327, 0.0271, 0.0322, 0.0216, 0.0215, 0.0340, 0.0047, 0.0103, 0.0313, 0.0280, 0.0104, 0.0205, 0.0524, 0.0157, 0.0582, 0.0840, 0.0587, 0.0930, 0.0530, 0.0856, 0.1419, 0.0914, 0.1776, 0.2117, 0.1832, 0.2008, 0.3480, 0.3636, 0.3715, 0.3409, 0.3524, 0.4518, 0.3919, 0.4691, 0.5759, 0.6106, 0.5907, 0.4977, 0.4486, 0.4530, 0.5449, 0.4227, 0.5519, 0.4729, 0.3847, 0.4267, 0.3211, 0.2822, 0.2082, 0.1876, 0.1665, 0.1514, 0.1831, 0.1145, 0.1037, 0.0819, 0.0853, 0.0638},
        {0.0935, 0.0199, 0.0585, 0.0451, 0.0581, 0.0129, 0.0007, 0.0186, 0.0480, 0.0092, 0.0001, 0.0182, 0.0265, 0.0032, 0.0344, 0.0524, 0.0350, 0.0050, 0.0285, 0.0221, 0.0234, 0.0475, 0.0229, 0.0055, -0.0051, 0.0356, 0.0131, 0.0132, 0.0421, 0.0046, 0.0144, 0.0448, 0.0271, 0.0484, 0.0389, 0.0236, 0.0641, 0.0512, 0.0779, 0.0964, 0.1187, 0.1281, 0.1706, 0.1763, 0.1812, 0.2130, 0.2096, 0.2377, 0.2815, 0.3446, 0.2934, 0.3545, 0.5313, 0.4009, 0.3495, 0.5269, 0.5478, 0.6389, 0.3883, 0.6068, 0.6556, 0.5260, 0.5727, 0.4416, 0.4416, 0.5022, 0.3515, 0.3804, 0.3928, 0.2998, 0.2796, 0.1918, 0.2318, 0.1859, 0.1309, 0.1503, 0.1123, 0.0954, 0.0403, 0.0434},
        {0.0733, 0.0781, 0.0589, 0.0422, 0.0423, 0.0644, 0.0230, 0.0269, 0.0315, 0.0184, 0.0450, 0.0113, 0.0346, 0.0260, 0.0438, 0.0283, 0.0137, 0.0051, 0.0351, 0.0279, 0.0326, 0.0092, 0.0421, 0.0014, 0.0308, 0.0250, 0.0030, 0.0237, 0.0571, 0.0123, 0.0451, 0.0205, 0.0283, 0.0470, 0.0319, 0.0367, 0.0608, 0.0676, 0.1023, 0.0944, 0.0882, 0.1234, 0.1367, 0.1376, 0.1358, 0.2166, 0.2969, 0.2027, 0.2449, 0.3614, 0.3172, 0.3719, 0.3375, 0.5420, 0.5270, 0.4283, 0.5084, 0.5715, 0.4343, 0.6297, 0.4514, 0.6113, 0.5659, 0.5606, 0.5265, 0.3970, 0.3827, 0.4121, 0.4052, 0.2808, 0.3070, 0.2185, 0.2310, 0.1570, 0.1341, 0.1327, 0.0710, 0.1393, 0.1117, 0.0795},
        {0.0266, 0.0780, 0.0450, 0.0333, 0.0176, 0.0377, 0.0605, 0.0013, 0.0282, 0.0010, 0.0175, 0.0423, 0.0350, 0.0160, 0.0141, 0.0400, 0.0208, 0.0473, 0.0236, 0.0108, -0.0014, 0.0079, 0.0436, 0.0417, 0.0353, 0.0075, 0.0424, 0.0460, 0.0045, 0.0210, 0.0312, 0.0434, 0.0180, 0.0575, 0.0453, 0.0799, 0.0586, 0.0828, 0.0785, 0.0878, 0.1193, 0.1655, 0.1755, 0.2156, 0.1707, 0.2397, 0.2902, 0.3072, 0.3369, 0.3103, 0.3498, 0.4748, 0.5500, 0.4748, 0.5182, 0.6126, 0.4754, 0.6019, 0.5349, 0.6451, 0.5442, 0.4130, 0.3809, 0.3460, 0.3943, 0.4987, 0.4599, 0.3819, 0.2684, 0.3588, 0.3180, 0.2130, 0.1722, 0.1623, 0.1485, 0.1144, 0.0730, 0.0958, 0.0766, 0.0610},
        {0.0748, 0.0420, 0.0478, 0.0359, 0.0383, 0.0412, 0.0442, 0.0255, 0.0461, 0.0359, 0.0527, 0.0073, 0.0168, 0.0009, 0.0221, 0.0419, 0.0387, 0.0293, 0.0418, 0.0439, 0.0458, 0.0111, 0.0315, 0.0122, 0.0518, 0.0191, 0.0194, 0.0109, 0.0164, 0.0183, 0.0071, 0.0638, 0.0529, 0.0363, 0.0546, 0.0529, 0.0925, 0.1151, 0.0976, 0.0705, 0.1530, 0.1755, 0.1845, 0.2224, 0.2673, 0.2996, 0.3514, 0.2657, 0.2463, 0.4596, 0.3090, 0.4764, 0.5726, 0.3464, 0.4998, 0.4536, 0.5723, 0.5023, 0.5076, 0.5958, 0.4883, 0.4106, 0.4256, 0.5493, 0.3941, 0.3836, 0.3145, 0.3194, 0.2382, 0.2547, 0.2078, 0.2426, 0.1724, 0.1663, 0.1114, 0.1598, 0.0994, 0.0660, 0.0611, 0.0570},
        {0.0388, 0.0543, 0.0586, 0.0391, 0.0570, 0.0621, 0.0339, 0.0325, 0.0344, 0.0368, 0.0072, 0.0314, 0.0181, 0.0167, 0.0428, 0.0086, 0.0511, 0.0067, 0.0163, 0.0308, 0.0139, 0.0420, 0.0086, 0.0341, 0.0260, 0.0378, 0.0262, 0.0247, 0.0287, 0.0081, 0.0202, 0.0713, 0.0430, 0.0370, 0.0596, 0.0477, 0.1232, 0.1117, 0.0915, 0.1364, 0.1778, 0.1712, 0.2010, 0.2516, 0.3121, 0.3412, 0.3666, 0.3389, 0.3092, 0.2777, 0.3440, 0.5362, 0.5397, 0.4268, 0.5567, 0.4943, 0.5679, 0.3984, 0.5102, 0.6335, 0.5785, 0.5870, 0.4862, 0.4442, 0.4377, 0.3012, 0.3829, 0.4121, 0.2234, 0.3350, 0.2110, 0.2355, 0.1330, 0.1550, 0.1741, 0.0855, 0.0864, 0.0902, 0.0570, 0.0486},
        {0.0547, 0.0161, 0.0717, 0.0385, 0.0440, 0.0526, 0.0476, 0.0439, 0.0290, 0.0397, 0.0440, 0.0470, 0.0275, 0.0001, 0.0370, 0.0116, 0.0470, 0.0313, 0.0252, 0.0186, 0.0298, 0.0058, 0.0182, 0.0166, 0.0229, 0.0197, 0.0325, 0.0279, 0.0394, 0.0170, 0.0153, 0.0522, 0.0598, 0.0440, 0.1103, 0.0962, 0.1346, 0.1636, 0.1436, 0.1862, 0.1671, 0.1948, 0.2774, 0.2904, 0.2616, 0.2479, 0.3681, 0.4392, 0.3964, 0.5416, 0.4233, 0.4733, 0.4464, 0.3774, 0.5459, 0.5604, 0.5277, 0.5534, 0.5492, 0.5410, 0.4666, 0.5767, 0.3836, 0.5005, 0.3417, 0.2924, 0.3357, 0.3593, 0.3273, 0.2451, 0.2360, 0.1878, 0.1503, 0.1389, 0.1135, 0.1032, 0.0737, 0.0651, 0.0705, 0.0767},
        {0.0745, 0.0205, 0.0558, 0.0594, 0.0131, 0.0352, 0.0070, -0.0005, 0.0302, 0.0505, 0.0470, 0.0347, 0.0104, 0.0422, 0.0347, 0.0265, 0.0357, 0.0061, 0.0179, 0.0088, 0.0230, 0.0114, 0.0256, 0.0421, 0.0291, 0.0177, 0.0184, 0.0239, 0.0236, 0.0361, 0.0410, 0.0891, 0.0855, 0.0802, 0.0934, 0.1061, 0.1432, 0.1387, 0.1425, 0.2004, 0.1483, 0.2638, 0.3450, 0.2693, 0.2756, 0.4231, 0.3150, 0.5019, 0.4018, 0.5641, 0.4163, 0.4926, 0.6203, 0.4576, 0.5386, 0.4102, 0.4887, 0.5133, 0.4762, 0.5385, 0.3817, 0.4921, 0.3402, 0.3047, 0.2855, 0.3841, 0.3438, 0.2256, 0.3034, 0.2857, 0.1768, 0.1977, 0.1231, 0.1419, 0.1030, 0.0915, 0.0871, 0.0737, 0.0381, 0.0618},
        {0.0633, 0.0454, 0.0189, 0.0320, 0.0492, 0.0434, 0.0267, 0.0453, 0.0139, 0.0378, 0.0372, 0.0205, 0.0342, 0.0489, 0.0259, 0.0396, 0.0182, 0.0418, 0.0515, 0.0122, 0.0091, 0.0448, 0.0333, 0.0302, 0.0558, 0.0401, 0.0218, 0.0592, 0.0765, 0.0865, 0.0915, 0.0587, 0.0688, 0.0974, 0.1173, 0.1027, 0.1390, 0.2157, 0.1699, 0.2155, 0.2842, 0.3083, 0.2984, 0.3517, 0.4333, 0.3753, 0.4906, 0.5628, 0.4947, 0.5610, 0.5142, 0.5842, 0.6148, 0.6677, 0.4424, 0.5949, 0.4440, 0.4547, 0.5245, 0.4873, 0.5622, 0.4032, 0.2832, 0.4368, 0.3151, 0.3530, 0.2942, 0.2992, 0.2160, 0.2277, 0.1534, 0.1478, 0.1329, 0.1197, 0.0897, 0.0918, 0.1096, 0.0557, 0.0575, 0.0741},
        {0.0564, 0.0198, 0.0541, 0.0313, 0.0468, 0.0218, 0.0251, 0.0403, 0.0211, 0.0318, 0.0396, 0.0133, 0.0426, 0.0050, 0.0160, 0.0151, 0.0528, 0.0090, 0.0204, 0.0423, 0.0114, 0.0136, 0.0139, 0.0613, 0.0677, 0.0428, 0.0455, 0.0332, 0.0911, 0.0507, 0.0755, 0.1075, 0.1112, 0.1237, 0.1563, 0.2073, 0.1903, 0.2058, 0.2655, 0.2679, 0.2704, 0.2533, 0.3175, 0.3820, 0.4848, 0.3918, 0.4425, 0.5754, 0.4607, 0.6129, 0.4567, 0.4362, 0.4766, 0.6200, 0.3944, 0.5038, 0.5709, 0.5241, 0.4882, 0.5416, 0.4776, 0.3422, 0.3137, 0.2624, 0.3705, 0.3294, 0.2018, 0.2118, 0.2375, 0.1867, 0.1752, 0.1760, 0.1585, 0.1437, 0.0898, 0.1190, 0.1084, 0.0759, 0.0471, 0.0694},
        {0.0524, 0.0386, 0.0581, 0.0364, 0.0496, 0.0104, 0.0515, 0.0301, 0.0408, 0.0233, 0.0128, 0.0465, 0.0050, 0.0274, 0.0322, 0.0403, 0.0222, 0.0359, 0.0368, 0.0326, 0.0191, 0.0556, 0.0443, 0.0551, 0.0446, 0.0704, 0.0508, 0.0611, 0.0927, 0.0870, 0.1132, 0.1190, 0.1074, 0.1447, 0.2308, 0.2376, 0.2297, 0.2594, 0.3463, 0.3087, 0.3226, 0.2846, 0.3475, 0.3245, 0.4336, 0.4124, 0.3974, 0.4075, 0.4960, 0.4419, 0.6164, 0.6370, 0.5881, 0.4541, 0.5748, 0.5887, 0.4608, 0.5181, 0.3853, 0.3181, 0.4167, 0.3229, 0.3868, 0.3326, 0.2558, 0.3121, 0.1921, 0.1880, 0.2240, 0.1544, 0.1330, 0.1231, 0.1176, 0.1065, 0.0899, 0.0528, 0.0738, 0.0687, 0.0263, 0.0540},
        {0.0572, 0.0357, 0.0526, 0.0236, 0.0558, 0.0115, 0.0505, 0.0201, 0.0485, 0.0043, 0.0219, 0.0476, 0.0394, 0.0079, 0.0314, 0.0137, 0.0259, 0.0407, 0.0506, 0.0333, 0.0262, 0.0564, 0.0486, 0.0666, 0.0620, 0.0619, 0.0938, 0.1153, 0.1327, 0.1481, 0.1525, 0.1430, 0.1321, 0.2591, 0.1894, 0.2389, 0.2220, 0.3094, 0.3240, 0.3275, 0.3312, 0.3670, 0.4226, 0.5004, 0.4874, 0.3827, 0.4953, 0.4437, 0.3933, 0.4588, 0.6618, 0.6647, 0.5138, 0.4837, 0.5243, 0.5465, 0.4134, 0.4946, 0.3856, 0.3873, 0.2911, 0.2870, 0.2690, 0.2360, 0.2581, 0.2977, 0.1733, 0.1879, 0.2135, 0.1639, 0.1223, 0.1248, 0.1109, 0.0679, 0.1043, 0.0591, 0.0764, 0.0562, 0.0334, 0.0316},
        {0.0224, 0.0307, 0.0471, 0.0274, 0.0256, 0.0601, 0.0062, 0.0255, 0.0188, 0.0121, 0.0308, 0.0301, 0.0512, 0.0483, 0.0140, 0.0327, 0.0528, 0.0424, 0.0683, 0.0553, 0.0564, 0.0598, 0.0623, 0.1013, 0.1114, 0.0755, 0.1370, 0.1272, 0.1513, 0.1599, 0.1076, 0.1553, 0.1876, 0.2221, 0.2354, 0.2832, 0.3463, 0.3646, 0.4396, 0.3564, 0.4306, 0.4795, 0.3580, 0.4506, 0.4116, 0.4736, 0.5979, 0.4200, 0.6211, 0.5493, 0.4724, 0.5151, 0.4755, 0.5454, 0.3674, 0.5444, 0.4233, 0.4217, 0.3126, 0.4487, 0.3008, 0.2597, 0.2224, 0.2543, 0.1803, 0.2244, 0.1806, 0.2009, 0.1558, 0.0889, 0.1157, 0.0651, 0.1070, 0.0970, 0.0843, 0.0691, 0.0498, 0.0670, 0.0706, 0.0218},
        {0.0328, 0.0399, 0.0426, 0.0093, 0.0346, 0.0327, 0.0285, 0.0407, 0.0328, 0.0124, 0.0388, 0.0175, 0.0163, 0.0283, 0.0259, 0.0342, 0.0664, 0.0339, 0.0434, 0.0425, 0.0285, 0.0680, 0.0870, 0.1107, 0.0628, 0.1108, 0.1694, 0.1418, 0.1945, 0.2242, 0.1682, 0.2276, 0.2946, 0.2165, 0.2450, 0.3598, 0.3788, 0.4595, 0.4226, 0.4826, 0.4843, 0.4230, 0.4989, 0.4012, 0.5690, 0.5697, 0.4438, 0.4895, 0.4613, 0.4348, 0.5114, 0.5791, 0.4946, 0.5970, 0.3722, 0.4790, 0.5093, 0.3212, 0.2939, 0.3358, 0.2618, 0.3134, 0.2702, 0.2318, 0.1910, 0.1300, 0.2119, 0.1273, 0.1489, 0.1296, 0.0859, 0.0639, 0.0839, 0.0872, 0.0634, 0.0341, 0.0274, 0.0595, 0.0491, 0.0493}
    };
    const float vr[] = {
        3.0000, 2.4815, 1.9717, 1.4792, 1.0125, 0.5793, 0.1869, -0.1580, -0.4496, -0.6830, -0.8542, -0.9604, -0.9998, -0.9716, -0.8764, -0.7158, -0.4925, -0.2103, 0.1261, 0.5111, 0.9380, 1.3997, 1.8885, 2.3959, 2.9136, 3.4328, 3.9446, 4.4405, 4.9121, 5.3514, 5.7511, 6.1043, 6.4051, 6.6484, 6.8302, 6.9474, 6.9979, 6.9810, 6.8968, 6.7469, 6.5338, 6.2611, 5.9333, 5.5560, 5.1355, 4.6791, 4.1942, 3.6893, 3.1727, 2.6532
    };
    const float wd[] = {
        3.0000, 2.4815, 1.9717, 1.4792, 1.0125, 0.5793, 0.1869, -0.1580, -0.4496, -0.6830, -0.8542, -0.9604, -0.9998, -0.9716, -0.8764, -0.7158, -0.4925, -0.2103, 0.1261, 0.5111, 0.9380, 1.3997, 1.8885, 2.3959, 2.9136, 3.4328, 3.9446, 4.4405, 4.9121, 5.3514, 5.7511, 6.1043, 6.4051, 6.6484, 6.8302, 6.9474, 6.9979, 6.9810, 6.8968, 6.7469, 6.5338, 6.2611, 5.9333, 5.5560, 5.1355, 4.6791, 4.1942, 3.6893, 3.1727, 2.6532
    };
    int g;
    for (g = 0; g < sizeof(vr) / sizeof(float); g++) {
        printf("g = %d    vr[g] = %.4f    wr[g] = %.4f    spec[g] = %.4f, %.4f, ...\n", g, vr[g], wd[g], spec[g][0], spec[g][1]);
    }
}

#pragma mark - Performance Tests

void RKTestPulseCompressionSpeed(void) {
    SHOW_FUNCTION_NAME
    int p, i, j, k;
    const size_t nfft = 1 << 13;
    fftwf_complex *f, *in, *out;
    RKInt16C *X;
    RKComplex *Y;
    const int testCount = 5000;
    struct timeval tic, toc;
    double mint, t;

    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&X, RKSIMDAlignSize, nfft * sizeof(RKInt16C)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&Y, RKSIMDAlignSize, nfft * sizeof(RKComplex)));
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&f, RKSIMDAlignSize, nfft * sizeof(fftwf_complex)))
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&in, RKSIMDAlignSize, nfft * sizeof(fftwf_complex)))
    POSIX_MEMALIGN_CHECK(posix_memalign((void **)&out, RKSIMDAlignSize, nfft * sizeof(fftwf_complex)))
    if (in == NULL || out == NULL) {
        RKLog("Error. Unable to allocate resources for FFTW.\n");
        return;
    }

    fftwf_plan planForwardInPlace = fftwf_plan_dft_1d(nfft, in, in, FFTW_FORWARD, FFTW_MEASURE);
    fftwf_plan planForwardOutPlace = fftwf_plan_dft_1d(nfft, in, out, FFTW_FORWARD, FFTW_MEASURE);
    fftwf_plan planBackwardInPlace = fftwf_plan_dft_1d(nfft, out, out, FFTW_FORWARD, FFTW_MEASURE);

    // Set some real values so that we don't have see NAN in the data / results, which could slow down FFTW
    for (k = 0; k < nfft; k++) {
        X[k].i = rand();
        X[k].q = 0;
        f[k][0] = 1.0f;
        f[k][1] = 0.0f;
    }

    RKLog(UNDERLINE("PulseCompression") "\n");

    mint = INFINITY;
    for (i = 0; i < 3; i++) {
        gettimeofday(&tic, NULL);
        for (j = 0; j < testCount; j++) {
            for (p = 0; p < 2; p++) {
                // Converting complex int16_t ADC samples to complex float
                RKSIMD_Int2Complex(X, (RKComplex *)in, nfft);
                fftwf_execute_dft(planForwardInPlace, in, in);
                fftwf_execute_dft(planForwardOutPlace, f, out);
                //memcpy(out, f, nfft * sizeof(RKComplex));
                RKSIMD_iymulc((RKComplex *)in, (RKComplex *)out, nfft);
                fftwf_execute_dft(planBackwardInPlace, out, out);
                RKSIMD_iyscl((RKComplex *)out, 1.0f / nfft, nfft);
                // Copy the output
                for (k = 0; k < nfft; k++) {
                    Y[k].i = out[k][0];
                    Y[k].q = out[k][1];
                }
            }
        }
        gettimeofday(&toc, NULL);
        t = RKTimevalDiff(toc, tic);
        RKLog(">Test %d -> %.3f ms / pulse\n", i, 1.0e3 * t / testCount);
        mint = MIN(mint, t);
    }
    RKLog(">Time for each pulse (%s gates) = %.3f ms / pulse (Best of 3)\n",
          RKIntegerToCommaStyleString(nfft), 1.0e3 * mint / testCount);
    RKLog(">Speed: %.2f pulses / sec / core\n", testCount / mint);

    fftwf_destroy_plan(planForwardInPlace);
    fftwf_destroy_plan(planForwardOutPlace);
    fftwf_destroy_plan(planBackwardInPlace);

    free(X);
    free(Y);
    free(f);
    free(in);
    free(out);
}

void RKTestMomentProcessorSpeed(void) {
    SHOW_FUNCTION_NAME
    int i, j, k;
    RKFFTModule *fftModule;
    RKScratch *space;
    RKBuffer pulseBuffer;
    RKBuffer rayBuffer;
    const int testCount = 500;
    const int pulseCount = 100;
    const int pulseCapacity = 1 << 12;

    RKPulseBufferAlloc(&pulseBuffer, pulseCapacity, pulseCount);
    RKRayBufferAlloc(&rayBuffer, pulseCapacity, 1);

    RKScratchAlloc(&space, pulseCapacity, 5, (uint8_t)ceilf(log2f((float)pulseCount)), true);
    fftModule = RKFFTModuleInit(pulseCapacity, 0);
    space->fftModule = fftModule;
    space->gateCount = pulseCapacity;

    RKPulse *pulses[pulseCount];
    RKComplex *X;
    RKIQZ Y;
    for (k = 0; k < pulseCount; k++) {
        RKPulse *pulse = RKGetPulseFromBuffer(pulseBuffer, k);
        pulse->header.t = k;
        pulse->header.gateCount = pulseCapacity;
        pulse->header.downSampledGateCount = pulseCapacity;
        X = RKGetComplexDataFromPulse(pulse, 0);
        Y = RKGetSplitComplexDataFromPulse(pulse, 0);
        for (j = 0; j < pulseCapacity; j++) {
            X[j].i = (RKFloat)rand() / RAND_MAX - 0.5f;
            X[j].q = (RKFloat)rand() / RAND_MAX - 0.5f;
            Y.i[j] = X[j].i;
            Y.q[j] = X[j].q;
        }
        X = RKGetComplexDataFromPulse(pulse, 1);
        Y = RKGetSplitComplexDataFromPulse(pulse, 1);
        for (j = 0; j < pulseCapacity; j++) {
            X[j].i = (RKFloat)rand() / RAND_MAX - 0.5f;
            X[j].q = (RKFloat)rand() / RAND_MAX - 0.5f;
            Y.i[j] = X[j].i;
            Y.q[j] = X[j].q;
        }
        pulses[k] = pulse;
    }

    double t, mint;
    struct timeval tic, toc;
    int (*method)(RKScratch *, RKPulse **, const uint16_t);

    RKRay *ray = RKGetRayFromBuffer(rayBuffer, 0);

    for (j = 0; j < 6; j++) {
        switch (j) {
            default:
                method = RKPulsePair;
                RKLog(UNDERLINE("PulsePair:") "\n");
                break;
            case 1:
                method = RKPulsePairHop;
                RKLog(UNDERLINE("PulsePairHop:") "\n");
                break;
            case 2:
                method = RKMultiLag;
                space->userLagChoice = 2;
                RKLog(UNDERLINE("MultiLag (L = %d):") "\n", space->userLagChoice);
                break;
            case 3:
                method = RKMultiLag;
                space->userLagChoice = 3;
                RKLog(UNDERLINE("MultiLag (L = %d):") "\n", space->userLagChoice);
                break;
            case 4:
                method = RKMultiLag;
                space->userLagChoice = 4;
                RKLog(UNDERLINE("MultiLag (L = %d):") "\n", space->userLagChoice);
                break;
            case 5:
                method = RKSpectralMoment;
                space->fftOrder = (uint8_t)ceilf(log2f((float)pulseCount));
                RKLog(UNDERLINE("SpectralMoment:") "\n");
                break;
        }
        mint = INFINITY;
        for (i = 0; i < 3; i++) {
            gettimeofday(&tic, NULL);
            for (k = 0; k < testCount; k++) {
                method(space, pulses, pulseCount);
                makeRayFromScratch(space, ray);
            }
            gettimeofday(&toc, NULL);
            t = RKTimevalDiff(toc, tic);
            RKLog(">Test %d -> %.2f ms\n", i, 1.0e3 * t / testCount);
            mint = MIN(mint, t);
        }
        RKLog(">Time for each ray (%s pulses x %s gates) = %.2f ms (Best of 3)\n",
              RKIntegerToCommaStyleString(pulseCount), RKIntegerToCommaStyleString(pulseCapacity), 1.0e3 * mint / testCount);
        RKLog(">Speed: %.2f rays / sec / core\n", testCount / mint);
    }

    RKFFTModuleFree(fftModule);
    RKScratchFree(space);
    free(pulseBuffer);
    free(rayBuffer);
}

void RKTestCacheWrite(void) {
    RKRawDataRecorder *fileEngine = RKRawDataRecorderInit();
    fileEngine->fd = open("._testwrite", O_CREAT | O_WRONLY, 0000644);
    if (fileEngine->fd < 0) {
        RKLog("Error. Unable to open file.\n");
        exit(EXIT_FAILURE);
    }

#ifdef FUNDAMENTAL_CACHE_WRITE_TEST

    RKRawDataRecorderSetCacheSize(fileEngine, 4);
    RKRawDataRecorderCacheWrite(fileEngine, bytes, 4);
    RKRawDataRecorderCacheWrite(fileEngine, &bytes[4], 2);
    RKRawDataRecorderCacheWrite(fileEngine, &bytes[6], 1);
    RKRawDataRecorderCacheFlush(fileEngine);

#endif

    RKBuffer pulseBuffer;
    RKPulseBufferAlloc(&pulseBuffer, 8192, 100);

    struct timeval time;
    double t0, t1;

    gettimeofday(&time, NULL);
    t1 = (double)time.tv_sec + 1.0e-6 * (double)time.tv_usec;

    int j, k;
    uint32_t len = 0;
    for (k = 1, j = 1; k < 50000; k++) {
        RKPulse *pulse = RKGetPulseFromBuffer(pulseBuffer, k % 100);
        pulse->header.gateCount = 16000;

        len += RKRawDataRecorderCacheWrite(fileEngine, &pulse->header, sizeof(RKPulseHeader));
        len += RKRawDataRecorderCacheWrite(fileEngine, RKGetInt16CDataFromPulse(pulse, 0), pulse->header.gateCount * sizeof(RKInt16C));
        len += RKRawDataRecorderCacheWrite(fileEngine, RKGetInt16CDataFromPulse(pulse, 1), pulse->header.gateCount * sizeof(RKInt16C));

        if (k % 2000 == 0) {
            RKRawDataRecorderCacheFlush(fileEngine);

            gettimeofday(&time, NULL);
            t0 = (double)time.tv_sec + 1.0e-6 * (double)time.tv_usec;
            printf("Speed = %.2f MBps (%d)\n", 1.0e-6 * len / (t0 - t1), len);

            if (j++ % 5 == 0) {
                printf("\n");
                t1 = t0;
                len = 0;
            }
        }
    }

    close(fileEngine->fd);

    // Remove the files that was just created.
    j = system("rm -f ._testwrite");
    if (j) {
        RKLog("Error. System call failed.   errno = %d\n", errno);
    }

    RKRawDataRecorderFree(fileEngine);
}

#pragma mark - Transceiver Emulator

//
// WARNING: Broken
//

void *RKTestTransceiverPlaybackRunLoop(void *input) {
    RKTestTransceiver *transceiver = (RKTestTransceiver *)input;
    RKRadar *radar = transceiver->radar;
    
    int j, k, s;
    char *c, string[RKMaximumStringLength];
    long fpos, fsize;
    long tic = 0;
    struct timeval t0, t1;
    bool even = true;

    RKFileHeader *fileHeader = &transceiver->fileHeaderCache;
    RKPulseHeader *pulseHeader = &transceiver->pulseHeaderCache;
    uint32_t gateCount;
    
    // Update the engine state
    transceiver->state |= RKEngineStateWantActive;
    transceiver->state &= ~RKEngineStateActivating;

    // Retrieve parameters from file
    transceiver->periodEven = transceiver->prt;
    transceiver->periodOdd =
    transceiver->sprt == 2 ? transceiver->prt * 3.0 / 2.0 :
    (transceiver->sprt == 3 ? transceiver->prt * 4.0 / 3.0 :
     (transceiver->sprt == 4 ? transceiver->prt * 5.0 / 4.0 : transceiver->prt));

    RKLog("%s Started.   mem = %s B\n", transceiver->name, RKUIntegerToCommaStyleString(transceiver->memoryUsage));
    
    // Use a counter that mimics microsecond increments
    //RKSetPulseTicsPerSeconds(radar, 1.0e6);
    
    transceiver->state |= RKEngineStateActive;
    
    // Open the folder, build a list of files
    char *filelist = malloc(1024 * RKMaximumPathLength * sizeof(char));
    if (filelist == NULL) {
        RKLog("%s Error. Unable allocate memory.\n", transceiver->name);
        return (void *)-1;
    }
    struct dirent *dir;
    struct stat status;
    DIR *did = opendir(transceiver->playbackFolder);
    if (did == NULL) {
        if (errno != ENOENT) {
            // It is possible that the root storage folder is empty, in this case errno = ENOENT is okay.
            RKLog("%s Error opening directory %s  errno = %d\n", transceiver->name, transceiver->playbackFolder, errno);
        }
        free(filelist);
        return (void *)-2;
    }
    k = 0;
    char pathname[RKMaximumPathLength];
    while ((dir = readdir(did)) != NULL && k < 1024) {
        if (dir->d_name[0] == '.') {
            continue;
        }
        snprintf(pathname, RKMaximumPathLength, "%s/%s", transceiver->playbackFolder, dir->d_name);
        if (dir->d_type == DT_UNKNOWN) {
            // Some OS reports regular file as unknown, need to us lstat() to determine the type
            lstat(pathname, &status);
            if (!S_ISREG(status.st_mode)) {
                continue;
            }
        } else if (dir->d_type != DT_REG) {
            continue;
        }
        c = strrchr(pathname, '.');
        if (c == NULL) {
            continue;
        }
        if (strcmp(".rkr", c) && strcmp(".rkc", c)) {
            continue;
        }
        strcpy(filelist + k * RKMaximumPathLength, pathname);
        k++;
    }
    closedir(did);
    const int count = k;
    // Sort the list by name (should be in time numbers)
    qsort(filelist, count, RKMaximumPathLength, string_cmp_by_filename);
    for (k = 0; k < count; k++) {
        RKLog(">%s %d. %s\n", transceiver->name, k, filelist + k * RKMaximumPathLength);
    }
 
    // Wait until the radar has been declared live. Otherwise the pulseIndex is never advanced properly.
    s = 0;
    transceiver->state |= RKEngineStateSleep0;
    while (!(radar->state & RKRadarStateLive)) {
        usleep(10000);
        if (++s % 10 == 0 && transceiver->verbose > 1) {
            RKLog("%s sleep 0/%.1f s\n", transceiver->name, (float)s * 0.01f);
        }
    }
    transceiver->state ^= RKEngineStateSleep0;
    
    // RKAddConfig(radar, RKConfigKeyPRF, (uint32_t)roundf(1.0f / transceiver->prt), RKConfigKeyNull);
    
    gettimeofday(&t1, NULL);
    
    FILE *fid;

    k = 0;   // k file index from the filelist
    while (transceiver->state & RKEngineStateWantActive) {
        RKLog("%s Opening filelist[%d] %s\n", transceiver->name, k, RKLastPartOfPath(filelist + k * RKMaximumPathLength));
        fid = fopen(filelist + k * RKMaximumPathLength, "rb");
        if (fid == NULL) {
            RKLog("%s Error. Unable to open file %d %s\n", transceiver->name, k, filelist + k * RKMaximumPathLength);
            usleep(100000);
            continue;
        }
        if (fseek(fid, 0L, SEEK_END)) {
            RKLog("%s Error. Unable to tell the file size.\n", transceiver->name);
            fsize = 0;
        } else {
            fsize = ftell(fid);
            RKLog("%s File size = %s B\n", transceiver->name, RKUIntegerToCommaStyleString(fsize));
        }
        j = 0;
        fpos = 0;
        rewind(fid);
        fread(fileHeader, sizeof(RKFileHeader), 1, fid);
        //RKLog("%s", transceiver->name);

        RKLog("%s Waveform %s", transceiver->name, fileHeader->config.waveform);
        sprintf(string, "waveforms/%s.rkwav", fileHeader->config.waveformName);
        if (!RKFilenameExists(string)) {
            RKLog("%s Error. Waveform %s does not exist in my collection.\n", transceiver->name, string);
            fclose(fid);
            continue;
        }
        RKLog("%s Loading waveform %s ...\n", transceiver->name, string);
        
        transceiver->waveformCache[0] = RKWaveformInitFromFile(string);
        transceiver->prt = fileHeader->config.prt[0];
        RKLog("%s PRT = %.2f ms (PRF = %s Hz)\n",
              transceiver->name,
              1.0e3 * transceiver->prt,
              RKIntegerToCommaStyleString(round(1.0 / transceiver->prt)));
        
        if (transceiver->prt <= 1.0e-4 || transceiver->prt > 0.1) {
            transceiver->prt = CLAMP(transceiver->prt, 1.0e-4, 0.1);
            RKLog("%s Info. Overriding PRT --> %.2f ms (PRF = %s Hz)\n",
                  transceiver->name,
                  1.0e3 * transceiver->prt,
                  RKIntegerToCommaStyleString(round(1.0 / transceiver->prt)));
        }
        
        RKAddConfig(radar,
                    RKConfigKeySystemNoise, fileHeader->config.noise[0], fileHeader->config.noise[1],
                    RKConfigKeySystemZCal, fileHeader->config.systemZCal[0], fileHeader->config.systemZCal[1],
                    RKConfigKeySystemDCal, fileHeader->config.systemDCal,
                    RKConfigKeySystemPCal, fileHeader->config.systemPCal,
                    RKConfigKeySNRThreshold, fileHeader->config.SNRThreshold,
                    RKConfigKeySQIThreshold, fileHeader->config.SQIThreshold,
                    RKConfigKeyTransitionGateCount, fileHeader->config.transitionGateCount,
                    RKConfigKeyRingFilterGateCount, fileHeader->config.ringFilterGateCount,
                    RKConfigKeyWaveform, transceiver->waveformCache[0],
                    RKConfigKeyPRF, 1.0 / fileHeader->config.prt[0],
                    RKConfigKeySweepElevation, fileHeader->config.sweepElevation,
                    RKConfigKeySweepAzimuth, fileHeader->config.sweepAzimuth,
                    RKConfigKeyNull);
        
        transceiver->periodOdd = transceiver->periodEven = transceiver->prt;
        transceiver->chunkSize = (transceiver->periodOdd + transceiver->periodEven) >= 0.02 ? 1 : MAX(1, (int)round(0.05 / transceiver->prt));
        RKLog("%s Using chunkSize = %d\n", transceiver->name, transceiver->chunkSize);

        while (transceiver->state & RKEngineStateWantActive && fpos < fsize) {
            RKPulse *pulse = RKGetVacantPulse(radar);
            if (pulse == NULL) {
                RKLog("%s Error. No vacant pulse for storage.\n", transceiver->name);
                break;
            }
            fread(pulseHeader, sizeof(RKPulseHeader), 1, fid);
            gateCount = MIN(pulse->header.capacity, pulseHeader->gateCount);
            fread(RKGetInt16CDataFromPulse(pulse, 0), sizeof(RKByte), gateCount * sizeof(RKInt16C), fid);
            fseek(fid, (pulseHeader->gateCount - gateCount) * sizeof(RKInt16C), SEEK_CUR);
            fread(RKGetInt16CDataFromPulse(pulse, 1), sizeof(RKByte), gateCount * sizeof(RKInt16C), fid);
            fseek(fid, (pulseHeader->gateCount - gateCount) * sizeof(RKInt16C), SEEK_CUR);
            
            pulse->header.gateSizeMeters = pulseHeader->gateSizeMeters;
            pulse->header.gateCount = gateCount;
            pulse->header.rawAzimuth = pulseHeader->rawAzimuth;
            pulse->header.rawElevation = pulseHeader->rawElevation;
            pulse->header.azimuthDegrees = pulseHeader->azimuthDegrees;
            pulse->header.elevationDegrees = pulseHeader->elevationDegrees;
            pulse->header.azimuthVelocityDegreesPerSecond = pulseHeader->azimuthVelocityDegreesPerSecond;
            pulse->header.elevationVelocityDegreesPerSecond = pulseHeader->elevationVelocityDegreesPerSecond;
            
            //        RKConfigAdvanceEllipsis(engine->configBuffer, engine->configIndex, engine->radarDescription->configBufferDepth,
            //                                RKConfigKeySweepElevation, (double)positionAfter->sweepElevationDegrees,
            //                                RKConfigKeySweepAzimuth, (double)positionAfter->sweepAzimuthDegrees,
            //                                RKConfigKeyPulseGateSize, pulse->header.gateSizeMeters,
            //                                RKConfigKeyPulseGateCount, pulse->header.gateCount,
            //                                RKConfigKeyPositionMarker, marker0,
            
            if (pulseHeader->marker & RKMarkerSweepBegin) {
                RKLog("%s Sweep begin.\n", transceiver->name);
                RKAddConfig(radar,
                            RKConfigKeyPulseGateCount, pulseHeader->gateCount,
                            RKConfigKeyPulseGateSize, pulseHeader->gateSizeMeters,
                            RKConfigKeyNull);
            }
            
            pulse->header.configIndex = radar->configIndex;

            RKSetPulseReady(radar, pulse);
            
//            RKLog("%s pulse->header.s = %04x  marker = %04x gateCount = %d\n", transceiver->name, pulseHeader->marker, pulseHeader->s, gateCount);
            
            fpos = ftell(fid);
            even = !even;
            tic++;
            
            if (j++ == transceiver->chunkSize) {
                gettimeofday(&t0, NULL);
                j = (int)(1000000.0 * RKTimevalDiff(t0, t1)) / transceiver->chunkSize;
//                j = 10000;
                usleep(j);
                t1 = t0;
                j = 0;
            }
        }
        
        fpos = ftell(fid);
        RKLog("%s Pulse count = %s   fpos = %s   fsize = %s\n", transceiver->name,
              RKUIntegerToCommaStyleString(j),
              RKUIntegerToCommaStyleString(fpos),
              RKUIntegerToCommaStyleString(fsize));
        fclose(fid);
        
        //periodTotal = 0.0;
        
        //k = RKNextModuloS(k, count);
        k++;
        if (k == count) {
            break;
        }
    }

    transceiver->state ^= RKEngineStateActive;
    
    free(filelist);
    
    return NULL;
}

//
// The transceiver emulator only has minimal functionality to get things going
// This isn't meant to fully simulate actual target returns
//
void *RKTestTransceiverRunLoop(void *input) {
    RKTestTransceiver *transceiver = (RKTestTransceiver *)input;
    RKRadar *radar = transceiver->radar;

    int j, k, p, g, s, w;
    double t = 0.0;
    double dt = 0.0;
    long tic = 0;
    struct timeval t0, t1, t2;
    bool even = true;
    
    // Update the engine state
    transceiver->state |= RKEngineStateWantActive;
    transceiver->state &= ~RKEngineStateActivating;

    // Derive some calculated parameters
    transceiver->periodEven = transceiver->prt;
    transceiver->periodOdd =
    transceiver->sprt == 2 ? transceiver->prt * 3.0 / 2.0 :
    (transceiver->sprt == 3 ? transceiver->prt * 4.0 / 3.0 :
     (transceiver->sprt == 4 ? transceiver->prt * 5.0 / 4.0 : transceiver->prt));
    transceiver->ticEven = (long)(transceiver->periodEven * 1.0e6);
    transceiver->ticOdd = (long)(transceiver->periodOdd * 1.0e6);
    transceiver->chunkSize = (transceiver->periodOdd + transceiver->periodEven) >= 0.02 ? 1 : MAX(1, (int)floor(0.05 / transceiver->prt));
    transceiver->gateSizeMeters = 1.5e8f / transceiver->fs;
    transceiver->gateCount = MIN(transceiver->gateCapacity, (1.5e8 * transceiver->prt) / transceiver->gateSizeMeters);
    
    // Show some info
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("%s fs = %s MHz (%.2f m)   %sPRF = %s Hz   (PRT = %.3f ms, %s)\n",
              transceiver->name,
              RKFloatToCommaStyleString(1.0e-6 * transceiver->fs),
              transceiver->gateSizeMeters,
              transceiver->sprt > 1 ? "Base " : "",
              RKIntegerToCommaStyleString((long)(1.0 / transceiver->prt)),
              1000.0 * transceiver->prt,
              transceiver->sprt == 1 ? "Normal" :
              (transceiver->sprt == 2 ? "2:3 Staggered" :
               (transceiver->sprt == 3 ? "3:4 Staggered" :
                (transceiver->sprt == 4 ? "4:5 Staggered" : "Normal"))));
        RKLog("%s gateCount = %s   R = %.1f km   chunkSize = %s   tics = %s\n",
              transceiver->name,
              RKIntegerToCommaStyleString(transceiver->gateCount),
              transceiver->gateCount * transceiver->gateSizeMeters * 1.0e-3,
              RKIntegerToCommaStyleString(transceiver->chunkSize),
              RKFloatToCommaStyleString(1.0e6 * transceiver->prt));
    }
    
    double periodTotal;

    float a;
    float r;
    float phi;
    float cosv;
    float sinv;
    int16_t noise;

    float *ra = (float *)malloc(transceiver->gateCount * sizeof(float));
    int16_t *rn = (int16_t *)malloc(transceiver->gateCount * sizeof(int16_t));
    if (ra == NULL || rn == NULL) {
        RKLog("Error. Unable to allocate memory in RKTransceiverRunLoop.\n");
        exit(EXIT_FAILURE);
    }
    memset(ra, 0, transceiver->gateCount * sizeof(float));
    memset(rn, 0, transceiver->gateCount * sizeof(int16_t));
    for (g = 0; g < transceiver->gateCount; g++) {
        r = (float)g * transceiver->gateSizeMeters * 0.1f;
        a = 600.0f * (cos(0.001f * r)
                     + 0.8f * cosf(0.003f * r + 0.8f) * cosf(0.003f * r + 0.8f) * cosf(0.003f * r + 0.8f)
                     + 0.3f * cosf(0.007f * r) * cosf(0.007f * r)
                     + 0.2f * cosf(0.01f * r + 0.3f)
                     + 0.5f);
        a *= (1000.0 / r) * fabsf(1.0f - (float)g / 50000.0f);
        ra[g] = a;
        rn[g] = (int16_t)(5.0f * ((float)rand() / RAND_MAX - 0.5f));
    }

    float dphi = transceiver->gateSizeMeters * 0.1531995963856f;
    while (dphi >= M_PI) {
        dphi -= 2.0 * M_PI;
    }
    while (dphi < -M_PI) {
        dphi += 2.0 * M_PI;
    }
    if (dphi < -M_PI || dphi >= M_PI) {
        RKLog("Error. Value of dphi = %.4f out of range!\n", dphi);
    }

    RKWaveform *waveform = transceiver->waveformCache[0];
    unsigned int waveformCacheIndex = 0;

#if defined(DEBUG_IIR)

    int16_t seq[] = {-3, -2 , -1, 0, 1, 2, 3, 2, 1, 0, -2};
    int len = sizeof(seq) / sizeof(int16_t);
    int ir = 0;
    int iq = 3;

#endif

    int nn;
    float temp;
    float volt;
    float room;
    RKHealth *health;
    
    RKLog("%s Started.   mem = %s B\n", transceiver->name, RKUIntegerToCommaStyleString(transceiver->memoryUsage));

    // Use a counter that mimics microsecond increments
    RKSetPulseTicsPerSeconds(radar, 1.0e6);
    
    transceiver->state |= RKEngineStateActive;

    // Wait until the radar has been declared live. Otherwise the pulseIndex is never advanced properly.
    s = 0;
    transceiver->state |= RKEngineStateSleep0;
    while (!(radar->state & RKRadarStateLive)) {
        usleep(10000);
        if (++s % 10 == 0 && transceiver->verbose > 1) {
            RKLog("%s sleep 0/%.1f s\n", transceiver->name, (float)s * 0.01f);
        }
    }
    transceiver->state ^= RKEngineStateSleep0;

//    RKAddConfig(radar,
//                RKConfigKeyWaveform, transceiver->waveformCache[0],
//                RKConfigKeyPRF, 1.0 / transceiver->prt,
//                RKConfigKeyNull);

    gettimeofday(&t1, NULL);
    gettimeofday(&t2, NULL);

    // g gate index
    // j sample index
    // k pseudo-random sequence to choose the pre-defined random numbers
    w = 0;   // waveform index
    while (transceiver->state & RKEngineStateWantActive) {

        periodTotal = 0.0;

        for (j = 0; j < transceiver->chunkSize && transceiver->state & RKEngineStateWantActive; j++) {
            RKPulse *pulse = RKGetVacantPulse(radar);
            
            // Fill in the header
            pulse->header.t = (uint64_t)(1.0e6 * t);
            pulse->header.gateCount = transceiver->gateCount;
            pulse->header.gateSizeMeters = transceiver->gateSizeMeters;

            // Fill in the data...
            if (waveformCacheIndex != transceiver->waveformCacheIndex) {
                waveformCacheIndex = transceiver->waveformCacheIndex;
                waveform = transceiver->waveformCache[transceiver->waveformCacheIndex];
                w = 0;
            }

            // Go through both polarizations
            for (p = 0; p < 2; p++) {
                RKInt16C *X = RKGetInt16CDataFromPulse(pulse, p);
                RKInt16C *S = waveform->iSamples[w];
                // Some random pattern for testing
                k = rand() % transceiver->gateCount;
                // Populate the pulse
                for (g = 1; g < waveform->depth; g++) {
                    noise = rn[k];
                    X->i = S->i + noise;
                    X->q = S->q + noise;
                    k = RKNextModuloS(k, transceiver->gateCount);
                    X++;
                    S++;
                }
                // Phase as a function of time (tic) wrapped into [-PI, PI]
                phi = fmod((double)(tic & 0xFFFF) / 655.36 * M_PI + M_PI, 2.0 * M_PI) - M_PI;
                //for (; g < transceiver->gateCount; g++) {
                for (; g < MIN(50000, transceiver->gateCount); g++) {
                    // sinf() and cosf() run faster with angle within 0 and 2 PI
                    phi += dphi;
                    if (phi < -3.14159265f) {
                        phi += 6.28318531f;
                    } else if (phi > 3.14159265f) {
                        phi -= 6.28318531f;
                    }
                    noise = rn[k];

                    //
                    // The following are using faster approximation of sin() and cos().
                    // They are less precise than the native functions but way faster.
                    // For a more precise version, use RKFastSineCosine()
                    //
                    // X->i = (int16_t)(ra[g] * cosf(phi) + noise);
                    // X->q = (int16_t)(ra[g] * sinf(phi) + noise);
                    //
                    RKFasterSineCosine(phi, &sinv, &cosv);
                    X->i = (int16_t)(ra[g] * cosv + noise);
                    X->q = (int16_t)(ra[g] * sinv + noise);

                    k = RKNextModuloS(k, transceiver->gateCount);
                    X++;
                }
                for (; g < transceiver->gateCount; g++) {
                    noise = rn[k];
                    X->i = noise;
                    X->q = noise;
                    k = RKNextModuloS(k, transceiver->gateCount);
                    X++;
                }

#if defined(DEBUG_IIR)

                X = RKGetInt16CDataFromPulse(pulse, p);

                // Override certain range gates
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];

                X = RKGetInt16CDataFromPulse(pulse, p);
                X += 128;

                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];
                X += 2;
                X->i = seq[ir];
                X->q = seq[iq];

#endif

            }

#if defined(DEBUG_IIR)

            ir = RKNextModuloS(ir, len);
            iq = RKNextModuloS(iq, len);

#endif

            RKSetPulseHasData(radar, pulse);
//            RKLog("%s pulse->header.i = %d   C%d  %s x %.1fm\n",
//                  transceiver->name,
//                  pulse->header.i, pulse->header.configIndex,
//                  RKIntegerToCommaStyleString(pulse->header.gateCount),
//                  RKIntegerToCommaStyleString(pulse->header.downSampledGateCount),
//                  pulse->header.gateSizeMeters * transceiver->radar->desc.pulseToRayRatio);

            if (even) {
                tic += transceiver->ticEven;
            } else {
                tic += transceiver->ticOdd;
            }

            w = RKNextModuloS(w, waveform->count);

            if (transceiver->sleepInterval > 0 && tic > 0 && tic % transceiver->sleepInterval == 0) {
                RKLog("%s sleeping at counter = %s / %s ... %.2e %.2e / %.2e %.2e\n",
                      transceiver->name,
                      RKIntegerToCommaStyleString((long)(transceiver->prt * transceiver->counter)),
                      RKIntegerToCommaStyleString(transceiver->sleepInterval),
                      radar->pulseClock->a, radar->pulseClock->b, radar->positionClock->a, radar->positionClock->b);
                transceiver->counter = 0;
                even = true;
                tic = 0;
                t += 2.0;
                sleep(2);
            }
            if (even) {
                periodTotal += transceiver->periodEven;
                t += transceiver->periodEven;
            } else {
                periodTotal += transceiver->periodOdd;
                t += transceiver->periodOdd;
            }
            even = !even;
        }

        // Report health
        gettimeofday(&t0, NULL);
        dt = RKTimevalDiff(t0, t2);
        if (dt > 0.1) {
            t2 = t0;
            nn = rand();
            temp = 1.0f * nn / RAND_MAX + 79.5f;
            volt = 1.0f * nn / RAND_MAX + 11.5f;
            room = 1.0f * nn / RAND_MAX + 21.5f + (transceiver->simFault && transceiver->transmitting ? 10.0f : 0.0f);
            health = RKGetVacantHealth(radar, RKHealthNodeTransceiver);
            if (health) {
                sprintf(health->string,
                        "{\"Trigger\":{\"Value\":true,\"Enum\":%d}, "
                        "\"PLL Clock\":{\"Value\":true,\"Enum\":%d}, "
                        "\"Target PRF\":{\"Value\":\"%s Hz\", \"Enum\":0}, "
                        "\"FPGA Temp\":{\"Value\":\"%.1fdegC\",\"Enum\":%d}, "
                        "\"XMC Voltage\":{\"Value\":\"%.1f V\",\"Enum\":%d}, "
                        "\"Ambient Temp\":{\"Value\":\"%.1fdegC\",\"Enum\":%d}, "
                        "\"Transmit H\":{\"Value\":\"%s dBm\", \"Enum\":%d}, "
                        "\"Transmit V\":{\"Value\":\"%s dBm\", \"Enum\":%d}, "
                        "\"Waveform\":{\"Value\":\"%s\", \"Enum\":0}, "
                        "\"TransceiverCounter\": %ld}",
                        RKStatusEnumActive,
                        RKStatusEnumNormal,
                        RKIntegerToCommaStyleString((long)(1.0 / transceiver->prt)),
                        temp, RKStatusFromTemperatureForCE(temp),
                        volt, volt > 12.2f ? RKStatusEnumHigh : RKStatusEnumNormal,
                        room, RKStatusFromTemperatureForComputers(room),
                        transceiver->transmitting ? RKFloatToCommaStyleString((float)50.0f + 0.001f * ((nn + 111) & 0x3ff)) : "-inf",
                        transceiver->transmitting ? RKStatusEnumActive : RKStatusEnumOff,
                        transceiver->transmitting ? RKFloatToCommaStyleString((float)50.0f + 0.001f * ((nn + 222) & 0x3ff)) : "-inf",
                        transceiver->transmitting ? RKStatusEnumActive : RKStatusEnumOff,
                        transceiver->waveformCache[transceiver->waveformCacheIndex]->name,
                        transceiver->counter);
                RKSetHealthReady(radar, health);
            }
        }

        // Wait to simulate the PRF
        s = 0;
        do {
            gettimeofday(&t0, NULL);
            dt = RKTimevalDiff(t0, t1);
            usleep(1000);
            if (++s % 1000 == 0) {
                printf("Sleeping ... n = %d ...\n", s);
            }
        } while (transceiver->state & RKEngineStateWantActive && dt < periodTotal);
        t1 = t0;
    }

    free(ra);
    free(rn);

    transceiver->state ^= RKEngineStateActive;
    return NULL;
}

RKTransceiver RKTestTransceiverInit(RKRadar *radar, void *input) {

    int i, j, k;
    RKTestTransceiver *transceiver = (RKTestTransceiver *)malloc(sizeof(RKTestTransceiver));
    if (transceiver == NULL) {
        RKLog("Error. Unable to allocate a test transceiver.\n");
        exit(EXIT_FAILURE);
    }
    memset(transceiver, 0, sizeof(RKTestTransceiver));
    sprintf(transceiver->name, "%s<TransceiverCast>%s",
            rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorTransceiver) : "",
            rkGlobalParameters.showColor ? RKNoColor : "");
    transceiver->state = RKEngineStateAllocated;
    transceiver->radar = radar;
    transceiver->memoryUsage = sizeof(RKTestTransceiver);
    transceiver->gateCapacity = RKGetPulseCapacity(radar);
    transceiver->gateCount = transceiver->gateCapacity;
    transceiver->fs = transceiver->gateCount >= 16000 ? 50.0e6 :
                     (transceiver->gateCount >= 8000 ? 25.0e6 :
                     (transceiver->gateCount >= 4000 ? 10.0e6 : 5.0e6));
    transceiver->prt = 0.001;
    transceiver->sprt = 1;
//    transceiver->waveformCache[0] = RKWaveformInitAsFrequencyHops(transceiver->fs, 0.0, 1.0e-6, 20.0e6, 5);
    transceiver->waveformCache[0] = RKWaveformInitAsSingleTone(transceiver->fs, 0.0, 1.0e-6);
//    RKWaveformSummary(transceiver->waveformCache[0]);

    // Parse out input parameters
    if (input) {
        char *sb = (char *)input, *se = NULL, *sv = NULL;
        while (*sb == ' ') {
            sb++;
        }
        if (radar->desc.initFlags & RKInitFlagVeryVeryVerbose) {
            RKLog("%s Parsing input.\n", transceiver->name);
        }
        while ((se = strchr(sb, ' ')) != NULL) {
            sv = se + 1;
            switch (*sb) {
                case 'D':
                    // Playback from a folder with rkr files
                    strcpy(transceiver->playbackFolder, sv);
                    k = (int)strlen(transceiver->playbackFolder);
                    if (transceiver->playbackFolder[k - 1] == '/') {
                        transceiver->playbackFolder[k - 1] = '\0';
                    }
                    RKLog("%s Playback from folder %s\n", transceiver->name, transceiver->playbackFolder);
                    break;
                case 'F':
                    transceiver->fs = atof(sv);
                    if (radar->desc.initFlags & RKInitFlagVeryVeryVerbose) {
                        RKLog(">%s fs = %s Hz", transceiver->name, RKIntegerToCommaStyleString((long)transceiver->fs));
                    }
                    break;
                case 'f':
                    i = sscanf(sv, "%d,%d", &j, &k);
                    transceiver->prt = 1.0 / (double)j;
                    if (i == 2) {
                        transceiver->sprt = k;
                    } else {
                        transceiver->sprt = 1;
                    }
                    if (radar->desc.initFlags & RKInitFlagVeryVeryVerbose) {
                        RKLog("%s PRF = %s Hz   (PRT = %.3f ms, %s)\n",
                              transceiver->name,
                              RKIntegerToCommaStyleString((long)j),
                              1000.0 * transceiver->prt,
                              transceiver->sprt == 1 ? "Normal" :
                              (transceiver->sprt == 2 ? "2:3 Staggered" :
                               (transceiver->sprt == 3 ? "3:4 Staggered" :
                                (transceiver->sprt == 4 ? "4:5 Staggered" : "Normal"))));
                    }
                    break;
                case 'g':
                    transceiver->gateCount = atoi(sv);
                    uint32_t capacity = RKGetPulseCapacity(radar);
                    if (transceiver->gateCount > capacity) {
                        RKLog("%s Warning. gateCount %s will be clamped to the capacity %s\n",
                              transceiver->name, RKIntegerToCommaStyleString(transceiver->gateCount), RKIntegerToCommaStyleString(capacity));
                        transceiver->gateCount = capacity;
                    }
                    if (radar->desc.initFlags & RKInitFlagVeryVeryVerbose) {
                        RKLog(">%s gateCount = %s\n", transceiver->name, RKIntegerToCommaStyleString(transceiver->gateCount));
                    }
                    break;
                case 'z':
                    transceiver->sleepInterval = atoi(sv);
                    if (radar->desc.initFlags & RKInitFlagVeryVeryVerbose) {
                        RKLog(">%s sleepInterval = %s", transceiver->name, RKIntegerToCommaStyleString(transceiver->sleepInterval));
                    }
                    break;
            }
            sb = strchr(sv, ' ');
            if (sb == NULL) {
                break;
            } else {
                while (*sb == ' ') {
                    sb++;
                }
            }
        }
    }
    
    transceiver->state |= RKEngineStateActivating;
    if (strlen(transceiver->playbackFolder)) {
        RKLog("%s Launching playback run loop ...\n", transceiver->name);
        if (pthread_create(&transceiver->tidRunLoop, NULL, RKTestTransceiverPlaybackRunLoop, transceiver)) {
            RKLog("%s. Unable to create transceiver playback run loop.\n", transceiver->name);
        }
    } else {
        if (pthread_create(&transceiver->tidRunLoop, NULL, RKTestTransceiverRunLoop, transceiver)) {
            RKLog("%s. Unable to create transceiver run loop.\n", transceiver->name);
        }
    }
    while (!(transceiver->state & RKEngineStateActive)) {
        usleep(10000);
    }

    return (RKTransceiver)transceiver;
}

int RKTestTransceiverExec(RKTransceiver transceiverReference, const char *command, char *response) {
    RKTestTransceiver *transceiver = (RKTestTransceiver *)transceiverReference;
    RKRadar *radar = transceiver->radar;

    int j, k;
    char *c;
    double bandwidth;
    double pulsewidth;
    double value;

    RKWaveform *waveform = NULL;
    char string[RKMaximumPathLength];

    if (!(radar->state & RKRadarStatePulseCompressionEngineInitialized)) {
        if (transceiver->verbose) {
            RKLog("%s Warning. No I/Q processors for '%s'.", transceiver->name, command);
        }
        if (response != NULL) {
            sprintf(response, "NAK. No I/Q processors yet." RKEOL);
        }
        return RKResultFailedToExecuteCommand;
    }
    
    if (transceiver->verbose) {
        RKLog("%s %s", transceiver->name, RKVariableInString("command", command, RKValueTypeString));
    }
    
    switch (command[0]) {
        case 'd':
            if (!strcmp(command, "disconnect")) {
                if (transceiver->state & RKEngineStateWantActive) {
                    transceiver->state ^= RKEngineStateWantActive;
                    if (radar->desc.initFlags & RKInitFlagVerbose) {
                        RKLog("%s Disconnecting ...", transceiver->name);
                    }
                } else {
                    if (radar->desc.initFlags & RKInitFlagVerbose) {
                        RKLog("%s Deactivate multiple times\n", transceiver->name);
                    }
                    return RKResultEngineDeactivatedMultipleTimes;
                }
                transceiver->state |= RKEngineStateDeactivating;
                pthread_join(transceiver->tidRunLoop, NULL);
                transceiver->state ^= RKEngineStateDeactivating;
                if (response != NULL) {
                    sprintf(response, "ACK. Transceiver stopped." RKEOL);
                }
                if (radar->desc.initFlags & RKInitFlagVerbose) {
                    RKLog("%s Stopped.\n", transceiver->name);
                }
            }
            break;
        case 'h':
            if (response != NULL) {
                sprintf(response,
                        "Commands:\n"
                        UNDERLINE("help") " - Help list.\n"
                        UNDERLINE("prt") " [value] - PRT set to value\n"
                        UNDERLINE("z") " [value] - Sleep interval set to value.\n"
                        );
            }
            break;
        case 'p':
            if (!strncmp(command, "prt", 3)) {
                k = sscanf(command, "%s %lf", string, &value);
                if (k == 2) {
                    transceiver->prt = value;
                    if (response != NULL) {
                        sprintf(response, "ACK. New PRT = %.3f ms" RKEOL, 1.0e3 * transceiver->prt);
                    }
                } else if (response != NULL) {
                    sprintf(response, "ACK. Current PRT = %.3f ms" RKEOL, 1.0e3 * transceiver->prt);
                    break;
                }
            } else if (!strncmp(command, "prf", 3)) {
                k = sscanf(command, "%s %lf", string, &value);
                if (k == 2) {
                    transceiver->prt = 1.0 / value;
                    if (response != NULL) {
                        sprintf(response, "ACK. New PRF = %.0f Hz" RKEOL, 1.0 / transceiver->prt);
                    }
                } else if (response != NULL) {
                    sprintf(response, "ACK. Current PRF = %.0f Hz" RKEOL, 1.0 / transceiver->prt);
                    break;
                }
            }
            transceiver->periodEven = transceiver->prt;
            transceiver->periodOdd =
            transceiver->sprt == 2 ? transceiver->prt * 3.0 / 2.0 :
            (transceiver->sprt == 3 ? transceiver->prt * 4.0 / 3.0 :
             (transceiver->sprt == 4 ? transceiver->prt * 5.0 / 4.0 : transceiver->prt));
            transceiver->ticEven = (long)(transceiver->periodEven * 1.0e6);
            transceiver->ticOdd = (long)(transceiver->periodOdd * 1.0e6);
            transceiver->chunkSize = (transceiver->periodOdd + transceiver->periodEven) >= 0.02 ? 1 : MAX(1, (int)floor(0.1 / transceiver->prt));
            value = 1.5e8 * transceiver->prt;
            transceiver->gateCount = MIN(transceiver->gateCapacity, value / transceiver->gateSizeMeters);
            RKAddConfig(radar, RKConfigKeyPRF, 1.0f / transceiver->prt, RKConfigKeyNull);
            if (radar->desc.initFlags & RKInitFlagVerbose) {
                RKLog("%s PRT = %s ms   gateCount = %s\n", transceiver->name,
                      RKFloatToCommaStyleString(1.0e3 * transceiver->prt),
                      RKIntegerToCommaStyleString(transceiver->gateCount));
            }
            break;
        case 's':
            if (!strcmp(command, "stop")) {
                RKLog("%s Stop transmitting.\n", transceiver->name);
                if (response != NULL) {
                    sprintf(response, "ACK. Transmitter Off." RKEOL);
                }
                break;
            }
            transceiver->sleepInterval = atoi(command + 1);
            RKLog("%s sleepInterval = %s", transceiver->name, RKIntegerToCommaStyleString(transceiver->sleepInterval));
            break;
        case 't':
            // Pretend a slow command
            RKPerformMasterTaskInBackground(radar, "w");
            if (response != NULL) {
                sprintf(response, "ACK. Command executed." RKEOL);
            }
            break;
        case 'w':
            // Waveform
            c = ((char *)command);
            if (command[1] == ' ') {
                c += 2;
            } else {
                c++;
            }
            if (*c == 's' && c[1] >= '0' && c[1] <= '9' && c[2] >= '0' && c[2] <= '9') {
                // Something like s01, s02, etc.
                pulsewidth = 1.0e-6 * atof(c + 1);
                RKLog("%s Waveform '%s'   pw = %.2f us\n", transceiver->name, c, 1.0e6 * pulsewidth);
                waveform = RKWaveformInitAsSingleTone(transceiver->fs, 0.0, pulsewidth);
            } else if ((*c == 't' || *c == 'q') && c[1] >= '0' && c[1] <= '9' && c[2] >= '0' && c[2] <= '9' && c[3] >= '0' && c[3] <= '9' && c[4] >= '0' && c[4] <= '9') {
                // Something like t0101, t0102, q0201, q0205, etc.
                string[0] = c[1]; string[1] = c[2]; string[2] = '\0';
                bandwidth = 1.0e6 * atof(string);
                pulsewidth = 1.0e-6 * atof(c + 3);
                RKLog("%s Waveform '%s'   pw = %.2f us\n", transceiver->name, c, 1.0e6 * pulsewidth);
                if (*c == 't') {
                    // Rectangular single tone at (bandwidth) MHz
                    waveform = RKWaveformInitAsSingleTone(transceiver->fs, bandwidth, pulsewidth);
                } else if (*c == 'q') {
                    // LFM at half of the bandwidth capacity
                    waveform = RKWaveformInitAsLinearFrequencyModulation(transceiver->fs, -0.5 * bandwidth, pulsewidth, bandwidth);
                    if (bandwidth > transceiver->fs) {
                        RKLog("%s Warning. Aliasing.   bandwidth = %.2f MHz > fs = %.2f MHz\n", transceiver->name, 1.0e-6 * bandwidth, 1.0e-6 * transceiver->fs);
                    }
                }
            } else if (*c == 'h' && c[1] >= '0' && c[1] <= '9' && c[2] >= '0' && c[2] <= '9' && c[3] >= '0' && c[3] <= '9' && c[4] >= '0' && c[4] <= '9') {
                // Something like h1005, h2007, etc.
                string[0] = c[1]; string[1] = c[2]; string[2] = '\0';
                bandwidth = 1.0e6 * atof(string);
                string[0] = c[3]; string[1] = c[4];
                k = atoi(string);
                if (strlen(c) > 5) {
                    pulsewidth = 1.0e-6 * atof(c + 5);
                } else {
                    pulsewidth = 1.0e-6;
                }
                RKLog("%s Waveform '%s'   hops over bw = %.2f MHz @ %d pairs   pw = %.2f us\n", transceiver->name, c, 1.0e-6 * bandwidth, k, 1.0e6 * pulsewidth);
                if (bandwidth > transceiver->fs) {
                    RKLog("%s Warning. Aliasing.   bandwidth = %.2f MHz > fs = %.2f MHz\n", transceiver->name, 1.0e-6 * bandwidth, 1.0e-6 * transceiver->fs);
                }
                // Frequency hop at the specified pulsewidth, bandwith and hop count
                waveform = RKWaveformInitAsFrequencyHops(transceiver->fs, 0.0, pulsewidth, bandwidth, k);
            } else if (*c == 'x') {
                // Experimental waveform
                waveform = RKWaveformInitAsTimeFrequencyMultiplexing(transceiver->fs, 0.0, 4.0e6, 50.0e-6);
            } else {
                // Load from a file
                sprintf(string, "%s/%s.rkwav", RKWaveformFolder, c);
                if (RKFilenameExists(string)) {
                    RKLog("Loading waveform from file '%s'...\n", string);
                    waveform = RKWaveformInitFromFile(string);
                    if (waveform == NULL) {
                        return RKResultFailedToSetWaveform;
                    }
                    pulsewidth = waveform->depth / waveform->fs;
                    RKWaveformSummary(waveform);
                    k = round(waveform->fs / transceiver->fs);
                    if (k > 1) {
                        RKLog("%s Adjusting waveform to RX sampling rate = %.2f MHz (x %d)    pw = %.2f us\n", transceiver->name, 1.0e-6 * transceiver->fs, k, 1.0e6 * pulsewidth);
                        RKWaveformDownConvert(waveform);
                        RKWaveformDecimate(waveform, k);
                    }
                } else if (response != NULL) {
                    sprintf(response, "NAK. Waveform '%s' not found." RKEOL, string);
                    return RKResultFailedToSetWaveform;
                }
            }
            if (waveform == NULL) {
                RKLog("%s No waveform.\n", transceiver->name);
                return RKResultFailedToSetWaveform;
            }
            if (radar->desc.pulseCapacity < waveform->depth) {
                RKLog("%s Warning. Waveform '%s' with %s samples not allowed (capacity = %s).\n", transceiver->name, string,
                      RKIntegerToCommaStyleString(waveform->depth), RKIntegerToCommaStyleString(radar->desc.pulseCapacity));
                RKLog("%s Warning. Waveform not changed.\n", transceiver->name);
                if (response != NULL) {
                    sprintf(response, "NAK. Waveform '%s' with %s samples not allowed (capacity = %s)." RKEOL, string,
                            RKIntegerToCommaStyleString(waveform->depth), RKIntegerToCommaStyleString(radar->desc.pulseCapacity));
                }
                return RKResultFailedToSetWaveform;
            }
            // Next cache index
            j = RKNextModuloS(transceiver->waveformCacheIndex, RKTestWaveformCacheCount);
            if (transceiver->waveformCache[j]) {
                RKLog("%s Freeing cache %d ...\n", transceiver->name, j);
                RKWaveformFree(transceiver->waveformCache[j]);
            }
            transceiver->waveformCache[j] = waveform;
            transceiver->waveformCacheIndex = j;
            RKWaveformSummary(waveform);
            if (response != NULL) {
                sprintf(response, "ACK. Waveform '%s' loaded." RKEOL, c);
            }
            RKSetWaveform(radar, waveform);
            transceiver->transmitting = true;
            break;
        case 'x':
            // Simulate critical temperature
            transceiver->simFault = !transceiver->simFault;
            sprintf(response, "ACK. simFault -> %d" RKEOL, transceiver->simFault);
            break;
        case 'z':
            transceiver->transmitting = false;
            break;
        default:
            if (response != NULL) {
                sprintf(response, "NAK. Command not understood." RKEOL);
            }
            break;
    }
    return RKResultSuccess;
}

int RKTestTransceiverFree(RKTransceiver transceiverReference) {
    int k;
    RKTestTransceiver *transceiver = (RKTestTransceiver *)transceiverReference;
    for (k = 0; k < 2; k++) {
        if (transceiver->waveformCache[k]) {
            if (transceiver->verbose > 1) {
                RKLog("%s Freeing waveform cache %d '%s' ...\n", transceiver->name, k, transceiver->waveformCache[k]->name);
            }
            RKWaveformFree(transceiver->waveformCache[k]);
        }
    }
    free(transceiver);
    return RKResultSuccess;
}

#pragma mark - Pedestal Emulator

void *RKTestPedestalRunLoop(void *input) {
    RKTestPedestal *pedestal = (RKTestPedestal *)input;
    RKRadar *radar = pedestal->radar;
    
    float azimuth = 0.0f;
    float elevation = 0.0f;
    double dt = 0.0;
    struct timeval t0, t1;
    unsigned long tic = 19760520;
    bool scanStartEndPPI = false;
    bool scanStartRHI = false;
    bool scanEndRHI = false;
    bool elTransition = false;

    pedestal->state |= RKEngineStateWantActive;
    pedestal->state &= ~RKEngineStateActivating;

    gettimeofday(&t1, NULL);

    RKLog("%s Started.   mem = %s B\n", pedestal->name, RKUIntegerToCommaStyleString(pedestal->memoryUsage));

    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("%s fs = %s Hz\n", pedestal->name, RKIntegerToCommaStyleString((long)(1.0 / PEDESTAL_SAMPLING_TIME)));
    }

    // Use a counter that mimics microsecond increments
    RKSetPositionTicsPerSeconds(radar, 1.0 / PEDESTAL_SAMPLING_TIME);
    int healthTicCount = (int)(0.1 / PEDESTAL_SAMPLING_TIME);
    
    pedestal->state |= RKEngineStateActive;

    int commandCount = pedestal->commandCount;
    
    while (pedestal->state & RKEngineStateWantActive) {
        if (commandCount != pedestal->commandCount) {
            commandCount = pedestal->commandCount;
            elevation = pedestal->scanElevation;
            azimuth = pedestal->scanAzimuth;
        }

        // Get a vacation position to fill it in with the latest reading
        RKPosition *position = RKGetVacantPosition(radar);
        if (position == NULL) {
            usleep(1000);
            continue;
        }
        position->tic = tic++;
        position->elevationDegrees = elevation;
        position->azimuthDegrees = azimuth;
        position->azimuthVelocityDegreesPerSecond = pedestal->speedAzimuth;
        position->elevationVelocityDegreesPerSecond = pedestal->speedElevation;
        position->flag |= RKPositionFlagAzimuthEnabled | RKPositionFlagElevationEnabled;

        if (pedestal->scanMode == RKTestPedestalScanModePPI) {
            position->sweepElevationDegrees = pedestal->scanElevation;
            position->sweepAzimuthDegrees = 0.0f;
            position->flag |= RKPositionFlagScanActive | RKPositionFlagAzimuthSweep | RKPositionFlagElevationPoint | RKPositionFlagScanActive | RKPositionFlagVCPActive;
        } else if (pedestal->scanMode == RKTestPedestalScanModeRHI) {
            position->sweepAzimuthDegrees = pedestal->scanAzimuth;
            position->sweepElevationDegrees = 0.0f;
            position->flag |= RKPositionFlagScanActive | RKPositionFlagElevationSweep | RKPositionFlagAzimuthPoint | RKPositionFlagScanActive | RKPositionFlagVCPActive;
        } else {
            position->sweepAzimuthDegrees = 0.0f;
            position->sweepElevationDegrees = 0.0f;
        }
        if (scanStartEndPPI) {
            scanStartEndPPI = false;
            position->flag |= RKPositionFlagAzimuthComplete;
            if (pedestal->verbose > 1) {
                RKLog("%s scanStartEndPPI\n", pedestal->name);
            }
        }
        if (scanStartRHI) {
            scanStartRHI = false;
            position->flag |= RKPositionFlagElevationComplete;
        } else if (scanEndRHI) {
            scanEndRHI = false;
        }
        RKSetPositionReady(radar, position);
        
        // Report health
        if (tic % healthTicCount == 0) {
            RKHealth *health = RKGetVacantHealth(radar, RKHealthNodePedestal);
            if (health) {
                sprintf(health->string, "{"
                        "\"Pedestal AZ\":{\"Value\":\"%.2f deg\",\"Enum\":%d}, "
                        "\"Pedestal EL\":{\"Value\":\"%.2f deg\",\"Enum\":%d}, "
                        "\"Pedestal AZ Safety\":{\"Value\":true,\"Enum\":%d}, "
                        "\"Pedestal EL Safety\":{\"Value\":true,\"Enum\":%d}, "
                        "\"VCP Active\":{\"Value\":true,\"Enum\":%d}, "
                        "\"Pedestal Operate\":{\"Value\":true,\"Enum\":%d}"
                        "}",
                        position->azimuthDegrees, RKStatusEnumNormal,
                        position->elevationDegrees, RKStatusEnumNormal,
                        RKStatusEnumNormal,
                        RKStatusEnumNormal,
                        position->elevationVelocityDegreesPerSecond > 0.1f || position->azimuthVelocityDegreesPerSecond > 0.1f ? RKStatusEnumNormal : RKStatusEnumStandby,
                        position->elevationVelocityDegreesPerSecond > 0.1f || position->azimuthVelocityDegreesPerSecond > 0.1f ? RKStatusEnumNormal : RKStatusEnumStandby);
                RKSetHealthReady(radar, health);
            }
        }
        
        // Posiiton change
        if (pedestal->scanMode == RKTestPedestalScanModePPI) {
            azimuth += pedestal->speedAzimuth * PEDESTAL_SAMPLING_TIME;
            if (azimuth >= 360.0f) {
                azimuth -= 360.0f;
            } else if (azimuth < 0.0f) {
                azimuth += 360.0f;
            }
            // Transition over 0 degree
            if (pedestal->speedAzimuth > 0.0f && azimuth < 5.0f && position->azimuthDegrees > 355.0f) {
                scanStartEndPPI = true;
            } else if (pedestal->speedAzimuth < 0.0f && azimuth > 355.0f && position->azimuthDegrees < 5.0f) {
                scanStartEndPPI = true;
            }
            if (scanStartEndPPI &&
                azimuth > pedestal->speedAzimuth * PEDESTAL_SAMPLING_TIME &&
                azimuth < 360.0f - pedestal->speedAzimuth * PEDESTAL_SAMPLING_TIME) {
                fprintf(stderr, "Unexpected. azimuth = %.2f   position->azimuthDegrees = %.2f   speed = %.2f\n",
                        azimuth, position->azimuthDegrees, pedestal->speedAzimuth);
            }
        } else if (pedestal->scanMode == RKTestPedestalScanModeRHI) {
            if (elTransition) {
                elevation -= 2.0f * pedestal->speedElevation * PEDESTAL_SAMPLING_TIME;
            } else {
                elevation += pedestal->speedElevation * PEDESTAL_SAMPLING_TIME;
            }
            if (elevation > 180.0f) {
                elevation -= 360.0f;
            } else if (elevation < -180.0f) {
                elevation += 360.0f;
            }
            if (pedestal->speedElevation > 0.0f) {
                if (elevation > pedestal->rhiElevationEnd) {
                    scanEndRHI = true;
                    elTransition = true;
                    position->flag &= ~RKPositionFlagScanActive;
                } else if (elevation < pedestal->rhiElevationStart) {
                    scanStartRHI = true;
                    elTransition = false;
                    position->flag |= RKPositionFlagScanActive;
                }
            }
        } else if (pedestal->scanMode == RKTestPedestalScanModeBadPedestal) {
            azimuth = (float)rand() * 360.0f / RAND_MAX;
        }
        
        // Wait to simulate sampling time
        do {
            gettimeofday(&t0, NULL);
            dt = RKTimevalDiff(t0, t1);
            usleep(1000);
        } while (pedestal->state & RKEngineStateWantActive && dt < PEDESTAL_SAMPLING_TIME);
        t1 = t0;
    }
    
    pedestal->state ^= RKEngineStateActive;
    return NULL;
}

RKPedestal RKTestPedestalInit(RKRadar *radar, void *input) {
    RKTestPedestal *pedestal = (RKTestPedestal *)malloc(sizeof(RKTestPedestal));
    if (pedestal == NULL) {
        RKLog("Error. Unable to allocate a test pedestal.\n");
        exit(EXIT_FAILURE);
    }
    memset(pedestal, 0, sizeof(RKTestPedestal));
    sprintf(pedestal->name, "%s<AimPedestalCast>%s",
            rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorPedestalRelayPedzy) : "",
            rkGlobalParameters.showColor ? RKNoColor : "");
    pedestal->memoryUsage = sizeof(RKTestPedestal);
    pedestal->radar = radar;
    pedestal->state = RKEngineStateAllocated;

    // Parse input here if there is any

    pedestal->state |= RKEngineStateActivating;
    if (pthread_create(&pedestal->tidRunLoop, NULL, RKTestPedestalRunLoop, pedestal)) {
        RKLog("%s. Unable to create pedestal run loop.\n", pedestal->name);
    }
    //RKLog("Pedestal input = '%s'\n", input == NULL ? "(NULL)" : input);
    while (!(pedestal->state & RKEngineStateActive)) {
        usleep(10000);
    }

    return (RKPedestal)pedestal;
}

int RKTestPedestalExec(RKPedestal pedestalReference, const char *command, char *response) {
    RKTestPedestal *pedestal = (RKTestPedestal *)pedestalReference;
    RKRadar *radar = pedestal->radar;
    
    int k;
    char sval[4][64];
    
    if (!strcmp(command, "disconnect")) {
        if (pedestal->state & RKEngineStateWantActive) {
            pedestal->state ^= RKEngineStateWantActive;
            if (radar->desc.initFlags & RKInitFlagVerbose) {
                RKLog("%s Disconnecting ...", pedestal->name);
            }
        } else {
            if (radar->desc.initFlags & RKInitFlagVerbose) {
                RKLog("%s Deactivate multiple times\n", pedestal->name);
            }
            return RKResultEngineDeactivatedMultipleTimes;
        }
        pedestal->state |= RKEngineStateDeactivating;
        pthread_join(pedestal->tidRunLoop, NULL);
        pedestal->state ^= RKEngineStateDeactivating;
        if (response != NULL) {
            sprintf(response, "ACK. Pedestal stopped." RKEOL);
        }
        if (radar->desc.initFlags & RKInitFlagVerbose) {
            RKLog("%s Stopped.\n", pedestal->name);
        }
    } else if (!strncmp(command, "state", 5)) {
        if (fabsf(pedestal->speedAzimuth) > 0.1f || fabsf(pedestal->speedElevation) > 0.1f) {
            sprintf(response, "1" RKEOL);
        } else {
            sprintf(response, "0" RKEOL);
        }
    } else if (!strncmp(command, "stop", 4)) {
        pedestal->scanMode = RKTestPedestalScanModeNull;
        pedestal->scanElevation = 0.0f;
        pedestal->scanAzimuth = 0.0f;
        pedestal->speedElevation = 0.0f;
        pedestal->speedAzimuth = 0.0f;
        if (response != NULL) {
            sprintf(response, "ACK. Pedestal stopped." RKEOL);
        }
    } else if (!strncmp(command, "ppi", 3)) {
        k = sscanf(command, "%s %s %s", sval[0], sval[1], sval[2]);
        if (k == 3) {
            pedestal->scanMode = RKTestPedestalScanModePPI;
            pedestal->commandCount++;
            pedestal->scanElevation = atof(sval[1]);
            pedestal->speedAzimuth = atof(sval[2]);
        }
        if (response != NULL) {
            sprintf(response, "ACK. PPI mode set at EL %.2f @ %.2f deg/sec" RKEOL, pedestal->scanElevation, pedestal->speedAzimuth);
        }
    } else if (!strncmp(command, "rhi", 3)) {
        k = sscanf(command, "%s %s %s %s", sval[0], sval[1], sval[2], sval[3]);
        if (k == 4) {
            pedestal->scanMode = RKTestPedestalScanModeRHI;
            pedestal->commandCount++;
            pedestal->scanAzimuth = atof(sval[1]);
            pedestal->speedElevation = atof(sval[3]);
            sscanf(sval[2], "%f,%f", &pedestal->rhiElevationStart, &pedestal->rhiElevationEnd);
        }
        if (response != NULL) {
            sprintf(response, "ACK. RHI mode set at AZ %.2f over %.2f-%.2f deg @ %.2f deg/sec" RKEOL,
                    pedestal->scanAzimuth, pedestal->rhiElevationStart, pedestal->rhiElevationEnd, pedestal->speedElevation);
        }
    } else if (!strncmp(command, "bad", 3)) {
        pedestal->scanMode = RKTestPedestalScanModeBadPedestal;
        pedestal->commandCount++;
        if (response != NULL) {
            sprintf(response, "ACK. Simulating bad pedestal" RKEOL);
        }
    } else if (!strcmp(command, "help")) {
        sprintf(response,
                "Commands:\n"
                UNDERLINE("help") " - Help list\n"
                UNDERLINE("ppi") " [EL] [AZ_RATE] - PPI scan at elevation EL at AZ_RATE deg/s.\n"
                UNDERLINE("rhi") " [AZ] [EL_START,EL_END] [EL_RATE] - RHI at AZ over EL_START to EL_END.\n"
                );
    } else if (response != NULL) {
        sprintf(response, "NAK. Command not understood." RKEOL);
    }
    
    return RKResultSuccess;
}

int RKTestPedestalFree(RKPedestal pedestalReference) {
    RKTestPedestal *pedestal = (RKTestPedestal *)pedestalReference;
    free(pedestal);
    return RKResultSuccess;
}

#pragma mark - Health Relay Emulator

void *RKTestHealthRelayRunLoop(void *input) {
    RKTestHealthRelay *healthRelay = (RKTestHealthRelay *)input;
    RKRadar *radar = healthRelay->radar;
    
    int n;
    float powerH, powerV;
    double latitude, longitude, heading;
    double dt = 0.0;
    struct timeval t1, t0;

    healthRelay->state |= RKEngineStateWantActive;
    healthRelay->state &= ~RKEngineStateActivating;

    gettimeofday(&t1, NULL);
    
    RKLog("%s Started.   mem = %s B\n", healthRelay->name, RKUIntegerToCommaStyleString(healthRelay->memoryUsage));
    
    if (radar->desc.initFlags & RKInitFlagVerbose) {
        RKLog("%s fs = %s Hz\n", healthRelay->name, RKIntegerToCommaStyleString((long)(1.0 / HEALTH_RELAY_SAMPLING_TIME)));
    }

    healthRelay->state |= RKEngineStateActive;

    while (healthRelay->state & RKEngineStateWantActive) {
        powerH = (float)rand() / RAND_MAX - 0.5f;
        powerV = (float)rand() / RAND_MAX - 0.5f;
        latitude = (double)rand() * 8.0e-6f / RAND_MAX + 35.181251;
        longitude = (double)rand() * 8.0e-6f / RAND_MAX - 97.4349928;
        heading = (double)rand() * 0.2 / RAND_MAX + 45.0;
        RKHealth *health = RKGetVacantHealth(radar, RKHealthNodeTweeta);
        if (health) {
            sprintf(health->string, "{"
                    "\"PSU H\":{\"Value\":true, \"Enum\":%d}, "
                    "\"PSU V\":{\"Value\":true, \"Enum\":%d}, "
                    "\"GPS Valid\":{\"Value\":true, \"Enum\":0}, "
                    "\"GPS Latitude\":{\"Value\":\"%.7f\",\"Enum\":0}, "
                    "\"GPS Longitude\":{\"Value\":\"%.7f\",\"Enum\":0}, "
                    "\"GPS Heading\":{\"Value\":\"%.1f deg\",\"Enum\":0}, "
                    "\"Platform Pitch\":{\"Value\":\"%.2f deg\",\"Enum\":%d}, "
                    "\"Platform Roll\":{\"Value\":\"%.2f deg\",\"Enum\":%d}"
                    "}",
                    RKStatusEnumNormal,
                    RKStatusEnumNormal,
                    latitude,
                    longitude,
                    heading,
                    powerH, RKStatusEnumNormal,
                    powerV, RKStatusEnumNormal);
            RKSetHealthReady(radar, health);
        }

        // Wait to simulate sampling time
        n = 0;
        do {
            gettimeofday(&t0, NULL);
            dt = RKTimevalDiff(t0, t1);
            usleep(10000);
            n++;
        } while (healthRelay->state & RKEngineStateWantActive && dt < HEALTH_RELAY_SAMPLING_TIME);
        t1 = t0;
    }

    healthRelay->state ^= RKEngineStateActive;
    return NULL;
}

RKHealthRelay RKTestHealthRelayInit(RKRadar *radar, void *input) {
    RKTestHealthRelay *healthRelay = (RKTestHealthRelay *)malloc(sizeof(RKTestHealthRelay));
    if (healthRelay == NULL) {
        RKLog("Error. Unable to allocate a test pedestal.\n");
        exit(EXIT_FAILURE);
    }
    memset(healthRelay, 0, sizeof(RKHealthRelay));
    sprintf(healthRelay->name, "%s<HealthRelayCast>%s",
            rkGlobalParameters.showColor ? RKGetBackgroundColorOfIndex(RKEngineColorHealthRelayTweeta) : "",
            rkGlobalParameters.showColor ? RKNoColor : "");
    healthRelay->memoryUsage = sizeof(RKTestPedestal);
    healthRelay->radar = radar;
    healthRelay->state = RKEngineStateAllocated;
    
    // Parse input here if there is any
    
    healthRelay->state |= RKEngineStateActivating;
    if (pthread_create(&healthRelay->tidRunLoop, NULL, RKTestHealthRelayRunLoop, healthRelay)) {
        RKLog("%s. Unable to create pedestal run loop.\n", healthRelay->name);
    }
    //RKLog("Pedestal input = '%s'\n", input == NULL ? "(NULL)" : input);
    while (!(healthRelay->state & RKEngineStateActive)) {
        usleep(10000);
    }
    
    return (RKHealthRelay)healthRelay;
}

int RKTestHealthRelayExec(RKHealthRelay healthRelayReference, const char *command, char *response) {
    RKTestHealthRelay *healthRelay = (RKTestHealthRelay *)healthRelayReference;
    RKRadar *radar = healthRelay->radar;

    if (!strcmp(command, "disconnect")) {
        if (healthRelay->state & RKEngineStateWantActive) {
            healthRelay->state ^= RKEngineStateWantActive;
            if (radar->desc.initFlags & RKInitFlagVerbose) {
                RKLog("%s Disconnecting ...", healthRelay->name);
            }
        } else {
            if (radar->desc.initFlags & RKInitFlagVerbose) {
                RKLog("%s Deactivate multiple times\n", healthRelay->name);
            }
            return RKResultEngineDeactivatedMultipleTimes;
        }
        healthRelay->state |= RKEngineStateDeactivating;
        pthread_join(healthRelay->tidRunLoop, NULL);
        healthRelay->state ^= RKEngineStateDeactivating;
        if (response != NULL) {
            sprintf(response, "ACK. Transceiver stopped." RKEOL);
        }
        if (radar->desc.initFlags & RKInitFlagVerbose) {
            RKLog("%s Stopped.\n", healthRelay->name);
        }
    } else if (!strcmp(command, "help")) {
        sprintf(response,
                "Commands:\n"
                UNDERLINE("help") " - Help list\n"
                );
    } else if (response != NULL) {
        sprintf(response, "NAK. Command not understood." RKEOL);
    }
    
    return RKResultSuccess;
}

int RKTestHealthRelayFree(RKHealthRelay healthRelayReference) {
    RKTestHealthRelay *healthRelay = (RKTestHealthRelay *)healthRelayReference;
    free(healthRelay);
    return RKResultSuccess;
}

#pragma mark - Others

static void *pushLoop(void *in) {
    RKCommandQueue *queue = (RKCommandQueue *)in;
    const char name[] = "<head>";
    RKCommand command;
    RKLog("%s Started.   queue @ %p\n", name, queue);
    for (int k = 0; k < 10; k++) {
        sprintf(command, "t %d", k);
        RKCommandQueuePush(queue, &command);
        RKLog("%s Command '%s' pushed.\n", name, command);
        usleep(200000);
    }
    RKLog("%s retiring ...\n", name);
    return NULL;
}

static void *popLoop(void *in) {
    RKCommandQueue *queue = (RKCommandQueue *)in;
    const char name[] = "<tail>";
    RKLog("%s Started.   queue @ %p\n", name, queue);
    do {
        RKCommand *command = RKCommandQueuePop(queue);
        if (command) {
            RKLog("%s Command '%s' popped.\n", name, command);
        } else {
            usleep(10000);
        }
    } while (queue->toc < 10);
    RKLog("%s retiring ...\n", name);
    return NULL;
}

void RKTestCommandQueue(void) {
    pthread_t tidPush, tidPop;
    RKCommandQueue *queue = RKCommandQueueInit(8, true);
    pthread_create(&tidPush, NULL, pushLoop, queue);
    pthread_create(&tidPop, NULL, popLoop, queue);
    pthread_join(tidPush, NULL);
    pthread_join(tidPop, NULL);
    RKCommandQueueFree(queue);
}

void RKTestSingleCommand(void) {
    SHOW_FUNCTION_NAME
}

void RKTestExperiment(void) {
    SHOW_FUNCTION_NAME
    // For Bird bath ZCal
    // o  A pedestal command to scan 360-deg at 90-deg EL
    // o  Wait for a sweep to complete
    // o  Stop the pedestal
    // o  Get the sweep, derive average ZDR
    // o  Derive ZCal
    
    // For SunCal
    // o  Get time of day --> elevation of the Sun
    // o  Pedestal command to scan 360-deg at Sun's EL
    // o  Wait for a sweep to complete
    // o  Find the peak of Sh + Sv
    // o  Derive heading
    
    // RadarKit should not know what is the pedestal command
    // One option: Have user fill in the task variables:
    // - Pedestal command for RKPedestalExec()
    // - Transceiver command for RKTransceiverExec()
    // - HealthRelay command for RKHealthRelayExec()
    // - Sweep end flag is defined here so waiting for a sweep complete is okay
    //   - Have the task sub-routine detect if the sweep is progressing as expected
    // - Stop command for RKPedestalExec()
    // - Stop command for RKTransceiverExec()
    // - Stop command for RKHealthRelayExec()
    // - Task function to modify pref.conf or user definied config file
    
    // Modify a few bytes
    FILE *fid = fopen("/Users/boonleng/Downloads/rkr/PX10K-20190827-203331.rkr", "r+");
    if (fid == NULL) {
        RKLog("Error. Unable to open file.\n");
        return;
    }
    RKFileHeader *fileHeader = (RKFileHeader *)malloc(sizeof(RKFileHeader));
    fread(fileHeader, sizeof(RKFileHeader), 1, fid);
    
    RKLog("PRT = %.4e\n", fileHeader->config.prt[0]);
    RKLog("SNRThreshold = %.4f dB\n", fileHeader->config.SNRThreshold);
    RKLog("SQIThreshold = %.4f\n", fileHeader->config.SQIThreshold);

    RKPulseHeader *pulseHeader = (RKPulseHeader *)malloc(sizeof(RKPulseHeader));
    fread(pulseHeader, sizeof(RKPulseHeader), 1, fid);
    
    RKLog("RKPulse capacity / gateCount = %u / %u\n", pulseHeader->capacity, pulseHeader->gateCount);
    
    // Change some values
    fileHeader->config.prt[0] = 1.0 / 1450.0;
    
    rewind(fid);
    fwrite(fileHeader, sizeof(RKFileHeader), 1, fid);

    // Forward to the end of file
    fseek(fid, 0L, SEEK_END);
    fclose(fid);
}

#pragma mark -
