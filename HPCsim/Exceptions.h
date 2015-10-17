/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim
 * FILE:             HPCsim/Exceptions.h
 * PURPOSE:          Exceptions handling
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <setjmp.h>
#include <signal.h>
#include <execinfo.h>

extern __thread jmp_buf gJumpEnv;
extern volatile __thread bool gInTry;
extern pthread_mutex_t gHandlerLock;
void SignalHandler(int signal, siginfo_t * sigInfo, void * context);

/* Magic marker to recognize signals we send on purpose in
 * in a try block (see HPCSIM_THROW)
 */
#define HPCSIM_MAGIC_MARKER 0x42424242

/* Begin of a try/except block */
#define HPCSIM_TRY \
    gInTry = true; \
    if (setjmp(gJumpEnv) == 0)

/* Begin of the except block.
 * It is not mandatory
 */
#define HPCSIM_EXCEPT else

/* Manually throw an error when in the try block
 * to get into the except block.
 */
#define HPCSIM_THROW \
    do { \
        sigval val; \
        val.sival_int = HPCSIM_MAGIC_MARKER; \
        sigqueue(getpid(), SIGSEGV, val); \
    } while(0)

/* End marker of a try/except block.
 * It is mandatory in any case.
 * Not using it will lead to broken error handling
 */
#define HPCSIM_END gInTry = false;
