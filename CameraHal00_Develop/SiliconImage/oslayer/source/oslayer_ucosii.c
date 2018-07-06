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
 * Module    : UCOSII Abstraction Layer
 *
 * Hierarchy :
 *
 * Purpose   : Encapsulates and abstracts services from different operating
 *             system, including user-mode as well as kernel-mode services.
 ******************************************************************************/
#ifdef UCOSII

#include <sys/alt_cache.h>
#include <ebase/trace.h>
#include "oslayer.h"

CREATE_TRACER(OSL_ERR, "OSL::Err: ", ERROR, BOOL_TRUE);
CREATE_TRACER(OSL_DBG, "OSL::Dbg: ", INFO, BOOL_FALSE);

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
    INT8U       err;
    OS_FLAGS    flags = 0u;

    OSLAYER_ASSERT(pEvent != NULL);

    if (InitState) {
        flags = 1u;
    }

    OSFlagQuery(pEvent->pFlagGroup, &err);
    if (err == OS_NO_ERR) {   /* Flags already initialized */
        osEventDestroy(pEvent); /* destroy before reusing */
    }

    pEvent->pFlagGroup = OSFlagCreate(flags, &err);
    pEvent->wait_type = Automatic ? OS_FLAG_CONSUME : 0u;

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U       err;

    OSLAYER_ASSERT(pEvent != NULL);

    OSFlagPost(pEvent->pFlagGroup, 1u, OS_FLAG_SET, &err);

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U       err;
    OS_FLAGS    flags;

    OSLAYER_ASSERT(pEvent != NULL);

    flags = OSFlagQuery(pEvent->pFlagGroup, &err);
    if(flags && (err == OS_NO_ERR))
    {
        OSFlagPost(pEvent->pFlagGroup, 1u, OS_FLAG_CLR, &err);
    }

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U       err;
    INT8U       opt;
    OS_FLAGS    flags;

    OSLAYER_ASSERT(pEvent != NULL);

    flags = OSFlagQuery(pEvent->pFlagGroup, &err);
    opt = flags ? OS_FLAG_CLR : OS_FLAG_SET;
    OSFlagPost(pEvent->pFlagGroup, 1u, opt, &err);

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U       err;

    OSLAYER_ASSERT(pEvent != NULL);

    (void) OSFlagPend(pEvent->pFlagGroup,
                1u,
                OS_FLAG_WAIT_SET_ALL + pEvent->wait_type,
                0u,
                &err);

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U           err;
    OSLAYER_STATUS  Ret = OSLAYER_ERROR;
    INT16U          ticks = (msec * alt_ticks_per_second()) / 1000u;

    OSLAYER_ASSERT(pEvent != NULL);

    if (!ticks) {  /* ensure minimal wait time */
        ticks = 1u;
    }

    (void) OSFlagPend(pEvent->pFlagGroup,
                1u,
                OS_FLAG_WAIT_SET_ALL + pEvent->wait_type,
                ticks,
                &err);

    if (err == OS_NO_ERR) {
        Ret = OSLAYER_OK;
    } else if (err == OS_TIMEOUT) {
        Ret = OSLAYER_TIMEOUT;
    } else {
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
    INT8U           err;

    OSLAYER_ASSERT(pEvent != NULL);

    (void) OSFlagDel(pEvent->pFlagGroup, OS_DEL_NO_PEND, &err);

    OSLAYER_ASSERT(err == OS_NO_ERR);

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
}
#endif /* OSLAYER_EVENT */







