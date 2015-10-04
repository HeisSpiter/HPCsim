/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          PI simulation
 * FILE:             examples/PiReduce/reduce.c
 * PURPOSE:          Routines for reduce support
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <stdlib.h>
#include <stdio.h>
#include "simulation.h"

typedef struct TContext
{
    double fTotal;
    double fInside;
} TContext;

/* Sanity check for our entry points */
TSimulationInit SimulationInit;
TReduceResult ReduceResult;
TSimulationUnload SimulationUnload;

int SimulationInit(unsigned char isPilot, unsigned int nThreads, unsigned long nEvents, unsigned long firstEvent, void ** simContext)
{
    TContext * context;

    UNUSED_PARAMETER(nThreads);
    UNUSED_PARAMETER(nEvents);
    UNUSED_PARAMETER(firstEvent);

    /* Check we're running in the context we where built for */
#ifdef USE_PILOT_THREAD
    if (!isPilot)
        return -1;
#else
    if (isPilot)
        return -1;
#endif

    /* Allocate our own context */
    context = malloc(sizeof(TContext));
    if (context == NULL)
    {
        return -1;
    }

    /* Init it */
    context->fTotal = 0.;
    context->fInside = 0;

    /* And return it */
    *simContext = context;

    return 0;
}

void ReduceResult(void * simContext, char const * outputFile, void const * id, uint32_t resultLength, void const * result)
{
    TContext * context = simContext;

    UNUSED_PARAMETER(outputFile);
    UNUSED_PARAMETER(id);

    /* Validate input size */
    if (resultLength != 2 * sizeof(double))
    {
        return;
    }

    /* Increment counters */
    context->fTotal += ((double *)result)[0];
    context->fInside += ((double *)result)[1];
}

void SimulationUnload(void * simContext)
{
    TContext * context = simContext;

    /* Compute PI for real */
    printf("Pi: %lf (with %lf samples)\n", (4.0 * context->fInside) / context->fTotal, context->fTotal);
}
