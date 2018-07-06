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
 * @file mim_ctrl_api.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <oslayer/oslayer.h>

#include <common/return_codes.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_queue.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include "mim_ctrl.h"
#include "mim_ctrl_cb.h"
#include "mim_ctrl_api.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER( MIM_CTRL_API_INFO , "MIM-CTRL-API: ", INFO,    0 );
CREATE_TRACER( MIM_CTRL_API_WARN , "MIM-CTRL-API: ", WARNING, 1 );
CREATE_TRACER( MIM_CTRL_API_ERROR, "MIM-CTRL-API: ", ERROR,   1 );



/******************************************************************************
 * MimCtrlInit()
 *****************************************************************************/
RESULT MimCtrlInit
( 
    MimCtrlConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    MimCtrlContext_t *pMimCtrlContext;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( (pConfig == NULL)
            || (pConfig->mimCbCompletion == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( pConfig->hCamerIc == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig->MaxPendingCommands == 0 ) 
    {
        return ( RET_OUTOFRANGE );
    }

    /* allocate control context */
    pMimCtrlContext= malloc( sizeof(MimCtrlContext_t) );
    if ( pMimCtrlContext == NULL )
    {
        TRACE( MIM_CTRL_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    memset( pMimCtrlContext, 0, sizeof(MimCtrlContext_t) );

    /* pre initialize control context */
    pMimCtrlContext->State           = eMimCtrlStateInvalid;
    pMimCtrlContext->MaxCommands     = pConfig->MaxPendingCommands;
    pMimCtrlContext->mimCbCompletion = pConfig->mimCbCompletion;
    pMimCtrlContext->pUserContext    = pConfig->pUserContext;
    pMimCtrlContext->hCamerIc        = pConfig->hCamerIc;

    /* create control process */
    result = MimCtrlCreate( pMimCtrlContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( MIM_CTRL_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pMimCtrlContext );
    }
    else
    {
        /* control context is initilized, we're ready and in idle state */
        pMimCtrlContext->State = eMimCtrlStateInitialize;

        /* success, so let's return control context */
        pConfig->hMimContext = (MimCtrlContextHandle_t)pMimCtrlContext;
    }

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * MimCtrlStart()
 *****************************************************************************/
RESULT MimCtrlStart
(
    MimCtrlContextHandle_t hMimContext
)
{
    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)hMimContext;

    RESULT result;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateInitialize )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = MimCtrlSendCommand( pMimCtrlCtx, MIM_CTRL_CMD_START );
    if ( result == RET_SUCCESS )
    {
        result = RET_PENDING;
    }

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * MimCtrlStop()
 *****************************************************************************/
RESULT MimCtrlStop
(
    MimCtrlContextHandle_t hMimContext
)
{
    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)hMimContext;

    RESULT result;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateRunning )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateInitialize )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateStopped )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateWaitForDma ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = MimCtrlSendCommand( pMimCtrlCtx, MIM_CTRL_CMD_STOP );
    if ( result == RET_SUCCESS )
    {
        result = RET_PENDING;
    }

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__  );

    return ( result );
}
 


/******************************************************************************
 * MimCtrlShutDown()
 *****************************************************************************/
RESULT MimCtrlShutDown
(
    MimCtrlContextHandle_t hMimContext
)
{
    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)hMimContext;

    RESULT result;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateInitialize )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = MimCtrlDestroy( pMimCtrlCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( MIM_CTRL_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    /* release context memory */
    free( pMimCtrlCtx );

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * MimCtrlAttachInQueue
 *****************************************************************************/
RESULT MimCtrlAttachInQueue
(
	MimCtrlContextHandle_t  hMimContext,
	osQueue                 *pQueue
)
{
    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)hMimContext;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateInitialize )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pMimCtrlCtx->pInQueue != NULL )
    {
        return ( RET_BUSY );
    }

    if ( pQueue != NULL )
    {
        pMimCtrlCtx->pInQueue = pQueue;
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * MimCtrlDetachInQueue
 *****************************************************************************/
RESULT MimCtrlDetachQueueToPath
(
	MimCtrlContextHandle_t  hMimContext,
    osQueue                 *pQueue
)
{
    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)hMimContext;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateInitialize )
            && ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pMimCtrlCtx->pInQueue == pQueue )
    {
        pMimCtrlCtx->pInQueue = NULL;
    }
    else
    {
        return ( RET_INVALID_PARM );
    }

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * MimCtrlLoadPicture
 *****************************************************************************/
RESULT MimCtrlLoadPicture
(
    MimCtrlContextHandle_t  hMimContext,
    PicBufMetaData_t        *pPicBuffer
)
{
    RESULT result = RET_PENDING;

    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)hMimContext;

    TRACE( MIM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pMimCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( MimCtrlGetState( pMimCtrlCtx ) != eMimCtrlStateRunning )
    {
        return ( RET_WRONG_STATE );
    }

    /* start dma transfer */
    pMimCtrlCtx->DmaCompletionCb.func           = MimCtrlDmaCompletionCb;
    pMimCtrlCtx->DmaCompletionCb.pUserContext   = pMimCtrlCtx;
    pMimCtrlCtx->DmaCompletionCb.pParam         = (void *)pPicBuffer;

    pMimCtrlCtx->pDmaPicBuffer = pPicBuffer;

    // MEMCPY( &pMimCtrlCtx->DmaPicBuffer, pPicBuffer, sizeof( PicBufMetaData_t ) );

    /* update mim-ctrl's state machine */
    result = MimCtrlSendCommand( pMimCtrlCtx, MIM_CTRL_CMD_DMA_TRANSFER );
    if ( result == RET_SUCCESS )
    {
        result = RET_PENDING;
    }

    TRACE( MIM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}




