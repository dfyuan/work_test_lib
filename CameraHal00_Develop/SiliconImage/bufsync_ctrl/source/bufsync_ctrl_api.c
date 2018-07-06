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
 * @file bufsync_ctrl_api.c
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
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include "bufsync_ctrl.h"
#include "bufsync_ctrl_cb.h"
#include "bufsync_ctrl_api.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( BUFSYNC_CTRL_API_INFO , "BUFSYNC-CTRL-API: ", INFO,    0 );
CREATE_TRACER( BUFSYNC_CTRL_API_WARN,  "BUFSYNC-CTRL-API: ", WARNING, 1 );
CREATE_TRACER( BUFSYNC_CTRL_API_ERROR, "BUFSYNC-CTRL-API: ", ERROR,   1 );



/******************************************************************************
 * BufSyncCtrlInit()
 *****************************************************************************/
RESULT BufSyncCtrlInit
(
    BufSyncCtrlConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    BufSyncCtrlContext_t *pBufSyncCtrlCtx;

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( (pConfig == NULL)
            || (pConfig->bufsyncCbCompletion == NULL)
            || (pConfig->pPicBufQueue1 == NULL)
            || (pConfig->pPicBufQueue2 == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( pConfig->MaxPendingCommands == 0 )
    {
        return ( RET_OUTOFRANGE );
    }

    /* allocate control context */
    pBufSyncCtrlCtx = malloc( sizeof(BufSyncCtrlContext_t) );
    if ( pBufSyncCtrlCtx == NULL )
    {
        TRACE( BUFSYNC_CTRL_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    memset( pBufSyncCtrlCtx, 0, sizeof(BufSyncCtrlContext_t) );

    pBufSyncCtrlCtx->State                  = eBufSyncCtrlStateInvalid;
    pBufSyncCtrlCtx->MaxCommands            = pConfig->MaxPendingCommands;
    pBufSyncCtrlCtx->bufsyncCbCompletion    = pConfig->bufsyncCbCompletion;
    pBufSyncCtrlCtx->pUserContext           = pConfig->pUserContext;

    pBufSyncCtrlCtx->QueueCtx[0].pOwner     = pBufSyncCtrlCtx;
    pBufSyncCtrlCtx->QueueCtx[0].pBufQueue  = pConfig->pPicBufQueue1;

    pBufSyncCtrlCtx->QueueCtx[1].pOwner     = pBufSyncCtrlCtx;
    pBufSyncCtrlCtx->QueueCtx[1].pBufQueue  = pConfig->pPicBufQueue2;

    /* create control process */
    result = BufSyncCtrlCreate( pBufSyncCtrlCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( BUFSYNC_CTRL_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pBufSyncCtrlCtx );

        pConfig->hBufSyncCtrl = NULL;
    }
    else
    {
        /* control context is initilized, we're ready and in idle state */
        pBufSyncCtrlCtx->State = eBufSyncCtrlStateInitialize;

        /* success, so let's return control context */
        pConfig->hBufSyncCtrl = (BufSyncCtrlHandle_t)pBufSyncCtrlCtx;
    }


    TRACE( BUFSYNC_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * BufSyncCtrlStart()
 *****************************************************************************/
RESULT BufSyncCtrlStart
(
    BufSyncCtrlHandle_t hBufSyncCtrl
)
{
    BufSyncCtrlContext_t *pBufSyncCtrlCtx = (BufSyncCtrlContext_t *)hBufSyncCtrl;

    BufSyncCmd_t Command;

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pBufSyncCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateInitialize )
            && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    MEMSET( &Command, 0, sizeof( BufSyncCmd_t ) );
    Command.CmdId = BUFSYNC_CTRL_CMD_START;
    BufSyncCtrlSendCommand( pBufSyncCtrlCtx, &Command );

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}



/******************************************************************************
 * BufSyncCtrlStop()
 *****************************************************************************/
RESULT BufSyncCtrlStop
(
    BufSyncCtrlHandle_t hBufSyncCtrl
)
{
    BufSyncCtrlContext_t *pBufSyncCtrlCtx = (BufSyncCtrlContext_t *)hBufSyncCtrl;

    BufSyncCmd_t Command;

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pBufSyncCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateRunning )
            && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    MEMSET( &Command, 0, sizeof( BufSyncCmd_t ) );
    Command.CmdId = BUFSYNC_CTRL_CMD_STOP;
    BufSyncCtrlSendCommand( pBufSyncCtrlCtx, &Command );

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}



/******************************************************************************
 * BufSyncCtrlShutDown()
 *****************************************************************************/
RESULT BufSyncCtrlShutDown
(
    BufSyncCtrlHandle_t hBufSyncCtrl
)
{
    BufSyncCtrlContext_t *pBufSyncCtrlCtx = (BufSyncCtrlContext_t *)hBufSyncCtrl;

    RESULT result = RET_SUCCESS;

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pBufSyncCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateInitialize )
            && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateStopped ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = BufSyncCtrlDestroy( pBufSyncCtrlCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( BUFSYNC_CTRL_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    /* release context memory */
    free( pBufSyncCtrlCtx );

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * BufSyncCtrlRegisterBufferCb()
 *****************************************************************************/
RESULT  BufSyncCtrlRegisterBufferCb
(
    BufSyncCtrlHandle_t     hBufSyncCtrl,
    BufSyncCtrlBufferCb_t   fpCallback,
    void                    *pBufferCbCtx
)
{
    BufSyncCtrlContext_t *pBufSyncCtrlCtx = (BufSyncCtrlContext_t *)hBufSyncCtrl;

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pBufSyncCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if( fpCallback == NULL )
    {
        return RET_NULL_POINTER;
    }

    if ( ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateInitialize )
      && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateRunning    )
      && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateStopped    ) )
    {
        return ( RET_WRONG_STATE );
    }

    osMutexLock( &pBufSyncCtrlCtx->BufferLock );

    pBufSyncCtrlCtx->BufferCb.fpCallback   = fpCallback;
    pBufSyncCtrlCtx->BufferCb.pBufferCbCtx = pBufferCbCtx;

    osMutexUnlock( &pBufSyncCtrlCtx->BufferLock );

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * BufSyncCtrlDeRegisterBufferCb()
 *****************************************************************************/
RESULT  BufSyncCtrlDeRegisterBufferCb
(
    BufSyncCtrlHandle_t  hBufSyncCtrl
)
{
    BufSyncCtrlContext_t *pBufSyncCtrlCtx = (BufSyncCtrlContext_t *)hBufSyncCtrl;

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pBufSyncCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateInitialize )
      && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateRunning    )
      && ( BufSyncCtrlGetState( pBufSyncCtrlCtx ) != eBufSyncCtrlStateStopped    ) )
    {
        return ( RET_WRONG_STATE );
    }

    osMutexLock( &pBufSyncCtrlCtx->BufferLock );

    pBufSyncCtrlCtx->BufferCb.fpCallback   = NULL;
    pBufSyncCtrlCtx->BufferCb.pBufferCbCtx = NULL;

    osMutexUnlock( &pBufSyncCtrlCtx->BufferLock );

    TRACE( BUFSYNC_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

