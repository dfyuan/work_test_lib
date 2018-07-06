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
 * @dom_ctrl.c
 *
 * @brief
 *   Implementation of dom ctrl.
 *
 *****************************************************************************/
/**
 * @page dom_ctrl_page DOM Ctrl
 * The Display Output Module displays image buffers in an X11 window.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref dom_ctrl_api
 * - @ref dom_ctrl_common
 * - @ref dom_ctrl
 *
 */

#ifdef DOM_FPS
#include <sys/time.h>
#endif // DOM_FPS

#include <ebase/trace.h>

#include <common/return_codes.h>
#include <common/align.h>

#include <oslayer/oslayer.h>

#include "dom_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(DOM_CTRL_INFO , "DOM-CTRL: ", INFO,  0);
CREATE_TRACER(DOM_CTRL_ERROR, "DOM-CTRL: ", ERROR, 1);

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
 * domCtrlGetState()
 *****************************************************************************/
static inline domCtrlState_t domCtrlGetState
(
    domCtrlContext_t    *pdomContext
);

/******************************************************************************
 * domCtrlSetState()
 *****************************************************************************/
static inline void domCtrlSetState
(
    domCtrlContext_t    *pdomContext,
    domCtrlState_t      newState
);


/******************************************************************************
 * domCtrlCompleteCommand()
 *****************************************************************************/
static void domCtrlCompleteCommand
(
    domCtrlContext_t        *pdomContext,
    domCtrlCmdId_t          CmdID,
    RESULT                  result
);

/******************************************************************************
 * domCtrlBufferReleaseCb()
 *****************************************************************************/
static void domCtrlBufferReleaseCb
(
    void            *pUserContext,  //!< Opaque user data pointer that was passed in on creation.
    MediaBuffer_t   *pBuffer,       //!< Pointer to buffer that is to be released.
    RESULT          DelayedResult   //!< Result of delayed buffer processing.
);

/******************************************************************************
 * domCtrlThreadHandler()
 *****************************************************************************/
static int32_t domCtrlThreadHandler
(
    void *p_arg
);

/******************************************************************************
 * domCtrlDisplayBuffer()
 *****************************************************************************/
static RESULT domCtrlDisplayBuffer
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferRGB565()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferRGB565
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferRGB666()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferRGB666
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferRGB888()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferRGB888
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferYUV444Planar()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV444Planar
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferYUV444Semi()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV444Semi
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferYUV422Planar()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Planar
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferYUV422Semi()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Semi
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferYUV422Semi3d_vertical()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Semi3d_vertical
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * domCtrlDisplayBufferYUV422Semi3d_anaglyph()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Semi3d_anaglyph
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
);



/******************************************************************************
 * ConvertYCbCr32ToRGB32()
 *****************************************************************************/
static void ConvertYCbCr32ToRGB32
(
    uint8_t     *pYCbCr32,
    uint32_t    PlaneSizePixel
);

/******************************************************************************
 * ConvertYCbCr32ToAnaglyph32()
 *****************************************************************************/
static void ConvertYCbCr32ToAnaglyph32
(
    uint8_t     *pYCbCr32_1,
    uint8_t     *pYCbCr32_2,
    uint8_t     *pYCbCr32,
    uint32_t    PlaneSizePixel
);

/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * domCtrlCreate()
 *****************************************************************************/
RESULT domCtrlCreate
(
    domCtrlContext_t    *pdomContext
)
{
    RESULT result;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pdomContext != NULL );

    // finalize init of context
    pdomContext->pCurDisplayBuffer = NULL;
#ifdef DOM_FPS
    pdomContext->FpsStartTimeMs = 0;
    pdomContext->FpsDisplayed   = 0;
    pdomContext->FpsSkipped     = 0;
