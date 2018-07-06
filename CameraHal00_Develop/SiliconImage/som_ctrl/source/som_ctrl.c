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
 * @som_ctrl.c
 *
 * @brief
 *   Implementation of som ctrl.
 *
 *****************************************************************************/
/**
 * @page som_ctrl_page SOM Ctrl
 * The Snapshot Output Module captures image buffers handed in on disk.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref som_ctrl_api
 * - @ref som_ctrl_common
 * - @ref som_ctrl
 *
 */

#include <time.h>

#include <ebase/trace.h>

#include <common/return_codes.h>
#include <common/align.h>

#include <oslayer/oslayer.h>

#include <bufferpool/media_buffer.h>

#include <version/version.h>
//#include <libexif/exif-data.h>

#include "som_ctrl.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(SOM_CTRL_INFO , "SOM-CTRL: ", INFO,  0);
CREATE_TRACER(SOM_CTRL_ERROR, "SOM-CTRL: ", ERROR, 1);

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
 * somCtrlSwapUInt16()
 *****************************************************************************/
static inline uint16_t somCtrlSwapUInt16
(
        uint16_t value
)
{
  return ( ( value << 8 & 0xFF00 )
         | ( value >> 8 & 0x00FF ) );
}

/******************************************************************************
 * somCtrlGetState()
 *****************************************************************************/
static inline somCtrlState_t somCtrlGetState
(
    somCtrlContext_t    *pSomContext
);

/******************************************************************************
 * somCtrlSetState()
 *****************************************************************************/
static inline void somCtrlSetState
(
    somCtrlContext_t    *pSomContext,
    somCtrlState_t      newState
);

/******************************************************************************
 * somCtrlCompleteCommand()
 *****************************************************************************/
static void somCtrlCompleteCommand
(
    somCtrlContext_t        *pSomContext,
    somCtrlCmdID_t          CmdID,
    RESULT                  result,
    somCtrlCmdParams_t      *pParams,
    somCtrlCompletionInfo_t *pInfo
);

/******************************************************************************
 * somCtrlThreadHandler()
 *****************************************************************************/
static int32_t somCtrlThreadHandler
(
    void *p_arg
);

/******************************************************************************
 * somCtrlStoreBuffer()
 *****************************************************************************/
static RESULT somCtrlStoreBuffer
(
    somCtrlContext_t    *pSomContext,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * somCtrlStoreBufferStop()
 *****************************************************************************/
static RESULT somCtrlStoreBufferStop
(
    somCtrlContext_t    *pSomContext
);

/******************************************************************************
 * somCtrlStoreBufferRAW()
 *****************************************************************************/
static RESULT somCtrlStoreBufferRAW
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader,
    bool_t              is16bit
);

/******************************************************************************
 * somCtrlFlushAverageBufferRAW()
 *****************************************************************************/
static RESULT somCtrlFlushAverageBufferRAW
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    bool_t              putHeader,
    bool_t              is16bit
);

/******************************************************************************
 * somGenExifHeader()
 *****************************************************************************/

static RESULT somGenExifHeader
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer
);

/******************************************************************************
 * somCtrlStoreBufferJpeg()
 *****************************************************************************/
static RESULT somCtrlStoreBufferJpeg
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader
);

/******************************************************************************
 * somCtrlStoreBufferYUV444Planar()
 *****************************************************************************/
static RESULT somCtrlStoreBufferYUV444Planar
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader
);

/******************************************************************************
 * somCtrlStoreBufferYUV422Semi()
 *****************************************************************************/
static RESULT somCtrlStoreBufferYUV422Semi
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader
);

/******************************************************************************
 * ConvertYCbCr444combToRGBcomb()
 *****************************************************************************/
static void ConvertYCbCr444combToRGBcomb
(
    uint8_t     *pYCbCr444,
    uint32_t    PlaneSizePixel
);

/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * somCtrlCreate()
 *****************************************************************************/
