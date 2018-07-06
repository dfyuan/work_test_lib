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
 * Module    : Operating System Abstraction Layer
 *
 * Hierarchy :
 *
 * Purpose   : Encapsulates and abstracts services from different operating
 *             system, including user-mode as well as kernel-mode services.
 ******************************************************************************/
#ifdef MFS
/******************************************************************************
* INCLUDES
*****************************************************************************/
#include "oslayer.h"

#include <hpi_sys.h>

#include <kernel/system.h>
#include <kernel/ct.h>
#include <kernel/mutex.h>
#include <kernel/mailbox.h>
#include <kernel/timer.h>

#include <lib/utils.h>

#include <ebase/trace.h>
#include <ebase/dct_assert.h>


/******************************************************************************
* LOCAL DEFINITIONS
*****************************************************************************/
#ifndef OSLAYER_MBOX_EVENT_0
#define OSLAYER_MBOX_EVENT_0   0
#endif

#ifndef OSLAYER_MBOX_SEMA_0
#define OSLAYER_MBOX_SEMA_0    0
#endif


#ifndef OSLAYER_MUTEX_SEMA_0
#define OSLAYER_MUTEX_SEMA_0   0
#endif


#ifndef OSLAYER_MUTEX_ID_0
#define OSLAYER_MUTEX_ID_0     0
#endif


/******************************************************************************
* GLOBALS
*****************************************************************************/
static int32_t g_oslayer_event_count        = 0;
static int32_t g_oslayer_semaphore_count    = 0;
static int32_t g_oslayer_mutex_count        = 0;

static OSLAYER_STATUS g_oslayer_last_result = OSLAYER_OK;

CREATE_TRACER(OS_INFO, "OSL MFS: ", INFO, 0);
USE_TRACER(OS_INFO);


/******************************************************************************
 *  osLayerMFSInit()
 ******************************************************************************
 *  @brief  Function called at startup of MFS system.
 *
 ******************************************************************************/
void osLayerMFSInit(void)
{
    int32_t i;

    TRACE(OS_INFO, "NUMBER_OF_USERS = %d\n", NUMBER_OF_USERS);

    for (i = 0; i < NUMBER_OF_USERS; i++)
    {
        /* mark all os layer instance records as "unassigned" */
        osInstanceRecord* p_inst_rec = GET_INSTANCE_RECORD(os_instance_record, i);
        p_inst_rec->act_id = (aid_t) -1;
    }
}



/******************************************************************************
 *  osLayerMFSGetInstRec()
 ******************************************************************************
 *  @brief  Function retrieving an instance record for a
 *          user of the oslayer (an activity)
 *
 ******************************************************************************/
static osInstanceRecord* osLayerMFSGetInstRec()
{
    int32_t i;
    aid_t act_id = getCurrentAid();

    for (i = 0; i < NUMBER_OF_USERS; i++)
    {
        /* search for instance record of activity or unassigned instance record */
        osInstanceRecord* pInstRec = GET_INSTANCE_RECORD(os_instance_record, i);

        if (pInstRec->act_id == (aid_t) -1)
        {
            /* this activity does not yet have an os layer instance record,
               so assign one to it */
            pInstRec->act_id   = act_id;
            pInstRec->timer_id = OSLAYER_USER_TIMER_0 + i;
            timerAssign(pInstRec->timer_id, act_id);
            return pInstRec;
        }

        if (pInstRec->act_id == act_id)
        {
            /* this activity already has an os layer instance record */
            return pInstRec;
        }
    }

    /* someone forgot to order an os layer instance record in his tab file */
    OSLAYER_ASSERT(0);
    return NULL;
}



/******************************************************************************
 * osAssignMailboxRxWakeUp ()
 ******************************************************************************
 *  @brief: Assign a Mailbox Activity to be woken up upon Receiving (MFS Only)
 *
 *  @param  Mailbox Id
 *  @return None
 ******************************************************************************/
