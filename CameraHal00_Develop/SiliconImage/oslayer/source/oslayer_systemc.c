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
/**
 * @file oslayer_systemc.h
 *
 * Encapsulates and abstracts services of SystemC
 *
 *****************************************************************************/
#ifdef LINUX

#ifdef MSVD_COSIM

#include "oslayer.h"
#include <sys/time.h>

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"

// replace MSVD_DEBUG1 (MSVD_DBG_ALL, ... by normal printf
#define MSVD_DEBUG1 fprintf
#define MSVD_DBG_ALL stdout

typedef struct _osScEvent
{
  sc_event ScEvent;
  int status;
} osScEvent;

typedef struct _osScMutex
{
  sc_mutex* ScMutex;
} osScMutex;

typedef struct _osScSemaphore
{
  sc_semaphore* ScSemaphore;
} osScSemaphore;

typedef struct _osScThreadHandle
{
  osEvent exit_event;
  sc_process_handle proc_handle;
  sc_spawn_options opts;
} osScThreadHandle;

int32_t g_result; // global result for dynamic SystemC thread

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
    /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    if(pEvent){
      pEvent->p_event = new osScEvent;
      pEvent->p_event->status = 0;
    }
    else{
      return OSLAYER_ERROR;
    }

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
    OSLAYER_STATUS Ret = OSLAYER_ERROR;

    /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    //    MSVD_DEBUG1(MSVD_DBG_ALL, g_msvd.debug_mask, "OS_SysC: send event %X", (int)pEvent);
    pEvent->p_event->ScEvent.notify();
    pEvent->p_event->status = 1;
    return Ret;
}


/******************************************************************************
 *  osEventReset()
 ******************************************************************************
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
 ******************************************************************************/
int32_t osEventReset(osEvent *pEvent)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;

   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    pEvent->p_event->ScEvent.cancel();
    pEvent->p_event->status = 0;
    Ret = OSLAYER_OK;
    return Ret;
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
    OSLAYER_STATUS Ret = OSLAYER_ERROR;

   /* check pointer */
    OSLAYER_ASSERT(pEvent == NULL);

    Ret = (OSLAYER_STATUS) osEventSignal(pEvent);

    return Ret;
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
    OSLAYER_STATUS Ret = OSLAYER_ERROR;

   /* check pointer */
    //    MSVD_DEBUG1(MSVD_DBG_ALL, g_msvd.debug_mask, "OS_SysC: wait event %X", (int)pEvent);
    OSLAYER_ASSERT(pEvent == NULL);
    if(pEvent->p_event->status == 0){
      wait(pEvent->p_event->ScEvent);
      //      MSVD_DEBUG1(MSVD_DBG_ALL, g_msvd.debug_mask, "OS_SysC: event received %X", (int)pEvent);
    }
    else{
      //      MSVD_DEBUG1(MSVD_DBG_ALL, g_msvd.debug_mask, "OS_SysC: event already active %X", (int)pEvent);
    }
    pEvent->p_event->status = 0;

    Ret = OSLAYER_OK;

    return Ret;
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
 ******************************************************************************/
int32_t osEventTimedWait(osEvent *pEvent, uint32_t msec)
{

  if (msec == 0) // check only if event already ocured
    {
      if (pEvent->p_event->status)
        {
          pEvent->p_event->status = 0;
          return OSLAYER_OK;
        }
      else return OSLAYER_TIMEOUT;
    }
  MSVD_DEBUG1(MSVD_DBG_ALL, "OS_SysC: Warning osEventTimedWait(*%X) called with time-out != 0 \n\tcalling osEventWait()\n", (int)pEvent);
  return  osEventWait(pEvent);

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

    pEvent->p_event->ScEvent.cancel();

    if(pEvent->p_event){
      delete pEvent->p_event;
      pEvent->p_event = (osScEvent*)0;
    }

    return OSLAYER_OK;
}
#endif /* OSLAYER_EVENT */



#ifdef OSLAYER_MUTEX

