/******************************************************************************
 *
 * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * Module    : Linux Kernel Abstraction Layer
 *
 * Hierarchy :
 *
 * Purpose   : Encapsulates and abstracts services from different operating
 *             system, including user-mode as well as kernel-mode services.
 ******************************************************************************/
#ifdef WIN32

#include "oslayer.h"


#ifdef OSLAYER_EVENT
/******************************************************************************
 *  osEventInit()
 ******************************************************************************
 *  @brief  Initialize an event object.
 *
 *  Init an event. Automatic reset flag as well as initial state could be set,
 *  but event is not signaled. Automatic reset means osEventReset() must
 *  not be called to set event state to zero. This also implies that all
 *  waiting threads, otherwise only one thread, will be restarted.
 *
 *  @param  pEvent         Reference of the event object
 *
 *  @param  Automatic      Automatic Reset flag
 *
 *  @param  InitState      Initial state of the event object (true/false)
 *
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Event successfully created
 *  @retval OSLAYER_ERROR  Event is not created
 *
 ******************************************************************************/
int32_t osEventInit(osEvent *pEvent, int32_t Automatic, int32_t InitState)
{
	int32_t ManualReset = !Automatic;


    /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    osEventDestroy(pEvent);
    pEvent->handle = CreateEvent(NULL, ManualReset, InitState, NULL);

    if(pEvent->handle == NULL)
        return OSLAYER_ERROR;
    else
        return OSLAYER_OK;
}

/******************************************************************************
 *  osEventSignal()
 ******************************************************************************
 *  @brief  Set the event state to true.
 *
 *  Set the state of the event object to true and signal waiting thread(s).
 *
 *  @param  pEvent                    Reference of the event object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Signal send successfully
 *  @retval OSLAYER_ERROR             Tried to signal a not initialized event
 *  @retval OSLAYER_OPERATION_FAILED  Signal not send
 *
 ******************************************************************************/
int32_t osEventSignal(osEvent *pEvent)
{
    /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent->handle == NULL)
        return OSLAYER_ERROR;
    else if(SetEvent(pEvent->handle))
        return OSLAYER_OK;
    else
        return OSLAYER_OPERATION_FAILED;
}


/******************************************************************************
*  osEventReset()
*******************************************************************************
 *  @brief  Reset the event state to true.
 *
 *  Reset the state of the event object to false.
 *
 *  @param  pEvent                    Reference of the event object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Event reset successfully
 *  @retval OSLAYER_ERROR             Tried to reset a not initialized event
 *  @retval OSLAYER_OPERATION_FAILED  Event not reset
 *
 *****************************************************************************/
int32_t osEventReset(osEvent *pEvent)
{
   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent->handle == NULL)
        return OSLAYER_ERROR;
    else if(ResetEvent(pEvent->handle))
        return OSLAYER_OK;
    else
        return OSLAYER_OPERATION_FAILED;
}

/******************************************************************************
 *  osEventPulse()
 ******************************************************************************
 *  @brief  Pulse the event false -> true -> false.
 *
 *  Pulse the state of the event object with the following sequence:
 *  false -> true -> false and signal waiting thread(s).
 *
 *  @param  pEvent                    Reference of the event object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Event pulsed successfully
 *  @retval OSLAYER_ERROR             Tried to pulse a not initialized event
 *  @retval OSLAYER_OPERATION_FAILED  Event not pulsed
 *
 ******************************************************************************/
int32_t osEventPulse(osEvent *pEvent)
{
   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent->handle == NULL)
        return OSLAYER_ERROR;
    else if(PulseEvent(pEvent->handle))
        return OSLAYER_OK;
    else
        return OSLAYER_OPERATION_FAILED;
}

/******************************************************************************
 *  osEventWait()
 ******************************************************************************
 *  @brief  Blocking wait for event to be true.
 *
 *  Wait for the state of the event object becoming true and block calling
 *  thread. The function call returns immediatly, if event is already signaled.
 *
 *  @param  pEvent                    Reference of the event object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Wait for event succeeded and function
 *                                    returned due to signal sent
 *  @retval OSLAYER_ERROR             Tried to wait for a not initialized event
 *  @retval OSLAYER_OPERATION_FAILED  Wait for event failed
 *
 ******************************************************************************/
