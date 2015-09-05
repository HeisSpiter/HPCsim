/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim
 * FILE:             HPCsim/main.cpp
 * PURPOSE:          Main simulation file
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <cassert>

#include "TThreadsFactory.h"
#include "RngStream.h"
#include "simulation.h"

#define PATH_MAX 0x1000

#define DEFAULT_NAME {'H', 'P', 'C', 's', 'i', 'm', '.', 'o', 'u', 't', '\0'}

struct TSimulationAddresses
{
    TSimulationInit fSimulationInit;
    TRunInit fRunInit;
    TEventInit fEventInit;
    TEventRun fEventRun;
    TEventClear fEventClear;
    TRunClear fRunClear;
    TSimulationUnload fSimulationUnload;
};

#ifdef USE_PILOT_THREAD
struct TPilotJobContext
{
    unsigned long fEvents;
    void * fSimulationContext;
};
#endif

static pthread_mutex_t gPipeLock;
static int gPipe[2];
static TResult gNullResult;
static TSimulationAddresses gSimulation;

#define LoadAndSetSimulationFunction(name)                      \
    gSimulation.f##name = (T##name)dlsym(simulationLib, #name); \
    if (gSimulation.f##name == 0) {                             \
        std::cerr << "Failed loading " << #name << std::endl;   \
        goto end;                                               \
    }

/* Exported */
double RandU01(void * context)
{
    RngStream * rand = reinterpret_cast<RngStream *>(context);

    return rand->RandU01();
}

/* Exported */
void QueueResult(TResult * result, void * context)
{
    RngStream * rand = reinterpret_cast<RngStream *>(context);

    /* Set our ID first, and send to write thread */
    memcpy(result->fId, rand->GetDigest(), sizeof(TResult::fId));
    pthread_mutex_lock(&gPipeLock);
    write(gPipe[1], result, sizeof(TResult));
    pthread_mutex_unlock(&gPipeLock);
}

static void * SimulationLoop(void * Arg)
{
#ifdef USE_PILOT_THREAD
    TPilotJobContext * context = reinterpret_cast<TPilotJobContext *>(Arg);
    void * simulationContext = context->fSimulationContext;

    for (unsigned long event = 0; event < context->fEvents; ++event)
#else
    void * simulationContext = reinterpret_cast<void *>(Arg);
#endif
    {
        void * eventContext;
        RngStream rand;

        /* Init the event */
        if (gSimulation.fEventInit(simulationContext, &rand, &eventContext) < 0)
        {
#ifdef USE_PILOT_THREAD
            continue;
#else
            sem_post(TThreadsFactory::GetInstance()->GetInitLock());
            return 0;
#endif
        }

        /* Release init lock */
        sem_post(TThreadsFactory::GetInstance()->GetInitLock());

        /* Call the simulation */
        gSimulation.fEventRun(simulationContext, eventContext);

#ifdef USE_PILOT_THREAD
        sem_wait(TThreadsFactory::GetInstance()->GetInitLock());
#endif

        /* Notify end of event */
        gSimulation.fEventClear(simulationContext, eventContext);
    }

#ifdef USE_PILOT_THREAD
    sem_post(TThreadsFactory::GetInstance()->GetInitLock());
#endif

    return 0;
}

