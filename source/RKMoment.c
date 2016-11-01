//
//  RKMoment.c
//  RadarKit
//
//  Created by Boon Leng Cheong on 9/20/15.
//
//

#include <RadarKit/RKMoment.h>

void *momentCore(void *);

#pragma mark -

void *momentCore(void *_in) {
    RKMomentWorker *me = (RKMomentWorker *)_in;
    RKMomentEngine *engine = me->parentEngine;

    int k;
    struct timeval t0, t1, t2;

    const int c = me->id;

    // Find the semaphore
    sem_t *sem = sem_open(me->semaphoreName, O_RDWR);
    if (sem == SEM_FAILED) {
        RKLog("Error. Unable to retrieve semaphore %d\n", c);
        return (void *)RKResultFailedToRetrieveSemaphore;
    };

    // Allocate local resources, use k to keep track of the total allocation
    double *busyPeriods, *fullPeriods;
    posix_memalign((void **)&busyPeriods, RKSIMDAlignSize, RKWorkerDutyCycleBufferSize * sizeof(double));
    posix_memalign((void **)&fullPeriods, RKSIMDAlignSize, RKWorkerDutyCycleBufferSize * sizeof(double));
    if (busyPeriods == NULL || fullPeriods == NULL) {
        RKLog("Error. Unable to allocate resources for duty cycle calculation\n");
        return (void *)RKResultFailedToAllocateDutyCycleBuffer;
    }
    k = 2 * RKWorkerDutyCycleBufferSize * sizeof(double);
    memset(busyPeriods, 0, RKWorkerDutyCycleBufferSize * sizeof(double));
    memset(fullPeriods, 0, RKWorkerDutyCycleBufferSize * sizeof(double));
    double allBusyPeriods = 0.0, allFullPeriods = 0.0;

    // Initiate a variable to store my name
    char name[20];
    if (rkGlobalParameters.showColor) {
        k = sprintf(name, "\033[3%dm", c % 7 + 1);
    } else {
        k = 0;
    }
    if (engine->coreCount > 9) {
        k += sprintf(name + k, "MC%02d", c);
    } else {
        k += sprintf(name + k, "MC%d", c);
    }
    if (rkGlobalParameters.showColor) {
        sprintf(name + k, "\033[0m");
    }

    float deltaAzimuth, deltaElevation;

    // Initialize some end-of-loop variables
    gettimeofday(&t0, NULL);
    gettimeofday(&t2, NULL);

    // The last index of the pulse buffer and the last index of the ray buffer
    uint32_t is, ie;
    uint32_t io = engine->rayBufferSize - engine->coreCount + c;

    // The latest index in the dutyCycle buffer
    int d0 = 0;

    // Log my initial state
    if (engine->verbose) {
        pthread_mutex_lock(&engine->coreMutex);
        RKLog(">%s started.  i0 = %d   mem = %s  tic = %d\n", name, io, RKIntegerToCommaStyleString(k), me->tic);
        pthread_mutex_unlock(&engine->coreMutex);
    }

    // Increase the tic once to indicate this processing core is created.
    me->tic++;

    //
    // Same as in RKPulseCompression.c
    //
    // free   busy       free   busy
    // .......|||||||||||.......|||||||||
    // t2 --- t1 --- t0/t2 --- t1 --- t0
    //        [ t0 - t1 ]
    // [    t0 - t2     ]
    //
    uint32_t tic = me->tic;

    while (engine->state == RKMomentEngineStateActive) {
        if (engine->useSemaphore) {
            #if defined(DEBUG_IQ)
            RKLog(">%s sem_wait()\n", coreName);
            #endif
            sem_wait(sem);
        } else {
            while (tic == me->tic && engine->state == RKMomentEngineStateActive) {
                usleep(1000);
            }
            tic = me->tic;
        }
        if (engine->state != RKMomentEngineStateActive) {
            break;
        }

        // Something happened
        gettimeofday(&t1, NULL);

        // Start of getting busy
        io = RKNextNModuloS(io, engine->coreCount, engine->rayBufferSize);
        me->lag = fmodf((float)(*engine->pulseIndex - me->pid + engine->pulseBufferSize) / engine->pulseBufferSize, 1.0f);

        // Start and end indices of the I/Q data
        is = engine->momentSource[io].origin;
        ie = RKNextNModuloS(is, engine->momentSource[io].length - 1, engine->pulseBufferSize);

        if (ie > engine->pulseBufferSize) {
            RKLog("is = %d   ie = %d   %d %d\n", is, ie, engine->momentSource[io].length - 1, engine->pulseBufferSize);
            exit(EXIT_FAILURE);
        }
        deltaAzimuth = engine->pulses[ie].header.azimuthDegrees - engine->pulses[is].header.azimuthDegrees;
        if (deltaAzimuth > 180.0f) {
            deltaAzimuth -= 360.0f;
        } else if (deltaAzimuth < -180.0f) {
            deltaAzimuth += 360.0f;
        }
        deltaAzimuth = fabsf(deltaAzimuth);
        deltaElevation = engine->pulses[ie].header.elevationDegrees - engine->pulses[is].header.elevationDegrees;
        if (deltaElevation > 180.0f) {
            deltaElevation -= 360.0f;
        } else if (deltaElevation < -180.0f) {
            deltaElevation += 360.0f;
        }
        deltaElevation = fabsf(deltaElevation);
        
        pthread_mutex_lock(&engine->coreMutex);
        RKLog("%s   %u   %u...%u   EL %.2f - %.2f ^ %.2f   AZ %.2f - %.2f ^ %.2f\n",
              name, io, is, ie,
              engine->pulses[is].header.elevationDegrees, engine->pulses[ie].header.elevationDegrees, deltaElevation,
              engine->pulses[is].header.azimuthDegrees, engine->pulses[ie].header.azimuthDegrees, deltaAzimuth);
        pthread_mutex_unlock(&engine->coreMutex);

        // Process each polarization separately and indepently
        usleep(3000);

        // ray->header.s |= ...
        me->pid = ie;
        
        // Done processing, get the time
        gettimeofday(&t0, NULL);

        // Drop the oldest reading, replace it, and add to the calculation
        allBusyPeriods -= busyPeriods[d0];
        allFullPeriods -= fullPeriods[d0];
        busyPeriods[d0] = RKTimevalDiff(t0, t1);
        fullPeriods[d0] = RKTimevalDiff(t0, t2);
        allBusyPeriods += busyPeriods[d0];
        allFullPeriods += fullPeriods[d0];
        d0 = RKNextModuloS(d0, RKWorkerDutyCycleBufferSize);
        me->dutyCycle = allBusyPeriods / allFullPeriods;

        t2 = t0;
    }

    free(busyPeriods);
    free(fullPeriods);

    RKLog(">%s ended.\n", name);

    return NULL;
}