void  osAssignMailboxRxWakeUp (mbId_t  m_id)
{
    aid_t  act_id = getCurrentAid();
    mbAssign(m_id, act_id);
    TRACE(OS_INFO,  "osAssignMailboxRxWakeUp: assigned m_id %d to activity %d\n", m_id, act_id);
}


/******************************************************************************
 * getNextEventId ()
 ******************************************************************************
 *  @brief  get next Event Id
 *
 *  @param  none
 *  @return Event Id
 ******************************************************************************/
mbId_t getNextEventId (void)
{
    mbId_t m_id;

    OSLAYER_ASSERT(g_oslayer_event_count < NUMBER_OF_EVENTS);
    m_id = OSLAYER_MBOX_EVENT_0 + g_oslayer_event_count;
    g_oslayer_event_count += 1;

    return m_id;
}

/******************************************************************************
 * checkEventIdUsed
 ******************************************************************************
 *  @brief  check if event id is currently in use
 *
 *  @param  Event Id to be tested
 *  @return 1 if id is in use, 0 else
 ******************************************************************************/
int32_t checkEventIdUsed(mbId_t id)
{
    if ((id >= OSLAYER_MBOX_EVENT_0) && (id < (OSLAYER_MBOX_EVENT_0 + g_oslayer_event_count)))
    {
        return 1;
    }
    return 0;
}

/******************************************************************************
 * getNextSemaphoreId ()
 ******************************************************************************
 *  @brief  get next ids for semaphore mailbox and mutex
 *
 *  @param  p_mailbox_id pointer to mailbox id to be set
 *  @param  p_mutex_id   pointer to mutex id to be set
 *  @return void
 ******************************************************************************/
void getNextSemaphoreIds (mbId_t* p_mailbox_id, mutexId_t* p_mutex_id)
{
    OSLAYER_ASSERT(g_oslayer_semaphore_count < NUMBER_OF_SEMAPHORES);
    *p_mailbox_id = OSLAYER_MBOX_SEMA_0 + g_oslayer_semaphore_count;
    *p_mutex_id   = OSLAYER_MUTEX_SEMA_0 + g_oslayer_semaphore_count;
    g_oslayer_semaphore_count += 1;
}

/******************************************************************************
 * checkSemaphoreIdUsed
 ******************************************************************************
 *  @brief  check if event id is currently in use
 *
 *  @param  Semaphore Id to be tested
 *  @return 1 if id is in use, 0 else
 ******************************************************************************/
int32_t checkSemaphoreIdsValidAndUsed(mbId_t mailbox_id, mutexId_t mutex_id)
{
    if (((mailbox_id >= OSLAYER_MBOX_SEMA_0) && (mailbox_id < (OSLAYER_MBOX_SEMA_0 + g_oslayer_semaphore_count))) &&
        ((mutex_id >= OSLAYER_MUTEX_SEMA_0) && (mutex_id < (OSLAYER_MUTEX_SEMA_0 + g_oslayer_semaphore_count))) &&
        ((mailbox_id - OSLAYER_MBOX_SEMA_0) == (mutex_id - OSLAYER_MUTEX_SEMA_0)))
    {
        return 1;
    }
    return 0;
}

/******************************************************************************
 * getNextMutexId ()
 ******************************************************************************
 *  @brief  get next Mutex Id
 *
 *  @param  none
 *  @return Mutex Id
 ******************************************************************************/
mutexId_t getNextMutexId (void)
{
    mutexId_t mutex_id;

    OSLAYER_ASSERT(g_oslayer_mutex_count < NUMBER_OF_MUTEXES);
    mutex_id = OSLAYER_MUTEX_ID_0 + g_oslayer_mutex_count;
    g_oslayer_mutex_count += 1;

    return mutex_id;
}

/******************************************************************************
 * checkMutexIdUsed
 ******************************************************************************
 *  @brief  check if event id is currently in use
 *
 *  @param  Mutex Id to be tested
 *  @return 1 if id is in use, 0 else
 ******************************************************************************/
