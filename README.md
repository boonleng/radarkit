RadarKit
========

A toolkit with various components of a radar signal processor. Mainly implement the real-time operation of data collection, data transportation through network, rudimentary processing from raw I/Q data to moment data. The main idea is to have user only implement the interface between a digital transceiver, a pedestal, and a general health monitor. RadarKit combines all of these information, generates radar product files, provides display live streams and redirects the control commands to the hardware.

## Getting the Project ##

Follow these steps to get the project

1. Clone a git project using the following command in Terminal:

    ```shell
    git clone https://git.arrc.ou.edu/cheo4524/radarkit.git
    ``````

2. Get the required packages, which can either be installed through one of the package managers or compiled from source.
    - [FFTW]
    - [NetCDF]

    ##### Debian #####

    ```shell
    apt-get install fftw netcdf
    ``````
    
    ##### CentOS 7 #####
    
    ```shell
    yum install epel-release
    yum install fftw-devel netcdf-devel
    ``````
    
    ##### Mac OS X #####
    
    I use [Homebrew] as my package manager for macOS. I highly recommend it.
    
    ```shell
    brew install fftw homebrew/science/netcdf
    ``````
    
3. Compile and install the framework.

    ```shell
    make
    sudo make install
    ``````
[FFTW]: http://www.fftw.org
[NetCDF]: http://www.unidata.ucar.edu/software/netcdf
[Homebrew]: http://brew.sh

## Basic Usage ##

1. Initialize a radar. Supply a tranceiver routine and pedestal routine. The health relay is omitted here for simplicity.

    ```c
    #include <RadarKit.h>
    
    int main() {
        RKRadar *radar = RKInit();
        RKSetTransceiver(radar, NULL, transceiverInit, NULL, NULL);
        RKSetPedestal(radar, NULL, pedestalInit, NULL, NULL);
        RKGoLive(radar);
        RKWaitWhileActive(radar);
        RKFree(radar);
    }
    ``````

