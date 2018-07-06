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
 * @file bufsync_ctrl.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/picture_buffer.h>
#include <common/return_codes.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include "bufsync_ctrl_cb.h"
#include "bufsync_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( BUFSYNC_CTRL_INFO , "BUFSYNC-CTRL: ", INFO   , 0 );
CREATE_TRACER( BUFSYNC_CTRL_WARN , "BUFSYNC-CTRL: ", WARNING, 1 );
CREATE_TRACER( BUFSYNC_CTRL_ERROR, "BUFSYNC-CTRL: ", ERROR  , 1 );

CREATE_TRACER( BUFSYNC_CTRL_DEBUG, "", INFO, 0 );



/******************************************************************************
 * BufSyncQueueThreadHandler()
 *****************************************************************************/
static int32_t BufSyncQueueThreadHandler
(
    void *p_arg
)
{
    TRACE( BUFSYNC_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg )
    {
        BufSysncQueueContext_t *pQueueCtx = (BufSysncQueueContext_t *)p_arg;
        bool_t bExit = BOOL_FALSE;

        do
        {
            /* wait for next buffer */
            MediaBuffer_t *pBuffer = NULL;
            OSLAYER_STATUS osStatus = osQueueTimedRead( pQueueCtx->pBufQueue, &pBuffer, 1000 );
            if ( OSLAYER_OK != osStatus )
            {
                if ( OSLAYER_TIMEOUT == osStatus )
                {
                    bExit = pQueueCtx->bExit;
                }
                else
                {
                    TRACE( BUFSYNC_CTRL_ERROR, "%s (receiving buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus );
                    continue; /* for now we simply try again */
                }
            }

            TRACE( BUFSYNC_CTRL_INFO, "%s (run) %d\n", __FUNCTION__, pQueueCtx->id);

            if ( NULL != pBuffer )
            {
                BufSyncCmd_t Command;

                TRACE( BUFSYNC_CTRL_DEBUG, "%s (received buffer = %lx from queue = %d)\n", __FUNCTION__, (ulong_t)pBuffer->pBaseAddress, pQueueCtx->id );

                osMutexLock ( &pQueueCtx->BufferLock );
                if ( pQueueCtx->pBuffer )
                {
                    MediaBufUnlockBuffer( pQueueCtx->pBuffer );
                }
                pQueueCtx->pBuffer = pBuffer;
                osMutexUnlock ( &pQueueCtx->BufferLock );

                MEMSET( &Command, 0, sizeof( BufSyncCmd_t ) );
                Command.CmdId   = BUFSYNC_CTRL_CMD_PROCESS_BUFFER;
                Command.pCmdCtx = (void *)pQueueCtx->id;
                BufSyncCtrlSendCommand( pQueueCtx->pOwner, &Command );

                bExit = pBuffer->last;
            }
        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }


    TRACE( BUFSYNC_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}



/******************************************************************************
 * CreateQueueThreads()
 *****************************************************************************/
static RESULT CreateQueueThreads
(
    BufSyncCtrlContext_t *pBufSyncCtrlCtx
)
{
    int32_t i = 0;

    for ( i = 0; i<BUFSYNC_MAX_QUEUES; i++ )
    {
        pBufSyncCtrlCtx->QueueCtx[i].id     = i;
        pBufSyncCtrlCtx->QueueCtx[i].bExit  = BOOL_FALSE;
        pBufSyncCtrlCtx->QueueCtx[i].pOwner = pBufSyncCtrlCtx;

        /* initialize lock */
        if ( OSLAYER_OK != osMutexInit( &pBufSyncCtrlCtx->QueueCtx[i].BufferLock ) )
        {
            TRACE( BUFSYNC_CTRL_ERROR, "%s (lock not created)\n", __FUNCTION__);
            return ( RET_FAILURE );
        }

        /* create handler thread */
        if ( OSLAYER_OK != osThreadCreate( &pBufSyncCtrlCtx->QueueCtx[i].Thread,
                                                BufSyncQueueThreadHandler, &pBufSyncCtrlCtx->QueueCtx[i] ) )
        {
            TRACE( BUFSYNC_CTRL_ERROR, "%s (queue thread not created)\n", __FUNCTION__);
            return ( RET_FAILURE );
        }
    }

    return ( RET_SUCCESS );
}



/******************************************************************************
 * DestroyQueueThreads()
 *****************************************************************************/
static RESULT DestroyQueueThreads
(
    BufSyncCtrlContext_t *pBufSyncCtrlCtx
)
{
    RESULT result = RET_SUCCESS;

    int32_t i = 0;

    OSLAYER_STATUS osStatus;

    for ( i = 0; i<BUFSYNC_MAX_QUEUES; i++ )
    {
        /* send handler thread a shutdown command */
        pBufSyncCtrlCtx->QueueCtx[i].bExit = BOOL_TRUE;

        /* wait for handler thread to have stopped due to the shutdown command given above */
        if ( OSLAYER_OK != osThreadWait( &pBufSyncCtrlCtx->QueueCtx[i].Thread ) )
        {
            TRACE( BUFSYNC_CTRL_ERROR, "%s (waiting for queue handler thread failed)\n", __FUNCTION__);
        }

        /* destroy handler thread */
        if ( OSLAYER_OK != osThreadClose( &pBufSyncCtrlCtx->QueueCtx[i].Thread ) )
        {
            TRACE( BUFSYNC_CTRL_ERROR, "%s (closing queue handler thread failed)\n", __FUNCTION__);
        }

        /* cancel any buffers waiting in queue */
        do
        {
            /* get next buffer from queue */
            MediaBuffer_t *pBuffer = NULL;

            osStatus = osQueueTryRead( pBufSyncCtrlCtx->QueueCtx[i].pBufQueue, &pBuffer );

            switch (osStatus)
            {
                case OSLAYER_OK:        /* got a command, so cancel it */
                    MediaBufUnlockBuffer( pBuffer );
                    break;

                case OSLAYER_TIMEOUT:   /* queue is empty */
                    break;

                default:                /* something is broken... */
                    UPDATE_RESULT( result, RET_FAILURE);
                    break;
            }
        } while (osStatus == OSLAYER_OK);
    }

    return ( result );
}



/******************************************************************************
 * BufSyncCtrlThreadHandler()
 *****************************************************************************/
static int32_t BufSyncCtrlThreadHandler
(
    void *p_arg
)
{
    TRACE( BUFSYNC_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg )
    {
        BufSyncCtrlContext_t *pBufSyncCtrlCtx = (BufSyncCtrlContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        do
        {
            /* set default result */
            RESULT result = RET_WRONG_STATE;
            bool_t completeCmd = BOOL_TRUE;

            /* wait for next command */
            BufSyncCmd_t Command;
            OSLAYER_STATUS osStatus = osQueueRead( &pBufSyncCtrlCtx->CommandQueue, &Command );
            if (OSLAYER_OK != osStatus)
            {
                TRACE( BUFSYNC_CTRL_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus );
                continue; /* for now we simply try again */
            }

            TRACE( BUFSYNC_CTRL_INFO, "%s (received command %d)\n", __FUNCTION__, Command.CmdId );
            switch ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) )
            {
                case eBufSyncCtrlStateInitialize:
                    {
                        TRACE( BUFSYNC_CTRL_INFO, "%s (enter init state)\n", __FUNCTION__);

                        switch ( Command.CmdId )
                        {
                            case BUFSYNC_CTRL_CMD_SHUTDOWN:
                                {
                                    BufSyncCtrlSetState( pBufSyncCtrlCtx, eBufSyncCtrlStateInvalid );
                                    completeCmd = BOOL_FALSE;
                                    bExit = BOOL_TRUE;
                                    break;
                                }

                            case BUFSYNC_CTRL_CMD_START:
                                {
                                    BufSyncCtrlSetState( pBufSyncCtrlCtx, eBufSyncCtrlStateRunning );
                                    result = RET_SUCCESS;
                                    break;
                                }

                            case BUFSYNC_CTRL_CMD_PROCESS_BUFFER:
                                {
                                    ulong_t id = (ulong_t)Command.pCmdCtx;
                                    if ( id<BUFSYNC_MAX_QUEUES )
                                    {
                                        if ( pBufSyncCtrlCtx->QueueCtx[id].pBuffer )
                                        {
                                            MediaBufUnlockBuffer( pBufSyncCtrlCtx->QueueCtx[id].pBuffer );
                                        }
                                    }
                                    completeCmd = BOOL_FALSE;

                                    break;
                                }

                            default:
                                {
                                    TRACE( BUFSYNC_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command.CmdId );
                                    break;
                                }
                         }

                        TRACE( BUFSYNC_CTRL_INFO, "%s (exit init state)\n", __FUNCTION__);

                        break;
                    }

                case eBufSyncCtrlStateRunning:
                    {
                        TRACE( BUFSYNC_CTRL_INFO, "%s (enter run state)\n", __FUNCTION__);

                        switch ( Command.CmdId )
                        {
                            case BUFSYNC_CTRL_CMD_PROCESS_BUFFER:
                                {
                                    TRACE( BUFSYNC_CTRL_INFO, "%s (BUFSYNC_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                                    osMutexLock( &pBufSyncCtrlCtx->QueueCtx[0].BufferLock );
                                    osMutexLock( &pBufSyncCtrlCtx->QueueCtx[1].BufferLock );

                                    MediaBuffer_t *pBuffer1 = pBufSyncCtrlCtx->QueueCtx[0].pBuffer;
                                    MediaBuffer_t *pBuffer2 = pBufSyncCtrlCtx->QueueCtx[1].pBuffer;

                                    if ( (NULL != pBuffer1) && (NULL != pBuffer2)
                                            && (NULL != pBufSyncCtrlCtx->BufferCb.fpCallback) )
                                    {
                                        pBufSyncCtrlCtx->QueueCtx[0].pBuffer = NULL;
                                        pBufSyncCtrlCtx->QueueCtx[1].pBuffer = NULL;

                                        osMutexUnlock( &pBufSyncCtrlCtx->QueueCtx[0].BufferLock );
                                        osMutexUnlock( &pBufSyncCtrlCtx->QueueCtx[1].BufferLock );

                                        pBuffer1->pNext = pBuffer2;
                                        pBuffer2->pNext = NULL;

                                        osMutexLock( &pBufSyncCtrlCtx->BufferLock );
                                        (pBufSyncCtrlCtx->BufferCb.fpCallback)( 0, pBuffer1, pBufSyncCtrlCtx->BufferCb.pBufferCbCtx );
                                        osMutexUnlock( &pBufSyncCtrlCtx->BufferLock );

                                        MediaBufUnlockBuffer( pBuffer2 );
                                        MediaBufUnlockBuffer( pBuffer1 );
                                    }
                                    else
                                    {
                                        osMutexUnlock( &pBufSyncCtrlCtx->QueueCtx[0].BufferLock );
                                        osMutexUnlock( &pBufSyncCtrlCtx->QueueCtx[1].BufferLock );
                                    }

                                    completeCmd = BOOL_FALSE;
                                    break;
                                }

                            case BUFSYNC_CTRL_CMD_STOP:
                                {
                                    BufSyncCtrlSetState( pBufSyncCtrlCtx, eBufSyncCtrlStateStopped );
                                    result = RET_SUCCESS;
                                    completeCmd = BOOL_TRUE;
                                    break;
                                }

                             default:
                                {
                                    TRACE( BUFSYNC_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command.CmdId );
                                    break;
                                }
                        }

                        TRACE( BUFSYNC_CTRL_INFO, "%s (exit run state)\n", __FUNCTION__);

                        break;
                    }

                case eBufSyncCtrlStateStopped:
                    {
                        TRACE( BUFSYNC_CTRL_INFO, "%s (enter stop state)\n", __FUNCTION__);

                        switch ( Command.CmdId )
                        {
                            case BUFSYNC_CTRL_CMD_START:
                                {
                                    BufSyncCtrlSetState( pBufSyncCtrlCtx, eBufSyncCtrlStateRunning );
                                    result = RET_SUCCESS;
                                    break;
                                }

                            case BUFSYNC_CTRL_CMD_SHUTDOWN:
                                {
                                    BufSyncCtrlSetState( pBufSyncCtrlCtx, eBufSyncCtrlStateInvalid );
                                    completeCmd = BOOL_FALSE;
                                    bExit       = BOOL_TRUE;
                                    break;
                                }

                            case BUFSYNC_CTRL_CMD_PROCESS_BUFFER:
                                {
                                    ulong_t id = (ulong_t)Command.pCmdCtx;
                                    if ( id<BUFSYNC_MAX_QUEUES )
                                    {
                                        osMutexLock( &pBufSyncCtrlCtx->QueueCtx[id].BufferLock );
                                        MediaBuffer_t *pBuffer = pBufSyncCtrlCtx->QueueCtx[id].pBuffer;
                                        if ( NULL != pBuffer )
                                        {
                                            pBufSyncCtrlCtx->QueueCtx[id].pBuffer = NULL;
                                            MediaBufUnlockBuffer( pBuffer );
                                        }
                                        osMutexUnlock( &pBufSyncCtrlCtx->QueueCtx[id].BufferLock );
                                    }

                                    completeCmd = BOOL_FALSE;
                                    break;
                                }

                            default:
                                {
                                    TRACE( BUFSYNC_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command.CmdId );
                                    break;
                                }
                        }

                        TRACE( BUFSYNC_CTRL_INFO, "%s (exit stop state)\n", __FUNCTION__);

                        break;
                    }

                default:
                    {
                        TRACE( BUFSYNC_CTRL_ERROR, "%s (illegal state %d)\n", __FUNCTION__, (int32_t)BufSyncCtrlGetState( pBufSyncCtrlCtx ) );
                        break;
                    }
            }

            if ( completeCmd == BOOL_TRUE )
            {
                /* complete command */
                BufSyncCtrlCompleteCommand( pBufSyncCtrlCtx, Command.CmdId, result );
            }

        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE( BUFSYNC_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}

/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * BufSyncCtrlCreate()
 *****************************************************************************/
RESULT BufSyncCtrlCreate
(
    BufSyncCtrlContext_t *pBufSyncCtrlCtx
)
{
    RESULT result = RET_SUCCESS;

    int32_t i;

    TRACE( BUFSYNC_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pBufSyncCtrlCtx != NULL );

    /* create command queue */
    if ( OSLAYER_OK != osQueueInit( &pBufSyncCtrlCtx->CommandQueue, pBufSyncCtrlCtx->MaxCommands, sizeof(BufSyncCmd_t) ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pBufSyncCtrlCtx->MaxCommands );
        return ( RET_FAILURE );
    }

    /* create buffer lock */
    if ( OSLAYER_OK != osMutexInit( &pBufSyncCtrlCtx->BufferLock ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (creating buffer-lock failed)\n", __FUNCTION__ );
        (void)osQueueDestroy( &pBufSyncCtrlCtx->CommandQueue );
        return ( RET_FAILURE );
    }

    /* create handler thread */
    if ( OSLAYER_OK != osThreadCreate( &pBufSyncCtrlCtx->Thread, BufSyncCtrlThreadHandler, pBufSyncCtrlCtx ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (thread not created)\n", __FUNCTION__);
        (void)osMutexDestroy( &pBufSyncCtrlCtx->BufferLock );
        (void)osQueueDestroy( &pBufSyncCtrlCtx->CommandQueue );
        return ( RET_FAILURE );
    }

    if ( RET_SUCCESS != CreateQueueThreads( pBufSyncCtrlCtx ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (queue-threads thread not created)\n", __FUNCTION__);
        (void)osMutexDestroy( &pBufSyncCtrlCtx->BufferLock );
        (void)osQueueDestroy( &pBufSyncCtrlCtx->CommandQueue );
        return ( RET_FAILURE );
    }

    TRACE( BUFSYNC_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * MomCtrlDestroy()
 *****************************************************************************/
RESULT BufSyncCtrlDestroy
(
    BufSyncCtrlContext_t *pBufSyncCtrlCtx
)
{
    OSLAYER_STATUS osStatus;
    RESULT result;
    uint32_t i;

    BufSyncCmd_t Command;

    TRACE( BUFSYNC_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pBufSyncCtrlCtx != NULL );

    result = DestroyQueueThreads( pBufSyncCtrlCtx );
    if ( RET_SUCCESS != result )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (closing queue-threads failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    /* send handler thread a shutdown command */
    MEMSET( &Command, 0, sizeof( BufSyncCmd_t ) );
    Command.CmdId = BUFSYNC_CTRL_CMD_SHUTDOWN;
    result = BufSyncCtrlSendCommand( pBufSyncCtrlCtx, &Command );
    if ( result != RET_SUCCESS )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    /* wait for handler thread to have stopped due to the shutdown command given above */
    if ( OSLAYER_OK != osThreadWait( &pBufSyncCtrlCtx->Thread ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__);
    }

    /* destroy handler thread */
    if ( OSLAYER_OK != osThreadClose( &pBufSyncCtrlCtx->Thread ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__);
    }

    /* cancel any commands waiting in command queue */
    do
    {
        /* get next command from queue */
        BufSyncCtrlCmdId_t Command;
        osStatus = osQueueTryRead( &pBufSyncCtrlCtx->CommandQueue, &Command );

        switch (osStatus)
        {
            case OSLAYER_OK:        /* got a command, so cancel it */
                BufSyncCtrlCompleteCommand( pBufSyncCtrlCtx, Command, RET_CANCELED );
                break;
            case OSLAYER_TIMEOUT:   /* queue is empty */
                break;
            default:                /* something is broken... */
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while (osStatus == OSLAYER_OK);

    /* destroy command queue */
    if ( OSLAYER_OK != osQueueDestroy( &pBufSyncCtrlCtx->CommandQueue ) )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__ );
    }

    (void)osMutexDestroy( &pBufSyncCtrlCtx->BufferLock );

    TRACE( BUFSYNC_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * BufSyncCtrlSendCommand()
 *****************************************************************************/
RESULT BufSyncCtrlSendCommand
(
    BufSyncCtrlContext_t    *pBufSyncCtrlCtx,
    BufSyncCmd_t            *pCommand
)
{
    TRACE( BUFSYNC_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pBufSyncCtrlCtx == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /* are we shutting down? */
    if ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) == eBufSyncCtrlStateInvalid )
    {
        return ( RET_CANCELED );
    }

    /* send command */
    OSLAYER_STATUS osStatus = osQueueWrite( &pBufSyncCtrlCtx->CommandQueue, pCommand );
    if ( osStatus != OSLAYER_OK )
    {
        TRACE( BUFSYNC_CTRL_ERROR, "%s (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, BufSyncCtrlGetState( pBufSyncCtrlCtx ), osStatus);
    }

    TRACE( BUFSYNC_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE );
}



/******************************************************************************
 * BufSyncCtrlCompleteCommand()
 *****************************************************************************/
void BufSyncCtrlCompleteCommand
(
    BufSyncCtrlContext_t    *pBufSyncCtrlCtx,
    BufSyncCtrlCmdId_t      Command,
    RESULT                  result
)
{
    DCT_ASSERT( pBufSyncCtrlCtx != NULL );
    DCT_ASSERT( pBufSyncCtrlCtx->bufsyncCbCompletion != NULL );

    pBufSyncCtrlCtx->bufsyncCbCompletion( Command, result, pBufSyncCtrlCtx->pUserContext );
}