#ifdef OSLAYER_MUTEX
/******************************************************************************
 *  osMutextInit()
 ******************************************************************************
 *  @brief  Initialize a mutex object.
 *          We use semaphores to implement mutexes as they are more
 *          lightweighted in UCOSII, and we don'w encounter the tast priority
 *          problem.
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
    OSLAYER_ASSERT(pMutex != NULL);

    OS_SEM_DATA data;

    if (OS_NO_ERR == OSSemQuery(pMutex->semaphore, &data)) { /* semaphore already exist! */
        osMutexDestroy(pMutex);
    }

    pMutex->semaphore = OSSemCreate(1u);
    return pMutex->semaphore ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U           err;

    OSLAYER_ASSERT(pMutex != NULL);

    OSSemPend(pMutex->semaphore, 0u, &err);

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    OSLAYER_ASSERT(pMutex != NULL);

    return (OSSemPost(pMutex->semaphore) == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
 *  @retval OSLAYER_TIMEOUT           Mutex not locked (already locked by
 *                                    someone else)
 *
 ******************************************************************************/
int32_t osMutexTryLock(osMutex *pMutex)
{
    OSLAYER_ASSERT(pMutex != NULL);
    return OSSemAccept(pMutex->semaphore) ? OSLAYER_OK : OSLAYER_TIMEOUT;
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
    INT8U           err;

    OSLAYER_ASSERT(pMutex != NULL);

    (void) OSSemDel(pMutex->semaphore, OS_DEL_NO_PEND, &err);
    OSLAYER_ASSERT(err == OS_NO_ERR);
    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
    OSLAYER_ASSERT(pSem != NULL);

    OS_SEM_DATA data;

    if (OS_NO_ERR == OSSemQuery(pSem->semaphore, &data)) { /* semaphore already exist! */
        osSemaphoreDestroy(pSem);
    }
    pSem->semaphore = OSSemCreate((INT16U) init_count);
    return pSem->semaphore ? OSLAYER_OK : OSLAYER_ERROR;
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
    INT8U   err;
    INT16U  ticks = (msec * alt_ticks_per_second()) / 1000u;

    OSLAYER_ASSERT(pSem != NULL);

    if (!ticks) {  /* ensure minimal wait time */
        ticks = 1u;
    }

    OSSemPend(pSem->semaphore, ticks, &err);

    if (err == OS_NO_ERR) {
        return OSLAYER_OK;
    } else if (err == OS_TIMEOUT) {
        return OSLAYER_TIMEOUT;
    } else {
        return OSLAYER_ERROR;
    }
}


/******************************************************************************
 *  osSemaphoreWait()
 ******************************************************************************
 *  @brief  Decrease the semaphore value in blocking mode.
 *
 *  @param  pSem                      Reference of the semaphore object
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
    INT8U   err;

    OSLAYER_ASSERT(pSem != NULL);

    OSSemPend(pSem->semaphore, 0u, &err);
    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
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
 *  @retval OSLAYER_TIMEOUT           Wait for semaphore failed
 *  @retval OSLAYER_ERROR             Tried to wait for a not initialized
 *                                    semaphore
 *
 ******************************************************************************/
int32_t osSemaphoreTryWait(osSemaphore *pSem)
{
    OSLAYER_ASSERT(pSem != NULL);
    return OSSemAccept(pSem->semaphore) ? OSLAYER_OK : OSLAYER_TIMEOUT;
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
    OSLAYER_ASSERT(pSem != NULL);
    return (OSSemPost(pSem->semaphore) == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_OPERATION_FAILED;
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
    INT8U           err;

    OSLAYER_ASSERT(pSem != NULL);

    (void) OSSemDel(pSem->semaphore, OS_DEL_NO_PEND, &err);
    OSLAYER_ASSERT(err == OS_NO_ERR);
    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_ERROR;
}
#endif /* OSLAYER_SEMAPHORE */








#ifdef OSLAYER_ATOMIC

static osMutex gAtomicMutex; /* variable to enable "atomic operations" in user mode */

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
    /* initialize atomic mutex */
    osMutexInit(&gAtomicMutex);
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
    /* destroy atomic mutex */
    osMutexDestroy(&gAtomicMutex);
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
    OSLAYER_ASSERT(bitpos < 32);

    /* Lock the atomic mutex */
    (void)osMutexLock(&gAtomicMutex);

    /* Set bit */
    *pVar |= (1 << bitpos);

    /* Unlock the atomic mutex */
    (void)osMutexUnlock(&gAtomicMutex);
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
    /* Lock the atomic mutex */
    (void)osMutexLock(&gAtomicMutex);

    /* Set variable */
    *pVar = value;

    /* Unlock the atomic mutex */
    (void)osMutexUnlock(&gAtomicMutex);
    return OSLAYER_OK;
}
#endif /* OSLAYER_ATOMIC */





#ifdef OSLAYER_THREAD

#define N_PRIO_GROUPS   (5u)
#define MIN_TASK_PRIO   (OS_LOWEST_PRIO - 4u)
#define MAX_TASK_PRIO   (4u)
#define N_USER_TASKS    ((MIN_TASK_PRIO) - (MAX_TASK_PRIO))
#define PRIO_GROUP_SIZE ((N_USER_TASKS) / (N_PRIO_GROUPS))

DCT_ASSERT_STATIC(MAX_TASK_PRIO <= MIN_TASK_PRIO);

static INT8U    taskName[OS_TASK_NAME_SIZE];
#undef CONFIG_STATIC_STACK

#if defined(CONFIG_STATIC_STACK)
static OS_STK   taskStacks[N_USER_TASKS][UCOSII_STACK_SIZE];
static OS_MEM*  taskStackPool = NULL;
#endif

#undef  CONFIG_OSLAYER_STACK_DEBUG
#if defined(CONFIG_OSLAYER_STACK_DEBUG)
static void dumpStackUsage(const char* prefix)
{
    uint32_t    i;
    INT8U       err;
    OS_STK_DATA data;

    TRACE(OSL_DBG, "%s\n", prefix);
    for (i=MAX_TASK_PRIO; i<=MIN_TASK_PRIO; ++i) {
        /* we use the error return parameter of OSTaskNameGet() to
         * decide if the task priority is free:
         */
        (void) OSTaskNameGet(i, taskName, &err);
        if (err == OS_NO_ERR) {
            if (OS_NO_ERR == OSTaskStkChk(i, &data)) {
                TRACE(OSL_DBG, "  Task %ld: Stack used: %ld, stack free: %ld.\n", i, data.OSUsed, data.OSFree);
            } else {
                TRACE(OSL_DBG, "  OSTaskStkChk for task %ld failed with err: %d.\n", i, err);
            }
        }
    }
}
#endif  /* #if defined(CONFIG_OSLAYER_STACK_DEBUG) */

/******************************************************************************
 *  osThreadProc()
 ******************************************************************************
 *
 *  Wrapper for the thread function.
 *
 ******************************************************************************/
static void osThreadProc(void *pParams)
{
    INT8U       err;
    osThread*   pThread = (osThread *)pParams;

    OSLAYER_ASSERT(pParams != NULL);

    /* call working thread: */
    (void) pThread->pThreadFunc(pThread->p_arg);

#if defined(CONFIG_OSLAYER_STACK_DEBUG)
    dumpStackUsage("Predelete: ");
#endif  /* #if defined(CONFIG_OSLAYER_STACK_DEBUG) */

    /* when execution comes to this point, the task deletes itself:  */
    err = OSTaskDel(OS_PRIO_SELF);

    /* execution must not reach this point! */
    OSLAYER_ASSERT(0);
}



static INT8U osThreadGetFreePriority(int32_t priorityGroup)
{
    INT8U   err;
    INT8U   i;
    INT8U   startPrio = MIN_TASK_PRIO + 1u;

    switch (priorityGroup) {
        case OSLAYER_THREAD_PRIO_HIGHEST:
            startPrio = MAX_TASK_PRIO;
            break;
        case OSLAYER_THREAD_PRIO_HIGH:
            startPrio = MAX_TASK_PRIO + PRIO_GROUP_SIZE;
            break;
        case OSLAYER_THREAD_PRIO_NORMAL:
            startPrio = MAX_TASK_PRIO + (PRIO_GROUP_SIZE * 2u);
            break;
        case OSLAYER_THREAD_PRIO_LOW:
            startPrio = MAX_TASK_PRIO + (PRIO_GROUP_SIZE * 3u);
            break;
        case OSLAYER_THREAD_PRIO_LOWEST:
            startPrio = MAX_TASK_PRIO + (PRIO_GROUP_SIZE * 4u);
            break;
        default:
            break;
    }

    for (i=startPrio; i<=MIN_TASK_PRIO; i++) {
        /* we use the error return parameter of OSTaskNameGet() to
         * decide if the task priority is free:
         */
        (void) OSTaskNameGet(i, taskName, &err);
        if (err != OS_NO_ERR) {
            break;  /* found empty task slot, or end of available task slots */
        }
    }

    if (i <= MIN_TASK_PRIO) {
        return i;
    }
    return OS_PRIO_SELF;    /* serves a error indication here */
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
int32_t osThreadCreate(osThread *pThread, osThreadFunc thread_func, void *arg)
{
    INT8U           err;

    /* check error(s) */
    OSLAYER_ASSERT(pThread != NULL);
    OSLAYER_ASSERT(thread_func != NULL);

#if defined(CONFIG_STATIC_STACK)
    if (!taskStackPool) {
        taskStackPool = OSMemCreate(taskStacks, N_USER_TASKS, UCOSII_STACK_SIZE * sizeof(OS_STK), &err);
        if (!taskStackPool) {
            TRACE(OSL_ERR, "Initializing Stack pool failed, Err: %d.\n", err);
            OSLAYER_ASSERT(0);
        }
    }
#endif

    pThread->pThreadFunc = thread_func;
    pThread->p_arg = arg;

    pThread->prio = osThreadGetFreePriority(OSLAYER_THREAD_PRIO_LOWEST);
    if (OS_PRIO_SELF == pThread->prio) {
        return OSLAYER_OPERATION_FAILED;
    }

    pThread->stackSize = UCOSII_STACK_SIZE;
#if defined(CONFIG_STATIC_STACK)
    pThread->stack = OSMemGet(taskStackPool, &err);
    {
        OS_MEM_DATA mem_data;
        OSMemQuery(taskStackPool, &mem_data);
#if 0
        TRACE(OSL_DBG, "Memory Pool at %p: %ld blocks of %ld byte, %ld used, %ld free.\n",
                mem_data.OSAddr,
                mem_data.OSNBlks,
                mem_data.OSBlkSize,
                mem_data.OSNUsed,
                mem_data.OSNFree);
#endif
    }
#else
    pThread->stack = malloc(pThread->stackSize * sizeof(OS_STK));
#endif
    if (!pThread->stack) {
        TRACE(OSL_ERR, "Allocating task stack failed.\n");
        OSLAYER_ASSERT(0);
        return OSLAYER_OPERATION_FAILED;
    }

    err = OSTaskCreateExt(  osThreadProc,
                            (void*) pThread,
                            (void *) &(pThread->stack[pThread->stackSize-1]),
                            pThread->prio,
                            pThread->prio,
                            pThread->stack,
                            pThread->stackSize,
                            NULL,
                            OS_TASK_OPT_STK_CHK + OS_TASK_OPT_STK_CLR);

    TRACE(OSL_DBG, "Created task %d, stack: %p - %p, result: %d.\n", pThread->prio, pThread->stack, pThread->stack + pThread->stackSize, err);
#if defined(CONFIG_OSLAYER_STACK_DEBUG)
    dumpStackUsage("PostCreate: ");
#endif  /* #if defined(CONFIG_OSLAYER_STACK_DEBUG) */

    return (err == OS_NO_ERR) ? OSLAYER_OK : OSLAYER_OPERATION_FAILED;
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
    INT8U err;
    INT8U new_prio = osThreadGetFreePriority(priority);

    OSLAYER_ASSERT(pThread != NULL);

    if (OS_PRIO_SELF == new_prio) {
        return OSLAYER_OPERATION_FAILED;
    }

    err = OSTaskChangePrio(pThread->prio, new_prio);
    if (OS_NO_ERR == err) {
        pThread->prio = new_prio;
        return OSLAYER_OK;
    }
    return OSLAYER_OPERATION_FAILED;
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
    INT8U   err;

    OSLAYER_ASSERT(pThread != NULL);

    while ((err = OSTaskDelReq(pThread->prio)) != OS_TASK_NOT_EXIST) {
        if (err != OS_NO_ERR) {
            return OSLAYER_OPERATION_FAILED;
        }
        OSTimeDly(1u);
    }

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
    OSLAYER_ASSERT(pThread != NULL);

    if (OSLAYER_OK != osThreadWait(pThread)) {
        return OSLAYER_OPERATION_FAILED;
    }

    //TRACE(OSL_DBG, "Freeing task stack at: %p\n", pThread->stack);
#if defined(CONFIG_STATIC_STACK)
    if (OS_NO_ERR != OSMemPut(taskStackPool, (void *) pThread->stack)) {
        TRACE(OSL_ERR, "Freeing task stack failed!\n");
    } else {
        OS_MEM_DATA mem_data;
        OSMemQuery(taskStackPool, &mem_data);
#if 0
        TRACE(OSL_DBG, "Memory Pool at %p: %ld blocks of %ld byte, %ld used, %ld free.\n",
                mem_data.OSAddr,
                mem_data.OSNBlks,
                mem_data.OSBlkSize,
                mem_data.OSNUsed,
                mem_data.OSNFree);
#endif
    }
#else
    free(pThread->stack);
#endif
    pThread->stack = NULL;
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
    uint32_t    time = msec;
    INT8U       hours;
    INT8U       minutes;
    INT8U       seconds;

    if (msec <= 65535)
    {
        INT16U  ticks = (alt_ticks_per_second() * msec) /  1000u;
        if (!ticks) {   /* wait at least one timer tick */
            ticks = 1u;
        }
        OSTimeDly(ticks);
    } else {
        msec = time % 1000u;
        time /= 1000u;

        seconds = time % 60u;
        time /= 60u;

        minutes = time % 60u;
        time /= 60u;

        hours = time;

        OSTimeDlyHMSM(hours, minutes, seconds, (INT8U) msec);
    }
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
    return (uint64_t) alt_nticks();
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
    return (uint64_t) alt_ticks_per_second();
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

#endif /* UCOSII */