#endif // DOM_FPS

    // create graphic subsystem dependent video player
    domCtrlVidplayConfig_t VidplayConfig;
    memset( &VidplayConfig, 0, sizeof(VidplayConfig) );
    VidplayConfig.hParent = pdomContext->Config.hParent;
    VidplayConfig.posX    = pdomContext->Config.posX;
    VidplayConfig.posY    = pdomContext->Config.posY;
    VidplayConfig.width   = pdomContext->Config.width;
    VidplayConfig.height  = pdomContext->Config.height;
    result = domCtrlVidplayInit( &VidplayConfig );
    if (result != RET_SUCCESS)
    {
        TRACE( DOM_CTRL_ERROR, "%s (creating video player failed)\n", __FUNCTION__ );
        return result;
    }
    pdomContext->hDomCtrlVidplay = VidplayConfig.domCtrlVidplayHandle;

    // add HAL reference
    result = HalAddRef( pdomContext->Config.HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( DOM_CTRL_ERROR, "%s (adding HAL reference failed)\n", __FUNCTION__ );
        domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
        return result;
    }

    // create command queue
    if ( OSLAYER_OK != osQueueInit( &pdomContext->CommandQueue, pdomContext->Config.MaxPendingCommands, sizeof(domCtrlCmdId_t) ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pdomContext->Config.MaxPendingCommands);
        HalDelRef( pdomContext->Config.HalHandle );
        domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
        return RET_FAILURE;
    }

    // create full buffer queue
    if ( OSLAYER_OK != osQueueInit( &pdomContext->FullBufQueue, pdomContext->Config.MaxBuffers, sizeof(MediaBuffer_t *) ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (creating buffer queue (depth: %d) failed)\n", __FUNCTION__, pdomContext->Config.MaxBuffers);
        osQueueDestroy( &pdomContext->CommandQueue );
        HalDelRef( pdomContext->Config.HalHandle );
        domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
        return RET_FAILURE;
    }

    // initialize draw mutex
    if ( OSLAYER_OK != osMutexInit( &pdomContext->drawMutex ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (connecting to media buffer queue ex failed)\n", __FUNCTION__);
        osQueueDestroy( &pdomContext->FullBufQueue );
        osQueueDestroy( &pdomContext->CommandQueue );
        HalDelRef( pdomContext->Config.HalHandle );
        domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
        return RET_FAILURE;
    }

    // allocate memory for draw context list
    pdomContext->pDrawContextList = malloc( sizeof(List) );
    if ( pdomContext->pDrawContextList == NULL )
    {
        TRACE( DOM_CTRL_ERROR, "%s (allocating memory for draw context list failed)\n", __FUNCTION__ );
        osMutexDestroy( &pdomContext->drawMutex );
        osQueueDestroy( &pdomContext->FullBufQueue );
        osQueueDestroy( &pdomContext->CommandQueue );
        HalDelRef( pdomContext->Config.HalHandle );
        domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
        return ( RET_OUTOFMEM );
    }
    //MEMSET( pdomContext->pDrawContextList, 0, sizeof( List ) );
    ListInit( pdomContext->pDrawContextList );

    // 'connect' to input queue
    pdomContext->InputQueueHighWM = BOOL_FALSE;

    // create handler thread
    if ( OSLAYER_OK != osThreadCreate( &pdomContext->Thread, domCtrlThreadHandler, pdomContext ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (creating handler thread failed)\n", __FUNCTION__);
        free( pdomContext->pDrawContextList );
        osMutexDestroy( &pdomContext->drawMutex );
        osQueueDestroy( &pdomContext->FullBufQueue );
        osQueueDestroy( &pdomContext->CommandQueue );
        HalDelRef( pdomContext->Config.HalHandle );
        domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
        return RET_FAILURE;
    }

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return RET_SUCCESS;
}


/******************************************************************************
 * domCtrlDestroy()
 *****************************************************************************/
RESULT domCtrlDestroy
(
    domCtrlContext_t *pdomContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pdomContext != NULL );

    // send handler thread a shutdown command
    lres = domCtrlSendCommand( pdomContext, DOM_CTRL_CMD_SHUTDOWN );
    if (lres != RET_SUCCESS)
    {
        TRACE(DOM_CTRL_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        UPDATE_RESULT( result, lres);
    }

    // wait for handler thread to have stopped due to the shutdown command given above
    if ( OSLAYER_OK != osThreadWait( &pdomContext->Thread ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy handler thread
    if ( OSLAYER_OK != osThreadClose( &pdomContext->Thread ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // cancel any further commands waiting in command queue
    do
    {
        // get next command from queue
        domCtrlCmdId_t CmdID;
        osStatus = osQueueTryRead( &pdomContext->CommandQueue, &CmdID );

        switch (osStatus)
        {
            case OSLAYER_OK:        // got a command, so cancel it
                domCtrlCompleteCommand( pdomContext, CmdID, RET_CANCELED );
                break;
            case OSLAYER_TIMEOUT:   // queue is empty
                break;
            default:                // something is broken...
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while (osStatus == OSLAYER_OK);

    // destroy full buffer queue
    if ( OSLAYER_OK != osQueueDestroy( &pdomContext->FullBufQueue ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (destroying full buffer queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy command queue
    if ( OSLAYER_OK != osQueueDestroy( &pdomContext->CommandQueue ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // remove HAL reference
    lres = HalDelRef( pdomContext->Config.HalHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( DOM_CTRL_ERROR, "%s (removing HAL reference failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy graphic subsystem dependent video player
    lres = domCtrlVidplayShutDown( pdomContext->hDomCtrlVidplay );
    if (lres != RET_SUCCESS)
    {
        TRACE( DOM_CTRL_ERROR, "%s (destroying video player failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy draw mutex
    if ( OSLAYER_OK != osMutexDestroy( &pdomContext->drawMutex ) )
    {
        TRACE(DOM_CTRL_ERROR, "%s (destroying draw mutex failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // deallocate memory of list and all remaining elements of list
    domCtrlDrawContext_t *pDrawCtx = (domCtrlDrawContext_t *)ListRemoveHead( pdomContext->pDrawContextList );
    while ( pDrawCtx )
    {
        if ( NULL != pDrawCtx->pIbdDrawCmds )
        {
            // Release memory of idb commands
            for ( uint32_t i = 0; i < pDrawCtx->NumDrawCmds; ++i )
            {
                if ( IBD_DRAW_TEXT == pDrawCtx->pIbdDrawCmds[i].cmdId )
                {
                    if ( NULL != pDrawCtx->pIbdDrawCmds[i].params.text.pcText )
                    {
                        free( pDrawCtx->pIbdDrawCmds[i].params.text.pcText );
                    }
                }
            }
            free( pDrawCtx->pIbdDrawCmds );
        }
        free( pDrawCtx );

        pDrawCtx = (domCtrlDrawContext_t *)ListRemoveHead( pdomContext->pDrawContextList );
    }
    free( pdomContext->pDrawContextList );

    // release display buffer?
    if (pdomContext->pCurDisplayBuffer != NULL)
    {
        free( pdomContext->pCurDisplayBuffer );
        pdomContext->pCurDisplayBuffer = NULL;
    }

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * domCtrlSendCommand()
 *****************************************************************************/
RESULT domCtrlSendCommand
(
    domCtrlContext_t    *pdomContext,
    domCtrlCmdId_t      CmdID
)
{
    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (pdomContext == NULL)
    {
        return RET_NULL_POINTER;
    }

    // are we shutting down?
    if ( domCtrlGetState( pdomContext ) == edomCtrlStateInvalid )
    {
        return RET_CANCELED;
    }

    // send command
    OSLAYER_STATUS osStatus = osQueueWrite( &pdomContext->CommandQueue, &CmdID);
    if (osStatus != OSLAYER_OK)
    {
        TRACE(DOM_CTRL_ERROR, "%s (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, domCtrlGetState( pdomContext ), osStatus);
    }

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE;
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * domCtrlGetState()
 *****************************************************************************/
static inline domCtrlState_t domCtrlGetState
(
    domCtrlContext_t    *pdomContext
)
{
    DCT_ASSERT( pdomContext != NULL );
    return pdomContext->State;
}


/******************************************************************************
 * domCtrlSetState()
 *****************************************************************************/
static inline void domCtrlSetState
(
    domCtrlContext_t    *pdomContext,
    domCtrlState_t      newState
)
{
    DCT_ASSERT( pdomContext != NULL );
    pdomContext->State = newState;
}


/******************************************************************************
 * domCtrlCompleteCommand()
 *****************************************************************************/
static void domCtrlCompleteCommand
(
    domCtrlContext_t        *pdomContext,
    domCtrlCmdId_t          CmdID,
    RESULT                  result
)
{
    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pdomContext != NULL );

    // do callback
    pdomContext->Config.domCbCompletion( CmdID, result, pdomContext->Config.pUserContext );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);
}


/******************************************************************************
 * domCtrlBufferReleaseCb()
 *****************************************************************************/
static void domCtrlBufferReleaseCb
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
    domCtrlContext_t *pdomContext = (domCtrlContext_t *)(pUserContext);

    // release buffer
    if ( pBuffer->pNext != NULL )
    {
        MediaBufUnlockBuffer( pBuffer->pNext );
    }
    MediaBufUnlockBuffer( pBuffer );

    // complete command
    domCtrlCompleteCommand( pdomContext, DOM_CTRL_CMD_PROCESS_BUFFER, DelayedResult );
}


/******************************************************************************
 * domCtrlThreadHandler()
 *****************************************************************************/
static int32_t domCtrlThreadHandler
(
    void *p_arg
)
{
    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg == NULL )
    {
        TRACE(DOM_CTRL_ERROR, "%s (arg pointer is NULL)\n", __FUNCTION__);
    }
    else
    {
        domCtrlContext_t *pdomContext = (domCtrlContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        // processing loop
        do
        {
            // set default result
            RESULT result = RET_WRONG_STATE;

            // wait for next command
            domCtrlCmdId_t CmdID;
            OSLAYER_STATUS osStatus = osQueueRead(&pdomContext->CommandQueue, &CmdID);
            if (OSLAYER_OK != osStatus)
            {
                TRACE(DOM_CTRL_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                continue; // for now we simply try again
            }

            // process command
            switch ( CmdID )
            {
                case DOM_CTRL_CMD_START:
                {
                    TRACE(DOM_CTRL_INFO, "%s (begin DOM_CTRL_CMD_START)\n", __FUNCTION__);

                    switch ( domCtrlGetState( pdomContext ) )
                    {
                        case edomCtrlStateIdle:
                        {
                            // clear display
                            result = domCtrlVidplayClear( pdomContext->hDomCtrlVidplay );
                            DCT_ASSERT(result == RET_SUCCESS);

                            // show display
                            result = domCtrlVidplayShow( pdomContext->hDomCtrlVidplay, BOOL_TRUE );
                            DCT_ASSERT(result == RET_SUCCESS);

                            // release display buffer?
                            /*if (pdomContext->pCurDisplayBuffer != NULL)
                            {
                                free( pdomContext->pCurDisplayBuffer );
                                pdomContext->pCurDisplayBuffer = NULL;
                            }*/

                            // we're up and running
                            domCtrlSetState( pdomContext, edomCtrlStateRunning );
                            result = RET_SUCCESS;

                            break;
                        }
                        default:
                        {
                            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
                        }
                    }

                    TRACE(DOM_CTRL_INFO, "%s (end DOM_CTRL_CMD_START)\n", __FUNCTION__);

                    break;
                }

                case DOM_CTRL_CMD_STOP:
                {
                    TRACE(DOM_CTRL_INFO, "%s (begin DOM_CTRL_CMD_STOP)\n", __FUNCTION__);

                    switch ( domCtrlGetState( pdomContext ) )
                    {
                        case edomCtrlStateRunning:
                        {
                            // hide display
                            result = domCtrlVidplayShow( pdomContext->hDomCtrlVidplay, BOOL_FALSE );
                            DCT_ASSERT(result == RET_SUCCESS);

                            // back to idle state
                            domCtrlSetState( pdomContext, edomCtrlStateIdle );

                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
                        }
                    }

                    TRACE(DOM_CTRL_INFO, "%s (end DOM_CTRL_CMD_STOP)\n", __FUNCTION__);

                    break;
                }

                case DOM_CTRL_CMD_SHUTDOWN:
                {
                    TRACE(DOM_CTRL_INFO, "%s (begin DOM_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    switch ( domCtrlGetState( pdomContext ) )
                    {
                        case edomCtrlStateIdle:
                        {
                            // release display buffer?
                            /*if (pdomContext->pCurDisplayBuffer != NULL)
                            {
                                free( pdomContext->pCurDisplayBuffer );
                                pdomContext->pCurDisplayBuffer = NULL;
                            }*/

                            domCtrlSetState( pdomContext, edomCtrlStateInvalid ); // stop further commands from being send to command queue
                            bExit = BOOL_TRUE;
                            result = RET_PENDING; // avoid completion below
                            break;
                        }
                        default:
                        {
                            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
                        }
                    }

                    TRACE(DOM_CTRL_INFO, "%s (end DOM_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    break;
                }

                case DOM_CTRL_CMD_PROCESS_BUFFER:
                {
                    TRACE(DOM_CTRL_INFO, "%s (begin DOM_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    MediaBuffer_t *pBuffer = NULL;

                    switch ( domCtrlGetState( pdomContext ) )
                    {
                        case edomCtrlStateIdle:
                        {
                            osStatus = osQueueTryRead( &pdomContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(DOM_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
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

                        case edomCtrlStateRunning:
                        {
                            osStatus = osQueueTryRead( &pdomContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(DOM_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                                break;
                            }

                            if ( pBuffer )
                            {
                                // very simple & crude way to avoid system stall due to input queue overflow:
                                // just skip displaying the buffer if high watermark is exceeded so that we
                                // quickly drain the queue; expected to need improvement, though
                                if (!pdomContext->InputQueueHighWM)
                                {
                                #ifdef DOM_FPS
                                    pdomContext->FpsDisplayed++;
                                #endif // DOM_FPS
                                    // display buffer
                                    result = domCtrlDisplayBuffer( pdomContext, pBuffer );

                                    // clear display after last buffer
                                    if (pBuffer->last)
                                    {
                                        // clear display
                                        result = domCtrlVidplayClear( pdomContext->hDomCtrlVidplay );
                                        DCT_ASSERT(result == RET_SUCCESS);

                                        // release display buffer?
                                        if (pdomContext->pCurDisplayBuffer != NULL)
                                        {
                                            free( pdomContext->pCurDisplayBuffer );
                                            pdomContext->pCurDisplayBuffer = NULL;
                                        }
                                    }
                                }
                            #ifdef DOM_FPS
                                else
                                {
                                    pdomContext->FpsSkipped++;
                                }

                                // get current time
                                struct timeval now;
                                gettimeofday( &now, NULL );
                                int32_t nowMs = (now.tv_sec*1000) + (now.tv_usec/1000);
                                int32_t deltaMs = nowMs - pdomContext->FpsStartTimeMs;

                                if ( !pdomContext->FpsStartTimeMs || (deltaMs > 2000) )
                                {
                                    // clear fps overlay
                                    domCtrlVidplaySetOverlayText( pdomContext->hDomCtrlVidplay, NULL );

                                    // reset measurement data (either initial run or very slow frame update...)
                                    pdomContext->FpsStartTimeMs = nowMs;
                                    pdomContext->FpsDisplayed = pdomContext->FpsSkipped = 0;
                                }
                                else if (deltaMs > 1000)
                                {
                                    char text[256] = {0};
                                    uint32_t FpsAll = pdomContext->FpsDisplayed + pdomContext->FpsSkipped;

                                    // update fps overlay
                                    snprintf( text, sizeof(text), "%4.1f fps (%df/%dms, displayed %d, skipped %d)",
                                                                  ((float)(FpsAll * 1000)) / deltaMs, FpsAll, deltaMs, pdomContext->FpsDisplayed, pdomContext->FpsSkipped );
                                    domCtrlVidplaySetOverlayText( pdomContext->hDomCtrlVidplay, text );

                                    // reset measurement data
                                    pdomContext->FpsStartTimeMs = nowMs;
                                    pdomContext->FpsDisplayed = pdomContext->FpsSkipped = 0;
                                }
                            #endif // DOM_FPS

                                // release buffer
                                if ( pBuffer->pNext != NULL )
                                {
                                    MediaBufUnlockBuffer( pBuffer->pNext );
                                }
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
                            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
                        }
                    }

                    TRACE(DOM_CTRL_INFO, "%s (end DOM_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    break;
                }

                default:
                {
                    TRACE(DOM_CTRL_ERROR, "%s (illegal command %d)\n", __FUNCTION__, CmdID);
                    result = RET_NOTSUPP;
                }
            }

            // complete command?
            if (result != RET_PENDING)
            {
                domCtrlCompleteCommand( pdomContext, CmdID, result );
            }
        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}


/******************************************************************************
 * domCtrlDisplayBuffer()
 *****************************************************************************/
static RESULT domCtrlDisplayBuffer
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pdomContext == NULL) || (pBuffer == NULL) )
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

    // depending on data format, layout & size call subroutines to:
    // ...get data into local buffer(s)
    // ...remove any trailing stuffing from lines where applicable
    // ...convert data into directly displayable format
    // ...display data
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        case PIC_BUF_TYPE_JPEG:
        case PIC_BUF_TYPE_YCbCr420:
            UPDATE_RESULT( result, RET_NOTSUPP );
            break;
        case PIC_BUF_TYPE_RGB565:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                {
                     RESULT lres = RET_NOTSUPP;
                     if ( pBuffer->pNext == NULL )
                     {
                         lres = domCtrlDisplayBufferRGB565( pdomContext, pBuffer );
                     }
                     UPDATE_RESULT( result, lres );
                     break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            break;

         case PIC_BUF_TYPE_RGB666:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                {
                     RESULT lres = RET_NOTSUPP;
                     if ( pBuffer->pNext == NULL )
                     {
                         lres = domCtrlDisplayBufferRGB666( pdomContext, pBuffer );
                     }
                     UPDATE_RESULT( result, lres );
                     break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            break;

         case PIC_BUF_TYPE_RGB888:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                {
                     RESULT lres = RET_NOTSUPP;
                     if ( pBuffer->pNext == NULL )
                     {
                         lres = domCtrlDisplayBufferRGB888( pdomContext, pBuffer );
                     }
                     UPDATE_RESULT( result, lres );
                     break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            break;

        case PIC_BUF_TYPE_YCbCr422:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_PLANAR:
                {
                    RESULT lres = RET_NOTSUPP;
                    if ( pBuffer->pNext == NULL )
                    {
                        lres = domCtrlDisplayBufferYUV422Planar( pdomContext, pBuffer );
                    }
                    UPDATE_RESULT( result, lres );
                    break;
                }

                case PIC_BUF_LAYOUT_COMBINED:
                    UPDATE_RESULT( result, RET_NOTSUPP );
                    break;

                case PIC_BUF_LAYOUT_SEMIPLANAR:
                {
                    RESULT lres = RET_NOTSUPP;
                    if ( pBuffer->pNext == NULL )
                    {
                        lres = domCtrlDisplayBufferYUV422Semi( pdomContext, pBuffer );
                    }
                    else
                    {
                        switch (pdomContext->ImgPresent)
                        {
                            case DOMCTRL_IMAGE_PRESENTATION_3D_VERTICAL:
                                lres = domCtrlDisplayBufferYUV422Semi3d_vertical( pdomContext, pBuffer );
                                break;
                            case DOMCTRL_IMAGE_PRESENTATION_3D_ANAGLYPH:
                                lres = domCtrlDisplayBufferYUV422Semi3d_anaglyph( pdomContext, pBuffer );
                                break;
                            default:
                                lres = domCtrlDisplayBufferYUV422Semi( pdomContext, pBuffer );
                        }
                    }
                    UPDATE_RESULT( result, lres );
                    break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            };
            break;

        case PIC_BUF_TYPE_YCbCr444:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_PLANAR:
                {
                    RESULT lres = RET_NOTSUPP;
                    if ( pBuffer->pNext == NULL )
                    {
                        lres = domCtrlDisplayBufferYUV444Planar( pdomContext, pBuffer );
                    }
                    UPDATE_RESULT( result, lres );
                    break;
                }

                case PIC_BUF_LAYOUT_COMBINED:
                    UPDATE_RESULT( result, RET_NOTSUPP );
                    break;

                case PIC_BUF_LAYOUT_SEMIPLANAR:
                {
                    RESULT lres = RET_NOTSUPP;
                    if ( pBuffer->pNext == NULL )
                    {
                        lres = domCtrlDisplayBufferYUV444Semi( pdomContext, pBuffer );
                    }
                    UPDATE_RESULT( result, lres );
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
        return result;
    }

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * domCtrlDisplayBufferRGB565()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferRGB565
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t PlaneSize = pPicBufMetaData->Data.RGB.combined.PicWidthBytes * pPicBufMetaData->Data.RGB.combined.PicHeightPixel;
    uint8_t *pBase;

    pBase  = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf)), pPicBufMetaData->Align);

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.RGB.combined.pBuffer),  pBase,  PlaneSize );
    UPDATE_RESULT( result, lres );

    uint8_t *pRGB565 = malloc( (PlaneSize << 1) );      // 16 bit per pixel -> 32 bit per pixel
    if (pRGB565 == NULL)
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        uint8_t *pBaseTmp   = pBase;
        uint8_t *pRGB565Tmp = pRGB565;
        uint32_t x,y;

        for ( y=0; y < pPicBufMetaData->Data.RGB.combined.PicHeightPixel; y++ )
        {
            uint16_t *pPixel = (uint16_t *)pBaseTmp;

            // walk through line
            for ( x=0; x < pPicBufMetaData->Data.RGB.combined.PicWidthPixel; x++ )
            {
                uint16_t pixel = *pPixel++;

                int32_t R = ( pixel & 0xF800U ) >> 11U;
                int32_t G = ( pixel & 0x07E0U ) >> 5U;
                int32_t B = ( pixel & 0x001FU ) >> 0U;

                B <<= 3U;
                G <<= 2U;
                R <<= 3U;

                // clip
                if (R<0) R=0; else if (R>255) R=255;
                if (G<0) G=0; else if (G>255) G=255;
                if (B<0) B=0; else if (B>255) B=255;

                *pRGB565Tmp++ = (uint8_t)B;
                *pRGB565Tmp++ = (uint8_t)G;
                *pRGB565Tmp++ = (uint8_t)R;
                // qt needs this alpha value
                *pRGB565Tmp++ = 0xff;
            }

            pBaseTmp += pPicBufMetaData->Data.RGB.combined.PicWidthBytes;
        }

        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pRGB565;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData->Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = pPicBufMetaData->Data.RGB.combined.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pRGB565;
    }

    // free local buffer
    free( pLocBuf );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}



/******************************************************************************
 * domCtrlDisplayBufferRGB666()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferRGB666
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t PlaneSize = pPicBufMetaData->Data.RGB.combined.PicWidthBytes * pPicBufMetaData->Data.RGB.combined.PicHeightPixel;
    uint8_t *pBase;

    pBase  = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf)), pPicBufMetaData->Align);

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.RGB.combined.pBuffer),  pBase,  PlaneSize );
    UPDATE_RESULT( result, lres );

    uint8_t *pRGB666 = malloc( PlaneSize );
    if (pRGB666 == NULL)
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        uint8_t *pBaseTmp   = pBase;
        uint8_t *pRGB666Tmp = pRGB666;
        uint32_t x,y;

        for ( y=0; y < pPicBufMetaData->Data.RGB.combined.PicHeightPixel; y++ )
        {
            uint8_t *pC = pBaseTmp;

            // walk through line
            for ( x=0; x < pPicBufMetaData->Data.RGB.combined.PicWidthPixel; x++ )
            {
                int32_t B = *pC++;
                int32_t G = *pC++;
                int32_t R = *pC++;
                pC++;

                B <<= 2U;
                G <<= 2U;
                R <<= 2U;

                // clip
                if (R<0) R=0; else if (R>255) R=255;
                if (G<0) G=0; else if (G>255) G=255;
                if (B<0) B=0; else if (B>255) B=255;

                *pRGB666Tmp++ = (uint8_t)B;
                *pRGB666Tmp++ = (uint8_t)G;
                *pRGB666Tmp++ = (uint8_t)R;
                // qt needs this alpha value
                *pRGB666Tmp++ = 0xff;
            }

            pBaseTmp += pPicBufMetaData->Data.RGB.combined.PicWidthBytes;
        }

        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pRGB666;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData->Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = pPicBufMetaData->Data.RGB.combined.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pRGB666;
    }

    // free local buffer
    free( pLocBuf );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}



/******************************************************************************
 * domCtrlDisplayBufferRGB888()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferRGB888
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t PlaneSize = pPicBufMetaData->Data.RGB.combined.PicWidthBytes * pPicBufMetaData->Data.RGB.combined.PicHeightPixel;
    uint8_t *pBase;

    pBase  = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf)), pPicBufMetaData->Align);

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.RGB.combined.pBuffer),  pBase,  PlaneSize );
    UPDATE_RESULT( result, lres );

    uint8_t *pRGB888 = malloc( PlaneSize );
    if (pRGB888 == NULL)
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        uint8_t *pBaseTmp   = pBase;
        uint8_t *pRGB888Tmp = pRGB888;
        uint32_t x,y;

        for ( y=0; y < pPicBufMetaData->Data.RGB.combined.PicHeightPixel; y++ )
        {
            uint8_t *pC = pBaseTmp;

            // walk through line
            for ( x=0; x < pPicBufMetaData->Data.RGB.combined.PicWidthPixel; x++ )
            {
                int32_t B = *pC++;
                int32_t G = *pC++;
                int32_t R = *pC++;
                pC++;

                *pRGB888Tmp++ = (uint8_t)B;
                *pRGB888Tmp++ = (uint8_t)G;
                *pRGB888Tmp++ = (uint8_t)R;
                // qt needs this alpha value
                *pRGB888Tmp++ = 0xff;
            }

            pBaseTmp += pPicBufMetaData->Data.RGB.combined.PicWidthBytes;
        }

        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pRGB888;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData->Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = pPicBufMetaData->Data.RGB.combined.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pRGB888;
    }

    // free local buffer
    free( pLocBuf );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * domCtrlDisplayBufferYUV444Planar()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV444Planar
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}

/******************************************************************************
 * domCtrlDisplayBufferYUV444Semi()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV444Semi
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }
    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}

/******************************************************************************
 * domCtrlDisplayBufferYUV422Planar()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Planar
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize = pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthBytes * pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel;
    uint8_t *pYTmp, *pYBase, *pCbTmp, *pCbBase, *pCrTmp, *pCrBase;

    pYTmp  = pYBase  = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf))                  , pPicBufMetaData->Align); // pPicBufMetaData->Data.YCbCr.planar.Y.pBuffer;
    pCbTmp = pCbBase = (uint8_t *) ALIGN_UP( ((ulong_t)(pYTmp)) +       YCPlaneSize, pPicBufMetaData->Align); // pPicBufMetaData->Data.YCbCr.planar.Cb.pBuffer;
    pCrTmp = pCrBase = (uint8_t *) ALIGN_UP( ((ulong_t)(pCbTmp)) + (YCPlaneSize>>1), pPicBufMetaData->Align); // pPicBufMetaData->Data.YCbCr.planar.Cr.pBuffer;

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.planar.Y.pBuffer),  pYBase,  YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // get chroma-blue plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.planar.Cb.pBuffer), pCbBase, (YCPlaneSize>>1) );
    UPDATE_RESULT( result, lres );

    // get chroma-red plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.planar.Cr.pBuffer), pCrBase, (YCPlaneSize>>1) );
    UPDATE_RESULT( result, lres );

    // we need a temporary helper buffer capable of holding 4 times the YPlane size (upscaled to 4:4:4 by pixel replication + alpha)
    uint8_t *pYCbCr32 = malloc( 4*YCPlaneSize );
    if (pYCbCr32 == NULL)
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        // upscale and combine each 4:2:2 pixel to 4:4:4+alpha while removing any gaps at line ends as well
        uint8_t *pYCbCr32Tmp = pYCbCr32;
        uint32_t x,y;
        for ( y=0; y < pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel; y++ )
        {
            // get line starts
            uint8_t *pY     = pYTmp;
            uint8_t *pCb    = pCbTmp;
            uint8_t *pCr    = pCrTmp;

            // walk through line
            for ( x=0; x < pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthPixel; x+=2 )
            {
                int32_t Cb = *pCb++;
                int32_t Cr = *pCr++;
                int32_t Y1 = *pY++;
                int32_t Y2 = *pY++;

                Y1 -=  16;
                Y2 -=  16;
                Cb -= 128;
                Cr -= 128;

                int32_t R1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                int32_t G1 = ( ((int32_t)(1.164*1024))*Y1 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                int32_t B1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                int32_t R2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                int32_t G2 = ( ((int32_t)(1.164*1024))*Y2 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                int32_t B2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                // clip
                if (R1<0) R1=0; else if (R1>255) R1=255;
                if (G1<0) G1=0; else if (G1>255) G1=255;
                if (B1<0) B1=0; else if (B1>255) B1=255;

                if (R2<0) R2=0; else if (R2>255) R2=255;
                if (G2<0) G2=0; else if (G2>255) G2=255;
                if (B2<0) B2=0; else if (B2>255) B2=255;

                *pYCbCr32Tmp++ = (uint8_t)B1;
                *pYCbCr32Tmp++ = (uint8_t)G1;
                *pYCbCr32Tmp++ = (uint8_t)R1;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;

                *pYCbCr32Tmp++ = (uint8_t)B2;
                *pYCbCr32Tmp++ = (uint8_t)G2;
                *pYCbCr32Tmp++ = (uint8_t)R2;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;
            }

            // update line starts
            pYTmp  += pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthBytes;
            pCbTmp += pPicBufMetaData->Data.YCbCr.planar.Cb.PicWidthBytes;
            pCrTmp += pPicBufMetaData->Data.YCbCr.planar.Cr.PicWidthBytes;
        }

        // convert consecutive YCbCr32 to RGB32; both are combined color component planes with 32bit aligned pixels (AAYYCbCr resp. AARRGGBB)
        // note: Qt4.6 only supports RGB formats of which RGB32 seems to be by far the fastet format - at least on the tested systems...
        // ConvertYCbCr32ToRGB32( pYCbCr32, YCPlaneSize );

        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pYCbCr32;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pYCbCr32;
    }

    // free local buffer
    free( pLocBuf );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * domCtrlDisplayBufferYUV422Semi()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Semi
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    uint8_t *pYTmp, *pYBase, *pCbCrTmp, *pCbCrBase;
    pYTmp    = pYBase    = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf)),              pPicBufMetaData->Align); // pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer;
    pCbCrTmp = pCbCrBase = (uint8_t *) ALIGN_UP( ((ulong_t)(pYTmp)) + YCPlaneSize , pPicBufMetaData->Align); // pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer;

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase,    YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase, YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // Draw using ibd
    PicBufMetaData_t pLocPicBufMetaData = *pPicBufMetaData;
    pLocPicBufMetaData.Data.YCbCr.semiplanar.Y.pBuffer = pYBase;
    pLocPicBufMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer = pCbCrBase;

    osMutexLock( &pdomContext->drawMutex );
    if ( 1 != ListEmpty( pdomContext->pDrawContextList ) )
    {
        domCtrlDrawContext_t *pDrawCtx = ( domCtrlDrawContext_t *)ListHead( pdomContext->pDrawContextList );
        ibdHandle_t ibdHandle = NULL;

        while ( pDrawCtx )
        {
            if ( NULL != pDrawCtx->pIbdDrawCmds )
            {
                if ( NULL == ibdHandle )
                {
                    TRACE(DOM_CTRL_INFO, "%s (draw)\n", __FUNCTION__);

                    ibdHandle = ibdOpenDirect( &pLocPicBufMetaData );
                    if ( NULL == ibdHandle )
                    {
                        TRACE( DOM_CTRL_ERROR, "%s ibdOpenDirect() failed.\n", __FUNCTION__ );
                        break;
                    }
                }

                RESULT lres;
                lres = ibdDraw( ibdHandle, pDrawCtx->NumDrawCmds, pDrawCtx->pIbdDrawCmds, true );
                UPDATE_RESULT( result, lres);
            }
            pDrawCtx = pDrawCtx->p_next;
        }

        if ( NULL != ibdHandle )
        {
            if ( RET_SUCCESS != ibdClose( ibdHandle ) )
            {
                TRACE( DOM_CTRL_ERROR, "%s ibdClose() failed\n", __FUNCTION__ );
            }
        }
    }
    osMutexUnlock( &pdomContext->drawMutex );

    // we need a temporary helper buffer capable of holding 4 times the YPlane size (upscaled to 4:4:4 by pixel replication + alpha)
    uint8_t *pYCbCr32 = malloc( 4*YCPlaneSize );
    if (pYCbCr32 == NULL)
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        // upscale and combine each 4:2:2 pixel to 4:4:4+alpha while removing any gaps at line ends as well
        uint8_t *pYCbCr32Tmp = pYCbCr32;
        uint32_t x,y;
        for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
        {
            // get line starts
            uint8_t *pY = pYTmp;
            uint8_t *pC = pCbCrTmp;

            // walk through line
            for (x=0; x < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
            {
                int32_t Cb = *pC++;
                int32_t Cr = *pC++;
                int32_t Y1 = *pY++;
                int32_t Y2 = *pY++;

                Y1 -=  16;
                Y2 -=  16;
                Cb -= 128;
                Cr -= 128;

                int32_t R1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                int32_t G1 = ( ((int32_t)(1.164*1024))*Y1 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                int32_t B1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                int32_t R2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                int32_t G2 = ( ((int32_t)(1.164*1024))*Y2 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                int32_t B2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                // clip
                if (R1<0) R1=0; else if (R1>255) R1=255;
                if (G1<0) G1=0; else if (G1>255) G1=255;
                if (B1<0) B1=0; else if (B1>255) B1=255;

                if (R2<0) R2=0; else if (R2>255) R2=255;
                if (G2<0) G2=0; else if (G2>255) G2=255;
                if (B2<0) B2=0; else if (B2>255) B2=255;

                *pYCbCr32Tmp++ = (uint8_t)B1;
                *pYCbCr32Tmp++ = (uint8_t)G1;
                *pYCbCr32Tmp++ = (uint8_t)R1;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;

                *pYCbCr32Tmp++ = (uint8_t)B2;
                *pYCbCr32Tmp++ = (uint8_t)G2;
                *pYCbCr32Tmp++ = (uint8_t)R2;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;
            }

            // update line starts
            pYTmp    += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
        }

        // convert consecutive YCbCr32 to RGB32; both are combined color component planes with 32bit aligned pixels (AAYYCbCr resp. AARRGGBB)
        // note: Qt4.6 only supports RGB formats of which RGB32 seems to be by far the fastet format - at least on the tested systems...
        // ConvertYCbCr32ToRGB32( pYCbCr32, YCPlaneSize );

        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pYCbCr32;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pYCbCr32;
    }

    // free local buffer
    free( pLocBuf );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * domCtrlDisplayBufferYUV422Semi3d_vertical()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Semi3d_vertical
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer1
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    MediaBuffer_t *pBuffer2 = (MediaBuffer_t *)pBuffer1->pNext;

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData1 = (PicBufMetaData_t *)(pBuffer1->pMetaData);
    if (pPicBufMetaData1 == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // get & check buffer meta data of second buffer
    PicBufMetaData_t *pPicBufMetaData2 = (PicBufMetaData_t *)(pBuffer2->pMetaData);
    if (pPicBufMetaData2 == NULL)
    {
        return RET_NULL_POINTER;
    }



    // allocate local buffer
    uint8_t *pLocBuf1 = malloc(MAX_ALIGNED_SIZE(pBuffer1->baseSize, pPicBufMetaData1->Align));
    if (pLocBuf1 == NULL)
    {
        return RET_OUTOFMEM;
    }

    DCT_ASSERT( ((pPicBufMetaData1 != NULL) && (pPicBufMetaData2 != NULL)) );

    DCT_ASSERT( pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthBytes  == pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicWidthBytes );
    DCT_ASSERT( pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel == pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicHeightPixel );

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize1 = pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    uint8_t *pYTmp1, *pYBase1, *pCbCrTmp1, *pCbCrBase1;
    pYTmp1    = pYBase1    = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf1)),               pPicBufMetaData1->Align); // pPicBufMetaData1->Data.YCbCr.semiplanar.Y.pBuffer;
    pCbCrTmp1 = pCbCrBase1 = (uint8_t *) ALIGN_UP( ((ulong_t)(pYTmp1)) + YCPlaneSize1 , pPicBufMetaData1->Align); // pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.pBuffer;

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData1->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase1,    YCPlaneSize1 );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase1, YCPlaneSize1 );
    UPDATE_RESULT( result, lres );

    // allocate local buffer
    uint8_t *pLocBuf2 = malloc(MAX_ALIGNED_SIZE(pBuffer2->baseSize, pPicBufMetaData2->Align));
    if (pLocBuf2 == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize2 = pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    uint8_t *pYTmp2, *pYBase2, *pCbCrTmp2, *pCbCrBase2;
    pYTmp2    = pYBase2    = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf2)),               pPicBufMetaData2->Align); // pPicBufMetaData1->Data.YCbCr.semiplanar.Y.pBuffer;
    pCbCrTmp2 = pCbCrBase2 = (uint8_t *) ALIGN_UP( ((ulong_t)(pYTmp2)) + YCPlaneSize2 , pPicBufMetaData2->Align); // pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.pBuffer;

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData2->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase2,    YCPlaneSize2 );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData2->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase2, YCPlaneSize2 );
    UPDATE_RESULT( result, lres );

    // Draw using ibd
    PicBufMetaData_t pLocPicBufMetaData = *pPicBufMetaData1;
    pLocPicBufMetaData.Data.YCbCr.semiplanar.Y.pBuffer = pYBase1;
    pLocPicBufMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer = pCbCrBase1;

    osMutexLock( &pdomContext->drawMutex );
    if ( 1 != ListEmpty( pdomContext->pDrawContextList ) )
    {
        domCtrlDrawContext_t *pDrawCtx = ( domCtrlDrawContext_t *)ListHead( pdomContext->pDrawContextList );
        ibdHandle_t ibdHandle = NULL;

        while ( pDrawCtx )
        {
            if ( NULL != pDrawCtx->pIbdDrawCmds )
            {
                if ( NULL == ibdHandle )
                {
                    TRACE(DOM_CTRL_INFO, "%s (draw)\n", __FUNCTION__);

                    ibdHandle = ibdOpenDirect( &pLocPicBufMetaData );
                    if ( NULL == ibdHandle )
                    {
                        TRACE( DOM_CTRL_ERROR, "%s ibdOpenDirect() failed.\n", __FUNCTION__ );
                        break;
                    }
                }

                RESULT lres;
                lres = ibdDraw( ibdHandle, pDrawCtx->NumDrawCmds, pDrawCtx->pIbdDrawCmds, true );
                UPDATE_RESULT( result, lres);
            }
            pDrawCtx = pDrawCtx->p_next;
        }

        if ( NULL != ibdHandle )
        {
            if ( RET_SUCCESS != ibdClose( ibdHandle ) )
            {
                TRACE( DOM_CTRL_ERROR, "%s ibdClose() failed\n", __FUNCTION__ );
            }
        }
    }
    osMutexUnlock( &pdomContext->drawMutex );

    // we need a temporary helper buffer capable of holding 4 times the YPlane size (upscaled to 4:4:4 by pixel replication + alpha)
    uint8_t *pYCbCr32 = malloc( 2 * 4 * YCPlaneSize1 );
    if (pYCbCr32 == NULL)
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        // upscale and combine each 4:2:2 pixel to 4:4:4+alpha while removing any gaps at line ends as well
        uint8_t *pYCbCr32Tmp = pYCbCr32;
        //uint8_t *pYCbCr32Tmp2 = ( pYCbCr32 + 4*pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthPixel/2 );

        uint32_t x,y;
        for (y=0; y < pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
        {
            // get line starts
            uint8_t *pY1 = pYTmp1;
            uint8_t *pC1 = pCbCrTmp1;

            int32_t R1;
            int32_t G1;
            int32_t B1;

            int32_t R2;
            int32_t G2;
            int32_t B2;

            // walk through line
            for (x=0; x < pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
            {
                int32_t Cb = *pC1++;
                int32_t Cr = *pC1++;
                int32_t Y1 = *pY1++;
                int32_t Y2 = *pY1++;

                Y1 -=  16;
                Y2 -=  16;
                Cb -= 128;
                Cr -= 128;

                R1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                G1 = ( ((int32_t)(1.164*1024))*Y1 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                B1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                R2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                G2 = ( ((int32_t)(1.164*1024))*Y2 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                B2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                // clip
                if (R1<0) R1=0; else if (R1>255) R1=255;
                if (G1<0) G1=0; else if (G1>255) G1=255;
                if (B1<0) B1=0; else if (B1>255) B1=255;

                if (R2<0) R2=0; else if (R2>255) R2=255;
                if (G2<0) G2=0; else if (G2>255) G2=255;
                if (B2<0) B2=0; else if (B2>255) B2=255;

                *pYCbCr32Tmp++ = (uint8_t)B1;
                *pYCbCr32Tmp++ = (uint8_t)G1;
                *pYCbCr32Tmp++ = (uint8_t)R1;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;

                *pYCbCr32Tmp++ = (uint8_t)B2;
                *pYCbCr32Tmp++ = (uint8_t)G2;
                *pYCbCr32Tmp++ = (uint8_t)R2;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;
            }

            // update line starts
            pYTmp1    += pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            pCbCrTmp1 += pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
        }

        for (y=0; y < pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
        {
            // get line starts
            uint8_t *pY1 = pYTmp2;
            uint8_t *pC1 = pCbCrTmp2;

            int32_t R1;
            int32_t G1;
            int32_t B1;

            int32_t R2;
            int32_t G2;
            int32_t B2;

            // walk through line
            for (x=0; x < pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
            {
                int32_t Cb = *pC1++;
                int32_t Cr = *pC1++;
                int32_t Y1 = *pY1++;
                int32_t Y2 = *pY1++;

                Y1 -=  16;
                Y2 -=  16;
                Cb -= 128;
                Cr -= 128;

                R1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                G1 = ( ((int32_t)(1.164*1024))*Y1 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                B1 = ( ((int32_t)(1.164*1024))*Y1 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                R2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
                G2 = ( ((int32_t)(1.164*1024))*Y2 - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
                B2 = ( ((int32_t)(1.164*1024))*Y2 + ((int32_t)(2.018*1024))*Cb                              ) >> 10;

                // clip
                if (R1<0) R1=0; else if (R1>255) R1=255;
                if (G1<0) G1=0; else if (G1>255) G1=255;
                if (B1<0) B1=0; else if (B1>255) B1=255;

                if (R2<0) R2=0; else if (R2>255) R2=255;
                if (G2<0) G2=0; else if (G2>255) G2=255;
                if (B2<0) B2=0; else if (B2>255) B2=255;

                *pYCbCr32Tmp++ = (uint8_t)B1;
                *pYCbCr32Tmp++ = (uint8_t)G1;
                *pYCbCr32Tmp++ = (uint8_t)R1;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;

                *pYCbCr32Tmp++ = (uint8_t)B2;
                *pYCbCr32Tmp++ = (uint8_t)G2;
                *pYCbCr32Tmp++ = (uint8_t)R2;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;
            }

            // update line starts
            pYTmp2    += pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            pCbCrTmp2 += pPicBufMetaData2->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
        }


        // convert consecutive YCbCr32 to RGB32; both are combined color component planes with 32bit aligned pixels (AAYYCbCr resp. AARRGGBB)
        // note: Qt4.6 only supports RGB formats of which RGB32 seems to be by far the fastet format - at least on the tested systems...
        // ConvertYCbCr32ToRGB32( pYCbCr32, 2 * YCPlaneSize1 );

        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pYCbCr32;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = 2 * pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pYCbCr32;
    }

    // free local buffer
    free( pLocBuf2 );
    free( pLocBuf1 );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}



/******************************************************************************
 * domCtrlDisplayBufferYUV422Semi3d_anaglyph()
 *****************************************************************************/
static RESULT domCtrlDisplayBufferYUV422Semi3d_anaglyph
(
    domCtrlContext_t    *pdomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(DOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pdomContext)
    {
        return RET_NULL_POINTER;
    }

    MediaBuffer_t *pBuffer2 = (MediaBuffer_t *)pBuffer->pNext;

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData1 = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData1 == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    // get & check buffer meta data of second buffer
    PicBufMetaData_t *pPicBufMetaData2 = (PicBufMetaData_t *)(pBuffer2->pMetaData);
    if (pPicBufMetaData2 == NULL)
    {
        return RET_NULL_POINTER;
    }

    // allocate local buffer
    uint8_t *pLocBuf1 = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData1->Align));
    if (pLocBuf1 == NULL)
    {
        return RET_OUTOFMEM;
    }

    // allocate local buffer
    uint8_t *pLocBuf2 = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData2->Align));
    if (pLocBuf1 == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize1 = pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    uint8_t *pYTmp1, *pYBase1, *pCbCrTmp1, *pCbCrBase1;
    pYTmp1    = pYBase1    = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf1))               , pPicBufMetaData1->Align); // pPicBufMetaData1->Data.YCbCr.semiplanar.Y.pBuffer;
    pCbCrTmp1 = pCbCrBase1 = (uint8_t *) ALIGN_UP( ((ulong_t)(pYTmp1)) + YCPlaneSize1  , pPicBufMetaData1->Align); // pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.pBuffer;

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData1->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase1,    YCPlaneSize1 );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase1, YCPlaneSize1 );
    UPDATE_RESULT( result, lres );

    // get base addresses & sizes of local planes
    uint32_t YCPlaneSize2 = pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    uint8_t *pYTmp2, *pYBase2, *pCbCrTmp2, *pCbCrBase2;
    pYTmp2    = pYBase2    = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf2))                , pPicBufMetaData2->Align); // pPicBufMetaData2->Data.YCbCr.semiplanar.Y.pBuffer;
    pCbCrTmp2 = pCbCrBase2 = (uint8_t *) ALIGN_UP( ((ulong_t)(pYTmp2)) + YCPlaneSize2   , pPicBufMetaData2->Align); // pPicBufMetaData2->Data.YCbCr.semiplanar.CbCr.pBuffer;

    // get luma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData2->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase2,    YCPlaneSize2 );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pdomContext->Config.HalHandle, (ulong_t)(pPicBufMetaData2->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase2, YCPlaneSize2 );
    UPDATE_RESULT( result, lres );

    // we need a temporary helper buffer capable of holding 4 times the YPlane size (upscaled to 4:4:4 by pixel replication + alpha)
    uint8_t *pYCbCr32 = malloc( 4*YCPlaneSize1 );
    if ( pYCbCr32 == NULL )
    {
        UPDATE_RESULT( result, RET_OUTOFMEM );
    }
    else
    {
        // upscale and combine each 4:2:2 pixel to 4:4:4+alpha while removing any gaps at line ends as well
        uint8_t *pYCbCr32Tmp = pYCbCr32;
        uint32_t x,y;
        for (y=0; y < pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
        {
            // get line starts
            uint8_t *pY_1 = pYTmp1;
            uint8_t *pC_1 = pCbCrTmp1;

            uint8_t *pY_2 = pYTmp2;
            uint8_t *pC_2 = pCbCrTmp2;

            // walk through line
            for (x=0; x < pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
            {
                int32_t Cb_1  = *pC_1++;
                int32_t Cr_1  = *pC_1++;
                int32_t Y_1_1 = *pY_1++;
                int32_t Y_1_2 = *pY_1++;

                int32_t Cb_2  = *pC_2++;
                int32_t Cr_2  = *pC_2++;
                int32_t Y_2_1 = *pY_2++;
                int32_t Y_2_2 = *pY_2++;

                Y_1_1   -=  16;
                Y_1_2   -=  16;
                Cb_1    -= 128;
                Cr_1    -= 128;

                Y_2_1   -=  16;
                Y_2_2   -=  16;
                Cb_2    -= 128;
                Cr_2    -= 128;

                //int32_t R_1_1 = ( ((int32_t)(1.164*1024))*Y_1_1 + ((int32_t)(1.596*1024))*Cr_1                                ) >> 10;
                int32_t G_1_1 = ( ((int32_t)(1.164*1024))*Y_1_1 - ((int32_t)(0.813*1024))*Cr_1 - ((int32_t)(0.391*1024))*Cb_1 ) >> 10;
                int32_t B_1_1 = ( ((int32_t)(1.164*1024))*Y_1_1 + ((int32_t)(2.018*1024))*Cb_1                                ) >> 10;

                //int32_t R_1_2 = ( ((int32_t)(1.164*1024))*Y_1_2 + ((int32_t)(1.596*1024))*Cr_1                                ) >> 10;
                int32_t G_1_2 = ( ((int32_t)(1.164*1024))*Y_1_2 - ((int32_t)(0.813*1024))*Cr_1 - ((int32_t)(0.391*1024))*Cb_1 ) >> 10;
                int32_t B_1_2 = ( ((int32_t)(1.164*1024))*Y_1_2 + ((int32_t)(2.018*1024))*Cb_1                                ) >> 10;

                //int32_t R_2_1 = ( ((int32_t)(1.164*1024))*Y_2_1 + ((int32_t)(1.596*1024))*Cr_2                                ) >> 10;
                int32_t G_2_1 = ( ((int32_t)(1.164*1024))*Y_2_1 - ((int32_t)(0.813*1024))*Cr_2 - ((int32_t)(0.391*1024))*Cb_2 ) >> 10;
                int32_t B_2_1 = ( ((int32_t)(1.164*1024))*Y_2_1 + ((int32_t)(2.018*1024))*Cb_2                                ) >> 10;

                //int32_t R_2_2 = ( ((int32_t)(1.164*1024))*Y_2_2 + ((int32_t)(1.596*1024))*Cr_2                                ) >> 10;
                int32_t G_2_2 = ( ((int32_t)(1.164*1024))*Y_2_2 - ((int32_t)(0.813*1024))*Cr_2 - ((int32_t)(0.391*1024))*Cb_2 ) >> 10;
                int32_t B_2_2 = ( ((int32_t)(1.164*1024))*Y_2_2 + ((int32_t)(2.018*1024))*Cb_2                                ) >> 10;

                // clip
                //if ( R_1_1 <0 ) R_1_1=0; else if (R_1_1>255) R_1_1=255;
                if ( G_1_1 <0 ) G_1_1=0; else if (G_1_1>255) G_1_1=255;
                if ( B_1_1 <0 ) B_1_1=0; else if (B_1_1>255) B_1_1=255;
                //if ( R_1_2 <0 ) R_1_2=0; else if (R_1_2>255) R_1_2=255;
                if ( G_1_2 <0 ) G_1_2=0; else if (G_1_2>255) G_1_2=255;
                if ( B_1_2 <0 ) B_1_2=0; else if (B_1_2>255) B_1_2=255;

                //if ( R_2_1 <0 ) R_2_1=0; else if (R_2_1>255) R_2_1=255;
                if ( G_2_1 <0 ) G_2_1=0; else if (G_2_1>255) G_2_1=255;
                if ( B_2_1 <0 ) B_2_1=0; else if (B_2_1>255) B_2_1=255;
                //if ( R_2_2 <0 ) R_2_2=0; else if (R_2_2>255) R_2_2=255;
                if ( G_2_2 <0 ) G_2_2=0; else if (G_2_2>255) G_2_2=255;
                if ( B_2_2 <0 ) B_2_2=0; else if (B_2_2>255) B_2_2=255;

                //  RGB -> ANAGLYPH
                //
                //  | R |      | 0.0 0.7 0.3 |   | R_a |    | 0.0 0.0 0.0 |   | R_b |
                //  | G |   =  | 0.0 0.0 0.0 | * | G_a |  + | 0.0 1.0 0.0 | * | G_b |
                //  | B |      | 0.0 0.0 0.0 |   | B_a |    | 0.0 0.0 1.0 |   | B_b |
                //
                int32_t R1 = ( ((int32_t)(0.7*1024)) * G_1_1 + ((int32_t)(0.3*1024)) * B_1_1 ) >> 10;
                int32_t G1 = ( ((int32_t)(1.0*1024)) * G_2_1 ) >> 10;
                int32_t B1 = ( ((int32_t)(1.0*1024)) * B_2_1 ) >> 10;

                int32_t R2 = ( ((int32_t)(0.7*1024)) * G_1_2 + ((int32_t)(0.3*1024)) * B_1_2 ) >> 10;
                int32_t G2 = ( ((int32_t)(1.0*1024)) * G_2_2 ) >> 10;
                int32_t B2 = ( ((int32_t)(1.0*1024)) * B_2_2 ) >> 10;

                *pYCbCr32Tmp++ = B1;
                *pYCbCr32Tmp++ = G1;
                *pYCbCr32Tmp++ = R1;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;

                *pYCbCr32Tmp++ = B2;
                *pYCbCr32Tmp++ = G2;
                *pYCbCr32Tmp++ = R2;
                // qt needs this alpha value
                *pYCbCr32Tmp++ = 0xff;
            }

            // update line starts
            pYTmp1    += pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            pCbCrTmp1 += pPicBufMetaData1->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;

            pYTmp2    += pPicBufMetaData2->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            pCbCrTmp2 += pPicBufMetaData2->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
        }


        // prepare a set of picture buffer meta data describing this buffer
        PicBufMetaData_t PicBuf;
        PicBuf.Type                             = PIC_BUF_TYPE_RGB32;
        PicBuf.Layout                           = PIC_BUF_LAYOUT_COMBINED;
        PicBuf.Data.RGB.combined.pBuffer        = pYCbCr32;
        PicBuf.Data.RGB.combined.PicWidthPixel  = pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicWidthBytes  = 4 * PicBuf.Data.RGB.combined.PicWidthPixel;
        PicBuf.Data.RGB.combined.PicHeightPixel = pPicBufMetaData1->Data.YCbCr.semiplanar.Y.PicHeightPixel;

        // finally display buffer
        lres = domCtrlVidplayDisplay( pdomContext->hDomCtrlVidplay, &PicBuf );
        UPDATE_RESULT( result, lres );
        if (lres != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, domCtrlGetState( pdomContext ));
        }

        // release previous display buffer?
        if (pdomContext->pCurDisplayBuffer != NULL)
        {
            free( pdomContext->pCurDisplayBuffer );
        }

        // update current display buffer
        pdomContext->pCurDisplayBuffer = pYCbCr32;
    }

    // free local buffer
    free( pLocBuf2 );
    free( pLocBuf1 );

    TRACE(DOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}



/******************************************************************************
 * ConvertYCbCr32ToAnaglyph32()
 *****************************************************************************/
static void ConvertYCbCr32ToAnaglyph32
(
    uint8_t     *pYCbCr32_1,
    uint8_t     *pYCbCr32_2,
    uint8_t     *pYCbCr32,
    uint32_t    PlaneSizePixel
)
{
    uint8_t *pRGB32 = pYCbCr32; // where to put the RGB data
    uint32_t pix;

    for (pix = 0; pix < PlaneSizePixel; pix++)
    {
        // get YCbCr32_1 pixel data
        int32_t Cr_1 = *pYCbCr32_1++;
        int32_t Cb_1 = *pYCbCr32_1++;
        int32_t Y_1  = *pYCbCr32_1++;
        pYCbCr32_1++; // skip alpha value

        // remove offset as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
        Y_1  -=  16;
        Cb_1 -= 128;
        Cr_1 -= 128;

        // get YCbCr32_2 pixel data
        int32_t Cr_2 = *pYCbCr32_2++;
        int32_t Cb_2 = *pYCbCr32_2++;
        int32_t Y_2  = *pYCbCr32_2++;
        pYCbCr32_2++; // skip alpha value

        // remove offset as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
        Y_2  -=  16;
        Cb_2 -= 128;
        Cr_2 -= 128;

        //int32_t R_1 = ( ((int32_t)(1.164*1024))*Y_1 + ((int32_t)(1.596*1024))*Cr_1                                ) >> 10;
        int32_t G_1 = ( ((int32_t)(1.164*1024))*Y_1 - ((int32_t)(0.813*1024))*Cr_1 - ((int32_t)(0.391*1024))*Cb_1 ) >> 10;
        int32_t B_1 = ( ((int32_t)(1.164*1024))*Y_1 + ((int32_t)(2.018*1024))*Cb_1                                ) >> 10;

        //int32_t R_2 = ( ((int32_t)(1.164*1024))*Y_2 + ((int32_t)(1.596*1024))*Cr_2                                ) >> 10;
        int32_t G_2 = ( ((int32_t)(1.164*1024))*Y_2 - ((int32_t)(0.813*1024))*Cr_2 - ((int32_t)(0.391*1024))*Cb_2 ) >> 10;
        int32_t B_2 = ( ((int32_t)(1.164*1024))*Y_2 + ((int32_t)(2.018*1024))*Cb_2                                ) >> 10;

        // clip
        //if (R_1<0) R_1=0; else if (R_1>255) R_1=255;
        if (G_1<0) G_1=0; else if (G_1>255) G_1=255;
        if (B_1<0) B_1=0; else if (B_1>255) B_1=255;

        // clip
        //if (R_1<0) R_1=0; else if (R_1>255) R_1=255;
        if (G_2<0) G_2=0; else if (G_2>255) G_2=255;
        if (B_2<0) B_2=0; else if (B_2>255) B_2=255;

        //  RGB -> ANAGLYPH
        //
        //  | R |      | 0.0 0.7 0.3 |   | R_a |    | 0.0 0.0 0.0 |   | R_b |
        //  | G |   =  | 0.0 0.0 0.0 | * | G_a |  + | 0.0 1.0 0.0 | * | G_b |
        //  | B |      | 0.0 0.0 0.0 |   | B_a |    | 0.0 0.0 1.0 |   | B_b |
        //
        int32_t R = ( ((int32_t)(0.7*1024)) * G_1 + ((int32_t)(0.3*1024)) * B_1 ) >> 10;
        int32_t G = ( ((int32_t)(1.0*1024)) * G_2 ) >> 10;
        int32_t B = ( ((int32_t)(1.0*1024)) * B_2 ) >> 10;

        // write back RGB32 data
        *pRGB32++ = (uint8_t) B;
        *pRGB32++ = (uint8_t) G;
        *pRGB32++ = (uint8_t) R;
        pRGB32++; // skip alpha value
    }
}


/******************************************************************************
 * ConvertYCbCr32ToRGB32()
 *****************************************************************************/
static void ConvertYCbCr32ToRGB32
(
    uint8_t     *pYCbCr32,
    uint32_t    PlaneSizePixel
)
{
    uint8_t *pRGB32 = pYCbCr32; // where to put the RGB data

    uint32_t pix;
    for (pix = 0; pix < PlaneSizePixel; pix++)
    {
        // get YCbCr32 pixel data
        int32_t Cr = *pYCbCr32++;
        int32_t Cb = *pYCbCr32++;
        int32_t Y  = *pYCbCr32++;
        pYCbCr32++; // skip alpha value

        // remove offset as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
        Y  -=  16;
        Cb -= 128;
        Cr -= 128;

        // convert to RGB
////#define USE_FLOAT
#if (1)
        // Standard Definition TV (BT.601) as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
    #ifdef USE_FLOAT
        float R = 1.164*Y + 1.596*Cr;
        float G = 1.164*Y - 0.813*Cr - 0.391*Cb;
        float B = 1.164*Y + 2.018*Cb;
    #else
        int32_t R = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(1.596*1024))*Cr                              ) >> 10;
        int32_t G = ( ((int32_t)(1.164*1024))*Y - ((int32_t)(0.813*1024))*Cr - ((int32_t)(0.391*1024))*Cb ) >> 10;
        int32_t B = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(2.018*1024))*Cb                              ) >> 10;
    #endif
#else
        // High Definition TV (BT.709) as in VideoDemystified 3; page 19; YCbCr to RGB(0..255)
    #ifdef USE_FLOAT
        float R = 1.164*Y + 1.793*Cr;
        float G = 1.164*Y - 0.534*Cr - 0.213*Cb;
        float B = 1.164*Y + 2.115*Cb;
    #else
        int32_t R = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(1.793*1024))*Cr                              ) >> 10;
        int32_t G = ( ((int32_t)(1.164*1024))*Y - ((int32_t)(0.534*1024))*Cr - ((int32_t)(0.213*1024))*Cb ) >> 10;
        int32_t B = ( ((int32_t)(1.164*1024))*Y + ((int32_t)(2.115*1024))*Cb                              ) >> 10;
    #endif
#endif
        // clip
        if (R<0) R=0; else if (R>255) R=255;
        if (G<0) G=0; else if (G>255) G=255;
        if (B<0) B=0; else if (B>255) B=255;

        // write back RGB32 data
        *pRGB32++ = (uint8_t) B;
        *pRGB32++ = (uint8_t) G;
        *pRGB32++ = (uint8_t) R;
        pRGB32++; // skip alpha value
    }
}

