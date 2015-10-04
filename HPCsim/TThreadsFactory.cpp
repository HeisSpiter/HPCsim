/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim
 * FILE:             HPCsim/TThreadsFactory.cpp
 * PURPOSE:          Threads factory
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <cassert>
#include "TThreadsFactory.h"

#include <iostream>

TThreadsFactory::TThreadsFactory()
{
    fThreads = 0;
    fMaxThreads = 0;
    fBeingDestroyed = false;
    if (sem_init(&fInitLock, 0, 1) != 0)
        assert(false);
    if (sem_init(&fThreadsLock, 0, 1) != 0)
        assert(false);
}

TThreadsFactory::~TThreadsFactory()
{
    /* First, deny any new thread creation */
    fBeingDestroyed = true;

    /* If we were initialised */
    if (fMaxThreads)
    {
        /* Wait for all the threads to complete */
        for (unsigned int i = 0; i < fMaxThreads; ++i)
            sem_wait(&fCreationLimiter);

        /* Also cleanup any left thread */
        for (unsigned int i = 0; i < fMaxThreads; ++i)
        {
            /* If that slot was used, then, it should be joinable */
            if (fThreads[i] != 0)
            {
                void * ret;

                assert(fThreads[i] & 1);

                /* Remove the mark */
                fThreads[i] &= ~1;
                /* And join */
                pthread_join(fThreads[i], &ret);
            }
        }

        /* Now, delete everything */
        delete[] fThreads;
        sem_destroy(&fCreationLimiter);
    }

    sem_destroy(&fInitLock);
    sem_destroy(&fThreadsLock);
}

bool TThreadsFactory::SetMaxThreads(unsigned int maxThreads)
{
    /* Don't allow max setting if already set */
    if (fMaxThreads > 0)
        return false;

    /* 0 is not acceptable as max threads */
    if (maxThreads == 0)
        return false;

    /* Attempt to init the semaphore */
    if (sem_init(&fCreationLimiter, 0, maxThreads) != 0)
        return false;

    /* Allocate memory for the threads tab */
    fThreads = new pthread_t[maxThreads];
    /* Zero out the entries */
    for (unsigned int i = 0; i < maxThreads; ++i)
        fThreads[i] = 0;

    /* Save size */
    fMaxThreads = maxThreads;

    /* Success */
    return true;
}

TThreadsFactory * TThreadsFactory::GetInstance(bool destroyInstance)
{
    static TThreadsFactory * gThisInstance = 0;

    /* If we don't exist yet, start ourselves */
    if (gThisInstance == 0 && !destroyInstance)
        gThisInstance = new TThreadsFactory();

    /* If we're asked to delete the instance */
    if (destroyInstance)
    {
        delete gThisInstance;
        gThisInstance = 0;
    }

    /* Return the instance */
    return gThisInstance;
}

bool TThreadsFactory::CreateThread(void * (* function)(void *), void * argument)
{
    unsigned int i = 0;
    TThreadContext * context;

    /* If we weren't configured, bail out */
    if (fMaxThreads == 0)
        return false;

    /* Don't allow creation if we're in the process of dying */
    if (fBeingDestroyed)
        return false;

    /* Wait until there's a room left for another thread */
    sem_wait(&fCreationLimiter);
    /* We'll be the only one to spawn now */
    sem_wait(&fInitLock);

    /* Don't allow creation if we're in the process of dying */
    if (fBeingDestroyed)
    {
        sem_post(&fInitLock);
        sem_post(&fCreationLimiter);
        return false;
    }

    /* Ensure we're the only ones to deal with the list */
    sem_wait(&fThreadsLock);
    /* Find a place where we can set our thread */
    for (; i < fMaxThreads; ++i)
    {
        /* Check if have found a thread that requires a join */
        if (fThreads[i] & 1)
        {
            void * ret;

            /* Remove the mark */
            fThreads[i] &= ~1;
            /* And join */
            pthread_join(fThreads[i], &ret);

            /* We'll use that one */
            break;
        }

        if (fThreads[i] == 0)
            break;
    }
    /* Clear for the others */
    sem_post(&fThreadsLock);

    /* There must exist! */
    assert(i != fMaxThreads);

    /* Initialise the context for the helping function */
    context = new TThreadContext(function, argument, i);

    /* Start thread */
    int err = pthread_create(&fThreads[i], 0, ThreadHelper, context);
    if (err == 0)
        return true;

    /* If we reach that point, we failed to spawn thread
     * Release everything not to deadlock
     */

    /* First, reset entry */
     fThreads[i] = 0;

    /* And then, release */
    sem_post(&fInitLock);
    sem_post(&fCreationLimiter);

    return false;
}

sem_t * TThreadsFactory::GetInitLock(void)
{
    return &fInitLock;
}

void TThreadsFactory::ResetThread(unsigned int id)
{
    /* Check we're killing a valid thread */
    assert(id < fMaxThreads);
    assert(fThreads[id] != 0);

    /* Ensure we won't modify our entry while someone else is reading it */
    sem_wait(&fThreadsLock);
    /* Mark the thread as waiting for a join */
    fThreads[id] |= 1;
    /* And open room for a new one */
    sem_post(&fCreationLimiter);
    /* Finally release our list, waiting threads you can go */
    sem_post(&fThreadsLock);
}

void TThreadsFactory::WaitForAllThreads(void)
{
    /* Wait for all the threads to complete
     * Beware, this will lock any new thread creation
     */
    for (unsigned int i = 0; i < fMaxThreads; ++i)
        sem_wait(&fCreationLimiter);

    /* Unlock everything now */
    for (unsigned int i = 0; i < fMaxThreads; ++i)
        sem_post(&fCreationLimiter);
}

void * ThreadHelper(void * context)
{
    void * ret;
    TThreadContext * threadContext = reinterpret_cast<TThreadContext *>(context);

    ret = threadContext->fFunction(threadContext->fArgument);

    /* We're done with user thread
     * It's time to finish ourselves and to release resources
     */
    TThreadsFactory::GetInstance()->ResetThread(threadContext->fId);
    delete threadContext;

    pthread_exit(ret);
    return ret;
}