void *pulseGatherer(void *_in) {
    RKMomentEngine *engine = (RKMomentEngine *)_in;

    int c, j, k;

    sem_t *sem[engine->coreCount];

    int skipCounter = 0;

    // Change the state to active so all the processing cores stay in the busy loop
    engine->state = RKMomentEngineStateActive;

    // Spin off N workers to process I/Q pulses
    for (c = 0; c < engine->coreCount; c++) {
        RKMomentWorker *worker = &engine->workers[c];
        snprintf(worker->semaphoreName, 16, "rk-mm-%03d", c);
        sem[c] = sem_open(worker->semaphoreName, O_CREAT | O_EXCL, 0600, 0);
        if (sem[c] == SEM_FAILED) {
            if (engine->verbose > 1) {
                RKLog("Info. Semaphore %s exists. Try to remove and recreate.\n", worker->semaphoreName);
            }
            if (sem_unlink(worker->semaphoreName)) {
                RKLog("Error. Unable to unlink semaphore %s.\n", worker->semaphoreName);
            }
            // 2nd trial
            sem[c] = sem_open(worker->semaphoreName, O_CREAT | O_EXCL, 0600, 0);
            if (sem[c] == SEM_FAILED) {
                RKLog("Error. Unable to remove then create semaphore %s\n", worker->semaphoreName);
                return (void *)RKResultFailedToInitiateSemaphore;
            } else if (engine->verbose > 1) {
                RKLog("Info. Semaphore %s removed and recreated.\n", worker->semaphoreName);
            }
        }
        worker->id = c;
        worker->parentEngine = engine;
        if (pthread_create(&worker->tid, NULL, momentCore, worker) != 0) {
            RKLog("Error. Failed to start a compression core.\n");
            return (void *)RKResultFailedToStartCompressionCore;
        }
    }

    // Wait for the workers to increase the tic count once
    // Using sem_wait here could cause a stolen post within the worker
    // See RKPulseCompression.c
    for (c = 0; c < engine->coreCount; c++) {
        while (engine->workers[c].tic == 0) {
            usleep(1000);
        }
    }

    // Increase the tic once to indicate the watcher is ready
    engine->tic++;

    // Beam index at t = 0 and t = 1 (previous sample)
    uint32_t i0, i1 = 0;
    uint32_t count = 0;

    // Here comes the busy loop
    j = 0;   // ray index
    k = 0;   // pulse index
    c = 0;   // core index
    RKLog("pulseGatherer() started.   c = %d   k = %d   engine->index = %d\n", c, k, *engine->pulseIndex);
    while (engine->state == RKMomentEngineStateActive) {
        // Wait until the engine index move to the next one for storage
        while (k == *engine->pulseIndex && engine->state == RKMomentEngineStateActive) {
            usleep(200);
            // Timeout and say "nothing" on the screen
        }
        RKPulse *pulse = &engine->pulses[k];
        while ((pulse->header.s & RKPulseStatusCompressed) == 0 && engine->state == RKMomentEngineStateActive) {
            usleep(200);
        }
        if (engine->state == RKMomentEngineStateActive) {

            // Gather the start and end pulses
            // Get the vacant ray
            // Post a worker to process for a ray

            i0 = floorf(pulse->header.azimuthDegrees);
            if (i1 != i0) {
                i1 = i0;
                // Inclusive count, the end pulse is used on both rays
                engine->momentSource[j].length = count + 1;

                // Assess the buffer fullness
                if (c == 0 && skipCounter == 0 &&  engine->workers[c].lag > 0.9f) {
                    engine->almostFull++;
                    skipCounter = engine->pulseBufferSize;
                    RKLog("Warning. I/Q Buffer overflow detected by pulseGatherer().\n");
                }

                if (skipCounter > 0) {
                    skipCounter--;
                } else {
                    if (count > 0) {
                        if (engine->useSemaphore) {
                            sem_post(sem[c]);
                        } else {
                            engine->workers[c].tic++;
                        }
                        c = RKNextModuloS(c, engine->coreCount);
                        // Start of ray
                        j = RKNextModuloS(j, engine->rayBufferSize);
                        engine->momentSource[j].origin = k;
                    }
                    count = 0;
                }
            }
            // Keep counting up
            count++;
        }
        // Update k to catch up for the next watch
        k = RKNextModuloS(k, engine->pulseBufferSize);
    }

    // Wait for workers to return
    for (c = 0; c < engine->coreCount; c++) {
        RKMomentWorker *worker = &engine->workers[c];
        if (engine->useSemaphore) {
            sem_post(sem[c]);
        }
        pthread_join(worker->tid, NULL);
        sem_unlink(worker->semaphoreName);
    }

    return NULL;
}