int32_t checkMutexIdUsed(mbId_t id)
{
    if ((id >= OSLAYER_MUTEX_ID_0) && (id < (OSLAYER_MUTEX_ID_0 + g_oslayer_mutex_count)))
    {
        return 1;
    }
    return 0;
}


/******************************************************************************
 * osGetLastResult ()
 ******************************************************************************
 *  @brief  get result from last OS function call
 *
 *  @param  none
 *  @return                OS layer Status of operation
 ******************************************************************************/
int32_t osGetLastResult (void)
{
    return  g_oslayer_last_result;
}


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
    /* check pointers */
    OSLAYER_ASSERT(pEvent != NULL);

    /* check if event is already initialized: */
    if (!(checkEventIdUsed(pEvent->m_id) && (OS_EVENT_INITIALIZED == pEvent->initialized)))
    {
        pEvent->m_id   = getNextEventId ();
    }

    pEvent->act_id = getCurrentAid();
    pEvent->state = InitState;
    pEvent->cond  = &pEvent->status;
    pEvent->automatic_reset = Automatic;
    pEvent->initialized = OS_EVENT_INITIALIZED;

    mbAssign (pEvent->m_id, pEvent->act_id);

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
    OSLAYER_ASSERT(pEvent != NULL);

    if  (OS_EVENT_INITIALIZED != pEvent->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    if(!pEvent->state)
    {
        struct system  *sys = GET_SYS();
        int32_t         shallRunAgain = sys->awakened;
        pEvent->state = 1;
        (void) mbSend (pEvent->m_id, &pEvent->cond);
       if (sys->awakened && !shallRunAgain)
       {    /* don't schedule again if we sent the signal to ourself */
           sys->awakened = 0;
       }

    }

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
}


