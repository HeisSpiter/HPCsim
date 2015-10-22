/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim
 * FILE:             HPCsim/Exceptions.cpp
 * PURPOSE:          Exceptions handling
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <cstdlib>
#include <iostream>
#include <pthread.h>

#include "Exceptions.h"
#include "simulation.h"

#define BUFFER_SIZE 0x100

__thread jmp_buf gJumpEnv;
volatile __thread bool gInTry = false;
pthread_mutex_t gHandlerLock;
const sigval_t gMagicMarker = {HPCSIM_MAGIC_MARKER};
/* Before accessing this buffer, make sure you locked gHandlerLock */
static void * gBuffer[BUFFER_SIZE];

void SignalHandler(int signal, siginfo_t * sigInfo, void * context)
{
    UNUSED_PARAMETER(context);

    /* If the signal wasn't sent on purpose by ourselves, then, it indicates
     * a broken behavior in either HPCsim or the called simulation.
     * We'll have to provide a backtrace to help debugging.
     */
    if (sigInfo->si_int != HPCSIM_MAGIC_MARKER)
    {
        char ** strings;
        int numberAddresses, trace;

        /* Lock the handler lock for two reasons:
         * First, we don't want to have a race condition accessing standard error
         * Our backtrace must be crystal clear
         * Second, the gBuffer is shared between all the instances of the handler
         * so, make sure it is only used by one at a time
         */
        pthread_mutex_lock(&gHandlerLock);

        /* If the signal occured while in a try block, it is the simulation shared object
         * which is failing. Inform the user
         */
        if (gInTry)
        {
            std::cerr << "Oops! Something went wrong in the simulation library" << std::endl;
        }
        std::cerr << "Exception " << signal << " occured at " << sigInfo->si_addr << std::endl;

        /* Get the backtrace which lead to the error */
        numberAddresses = backtrace(gBuffer, BUFFER_SIZE);
        if (numberAddresses != 0)
        {
            /* Try to resolve symbols */
            strings = backtrace_symbols(gBuffer, numberAddresses);
            if (strings == 0)
            {
                numberAddresses = 0;
            }
        }

        /* Print the backtrace */
        for (trace = 0; trace < numberAddresses; ++trace)
        {
            std::cerr << (numberAddresses - 1 - trace) << ": " << strings[trace] << "\n";
        }

        /* No need for the lock any longer */
        pthread_mutex_unlock(&gHandlerLock);

        free(strings);
    }

    /* If we were in a try block, jump to the exception handler */
    if (gInTry)
    {
        longjmp(gJumpEnv, signal);
    }
}
