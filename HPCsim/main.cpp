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
#include <cstddef>

#include "Exceptions.h"
#include "TThreadsFactory.h"
#include "RngStream.h"
#include "simulation.h"

#define PATH_MAX 0x1000

#define DEFAULT_NAME {'H', 'P', 'C', 's', 'i', 'm', '.', 'o', 'u', 't', '\0'}

struct TSimulationClass
{
    TSimulationInit * fSimulationInit;
    TRunInit * fRunInit;
#ifdef USE_PILOT_THREAD
    TPilotInit * fPilotInit;
#endif
    TEventInit * fEventInit;
    TEventRun * fEventRun;
    TEventClear * fEventClear;
#ifdef USE_PILOT_THREAD
    TPilotClear * fPilotClear;
#endif
    TReduceResult * fReduceResult;
    TRunClear * fRunClear;
    TSimulationUnload * fSimulationUnload;

    bool fCheckPoint;
    void * fSimulationContext;
};

#ifdef USE_PILOT_THREAD
struct TPilotJobContext
{
    unsigned long fEvents;
};
#endif

static pthread_mutex_t gPipeLock;
static int gPipe[2];
static TResult gNullResult;
static TSimulationClass gSimulation;
#ifdef USE_PILOT_THREAD
static const unsigned char gUsingPilot = 1;
#else
static const unsigned char gUsingPilot = 0;
#endif
static __thread RngStream * tRand = 0;

#define LoadAndSetSimulationFunction(name)                       \
    *(void **)&gSimulation.f##name = dlsym(simulationLib, #name)

/* Exported */
extern "C" double RandU01(void)
{
    return tRand->RandU01();
}

/* Exported */
extern "C" void QueueResult(TResult * result)
{
    /* Set our ID first, and send to write thread */
    memcpy(result->fId, tRand->GetDigest(), sizeof(TResult::fId));
    pthread_mutex_lock(&gPipeLock);
    /* Note regarding valgrind: valgrind will complain because of a call to write()
     * referencing non-initialized memory. This is to be expected: the TResult is allocated
     * from stack and is only initialized with meaningful data. Furthermore, when going to
     * disk, only relevant bits will be written. So, for performances reasons, we don't zero
     * the stack object, and tolerate valgrind complain.
     */
    UNUSED_RETURN(write(gPipe[1], result, sizeof(TResult)));
    pthread_mutex_unlock(&gPipeLock);
}

static void * SimulationLoop(void * Arg)
{
#ifdef USE_PILOT_THREAD
    TPilotJobContext * context = reinterpret_cast<TPilotJobContext *>(Arg);
    void * pilotContext = 0;

    /* Init the pilot */
    if (gSimulation.fPilotInit != 0)
    {
        HPCSIM_TRY
        {
            if (gSimulation.fPilotInit(gSimulation.fSimulationContext, &pilotContext) < 0)
            {
                HPCSIM_THROW;
            }
        }
        HPCSIM_EXCEPT
        {
            sem_post(TThreadsFactory::GetInstance()->GetInitLock());
            return 0;
        }
        HPCSIM_END
    }

    for (volatile unsigned long event = 0; event < context->fEvents; ++event)
#else
    UNUSED_PARAMETER(Arg);
#endif
    {
        void * eventContext = 0;
        RngStream rand;

        tRand = &rand;
        /* Init the event */
        if (gSimulation.fEventInit != 0)
        {
            HPCSIM_TRY
            {
#ifdef USE_PILOT_THREAD
                if (gSimulation.fEventInit(gSimulation.fSimulationContext, pilotContext, &eventContext) < 0)
#else
                if (gSimulation.fEventInit(gSimulation.fSimulationContext, &eventContext) < 0)
#endif
                {
                    HPCSIM_THROW;
                }
           }
           HPCSIM_EXCEPT
           {
#ifdef USE_PILOT_THREAD
                continue;
#else
                sem_post(TThreadsFactory::GetInstance()->GetInitLock());
                return 0;
#endif
            }
            HPCSIM_END
        }

        /* Release init lock */
        sem_post(TThreadsFactory::GetInstance()->GetInitLock());

        /* Call the simulation */
        HPCSIM_TRY
        {
#ifdef USE_PILOT_THREAD
            gSimulation.fEventRun(gSimulation.fSimulationContext, pilotContext, eventContext);
#else
            gSimulation.fEventRun(gSimulation.fSimulationContext, eventContext);
#endif
        }
        HPCSIM_EXCEPT
        {
#ifdef USE_PILOT_THREAD
            break;
#else
            return 0;
#endif
        }
        HPCSIM_END

        /* Notify end of event */
        if (gSimulation.fEventClear != 0)
        {
            HPCSIM_TRY
            {
#ifdef USE_PILOT_THREAD
                gSimulation.fEventClear(gSimulation.fSimulationContext, pilotContext, eventContext);
#else
                gSimulation.fEventClear(gSimulation.fSimulationContext, eventContext);
#endif
            }
            HPCSIM_EXCEPT
            {
#ifdef USE_PILOT_THREAD
                break;
#else
                return 0;
#endif
            }
            HPCSIM_END
        }

        tRand = 0;

#ifdef USE_PILOT_THREAD
        sem_wait(TThreadsFactory::GetInstance()->GetInitLock());
#endif
    }

#ifdef USE_PILOT_THREAD
    if (gSimulation.fPilotClear != 0)
    {
        HPCSIM_TRY
        {
            gSimulation.fPilotClear(gSimulation.fSimulationContext, pilotContext);
        }
        HPCSIM_END
    }

    sem_post(TThreadsFactory::GetInstance()->GetInitLock());
#endif

    return 0;
}

