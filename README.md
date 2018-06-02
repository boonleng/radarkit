RadarKit
===

First of all, many thanks for your interest in the framework! :smile: :thumbsup: :punch:

The RadarKit is a straight C framework. This is a toolkit that abstracts various components of a radar signal processor. Mainly, the real-time operations of data collection, data transportation through network, rudimentary processing from raw I/Q data to _base moment_ products. The main idea is to have developers only implement the interface between a _digital transceiver_, a _pedestal_, and a generic _health relay_. The RadarKit framework combines all of these information, generates radar product files, provides live data and display streams, and redirects the control commands to the hardware.

## System Requirements
- Processors capable of SSE, SSE2, SSE3
- Optional: AVX, AVX-256, AVX-512

## Getting the Project

Follow these steps to get the project

1. Clone a git project using one of the following commands in Terminal:

    ```shell
    git clone https://github.com/ouradar/radarkit.git
    ```
    or
    ```shell
    git clone https://git.arrc.ou.edu/radar/radarkit.git
    ```

2. Get the required packages, which can either be installed through one of the package managers or compiled from source, which I would recommend for the FFTW library since it can take advantage of SSE, SSE2, AVX and AVX-2 accelerations.
    - [FFTW]
    - [NetCDF]

    ##### Debian / Ubuntu #####

    ```shell
    apt-get install libfftw3-dev libnetcdf-dev
    ```
    
    ##### CentOS 7 #####
    
    ```shell
    yum install epel-release
    yum install fftw-devel netcdf-devel
    ```
    
    ##### Mac OS X #####
    
    I use [Homebrew] as my package manager for macOS. I highly recommend it.
    
    ```shell
    brew install fftw homebrew/science/netcdf
    ```
    
3. Compile and install the framework.

    ```shell
    make
    sudo make install
    ```
[FFTW]: http://www.fftw.org
[NetCDF]: http://www.unidata.ucar.edu/software/netcdf
[Homebrew]: http://brew.sh


## Basic Usage for a Radar Host

1. Initialize a _radar_ object (although RadarKit is not an objective implementation but it is easier to think this way). Supply the necessary _tranceiver_ routines and _pedestal_ routines. The _health relay_ is omitted here for simplicity.

    ```c
    #include <RadarKit.h>
    
    int main() {
        RKRadar *radar = RKInit();
        RKSetTransceiver(radar, userInput, transceiverInit, transceiverExec, transceiverFree);
        RKSetPedestal(radar, userInput, pedestalInit, pedestalExec, pedestalFree);
        RKGoLive(radar);
        RKWaitWhileActive(radar);
        RKFree(radar);
    }
    ```

2. Set up a _transceiver_ initialization and run-loop routines. The initialization routine returns a user-defined pointer, and a run-loop routine receives I/Q data. The initialization routine must return immediately, and the run-loop routine should be created as a separate thread.

    ```c
    RKTransceiver transceiverInit(RKRadar *radar, void *userInput) {
        // Allocate your own resources, define your structure somewhere else
        UserTransceiverStruct *resource = (UserTransceiverStruct *)malloc(sizeof(UserTransceiverStruct));
        
        // Be sure to save a reference to the radar
        resource->radar = radar
        
        // Create your run loop as a separate thread so you can return immediately
        pthread_create(&resource->tid, NULL, transceiverRunLoop, resource);
        
        return (RKTransceiver)resource;
    }
    
    int transceiverExec(RKTransceiver yourTransceiver, const char *command, char *feedback) {
        // Execute commands stored in const char *command
        
        // Provide text feedback to char *feedback
    }
    
    int transceiverFree(RKTransceiver yourTransceiver) {
        // Free up resources
    }
    
    void *transceiverRunLoop(void *in) {
        // Type cast the input to something you defined earlier
        UserTransceiverStruct *resource = (UserTransceiverStruct *)in;
        
        // Now you can recover the radar reference you provided in init routine.
        RKRadar *radar = resource->radar;
        
        // Here is the busy run loop
        while (radar->active) {
            RKPulse *pulse = RKGetVacantPulse(radar);
            pulse->header.gateCount = 1000;
            
            // Go through both polarizations
            for (int p = 0; p < 2; p++) {
                // Get a data pointer to the 16-bit data
                RKInt16C *X = RKGetInt16CDataFromPulse(pulse, p);
                // Go through all range gates and fill in the samples
                for (int g = 0; g < 1000; g++) {
                    // Copy the I/Q samples from hardware interface
                    X->i = 0;
                    X->q = 1;
                    X++;
                }
            }
            RKSetPulseHasData(radar, pulse);
        }
    }
    ```
    
