/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim SDK
 * FILE:             SDK/simulation.h
 * PURPOSE:          Simulation header
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Called right after the simulation has been loaded.
 * @param nThreads Number of concurrent threads that will be created
 * @param nEvents Total number of events proceed
 * @param firstEvent The number of events skipped
 * @param context Output variable. The user can allocate memory that will be passed to any further call
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TSimulationInit)(unsigned int nThreads, unsigned long nEvents, unsigned long firstEvent, void ** context);
/**
 * Called right before the event loop.
 * @param context The allocated buffer during SimulationInit()
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TRunInit)(void * context);
/**
 * Called right before the event run starts. There's only one call to EventInit() at a time (no concurrency).
 * As such, for performances reasons, keep it as short as possible.
 * @param context The allocated buffer during SimulationInit()
 * @param rand Random context, to be used with RandU01() and QueueResult()
 * @param eventContext Output variable. The user can allocate memory that will be passed to any further call to event function
 * @return -1 in case of error, 0 otherwise
 */
typedef int (* TEventInit)(void * context, void * rand, void ** eventContext);
/**
 * This is the worker routine for the simulation. Several can run in parallel (only one per event though).
 * @param context The allocated buffer during SimulationInit()
 * @param eventContext The allocated buffer during EventInit()
 */
typedef void (* TEventRun)(void * context, void * eventContext);
/**
 * Called right after the event run, use it to cleanup your event related stuff (such as the event context)
 * @param context The allocated buffer during SimulationInit()
 * @param eventContext The allocated buffer during EventInit()
 */
typedef void (* TEventClear)(void * context, void * eventContext);
/**
 * Called right after the run finishes (after the last event was proceed)
 * @param context The allocated buffer during SimulationInit()
 */
typedef void (* TRunClear)(void * context);
/**
 * Called before HPCsim end. Use it to deallocate anything you would have allocated.
 * @param context The allocated buffer during SimulationInit()
 */
typedef void (* TSimulationUnload)(void * context);

#ifndef SHA384_DIGEST_LENGTH
#define SHA384_DIGEST_LENGTH 48
#endif

typedef struct TResult
{
    unsigned char fId[SHA384_DIGEST_LENGTH];
    unsigned short fResultLength;
    unsigned char fResult[0x800];
} TResult;

/**
 * Exported function for the user. It allows drawing a PRN in the context
 * of an event.
 * @param rand Random context, received on the EventInit() call.
 * @return a number between 0 & 1, uniformely distributed.
 */
double RandU01(void * rand);
/**
 * Exported function for the user. It allows queueing a result for defered
 * writing.
 * @param result The result to write. fId isn't to be completed by the user.
 * @param rand The event context, as received on the EventInit() call.
 */
void QueueResult(TResult * result, void * rand);

#ifdef __cplusplus
}
#endif
