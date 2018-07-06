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
 * @file exa_ctrl_api.c
 *
 * @brief
 *   Implementation of exa ctrl API.
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

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "exa_ctrl.h"
#include "exa_ctrl_api.h"

CREATE_TRACER(EXA_CTRL_API_INFO , "EXA-CTRL-API: ", INFO,  0);
CREATE_TRACER(EXA_CTRL_API_ERROR, "EXA-CTRL-API: ", ERROR, 1);


/***** local functions ***********************************************/

/***** API implementation ***********************************************/

/******************************************************************************
 * exaCtrlInit()
 *****************************************************************************/
RESULT exaCtrlInit
(
    exaCtrlConfig_t *pConfig
)
{
    RESULT result = RET_FAILURE;

    exaCtrlContext_t *pExaContext;

    TRACE( EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ( pConfig->exaCbCompletion == NULL ) ||
         ( pConfig->MaxPendingCommands == 0 ) )
    {
        return ( RET_INVALID_PARM );
    }

    // allocate control context
    pExaContext = malloc( sizeof(exaCtrlContext_t) );
    if ( pExaContext == NULL )
    {
        TRACE( EXA_CTRL_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pExaContext, 0, sizeof(exaCtrlContext_t) );

    // pre initialize control context
    pExaContext->State           = eExaCtrlStateInvalid;
    pExaContext->MaxCommands     = pConfig->MaxPendingCommands;
    pExaContext->MaxBuffers      = pConfig->MaxBuffers;
    pExaContext->exaCbCompletion = pConfig->exaCbCompletion;
    pExaContext->pUserContext    = pConfig->pUserContext;
    pExaContext->HalHandle       = pConfig->HalHandle;

    // create control process
    result = exaCtrlCreate( pExaContext );
    if (result != RET_SUCCESS)
    {
        TRACE( EXA_CTRL_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pExaContext );
    }
    else
    {
        // control context is initialized, we're ready and in idle state
        pExaContext->State = eExaCtrlStateIdle;

        // success, so let's return the control context handle
        pConfig->exaCtrlHandle = (exaCtrlHandle_t)pExaContext;
    }

    TRACE( EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * exaCtrlStart()
 *****************************************************************************/
RESULT exaCtrlStart
(
    exaCtrlHandle_t     exaCtrlHandle,
    exaCtrlSampleCb_t   exaCbSample,
    void                *pSampleContext,
    uint8_t             SampleSkip
)
{
    TRACE(EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if (exaCtrlHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)exaCtrlHandle;

    // prepare command
    exaCtrlCmd_t Command;
    MEMSET( &Command, 0, sizeof(Command) );
    Command.CmdID = EXA_CTRL_CMD_START;
    Command.Params.Start.exaCbSample = exaCbSample;
    Command.Params.Start.pSampleContext = pSampleContext;
    Command.Params.Start.SampleSkip = SampleSkip;

    // send command
    RESULT result = exaCtrlSendCommand( pExaContext, &Command );
    if (result != RET_SUCCESS)
    {
         TRACE(EXA_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * exaCtrlStop()
 *****************************************************************************/
RESULT exaCtrlStop
(
    exaCtrlHandle_t exaCtrlHandle
)
{
    TRACE(EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( exaCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)exaCtrlHandle;

    // prepare command
    exaCtrlCmd_t Command;
    MEMSET( &Command, 0, sizeof(Command) );
    Command.CmdID = EXA_CTRL_CMD_STOP;

    // send command
    RESULT result = exaCtrlSendCommand( pExaContext, &Command );
    if (result != RET_SUCCESS)
    {
         TRACE(EXA_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * exaCtrlPause()
 *****************************************************************************/
RESULT exaCtrlPause
(
    exaCtrlHandle_t exaCtrlHandle
)
{
    TRACE(EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( exaCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)exaCtrlHandle;

    // prepare command
    exaCtrlCmd_t Command;
    MEMSET( &Command, 0, sizeof(Command) );
    Command.CmdID = EXA_CTRL_CMD_PAUSE;

    // send command
    RESULT result = exaCtrlSendCommand( pExaContext, &Command );
    if (result != RET_SUCCESS)
    {
         TRACE(EXA_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * exaCtrlResume()
 *****************************************************************************/
RESULT exaCtrlResume
(
    exaCtrlHandle_t exaCtrlHandle
)
{
    TRACE(EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( exaCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)exaCtrlHandle;

    // prepare command
    exaCtrlCmd_t Command;
    MEMSET( &Command, 0, sizeof(Command) );
    Command.CmdID = EXA_CTRL_CMD_RESUME;

    // send command
    RESULT result = exaCtrlSendCommand( pExaContext, &Command );
    if (result != RET_SUCCESS)
    {
         TRACE(EXA_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * exaCtrlShutDown()
 *****************************************************************************/
RESULT exaCtrlShutDown
(
    exaCtrlHandle_t exaCtrlHandle
)
{
    TRACE(EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( exaCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)exaCtrlHandle;

    RESULT result = exaCtrlDestroy( pExaContext );
    if (result != RET_SUCCESS)
    {
         TRACE(EXA_CTRL_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    // release context memory
    free( pExaContext );

    TRACE(EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * exaCtrlShowBuffer()
 *****************************************************************************/
RESULT  exaCtrlShowBuffer
(
    exaCtrlHandle_t         exaCtrlHandle,
    MediaBuffer_t           *pBuffer
)
{
    exaCtrlContext_t *pExaContext = (exaCtrlContext_t *)exaCtrlHandle;

    TRACE( EXA_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pExaContext == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if( pBuffer == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ( eExaCtrlStateIdle    != pExaContext->State )
      && ( eExaCtrlStatePaused  != pExaContext->State )
      && ( eExaCtrlStateRunning != pExaContext->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    MediaBufLockBuffer( pBuffer );
    OSLAYER_STATUS osStatus = osQueueTryWrite( &pExaContext->FullBufQueue, &pBuffer );
    if ( osStatus == OSLAYER_OK )
    {
        // prepare command
        exaCtrlCmd_t Command;
        MEMSET( &Command, 0, sizeof(Command) );
        Command.CmdID = EXA_CTRL_CMD_PROCESS_BUFFER;

        RESULT result = exaCtrlSendCommand( pExaContext, &Command );
        if (result != RET_SUCCESS)
        {
            TRACE(EXA_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        }
    }
    else
    {
        MediaBufUnlockBuffer( pBuffer );
    }

    TRACE( EXA_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}
