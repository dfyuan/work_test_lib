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
 * @mim_ctrl.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/

#include <ebase/trace.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>
#include <cameric_drv/cameric_mi_drv_api.h>

#include "mim_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER( MIM_CTRL_INFO , "MIM-CTRL: ", INFO,      0 );
CREATE_TRACER( MIM_CTRL_WARN , "MIM-CTRL: ", WARNING,   1 );
CREATE_TRACER( MIM_CTRL_ERROR, "MIM-CTRL: ", ERROR,     1 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/******************************************************************************
 * MimCtrlThreadHandler()
 *****************************************************************************/
static int32_t MimCtrlThreadHandler
(
    void *p_arg
)
{
    TRACE( MIM_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( p_arg == NULL )
    {
        TRACE( MIM_CTRL_ERROR, "%s (arg pointer is NULL)\n", __FUNCTION__ );
    }
    else
    {
        MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        do 
        {
            /* set default result */
            RESULT result = RET_WRONG_STATE;
            bool_t completeCmd = BOOL_TRUE;

            /* wait for next command */
            MimCtrlCmdId_t Command;
            OSLAYER_STATUS osStatus = osQueueRead( &pMimCtrlCtx->CommandQueue, &Command );
            if (OSLAYER_OK != osStatus)
            {
                TRACE( MIM_CTRL_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus );
                continue; /* for now we simply try again */
            }

            TRACE( MIM_CTRL_INFO, "%s (received command %d)\n", __FUNCTION__, Command );
            switch ( MimCtrlGetState( pMimCtrlCtx ) )
            {
                case eMimCtrlStateInitialize:
                    {
                        TRACE( MIM_CTRL_INFO, "%s (enter init state)\n", __FUNCTION__ );

                        switch ( Command )
                        {
                            case MIM_CTRL_CMD_SHUTDOWN:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateInvalid );
                                    completeCmd = BOOL_FALSE;
                                    bExit = BOOL_TRUE;
                                    break;
                                }

                            case MIM_CTRL_CMD_START:
                                {
                                     MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateRunning );
                                     result = RET_SUCCESS;

                                     break;
                                }

                            default:                                
                                {
                                    TRACE( MIM_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command);
                                    break;
                                }
                        }

                        TRACE( MIM_CTRL_INFO, "%s (exit init state)\n", __FUNCTION__ );

                        break;
                    }

                case eMimCtrlStateRunning:
                    {
                        TRACE( MIM_CTRL_INFO, "%s (enter run state)\n", __FUNCTION__ );

                        switch ( Command )
                        {
                            case MIM_CTRL_CMD_DMA_TRANSFER:
                                {
                                    result = CamerIcDriverLoadPicture( pMimCtrlCtx->hCamerIc, 
                                            pMimCtrlCtx->pDmaPicBuffer, &pMimCtrlCtx->DmaCompletionCb, BOOL_FALSE );
                                    if ( result != RET_PENDING )
                                    {
                                        completeCmd = BOOL_TRUE;
                                    }
                                    else
                                    {
                                        MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateWaitForDma );
                                        completeCmd = BOOL_FALSE;
                                    }

                                    break;
                                }

                            case MIM_CTRL_CMD_STOP:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateStopped );
                                    result = RET_SUCCESS;

                                    break;
                                }
 
                            default:                                
                                {
                                    TRACE( MIM_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command);
                                    break;
                                }
                        }

                        TRACE( MIM_CTRL_INFO, "%s (exit run state)\n", __FUNCTION__ );

                        break;
                    }

                case eMimCtrlStateWaitForDma:
                    {
                        TRACE( MIM_CTRL_INFO, "%s (enter waiting_for_dma state)\n", __FUNCTION__ );

                        switch ( Command )
                        {
                            case MIM_CTRL_CMD_DMA_TRANSFER:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateRunning );
                                    completeCmd = BOOL_TRUE;
                                    result = pMimCtrlCtx->dmaResult;

                                    break;
                                }

                            case MIM_CTRL_CMD_STOP:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateStopped );
                                    result = RET_SUCCESS;

                                    break;
                                }
 
                            default:                                
                                {
                                    TRACE( MIM_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command);
                                    break;
                                }
                        }

                        TRACE( MIM_CTRL_INFO, "%s (exit waiting_for_dma state)\n", __FUNCTION__ );

                        break;
                     }

                case eMimCtrlStateStopped:
                    {
                        TRACE( MIM_CTRL_INFO, "%s (enter stop state)\n", __FUNCTION__ );

                        switch ( Command )
                        {
                            case MIM_CTRL_CMD_DMA_TRANSFER:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateStopped );
                                    completeCmd = BOOL_TRUE;
                                    result = pMimCtrlCtx->dmaResult;

                                    break;
                                }

                            case MIM_CTRL_CMD_SHUTDOWN:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateInvalid );
                                    completeCmd = BOOL_FALSE;
                                    bExit       = BOOL_TRUE;
                                    break;
                                }

                            case MIM_CTRL_CMD_START:
                                {
                                    MimCtrlSetState( pMimCtrlCtx, eMimCtrlStateRunning );
                                    result = RET_SUCCESS;

                                    break;
                                }

                             default:                                
                                {
                                    TRACE( MIM_CTRL_ERROR, "%s (invalid command %d)\n", __FUNCTION__, (int32_t)Command);
                                    break;
                                }
                        }

                        TRACE( MIM_CTRL_INFO, "%s (exit stop state)\n", __FUNCTION__ );

                        break;
                    }

                default:
                    {
                        TRACE( MIM_CTRL_ERROR, "%s (illegal state %d)\n", __FUNCTION__, (int32_t)MimCtrlGetState( pMimCtrlCtx ) );
                        break;
                    }
            }

            if ( completeCmd == BOOL_TRUE )
            {
                /* complete command */
                MimCtrlCompleteCommand( pMimCtrlCtx, Command, result );
            }

        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE( MIM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( 0 );
}



