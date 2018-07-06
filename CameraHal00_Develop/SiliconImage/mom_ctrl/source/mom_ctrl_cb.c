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
 * @mom_ctrl.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/

#include <ebase/trace.h>
#include <common/return_codes.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include "mom_ctrl_cb.h"
#include "mom_ctrl.h"


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( MOM_CTRL_CB_INFO , "MOM-CTRL-CB: ", INFO      , 0 );
CREATE_TRACER( MOM_CTRL_CB_WARN , "MOM-CTRL-CB: ", WARNING   , 1 );
CREATE_TRACER( MOM_CTRL_CB_ERROR, "MOM-CTRL-CB: ", ERROR     , 1 );

CREATE_TRACER( MOM_CTRL_CB_DEBUG, "", INFO, 1 );


#define container_of(ptr, type, member) \
    ((type *)(((uint32_t)(uint8_t *)(ptr)) - (uint32_t)(&((type *)0)->member)))


#define CAMERIC_MI_REQUEST_GET_EMPTY_BUFFER_TIMEOUT 0



/******************************************************************************
 * MomCtrlPictureBufferPoolNotifyCbMainPath()
 *****************************************************************************/
void MomCtrlPictureBufferPoolNotifyCbMainPath
(
    MediaBufQueueExEvent_t  event,
    void                    *pUserContext,
    const MediaBuffer_t     *pMediaBuffer
)
{
    TRACE( MOM_CTRL_CB_INFO, "%s (enter %p %d)\n", __FUNCTION__, pUserContext, event );

    MomCtrlContext_t *pMomCtrlCtx = (MomCtrlContext_t *)pUserContext;

    if ( (pMomCtrlCtx != NULL) && (MomCtrlGetState( pMomCtrlCtx ) == eMomCtrlStateRunning) )
    {
        switch ( event )
        {
            case EMPTY_BUFFER_ADDED:
                {
                    MediaBuffer_t *pBuffer = MediaBufPoolGetBuffer( pMomCtrlCtx->pPicBufPoolMainPath );
                    if ( pBuffer != NULL )
                    {
                        OSLAYER_STATUS osStatus = osQueueWrite( &pMomCtrlCtx->EmptyBufQueue[MOM_CTRL_PATH_MAINPATH-1], &pBuffer );
                        DCT_ASSERT( osStatus == OSLAYER_OK );
                    }

                    break;
                }

            default:
                {
                    break;
                }
        }
    }

    TRACE( MOM_CTRL_CB_INFO, "%s (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * MomCtrlPictureBufferPoolNotifyCbSelfPath()
 *****************************************************************************/
void MomCtrlPictureBufferPoolNotifyCbSelfPath
(
    MediaBufQueueExEvent_t  event,
    void                    *pUserContext,
    const MediaBuffer_t     *pMediaBuffer
)
{
    TRACE( MOM_CTRL_CB_INFO, "%s (enter %p %d)\n", __FUNCTION__, pUserContext, event );

    MomCtrlContext_t *pMomCtrlCtx = (MomCtrlContext_t *)pUserContext;
    if ( (pMomCtrlCtx != NULL) && (MomCtrlGetState( pMomCtrlCtx ) == eMomCtrlStateRunning) )
    {
        switch ( event )
        {
            case EMPTY_BUFFER_ADDED:
                {
                    MediaBuffer_t *pBuffer = MediaBufPoolGetBuffer( pMomCtrlCtx->pPicBufPoolSelfPath );
                    if ( pBuffer != NULL )
                    {
                        OSLAYER_STATUS osStatus = osQueueWrite( &pMomCtrlCtx->EmptyBufQueue[MOM_CTRL_PATH_SELFPATH-1], &pBuffer );
                        DCT_ASSERT( osStatus == OSLAYER_OK );
                    }
                    else
                    {
                    }

                    break;
                }

            default:
                {
                    break;
                }
        }
    }

    TRACE( MOM_CTRL_CB_INFO, "%s (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * MomCtrlCamerIcDrvRequestCb()
 *****************************************************************************/
RESULT MomCtrlCamerIcDrvRequestCb
(
    const uint32_t request,
    void **param,
    void *pUserContext
)
{
    RESULT result = RET_FAILURE;

    TRACE( MOM_CTRL_CB_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( (pUserContext != NULL) && ( param != NULL) )
    {
        MomCtrlContext_t *pMomCtrlCtx = (MomCtrlContext_t *)pUserContext;

        if ( MomCtrlGetState( pMomCtrlCtx ) == eMomCtrlStateRunning )
        {
            switch ( request )
            {
                case CAMERIC_MI_REQUEST_GET_EMPTY_MP_BUFFER:
                case CAMERIC_MI_REQUEST_GET_EMPTY_SP_BUFFER:
                    {
                        MediaBuffer_t *pBuffer = NULL;
                        OSLAYER_STATUS osStatus;
                        uint32_t path;

                        path = ( request == CAMERIC_MI_REQUEST_GET_EMPTY_MP_BUFFER ) ? MOM_CTRL_PATH_MAINPATH : MOM_CTRL_PATH_SELFPATH;

                        osStatus = osQueueTimedRead( &pMomCtrlCtx->EmptyBufQueue[path-1], &pBuffer, CAMERIC_MI_REQUEST_GET_EMPTY_BUFFER_TIMEOUT );
                        if ( (pBuffer != NULL) && (osStatus == OSLAYER_OK) )
                        {
                            TRACE( MOM_CTRL_CB_INFO, "%s (req buffer %p)\n", __FUNCTION__, pBuffer);
                            *param = pBuffer;
                            result = RET_SUCCESS;
                        }
                        else
                        {
                            TRACE( MOM_CTRL_CB_INFO, "%s (req buffer timed out)\n", __FUNCTION__);
                            *param = NULL;
                            result = RET_NOTAVAILABLE;
                        }

                        break;
                    }

                default:
                    {
                        break;
                    }
            }
        }
        else
        {
            result = RET_WRONG_STATE;
        }
    }

    TRACE( MOM_CTRL_CB_INFO, "%s (exit res=%d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * MomCtrlCamerIcDrvEventCb()
 *****************************************************************************/
void MomCtrlCamerIcDrvEventCb
(
    const uint32_t  event,
    void            *param,
    void            *pUserContext
)
{
    MomCtrlContext_t *pMomCtrlCtx = (MomCtrlContext_t *)pUserContext;

    TRACE( MOM_CTRL_CB_INFO, "%s (enter)\n", __FUNCTION__ );

    if (pMomCtrlCtx != NULL)
    {

        if ( MomCtrlGetState( pMomCtrlCtx ) == eMomCtrlStateRunning )
        {
            switch ( event )
            {

                case CAMERIC_MI_EVENT_FULL_MP_BUFFER:
                    {
                    	DCT_ASSERT( NULL != param );

                        OSLAYER_STATUS osStatus;

                        MediaBuffer_t   *pBuffer = (MediaBuffer_t *)param;
                        PicBufMetaData_t *pMetaData = (PicBufMetaData_t *)pBuffer->pMetaData;

                        osStatus = osTimeStampUs( &pMetaData->TimeStampUs );
                        DCT_ASSERT( osStatus == OSLAYER_OK );

                        osStatus = osQueueWrite( &pMomCtrlCtx->FullBufQueue[MOM_CTRL_PATH_MAINPATH-1], &pBuffer );
                        DCT_ASSERT( osStatus == OSLAYER_OK );

                        MomCtrlSendCommand( pMomCtrlCtx, MOM_CTRL_CMD_PROC_FULL_BUFFER_MAINPATH );

                        break;
                    }

                case CAMERIC_MI_EVENT_FLUSHED_MP_BUFFER:
                    {
                    	DCT_ASSERT( NULL != param );

                        /* mark as empty */
                        MediaBuffer_t *pBuffer = (MediaBuffer_t *)param;
                        MediaBufPoolFreeBuffer( pMomCtrlCtx->pPicBufPoolMainPath, pBuffer );

                        break;
                    }

                case CAMERIC_MI_EVENT_DROPPED_MP_BUFFER:
                    {
                        TRACE( MOM_CTRL_CB_INFO, "%s (MP buffer dropped)\n", __FUNCTION__ );

                        break;
                    }

                case CAMERIC_MI_EVENT_FULL_SP_BUFFER:
                    {
                    	DCT_ASSERT( NULL != param );

                        OSLAYER_STATUS osStatus;

                        MediaBuffer_t *pBuffer = (MediaBuffer_t *)param;
                        PicBufMetaData_t *pMetaData = (PicBufMetaData_t *)pBuffer->pMetaData;

                        osStatus = osTimeStampUs( &pMetaData->TimeStampUs );
                        DCT_ASSERT( osStatus == OSLAYER_OK );

                        osStatus = osQueueWrite( &pMomCtrlCtx->FullBufQueue[MOM_CTRL_PATH_SELFPATH-1], &pBuffer );
                        DCT_ASSERT( osStatus == OSLAYER_OK );

                        MomCtrlSendCommand( pMomCtrlCtx, MOM_CTRL_CMD_PROC_FULL_BUFFER_SELFPATH );

                        break;
                    }

                case CAMERIC_MI_EVENT_FLUSHED_SP_BUFFER:
                    {
                    	DCT_ASSERT( NULL != param );

                        /* mark as empty */
                        MediaBuffer_t *pBuffer = (MediaBuffer_t *)param;
                        MediaBufPoolFreeBuffer( pMomCtrlCtx->pPicBufPoolSelfPath, pBuffer );

                        break;
                    }

                case CAMERIC_MI_EVENT_DROPPED_SP_BUFFER:
                    {
                        TRACE( MOM_CTRL_CB_INFO, "%s (SP buffer dropped)\n", __FUNCTION__ );

                        break;
                    }

                default:
                    {
                        TRACE( MOM_CTRL_CB_ERROR, "%s unknown event\n", __FUNCTION__ );
                        break;
                    }
            }
        }
        else
        {
            TRACE( MOM_CTRL_CB_ERROR, "%s (wrong state)\n", __FUNCTION__ );
        }
    }

    TRACE( MOM_CTRL_CB_INFO, "%s (exit)\n", __FUNCTION__ );
}