#ifdef __cplusplus
extern "C"
{
#endif
  extern sc_mutex* get_sc_mutex();
#ifdef __cplusplus
}
#endif

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

  pMutex->p_mutex = new osScMutex;
  pMutex->p_mutex->ScMutex = get_sc_mutex();

  if(!pMutex->p_mutex->ScMutex){
    return OSLAYER_ERROR;
  }

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
    OSLAYER_STATUS Ret = OSLAYER_ERROR;


	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    if(pMutex != NULL && pMutex->p_mutex == NULL){
      osMutexInit(pMutex);
    }

    if(!pMutex->p_mutex->ScMutex->trylock())
        Ret = OSLAYER_OK;
    else
        Ret = OSLAYER_SIGNAL_PENDING;

    return Ret;
}


/******************************************************************************
 *  osMutexUnlock()
 ******************************************************************************
 *  @brief  Unlock a mutex object.
 *
 *  @param  pMutex         Reference of the mutex object
 *
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Mutex successfully unlocked
 *
 ******************************************************************************/
int32_t osMutexUnlock(osMutex *pMutex)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;


	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    if(!pMutex->p_mutex->ScMutex->unlock())
        Ret = OSLAYER_OK;
    else
        Ret = OSLAYER_OPERATION_FAILED;

    return Ret;

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
 *
 ******************************************************************************/
int32_t osMutexTryLock(osMutex *pMutex)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;

	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    if(!pMutex->p_mutex->ScMutex->trylock())
	    Ret = OSLAYER_OK;
    else
	    Ret = OSLAYER_TIMEOUT;
    return Ret;
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
    OSLAYER_STATUS Ret = OSLAYER_ERROR;
	/* check pointer */
    OSLAYER_ASSERT(pMutex == NULL);

    if(pMutex->p_mutex){
      delete pMutex->p_mutex;
      pMutex->p_mutex = (osScMutex*)0;
    }

    Ret = OSLAYER_OK;

    return Ret;
}
#endif /* OSLAYER_MUTEX */

#ifdef OSLAYER_SEMAPHORE
/******************************************************************************
 *  osSemaphoreInit()
 ******************************************************************************
 *  @brief  Init a semaphore with init count.
 *
 *  @param  pSem           Reference of the semaphore object
 *
 *  @return                Status of operation
 *  @retval OSLAYER_OK     Semaphore successfully created
 *  @retval OSLAYER_ERROR  Semaphore is not created
 *
 ******************************************************************************/
