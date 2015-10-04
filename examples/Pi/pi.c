/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          PI simulation
 * FILE:             examples/Pi/pi.c
 * PURPOSE:          Main simulation file
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <stdlib.h>
#include "simulation.h"

/* Sanity check for our entry points */
TSimulationInit SimulationInit;
TEventRun EventRun;

/* Reduce comes with its own init implementation */
#ifndef BUILD_WITH_REDUCE
int SimulationInit(unsigned char isPilot, unsigned int nThreads, unsigned long nEvents, unsigned long firstEvent, void ** simContext)
{
    UNUSED_PARAMETER(nThreads);
    UNUSED_PARAMETER(nEvents);
    UNUSED_PARAMETER(firstEvent);
    UNUSED_PARAMETER(simContext);

    /* Check we're running in the context we where built for */
#ifdef USE_PILOT_THREAD
    if (!isPilot)
        return -1;

    return 0;
#else
    if (isPilot)
        return -1;

    return 0;
#endif
}
#endif

#ifdef USE_PILOT_THREAD
void EventRun(void * simContext, void * pilotContext, void * eventContext)
#else
void EventRun(void * simContext, void * eventContext)
#endif
{
    double total, inside;
    TResult result;
    double * resultBuffer;

    UNUSED_PARAMETER(simContext);
#ifdef USE_PILOT_THREAD
    UNUSED_PARAMETER(pilotContext);
#endif
    UNUSED_PARAMETER(eventContext);

    /* We'll compute 10,000 values on each thread */
    for (total = 0, inside = 0; total < 10000; ++total)
    {
        double x = RandU01();
        double y = RandU01();

        if (x * x + y * y < 1.0)
        {
            ++inside;
        } 
    }

    /* We'll return 2 unsigned long */
    result.fResultLength = 2 * sizeof(double);

    /* First, total and then inside */
    resultBuffer = (double *)&result.fResult[0];
    resultBuffer[0] = total;
    resultBuffer[1] = inside;

    /* Write the result */
    QueueResult(&result);

    return;
}