int32_t osEventWait(osEvent *pEvent)
{
   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent->handle == NULL)
        return OSLAYER_ERROR;
    else if(WaitForSingleObject(pEvent->handle, INFINITE) == WAIT_OBJECT_0)
        return OSLAYER_OK;
    else
        return OSLAYER_OPERATION_FAILED;
}

/******************************************************************************
 *  osEventTimedWait()
 ******************************************************************************
 *  @brief  Blocking wait with timeout for event to be true.
 *
 *  Wait for the state of the event object becoming true and block calling
 *  thread. The function call returns immediatly, if event is already signaled.
 *  If specified timeout elapses, the function returns with apropriate timeout
 *  error code.
 *
 *  @param  pEvent      reference of the event object
 *
 *  @param  msec        timeout value in milliseconds
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Wait for event succeeded and function
 *                                    returned due to signal sent
 *  @retval OSLAYER_TIMEOUT           Wait for event succeeded and function
 *                                    returned due to timeout and no signal
 *                                    was sent
 *  @retval OSLAYER_OPERATION_FAILED  Wait for event failed
 *
 *****************************************************************************/
int32_t osEventTimedWait(osEvent *pEvent, uint32_t msec)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;
    int32_t i_res;

   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent->handle != NULL)
    {
        i_res = WaitForSingleObject(pEvent->handle, msec);

        if(i_res == WAIT_OBJECT_0)
            Ret = OSLAYER_OK;
        else if(i_res == WAIT_TIMEOUT)
            Ret = OSLAYER_TIMEOUT;
        else
            Ret = OSLAYER_OPERATION_FAILED;
    }

    return Ret;
}


/******************************************************************************
 *  osEventDestroy()
 ******************************************************************************
 *  @brief  Destroy the event.
 *
 *  Destroy the event and free resources associated with event object.
 *
 *  @param  pEvent      Reference of the event object
 *
 *  @return             always OSLAYER_OK
 ******************************************************************************/
int32_t osEventDestroy(osEvent *pEvent)
{
   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent->handle)
    {
        (void)CloseHandle(pEvent->handle);
        pEvent->handle = NULL;
    }

    return OSLAYER_OK;
}
#endif /* OSLAYER_EVENT */




#ifdef OSLAYER_MUTEX
/******************************************************************************
 *  osMutextInit()
 ******************************************************************************
 *  @brief  Initialize a mutex object.
 *
 *  @param  pMutex         Reference of the mutex object
 *
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Mutex successfully created
 *  @retval OSLAYER_ERROR  Mutex is not created
 *
 ******************************************************************************/
int32_t osMutexInit(osMutex *pMutex)
{
	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);


	/*
     *  Windows 2000:  If the high-order bit is set, the function preallocates
	 *                 the event used by the EnterCriticalSection function.
	 *                 Otherwise EnterCriticalSection allocates the event and
	 *                 possibly could throw an exception in a low memory
	 *                 situation. This is nor more needed for XP and later
	 *                 and will be ignored.
     *
	 * if(!InitializeCriticalSectionAndSpinCount(&pMutex->sCritSection, 0x80000400))
     *     return OSLAYER_ERROR;
	 * else
	 *     return OSLAYER_OK;
     */

    InitializeCriticalSection(&pMutex->sCritSection);
    return OSLAYER_OK;
}

/******************************************************************************
 *  osMutexLock()
 ******************************************************************************
 *  @brief  Lock a mutex object.
 *
 *          Lock the mutex. Thread will be blocked if mutex already locked.
 *
 *  @param  pMutex         Reference of the mutex object
 *
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Mutex successfully locked
 *
 ******************************************************************************/
int32_t osMutexLock(osMutex *pMutex)
{
	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

	EnterCriticalSection(&pMutex->sCritSection);

    return OSLAYER_OK;
}


/******************************************************************************
 *   osMutexUnlock()
 ******************************************************************************
 *  @brief  Unlock a mutex object.
 *
 *   @param  pMutex         Reference of the mutex object
 *
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Mutex successfully unlocked
 *
 ******************************************************************************/
int32_t osMutexUnlock(osMutex *pMutex)
{
	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    LeaveCriticalSection(&pMutex->sCritSection);
    return OSLAYER_OK;
}


/******************************************************************************
 *  osMutexTryLock()
 ******************************************************************************
 *  @brief  Try to lock a mutex object.
 *
 *  Try to lock mutex in Non-Blocking mode. Returns OSLAYER_OK if successful.
 *
 *  @param  pMutex                    Reference of the mutex object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Mutex successfully locked
 *  @retval OSLAYER_OPERATION_FAILED  Mutex not locked (already locked by
 *                                    someone else)
 ******************************************************************************/
