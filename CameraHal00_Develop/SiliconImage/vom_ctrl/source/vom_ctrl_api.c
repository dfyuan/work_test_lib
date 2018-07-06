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
 * @file vom_ctrl_api.c
 *
 * @brief
 *   Implementation of vom ctrl API.
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

#include <ebase/types.h>
#include <ebase/trace.h>

#include "vom_ctrl.h"
#include "vom_ctrl_api.h"

CREATE_TRACER(VOM_CTRL_API_INFO , "VOM-CTRL-API: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_API_ERROR, "VOM-CTRL-API: ", ERROR, 1);


/***** local functions ***********************************************/

/***** API implementation ***********************************************/

/******************************************************************************
 * vomCtrlInit()
 *****************************************************************************/
RESULT vomCtrlInit
(
    vomCtrlConfig_t *pConfig
)
{
    RESULT result = RET_FAILURE;

    vomCtrlContext_t *pVomContext;

    TRACE( VOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pConfig->vomCbCompletion == NULL)
      || ( pConfig->MaxPendingCommands == 0) )
    {
        return ( RET_INVALID_PARM );
    }

    // get video format details
    const Cea861VideoFormatDetails_t *pVideoFormatDetails;
    pVideoFormatDetails = Cea861GetVideoFormatDetails( pConfig->VideoFormat );
    if (pVideoFormatDetails == NULL)
    {
        return RET_INVALID_PARM;
    }

    // check 3D video format details?
    if (pConfig->Enable3D)
    {
        Cea861VideoFormatDetails_t SubCeaFormatDetails;

        result = Hdmi3DGetVideoFormatDetails( pConfig->VideoFormat3D, 0, pVideoFormatDetails, &SubCeaFormatDetails );
        if (RET_SUCCESS != result)
        {
            return result;
        }
    }

    // allocate control context
    pVomContext = malloc( sizeof(vomCtrlContext_t) );
    if ( pVomContext == NULL )
    {
        TRACE( VOM_CTRL_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    memset( pVomContext, 0, sizeof(vomCtrlContext_t) );

    // pre initialize control context
    pVomContext->State           = eVomCtrlStateInvalid;
    pVomContext->MaxCommands     = pConfig->MaxPendingCommands;
    pVomContext->vomCbCompletion = pConfig->vomCbCompletion;
    pVomContext->pUserContext    = pConfig->pUserContext;
    pVomContext->pVideoFormat    = pVideoFormatDetails;
    pVomContext->HalHandle       = pConfig->HalHandle;
    pVomContext->Enable3D        = pConfig->Enable3D;
    pVomContext->VideoFormat3D   = pConfig->VideoFormat3D;
    pVomContext->MaxBuffers      = (0 == pConfig->MaxBuffers) ? 1 : pConfig->MaxBuffers;

    // create control process
    result = vomCtrlCreate( pVomContext );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pVomContext );
    }
    else
    {
        // control context is initilized, we're ready and in idle state
        pVomContext->State = eVomCtrlStateIdle;

        // success, so let's return the control context handle
        pConfig->VomCtrlHandle = (vomCtrlHandle_t)pVomContext;
    }

    TRACE( VOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * vomCtrlStart()
 *****************************************************************************/
RESULT vomCtrlStart
(
    vomCtrlHandle_t VomCtrlHandle
)
{
    TRACE(VOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( VomCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    vomCtrlContext_t *pVomContext = (vomCtrlContext_t *)VomCtrlHandle;

    RESULT result = vomCtrlSendCommand( pVomContext, VOM_CTRL_CMD_START );
    if (result != RET_SUCCESS)
    {
         TRACE(VOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(VOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * vomCtrlStop()
 *****************************************************************************/
RESULT vomCtrlStop
(
    vomCtrlHandle_t VomCtrlHandle
)
{
    TRACE(VOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( VomCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    vomCtrlContext_t *pVomContext = (vomCtrlContext_t *)VomCtrlHandle;

    RESULT result = vomCtrlSendCommand( pVomContext, VOM_CTRL_CMD_STOP );
    if (result != RET_SUCCESS)
    {
         TRACE(VOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(VOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * vomCtrlShutDown()
 *****************************************************************************/
RESULT vomCtrlShutDown
(
    vomCtrlHandle_t VomCtrlHandle
)
{
    TRACE(VOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( VomCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    vomCtrlContext_t *pVomContext = (vomCtrlContext_t *)VomCtrlHandle;

    RESULT result = vomCtrlDestroy( pVomContext );
    if (result != RET_SUCCESS)
    {
         TRACE(VOM_CTRL_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    // release context memory
    free( pVomContext );

    TRACE(VOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * vomCtrlShowBuffer()
 *****************************************************************************/
RESULT  vomCtrlShowBuffer
(
	vomCtrlHandle_t  VomCtrlHandle,
	MediaBuffer_t    *pBuffer
)
{
	vomCtrlContext_t *pVomCtrlCtx = (vomCtrlContext_t *)VomCtrlHandle;

    TRACE( VOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pVomCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if( pBuffer == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ( eVomCtrlStateIdle    != pVomCtrlCtx->State )
      && ( eVomCtrlStateRunning != pVomCtrlCtx->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pBuffer->pNext != NULL )
    {
        MediaBufLockBuffer( pBuffer->pNext );
    }
    MediaBufLockBuffer( pBuffer );
    OSLAYER_STATUS osStatus = osQueueTryWrite( &pVomCtrlCtx->FullBufQueue, &pBuffer );
    if ( osStatus == OSLAYER_OK )
    {
        RESULT result = vomCtrlSendCommand( pVomCtrlCtx, VOM_CTRL_CMD_PROCESS_BUFFER );
        if (result != RET_SUCCESS)
        {
            TRACE(VOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        }
    }
    else
    {
        if ( pBuffer->pNext != NULL )
        {
            MediaBufUnlockBuffer( pBuffer->pNext );
        }
        MediaBufUnlockBuffer( pBuffer );
    }

    TRACE( VOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}