RESULT somCtrlCreate
(
    somCtrlContext_t    *pSomContext
)
{
    RESULT result;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pSomContext != NULL );

    // finalize init of context
    pSomContext->acBaseFileName[0] = '\0';
    pSomContext->NumOfFrames = 0;
    pSomContext->ForceRGBOut = false;
    pSomContext->ExtendName = false;
    pSomContext->FramesCaptured = pSomContext->FramesLost = 0;
    pSomContext->pAveragedData = NULL;
    pSomContext->pFile  = NULL;
    pSomContext->pFileRight  = NULL;

    // add HAL reference
    result = HalAddRef( pSomContext->HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( SOM_CTRL_ERROR, "%s (adding HAL reference failed)\n", __FUNCTION__ );
        return result;
    }

    // create command queue
    if ( OSLAYER_OK != osQueueInit( &pSomContext->CommandQueue, pSomContext->MaxCommands, sizeof(somCtrlCmd_t) ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pSomContext->MaxCommands);
        HalDelRef( pSomContext->HalHandle );
        return ( RET_FAILURE );
    }

    // create full buffer queue
    if ( OSLAYER_OK != osQueueInit( &pSomContext->FullBufQueue, pSomContext->MaxBuffers, sizeof(MediaBuffer_t *) ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (creating command queue (depth: %d) failed)\n", __FUNCTION__, pSomContext->MaxBuffers);
        osQueueDestroy( &pSomContext->CommandQueue );
        HalDelRef( pSomContext->HalHandle );
        return ( RET_FAILURE );
    }

    // create handler thread
    if ( OSLAYER_OK != osThreadCreate( &pSomContext->Thread, somCtrlThreadHandler, pSomContext ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (creating handler thread failed)\n", __FUNCTION__);
        osQueueDestroy( &pSomContext->FullBufQueue );
        osQueueDestroy( &pSomContext->CommandQueue );
        HalDelRef( pSomContext->HalHandle );
        return ( RET_FAILURE );
    }

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * somCtrlDestroy()
 *****************************************************************************/
RESULT somCtrlDestroy
(
    somCtrlContext_t *pSomContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pSomContext != NULL );

    // send handler thread a shutdown command
    // ...prepare command
    somCtrlCmd_t Command;
    memset( &Command, 0, sizeof(Command) );
    Command.CmdID = SOM_CTRL_CMD_SHUTDOWN;
    // ....send command
    lres = somCtrlSendCommand( pSomContext, &Command );
    if (lres != RET_SUCCESS)
    {
        TRACE(SOM_CTRL_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        UPDATE_RESULT( result, lres);
    }

    // wait for handler thread to have stopped due to the shutdown command given above
    if ( OSLAYER_OK != osThreadWait( &pSomContext->Thread ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (waiting for handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy handler thread
    if ( OSLAYER_OK != osThreadClose( &pSomContext->Thread ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (closing handler thread failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // cancel any further commands waiting in command queue
    do
    {
        // get next command from queue
        somCtrlCmd_t Command;
        osStatus = osQueueTryRead( &pSomContext->CommandQueue, &Command );

        switch (osStatus)
        {
            case OSLAYER_OK:        // got a command, so cancel it
                somCtrlCompleteCommand( pSomContext, Command.CmdID, RET_CANCELED, Command.pParams, NULL );
                break;
            case OSLAYER_TIMEOUT:   // queue is empty
                break;
            default:                // something is broken...
                UPDATE_RESULT( result, RET_FAILURE);
                break;
        }
    } while (osStatus == OSLAYER_OK);

    // destroy full buffer queue
    if ( OSLAYER_OK != osQueueDestroy( &pSomContext->FullBufQueue ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (destroying full buffer queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // destroy command queue
    if ( OSLAYER_OK != osQueueDestroy( &pSomContext->CommandQueue ) )
    {
        TRACE(SOM_CTRL_ERROR, "%s (destroying command queue failed)\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE);
    }

    // remove HAL reference
    lres = HalDelRef( pSomContext->HalHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( SOM_CTRL_ERROR, "%s (removing HAL reference failed)\n", __FUNCTION__ );
        UPDATE_RESULT( result, RET_FAILURE);
    }

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * somCtrlSendCommand()
 *****************************************************************************/
RESULT somCtrlSendCommand
(
    somCtrlContext_t    *pSomContext,
    somCtrlCmd_t        *pCommand
)
{
    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if( (pSomContext == NULL) || (pCommand == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // are we shutting down?
    if ( somCtrlGetState( pSomContext ) == eSomCtrlStateInvalid )
    {
        return RET_CANCELED;
    }

    // send command
    OSLAYER_STATUS osStatus = osQueueWrite( &pSomContext->CommandQueue, pCommand);
    if (osStatus != OSLAYER_OK)
    {
        TRACE(SOM_CTRL_ERROR, "%s (sending command to queue failed -> OSLAYER_STATUS=%d)\n", __FUNCTION__, somCtrlGetState( pSomContext ), osStatus);
    }

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( (osStatus == OSLAYER_OK) ? RET_SUCCESS : RET_FAILURE);
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * somCtrlGetState()
 *****************************************************************************/
static inline somCtrlState_t somCtrlGetState
(
    somCtrlContext_t    *pSomContext
)
{
    DCT_ASSERT( pSomContext != NULL );
    return ( pSomContext->State );
}


/******************************************************************************
 * somCtrlSetState()
 *****************************************************************************/
static inline void somCtrlSetState
(
    somCtrlContext_t    *pSomContext,
    somCtrlState_t      newState
)
{
    DCT_ASSERT( pSomContext != NULL );
    pSomContext->State = newState;
}


/******************************************************************************
 * somCtrlCompleteCommand()
 *****************************************************************************/
static void somCtrlCompleteCommand
(
    somCtrlContext_t        *pSomContext,
    somCtrlCmdID_t          CmdID,
    RESULT                  result,
    somCtrlCmdParams_t      *pParams,
    somCtrlCompletionInfo_t *pInfo
)
{
    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pSomContext != NULL );

    // do callback
    pSomContext->somCbCompletion( CmdID, result, pParams, pInfo, pSomContext->pUserContext );

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);
}


/******************************************************************************
 * somCtrlThreadHandler()
 *****************************************************************************/
static int32_t somCtrlThreadHandler
(
    void *p_arg
)
{
    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg == NULL )
    {
        TRACE(SOM_CTRL_ERROR, "%s (arg pointer is NULL)\n", __FUNCTION__);
    }
    else
    {
        somCtrlContext_t *pSomContext = (somCtrlContext_t *)p_arg;

        bool_t bExit = BOOL_FALSE;

        // processing loop
        do
        {
            // set default result
            RESULT result = RET_WRONG_STATE;

            // wait for next command
            somCtrlCmd_t Command;
            OSLAYER_STATUS osStatus = osQueueRead(&pSomContext->CommandQueue, &Command);
            if (OSLAYER_OK != osStatus)
            {
                TRACE(SOM_CTRL_ERROR, "%s (receiving command failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                continue; // for now we simply try again
            }

            // process command
            switch ( Command.CmdID )
            {
                case SOM_CTRL_CMD_START:
                {
                    TRACE(SOM_CTRL_INFO, "%s (begin SOM_CTRL_CMD_START)\n", __FUNCTION__);

                    switch ( somCtrlGetState( pSomContext ) )
                    {
                        case eSomCtrlStateIdle:
                        {
                            pSomContext->pStartParams = Command.pParams;
                            if (Command.pParams->Start.szBaseFileName && (strlen(Command.pParams->Start.szBaseFileName) != 0) )
                            {
                                snprintf(pSomContext->acBaseFileName, sizeof(pSomContext->acBaseFileName), "%s", Command.pParams->Start.szBaseFileName);
                            }
                            else
                            {
                                snprintf(pSomContext->acBaseFileName, sizeof(pSomContext->acBaseFileName), "CamerIC");
                            }
                            pSomContext->NumOfFrames      = Command.pParams->Start.NumOfFrames;
                            pSomContext->NumSkipFrames    = Command.pParams->Start.NumSkipFrames;
                            pSomContext->AverageFrames    = Command.pParams->Start.AverageFrames;
                            pSomContext->ForceRGBOut      = Command.pParams->Start.ForceRGBOut;
                            pSomContext->ExtendName       = Command.pParams->Start.ExtendName;
                            pSomContext->ExifHeader       = Command.pParams->Start.Exif;
                            pSomContext->FramesCaptured   = pSomContext->FramesLost = 0;
                            pSomContext->CurrentPicType   = PIC_BUF_TYPE_INVALID;
                            pSomContext->CurrentPicLayout = PIC_BUF_LAYOUT_INVALID;
                            pSomContext->CurrentPicWidth  = pSomContext->CurrentPicHeight = 0;
                            somCtrlSetState( pSomContext, eSomCtrlStateRunning );

                            // pending start command
                            somCtrlCompletionInfo_t Info;
                            memset( &Info, 0, sizeof(Info) );
                            somCtrlCompleteCommand( pSomContext, SOM_CTRL_CMD_START, RET_PENDING, pSomContext->pStartParams, &Info );
                            result = RET_PENDING;
                            break;
                        }
                        default:
                        {
                            TRACE(SOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, somCtrlGetState( pSomContext ));
                        }
                    }

                    TRACE(SOM_CTRL_INFO, "%s (end SOM_CTRL_CMD_START)\n", __FUNCTION__);

                    break;
                }

                case SOM_CTRL_CMD_STOP:
                {
                    TRACE(SOM_CTRL_INFO, "%s (begin SOM_CTRL_CMD_STOP)\n", __FUNCTION__);

                    switch ( somCtrlGetState( pSomContext ) )
                    {
                        case eSomCtrlStateRunning:
                        {
                            // finalize capture
                            somCtrlStoreBufferStop( pSomContext );

                            // prepare completion info & complete pending start command
                            somCtrlCompletionInfo_t Info;
                            memset( &Info, 0, sizeof(Info) );
                            Info.Start.FramesCaptured = pSomContext->FramesCaptured;
                            Info.Start.FramesLost     = pSomContext->FramesLost;
                            somCtrlCompleteCommand( pSomContext, SOM_CTRL_CMD_START, RET_CANCELED, pSomContext->pStartParams, &Info );

                            // cleanup
                            pSomContext->pStartParams = NULL;
                            pSomContext->acBaseFileName[0] = '\0';
                            pSomContext->NumOfFrames = 0;
                            pSomContext->NumSkipFrames = 0;
                            pSomContext->FramesCaptured = 0;
                            pSomContext->AverageFrames = false;
                            pSomContext->ForceRGBOut = false;
                            pSomContext->ExtendName = false;

                            // back to idle state
                            somCtrlSetState( pSomContext, eSomCtrlStateIdle );

                            result = RET_SUCCESS;
                            break;
                        }
                        case eSomCtrlStateIdle:
                        {
                            // we allow this state as well, as state running may be left automatically
                            result = RET_SUCCESS;
                            break;
                        }
                        default:
                        {
                            TRACE(SOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, somCtrlGetState( pSomContext ));
                        }
                    }

                    TRACE(SOM_CTRL_INFO, "%s (end SOM_CTRL_CMD_STOP)\n", __FUNCTION__);

                    break;
                }

                case SOM_CTRL_CMD_SHUTDOWN:
                {
                    TRACE(SOM_CTRL_INFO, "%s (begin SOM_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    switch ( somCtrlGetState( pSomContext ) )
                    {
                        case eSomCtrlStateIdle:
                        {
                            somCtrlSetState( pSomContext, eSomCtrlStateInvalid ); // stop further commands from being send to command queue
                            bExit = BOOL_TRUE;
                            result = RET_PENDING; // avoid completion below
                            break;
                        }
                        default:
                        {
                            TRACE(SOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, somCtrlGetState( pSomContext ));
                        }
                    }

                    TRACE(SOM_CTRL_INFO, "%s (end SOM_CTRL_CMD_SHUTDOWN)\n", __FUNCTION__);

                    break;
                }

                case SOM_CTRL_CMD_PROCESS_BUFFER:
                {
                    TRACE(SOM_CTRL_INFO, "%s (begin SOM_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    MediaBuffer_t *pBuffer = NULL;

                    switch ( somCtrlGetState( pSomContext ) )
                    {
                        case eSomCtrlStateIdle:
                        {
                            osStatus = osQueueTryRead( &pSomContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(SOM_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                                break;
                            }

                            // just discard buffer
                            if ( pBuffer->pNext != NULL )
                            {
                                MediaBufUnlockBuffer( pBuffer->pNext );
                            }
                            MediaBufUnlockBuffer( pBuffer );

                            result = RET_SUCCESS;
                            break;
                        }

                        case eSomCtrlStateRunning:
                        {
                            osStatus = osQueueTryRead( &pSomContext->FullBufQueue, &pBuffer );
                            if ( ( osStatus != OSLAYER_OK ) || ( pBuffer == NULL ) )
                            {
                                TRACE(SOM_CTRL_ERROR, "%s (receiving full buffer failed -> OSLAYER_RESULT=%d)\n", __FUNCTION__, osStatus);
                                break;
                            }

                            if ( pBuffer )
                            {
                                if ( 0 == pSomContext->NumSkipFrames )
                                {
                                    // write buffer to disk
                                    result = somCtrlStoreBuffer( pSomContext, pBuffer );

                                    // stop if no more buffers to capture or last buffer or capture error
                                    if ( (++pSomContext->FramesCaptured == pSomContext->NumOfFrames) || (pBuffer->last) || (result != RET_SUCCESS) )
                                    {
                                        // finalize capture
                                        somCtrlStoreBufferStop( pSomContext );

                                        // prepare completion info & complete pending start command
                                        somCtrlCompletionInfo_t Info;
                                        memset( &Info, 0, sizeof(Info) );
                                        Info.Start.FramesCaptured = pSomContext->FramesCaptured;
                                        Info.Start.FramesLost     = pSomContext->FramesLost;
                                        somCtrlCompleteCommand( pSomContext, SOM_CTRL_CMD_START, 
                                                (result != RET_SUCCESS) ? RET_CANCELED : RET_SUCCESS, pSomContext->pStartParams, &Info );

                                        // cleanup
                                        pSomContext->pStartParams = NULL;
                                        pSomContext->acBaseFileName[0] = '\0';
                                        pSomContext->NumOfFrames = pSomContext->FramesCaptured = 0;
                                        pSomContext->AverageFrames = false;
                                        pSomContext->ForceRGBOut = false;
                                        pSomContext->ExtendName = false;

                                        // back to idle state
                                        somCtrlSetState( pSomContext, eSomCtrlStateIdle );
                                    }
                                }
                                else
                                {
                                    --pSomContext->NumSkipFrames;
                                }

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
                            TRACE(SOM_CTRL_ERROR, "%s (wrong state %d)\n", __FUNCTION__, somCtrlGetState( pSomContext ));
                        }
                    }

                    TRACE(SOM_CTRL_INFO, "%s (end SOM_CTRL_CMD_PROCESS_BUFFER)\n", __FUNCTION__);

                    break;
                }

                default:
                {
                    TRACE(SOM_CTRL_ERROR, "%s (illegal command %d)\n", __FUNCTION__, Command);
                    result = RET_NOTSUPP;
                }
            }

            // complete command?
            if (result != RET_PENDING)
            {
                somCtrlCompleteCommand( pSomContext, Command.CmdID, result, Command.pParams, NULL );
            }
        }
        while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}


/******************************************************************************
 * somCtrlStoreBuffer()
 *****************************************************************************/
static RESULT somCtrlStoreBuffer
(
    somCtrlContext_t    *pSomContext,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;

    char szFileName[FILENAME_MAX] = "";
    char szFileNameRight[FILENAME_MAX] = "";
    uint32_t width  = 0;
    uint32_t height = 0;
    bool_t   isLeftRight = false;
    bool_t   isNewFile = false;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pSomContext == NULL) || (pBuffer == NULL) )
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

    // get dimensions for filename creation
    switch ( pPicBufMetaData->Type )
    {
        case PIC_BUF_TYPE_DATA:
            break;
        case PIC_BUF_TYPE_RAW8:
            width  = pPicBufMetaData->Data.raw.PicWidthPixel;
            height = pPicBufMetaData->Data.raw.PicHeightPixel;
            break;
        case PIC_BUF_TYPE_RAW16:
            width  = pPicBufMetaData->Data.raw.PicWidthPixel;
            height = pPicBufMetaData->Data.raw.PicHeightPixel;
            break;
        case PIC_BUF_TYPE_JPEG:
            width  = pPicBufMetaData->Data.jpeg.PicWidthPixel;
            height = pPicBufMetaData->Data.jpeg.PicHeightPixel;
            isLeftRight = true;
            break;
        case PIC_BUF_TYPE_YCbCr444:
            switch ( pPicBufMetaData->Layout )
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    width  = pPicBufMetaData->Data.YCbCr.combined.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.combined.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_PLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel;
                    break;
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            isLeftRight = true;
            break;
        case PIC_BUF_TYPE_YCbCr422:
            switch ( pPicBufMetaData->Layout )
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    width  = pPicBufMetaData->Data.YCbCr.combined.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.combined.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_PLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel;
                    break;
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            isLeftRight = true;
            break;
        case PIC_BUF_TYPE_YCbCr420:
            switch ( pPicBufMetaData->Layout )
            {
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_PLANAR:
                    width  = pPicBufMetaData->Data.YCbCr.planar.Y.PicWidthPixel;
                    height = pPicBufMetaData->Data.YCbCr.planar.Y.PicHeightPixel;
                    break;
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            isLeftRight = true;
            break;
        case PIC_BUF_TYPE_RGB888:
            switch ( pPicBufMetaData->Layout )
            {
                case PIC_BUF_LAYOUT_COMBINED:
                    width  = pPicBufMetaData->Data.RGB.combined.PicWidthPixel;
                    height = pPicBufMetaData->Data.RGB.combined.PicHeightPixel;
                    break;
                case PIC_BUF_LAYOUT_PLANAR:
                    width  = pPicBufMetaData->Data.RGB.planar.R.PicWidthPixel;
                    height = pPicBufMetaData->Data.RGB.planar.R.PicHeightPixel;
                    break;
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            isLeftRight = true;
            break;
        default:
            UPDATE_RESULT( result, RET_NOTSUPP );
    }
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // is 3D, left/right image
    isLeftRight &= (NULL != pBuffer->pNext);

    // check format, layout & size haven't changed for contiguous capture
    if (pSomContext->pFile != NULL)
    {
        bool_t hasChanged = false;

        if ( (pPicBufMetaData->Type   == pSomContext->CurrentPicType)
          && (pPicBufMetaData->Layout == pSomContext->CurrentPicLayout) )
        {
            hasChanged = (width != pSomContext->CurrentPicWidth) || (height != pSomContext->CurrentPicHeight);
        }
        else
        {
            hasChanged = true;
        }

        // close the file? (we'll open a new one with a more suitable name later on)
        if ( (hasChanged)                                                       // if some picture params have changed
          || ((pSomContext->NumOfFrames != 0) && !pSomContext->AverageFrames) ) // or if not capturing contiguously and not averaging
        {
            // cleanup
            somCtrlStoreBufferStop( pSomContext );
        }
    }

    // open new file?
    if (pSomContext->pFile == NULL)
    {
        isNewFile = true;

        // build filename first:
        // ...get suitable file extension (.raw, .jpg, .yuv, .rgb)
        // ...get data type & layout string
        char *szFileExt = "";
        bool_t isData = false;
        bool_t isJpeg = false;
        char *szTypeLayout = ""; // not used for DATA or JPEG
        switch ( pPicBufMetaData->Type )
        {
            case PIC_BUF_TYPE_DATA:
                szFileExt = ".dat";
                isData    = true;
                break;
            case PIC_BUF_TYPE_RAW8:
                szFileExt    = ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                        szTypeLayout = "_raw8_RGGB";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                        szTypeLayout = "_raw8_GRBG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                        szTypeLayout = "_raw8_GBRG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                        szTypeLayout = "_raw8_BGGR";
                        break;
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = "_raw8";
                        break;
                    default:
                        UPDATE_RESULT( result, RET_NOTSUPP );
                }
                break;
            case PIC_BUF_TYPE_RAW16:
                szFileExt    = ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                        szTypeLayout = "_raw16_RGGB";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                        szTypeLayout = "_raw16_GRBG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                        szTypeLayout = "_raw16_GBRG";
                        break;
                    case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                        szTypeLayout = "_raw16_BGGR";
                        break;
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = "_raw16";
                        break;
                    default:
                        UPDATE_RESULT( result, RET_NOTSUPP );
                }
                break;
            case PIC_BUF_TYPE_JPEG:
                szFileExt = ".jpg";
                isJpeg    = true;
                break;
            case PIC_BUF_TYPE_YCbCr444:
                szFileExt = pSomContext->ForceRGBOut ? ".ppm" : ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv444_comb";
                        break;
                    case PIC_BUF_LAYOUT_PLANAR:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv444_plan";
                        break;
                    default:
                        UPDATE_RESULT( result, RET_NOTSUPP );
                }
                break;
            case PIC_BUF_TYPE_YCbCr422:
                szFileExt = pSomContext->ForceRGBOut ? ".ppm" : ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv422_comb";
                        break;
                    case PIC_BUF_LAYOUT_SEMIPLANAR:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv422_semi";
                        break;
                    case PIC_BUF_LAYOUT_PLANAR:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv422_plan";
                        break;
                    default:
                        UPDATE_RESULT( result, RET_NOTSUPP );
                }
                break;
            case PIC_BUF_TYPE_YCbCr420:
                szFileExt = pSomContext->ForceRGBOut ? ".ppm" : ".pgm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_SEMIPLANAR:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv420_semi";
                        break;
                    case PIC_BUF_LAYOUT_PLANAR:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_yuv420_plan";
                        break;
                    default:
                        UPDATE_RESULT( result, RET_NOTSUPP );
                }
                break;
            case PIC_BUF_TYPE_RGB888:
                szTypeLayout = ".ppm";
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_COMBINED:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_rgb888_comb";
                        break;
                    case PIC_BUF_LAYOUT_PLANAR:
                        szTypeLayout = pSomContext->ForceRGBOut ? "" : "_rgb888_plan";
                        break;
                    default:
                        UPDATE_RESULT( result, RET_NOTSUPP );
                }
                break;
            default:
                UPDATE_RESULT( result, RET_NOTSUPP );
        }
        if (result != RET_SUCCESS)
        {
            return result;
        }

        // ...create image dimension string
        char szDimensions[20] = "";
        if (!isData && !isJpeg) // but neither for DATA nor for JPEG
        {
            snprintf( szDimensions, sizeof(szDimensions)-1, "_%dx%d", width, height );
        }

        // ...create date/time string
        char szDateTime[20] = "";
        if (!isJpeg) // but not for JPEG
        {
            // remember creation time if first file
            if (pSomContext->FramesCaptured + pSomContext->FramesLost == 0)
            {
                time_t t;
                t = time( NULL );
                pSomContext->FileCreationTime = *localtime(&t);
            }

            // always use creation time of first file for file name in a sequence of files
            strftime( szDateTime, sizeof(szDateTime), "_%Y%m%d_%H%M%S", &pSomContext->FileCreationTime );
        }

        // ...create sequence number string
        char szNumber[20] = "";
        if (pSomContext->NumOfFrames > 1)
        {
            // averaged images are a little special...
            if ( pSomContext->AverageFrames )
            {
                snprintf( szNumber, sizeof(szNumber)-1, "_%04davg", pSomContext->NumOfFrames );
            }
            else
            {
                snprintf( szNumber, sizeof(szNumber)-1, "_%04d", pSomContext->FramesCaptured );
            }
        }
        else if (pSomContext->NumOfFrames == 0)
        {
            snprintf( szNumber, sizeof(szNumber)-1, "_multi" );
        }

        // ...combine all parts
        uint32_t combLen = strlen(pSomContext->acBaseFileName) + strlen(szDimensions) + strlen(szTypeLayout) + strlen(szDateTime) + strlen(szNumber) + strlen(szFileExt);
        if ( combLen >= FILENAME_MAX)
        {
            TRACE(SOM_CTRL_ERROR, "%s Max filename length exceeded.\n"
                                  " len(BaseFileName) = %3d\n"
                                  " len(Dimensions)   = %3d\n"
                                  " len(TypeLayout)   = %3d\n"
                                  " len(DateTime)     = %3d\n"
                                  " len(Number)       = %3d\n"
                                  " len(FileExt)      = %3d\n"
                                  " --------------------------\n"
                                  " max = %3d        >= %3d\n",
                                  __FUNCTION__,
                                  strlen(pSomContext->acBaseFileName), strlen(szDimensions), strlen(szTypeLayout),
                                  strlen(szDateTime), strlen(szNumber), strlen(szFileExt), FILENAME_MAX, combLen);
            return RET_INVALID_PARM;
        }

        if ( !isLeftRight)
        {
            if (pSomContext->ExtendName)
            {
                snprintf( szFileName, FILENAME_MAX, "%s%s%s%s%s%s", pSomContext->acBaseFileName, szDimensions, szTypeLayout, szDateTime, szNumber, szFileExt );
            }
            else if (pSomContext->NumOfFrames > 1) // images in series always need to be numbered
            {
                snprintf( szFileName, FILENAME_MAX, "%s%s%s", pSomContext->acBaseFileName, szNumber, szFileExt );
            }
            else
            {
                snprintf( szFileName, FILENAME_MAX, "%s%s", pSomContext->acBaseFileName, szFileExt );
            }
            szFileName[FILENAME_MAX-1] = '\0';

            // then open file
            pSomContext->pFile = fopen( szFileName, "wb" );
            if (pSomContext->pFile == NULL)
            {
                TRACE(SOM_CTRL_ERROR, "%s Couldn't open file '%s'.\n", __FUNCTION__, szFileName);
                return RET_FAILURE;
            }
        }
        else
        {
            if (pSomContext->ExtendName)
            {
                snprintf( szFileName, FILENAME_MAX, "%s%s%s%s%s%s%s", pSomContext->acBaseFileName, szDimensions, szTypeLayout, szDateTime, szNumber, "_left" , szFileExt );
                snprintf( szFileNameRight, FILENAME_MAX, "%s%s%s%s%s%s%s", pSomContext->acBaseFileName, szDimensions, szTypeLayout, szDateTime, szNumber, "_right" , szFileExt );
            }
            else if (pSomContext->NumOfFrames > 1) // images in series always need to be numbered
            {
                snprintf( szFileName, FILENAME_MAX, "%s%s%s%s", pSomContext->acBaseFileName, szNumber, "_left", szFileExt );
                snprintf( szFileNameRight, FILENAME_MAX, "%s%s%s%s", pSomContext->acBaseFileName, szNumber, "_right", szFileExt );
            }
            else
            {
                snprintf( szFileName, FILENAME_MAX, "%s%s%s", pSomContext->acBaseFileName, "_left", szFileExt );
                snprintf( szFileNameRight, FILENAME_MAX, "%s%s%s", pSomContext->acBaseFileName, "_right", szFileExt );
            }
            szFileName[FILENAME_MAX-1] = '\0';
            szFileNameRight[FILENAME_MAX-1] = '\0';

            // then open file
            pSomContext->pFile = fopen( szFileName, "wb" );
            if (pSomContext->pFile == NULL)
            {
                TRACE(SOM_CTRL_ERROR, "%s Couldn't open file '%s'.\n", __FUNCTION__, szFileName);
                return RET_FAILURE;
            }

            pSomContext->pFileRight = fopen( szFileNameRight, "wb" );
            if (pSomContext->pFileRight == NULL)
            {
                fclose( pSomContext->pFile );
                TRACE(SOM_CTRL_ERROR, "%s Couldn't open file '%s'.\n", __FUNCTION__, szFileNameRight);
                return RET_FAILURE;
            }
        }

        // and remember new picture params
        pSomContext->CurrentPicType   = pPicBufMetaData->Type;
        pSomContext->CurrentPicLayout = pPicBufMetaData->Layout;
        pSomContext->CurrentPicWidth  = width;
        pSomContext->CurrentPicHeight = height;
    }

    // depending on data format, layout & size call subroutines to:
    // ...get data into local buffer(s)
    // ...write local buffer(s) to file while removing any trailing stuffing from lines where applicable
    // ...averaging pixel data is handled by @ref somCtrlStoreBufferRAW() function internally
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                case PIC_BUF_LAYOUT_COMBINED:
                {
                    RESULT lres = somCtrlStoreBufferRAW( pSomContext, pSomContext->pFile, pBuffer, isNewFile, (pPicBufMetaData->Type == PIC_BUF_TYPE_RAW16) );
                    UPDATE_RESULT( result, lres );
                    break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            break;
        case PIC_BUF_TYPE_JPEG:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_COMBINED:
                {
                    RESULT lres = somCtrlStoreBufferJpeg( pSomContext, pSomContext->pFile, pBuffer, isNewFile );
                    UPDATE_RESULT( result, lres );

                    if ( isLeftRight )
                    {
                        RESULT lres = somCtrlStoreBufferJpeg( pSomContext, pSomContext->pFileRight, pBuffer->pNext, isNewFile );
                        UPDATE_RESULT( result, lres );
                    }
                    break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            break;
        case PIC_BUF_TYPE_YCbCr444:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_PLANAR:
                {
                    RESULT lres = somCtrlStoreBufferYUV444Planar( pSomContext, pSomContext->pFile, pBuffer, isNewFile );
                    UPDATE_RESULT( result, lres );

                    if ( isLeftRight )
                    {
                        RESULT lres = somCtrlStoreBufferYUV444Planar( pSomContext, pSomContext->pFileRight, pBuffer->pNext, isNewFile );
                        UPDATE_RESULT( result, lres );
                    }
                    break;
                }
                case PIC_BUF_LAYOUT_COMBINED:
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    UPDATE_RESULT( result, RET_NOTSUPP );
                    break;
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
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
                    RESULT lres = somCtrlStoreBufferYUV422Semi( pSomContext, pSomContext->pFile, pBuffer, isNewFile );
                    UPDATE_RESULT( result, lres );

                    if ( isLeftRight )
                    {
                        RESULT lres = somCtrlStoreBufferYUV422Semi( pSomContext, pSomContext->pFileRight, pBuffer->pNext, isNewFile );
                        UPDATE_RESULT( result, lres );
                    }
                    break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            }
            break;
        case PIC_BUF_TYPE_DPCC:
            printf("DPCC ------------------------------------------------------ \n");
            break;
        default:
            UPDATE_RESULT( result, RET_NOTSUPP );
    }
    if (result != RET_SUCCESS)
    {
        return result;
    }

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * somCtrlStoreBufferStop()
 *****************************************************************************/
static RESULT somCtrlStoreBufferStop
(
    somCtrlContext_t    *pSomContext
)
{
    RESULT result = RET_SUCCESS;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }

    // cleanup
    if(pSomContext->pFile)
    {
        // write average data buffer(s) to file first?
        if ( pSomContext->AverageFrames && pSomContext->pAveragedData )
        {
            // call subroutines depending on data format, layout & size
            switch (pSomContext->CurrentPicType)
            {
                case PIC_BUF_TYPE_RAW8:
                case PIC_BUF_TYPE_RAW16:
                    switch (pSomContext->CurrentPicLayout)
                    {
                        case PIC_BUF_LAYOUT_BAYER_RGRGGBGB:
                        case PIC_BUF_LAYOUT_BAYER_GRGRBGBG:
                        case PIC_BUF_LAYOUT_BAYER_GBGBRGRG:
                        case PIC_BUF_LAYOUT_BAYER_BGBGGRGR:
                        case PIC_BUF_LAYOUT_COMBINED:
                        {
                            RESULT lres = somCtrlFlushAverageBufferRAW( pSomContext, pSomContext->pFile, true, (pSomContext->CurrentPicType == PIC_BUF_TYPE_RAW16) );
                            UPDATE_RESULT( result, lres );
                            break;
                        }
                        default:
                            UPDATE_RESULT( result, RET_NOTSUPP );
                    }
                    break;
                default:    // make lint happy
                    break;
            }
        }

        // close file
        fclose( pSomContext->pFile );
        pSomContext->pFile = NULL;

        if(pSomContext->pFileRight)
        {
            fclose( pSomContext->pFileRight );
            pSomContext->pFileRight = NULL;
        }
    }

    // release averaged data buffer?
    if ( pSomContext->pAveragedData )
    {
        free( pSomContext->pAveragedData );
        pSomContext->pAveragedData = NULL;
    }

    pSomContext->CurrentPicType   = PIC_BUF_TYPE_INVALID;
    pSomContext->CurrentPicLayout = PIC_BUF_LAYOUT_INVALID;
    pSomContext->CurrentPicWidth  = pSomContext->CurrentPicHeight = 0;

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * somCtrlStoreBufferRAW()
 *****************************************************************************/
static RESULT somCtrlStoreBufferRAW
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader,
    bool_t              is16bit
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }

    if (!pFile)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation assumes that on-board memory is used for buffers!

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // need to allocate memory for summing up frames?
    if ( pSomContext->AverageFrames && (pSomContext->pAveragedData == NULL) )
    {
        uint32_t bufferSize = pPicBufMetaData->Data.raw.PicWidthPixel * pPicBufMetaData->Data.raw.PicHeightPixel * sizeof(uint32_t);
        pSomContext->pAveragedData = malloc( bufferSize );
        if ( pSomContext->pAveragedData == NULL )
        {
            free( pLocBuf );
            return RET_OUTOFMEM;
        }
    }

    // get base address & size of local plane
    uint32_t RawPlaneSize = pPicBufMetaData->Data.raw.PicWidthBytes * pPicBufMetaData->Data.raw.PicHeightPixel;
    uint8_t *pRawTmp, *pRawBase;
    pRawTmp = pRawBase = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf)), pPicBufMetaData->Align);

    // get raw plane from on-board memory
    lres = HalReadMemory( pSomContext->HalHandle, (ulong_t)(pPicBufMetaData->Data.raw.pBuffer), pRawBase, RawPlaneSize );
    UPDATE_RESULT( result, lres );

    if ( !pSomContext->AverageFrames )
    {
        // write out raw image; no matter what pSomContext->ForceRGBOut requests

        // write pgm header
        fprintf( pFile,
                "%sP5\n%d %d\n#####<DCT Raw>\n#<Type>%u</Type>\n#<Layout>%u</Layout>\n#<TimeStampUs>%lli</TimeStampUs>\n#####</DCT Raw>\n%d\n",
                putHeader ? "" : "\n", pPicBufMetaData->Data.raw.PicWidthPixel, pPicBufMetaData->Data.raw.PicHeightPixel,
                        pPicBufMetaData->Type, pPicBufMetaData->Layout, pPicBufMetaData->TimeStampUs, is16bit ? 65535 : 255 );

        // write raw plane to file
        if ( (pPicBufMetaData->Data.raw.PicWidthPixel * (is16bit ? 2 : 1)) == pPicBufMetaData->Data.raw.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pRawBase, RawPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.raw.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pRawTmp, (pPicBufMetaData->Data.raw.PicWidthPixel * (is16bit ? 2 : 1)), 1, pFile ) )
                {
                    UPDATE_RESULT( result, RET_FAILURE );
                    break; // for loop
                }
                pRawTmp += pPicBufMetaData->Data.raw.PicWidthBytes;
            }
        }
    }
    else
    {
        // update average data

        if ( pSomContext->pAveragedData == NULL )
        {
            TRACE(SOM_CTRL_ERROR, "%s pSomContext->pAveragedData == NULL\n", __FUNCTION__);
            UPDATE_RESULT( result, RET_NULL_POINTER );
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t x,y;
            uint32_t *pRawAveragedTmp = (uint32_t*)(pSomContext->pAveragedData);
            for (y=0; y < pPicBufMetaData->Data.raw.PicHeightPixel; y++)
            {
                if ( is16bit )
                {
                    uint16_t *pRawLine = (uint16_t*)pRawTmp;
                    for (x=0; x < pPicBufMetaData->Data.raw.PicWidthPixel; x++)
                    {
                        *pRawAveragedTmp += somCtrlSwapUInt16( *pRawLine );
                        pRawAveragedTmp++;
                        pRawLine++;
                    }
                }
                else
                {
                    uint8_t *pRawLine = pRawTmp;
                    for (x=0; x < pPicBufMetaData->Data.raw.PicWidthPixel; x++)
                    {
                        *pRawAveragedTmp += *pRawLine;
                        pRawAveragedTmp++;
                        pRawLine++;
                    }
                }

                pRawTmp += pPicBufMetaData->Data.raw.PicWidthBytes;
            }
        }

    }

    // free local buffer
    free( pLocBuf );

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * somCtrlFlushAverageBufferRAW()
 *****************************************************************************/
static RESULT somCtrlFlushAverageBufferRAW
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    bool_t              putHeader,
    bool_t              is16bit
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }

    if (!pFile)
    {
        return RET_NULL_POINTER;
    }

    if ( pSomContext->pAveragedData == NULL )
    {
        TRACE(SOM_CTRL_ERROR, "%s pSomContext->pAveragedData == NULL\n", __FUNCTION__);
        return RET_NULL_POINTER;
    }

    // finalize average calculation
    // includes in place pixel data width reduction as well
    uint32_t i = 0;
    uint32_t RawAveragedPlaneSize = pSomContext->CurrentPicWidth * pSomContext->CurrentPicHeight;
    uint32_t *pAveragedDataTmp = pSomContext->pAveragedData;
    if ( is16bit )
    {
        uint16_t *pRawDataTmp = (uint16_t*)pAveragedDataTmp;
        for(i=0; i < RawAveragedPlaneSize; i++)
        {
            *pRawDataTmp++ = somCtrlSwapUInt16( *pAveragedDataTmp++ / pSomContext->FramesCaptured );
        }
    }
    else
    {
        uint8_t *pRawDataTmp = (uint8_t*)pAveragedDataTmp;
        for(i=0; i < RawAveragedPlaneSize; i++)
        {
            *pRawDataTmp++ = *pAveragedDataTmp++ / pSomContext->FramesCaptured;
        }
    }

    // write pgm header
    fprintf( pFile,
            "%sP5\n%d %d\n#####<DCT Raw>\n#<Type>%u</Type>\n#<Layout>%u</Layout>\n#<TimeStampUs>%lli</TimeStampUs>\n#####</DCT Raw>\n%d\n",
            putHeader ? "" : "\n", pSomContext->CurrentPicWidth, pSomContext->CurrentPicHeight,
                    pSomContext->CurrentPicType, pSomContext->CurrentPicLayout, -1ll, is16bit ? 65535 : 255 );


    // write raw plane to file
    // a single write will do
    if ( 1 != fwrite( pSomContext->pAveragedData, RawAveragedPlaneSize * (is16bit ? 2 : 1), 1, pFile ) )
    {
        UPDATE_RESULT( result, RET_FAILURE );
    }

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}

/******************************************************************************
 * init_tag()
 * Get an existing tag, or create one if it doesn't exist
 *****************************************************************************/
#if 0
static ExifEntry *init_tag
(
    ExifData    *exif,
    ExifIfd     ifd,
    ExifTag     tag
)
{
    ExifEntry *entry;
    
    // Return an existing tag if one exists
    if ( !((entry = exif_content_get_entry (exif->ifd[ifd], tag))) )
    {
        // allocate a new entry
        entry = exif_entry_new ();
        DCT_ASSERT( entry != NULL );
        
        // tag must be set before calling exif_content_add_entry
        entry->tag = tag;

        // attach the ExifEntry to an IFD
        exif_content_add_entry( exif->ifd[ifd], entry );

        // allocate memory for the entry and fill with default data
        exif_entry_initialize( entry, tag );

        // Ownership of the ExifEntry has now been passed to the IFD.
        // One must be very careful in accessing a structure after
        // unref'ing it; in this case, we know "entry" won't be freed
        // because the reference count was bumped when it was added to
        // the IFD.
        exif_entry_unref(entry);
    }
    
    return ( entry );
}
#endif
/******************************************************************************
 * create_tag()
 *****************************************************************************/
#if 0
static ExifEntry *create_tag
(
    ExifData    *exif,
    ExifIfd     ifd,
    ExifTag     tag,
    size_t      len,
    ExifFormat  format
)
{
    void *buf;
    ExifEntry *entry;
    
    // create a memory allocator to manage this ExifEntry
    ExifMem *mem = exif_mem_new_default();
    DCT_ASSERT( mem != NULL );

    // create a new ExifEntry using our allocator
    entry = exif_entry_new_mem (mem);
    DCT_ASSERT( entry != NULL );

    // allocate memory to use for holding the tag data
    buf = exif_mem_alloc(mem, len);
    DCT_ASSERT( buf != NULL );

    // fill in the entry
    entry->data = buf;
    entry->size = len;
    entry->tag = tag;
    entry->components = len;
    entry->format = format;

    // attach the ExifEntry to an IFD
    exif_content_add_entry (exif->ifd[ifd], entry);

    // the ExifMem and ExifEntry are now owned elsewhere
    exif_mem_unref(mem);
    exif_entry_unref(entry);

    return ( entry );
}
#endif

/******************************************************************************
 * somGenExifHeader()
 *****************************************************************************/
// special header required for EXIF_TAG_USER_COMMENT
#define ASCII_MODEL "camerIc\0"
#define ASCII_MAKE  "Dream Chip Technologies GmbH\0"

// raw EXIF header data
static const unsigned char exif_header[] =
{
  0xff, 0xd8, 0xff, 0xe1
};

// length of data in exif_header
static const unsigned int exif_header_len = sizeof(exif_header);

static RESULT somGenExifHeader
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);
    #if 0
    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }
    
    if (!pFile)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }

    unsigned char *exif_data;
    unsigned int exif_data_len;
    ExifEntry *entry;

    ExifData *exif = exif_data_new();
    if ( !exif )
    {
        TRACE(SOM_CTRL_ERROR, "%s Out of memory\n", __FUNCTION__);
        return RET_OUTOFMEM;
    }

    /* Set the image options */
    exif_data_set_option(exif, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
    exif_data_set_data_type(exif, EXIF_DATA_TYPE_COMPRESSED);
    exif_data_set_byte_order(exif, EXIF_BYTE_ORDER_INTEL);

    /* Create the mandatory EXIF fields with default data */
    exif_data_fix(exif);

    TRACE(SOM_CTRL_INFO, "%s Adding a Model reference\n", __FUNCTION__ );
    entry = create_tag(exif, EXIF_IFD_0, EXIF_TAG_MODEL, strlen(ASCII_MODEL), EXIF_FORMAT_ASCII );
    strncpy( (char *)entry->data, ASCII_MODEL, entry->size );

    TRACE(SOM_CTRL_INFO, "%s Adding a Make reference\n", __FUNCTION__ );
    entry = create_tag(exif, EXIF_IFD_0, EXIF_TAG_MAKE, strlen(ASCII_MAKE), EXIF_FORMAT_ASCII );
    strncpy( (char *)entry->data, ASCII_MAKE, entry->size );
    
    TRACE(SOM_CTRL_INFO, "%s Adding a Software reference\n", __FUNCTION__ );
    entry = create_tag(exif, EXIF_IFD_0, EXIF_TAG_SOFTWARE, strlen(GetVersion()), EXIF_FORMAT_ASCII );
    strncpy( (char *)entry->data, GetVersion(), entry->size );

    entry = init_tag(exif, EXIF_IFD_0, EXIF_TAG_IMAGE_WIDTH);
    exif_set_long(entry->data, EXIF_BYTE_ORDER_INTEL, pPicBufMetaData->Data.jpeg.PicWidthPixel);
    
    entry = init_tag(exif, EXIF_IFD_0, EXIF_TAG_IMAGE_LENGTH);
    exif_set_long(entry->data, EXIF_BYTE_ORDER_INTEL, pPicBufMetaData->Data.jpeg.PicHeightPixel);

    entry = init_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_X_DIMENSION);
    exif_set_long(entry->data, EXIF_BYTE_ORDER_INTEL, pPicBufMetaData->Data.jpeg.PicWidthPixel);

    entry = init_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_PIXEL_Y_DIMENSION);
    exif_set_long(entry->data, EXIF_BYTE_ORDER_INTEL, pPicBufMetaData->Data.jpeg.PicHeightPixel);

    entry = init_tag(exif, EXIF_IFD_0, EXIF_TAG_COLOR_SPACE);
    exif_set_short(entry->data, EXIF_BYTE_ORDER_INTEL, 1);
    
    // get a pointer to the EXIF data block we just created
    exif_data_save_data(exif, &exif_data, &exif_data_len);
    DCT_ASSERT( exif_data != NULL );

    // write EXIF header to file
    if ( fwrite(exif_header, exif_header_len, 1, pFile) != 1 )
    {
        TRACE(SOM_CTRL_ERROR, "%s Error writing to file\n", __FUNCTION__);
        goto errout;
    }

    // write EXIF block length in big-endian order
    if ( fputc((exif_data_len+2) >> 8, pFile) < 0 )
    {
        TRACE(SOM_CTRL_ERROR, "%s Error writing to file\n", __FUNCTION__);
        goto errout;
    }
    if ( fputc((exif_data_len+2) & 0xff, pFile) < 0 )
    {
        TRACE(SOM_CTRL_ERROR, "%s Error writing to file\n", __FUNCTION__);
        goto errout;
    }

    // write EXIF data block
    if ( fwrite(exif_data, exif_data_len, 1, pFile) != 1 )
    {
        TRACE(SOM_CTRL_ERROR, "%s Error writing to file\n", __FUNCTION__);
        goto errout;
    }

errout:
    free(exif_data);
    exif_data_unref(exif);

    TRACE(SOM_CTRL_INFO, "%s (exit %d)\n", __FUNCTION__, result );
#endif
    return result;
}


