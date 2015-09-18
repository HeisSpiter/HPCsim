/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          PI simulation
 * FILE:             examples/Pi/pi.c
 * PURPOSE:          Main simulation file
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <stdlib.h>
#include "simulation.h"

int SimulationInit(unsigned int nThreads, unsigned long nEvents, unsigned long firstEvent, void ** simContext)
{
    /* Nothing to do */
    return 0;
}

int RunInit(void * simContext)
{
    /* Nothing to do */
    return 0;
}

#ifdef USE_PILOT_THREAD
int PilotInit(void * simContext, void ** pilotContext)
{
    /* Nothing to do */
    return 0;
}
#endif

#ifdef USE_PILOT_THREAD
int EventInit(void * simContext, void * pilotContext, void * rand, void ** eventContext)
#else
int EventInit(void * simContext, void * rand, void ** eventContext)
#endif
{
    /* Only save the rand pointer */
    *eventContext = rand;

    return 0;
}

#ifdef USE_PILOT_THREAD
void EventRun(void * simContext, void * pilotContext, void * eventContext)
#else
void EventRun(void * simContext, void * eventContext)
#endif
{
    double total, inside;
    TResult result;

    /* We'll compute 10,000 values on each thread */
    for (total = 0, inside = 0; total < 10000; ++total)
    {
        double x = RandU01(eventContext);
        double y = RandU01(eventContext);

        if (x * x + y * y < 1.0)
        {
            ++inside;
        } 
    }

    /* We'll return 2 unsigned long */
    result.fResultLength = 2 * sizeof(double);

    /* First, total and then inside */
    ((double *)result.fResult)[0] = total;
    ((double *)result.fResult)[1] = inside;

    /* Write the result */
    QueueResult(&result, eventContext);

    return;
}

#ifdef USE_PILOT_THREAD
void EventClear(void * simContext, void * pilotContext, void * eventContext)
#else
void EventClear(void * simContext, void * eventContext)
#endif
{
    /* Nothing to do */
    return;
}

void PilotClear(void * simContext, void * pilotContext)
{
    /* Nothing to do */
    return;
}

void RunClear(void * simContext)
{
    /* Nothing to do */
    return;
}

void SimulationUnload(void * simContext)
{
    /* Nothing to do */
    return;
}