int32_t osSemaphoreInit(osSemaphore *pSem, int32_t init_count)
{
	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    pSem->p_semaphore = new osScSemaphore;
    pSem->p_semaphore->ScSemaphore = new sc_semaphore(init_count);

    return OSLAYER_OK;
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
 *                                    semaphore
 *
 ******************************************************************************/
int32_t osSemaphoreTimedWait(osSemaphore *pSem, uint32_t msec)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;


	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    pSem->p_semaphore->ScSemaphore->wait();
    Ret = OSLAYER_OK;

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


	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    pSem->p_semaphore->ScSemaphore->wait();
    Ret = OSLAYER_OK;

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
    OSLAYER_STATUS Ret = OSLAYER_ERROR;


	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    if(!pSem->p_semaphore->ScSemaphore->trywait())
        Ret = OSLAYER_OK;
    else
        Ret = OSLAYER_TIMEOUT;
    return Ret;
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
 *
 ******************************************************************************/
int32_t osSemaphorePost(osSemaphore *pSem)
{
    OSLAYER_STATUS Ret = OSLAYER_ERROR;


	/* check pointer */
    OSLAYER_ASSERT(pSem == NULL);

    pSem->p_semaphore->ScSemaphore->post();
    Ret = OSLAYER_OK;

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

    if(pSem->p_semaphore){
      delete pSem->p_semaphore->ScSemaphore;
      pSem->p_semaphore->ScSemaphore = (sc_semaphore*)0;
      delete pSem->p_semaphore;
      pSem->p_semaphore = (osScSemaphore*)0;
    }


    return OSLAYER_OK;
}
#endif /* OSLAYER_SEMAPHORE */




#ifdef OSLAYER_THREAD
/******************************************************************************
 *  osThreadProc()
 ******************************************************************************
 *
 *  Wrapper for the thread function.
 *
 ******************************************************************************/
static void* osThreadProc(void *pParams)
{
    osThread *pThread = (osThread *)pParams;

    /* check error(s) */
    if(pParams == NULL)
        return ((void *) -1);

    pThread->p_handle->proc_handle = sc_spawn(sc_bind(pThread->pThreadFunc, pThread->p_arg), "OSLAYER_SC_THREAD");
    osEventSignal(&pThread->p_handle->exit_event);

    return ((void *) 1);
}

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
int32_t osThreadCreate(osThread *pThread, osThreadFunc thread_func/*int32_t (*osThreadFunc)(void *)*/, void *arg)
{
    int32_t res = OSLAYER_OPERATION_FAILED;
    /* check error(s) */
    OSLAYER_ASSERT(pThread == NULL);
    OSLAYER_ASSERT(osThreadFunc == NULL);

    if(pThread->p_handle){
      delete pThread->p_handle;
      pThread->p_handle = NULL;
    }
    pThread->p_handle    = new osScThreadHandle;
    //    pThread->p_handle->opts.dont_initialize();
    pThread->pThreadFunc = thread_func;
    pThread->p_arg       = arg;

    if(osEventInit(&pThread->p_handle->exit_event, 1, 0) != OSLAYER_OK)
        return OSLAYER_OPERATION_FAILED;
    pThread->p_handle->proc_handle = sc_spawn(&g_result, sc_bind(pThread->pThreadFunc, pThread->p_arg), "OSLAYER_SC_THREAD", (sc_spawn_options*)&(pThread->p_handle->opts));

    res = OSLAYER_OK;

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
    OSLAYER_STATUS res = OSLAYER_OK;

    return res;
}


/******************************************************************************
 *  osThreadWait()
 ******************************************************************************
 *  @brief  Wait until thread exits.
 *
 *          To avoid memory leak, always call osThreadWait() and wait for the
 *          child thread to terminate in the calling thread. Otherwise you
 *          have to cope with ZOMBIE threads under linux (memory resources
 *          still blocked and not reusable).
 *
 *  @param  pThread       Reference of the semaphore object
 *
 * @return                always OSLAYER_OK
 ******************************************************************************/
int32_t osThreadWait(osThread *pThread)
{
    /* check error(s) */
    OSLAYER_ASSERT(pThread == NULL);

    pThread->p_handle->proc_handle.wait();

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
    osEventDestroy(&pThread->p_handle->exit_event);

    if(pThread->p_handle){
      delete pThread->p_handle;
      pThread->p_handle = (osScThreadHandle*)0;
    }

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
#ifndef OSLAYER_KERNEL

    if (msec)
    {
    	if(msec > 1024)
    	{
    		sleep(msec >> 10);
    	}
    	else
    	{
        	sleep(1);
    	}
    }
    else
    {
        sched_yield();
    }
#else
    if(msec)
        schedule_timeout(msecs_to_jiffies(msec));
    else
        schedule();
#endif
    return OSLAYER_OK;
}


/******************************************************************************
 *  osGetTick()
 ******************************************************************************
 *  @brief  Obtain the clock tick.
 *
 *  @return            Current clock tick. The resoultion of clock tick can
 *                     be requested with @ref osGetFrequency
 *
 ******************************************************************************/
uint64_t osGetTick(void)
{
    struct timeval tv;

#ifndef OSLAYER_KERNEL
    gettimeofday(&tv, NULL);
#else
    do_gettimeofday(&tv);
#endif
    return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
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
    return (uint64_t)1000000;
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
#ifndef OSLAYER_KERNEL
	return malloc(size);
#else
	return kmalloc(size, GFP_USER);
#endif
}


/******************************************************************************
 *  osMallocEx()
 ******************************************************************************
 *  @brief  Allocate a continuous block of memory.
 *
 *  @param  size       Size of memory block to be allocated
 *
 *  @param  type       Type of memory block to be allocated
 *
 ******************************************************************************/
#ifdef OSLAYER_KERNEL
void* osMallocEx(uint32_t size, uint32_t type)
{
    if((uiType != OSLAYER_KERNEL) || (uiType != OSLAYER_USER) || (uiType != OSLAYER_ATOMIC))
        return NULL;

	return kmalloc(size, type);
}
#endif


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
#ifndef OSLAYER_KERNEL
	free(p);
#else
	kfree(p);
#endif
    return OSLAYER_OK;
}
#endif /* OSLAYER_MISC */