int32_t osMutexTryLock(osMutex *pMutex)
{
	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    if(TryEnterCriticalSection(&pMutex->sCritSection))
        return OSLAYER_OK;
    else
        return OSLAYER_OPERATION_FAILED;
}

/******************************************************************************
 *  osMutexDestroy()
 ******************************************************************************
 *  @brief  Destroy a mutex object.
 *
 *  @param  pMutex      Reference of the mutex object
 *
 *  @return             always OSLAYER_OK
 ******************************************************************************/
int32_t osMutexDestroy(osMutex *pMutex)
{
	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    DeleteCriticalSection(&pMutex->sCritSection);

    return OSLAYER_OK;
}
#endif /* OSLAYER_MUTEX */








#ifdef OSLAYER_SEMAPHORE
/******************************************************************************
 *  osSemaphoreInit()
 ******************************************************************************
 *  @brief  Init a semaphore with init count.
 +
 +  @param  pSem           Reference of the semaphore object
 +
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Semaphore successfully created
 +  @retval OSLAYER_ERROR  Semaphore is not created
 *
 ******************************************************************************/
int32_t osSemaphoreInit(osSemaphore *pSem, int32_t init_count)
{
	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

	/* max semaphore count is set to 0x7fffffff. OK, it's just a */
	/* number...*/
    pSem->handle = CreateSemaphore(NULL, init_count, 2147483647L, 0);
    if(pSem->handle != 0)
		return OSLAYER_OK;
	else
		return OSLAYER_ERROR;
}

/******************************************************************************
 *  osSemaphoreTimedWait()
 ******************************************************************************
 *  @brief  Decrease the semaphore value in blocking mode, but with timeout.
 *
 *  @param  pSem                      Reference of the semaphore object
 *
 *  @param  msec                      Timeout value in milliseconds
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Wait for semaphore succeeded and
 *                                    function returned due to signal sent
 *  @retval OSLAYER_TIMEOUT           Wait for semaphore succeeded and
 *                                    function returned due to timeout and
 *                                    no signal was sent
 *  @retval OSLAYER_OPERATION_FAILED  Wait for semaphore failed
 *  @retval OSLAYER_ERROR             Tried to wait for a not initialized
 +                                    semaphore
 *
 ******************************************************************************/
int32_t osSemaphoreTimedWait(osSemaphore *pSem, uint32_t msec)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;
	int32_t res;

	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    if(pSem->handle)
    {
        res = WaitForSingleObject(pSem->handle, msec);
        Ret = OSLAYER_OK;

        if(res == WAIT_TIMEOUT)
            Ret = OSLAYER_TIMEOUT;
        else if(res != WAIT_OBJECT_0)
            Ret = OSLAYER_OPERATION_FAILED;
    }

    return Ret;
}

/******************************************************************************
 *  osSemaphoreWait()
 ******************************************************************************
 *  @brief  Decrease the semaphore value in blocking mode.
 *
 *  @param  pSem                      Reference of the semaphore object
 *
 *  @param  msec                      Timeout value in milliseconds
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Wait for semaphore succeeded and
 *                                    function returned due to signal sent
 *  @retval OSLAYER_OPERATION_FAILED  Wait for semaphore failed
 *  @retval OSLAYER_ERROR             Tried to wait for a not initialized
 *                                    semaphore
 *
 ******************************************************************************/
int32_t osSemaphoreWait(osSemaphore *pSem)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;
	int32_t res;


	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    if(pSem->handle)
    {
        res = WaitForSingleObject(pSem->handle, INFINITE);
        Ret = OSLAYER_OK;

        if(res != WAIT_OBJECT_0)
            Ret = OSLAYER_OPERATION_FAILED;
    }

	return Ret;
}

/******************************************************************************
 *  osSemaphoreTryWait()
 ******************************************************************************
 *  @brief  Try to wait for a semaphore object.
 *
 *          Try to decrease the semaphore value in non-blocking mode. This
 *          functionality is realized internally with a timed wait called
 *          with a timeout value of zero.
 *
 *  @param  pSem                      Reference of the mutex object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Wait for semaphore succeeded and
 *                                    function returned due to signal sent
 *  @retval OSLAYER_TIMEOUT           Wait for semaphore succeeded and
 *                                    function returned due to timeout and
 *                                    no signal was sent
 *  @retval OSLAYER_OPERATION_FAILED  Wait for semaphore failed
 *  @retval OSLAYER_ERROR             Tried to wait for a not initialized
 *                                    semaphore
 *
 ******************************************************************************/
