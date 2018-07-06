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
 * @file som_ctrl_api.c
 *
 * @brief
 *   Implementation of som ctrl API.
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

#include <ebase/types.h>
#include <ebase/trace.h>

#include "som_ctrl.h"
#include "som_ctrl_api.h"

CREATE_TRACER(SOM_CTRL_API_INFO , "SOM-CTRL-API: ", INFO, 0);
CREATE_TRACER(SOM_CTRL_API_ERROR, "SOM-CTRL-API: ", ERROR, 1);


/***** local functions ***********************************************/

/***** API implementation ***********************************************/

/******************************************************************************
 * somCtrlInit()
 *****************************************************************************/
RESULT somCtrlInit
(
    somCtrlConfig_t *pConfig
)
{
    RESULT result = RET_FAILURE;

    somCtrlContext_t *pSomContext;

    TRACE( SOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pConfig->somCbCompletion == NULL)
      || ( pConfig->MaxPendingCommands == 0) )
    {
        return ( RET_INVALID_PARM );
    }

    // allocate control context
    pSomContext = malloc( sizeof(somCtrlContext_t) );
    if ( pSomContext == NULL )
    {
        TRACE( SOM_CTRL_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    memset( pSomContext, 0, sizeof(somCtrlContext_t) );

    // pre initialize control context
    pSomContext->State           = eSomCtrlStateInvalid;
    pSomContext->MaxCommands     = pConfig->MaxPendingCommands;
    pSomContext->MaxBuffers      = (0 == pConfig->MaxBuffers) ? 1 : pConfig->MaxBuffers;
    pSomContext->somCbCompletion = pConfig->somCbCompletion;
    pSomContext->pUserContext    = pConfig->pUserContext;
    pSomContext->HalHandle       = pConfig->HalHandle;

    // create control process
    result = somCtrlCreate( pSomContext );
    if (result != RET_SUCCESS)
    {
        TRACE( SOM_CTRL_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pSomContext );
    }
    else
    {
        // control context is initilized, we're ready and in idle state
        pSomContext->State = eSomCtrlStateIdle;

        // success, so let's return the control context handle
        pConfig->somCtrlHandle = (somCtrlHandle_t)pSomContext;
    }

    TRACE( SOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * somCtrlStart()
 *****************************************************************************/
RESULT somCtrlStart
(
    somCtrlHandle_t     somCtrlHandle,
    somCtrlCmdParams_t  *pParams
)
{
    TRACE(SOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if (somCtrlHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    somCtrlContext_t *pSomContext = (somCtrlContext_t *)somCtrlHandle;

    if ( eSomCtrlStateIdle    != pSomContext->State )
    {
        return ( RET_WRONG_STATE );
    }

    // prepare command
    somCtrlCmd_t Command;
    memset( &Command, 0, sizeof(Command) );
    Command.CmdID = SOM_CTRL_CMD_START;
    Command.pParams = pParams;

    // send command
    RESULT result = somCtrlSendCommand( pSomContext, &Command );
    if (result != RET_SUCCESS)
    {
         TRACE(SOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(SOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * somCtrlStop()
 *****************************************************************************/
RESULT somCtrlStop
(
    somCtrlHandle_t somCtrlHandle
)
{
    TRACE(SOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( somCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    somCtrlContext_t *pSomContext = (somCtrlContext_t *)somCtrlHandle;

    if ( ( eSomCtrlStateIdle    != pSomContext->State )
      && ( eSomCtrlStateRunning != pSomContext->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    // prepare command
    somCtrlCmd_t Command;
    memset( &Command, 0, sizeof(Command) );
    Command.CmdID = SOM_CTRL_CMD_STOP;

    // send command
    RESULT result = somCtrlSendCommand( pSomContext, &Command );
    if (result != RET_SUCCESS)
    {
         TRACE(SOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(SOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * somCtrlShutDown()
 *****************************************************************************/
RESULT somCtrlShutDown
(
    somCtrlHandle_t somCtrlHandle
)
{
    TRACE(SOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( somCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    somCtrlContext_t *pSomContext = (somCtrlContext_t *)somCtrlHandle;

    RESULT result = somCtrlDestroy( pSomContext );
    if (result != RET_SUCCESS)
    {
         TRACE(SOM_CTRL_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    // release context memory
    free( pSomContext );

    TRACE(SOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * somCtrlStoreBuffer()
 *****************************************************************************/
RESULT  somCtrlStoreBuffer
(
    somCtrlHandle_t         somCtrlHandle,
    MediaBuffer_t           *pBuffer
)
{
    somCtrlContext_t *pSomCtrlCtx = (somCtrlContext_t *)somCtrlHandle;

    TRACE( SOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pSomCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if( pBuffer == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ( eSomCtrlStateIdle    != pSomCtrlCtx->State )
      && ( eSomCtrlStateRunning != pSomCtrlCtx->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pBuffer->pNext != NULL )
    {
        MediaBufLockBuffer( pBuffer->pNext );
    }
    MediaBufLockBuffer( pBuffer );
    OSLAYER_STATUS osStatus = osQueueTryWrite( &pSomCtrlCtx->FullBufQueue, &pBuffer );
    if ( osStatus == OSLAYER_OK )
    {
        // prepare command
        somCtrlCmd_t Command;
        memset( &Command, 0, sizeof(Command) );
        Command.CmdID = SOM_CTRL_CMD_PROCESS_BUFFER;

        RESULT result = somCtrlSendCommand( pSomCtrlCtx, &Command );
        if (result != RET_SUCCESS)
        {
            TRACE(SOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
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

    TRACE( SOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}
