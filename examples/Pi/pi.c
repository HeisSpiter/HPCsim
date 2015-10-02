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

int SimulationInit(unsigned char isPilot, unsigned int nThreads, unsigned long nEvents, unsigned long firstEvent, void ** simContext)
{
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
    ((double *)result.fResult)[0] = total;
    ((double *)result.fResult)[1] = inside;

    /* Write the result */
    QueueResult(&result);

    return;
}
