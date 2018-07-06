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
 * @exa_ctrl.c
 *
 * @brief
 *   Implementation of exa ctrl.
 *
 *****************************************************************************/
/**
 * @page exa_ctrl_page EXA Ctrl
 * The External Algorithm Module captures image and calls external algorithm.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref exa_ctrl_api
 * - @ref exa_ctrl_common
 * - @ref exa_ctrl
 *
 */

#include <time.h>

#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/align.h>

#include <oslayer/oslayer.h>

#include <bufferpool/media_buffer_queue_ex.h>

#include "exa_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(EXA_CTRL_INFO , "EXA-CTRL: ", INFO,  0);
CREATE_TRACER(EXA_CTRL_ERROR, "EXA-CTRL: ", ERROR, 1);

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
 * exaCtrlGetState()
 *****************************************************************************/
static inline exaCtrlState_t exaCtrlGetState
(
    exaCtrlContext_t    *pExaContext
);

/******************************************************************************
 * exaCtrlSetState()
 *****************************************************************************/
static inline void exaCtrlSetState
(
    exaCtrlContext_t    *pExaContext,
    exaCtrlState_t      newState
);

/******************************************************************************
 * exaCtrlCompleteCommand()
 *****************************************************************************/
static void exaCtrlCompleteCommand
(
    exaCtrlContext_t        *pExaContext,
    exaCtrlCmdID_t          CmdID,
    RESULT                  result
);

/******************************************************************************
 * exaCtrlThreadHandler()
 *****************************************************************************/
static int32_t exaCtrlThreadHandler
(
    void *p_arg
);

/******************************************************************************
 * exaCtrlMapBuffer()
 *****************************************************************************/
