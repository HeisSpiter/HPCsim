/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim SDK
 * FILE:             SDK/simulation.h
 * PURPOSE:          Simulation header
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#ifdef HAVE_STDINT_H
#ifdef __cpluplus
#include <cstdint>
#else
#include <stdint.h>
#endif
#else
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Called right after the simulation has been loaded.
 * @param isPilot Set to 1 if HPCsim was built with USE_PILOT_THREAD, to 0 otherwise
 * @param nThreads Number of concurrent threads that will be created
 * @param nEvents Total number of events proceed
 * @param firstEvent The number of events skipped
 * @param simContext Output variable. The user can allocate memory that will be passed to any further call
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TSimulationInit)(unsigned char isPilot, unsigned int nThreads, unsigned long nEvents, unsigned long firstEvent, void ** simContext);
/**
 * Called right before the event loop.
 * @param simContext The allocated buffer during SimulationInit()
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TRunInit)(void * simContext);
#ifdef USE_PILOT_THREAD
/**
 * Called right after the pilot job was created. There's only one call to PilotInit() at a time (no concurrency).
 * As such, for performances reasons, keep it as short as possible.
 * @param simContext The allocated buffer during SimulationInit()
 * @param pilotContext Output variable. The user can allocate memory that will be passed to any further call to event function
 */
typedef int (* TPilotInit)(void * simContext, void ** pilotContext);
/**
 * Called right before the event run starts. There's only one call to EventInit() at a time (no concurrency).
 * As such, for performances reasons, keep it as short as possible.
 * @param simContext The allocated buffer during SimulationInit()
 * @param pilotContext The allocated buffer during PilotInit()
 * @param eventContext Output variable. The user can allocate memory that will be passed to any further call to event function
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TEventInit)(void * simContext, void * pilotContext, void ** eventContext);
/**
 * This is the worker routine for the simulation. Several can run in parallel (only one per event though).
 * @param simContext The allocated buffer during SimulationInit()
 * @param pilotContext The allocated buffer during PilotInit()
 * @param eventContext The allocated buffer during EventInit()
 */
typedef void (* TEventRun)(void * simContext, void * pilotContext, void * eventContext);
/**
 * Called right after the event run, use it to cleanup your event related stuff (such as the event context)
 * @param simContext The allocated buffer during SimulationInit()
 * @param pilotContext The allocated buffer during PilotInit()
 * @param eventContext The allocated buffer during EventInit()
 */
typedef void (* TEventClear)(void * simContext, void * pilotContext, void * eventContext);
/**
 * Called after the last event of the pilot job was cleared, use it to cleanup your pilot job related stuff (such as the pilot context)
 * @param simContext The allocated buffer during SimulationInit()
 * @param pilotContext The allocated buffer during PilotInit()
 */
typedef void (* TPilotClear)(void * simContext, void * pilotContext);
#else
/**
 * Called right before the event run starts. There's only one call to EventInit() at a time (no concurrency).
 * As such, for performances reasons, keep it as short as possible.
 * @param simContext The allocated buffer during SimulationInit()
 * @param eventContext Output variable. The user can allocate memory that will be passed to any further call to event function
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TEventInit)(void * simContext, void ** eventContext);
/**
 * This is the worker routine for the simulation. Several can run in parallel (only one per event though).
 * @param simContext The allocated buffer during SimulationInit()
 * @param eventContext The allocated buffer during EventInit()
 */
typedef void (* TEventRun)(void * simContext, void * eventContext);
/**
 * Called right after the event run, use it to cleanup your event related stuff (such as the event context)
 * @param simContext The allocated buffer during SimulationInit()
 * @param eventContext The allocated buffer during EventInit()
 */
typedef void (* TEventClear)(void * simContext, void * eventContext);
#endif
/**
 * Called right after the run finishes (after the last event was proceed)
 * @param simContext The allocated buffer during SimulationInit()
 */
typedef void (* TRunClear)(void * simContext);
/**
 * Called before HPCsim end. Use it to deallocate anything you would have allocated.
 * @param simContext The allocated buffer during SimulationInit()
 */
typedef void (* TSimulationUnload)(void * simContext);

#ifndef SHA384_DIGEST_LENGTH
#define SHA384_DIGEST_LENGTH 48
#endif

typedef struct TResult
{
    uint8_t fId[SHA384_DIGEST_LENGTH];
    uint32_t fResultLength;
    uint8_t fResult[0x800];
} TResult;

/**
 * Exported function for the user. It allows drawing a PRN in the context
 * of an event, using the pseudo-random stream associated to the event.
 * You cannot (and have not to) call it outside an event run. It can only be
 * called during EventInit(), EventRun(), EventClear().
 * @return a number between 0 & 1, uniformely distributed.
 */
double RandU01(void);
/**
 * Exported function for the user. It allows queueing a result for defered
 * writing. It will be written with the ID associated to the current event.
 * You cannot (and have not to) call it outside an event run. It can only be
 * called during EventInit(), EventRun(), EventClear().
 * @param result The result to write. fId isn't to be completed by the user.
 */
void QueueResult(TResult * result);

#ifdef __cplusplus
}
#endif

#endif