/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * MimCtrlCreate()
 *****************************************************************************/
RESULT MimCtrlCreate
(
    MimCtrlContext_t  *pMimCtrlCtx
)
{
    RESULT result;
    uint32_t i;

    TRACE( MIM_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMimCtrlCtx != NULL );

    /* create command queue */
    if ( OSLAYER_OK != osQueueInit( &pMimCtrlCtx->CommandQueue, pMimCtrlCtx->MaxCommands, sizeof(MimCtrlCmdId_t) ) )
    {
        TRACE( MIM_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pMimCtrlCtx->MaxCommands );
        return ( RET_FAILURE );
    }

    /* create handler thread */
    if ( OSLAYER_OK != osThreadCreate( &pMimCtrlCtx->Thread, MimCtrlThreadHandler, pMimCtrlCtx ) )
    {
        CamerIcCompletionCb_t *DmaTransferCmd;

        TRACE( MIM_CTRL_ERROR, "%s (thread not created)\n", __FUNCTION__);
        osQueueDestroy( &pMimCtrlCtx->CommandQueue );

        return ( RET_FAILURE );
    }

    TRACE( MIM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * MimCtrlDestroy()
 *****************************************************************************/
RESULT MimCtrlDestroy
(
    MimCtrlContext_t *pMimCtrlCtx
)
{
    OSLAYER_STATUS osStatus;
    RESULT result = RET_SUCCESS;

    TRACE( MIM_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMimCtrlCtx != NULL );

    /* send handler thread a shutdown command */
    result = MimCtrlSendCommand( pMimCtrlCtx, MIM_CTRL_CMD_SHUTDOWN );
    if ( result != RET_SUCCESS )
    {
        TRACE( MIM_CTRL_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    /* wait for handler thread to have stopped due to the shutdown command given above */
    if ( OSLAYER_OK != osThreadWait( &pMimCtrlCtx->Thread ) )
    {
        TRACE( MIM_CTRL_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__);
    }

    /* destroy handler thread */
    if ( OSLAYER_OK != osThreadClose( &pMimCtrlCtx->Thread ) )
    {
        TRACE( MIM_CTRL_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__);
    }

    /* cancel any commands waiting in command queue */
    do
    {
        /* get next command from queue */
        MimCtrlCmdId_t Command;
        osStatus = osQueueTryRead( &pMimCtrlCtx->CommandQueue, &Command );

        switch (osStatus)
        {
            case OSLAYER_OK:        /* got a command, so cancel it */
                MimCtrlCompleteCommand( pMimCtrlCtx, Command, RET_CANCELED );
                break;
            case OSLAYER_TIMEOUT:   /* queue is empty */
                break;
            default:                /* something is broken... */
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while ( osStatus == OSLAYER_OK );

    /* destroy command queue */
    if ( OSLAYER_OK != osQueueDestroy( &pMimCtrlCtx->CommandQueue ) )
    {
        TRACE( MIM_CTRL_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    TRACE( MIM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

     return ( result );
}



/******************************************************************************
 * MimCtrlSendCommand()
 *****************************************************************************/
RESULT MimCtrlSendCommand
(
    MimCtrlContext_t    *pMimCtrlCtx,
    MimCtrlCmdId_t      Command
)
{
    TRACE( MIM_CTRL_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /* are we shutting down? */
    if ( MimCtrlGetState( pMimCtrlCtx ) == eMimCtrlStateInvalid )
    {
        return ( RET_CANCELED );
    }

    /* send command */
    OSLAYER_STATUS osStatus = osQueueWrite( &pMimCtrlCtx->CommandQueue, &Command );
    if ( osStatus != OSLAYER_OK )
    {
        TRACE( MIM_CTRL_ERROR, "%s (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, MimCtrlGetState( pMimCtrlCtx ), osStatus);
    }

    TRACE( MIM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE );
}



/******************************************************************************
 * MimCtrlCompleteCommand()
 *****************************************************************************/
void MimCtrlCompleteCommand
(
    MimCtrlContext_t    *pMimCtrlCtx,
    MimCtrlCmdId_t      Command,
    RESULT              result
)
{
    DCT_ASSERT( pMimCtrlCtx != NULL );

    pMimCtrlCtx->mimCbCompletion( Command, result, pMimCtrlCtx->pUserContext );
}

