/*
 * COPYRIGHT:        See LICENSE in the top level directory
 * PROJECT:          HPCsim
 * FILE:             HPCsim/TThreadsFactory.h
 * PURPOSE:          Threads factory
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

#include <pthread.h>
#include <semaphore.h>

class TThreadsFactory
{
public:
    /**
     * This function is the function to call to create the thread.
     * You've to make sure you've first called once SetMaxThreads() otherwise
     * creation will be denied.
     * If no thread can be created at that time (max capacity) the function
     * will block the caller until it can create the thread.
     * For any other reason that leads to thread creation failure, it will return false
     * @param function Function to execute as thread entry point
     * @param argument Optional argument to pass to the function
     * @return true on creation success, false otherwise
     */
    bool CreateThread(void * (* function)(void *), void * argument);
    /**
     * This function defines the maximum number of threads that the threads factory can manage
     * and have running at a time.
     * You have to call it (and only once!) before any call to CreateThread(). Trying to call
     * it several times will just make it fail and leave the limit unchanged.
     * @param maxThreads The maximum number of threads. Minimum 1
     * @return true if it could set the limit, false otherwise.
     */
    bool SetMaxThreads(unsigned int maxThreads);
    /**
     * This function returns the init lock so that the thread entrypoint can release it once it's done
     * with RNG init.
     * @return The init lock. It cannot fail.
     */
    sem_t * GetInitLock(void);
    /**
     * This is the static function to have the thread factory. It is unique and this is the only way to
     * create and use it.
     * @return A pointer to the thread factory class.
     */
    static TThreadsFactory * GetInstance();
    /**
     * This function will be invoked automatically at the end of a thread and will mark it as available
     * for the thread factory. So that it can be joined and released.
     * @param id Internal ID of the thread
     */
    void ResetThread(unsigned int id);
    /**
     * This is a functiont to wait on all the current running threads. It won't join them nor release resources.
     * Note that while you're waiting on said threads, any thread creation attempt will block.
     */
    void WaitForAllThreads(void);
    /**
     * Destructor.
     */
    ~TThreadsFactory();

private:
    /**
     * Constructor. Kept private to use this class only as singleton
     * @see GetInstance()
     */
    TThreadsFactory();

    /**
     * Maximum of threads that can be run at a time by the factory
     * @see SetMaxThreads()
    */
    unsigned int fMaxThreads;
    /**
     * This semaphore is the way to limit the number of threads.
     * It is initialized to fMaxThreads and then, any thread creation
     * will cause an acquire.
     * When no room is available, the caller will then wait until there's some
     */
    sem_t fCreationLimiter;
    /**
     * This semaphore is used to ensure that only thread can be started at a time
     * and even to make sure that we cannot start a new thread before the old one
     * is done with init.
     * It is up to the thread routine to release it!
     * @see GetInitLock()
     */
    sem_t fInitLock;
    /**
     * This semaphore is here to protect the access to the threads list and to
     * prevent any race condition between a thread starting and a thread finishing,
     * both needing to access the list.
     * @see ResetThread()
     */
    sem_t fThreadsLock;
    /**
     * This is the list containing all the threads available and/or running. It can
     * contain as many threads as fMaxThreads.
     * If an entry has the value 0, the room is empty, a new thread can use it.
     * If an entry has an even number, the room is busy, a thread is currently using it.
     * If an entry has an odd number, the room is free BUT requires a join before use.
     * You've to remove one before you're able to join the finished thread.
     * This join effort is required to prevent any threads leak (as for SIGCHLD and zombies).
     */
    pthread_t * fThreads;
    /**
     * This variable is set to false most of the time.
     * When set to true, it means the class reached its destructor and any thread creation request
     * will be denied.
     */
    bool fBeingDestroyed;
};

struct TThreadContext
{
    void * (* fFunction)(void *);
    void * fArgument;
    unsigned int fId;
    pthread_t fThread;
    TThreadContext(void * (* function)(void *), void * argument, unsigned int id) : fFunction(function), fArgument(argument), fId(id) { } ;
};

void * ThreadHelper(void * context);