static RESULT exaCtrlAlgorithmCallback
(
    exaCtrlContext_t    *pExaContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * exaCtrlMapBufferYUV422Semi()
 *****************************************************************************/
static RESULT exaCtrlMapBufferYUV422Semi
(
    exaCtrlContext_t    *pExaContext,
    PicBufMetaData_t    *pPicBufMetaData
);

/******************************************************************************
 * exaCtrlUnMapBufferYUV422Semi()
 *****************************************************************************/
static RESULT exaCtrlUnMapBufferYUV422Semi
(
    exaCtrlContext_t    *pExaContext
);

/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * exaCtrlCreate()
 *****************************************************************************/
RESULT exaCtrlCreate
(
    exaCtrlContext_t    *pExaContext
)
{
    RESULT result;

    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pExaContext != NULL );

    // finalize init of context
    pExaContext->exaCbSample    = NULL;
    pExaContext->pSampleContext = NULL;
    pExaContext->SampleSkip = 0;
    pExaContext->SampleIdx = 0;

    // add HAL reference
    result = HalAddRef( pExaContext->HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( EXA_CTRL_ERROR, "%s (adding HAL reference failed)\n", __FUNCTION__ );
        return result;
    }

    // create command queue
    if ( OSLAYER_OK != osQueueInit( &pExaContext->CommandQueue, pExaContext->MaxCommands, sizeof(exaCtrlCmd_t) ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pExaContext->MaxCommands);
        HalDelRef( pExaContext->HalHandle );
        return ( RET_FAILURE );
    }

    // create full buffer queue
    if ( OSLAYER_OK != osQueueInit( &pExaContext->FullBufQueue, pExaContext->MaxBuffers, sizeof(MediaBuffer_t *) ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (creating buffer queue (depth: %d) failed)\n", __FUNCTION__, pExaContext->MaxBuffers);
        osQueueDestroy( &pExaContext->CommandQueue );
        HalDelRef( pExaContext->HalHandle );
        return ( RET_FAILURE );
    }

    // create handler thread
    if ( OSLAYER_OK != osThreadCreate( &pExaContext->Thread, exaCtrlThreadHandler, pExaContext ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (creating handler thread failed)\n", __FUNCTION__);
        osQueueDestroy( &pExaContext->FullBufQueue );
        osQueueDestroy( &pExaContext->CommandQueue );
        HalDelRef( pExaContext->HalHandle );
        return ( RET_FAILURE );
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * exaCtrlDestroy()
 *****************************************************************************/
RESULT exaCtrlDestroy
(
    exaCtrlContext_t *pExaContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pExaContext != NULL );

    // send handler thread a shutdown command
    // ...prepare command
    exaCtrlCmd_t Command;
    MEMSET( &Command, 0, sizeof(Command) );
    Command.CmdID = EXA_CTRL_CMD_SHUTDOWN;
    // ....send command
    lres = exaCtrlSendCommand( pExaContext, &Command );
    if (lres != RET_SUCCESS)
    {
        TRACE(EXA_CTRL_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        UPDATE_RESULT( result, lres);
    }

    // wait for handler thread to have stopped due to the shutdown command given above
    if ( OSLAYER_OK != osThreadWait( &pExaContext->Thread ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy handler thread
    if ( OSLAYER_OK != osThreadClose( &pExaContext->Thread ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // cancel any further commands waiting in command queue
    do
    {
        // get next command from queue
        exaCtrlCmd_t Command;
        osStatus = osQueueTryRead( &pExaContext->CommandQueue, &Command );

        switch (osStatus)
        {
            case OSLAYER_OK:        // got a command, so cancel it
                exaCtrlCompleteCommand( pExaContext, Command.CmdID, RET_CANCELED );
                break;
            case OSLAYER_TIMEOUT:   // queue is empty
                break;
            default:                // exaething is broken...
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while (osStatus == OSLAYER_OK);

    // destroy full buffer queue
    if ( OSLAYER_OK != osQueueDestroy( &pExaContext->FullBufQueue ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (destroying full buffer queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy command queue
    if ( OSLAYER_OK != osQueueDestroy( &pExaContext->CommandQueue ) )
    {
        TRACE(EXA_CTRL_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // remove HAL reference
    lres = HalDelRef( pExaContext->HalHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( EXA_CTRL_ERROR, "%s (removing HAL reference failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * exaCtrlSendCommand()
 *****************************************************************************/
RESULT exaCtrlSendCommand
(
    exaCtrlContext_t    *pExaContext,
    exaCtrlCmd_t        *pCommand
)
{
    TRACE(EXA_CTRL_INFO, "%s (enter), command=%u \n", __FUNCTION__, pCommand->CmdID);

    if( (pExaContext == NULL) || (pCommand == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // are we shutting down?
    if ( exaCtrlGetState( pExaContext ) == eExaCtrlStateInvalid )
    {
        return RET_CANCELED;
    }

    // send command
    OSLAYER_STATUS osStatus = osQueueWrite( &pExaContext->CommandQueue, pCommand);
    if (osStatus != OSLAYER_OK)
    {
        TRACE(EXA_CTRL_ERROR, "%s (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ), osStatus);
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE);
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * exaCtrlGetState()
 *****************************************************************************/
static inline exaCtrlState_t exaCtrlGetState
(
    exaCtrlContext_t    *pExaContext
)
{
    DCT_ASSERT( pExaContext != NULL );
    return ( pExaContext->State );
}


/******************************************************************************
 * exaCtrlSetState()
 *****************************************************************************/
static inline void exaCtrlSetState
(
    exaCtrlContext_t    *pExaContext,
    exaCtrlState_t      newState
)
{
    DCT_ASSERT( pExaContext != NULL );
    pExaContext->State = newState;
}


/******************************************************************************
 * exaCtrlCompleteCommand()
 *****************************************************************************/
static void exaCtrlCompleteCommand
(
    exaCtrlContext_t        *pExaContext,
    exaCtrlCmdID_t          CmdID,
    RESULT                  result
)
{
    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pExaContext != NULL );

    // do callback
    pExaContext->exaCbCompletion( CmdID, result, pExaContext->pUserContext );

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__);
}


/******************************************************************************
 * exaCtrlThreadHandler()
 *****************************************************************************/
static int32_t exaCtrlThreadHandler
(
    void *p_arg
)
{
    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg == NULL )
    {
        TRACE(EXA_CTRL_ERROR, "%s (arg pointer is NULL)\n", __FUNCTION__);
    }
    else
    {
        exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        // processing loop
        do
        {
            // set default result
            RESULT result = RET_WRONG_STATE;

            // wait for next command
            exaCtrlCmd_t Command;
            OSLAYER_STATUS osStatus = osQueueRead(&pExaContext->CommandQueue, &Command);
            if (OSLAYER_OK != osStatus)
            {
                TRACE(EXA_CTRL_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                continue; // for now we simply try again
            }

            // process command
            switch ( Command.CmdID )
            {
                case EXA_CTRL_CMD_START:
                {
                    TRACE(EXA_CTRL_INFO, "%s (begin EXA_CTRL_CMD_START)\n", __FUNCTION__);

                    switch ( exaCtrlGetState( pExaContext ) )
                    {
                        case eExaCtrlStatePaused:
                        case eExaCtrlStateIdle:
                        {
                            pExaContext->exaCbSample    = Command.Params.Start.exaCbSample;
                            pExaContext->pSampleContext = Command.Params.Start.pSampleContext;
                            pExaContext->SampleSkip     = Command.Params.Start.SampleSkip;
                            pExaContext->SampleIdx      = 0;

                            exaCtrlSetState( pExaContext, eExaCtrlStateRunning );

                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(EXA_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));
                        }
                    }

                    TRACE(EXA_CTRL_INFO, "%s (end EXA_CTRL_CMD_START)\n", __FUNCTION__);

                    break;
                }

                case EXA_CTRL_CMD_PAUSE:
                {
                    TRACE(EXA_CTRL_INFO, "%s (begin EXA_CTRL_CMD_PAUSE)\n", __FUNCTION__);

                    switch ( exaCtrlGetState( pExaContext ) )
                    {
                        case eExaCtrlStateRunning:
                        {
                            exaCtrlSetState( pExaContext, eExaCtrlStatePaused );
                        }
                        case eExaCtrlStatePaused:
                        case eExaCtrlStateIdle:
                        {
                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(EXA_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));
                        }
                    }

                    TRACE(EXA_CTRL_INFO, "%s (end EXA_CTRL_CMD_PAUSE)\n", __FUNCTION__);

                    break;
                }

                case EXA_CTRL_CMD_RESUME:
                {
                    TRACE(EXA_CTRL_INFO, "%s (begin EXA_CTRL_CMD_RESUME)\n", __FUNCTION__);

                    switch ( exaCtrlGetState( pExaContext ) )
                    {
                        case eExaCtrlStatePaused:
                        {
                            exaCtrlSetState( pExaContext, eExaCtrlStateRunning );
                        }
                        case eExaCtrlStateIdle:
                        {
                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(EXA_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));
                        }
                    }

                    TRACE(EXA_CTRL_INFO, "%s (end EXA_CTRL_CMD_RESUME)\n", __FUNCTION__);

                    break;
                }

                case EXA_CTRL_CMD_STOP:
                {
                    TRACE(EXA_CTRL_INFO, "%s (begin EXA_CTRL_CMD_STOP)\n", __FUNCTION__);

                    switch ( exaCtrlGetState( pExaContext ) )
                    {
                        case eExaCtrlStatePaused:
                        case eExaCtrlStateRunning:
                        {
                            // reset
                            pExaContext->exaCbSample = NULL;
                            pExaContext->pSampleContext = NULL;
                            pExaContext->SampleSkip = 0;
                            pExaContext->SampleIdx = 0;

                            // back to idle state
                            exaCtrlSetState( pExaContext, eExaCtrlStateIdle );
                        }
                        case eExaCtrlStateIdle:
                        {
                            // we allow this state as well, as state running may be left automatically
                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(EXA_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));
                        }
                    }

                    TRACE(EXA_CTRL_INFO, "%s (end EXA_CTRL_CMD_STOP)\n", __FUNCTION__);

                    break;
                }

                case EXA_CTRL_CMD_SHUTDOWN:
                {
                    TRACE(EXA_CTRL_INFO, "%s (begin EXA_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    switch ( exaCtrlGetState( pExaContext ) )
                    {
                        case eExaCtrlStatePaused:
                        case eExaCtrlStateRunning:
                        {
                            // reset
                            pExaContext->exaCbSample = NULL;
                            pExaContext->pSampleContext = NULL;
                            pExaContext->SampleSkip = 0;
                            pExaContext->SampleIdx = 0;
                        }
                        case eExaCtrlStateIdle:
                        {
                            exaCtrlSetState( pExaContext, eExaCtrlStateInvalid ); // stop further commands from being send to command queue
                            bExit = BOOL_TRUE;
                            result = RET_PENDING; // avoid completion below
                            break;
                        }
                        default:
                        {
                            TRACE(EXA_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));
                        }
                    }

                    TRACE(EXA_CTRL_INFO, "%s (end EXA_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    break;
                }

                case EXA_CTRL_CMD_PROCESS_BUFFER:
                {
                    TRACE(EXA_CTRL_INFO, "%s (begin EXA_CTRL_CMD_PROCESS_BUFFER, state=%d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));

                    switch ( exaCtrlGetState( pExaContext ) )
                    {
                        case eExaCtrlStatePaused:
                        case eExaCtrlStateIdle:
                        {
                            // just discard buffer
                            MediaBuffer_t *pBuffer = NULL;
                            osStatus = osQueueTryRead( &pExaContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(EXA_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                                break;
                            }
                            MediaBufUnlockBuffer( pBuffer );

                            result = RET_SUCCESS;
                            break;
                        }

                        case eExaCtrlStateRunning:
                        {
                            MediaBuffer_t *pBuffer = NULL;
                            osStatus = osQueueTryRead( &pExaContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(EXA_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                               break;
                            }

                            if ( pBuffer )
                            {
                                result = RET_SUCCESS;

                                if ( 0 == (pExaContext->SampleIdx % (pExaContext->SampleSkip + 1)) )
                                {
                                    // get image and call external algorithm
                                    result = exaCtrlAlgorithmCallback( pExaContext, pBuffer );
                                }
                                ++pExaContext->SampleIdx;

                                // release buffer
                                MediaBufUnlockBuffer( pBuffer );
                            }
                            else
                            {
                                result = RET_FAILURE;
                            }

                            break;
                        }

                        default:
                        {
                            TRACE(EXA_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, exaCtrlGetState( pExaContext ));
                        }
                    }

                    TRACE(EXA_CTRL_INFO, "%s (end EXA_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    break;
                }

                default:
                {
                    TRACE(EXA_CTRL_ERROR, "%s (illegal command %d)\n", __FUNCTION__, Command);
                    result = RET_NOTSUPP;
                }
            }

            // complete command?
            if (result != RET_PENDING)
            {
                exaCtrlCompleteCommand( pExaContext, Command.CmdID, result );
            }
        }
        while ( bExit == BOOL_FALSE );  // !bExit
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}

static RESULT exaCtrlAlgorithmCallback
(
    exaCtrlContext_t   *pExaContext,
    MediaBuffer_t      *pBuffer
)
{
    RESULT result = RET_SUCCESS;

    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pExaContext == NULL) || (pBuffer == NULL) )
    {
        return RET_NULL_POINTER;
    }
    if ( (pExaContext->exaCbSample == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    result = PicBufIsConfigValid( pPicBufMetaData );
    if (RET_SUCCESS != result)
    {
        return result;
    }

    // check type, select map & unmap functions
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        case PIC_BUF_TYPE_JPEG:
        case PIC_BUF_TYPE_YCbCr444:
        case PIC_BUF_TYPE_YCbCr420:
        case PIC_BUF_TYPE_RGB888:
            UPDATE_RESULT( result, RET_NOTSUPP );
            break;
        case PIC_BUF_TYPE_YCbCr422:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_PLANAR:
                case PIC_BUF_LAYOUT_COMBINED:
                    UPDATE_RESULT( result, RET_NOTSUPP );
                    break;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                {
                    // set format/layout dependent function pointers
                    pExaContext->MapBuffer   = exaCtrlMapBufferYUV422Semi;
                    pExaContext->UnMapBuffer = exaCtrlUnMapBufferYUV422Semi;
                    break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            };
            break;
        default:
            UPDATE_RESULT( result, RET_NOTSUPP );
    }
    if (result != RET_SUCCESS)
    {
        TRACE(EXA_CTRL_ERROR, "%s unsupported buffer config -> RESULT=%d\n", __FUNCTION__, result);
        return result;
    }

    // map buffer
    result = pExaContext->MapBuffer( pExaContext, pPicBufMetaData );
    if (RET_SUCCESS != result)
    {
        TRACE(EXA_CTRL_ERROR, "%s MapBuffer() failed -> RESULT=%d\n", __FUNCTION__, result);
        return result;
    }

    // external algorithm callback
    result = pExaContext->exaCbSample( &(pExaContext->MappedMetaData), pExaContext->pSampleContext );
    if (RET_SUCCESS != result)
    {
        TRACE(EXA_CTRL_ERROR, "%s exaCbSample() failed -> RESULT=%d\n", __FUNCTION__, result);
        return result;
    }

    // unmap buffer again
    result = pExaContext->UnMapBuffer( pExaContext );
    if (RET_SUCCESS != result)
    {
        TRACE(EXA_CTRL_ERROR, "%s UnMapBuffer() failed -> RESULT=%d\n", __FUNCTION__, result);
        return result;
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}

static RESULT exaCtrlMapBufferYUV422Semi
(
    exaCtrlContext_t    *pExaContext,
    PicBufMetaData_t    *pPicBufMetaData
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pExaContext == NULL) || (pPicBufMetaData == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // copy buffer meta data; clear mapped buffer pointers for easier unmapping on errors below
    pExaContext->MappedMetaData = *pPicBufMetaData;
    pExaContext->MappedMetaData.Data.YCbCr.semiplanar.Y.pBuffer = NULL;
    pExaContext->MappedMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer = NULL;
    // note: implementation which assumes that on-board memory is used for buffers!

    // get sizes & base addresses of planes
    uint32_t YCPlaneSize  = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    ulong_t YBaseAddr    = (ulong_t) (pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer);
    ulong_t CbCrBaseAddr = (ulong_t) (pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer);

    // map luma plane
    lres = HalMapMemory( pExaContext->HalHandle,
                         YBaseAddr,
                         YCPlaneSize,
                         HAL_MAPMEM_READONLY,
                         (void**)&(pExaContext->MappedMetaData.Data.YCbCr.semiplanar.Y.pBuffer)    );
    UPDATE_RESULT( result, lres );

    // map combined chroma plane
    lres = HalMapMemory( pExaContext->HalHandle,
                         CbCrBaseAddr,
                         YCPlaneSize,
                         HAL_MAPMEM_READONLY,
                         (void**)&(pExaContext->MappedMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer) );
    UPDATE_RESULT( result, lres );

    // check for errors
    if (result != RET_SUCCESS)
    {
        TRACE(EXA_CTRL_ERROR, "%s mapping buffer failed (RESULT=%d)\n", __FUNCTION__, result);
        // unmap partially mapped buffer
        exaCtrlUnMapBufferYUV422Semi( pExaContext );
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}

static RESULT exaCtrlUnMapBufferYUV422Semi
(
    exaCtrlContext_t    *pExaContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(EXA_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (pExaContext == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // unmap (partially) mapped buffer
    if (pExaContext->MappedMetaData.Data.YCbCr.semiplanar.Y.pBuffer)
    {
        // unmap luma plane
        lres = HalUnMapMemory( pExaContext->HalHandle, pExaContext->MappedMetaData.Data.YCbCr.semiplanar.Y.pBuffer    );
        UPDATE_RESULT( result, lres );
    }
    if (pExaContext->MappedMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer)
    {
        // unmap combined chroma plane
        lres = HalUnMapMemory( pExaContext->HalHandle, pExaContext->MappedMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer );
        UPDATE_RESULT( result, lres );
    }

    TRACE(EXA_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}