/******************************************************************************
 * somCtrlStoreBufferJpeg()
 *****************************************************************************/
static RESULT somCtrlStoreBufferJpeg
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }

    if (!pFile)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }

    // allocate local buffer
    uint8_t *pLocBuf = malloc(MAX_ALIGNED_SIZE(pBuffer->baseSize, pPicBufMetaData->Align));
    if (pLocBuf == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get base address & size of local plane
    uint32_t DataSize = pPicBufMetaData->Data.jpeg.DataSize;
    uint32_t RawPlaneSize = ALIGN_UP( DataSize, 8 );
    uint8_t *pRawTmp, *pRawBase;
    pRawTmp = pRawBase = (uint8_t *) ALIGN_UP( ((ulong_t)(pLocBuf)), pPicBufMetaData->Align);

    // get raw plane from on-board memory
    lres = HalReadMemory( pSomContext->HalHandle, (ulong_t)( pPicBufMetaData->Data.jpeg.pData), pRawBase, RawPlaneSize );
    UPDATE_RESULT( result, lres );

    if ( putHeader && pSomContext->ExifHeader )
    {
        /* start of JPEG image data section */
        static const unsigned int image_data_offset = 20;

        lres = somGenExifHeader( pSomContext, pFile, pBuffer );
        UPDATE_RESULT( result, lres );

        // skip JFIF header here, replaced by exif header
        if ( 1 != fwrite( pRawBase + image_data_offset, (DataSize - image_data_offset), 1, pFile ) )
        {
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }
    else
    {
        // a single write will do
        if ( 1 != fwrite( pRawBase, DataSize, 1, pFile ) )
        {
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }

    // free local buffer
    free( pLocBuf );

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * somCtrlStoreBufferYUV444Planar()
 *****************************************************************************/
static RESULT somCtrlStoreBufferYUV444Planar
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }

    if (!pFile)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation assumes that on-board memory is used for buffers!

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
    lres = HalReadMemory( pSomContext->HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase,    YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pSomContext->HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase, YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // write out raw or RGB image?
    if (!pSomContext->ForceRGBOut)
    {
        // write pgm header
        fprintf( pFile, "%sP5\n%d %d\n255\n", putHeader ? "" : "\n", pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, 2 * pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel );

        // write luma plane to file
        if ( pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel == pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pYBase, YCPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pYTmp, pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, 1, pFile ) )
                {
                    UPDATE_RESULT( result, RET_FAILURE );
                    break; // for loop
                }
                pYTmp += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            }
        }

        // write combined chroma plane to file
        if ( pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel == pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pCbCrBase, YCPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pCbCrTmp, pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel, 1, pFile ) )
                {
                    UPDATE_RESULT( result, RET_FAILURE );
                    break; // for loop
                }
                pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
            }
        }
    }
    else
    {
        // we need a temporary helper buffer capable of holding 3 times the YPlane size (upscaled to 4:4:4 by pixel replication)
        uint8_t *pYCbCr444 = malloc(3*YCPlaneSize);
        if (pYCbCr444 == NULL)
        {
            UPDATE_RESULT( result, RET_OUTOFMEM );
        }
        else
        {
            // upscale and combine each 4:2:2 pixel to 4:4:4 while removing any gaps at line ends as well
            uint8_t *pYCbCr444Tmp = pYCbCr444;
            uint32_t x,y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
            {
                // get line starts
                uint8_t *pY = pYTmp;
                uint8_t *pC = pCbCrTmp;

                // walk through line
                for (x=0; x < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
                {
                    uint8_t Cb, Cr;
                    *pYCbCr444Tmp++ = *pY++;
                    *pYCbCr444Tmp++ = Cb = *pC++;
                    *pYCbCr444Tmp++ = Cr = *pC++;
                    *pYCbCr444Tmp++ = *pY++;
                    *pYCbCr444Tmp++ = Cb;
                    *pYCbCr444Tmp++ = Cr;
                }

                // update line starts
                pYTmp    += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
                pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
            }

            // inplace convert consecutive YCbCr444 to RGB; both are combined color component planes
            ConvertYCbCr444combToRGBcomb( pYCbCr444, YCPlaneSize );

            // write ppm header
            fprintf( pFile, "%sP6\n%d %d\n255\n", putHeader ? "" : "", pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel );

            // finally write result
            if ( 1 != fwrite( pYCbCr444, 3 * YCPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }

            // release helper buffer
            free(pYCbCr444);
        }
    }

    // free local buffer
    free( pLocBuf );

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}



/******************************************************************************
 * somCtrlStoreBufferYUV422Semi()
 *****************************************************************************/
static RESULT somCtrlStoreBufferYUV422Semi
(
    somCtrlContext_t    *pSomContext,
    FILE                *pFile,
    MediaBuffer_t       *pBuffer,
    bool_t              putHeader
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(SOM_CTRL_INFO, "%s (enter)\n", __FUNCTION__);

    if (!pSomContext)
    {
        return RET_NULL_POINTER;
    }

    if (!pFile)
    {
        return RET_NULL_POINTER;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation assumes that on-board memory is used for buffers!

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
    lres = HalReadMemory( pSomContext->HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer),    pYBase,    YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // get combined chroma plane from on-board memory
    lres = HalReadMemory( pSomContext->HalHandle, (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer), pCbCrBase, YCPlaneSize );
    UPDATE_RESULT( result, lres );

    // write out raw or RGB image?
    if (!pSomContext->ForceRGBOut)
    {
        // write pgm header
        fprintf( pFile, "%sP5\n%d %d\n255\n", putHeader ? "" : "\n", pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, 2 * pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel );

        // write luma plane to file
        if ( pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel == pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pYBase, YCPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pYTmp, pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, 1, pFile ) )
                {
                    UPDATE_RESULT( result, RET_FAILURE );
                    break; // for loop
                }
                pYTmp += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
            }
        }

        // write combined chroma plane to file
        if ( pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel == pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes )
        {
            // a single write will do
            if ( 1 != fwrite( pCbCrBase, YCPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }
        }
        else
        {
            // remove trailing gaps from lines
            uint32_t y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicHeightPixel; y++)
            {
                if ( 1 != fwrite( pCbCrTmp, pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel, 1, pFile ) )
                {
                    UPDATE_RESULT( result, RET_FAILURE );
                    break; // for loop
                }
                pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
            }
        }
    }
    else
    {
        // we need a temporary helper buffer capable of holding 3 times the YPlane size (upscaled to 4:4:4 by pixel replication)
        uint8_t *pYCbCr444 = malloc(3*YCPlaneSize);
        if (pYCbCr444 == NULL)
        {
            UPDATE_RESULT( result, RET_OUTOFMEM );
        }
        else
        {
            // upscale and combine each 4:2:2 pixel to 4:4:4 while removing any gaps at line ends as well
            uint8_t *pYCbCr444Tmp = pYCbCr444;
            uint32_t x,y;
            for (y=0; y < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel; y++)
            {
                // get line starts
                uint8_t *pY = pYTmp;
                uint8_t *pC = pCbCrTmp;

                // walk through line
                for (x=0; x < pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel; x+=2)
                {
                    uint8_t Cb, Cr;
                    *pYCbCr444Tmp++ = *pY++;
                    *pYCbCr444Tmp++ = Cb = *pC++;
                    *pYCbCr444Tmp++ = Cr = *pC++;
                    *pYCbCr444Tmp++ = *pY++;
                    *pYCbCr444Tmp++ = Cb;
                    *pYCbCr444Tmp++ = Cr;
                }

                // update line starts
                pYTmp    += pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes;
                pCbCrTmp += pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes;
            }

            // inplace convert consecutive YCbCr444 to RGB; both are combined color component planes
            ConvertYCbCr444combToRGBcomb( pYCbCr444, YCPlaneSize );

            // write ppm header
            fprintf( pFile, "%sP6\n%d %d\n255\n", putHeader ? "" : "", pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel, pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel );

            // finally write result
            if ( 1 != fwrite( pYCbCr444, 3 * YCPlaneSize, 1, pFile ) )
            {
                UPDATE_RESULT( result, RET_FAILURE );
            }

            // release helper buffer
            free(pYCbCr444);
        }
    }

    // free local buffer
    free( pLocBuf );

    TRACE(SOM_CTRL_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}

/******************************************************************************
 * ConvertYCbCr444combToRGBcomb()
 *****************************************************************************/
static void ConvertYCbCr444combToRGBcomb
(
    uint8_t     *pYCbCr444,
    uint32_t    PlaneSizePixel
)
{
    uint32_t pix;
    for (pix = 0; pix < PlaneSizePixel; pix++)
    {
        // where to put the RGB data
        uint8_t *pPix = pYCbCr444;

        // get YCbCr pixel data
        int32_t Y  = *pYCbCr444++;
        int32_t Cb = *pYCbCr444++; // TODO: order in marvin output is CrCb and not CbCr as expected
        int32_t Cr = *pYCbCr444++; //       s. above

        // remove offset as in VideoDemystified 3; page 18f; YCbCr to RGB(0..255)
        Y  -=  16;
        Cr -= 128;
        Cb -= 128;

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

        // write back RGB data
        *pPix++ = (uint8_t) R;
        *pPix++ = (uint8_t) G;
        *pPix++ = (uint8_t) B;
    }
}
