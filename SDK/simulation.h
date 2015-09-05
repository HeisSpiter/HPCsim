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

typedef int (* TSimulationInit)(unsigned int, unsigned long, unsigned long, void **);
typedef int (* TRunInit)(void *);
typedef int (* TEventInit)(void *, void *, void **);
typedef void (* TEventRun)(void *, void *);
typedef void (* TEventClear)(void *, void *);
typedef void (* TRunClear)(void *);
typedef void (* TSimulationUnload)(void *);

#ifndef SHA384_DIGEST_LENGTH
#define SHA384_DIGEST_LENGTH 48
#endif

typedef struct TResult
{
    unsigned char fId[SHA384_DIGEST_LENGTH];
    unsigned short fResultLength;
    unsigned char fResult[0x800];
} TResult;

double RandU01(void *);
void QueueResult(TResult *, void *);

#ifdef __cplusplus
}
#endif