int32_t osSemaphoreTryWait(osSemaphore *pSem)
{
    return osSemaphoreTimedWait(pSem, 0);
}

/******************************************************************************
 *  osSemaphorePost()
 ******************************************************************************
 *  @brief  Increase the semaphore value.
 *
 *  @param  pSem                      Reference of the semaphore object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Semaphore value successfully increased
 *  @retval OSLAYER_ERROR             Tried to increase the value of a not
 *                                    initialized semaphore object
 *  @retval OSLAYER_OPERATION_FAILED  Increase of semaphore value failed
 ******************************************************************************/
int32_t osSemaphorePost(osSemaphore *pSem)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;


	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    if(pSem->handle)
    {
		/* Increment is 1.*/
        if(ReleaseSemaphore(pSem->handle, 1, 0))
            Ret = OSLAYER_OK;
        else
            Ret = OSLAYER_OPERATION_FAILED;
    }

    return Ret;
}

/******************************************************************************
 *  osSemaphoreDestroy()
 ******************************************************************************
 *  @brief  Destroy the semaphore object.
 *
 *  @param  pSem        Reference of the semaphore object
 *
 *  @return             always OSLAYER_OK
 ******************************************************************************/
int32_t osSemaphoreDestroy(osSemaphore *pSem)
{
	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    if(pSem->handle)
    {
        (void)CloseHandle(pSem->handle);
        pSem->handle = NULL;
    }

    return OSLAYER_OK;
}
#endif /* OSLAYER_SEMAPHORE */






#ifdef OSLAYER_ATOMIC
/******************************************************************************
 *  osAtomicInit()
 ******************************************************************************
 *  @brief  Initialize atomic operation functionality.
 *
 *  This function must be called before any other osAtomicXXX call.
 *
 ******************************************************************************/
int32_t osAtomicInit()
{
    /* add code here if needed */
    return OSLAYER_OK;
}

/******************************************************************************
 *  osAtomicShutdown()
 ******************************************************************************
 *  @brief  Shutdown atomic operation functionality.
 *
 *  This function must be called before process is terminated
 *  when osAtomicInit has been called before.
 *
 *  @return             always OSLAYER_OK
 ******************************************************************************/
int32_t osAtomicShutdown()
{
    /* add code here if needed */
    return OSLAYER_OK;
}

/******************************************************************************
 *  osAtomicTestAndClearBit()
 ******************************************************************************
 *  @brief  Test and set a bit position atomically.
 *
 *  Test if a bit position inside a variable is set and clears the bit
 *  afterwards. The complete operation is atomic (includes IRQ safety).
 *
 *  @param  pVar           32-bit unsigned variable to be modified
 *  @param  bitpos         Bit to be tested and cleared
 *
 *  @return                *pVar & (1 << bitpos)
 *
 ******************************************************************************/
uint32_t osAtomicTestAndClearBit(uint32_t* pVar, uint32_t bitpos)
{
    /* TODO: implement it */
    OSLAYER_ASSERT(0);
}

/******************************************************************************
 * osAtomicSetBit()
 ******************************************************************************
 * @brief  Set a bit position atomically.
 *
 * Set a bit position inside a variable.
 * The operation is atomic (includes IRQ safety).
 *
 * @param  pVar           32-bit unsigned variable to be modified
 * @param  bitpos         Bit to be set
 *
 *  @return               always OSLAYER_OK
 ******************************************************************************/
int32_t osAtomicSetBit(uint32_t* pVar, uint32_t bitpos)
{
    /* TODO: implement it */
    OSLAYER_ASSERT(0);
}

/******************************************************************************
 * osAtomicSet()
 ******************************************************************************
 * @brief  Set value atomically.
 *
 * Set variable to value. The operation is atomic (includes IRQ safety).
 *
 * @param  pVar           32-bit unsigned variable to be modified
 * @param  value          Value to be set
 *
 *  @return               always OSLAYER_OK
*******************************************************************************/
int32_t osAtomicSet(uint32_t* pVar, uint32_t value)
{
    /* TODO: implement it */
    OSLAYER_ASSERT(0);
}
#endif /* OSLAYER_ATOMIC */