3. Set up a set of _pedestal_ initialization and run-loop routines. The initialization routine returns a user-defined pointer, and a run-loop routine receives position data. The initialization routine must return immediately, and the run-loop routine should be created as a separate thread.
 
    ```c
    RKPedestal pedestalInit(RKRadar *radar, void *userInput) {
        // Allocate your own resources, define your structure somewhere else
        UserPedestalStruct *resource = (UserPedestalStruct *)malloc(sizeof(UserPedestalStruct));
        
        // Be sure to save a reference to the radar
        resource->radar = radar
        
        // Create your run loop as a separate thread so you can return immediately
        pthread_create(&resource->tid, NULL, pedestalRunLoop, resource);
        
        return (RKPedestal)resource;
    }

    int pedestalExec(RKPedestal yourPedestal, const char *command, char *feedback) {
        // Execute commands stored in const char *command
        
        // Provide text feedback to char *feedback
    }
    
    int transceiverFree(RKPedestal yourPedestal) {
        // Free up resources
    }

    int pedestalRunLoop(void *in) {
        // Type cast the input to something you defined earlier
        UserPedestalStruct *resource = (UserPedestalStruct *)in;
        
        // Now you can recover the radar reference you provided in init routine.
        RKRadar *radar = resource->radar;
        
        // Here is the busy run loop
        while (radar->active) {
            RKPosition *position = RKGetVacantPosition(radar);
            
            // Copy the position from hardware interface
            position->az = 1.0;
            position->el = 0.5;
            RKSetPositionReady(radar, position);
        }
    }
    ```
4. Set up _health relay_ initialization and run-loop routines just like the previous two examples.

5. Build the program and link to the RadarKit framework. Note that the required packages should be applied too.

    ```shell
    gcc -o program program.c -lRadarKit -lfftw3f -lnetcdf
    ```


This example is extremely simple. Many optional arguments were set to NULL (execution and free routines were omitted). The actual radar will be more complex but this short example illustrates the simplicity of using RadarKit to abstract all the DSP and non-hardware related tasks.


## Basic Usage on Signal Processing Space

A seprate processing space to generate high-level products is implemented through [PyRadarKit], which is primarily housed at

```
https://git.arrc.ou.edu/radar/PyRadarKit.git
```

and

```
https://github.com/OURadar/PyRadarKit
```


Design Philosophy
===

Three major hardware components of a radar: (i) a __digital transceiver__, (ii) a __pedestal__, and (iii) a __health relay__ (_auxiliary controller_) are not tightly coupled with the RadarKit framework. Only a set of protocol functions are defined so that the RadarKit can be interfaced with other libraries, which are specific to the hardware and/or vendor design. It is the responsibility of user to implement the appropriate interface routines to bridge the data transport and/or control commands. There are three functions needed for each hardware: _init_, _exec_ and _free_, which are routines to allocate an object--akin to object oriented programming, althought RadarKit is a straight C implementation, interact with the object and deallocate the object, respectively. The _exec_ routine has the form to accept text command and produce text response. Some keywords for the command are already defined in the framework so user should not use them. They are intercepted prior to passing down to the _exec_ routine. Detailed usage on these functions will be discussed in detail later.

The __digital transceiver__ is the hardware that requires high-speed data throughput. RadarKit is designed so that redudant memory copy is minimized. That is, a pointer to the memory space for payload will be provided upon a request. User defined routines fill in the data, typically through a copy mechanism through DMA to transport the I/Q data from a transceiver memory to the host memory, which is initialized and managed by RadarKit. The fundamental form is signed 16-bit I and Q, which is a part of `RKPulse` defined in the framework.

The __pedestal__ is the hardware that is usually low speed, typically on the orders of 10 KBps if the position reading is provided at about 100 samples per second. A RadarKit position structure `RKPosition` is defined in the framework. If an interface software [pedzy] is used, which is a light weight pedestal controller, RadarKit can readily ingest position data through a network connection. Otherwise, an `RKPedestalPedzy` replacement can be implemented to provide same functionality. In this case, user is also free to define a new position type. The RadarKit framework does not restrict this definition.