#ifdef OSLAYER_ATOMIC
#ifndef OSLAYER_KERNEL
static osMutex gAtomicMutex; /* variable to enable "atomic operations" in user mode */
#endif /* OSLAYER_KERNEL */

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
#ifndef OSLAYER_KERNEL
    /* initialize atomic mutex */
    osMutexInit(&gAtomicMutex);
#endif /* OSLAYER_KERNEL */
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
 ******************************************************************************/
int32_t osAtomicShutdown()
{
#ifndef OSLAYER_KERNEL
    /* destroy atomic mutex */
    osMutexDestroy(&gAtomicMutex);
#endif /* OSLAYER_KERNEL */
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
#ifndef OSLAYER_KERNEL
    uint32_t ret;

    OSLAYER_ASSERT(bitpos < 32);

    /* Lock the atomic mutex */
    (void)osMutexLock(&gAtomicMutex);

    /* Test bit */
    ret = *pVar & (1 << bitpos);
    /* Clear bit */
    *pVar &= ~(1 << bitpos);

    /* Unlock the atomic mutex */
    (void)osMutexUnlock(&gAtomicMutex);

    return ret;
#else  /* OSLAYER_KERNEL */
    /* TODO: implement it */
    OSLAYER_ASSERT(0);
#endif /* OSLAYER_KERNEL */
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
 + @param  bitpos         Bit to be set
 *
 + @return                always OSLAYER_OK
 ******************************************************************************/
int32_t osAtomicSetBit(uint32_t* pVar, uint32_t bitpos)
{
#ifndef OSLAYER_KERNEL
    OSLAYER_ASSERT(bitpos < 32);

    /* Lock the atomic mutex */
    (void)osMutexLock(&gAtomicMutex);

    /* Set bit */
    *pVar |= (1 << bitpos);

    /* Unlock the atomic mutex */
    (void)osMutexUnlock(&gAtomicMutex);
#else /* OSLAYER_KERNEL */
    /* TODO: implement it */
    OSLAYER_ASSERT(0);
#endif /* OSLAYER_KERNEL */
    return OSLAYER_OK;
}

/******************************************************************************
 *  osAtomicSet()
 ******************************************************************************
 * @brief  Set value atomically.
 *
 * Set variable to value. The operation is atomic (includes IRQ safety).
 *
 * @param  pVar           32-bit unsigned variable to be modified
 * @param  value          Value to be set
 *
 * @return                always OSLAYER_OK
 ******************************************************************************/
int32_t osAtomicSet(uint32_t* pVar, uint32_t value)
{
#ifndef OSLAYER_KERNEL
    /* Lock the atomic mutex */
    (void)osMutexLock(&gAtomicMutex);

    /* Set variable */
    *pVar = value;

    /* Unlock the atomic mutex */
    (void)osMutexUnlock(&gAtomicMutex);
#else /* OSLAYER_KERNEL */
    /* TODO: implement it */
    OSLAYER_ASSERT(0);
#endif /* OSLAYER_KERNEL */
    return OSLAYER_OK;
}

#endif /* OSLAYER_ATOMIC */

#endif /*MSVD_COSIM*/

#endif /* LINUX */