#pragma mark -

RKMomentEngine *RKMomentEngineInit(void) {
    RKMomentEngine *engine = (RKMomentEngine *)malloc(sizeof(RKMomentEngine));
    if (engine == NULL) {
        RKLog("Error. Unable to allocate a momment engine.\n");
        return NULL;
    }
    memset(engine, 0, sizeof(RKMomentEngine));
    engine->state = RKMomentEngineStateAllocated;
    engine->verbose = 1;
    engine->useSemaphore = true;
    pthread_mutex_init(&engine->coreMutex, NULL);
    return engine;
}

void RKMomentEngineFree(RKMomentEngine *engine) {
    if (engine->state == RKMomentEngineStateActive) {
        RKMomentEngineStop(engine);
    }
    free(engine->momentSource);
    free(engine);
}

void RKMomentEngineSetInputOutputBuffers(RKMomentEngine *engine,
                                         RKPulse *pulses,
                                         uint32_t *pulseIndex,
                                         const uint32_t pulseBufferSize,
                                         RKFloatRay *rays,
                                         uint32_t *rayIndex,
                                         const uint32_t rayBufferSize) {
    engine->pulses = pulses;
    engine->pulseIndex = pulseIndex;
    engine->pulseBufferSize = pulseBufferSize;
    engine->rawRays = rays;
    engine->rayIndex = rayIndex;
    engine->rayBufferSize = rayBufferSize;
    engine->encodedRays = NULL;
    engine->momentSource = (RKMomentSource *)malloc(rayBufferSize * sizeof(RKMomentSource));
    if (engine->momentSource == NULL) {
        RKLog("Error. Unable to allocate momentSource.\n");
        exit(EXIT_FAILURE);
    }
    memset(engine->momentSource, 0, rayBufferSize * sizeof(RKMomentSource));
}

void RKMomentEngineSetCoreCount(RKMomentEngine *engine, const unsigned int count) {
    if (engine->state == RKMomentEngineStateActive) {
        RKLog("Error. Core count cannot be changed when the engine is active.\n");
        return;
    }
    engine->coreCount = count;
}

int RKMomentEngineStart(RKMomentEngine *engine) {
    engine->state = RKMomentEngineStateActivating;
    if (engine->coreCount == 0) {
        engine->coreCount = 4;
    }
    if (engine->workers != NULL) {
        RKLog("Error. RKMomentEngine->workers should be NULL here.\n");
    }
    engine->workers = (RKMomentWorker *)malloc(engine->coreCount * sizeof(RKMomentWorker));
    memset(engine->workers, 0, engine->coreCount * sizeof(RKMomentWorker));
    if (engine->verbose) {
        RKLog("Starting pulseGatherer() ...\n");
    }
    if (pthread_create(&engine->tidPulseGatherer, NULL, pulseGatherer, engine) != 0) {
        RKLog("Error. Failed to start a pulse watcher.\n");
        return RKResultFailedToStartPulseGatherer;
    }
    while (engine->tic == 0) {
        usleep(1000);
    }

    return RKResultNoError;
}

int RKMomentEngineStop(RKMomentEngine *engine) {
    if (engine->state != RKMomentEngineStateActive) {
        if (engine->verbose > 1) {
            RKLog("Info. Pulse compression engine is being or has been deactivated.\n");
        }
        return RKResultEngineDeactivatedMultipleTimes;
    }
    engine->state = RKMomentEngineStateDeactivating;
    pthread_join(engine->tidPulseGatherer, NULL);
    if (engine->verbose) {
        RKLog("pulseGatherer() ended\n");
    }
    free(engine->workers);
    engine->workers = NULL;
    engine->state = RKMomentEngineStateNull;
    return RKResultNoError;
}