The __health relay__ is the hardware that is also low speed, typically on the orders of 1 KBps. This is also the hardware that can be called an _auxiliary controller_, where everything else is interfaced through this relay and the health information is probed through this controller. A RadarKit health structure `RKHealth` is defined in the framework. More than one health node can be implemented. They provide health information using JSON strings through TCP/IP socket connections. If an interface software [tweeta] or [tweeto] is used, RadarKit can readily ingest auxiliary hardware health data through a TCP/IP network connection. Otherwise, an `RKHealthRelayTweeta` replacement can be implemented to provide same functionality. The RadarKit framework does not restrict this definition.

Base radar products are generated on a ray-by-ray basis. Each ray is of type `RKRay`. Once a sweep is complete, a Level-II data file in NetCDF format will be generated. Live streams and can be view through a desktop application [iRadar].

[pedzy]: https://git.arrc.ou.edu/cheo4524/pedzy
[tweeta]: https://git.arrc.ou.edu/dstarchman/tweeta
[tweeto]: https://git.arrc.ou.edu/cheo4524/tweeto.git
[iRadar]: https://arrc.ou.edu/tools
[PyRadarKit]: https://git.arrc.ou.edu/radar/PyRadarKit.git

Radar Struct
===

This is about the only structure you need to worry about. A radar structure represents an object-like structure where everything is encapsulated.


### Life Cycle

These are functions that allocate and deallocate a radar struct.

```c
RKRadar *RKInitWithDesc(RKRadarDesc);
RKRadar *RKInitLean(void);               // For a lean system, PX-1000 like
RKRadar *RKInitMean(void);               // For a medium system, RaXPol like
RKRadar *RKInitFull(void);               // For a high-performance system, PX-10,000 like
RKRadar *RKInit(void);                   // Everything based on default settings, in between mean & lean
int RKFree(RKRadar *radar);
```

### Properties

Hardware hooks are provided to communicate with a digital transceiver, a positioner and various sensors. They must obey the protocol to implement three important functions: _init_, _exec_ and _free_ routines. These functions will be called to start the hardware routine, execute text form commands that will be passed down the master controller, and to deallocate the resources properly upon exit, respectively.

```c
// Set the transceiver. Pass in function pointers: init, exec and free
int RKSetTransceiver(RKRadar *,
                     void *initInput,
                     RKTransceiver initRoutine(RKRadar *, void *),
                     int execRoutine(RKTransceiver, const char *, char *),
                     int freeRoutine(RKTransceiver));

// Set the pedestal. Pass in function pointers: init, exec and free
int RKSetPedestal(RKRadar *,
                  void *initInput,
                  RKPedestal initRoutine(RKRadar *, void *),
                  int execRoutine(RKPedestal, const char *, char *),
                  int freeRoutine(RKPedestal));

// Set the health relay. Pass in function pointers: init, exec and free
int RKSetHealthRelay(RKRadar *,
                     void *initInput,
                     RKHealthRelay initRoutine(RKRadar *, void *),
                     int execRoutine(RKHealthRelay, const char *, char *),
                     int freeRoutine(RKHealthRelay));

// Some states of the radar
int RKSetVerbosity(RKRadar *radar, const int verbose);
int RKSetProcessingCoreCounts(RKRadar *radar,
                              const unsigned int pulseCompressionCoreCount,
                              const unsigned int momentProcessorCoreCount);

// Waveform routines
RKWaveform *RKWaveformInitAsImpulse(void);
RKWaveform *RKWaveformInitFromFile(const char *filename);
RKWaveform *RKWaveformInitAsLinearFrequencyModulation(const double fs, const double fc, const double pulsewidth, const double bandwidth);
RKWaveform *RKWaveformInitAsFrequencyHops(const double fs, const double fc, const double pulsewidth, const double bandwidth, const int count);
void RKWaveformOnes(RKWaveform *);
void RKWaveformHops(RKWaveform *, const double fs, const double fc, const double bandwidth);
void RKWaveformLinearFrequencyModulation(RKWaveform *, const double fs, const double fc, const double pulsewidth, const double bandwidth);
void RKWaveformDecimate(RKWaveform *, const int);
void RKWaveformConjuate(RKWaveform *);
void RKWaveformDownConvert(RKWaveform *);
void RKWaveformWrite(RKWaveform *, const char *);
void RKWaveformNormalizeNoiseGain(RKWaveform *);
void RKWaveformSummary(RKWaveform *);

// Some operating parameters
int RKSetWaveform(RKRadar *radar, RKWaveform *waveform, const int group);
int RKSetWaveformToImpulse(RKRadar *radar);
uint32_t RKGetPulseCapacity(RKRadar *radar);
```