#ifdef OSLAYER_THREAD
/******************************************************************************
 *  osThreadCreate()
 ******************************************************************************
 *  @brief  Create a thread.
 *
 *  @param  pThread                   Reference of the semaphore object
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Thread object created successfully
 *  @retval OSLAYER_OPERATION_FAILED  Creation of thread object failed
 *
 ******************************************************************************/
int32_t osThreadCreate(osThread *pThread, osThreadFunc thread_func, void *arg)
{
    OSLAYER_STATUS res = OSLAYER_OPERATION_FAILED;


	/* check error(s) */
    OSLAYER_ASSERT(pThread == NULL);
    OSLAYER_ASSERT(thread_func == NULL);

	if(osMutexInit(&pThread->access_mut) != OSLAYER_OK)
        return OSLAYER_OPERATION_FAILED;
	else
    {
        if(pThread->handle != NULL)
            osThreadWait(pThread);

		(void)osMutexLock(&pThread->access_mut);

		pThread->handle = (HANDLE)CreateThread(0, 0, (LPTHREAD_START_ROUTINE)thread_func,
												arg, 0, 0);

        res = ((pThread->handle) ? OSLAYER_OK : OSLAYER_OPERATION_FAILED);
		(void)osMutexUnlock(&pThread->access_mut);
    }

    return res;
}

/******************************************************************************
 *  osThreadSetPriority()
 ******************************************************************************
 *  @brief  Set thread priority.
 *
 *  @param  pThread                   Reference of the semaphore object
 *
 *  @param  priority                  New thread priority to be set
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Thread priority changed successfully
 *  @retval OSLAYER_ERROR             Invalid thread priority value passed
 *  @retval OSLAYER_OPERATION_FAILED  Thread priority change failed
 *
 ******************************************************************************/
int32_t osThreadSetPriority(osThread *pThread, OSLAYER_THREAD_PRIO priority)
{
    OSLAYER_STATUS res = OSLAYER_OPERATION_FAILED;
    bool b_res = false;

    if(pThread->handle != NULL)
    {
        (void)osMutexLock(&pThread->access_mut);

        switch (priority) {
        case OSLAYER_THREAD_PRIO_HIGHEST:
            b_res = SetThreadPriority(pThread->handle,
                                      THREAD_PRIORITY_HIGHEST);
            break;

        case OSLAYER_THREAD_PRIO_HIGH:
            b_res = SetThreadPriority(pThread->handle,
                                      THREAD_PRIORITY_ABOVE_NORMAL);
            break;

        case OSLAYER_THREAD_PRIO_NORMAL:
            b_res = SetThreadPriority(pThread->handle,
                                      THREAD_PRIORITY_NORMAL);
            break;

        case OSLAYER_THREAD_PRIO_LOW:
            b_res = SetThreadPriority(pThread->handle,
                                      THREAD_PRIORITY_BELOW_NORMAL);
            break;

        case OSLAYER_THREAD_PRIO_LOWEST:
            b_res = SetThreadPriority(pThread->handle,
                                      THREAD_PRIORITY_LOWEST);
            break;

        default:
            b_res = false;
            break;
        }

		(void)osMutexUnlock(&pThread->access_mut);
    }

    if(b_res )
        res = OSLAYER_OK;
    else
        res = OSLAYER_OPERATION_FAILED;

    return res;
}

/******************************************************************************
 *  osThreadWait()
 ******************************************************************************
 *  @brief  Wait until thread exits.
 *
 *          To avoid memory leak, always call @ref osThreadWait and wait
 *          for the child thread to terminate in the calling thread.
 *
 *  @param  pThread       Reference of the semaphore object
 *
 *  @return               always OSLAYER_OK
 ******************************************************************************/
int32_t osThreadWait(osThread *pThread)
{
    /* check error(s) */
    OSLAYER_ASSERT(pThread == NULL);

    (void)osMutexLock(&pThread->access_mut);
    if(pThread->handle)
    {
        pThread->wait_count++;
	    (void)osMutexUnlock(&pThread->access_mut);

        (void)WaitForSingleObject(pThread->handle, INFINITE);

		(void)osMutexLock(&pThread->access_mut);
		pThread->wait_count--;
		if(pThread->wait_count == 0)
        {
            (void)CloseHandle(pThread->handle);
            pThread->handle = NULL;
        }
    }

	(void)osMutexUnlock(&pThread->access_mut);

	return OSLAYER_OK;
}