/******************************************************************************
 *  osEventReset()
 ******************************************************************************
 *   @brief  Reset the event state to true.
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
    /* check pointer */
    OSLAYER_ASSERT(pEvent != NULL);

    if  (OS_EVENT_INITIALIZED != pEvent->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    pEvent->state = 0;
    mbRecv(pEvent->m_id, &pEvent->cond);

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
    OSLAYER_ASSERT(pEvent != NULL);

    if  (OS_EVENT_INITIALIZED != pEvent->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    /* pulse event by sending entry to mailbox */
    (void) mbSend (pEvent->m_id, &pEvent->cond);
    pEvent->state = 0;

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
    OSLAYER_ASSERT(pEvent != NULL);

    if  (OS_EVENT_INITIALIZED != pEvent->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    g_oslayer_last_result = OSLAYER_OK;

    TRACE(OS_INFO, "osEventWait, act_id %d\n", getCurrentAid());

    if(!pEvent->state)
    {
        runState_t state;
        if (getCurrentAid() != pEvent->act_id)
        {
            pEvent->act_id = getCurrentAid();
            mbAssign (pEvent->m_id, pEvent->act_id);
            TRACE(OS_INFO, "reassigned Event MB %d to Act: %d\n", pEvent->m_id, pEvent->act_id);
        }
        state = mbRecv(pEvent->m_id, &pEvent->cond);
        if (SCHED_BUSY(state))
        {
            TRACE(OS_INFO, "osEventWait, return state %d\n", state);
            return state;
        }
    }

    if (pEvent->automatic_reset)
    {
        pEvent->state = 0;
    }

    return CT_DONE;
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
 *
 *  @retval OSLAYER_OPERATION_FAILED  Wait for event failed
 *
 ******************************************************************************/
int32_t osEventTimedWait(osEvent *pEvent, uint32_t msec)
{
    osInstanceRecord* pInstRec = osLayerMFSGetInstRec();

    g_oslayer_last_result = OSLAYER_OK;

    CT_BEGIN(pInstRec->ct_state);

    /* check pointer */
    OSLAYER_ASSERT(pEvent != NULL);

    if  (OS_EVENT_INITIALIZED != pEvent->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        CT_RETURN_DONE();
    }

    TRACE(OS_INFO, "osEventTimedWait with timeout %d msecs\n", msec);

    if (msec == 0)
    {
        runState_t state;
        state = mbRecv(pEvent->m_id, &pEvent->cond);
        if (SCHED_BUSY(state))
        {
            g_oslayer_last_result = OSLAYER_TIMEOUT;
        }
    }
    else
    {
        int32_t abort;
        timerSet(pInstRec->timer_id, (HZ * msec) / 1000);

        if (getCurrentAid() != pEvent->act_id)
        {
            pEvent->act_id = getCurrentAid();
            mbAssign (pEvent->m_id, pEvent->act_id);
            TRACE(OS_INFO, "reassigned Event MB %d to Act: %d\n", pEvent->m_id, pEvent->act_id);
        }
        CT_WAIT_ABORTABLE( mbRecv(pEvent->m_id, (void **) &pEvent->cond),
                           timerIsExpired(pInstRec->timer_id), abort);

        if (abort == 0)
        {
            timerCancel (pInstRec->timer_id);
        }
        else
        {
            g_oslayer_last_result = OSLAYER_TIMEOUT;
        }
    }

    if (pEvent->automatic_reset)
    {
        pEvent->state = 0;
    }

    CT_END();

    return CT_DONE;
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
 *****************************************************************************/
int32_t  osEventDestroy(osEvent *pEvent)
{
    /* check pointer */
    OSLAYER_ASSERT(pEvent != NULL);

    if  (OS_EVENT_INITIALIZED != pEvent->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    mbRecv(pEvent->m_id, &pEvent->cond);

    if (g_oslayer_event_count > 0)
    {
       g_oslayer_event_count --;
    }
    pEvent->initialized = 0;

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
 *****************************************************************************/
int32_t osMutexInit(osMutex *pMutex)
{
    /* check pointer */
    OSLAYER_ASSERT(pMutex != NULL);

    /* check if mutex is already initialized: */
    if (!(checkMutexIdUsed(pMutex->mutex) && (OS_MUTEX_INITIALIZED == pMutex->initialized)))
    {
        pMutex->mutex = getNextMutexId ();
    }

    pMutex->initialized = OS_MUTEX_INITIALIZED;
    mutexRelease(pMutex->mutex); /* Release Previous - MFS has no initialisation */

    g_oslayer_last_result = OSLAYER_OK;

    return(CT_DONE);
}

/******************************************************************************
 *  osMutexLock()
 ******************************************************************************
 *  @brief  Lock a mutex object.
 *
 *         Lock the mutex. Thread will be blocked if mutex already locked.
 *
 *  @param  pMutex         Reference of the mutex object
 *
 *  @return                Status of operation
 *  retval OSLAYER_OK     Mutex successfully locked
 *
 ******************************************************************************/
int32_t osMutexLock(osMutex *pMutex)
{
    ctStatus_t state;

    g_oslayer_last_result = OSLAYER_OK;

    /* check pointer */
    OSLAYER_ASSERT(pMutex != NULL);

    if (OS_MUTEX_INITIALIZED != pMutex->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    state = mutexAcquire (pMutex->mutex);

    if (SCHED_BUSY(state))
    {
        if (mutexOwner(pMutex->mutex) != getCurrentAid())
        {
            return state;
        }
    }

    return CT_DONE;
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
    /* check pointer */
    OSLAYER_ASSERT(pMutex != NULL);

    if (OS_MUTEX_INITIALIZED != pMutex->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    if (mutexOwner(pMutex->mutex) != getCurrentAid())
    {
       g_oslayer_last_result = OSLAYER_OPERATION_FAILED;
    }
    else
    {
        mutexRelease(pMutex->mutex);
        g_oslayer_last_result = OSLAYER_OK;
    }

    return CT_DONE;
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
    ctStatus_t state;

    /* check pointer */
    OSLAYER_ASSERT(pMutex != NULL);

    if (OS_MUTEX_INITIALIZED != pMutex->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    state = mutexAcquire (pMutex->mutex);
    g_oslayer_last_result = OSLAYER_OK;
    if (SCHED_BUSY(state))
    {
        if (mutexOwner(pMutex->mutex) != getCurrentAid())
        {
            g_oslayer_last_result = OSLAYER_OPERATION_FAILED;
        }
    }

    return CT_DONE;
}


/******************************************************************************
 *  osMutexDestroy()
 ******************************************************************************
 *  @brief  Destroy a mutex object.
 *
 *  @param  pMutex                    Reference of the mutex object
 *
 ******************************************************************************/
int32_t osMutexDestroy(osMutex *pMutex)
{
    /* check pointer */
    OSLAYER_ASSERT(pMutex != NULL);

    if (OS_MUTEX_INITIALIZED != pMutex->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    if (g_oslayer_mutex_count > 0)
    {
       g_oslayer_mutex_count --;
    }
    pMutex->initialized = 0;

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
    OSLAYER_ASSERT(pSem != NULL);

    /* check if semaphore is already initialized: */
    if (!(checkSemaphoreIdsValidAndUsed(pSem->m_id, pSem->mutex) && (OS_SEMAPHORE_INITIALIZED == pSem->initialized)))
    {
        getNextSemaphoreIds(&pSem->m_id, &pSem->mutex);
    }

    pSem->act_id = getCurrentAid();
    pSem->count  = init_count;
    pSem->cond   = &pSem->status;
    pSem->initialized = OS_SEMAPHORE_INITIALIZED;
    mbAssign (pSem->m_id, pSem->act_id);

    g_oslayer_last_result = OSLAYER_OK;

    return(CT_DONE);
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
 *****************************************************************************/
int32_t osSemaphoreTimedWait(osSemaphore *pSem, uint32_t msec)
{
    osInstanceRecord* pInstRec = osLayerMFSGetInstRec();
    g_oslayer_last_result = OSLAYER_OK;

    CT_BEGIN(pInstRec->ct_state);

    /* check pointer */
    OSLAYER_ASSERT(pSem != NULL);

    if (OS_SEMAPHORE_INITIALIZED != pSem->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        CT_RETURN_DONE();
    }

    CT_WAIT(mutexAcquire(pSem->mutex));

    if(pSem->count == 0)
    {
        /* TODO: we cannot just use one timer here, we need one timer per activity! */
        int32_t abort;
        timerSet(pInstRec->timer_id, (HZ * msec) / 1000);

        if (getCurrentAid() != pSem->act_id)
        {
            pSem->act_id = getCurrentAid();
            mbAssign (pSem->m_id, pSem->act_id);
            TRACE(OS_INFO, "reassigned Semaphore MB %d to Act: %d\n", pSem->m_id, pSem->act_id);
        }
        CT_WAIT_ABORTABLE( mbRecv(pSem->m_id, (void **) &pSem->cond),
                            timerIsExpired(pInstRec->timer_id), abort);
        if (abort==0) {
            timerCancel(pInstRec->timer_id);
        }
        else
        {
            g_oslayer_last_result = OSLAYER_TIMEOUT;
        }
    }
    else
    {
        (void) mbRecv(pSem->m_id, (void *) &pSem->cond);
        pSem->count--;
    }

    mutexRelease (pSem->mutex);

    CT_END();

    return CT_DONE;
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
    ctStatus_t state;
    osInstanceRecord* pInstRec = osLayerMFSGetInstRec();
    g_oslayer_last_result = OSLAYER_OK;

    CT_BEGIN(pInstRec->ct_state);

    /* check pointer */
    OSLAYER_ASSERT(pSem != NULL);

    if (OS_SEMAPHORE_INITIALIZED != pSem->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        CT_RETURN_DONE();
    }

    CT_WAIT(mutexAcquire(pSem->mutex));

    if (getCurrentAid() != pSem->act_id)
    {
        pSem->act_id = getCurrentAid();
        mbAssign (pSem->m_id, pSem->act_id);
        TRACE(OS_INFO, "reassigned Semaphore MB %d to Act: %d\n", pSem->m_id, pSem->act_id);
    }
    state = mbRecv(pSem->m_id, (void *) &pSem->cond);
    if (SCHED_BUSY(state))
    {
        mutexRelease (pSem->mutex);
        return state;
    }

    if (pSem->count == 0)
    {
        g_oslayer_last_result = OSLAYER_OPERATION_FAILED;
    }
    else
    {
       pSem->count--;
    }

    mutexRelease (pSem->mutex);

    CT_END();

    return CT_DONE;
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
 +  @retval OSLAYER_TIMEOUT           Wait for semaphore succeeded and
 *                                    function returned due to timeout and
 *                                    no signal was sent
 +  @retval OSLAYER_OPERATION_FAILED  Wait for semaphore failed
 *  @retval OSLAYER_ERROR             Tried to wait for a not initialized
 *                                    semaphore
 *
 *****************************************************************************/
int32_t osSemaphoreTryWait(osSemaphore *pSem)
{
    osInstanceRecord* pInstRec = osLayerMFSGetInstRec();
    g_oslayer_last_result = OSLAYER_OK;

    CT_BEGIN(pInstRec->ct_state);

    /* check pointer */
    OSLAYER_ASSERT(pSem != NULL);

    if (OS_SEMAPHORE_INITIALIZED != pSem->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        CT_RETURN_DONE();
    }

    CT_WAIT(mutexAcquire (pSem->mutex));

    if(pSem->count == 0)
    {
        g_oslayer_last_result = OSLAYER_TIMEOUT;
    }
    else
    {
        mbRecv(pSem->m_id, (void *) &pSem->cond);
        pSem->count--;
    }

    mutexRelease (pSem->mutex);

    CT_END();

    return CT_DONE;
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
    /* The following behaviour is simulated:
     * The application can create a semaphore with an initial count of zero.
     * This sets the semaphore's state to nonsignaled and blocks all threads
     * from accessing the protected resource. When the application finishes its
     * initialization, it uses osSemaphorePost() to increase the count to
     * its maximum value, to permit normal access to the protected resource.
     */

    /* check pointer */
    OSLAYER_ASSERT(pSem != NULL);

    if (OS_SEMAPHORE_INITIALIZED != pSem->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    if(pSem->count++ == 0)
    {
        struct system  *sys = GET_SYS();
        int32_t         shallRunAgain = sys->awakened;

        (void) mbSend(pSem->m_id, (void *) &pSem->cond);
        if (sys->awakened && !shallRunAgain)
        {    /* don't schedule again if we sent the signal to ourself */
            sys->awakened = 0;
        }
    }

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
}

/*****************************************************************************
 *  osSemaphoreDestroy()
 *****************************************************************************
 *  @brief  Destroy the semaphore object.
 *
 *  @param  pSem                      Reference of the semaphore object
 *
 ****************************************************************************/
int32_t osSemaphoreDestroy(osSemaphore *pSem)
{
    /* check pointer */
    OSLAYER_ASSERT(pSem != NULL);

    if (OS_SEMAPHORE_INITIALIZED != pSem->initialized)
    {
        g_oslayer_last_result = OSLAYER_ERROR;
        return CT_DONE;
    }

    if (g_oslayer_semaphore_count > 0)
    {
       g_oslayer_semaphore_count --;
    }
    pSem->initialized = 0;

	g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
 *****************************************************************************/
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
    uint32_t ret;

    OSLAYER_ASSERT(bitpos < 32);

    /* Test bit */
    ret = *pVar & (1 << bitpos);
    /* Clear bit */
    *pVar &= ~(1 << bitpos);

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
 * @param  bitpos         Bit to be set
 *
 ******************************************************************************/
int32_t osAtomicSetBit(uint32_t* pVar, uint32_t bitpos)
{
    OSLAYER_ASSERT(bitpos < 32);

    /* Set bit */
    *pVar |= (1 << bitpos);

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
 ******************************************************************************/
int32_t osAtomicSet(uint32_t* pVar, uint32_t value)
{
    /* Set variable */
    *pVar = value;

    return OSLAYER_OK;
}
#endif /* OSLAYER_ATOMIC */









#ifdef OSLAYER_THREAD
/******************************************************************************
 *  osThreadCreate()
 ******************************************************************************
 *  @brief  Create a thread.
 *
 *  @param  pThread                   Reference of the semaphore object
 *  @param  activity_id               Normally tread function, in this case activity id
 *  @param  p_arg                     Normally pointer to be passed to thread function, unused here
 *
 *  @return                           Status of operation
 *  @retval OSLAYER_OK                Thread object created successfully
 *  @retval OSLAYER_OPERATION_FAILED  Creation of thread object failed
 *
 ******************************************************************************/
int32_t osThreadCreate(osThread *pThread, osThreadFunc activity_id, void *p_arg)
{
    OSLAYER_ASSERT(pThread != NULL);

    (void) p_arg;

    /* convention is that the activity id is passed in the thread_func parameter */
    pThread->act_id = activity_id;

    if (getCurrentAid() != pThread->act_id)
    {
        setRunnable(pThread->act_id);
    }

    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
}


/******************************************************************************
 *   osThreadSetPriority()
 *******************************************************************************
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
int32_t  osThreadSetPriority(osThread *pThread, OSLAYER_THREAD_PRIO priority)
{
    OSLAYER_ASSERT(pThread != NULL);

    (void) pThread;
    (void) priority;

    /* not supported by MFS OS layer so far, but still no error */
    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
 +  @param  pThread                   Reference of the semaphore object
 *
 *****************************************************************************/
int32_t osThreadWait(osThread *pThread)
{
    (void) pThread;

    /* no effect in MFS OS layer so far, but still no error */
    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
}

/******************************************************************************
 *  osThreadClose()
 ******************************************************************************
 *  @brief  Destroy thread object.
 *
 *  @param  pThread       Reference of the semaphore object
 *
 *****************************************************************************/
int32_t osThreadClose(osThread *pThread)
{
    (void) pThread;

    /* no effect in MFS OS layer so far, but still no error */
    g_oslayer_last_result = OSLAYER_OK;

    return CT_DONE;
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
 ******************************************************************************/
int32_t osSleep (uint32_t msec)
{
    osInstanceRecord* pInstRec = osLayerMFSGetInstRec();
    g_oslayer_last_result = OSLAYER_OK;

    CT_BEGIN(pInstRec->ct_state);
    if (msec)
    {
        timerSet(pInstRec->timer_id, (HZ * msec) / 1000);
        CT_IDLE_UNTIL(timerIsExpired(pInstRec->timer_id));
    }
    else
    {
        CT_CORETURN(SCHED_RUN);
    }
    CT_END();
    return (CT_DONE);
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
    uint64_t time;
    struct system  *sys = GET_SYS();
    time = (sys->lboltH & 0xFFFFFFFFul);
    time = (time << 32) | sys->lbolt;
    return (time);
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
#if defined (PICO)
    return (uint64_t)1000000;
#else
    return (uint64_t)1000;
#endif
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
    /* must not be used for MFS */
    DCT_ASSERT(0);
    return 0;
}


/******************************************************************************
 *  osFree()
 ******************************************************************************
 *  @brief  Free a continuous block of memory.
 *
 *  @param  p       Pointer to previously allocated memory block
 *
 ******************************************************************************/
int32_t osFree(void *p)
{
    /* must not be used for MFS */
    DCT_ASSERT(0);
    return 0;
}

#endif /* OSLAYER_MISC */


#endif /* MFS */