### Interactions

```c
// State
int RKGoLive(RKRadar *);
int RKWaitWhileActive(RKRadar *);
int RKStart(RKRadar *);
int RKStop(RKRadar *);
int RKSoftRestart(RKRadar *);
int RKResetClocks(RKRadar *);
int RKExecuteCommand(RKRadar *, const char *, char *);
void RKPerformMasterTaskInBackground(RKRadar *, const char *);

// Tasks
void RKMeasureNoise(RKRadar *);
void RKSetSNRThreshold(RKRadar *, const RKFloat);

// Status
RKStatus *RKGetVacantStatus(RKRadar *);
void RKSetStatusReady(RKRadar *, RKStatus *);

// Configs
void RKAddConfig(RKRadar *radar, ...);
RKConfig *RKGetLatestConfig(RKRadar *radar);

// Healths
RKHealthNode RKRequestHealthNode(RKRadar *);
RKHealth *RKGetVacantHealth(RKRadar *, const RKHealthNode);
void RKSetHealthReady(RKRadar *, RKHealth *);
RKHealth *RKGetLatestHealth(RKRadar *);
RKHealth *RKGetLatestHealthOfNode(RKRadar *, const RKHealthNode);
int RKGetEnumFromLatestHealth(RKRadar *, const char *);

// Positions
RKPosition *RKGetVacantPosition(RKRadar *);
void RKSetPositionReady(RKRadar *, RKPosition *);
RKPosition *RKGetLatestPosition(RKRadar *);
float RKGetPositionUpdateRate(RKRadar *);

// Pulses
RKPulse *RKGetVacantPulse(RKRadar *);
void RKSetPulseHasData(RKRadar *, RKPulse *);
void RKSetPulseReady(RKRadar *, RKPulse *);
RKPulse *RKGetLatestPulse(RKRadar *);

// Rays
RKRay *RKGetVacantRay(RKRadar *);
void RKSetRayReady(RKRadar *, RKRay *);

// Waveform Calibrations
void RKAddWaveformCalibration(RKRadar *, const RKWaveformCalibration *);
void RKUpdateWaveformCalibration(RKRadar *, const uint8_t, const RKWaveformCalibration *);
void RKClearWaveformCalibrations(RKRadar *);
void RKConcludeWaveformCalibrations(RKRadar *);

// Controls
void RKAddControl(RKRadar *, const RKControl *);
void RKAddControlAsLabelAndCommand(RKRadar *radar, const char *label, const char *command);
void RKUpdateControl(RKRadar *, const uint8_t, const RKControl *);
void RKClearControls(RKRadar *);
void RKConcludeControls(RKRadar *);
```

### Accessing Data of Pulses / Rays (RKFoundation.h)

Most data are stored in the plain C format within a structure of a pulse or ray. RadarKit uses a carefully designed header structure to ensure SIMD alignment of all the data so that SIMD parallelization can be utilized even at the moment data level. For a pulse, the raw data straight from the ADC is stored as 16-bit signed integer, interleaved between the real and imaginary part. I(0), Q(0), I(1), Q(1), ..., I(N-1), Q(N-1). The pulse compression engine uses these data to produced a compressed pulse through match filtering and stores the result in both interleaved I/Q and blocked I/Q. The interleaved I/Q is I(0), Q(0), I(1), Q(1), ..., I(N-1), Q(N-1) while the blocked I/Q would be in I(0), I(1), ..., I(N-1), Q0, Q1, ..., Q(N-1). Depending on the algorithm you want to introduce, one format may be more efficient than the other. Choose wisely.

