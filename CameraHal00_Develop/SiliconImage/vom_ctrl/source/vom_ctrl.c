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
 * @vom_ctrl.c
 *
 * @brief
 *   Implementation of vom ctrl.
 *
 *****************************************************************************/
/**
 * @page vom_ctrl_page VOM Ctrl
 * The Video Output Module displays image buffers handed in via QuadMVDU_FX on
 * a connected HDMI display device.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref vom_ctrl_api
 * - @ref vom_ctrl_common
 * - @ref vom_ctrl
 * - @ref vom_ctrl_mvdu
 *
 */

#include <ebase/trace.h>

#include <common/return_codes.h>
#include <common/picture_buffer.h>

#include <oslayer/oslayer.h>

#include <bufferpool/media_buffer_queue_ex.h>

#include "vom_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(VOM_CTRL_INFO , "VOM-CTRL: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_ERROR, "VOM-CTRL: ", ERROR, 1);


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
 * vomCtrlGetState()
 *****************************************************************************/
static inline vomCtrlState_t vomCtrlGetState
(
    vomCtrlContext_t    *pVomContext
);

/******************************************************************************
 * vomCtrlSetState()
 *****************************************************************************/
static inline void vomCtrlSetState
(
    vomCtrlContext_t    *pVomContext,
    vomCtrlState_t      newState
);

/******************************************************************************
 * vomCtrlCompleteCommand()
 *****************************************************************************/
static void vomCtrlCompleteCommand
(
    vomCtrlContext_t    *pVomContext,
    vomCtrlCmdId_t      Command,
    RESULT              result
);

/******************************************************************************
 * vomCtrlBufferReleaseCb()
 *****************************************************************************/
static void vomCtrlBufferReleaseCb
(
    void            *pUserContext,  //!< Opaque user data pointer that was passed in on creation.
    MediaBuffer_t   *pBuffer,       //!< Pointer to buffer that is to be released.
    RESULT          DelayedResult   //!< Result of delayed buffer processing.
);

/******************************************************************************
 * vomCtrlThreadHandler()
 *****************************************************************************/
static int32_t vomCtrlThreadHandler
(
    void *p_arg
);


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * vomCtrlCreate()
 *****************************************************************************/