/******************************************************************************
 *  osThreadClose()
 ******************************************************************************
 *  @brief  Destroy thread object.
 *
 *  @param  pThread       Reference of the semaphore object
 *
 *  @return               always OSLAYER_OK
 ******************************************************************************/
int32_t osThreadClose(osThread *pThread)
{
    /* check error(s) */
    OSLAYER_ASSERT(pThread == NULL);

	osThreadWait(pThread);
    osMutexDestroy(&pThread->access_mut);

    return OSLAYER_OK;
}
#endif /* OSLAYER_THREAD */







#ifdef OSLAYER_MISC
/******************************************************************************
 *  osSleep()
 ******************************************************************************
 *  @brief  Yield the execution of current thread for msec miliseconds.
 *
 *  @param  msec       Wait time in millisecobds
 *
 *  @return            always OSLAYER_OK
 ******************************************************************************/
int32_t osSleep(uint32_t msec)
{
    Sleep(msec);
    return OSLAYER_OK;
}

/******************************************************************************
 *  osGetTick()
 ******************************************************************************
 *  @brief  Obtain the clock tick.
 +
 *  @return            Current clock tick. The resoultion of clock tick can
 +                     be requested with @ref osGetFrequency
 *
 ******************************************************************************/
uint64_t osGetTick(void)
{
    LARGE_INTEGER t1;

    (void)QueryPerformanceCounter(&t1);
    return t1.QuadPart;
}


/******************************************************************************
 *  osGetFrequency()
 ******************************************************************************
 *  @brief  Obtain the clock resolution.
 *
 *  @return            The resoultion of the clock tick.
 *
 ******************************************************************************/
uint64_t osGetFrequency(void)
{
    LARGE_INTEGER t1;

    (void)QueryPerformanceFrequency(&t1);
    return t1.QuadPart;
}








/******************************************************************************
 *  osMalloc()
 ******************************************************************************
 *  @brief  Allocate a continuous block of memory.
 *
 *  @param  size       Size of memory block to be allocated
 *
 ******************************************************************************/
void* osMalloc(uint32_t size)
{
	return malloc(size);
}


/******************************************************************************
 *  osFree()
 ******************************************************************************
 *  @brief  Free a continuous block of memory.
 *
 *  @param  p       Pointer to previously allocated memory block
 *
 *  @return         always OSLAYER_OK
 ******************************************************************************/
int32_t osFree(void *p)
{
	free(p);
	return OSLAYER_OK;
}
#endif /* OSLAYER_MISC */







#ifdef OSLAYER_MISC
#ifdef OSLAYER_KERNEL
/******************************************************************************
 *  osSpinLockInit()
 ******************************************************************************
 *  @brief  Init a spin lock object.
 *
 *  @param  p_spin_lock      Reference of the spin lock object
 *
 *  @return                  always OSLAYER_OK
 ******************************************************************************/
int32_t osSpinLockInit(osSpinLock *p_spin_lock)
{
    KeInitializeSpinLock(&p_spin_lock->lock);
	return OSLAYER_OK;
}


/******************************************************************************
 *  osSpinLockAcquire()
 ******************************************************************************
 *  @brief  Acquire a spin lock object.
 *
 *  @param  p_spin_lock      Reference of the spin lock object
 *
 *  @param  flags            Interrupt context flags which need to be safed
 *                           in a local variable
 *
 *  @return                  always OSLAYER_OK
 ******************************************************************************/
int32_t osSpinLockAcquire(osSpinLock *p_spin_lock, uint32_t flags)
{
    KeAcquireSpinLock(&p_spin_lock->lock, &(KIRQL)flags);
	return OSLAYER_OK;
}


/******************************************************************************
 *  osSpinLockRelease()
 ******************************************************************************
 *  @brief  Release a spin lock object.
 *
 *  @param  p_spin_lock      Reference of the spin lock object
 *
 *  @param  flags            Interrupt context flags which need to be safed
 *                           in a local variable
 *
 *  @return                  always OSLAYER_OK
 ******************************************************************************/
int32_t osSpinLockRelease(osSpinLock *p_spin_lock, uint32_t flags)
{
    KeReleaseSpinLock(&p_spin_lock->lock, (KIRQL)flags);
	return OSLAYER_OK;
}
#endif /* OSLAYER_KERNEL */
#endif /* OSLAYER_MISC */


#endif /* WIN32 */