static void * WriteResults(void * Arg)
{
    TResult result;
    char * outputFile = reinterpret_cast<char *>(Arg);

#define LOOP_FOR_EVENTS(f)                                        \
    while ((read(gPipe[0], &result, sizeof(TResult)) > 0) &&      \
           (memcmp(&result, &gNullResult, sizeof(TResult)) != 0)) \
        f

    /* For performances reason (compiler optimisation, distinguish the two cases) */
    if (gSimulation.fReduceResult == 0)
    {
        int outFD;

        /* Whatever happens, we create if needed, and we want to write */
        int flags = O_WRONLY | O_CREAT;

        /* In case we are in checkpoint mode, just append at the end of file,
         * otherwise, erase any content
         */
        if (!gSimulation.fCheckPoint)
        {
            flags |= O_TRUNC;
        }

        /* Open the output file */
        outFD = open(outputFile, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        assert(outFD != -1);

        /* Go to the end of the file if checkpoint */
        if (gSimulation.fCheckPoint)
        {
            lseek(outFD, 0, SEEK_END);
        }

        /* Read the incoming event and write only what's needed to the output file.
         * This loop will end on end signal event
         */
        LOOP_FOR_EVENTS(UNUSED_RETURN(write(outFD, &result, offsetof(TResult, fResult) + result.fResultLength)));

        close(outFD);
    }
    else
    {
        /* Read the incoming event and pass it to the simulation
         * This loop will end on end signal event
         */
        HPCSIM_TRY
        {
            LOOP_FOR_EVENTS(gSimulation.fReduceResult(gSimulation.fSimulationContext, outputFile, result.fId, result.fResultLength, result.fResult));
        }
        HPCSIM_END
    }

    return 0;
}

static void PrintUsage(char * name)
{
    std::cerr << "Usage: " << name << " --simulation|-s name.so [--threads|-t X --first|-f X --events|-e X --output|-o name --checkpoint|-c]" << std::endl;
    std::cerr << "\t- Simulation: path of the shared library containing the simulation" << std::endl;
    std::cerr << "\t- Threads: amount of threads to use for computing (min 1). Beware an extra thread will be used for results writing" << std::endl;
    std::cerr << "\t- First: start the event loop at this event" << std::endl;
    std::cerr << "\t- Events: number of events to compute" << std::endl;
    std::cerr << "\t- Output: name of the output file to write" << std::endl;
    std::cerr << "\t- Checkpoint: HPCsim will read existing output file to continue the simulation where it was stopped, instead of simulating everything" << std::endl;
}

int main(int argc, char * argv[])
{
    int option;
    volatile unsigned int nThreads = 1;
    volatile unsigned long nEvents = 100;
    volatile unsigned long firstEvent = 0;
    char outputFile[PATH_MAX] = DEFAULT_NAME;
    char simulationFile[PATH_MAX] = "";
    pthread_t writingThread;
    void * ret;
    void * simulationLib;
    struct sigaction sigHandling;
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
            {"checkpoint", no_argument, 0, 'c'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        option = getopt_long(argc, argv, "e:t:o:f:s:c", long_options, &option_index);
        if (option == -1)
            break;

        switch (option)
        {
            static bool written = false;

            case 't':
                if (optarg[0] == 'a' && optarg[1] == 0)
                {
                    nThreads = sysconf(_SC_NPROCESSORS_ONLN);
                }
                else
                {
                    nThreads = strtoul(optarg, 0, 10);
                }

                if (nThreads == 0)
                {
                    nThreads = 1;
                }
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

            case 'c':
                gSimulation.fCheckPoint = true;
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
        std::cerr << "Failed loading " << simulationFile << std::endl;
        return 0;
    }

    /* Init error lock */
    pthread_mutex_init(&gHandlerLock, 0);

    /* Load its functions */
    LoadAndSetSimulationFunction(SimulationInit);
    LoadAndSetSimulationFunction(RunInit);
#ifdef USE_PILOT_THREAD
    LoadAndSetSimulationFunction(PilotInit);
#endif
    LoadAndSetSimulationFunction(EventInit);
    LoadAndSetSimulationFunction(EventRun);
    LoadAndSetSimulationFunction(EventClear);
#ifdef USE_PILOT_THREAD
    LoadAndSetSimulationFunction(PilotClear);
#endif
    LoadAndSetSimulationFunction(ReduceResult);
    LoadAndSetSimulationFunction(RunClear);
    LoadAndSetSimulationFunction(SimulationUnload);

    /* We need at least something to run */
    if (gSimulation.fEventRun == 0)
    {
        std::cerr << "No EventRun() entry point present" << std::endl;
        goto end;
    }

    /* Initialize our signal handling */
    memset(&sigHandling, 0, sizeof(struct sigaction));
    sigHandling.sa_sigaction = SignalHandler;
    /* We want it to be automatically reset and we want extended info */
    sigHandling.sa_flags = SA_SIGINFO;

    /* These are all the signals we will handle
     * So that simulation can still use SIGUSR1/SIGUSR2 for internal sync
     */
    sigaction(SIGABRT, &sigHandling, NULL);
    sigaction(SIGBUS, &sigHandling, NULL);
    sigaction(SIGFPE, &sigHandling, NULL);
    sigaction(SIGILL, &sigHandling, NULL);
    sigaction(SIGSEGV, &sigHandling, NULL);
    sigaction(SIGSYS, &sigHandling, NULL);
    sigaction(SIGXCPU, &sigHandling, NULL);
    sigaction(SIGXFSZ, &sigHandling, NULL);

    /* Init the simulation */
    if (gSimulation.fSimulationInit != 0)
    {
        HPCSIM_TRY
        {
            if (gSimulation.fSimulationInit(gUsingPilot, nThreads, nEvents, firstEvent, &gSimulation.fSimulationContext) < 0)
            {
                HPCSIM_THROW;
            }
        }
        HPCSIM_EXCEPT
        {
            std::cerr << "Failed initializing library" << std::endl;
            goto end;
        }
        HPCSIM_END
    }

    /* Advance in the generator */
    RngStream::AdvanceStream(firstEvent);

    /* Checkpoint: time to "replay" already handled events */
    if (gSimulation.fCheckPoint)
    {
        int inFD;

        /* Open the previous output file in read-only */
        inFD = open(outputFile, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (inFD >= 0)
        {
            /* Small optimization to process the file a bit faster:
             * we won't keep reading events that are already validated
             */
            off_t offset = 0;

            /* So, as long as we have events to simulate, try to find them in the old file */
            while (nEvents != 0)
            {
                bool found = false;
                off_t readOff = offset;

                /* Try to reach the offset were there are unhandled events,
                 * if we fail, kill the optimization and go back to the begin
                 * of the file
                 */
                if (lseek(inFD, offset, SEEK_SET) == -1)
                {
                    lseek(inFD, 0, SEEK_SET);
                }

                /* While we find contents in the file */
                while (true)
                {
                    uint32_t resultLength;
                    uint8_t id[ID_FIELD_SIZE];

                    /* Read the ID */
                    if (read(inFD, id, sizeof(id)) != sizeof(id))
                    {
                        break;
                    }

                    /* Read the result length */
                    if (read(inFD, &resultLength, sizeof(resultLength)) != sizeof(resultLength))
                    {
                        break;
                    }

                    /* Compare ID with current PRNG (this works due to the nature of RngStream init) */
                    if (memcmp(RngStream::GetNextSeed(), id, sizeof(id)) == 0)
                    {
                        /* Matching, can we optimize reading? */
                        found = true;
                        /* We read a the first offset without known ID, so we eat it */
                        if (offset == readOff)
                        {
                            offset = offset + resultLength + sizeof(resultLength) + sizeof(id);
                        }

                        /* Done for this ID */
                        break;
                    }

                    /* Jump to the next event */
                    if (lseek(inFD, resultLength, SEEK_CUR) == -1)
                    {
                        break;
                    }

                    /* Update offset */
                    readOff = offset + resultLength + sizeof(resultLength) + sizeof(id);
                }

                /* If the current ID wasn't found in file, stop here
                 * It means we found an event that wasn't written in the
                 * file, this is where we have to start again the simulation
                 */
                if (!found)
                {
                    break;
                }

                /* Otherwise, the ID was found, so skip this generator */
                RngStream::AdvanceStream(1);
                /* And this will be one less event to generate */
                --nEvents;
            }

            close(inFD);
        }
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
    if (gSimulation.fRunInit != 0)
    {
        HPCSIM_TRY
        {
            if (gSimulation.fRunInit(gSimulation.fSimulationContext) < 0)
            {
                HPCSIM_THROW;
            }
        }
        HPCSIM_EXCEPT
        {
            std::cerr << "Failed initializing run" << std::endl;
            goto end4;
        }
        HPCSIM_END
    }

#ifndef USE_PILOT_THREAD
    /* Hot loop, the simulation happens here */
    for (unsigned long event = 0; event < nEvents; ++event)
    {
        TThreadsFactory::GetInstance()->CreateThread(SimulationLoop, 0);
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
        context->fEvents = eventsPerThread + ((thread < padding) ? 1 : 0);
        TThreadsFactory::GetInstance()->CreateThread(SimulationLoop, context);

        ++context;
    }
#endif

    /* Wait for all the propagations to finish */
    TThreadsFactory::GetInstance()->WaitForAllThreads();

#ifdef USE_PILOT_THREAD
    delete[] contexts;
#endif

    /* Signal end of run */
    if (gSimulation.fRunClear != 0)
    {
        HPCSIM_TRY
        {
            gSimulation.fRunClear(gSimulation.fSimulationContext);
        }
        HPCSIM_END
    }

end4:
    /* Send the end signal to writer */
    UNUSED_RETURN(write(gPipe[1], &gNullResult, sizeof(TResult)));

    /* Wait for the end of the writer */
    pthread_join(writingThread, &ret);

end3:
    close(gPipe[0]);
    close(gPipe[1]);
end2:
    TThreadsFactory::GetInstance(true);
    pthread_mutex_destroy(&gPipeLock);
    if (gSimulation.fSimulationUnload != 0)
    {
        HPCSIM_TRY
        {
            gSimulation.fSimulationUnload(gSimulation.fSimulationContext);
        }
        HPCSIM_END
    }
end:
    pthread_mutex_destroy(&gHandlerLock);
    dlclose(simulationLib);
    return 0;
}