RESULT vomCtrlCreate
(
    vomCtrlContext_t    *pVomContext
)
{
    TRACE(VOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pVomContext != NULL );

    // connect to MVDU
    RESULT result = vomCtrlMvduInit(&pVomContext->MvduHandle, pVomContext->pVideoFormat, pVomContext->Enable3D, pVomContext->VideoFormat3D, vomCtrlBufferReleaseCb, pVomContext, pVomContext->HalHandle);
    if (RET_SUCCESS != result)
    {
        TRACE(VOM_CTRL_ERROR, "%s (initializing MVDU failed)\n", __FUNCTION__);
        return result;
    }

    // add HAL reference
    result = HalAddRef( pVomContext->HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_ERROR, "%s (adding HAL reference failed)\n", __FUNCTION__ );
        HalDelRef( pVomContext->HalHandle );
        return result;
    }

    // create command queue
    if ( OSLAYER_OK != osQueueInit( &pVomContext->CommandQueue, pVomContext->MaxCommands, sizeof(vomCtrlCmdId_t) ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pVomContext->MaxCommands);
        vomCtrlMvduDestroy(pVomContext->MvduHandle);
        HalDelRef( pVomContext->HalHandle );
        return ( RET_FAILURE );
    }

    // create full buffer queue
    if ( OSLAYER_OK != osQueueInit( &pVomContext->FullBufQueue, pVomContext->MaxBuffers, sizeof(MediaBuffer_t *) ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (creating buffer queue (depth: %d) failed)\n", __FUNCTION__, pVomContext->MaxBuffers);
        osQueueDestroy( &pVomContext->CommandQueue );
        vomCtrlMvduDestroy(pVomContext->MvduHandle);
        HalDelRef( pVomContext->HalHandle );
        return RET_FAILURE;
    }

    // create handler thread
    if ( OSLAYER_OK != osThreadCreate( &pVomContext->Thread, vomCtrlThreadHandler, pVomContext ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (creating handler thread failed)\n", __FUNCTION__);
        osQueueDestroy( &pVomContext->FullBufQueue );
        osQueueDestroy( &pVomContext->CommandQueue );
        vomCtrlMvduDestroy(pVomContext->MvduHandle);
        HalDelRef( pVomContext->HalHandle );
        return ( RET_FAILURE );
    }

    TRACE(VOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * vomCtrlDestroy()
 *****************************************************************************/
RESULT vomCtrlDestroy
(
    vomCtrlContext_t *pVomContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE(VOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pVomContext != NULL );

    // send handler thread a shutdown command
    lres = vomCtrlSendCommand( pVomContext, VOM_CTRL_CMD_SHUTDOWN );
    if (lres != RET_SUCCESS)
    {
        TRACE(VOM_CTRL_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // wait for handler thread to have stopped due to the shutdown command given above
    if ( OSLAYER_OK != osThreadWait( &pVomContext->Thread ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy handler thread
    if ( OSLAYER_OK != osThreadClose( &pVomContext->Thread ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // cancel any further commands waiting in command queue
    do
    {
        // get next command from queue
        vomCtrlCmdId_t Command;
        osStatus = osQueueTryRead( &pVomContext->CommandQueue, &Command );

        switch (osStatus)
        {
            case OSLAYER_OK:        // got a command, so cancel it
                vomCtrlCompleteCommand( pVomContext, Command, RET_CANCELED );
                break;
            case OSLAYER_TIMEOUT:   // queue is empty
                break;
            default:                // something is broken...
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while (osStatus == OSLAYER_OK);

    // destroy full buffer queue
    if ( OSLAYER_OK != osQueueDestroy( &pVomContext->FullBufQueue ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (destroying full buffer queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy command queue
    if ( OSLAYER_OK != osQueueDestroy( &pVomContext->CommandQueue ) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // disconnect from MVDU
    if ( RET_SUCCESS != vomCtrlMvduDestroy(pVomContext->MvduHandle) )
    {
        TRACE(VOM_CTRL_ERROR, "%s (shutting down MVDU failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // remove HAL reference
    lres = HalDelRef( pVomContext->HalHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_ERROR, "%s (removing HAL reference failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    TRACE(VOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * vomCtrlSendCommand()
 *****************************************************************************/
RESULT vomCtrlSendCommand
(
    vomCtrlContext_t    *pVomContext,
    vomCtrlCmdId_t      Command
)
{
    TRACE(VOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if( pVomContext == NULL )
    {
        return RET_NULL_POINTER;
    }

    // are we shutting down?
    if ( vomCtrlGetState( pVomContext ) == eVomCtrlStateInvalid )
    {
        return RET_CANCELED;
    }

    // send command
    OSLAYER_STATUS osStatus = osQueueWrite( &pVomContext->CommandQueue, &Command);
    if (osStatus != OSLAYER_OK)
    {
        TRACE(VOM_CTRL_ERROR, "%s (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, vomCtrlGetState( pVomContext ), osStatus);
    }

    TRACE(VOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE);
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * vomCtrlGetState()
 *****************************************************************************/
static inline vomCtrlState_t vomCtrlGetState
(
    vomCtrlContext_t    *pVomContext
)
{
    DCT_ASSERT( pVomContext != NULL );
    return ( pVomContext->State );
}


/******************************************************************************
 * vomCtrlSetState()
 *****************************************************************************/
static inline void vomCtrlSetState
(
    vomCtrlContext_t    *pVomContext,
    vomCtrlState_t      newState
)
{
    DCT_ASSERT( pVomContext != NULL );
    pVomContext->State = newState;
}


/******************************************************************************
 * vomCtrlCompleteCommand()
 *****************************************************************************/
static void vomCtrlCompleteCommand
(
    vomCtrlContext_t    *pVomContext,
    vomCtrlCmdId_t      Command,
    RESULT              result
)
{
    TRACE(VOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pVomContext != NULL );
    pVomContext->vomCbCompletion( Command, result, pVomContext->pUserContext );

    TRACE(VOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);
}


/******************************************************************************
 * vomCtrlBufferReleaseCb()
 *****************************************************************************/
static void vomCtrlBufferReleaseCb
(
    void            *pUserContext,
    MediaBuffer_t   *pBuffer,
    RESULT          DelayedResult
)
{
    RESULT result;

    DCT_ASSERT(pUserContext != NULL);
    DCT_ASSERT(pBuffer != NULL);

    // get context
    vomCtrlContext_t *pVomContext = (vomCtrlContext_t *)(pUserContext);

    if ( pBuffer->pNext != NULL )
    {
        MediaBufUnlockBuffer( pBuffer->pNext );
    }
    MediaBufUnlockBuffer( pBuffer );

    // complete command
    vomCtrlCompleteCommand( pVomContext, VOM_CTRL_CMD_PROCESS_BUFFER, DelayedResult );
}


/******************************************************************************
 * vomCtrlThreadHandler()
 *****************************************************************************/
static int32_t vomCtrlThreadHandler
(
    void *p_arg
)
{
    TRACE(VOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg == NULL )
    {
        TRACE(VOM_CTRL_ERROR, "%s (arg pointer is NULL)\n", __FUNCTION__);
    }
    else
    {
        vomCtrlContext_t *pVomContext = (vomCtrlContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        // processing loop
        do
        {
            // set default result
            RESULT result = RET_WRONG_STATE;

            // wait for next command
            vomCtrlCmdId_t Command;
            OSLAYER_STATUS osStatus = osQueueRead(&pVomContext->CommandQueue, &Command);
            if (OSLAYER_OK != osStatus)
            {
                TRACE(VOM_CTRL_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                continue; // for now we simply try again
            }

            // process command
            switch ( Command )
            {
                case VOM_CTRL_CMD_START:
                {
                    TRACE(VOM_CTRL_INFO, "%s (begin VOM_CTRL_CMD_START)\n", __FUNCTION__);

                    switch ( vomCtrlGetState( pVomContext ) )
                    {
                        case eVomCtrlStateIdle:
                        {
                            vomCtrlSetState( pVomContext, eVomCtrlStateRunning );
                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(VOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, vomCtrlGetState( pVomContext ));
                        }
                    }

                    TRACE(VOM_CTRL_INFO, "%s (end VOM_CTRL_CMD_START)\n", __FUNCTION__);

                    break;
                }

                case VOM_CTRL_CMD_STOP:
                {
                    TRACE(VOM_CTRL_INFO, "%s (begin VOM_CTRL_CMD_STOP)\n", __FUNCTION__);

                    switch ( vomCtrlGetState( pVomContext ) )
                    {
                        case eVomCtrlStateRunning:
                        {
                            vomCtrlSetState( pVomContext, eVomCtrlStateIdle );
                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(VOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, vomCtrlGetState( pVomContext ));
                        }
                    }

                    TRACE(VOM_CTRL_INFO, "%s (end VOM_CTRL_CMD_STOP)\n", __FUNCTION__);

                    break;
                }

                case VOM_CTRL_CMD_SHUTDOWN:
                {
                    TRACE(VOM_CTRL_INFO, "%s (begin VOM_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    switch ( vomCtrlGetState( pVomContext ) )
                    {
                        case eVomCtrlStateIdle:
                        {
                            vomCtrlSetState( pVomContext, eVomCtrlStateInvalid ); // stop further commands from being send to command queue
                            bExit = BOOL_TRUE;
                            result = RET_PENDING; // avoid completion below
                            break;
                        }
                        default:
                        {
                            TRACE(VOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, vomCtrlGetState( pVomContext ));
                        }
                    }

                    TRACE(VOM_CTRL_INFO, "%s (end VOM_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    break;
                }

                case VOM_CTRL_CMD_PROCESS_BUFFER:
                {
                    TRACE(VOM_CTRL_INFO, "%s (begin VOM_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    MediaBuffer_t *pBuffer = NULL;

                    switch ( vomCtrlGetState( pVomContext ) )
                    {
                        case eVomCtrlStateIdle:
                        {
                            osStatus = osQueueTryRead( &pVomContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(VOM_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                                break;
                            }

                            // just discard the buffer
                            if ( pBuffer->pNext != NULL )
                            {
                                MediaBufUnlockBuffer( pBuffer->pNext );
                            }
                            MediaBufUnlockBuffer( pBuffer );

                            result = RET_SUCCESS;
                            break;
                        }

                        case eVomCtrlStateRunning:
                        {
                            osStatus = osQueueTryRead( &pVomContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(VOM_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                                break;
                            }

                            // send buffer to MVDU_FX
                            result = vomCtrlMvduDisplay( pVomContext->MvduHandle, pBuffer );

                            // immediately release buffer if it's not pending
                            if (result != RET_PENDING)
                            {
                                if ( pBuffer->pNext != NULL )
                                {
                                    MediaBufUnlockBuffer( pBuffer->pNext );
                                }
                                MediaBufUnlockBuffer( pBuffer );
                            }
                            
                            break;
                        }

                        default:
                        {
                            TRACE(VOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, vomCtrlGetState( pVomContext ));
                        }
                    }

                    TRACE(VOM_CTRL_INFO, "%s (end VOM_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    break;
                }

                default:
                {
                    TRACE(VOM_CTRL_ERROR, "%s (illegal command %d)\n", __FUNCTION__, Command);
                    result = RET_NOTSUPP;
                }
            }

            // complete command?
            if (result != RET_PENDING)
            {
                vomCtrlCompleteCommand( pVomContext, Command, result );
            }
        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE(VOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}