```c
// Getting various data pointers of a pulse
RKInt16C *RKGetInt16CDataFromPulse(RKPulse *, const uint32_t channelIndex);
RKComplex *RKGetComplexDataFromPulse(RKPulse *, const uint32_t channelIndex);
RKIQZ RKGetSplitComplexDataFromPulse(RKPulse *, const uint32_t channelIndex);

// Getting various data pointers of a ray
uint8_t *RKGetUInt8DataFromRay(RKRay *, const uint32_t productIndex);
float *RKGetFloatDataFromRay(RKRay *, const uint32_t productIndex);
```

# Hardware Routines

As mentioend previously, the initialization, execution and deallocation routines of the _transceiver_, _pedestal_, and _health relay_ must have a strict form, as follows. The intialization of the hardware must be in the form of

```c
RKTransceiver initRoutine(RKRadar *, void *);
RKPedestal    initRoutine(RKRadar *, void *);
RKHealthRelay initRoutine(RKRadar *, void *);
```

while the execution of command and the return of response must be in the form of

```c
int execRoutine(RKTransceiver, const char *command, char *response);
int execRoutine(RKPedestal, const char *command, char *response);
int execRoutine(RKHealthRelay, const char *command, char *response);
```

and finally, the resource free routine must be in the form of

```c
int freeRoutine(RKTransceiver);
int freeRoutine(RKPedestal);
int freeRoutine(RKHealthRelay);
```

Here is a simple example of execution routine of a transceiver that response to a PRT change

```c
int execRoutine(RKTransceiver userTransceiver, const char *command, char *response) {
    // Type cast it to your defined type
    UserTransceiverStruct transceiver = (UserTransceiverStruct *)userTransceiver;
    
    // Restore the radar reference.
    RKRadar *radar = transceiver->radar;
    
    // Do something with the instruction, say change the prt
    float prt;
    char dummy[64];
    if (!strcmp(command, "prt")) {
        sscanf(command, "%s %f", dummy, &prt);
        transceiver->prt = prt;
        sprintf(response, "ACK. Command executed.");
    }
    return 0;
}

```

Reserved Keywords for Commands
===

### `disconnect`

This is a command the master controller issues when everything should stop.

### `state`

This is a command the master controller issues for checking if the component wants to report opereate (1) or standby (0)

RadarKit Test Program
===

A test program is provided to assess if everything can run properly with your system. Call it with a _help_ option to show all the available options.

```
rktest --help
```

### RadarKit Performance Test

Some performance tests are implemented to get an idea of the number of workers to use. Here's an example output from the RaXPol main host:

```
marina:~/radarkit root$ rktest -T 51
===========================
RKTestPulseCompressionSpeed
===========================
2017/12/11 08:34:31 PulseCompression
                    Test 0 -> 0.220 ms / pulse
                    Test 1 -> 0.220 ms / pulse
                    Test 2 -> 0.220 ms / pulse
                    Time for each pulse (8,192 gates) = 0.220 ms / pulse (Best of 3)
                    Speed: 4541.07 pulses / sec
marina:~/radarkit root$ rktest -T 52
==========================
RKTestMomentProcessorSpeed
==========================
2017/12/11 08:34:34 PulsePairHop:
                    Test 0 -> 1.12 ms
                    Test 1 -> 1.12 ms
                    Test 2 -> 1.12 ms
                    Time for each ray (100 pulses x 4,096 gates) = 1.12 ms (Best of 3)
                    Speed: 892.23 rays / sec
2017/12/11 08:34:34 MultiLag (L = 2):
                    Test 0 -> 4.87 ms
                    Test 1 -> 4.88 ms
                    Test 2 -> 4.87 ms
                    Time for each ray (100 pulses x 4,096 gates) = 4.87 ms (Best of 3)
                    Speed: 205.22 rays / sec
2017/12/11 08:34:36 MultiLag (L = 3):
                    Test 0 -> 6.65 ms
                    Test 1 -> 6.65 ms
                    Test 2 -> 6.65 ms
                    Time for each ray (100 pulses x 4,096 gates) = 6.65 ms (Best of 3)
                    Speed: 150.39 rays / sec
2017/12/11 08:34:38 MultiLag (L = 4):
                    Test 0 -> 8.36 ms
                    Test 1 -> 8.35 ms
                    Test 2 -> 8.35 ms
                    Time for each ray (100 pulses x 4,096 gates) = 8.35 ms (Best of 3)
                    Speed: 119.83 rays / sec
```
