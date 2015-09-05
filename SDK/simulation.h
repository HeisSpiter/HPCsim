/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim SDK
 * FILE:             SDK/simulation.h
 * PURPOSE:          Simulation header
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

typedef int (* TSimulationInit)(void **);
typedef int (* TRunInit)(void *);
typedef int (* TEventInit)(void *, void *);
typedef void (* TEventRun)(void *);
typedef void (* TEventClear)(void *);
typedef void (* TRunClear)(void *);
typedef void (* TSimulationUnload)(void *);

typedef struct TResult
{
    unsigned char fId[SHA384_DIGEST_LENGTH];
    unsigned short fResultLength;
    unsigned char fResult[0x800];
} TResult;

double RandU01(void *);
void QueueResult(TResult *, void *);