static void * WriteResults(void * Arg)
{
    TResult result;
    char * outputFile = reinterpret_cast<char *>(Arg);
    int outFD;

    /* Open the output file */
    outFD = open(outputFile, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    assert(outFD != -1);

    /* Read the incoming event */
    while (read(gPipe[0], &result, sizeof(TResult)) > 0)
    {
        /* End signal => stop dealing with data */
        if (memcmp(&result, &gNullResult, sizeof(TResult)) == 0)
            break;

        /* Write the event to the output file */
        write(outFD, &result, sizeof(TResult));
    }

    close(outFD);

    return 0;
}

static void PrintUsage(char * name)
{
    std::cerr << "Usage: " << name << " --simulation|-s name.so [--threads|-t X --first|-f X --events|-e X --output|-o name]" << std::endl;
    std::cerr << "\t- Simulation: path of the shared library containing the simulation" << std::endl;
    std::cerr << "\t- Threads: amount of threads to use for computing (min 1). Beware an extra thread will be used for results writing" << std::endl;
    std::cerr << "\t- First: start the event loop at this event" << std::endl;
    std::cerr << "\t- Events: number of events to compute" << std::endl;
    std::cerr << "\t- Output: name of the output file to write" << std::endl;
}

int main(int argc, char * argv[])
{
    int option;
    unsigned int nThreads = 1;
    unsigned long nEvents = 100;
    unsigned long firstEvent = 0;
    char outputFile[PATH_MAX] = DEFAULT_NAME;
    char simulationFile[PATH_MAX] = "";
    pthread_t writingThread;
    void * ret;
    void * simulationLib;
    void * simulationContext;
#ifdef USE_PILOT_THREAD
    unsigned long eventsPerThread = 0;
    unsigned long padding = 0;
    TPilotJobContext * contexts = 0;
    TPilotJobContext * context = 0;
#endif

    /* Parse arguments */
    while (true)
    {
        static struct option long_options[] =
        {
            {"threads", required_argument, 0, 't'},
            {"events", required_argument, 0, 'e'},
            {"output", required_argument, 0, 'o'},
            {"first", required_argument, 0, 'f'},
            {"simulation", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        option = getopt_long(argc, argv, "e:t:o:f:s:", long_options, &option_index);
        if (option == -1)
            break;

        switch (option)
        {
            static bool written = false;

            case 't':
                nThreads = strtoul(optarg, 0, 10);
                break;

            case 'e':
                nEvents = strtoul(optarg, 0, 10);
                break;

            case 'o':
                strncpy(outputFile, optarg, PATH_MAX - 1);
                outputFile[PATH_MAX - 1] = '\0';
                break;

            case 'f':
                firstEvent = strtoul(optarg, 0, 10);
                break;

            case 's':
                strncpy(simulationFile, optarg, PATH_MAX - 1);
                simulationFile[PATH_MAX - 1] = '\0';
                break;

            case '?':
                if (!written)
                {
                    PrintUsage(argv[0]);
                    written = true;
                }
                break;
        }
    }

    if (simulationFile[0] == '\0')
    {
        PrintUsage(argv[0]);
        return 0;
    }

    /* Open the simulation worker */
    simulationLib = dlopen(simulationFile, RTLD_NOW | RTLD_LOCAL);
    if (simulationLib == 0)
    {
        std::cerr << "Failed loading " << simulationLib << std::endl;
        return 0;
    }

    /* Load its functions */
    LoadAndSetSimulationFunction(SimulationInit);
    LoadAndSetSimulationFunction(RunInit);
    LoadAndSetSimulationFunction(EventInit);
    LoadAndSetSimulationFunction(EventRun);
    LoadAndSetSimulationFunction(EventClear);
    LoadAndSetSimulationFunction(RunClear);
    LoadAndSetSimulationFunction(SimulationUnload);

    /* Init the simulation */
    if (gSimulation.fSimulationInit(nThreads, nEvents, firstEvent, &simulationContext) < 0)
    {
        std::cerr << "Failed initializing library" << std::endl;
        goto end;
    }

    /* Advance in the generator */
    for (unsigned long i = 0; i < firstEvent; ++i)
    {
        RngStream rand;
    }

    /* Initialize our pipe lock */
    pthread_mutex_init(&gPipeLock, 0);
    /* Start our threads factory */
    TThreadsFactory::GetInstance()->SetMaxThreads(nThreads);
    /* Init our null event */
    memset(&gNullResult, 0, sizeof(TResult));

    /* Open the communication pipe */
    if (pipe(gPipe) == -1)
    {
        std::cerr << "Failed creating pipes" << std::endl;
        goto end2;
    }

    /* Start our background writing thread */
    if (pthread_create(&writingThread, 0, WriteResults, outputFile) != 0)
    {
        std::cerr << "Failed creating writing thread" << std::endl;
        goto end3;
    }

    /* Initialize the run */
    if (gSimulation.fRunInit(simulationContext) < 0)
    {
        std::cerr << "Failed initializing run" << std::endl;
        goto end4;
    }

#ifndef USE_PILOT_THREAD
    /* Hot loop, the simulation happens here */
    for (unsigned long event = 0; event < nEvents; ++event)
    {
        TThreadsFactory::GetInstance()->CreateThread(SimulationLoop, simulationContext);
    }
#else
    /* Alloc once, use multipe times - reduce overhead */
    contexts = new TPilotJobContext[nThreads];

    /* Compute number of events required per thread */
    eventsPerThread = nEvents / nThreads;
    /* Compute number of threads where we have to +1 number of events to get exact amount */
    padding = nEvents % nThreads;
    /* Start at first event */
    context = contexts;

    for (unsigned long thread = 0; thread < nThreads; ++thread)
    {
        context->fSimulationContext = simulationContext;
        context->fEvents = eventsPerThread + ((thread < padding) ? 1 : 0);
        TThreadsFactory::GetInstance()->CreateThread(simulationContext, context);

        ++context;
    }
#endif

    /* Wait for all the propagations to finish */
    TThreadsFactory::GetInstance()->WaitForAllThreads();

#ifdef USE_PILOT_THREAD
    delete contexts;
#endif

    /* Signal end of run */
    gSimulation.fRunClear(simulationContext);

end4:
    /* Send the end signal to writer */
    write(gPipe[1], &gNullResult, sizeof(TResult));

    /* Wait for the end of the writer */
    pthread_join(writingThread, &ret);

end3:
    close(gPipe[0]);
    close(gPipe[1]);
end2:
    pthread_mutex_destroy(&gPipeLock);
    gSimulation.fSimulationUnload(simulationContext);
end:
    dlclose(simulationLib);
    return 0;
}