2. Set up a _transceiver_ initialization and run-loop routines. The initialization routine returns a user-defined pointer, and a run-loop routine receives I/Q data. The initialization routine must return immediately, and the run-loop routine should be created as a separate thread.

    ```c
    RKTransceiver transceiverInit(RKRadar *radar, void *userInput) {
        // Allocate your own resources, define your structure somewhere else
        UserStruct *resource = malloc(sizeof(UserStruct));
        
        // Be sure to save a reference to radar
        resource->radar = radar
        
        // Create your run loop as a separate thread so you can return immediately
        pthread_create(&resource->tid, NULL, transceiverRunLoop, resource);
        
        return (RKTransceiver)resource;
    }
    
    void *transceiverRunLoop(void *in) {
        // Type cast the input to something you defined earlier
        UserStruct *resource = (UserStruct *)in;
        
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
    ``````
    
3. Set up a set of _pedestal_ initialization and run-loop routines. The initialization routine returns a user-defined pointer, and a run-loop routine receives position data. The initialization routine must return immediately, and the run-loop routine should be created as a separate thread.
 
    ```c
    RKPedestal pedestalInit(RKRadar *radar, void *userInput) {
        // Allocate your own resources, define your structure somewhere else
        UserStruct *resource = malloc(sizeof(UserStruct));
        
        // Be sure to save a reference to radar
        resource->radar = radar
        
        // Create your run loop as a separate thread so you can return immediately
        pthread_create(&resource->tid, NULL, pedestalRunLoop, resource);
        
        return (RKPedestal)resource;
    }

    int pedestalRunLoop(void *in) {
        // Type cast the input to something you defined earlier
        UserStruct *resource = (UserStruct *)in;
        
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
    ``````
4. Set up _health relay_ initialization and run-loop routines just like the previous two examples.

5. Build the program and link to the RadarKit framework. Note that the required packages should be applied too.

    ```shell
    gcc -o program program.c -lRadarKit -lfftw -lnetcdf
    ``````

This example is extremely simple. Many optional arguments were set to NULL (execution and free routines are omitted). The actual radar will be more complex but this short example illustrates the simplicity of using RadarKit to abstract all the DSP and non-hardware related tasks.


Design Philosophy
=================

Three major hardware components of a radar: (i) a __digital transceiver__, (ii) a __pedestal__, and (iii) a __health relay__ (_auxiliary controller_) are not tightly coupled with the RadarKit framework. Only a set of protocol functions are defined so it can be interfaced with other libraries, which are specific to these hardware. It is the user responsibility to implement the appropriate interface routines to bridge the data transport and/or control commands. Some keywords are defined in the framework (still in a work in progress).

The __digital transceiver__ is the hardware that requires high-speed data throughput. RadarKit is designed so that redudant memory copy is minimized. That is, a pointer to the memory space for payload will be provided upon a request. User defined routines fill in the data, typically through a copy mechanism through DMA to transport the I/Q data from a transceiver memory to the host memory, which is initialized and managed by RadarKit. The fundamental form is signed 16-bit I and Q, which is a part of `RKPulse` defined in the framework.

The __pedestal__ is the hardware that usually is relatively low speed, typically on the orders of 10 KBps if the position reading is provided at about 100 samples per second. A proposed strcture `RKPosition` is defined in the framework. If an interface software [pedzy] is used, which is a light weight pedestal controller, RadarKit can readily ingest position data through a network connection. Otherwise, an `RKPedestalPedzy` replacement can be implemented to provide same functionality.

The __health relay__ is the hardware that usually is also relatively low speed, typically on thge orders of 1 KBps. This is also the hardware that can be called an _auxiliary controller_, where everything else is interfaced through this relay and the health information is probed through this controller. A proposed structure `RKHealth` is defined in the framework. They should all be providing health information using JSON strings through a socket connection. If an interface software [tweeta] is used, RadarKit can readily ingest auxiliary hardware health data through a network connection. Otherwise, an `RKHealthRelayTweeta` replacement can be implemented to provide same functionality.

Base radar rroducts are generated on a ray-by-ray basis. Each ray is of type `RKRay`. Once a sweep is complete, a Level-II data file in NetCDF format will be generated. Live streams and can be view through a desktop application [iRadar].

[pedzy]: https://git.arrc.ou.edu/cheo4524/pedzy
[tweeta]: https://git.arrc.ou.edu/dstarchman/tweeta
[iRadar]: https://arrc.ou.edu/tools


Radar Struct
============

This is about the only structure you need to worry about. A radar structure represents an object-like structure where everything is encapsulated.


### Life Cycle ###

These are functions that allocate and deallocate a radar struct.

```c
RKRadar *RKInitWithDesc(RKRadarDesc);
RKRadar *RKInitLean(void);               // For a lean system, PX-1000 like
RKRadar *RKInitMean(void);               // For a medium system, RaXPol like
RKRadar *RKInitFull(void);               // For a high-performance system, PX-10,000 like
RKRadar *RKInit(void);                   // Everything based on default settings, in between mean & lean
int RKFree(RKRadar *radar);
``````


### Properties ###

Hardware hooks are provided to communicate with a digital transceiver, a positioner and various sensors. They must obey the protocol to implement three important functions: _init_, _exec_ and _free_ routines. These functions will be called to start the hardware routine, execute text form commands that will be passed down the master controller, and to deallocate the resources properly upon exit, respectively.

```c
// Set the transceiver. Pass in function pointers: init, exec and free
int RKSetTransceiver(RKRadar *,
                     void *initInput,
                     RKTransceiver initRoutine(RKRadar *, void *),
                     int execRoutine(RKTransceiver, const char *),
                     int freeRoutine(RKTransceiver));

// Set the pedestal. Pass in function pointers: init, exec and free
int RKSetPedestal(RKRadar *,
                  void *initInput,
                  RKPedestal initRoutine(RKRadar *, void *),
                  int execRoutine(RKPedestal, const char *),
                  int freeRoutine(RKPedestal));

// Set the health relay. Pass in function pointers: init, exec and free
int RKSetHealthRelay(RKRadar *,
                     void *initInput,
                     RKHealthRelay initRoutine(RKRadar *, void *),
                     int execRoutine(RKHealthRelay, const char *),
                     int freeRoutine(RKHealthRelay));

// Some states of the radar
int RKSetVerbose(RKRadar *radar, const int verbose);
int RKSetProcessingCoreCounts(RKRadar *radar,
                              const unsigned int pulseCompressionCoreCount,
                              const unsigned int momentProcessorCoreCount);

// Some operating parameters
int RKSetWaveform(RKRadar *radar, const char *filename, const int group, const int maxDataLength);
int RKSetWaveformToImpulse(RKRadar *radar);
int RKSetWaveformTo121(RKRadar *radar);
uint32_t RKGetPulseCapacity(RKRadar *radar);
```

### Interactions ###

```c
// The radar engine state
int RKGoLive(RKRadar *);
int RKWaitWhileActive(RKRadar *);
int RKStop(RKRadar *);

// Positions
RKPosition *RKGetVacantPosition(RKRadar *);
void RKSetPositionReady(RKRadar *, RKPosition *);

// Pulses
RKPulse *RKGetVacantPulse(RKRadar *);
void RKSetPulseHasData(RKRadar *, RKPulse *);
void RKSetPulseReady(RKRadar *, RKPulse *);

// Rays
RKRay *RKGetVacantRay(RKRadar *);
void RKSetRayReady(RKRadar *, RKRay *);
```


RadarKit Test Program
---------------------

A test program is provided to assess if everything can run properly with your system.

```
rktest [options]

OPTIONS:
     Unless specifically stated, all options are interpreted in sequence. Some
     options can be specified multiples times for repetitions. For example, the
     verbosity level increaes by one for every -v.

  -c (--core) P,M (no space after comma)
         Sets the number of threads for pulse compression to P
         and the number of threads for product generator to M.
         If not specified, the default core counts are 8 / 4.

  -f (--prf) value
         Sets the pulse repetition frequency (PRF) to value in Hz.
         If not specified, the default PRF = 5000 Hz.

  -F (--fs or -b) value
         Sets the sampling frequency (bandwidth) to value in Hz.
         If not specified, the default will be used.

  -g (--gate) value
         Sets the number of range gates to value.
         If not specified, the default gate count is 8192.

  -h (--help)
         Shows this help text.

  -L (--test-lean-system)
         Run with arguments '-v -f 2000 -F 5e6 -c 2,2'.

  -M (--test-medium-system)
         Run with arguments '-v -f 5000 -F 20e6 -c 4,2'.

  -p (--pedzy-host) hostname
         Sets the host of pedzy pedestal controller.

  -s (--simulate)
         Sets the program to simulate data stream (default, if none of the tests
         is specified).

  -v (--verbose)
         Increases verbosity level, which can be specified multiple times.

  --test-mod
         Sets the program to test modulo macros.

  --test-simd
         Sets the program to test SIMD instructions.
         To test the SIMD performance, use --test-simd=2

  --test-pulse-compression
         Sets the program to test the pulse compression using a simple case with.
         an impulse filter.

  --test-processor
         Sets the program to test the moment processor.


EXAMPLES:
     Here are some examples of typical configurations.

  radar
         Runs the program with default settings.

  radar -f 2000
         Runs the program with PRF = 2000 Hz.
```
