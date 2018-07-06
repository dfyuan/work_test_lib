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
 * @file dom_ctrl_api.c
 *
 * @brief
 *   Implementation of dom ctrl API.
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

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "dom_ctrl.h"
#include "dom_ctrl_api.h"

CREATE_TRACER(DOM_CTRL_API_INFO , "DOM-CTRL-API: ", INFO,  0);
CREATE_TRACER(DOM_CTRL_API_ERROR, "DOM-CTRL-API: ", ERROR, 1);


/***** local functions ***********************************************/

/***** API implementation ***********************************************/

/******************************************************************************
 * domCtrlInit()
 *****************************************************************************/
RESULT domCtrlInit
(
    domCtrlConfig_t *pConfig
)
{
    RESULT result = RET_FAILURE;

    domCtrlContext_t *pdomContext;

    TRACE( DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    //if ( (pConfig->pInputQueue == NULL) || (pConfig->domCbCompletion == NULL)
    //  || ( pConfig->MaxPendingCommands == 0) )
    if ( ( pConfig->domCbCompletion == NULL ) ||
         ( pConfig->MaxPendingCommands == 0 ) )
    {
        return ( RET_INVALID_PARM );
    }

    // allocate control context
    pdomContext = malloc( sizeof(domCtrlContext_t) );
    if ( pdomContext == NULL )
    {
        TRACE( DOM_CTRL_API_ERROR, "%s (allocating control context failed)\n", __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    memset( pdomContext, 0, sizeof(domCtrlContext_t) );

    // pre initialize control context
    pdomContext->State      = edomCtrlStateInvalid;
    pdomContext->Config     = *pConfig;
    pdomContext->ImgPresent = pConfig->ImgPresent;
    pdomContext->Config.MaxBuffers = ( 0 == pConfig->MaxBuffers ) ? 1 : pConfig->MaxBuffers;

    // create control process
    result = domCtrlCreate( pdomContext );
    if (result != RET_SUCCESS)
    {
        TRACE( DOM_CTRL_API_ERROR, "%s (creating control process failed)\n", __FUNCTION__ );
        free( pdomContext );
    }
    else
    {
        // control context is initilized, we're ready and in idle state
        pdomContext->State = edomCtrlStateIdle;

        // success, so let's return the control context handle
        pConfig->domCtrlHandle = (domCtrlHandle_t)pdomContext;
    }

    TRACE( DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * domCtrlStart()
 *****************************************************************************/
RESULT domCtrlStart
(
    domCtrlHandle_t domCtrlHandle
)
{
    TRACE(DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if (domCtrlHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    domCtrlContext_t *pdomContext = (domCtrlContext_t *)domCtrlHandle;

    // send command
    RESULT result = domCtrlSendCommand( pdomContext, DOM_CTRL_CMD_START );
    if (result != RET_SUCCESS)
    {
         TRACE(DOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * domCtrlStop()
 *****************************************************************************/
RESULT domCtrlStop
(
    domCtrlHandle_t domCtrlHandle
)
{
    TRACE(DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( domCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    domCtrlContext_t *pdomContext = (domCtrlContext_t *)domCtrlHandle;

    // send command
    RESULT result = domCtrlSendCommand( pdomContext, DOM_CTRL_CMD_STOP );
    if (result != RET_SUCCESS)
    {
         TRACE(DOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return (result != RET_SUCCESS) ? result : RET_PENDING;
}


/******************************************************************************
 * domCtrlShutDown()
 *****************************************************************************/
RESULT domCtrlShutDown
(
    domCtrlHandle_t domCtrlHandle
)
{
    TRACE(DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__);

    if( domCtrlHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    domCtrlContext_t *pdomContext = (domCtrlContext_t *)domCtrlHandle;

    RESULT result = domCtrlDestroy( pdomContext );
    if (result != RET_SUCCESS)
    {
         TRACE(DOM_CTRL_API_ERROR, "%s (destroying control process failed -> RESULT=%d)\n", __FUNCTION__, result);
    }

    // release context memory
    free( pdomContext );

    TRACE(DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}

/******************************************************************************
 * domCtrlShowBuffer()
 *****************************************************************************/
RESULT  domCtrlShowBuffer
(
    domCtrlHandle_t         hDomContext,
    MediaBuffer_t           *pBuffer
)
{
    domCtrlContext_t *pDomCtrlCtx = (domCtrlContext_t *)hDomContext;

    TRACE( DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pDomCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if( pBuffer == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    if ( ( edomCtrlStateIdle    != pDomCtrlCtx->State )
      && ( edomCtrlStateRunning != pDomCtrlCtx->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pBuffer->pNext != NULL )
    {
        MediaBufLockBuffer( pBuffer->pNext );
    }
    MediaBufLockBuffer( pBuffer );
    OSLAYER_STATUS osStatus = osQueueTryWrite( &pDomCtrlCtx->FullBufQueue, &pBuffer );
    if ( osStatus == OSLAYER_OK )
    {
        RESULT result = domCtrlSendCommand( pDomCtrlCtx, DOM_CTRL_CMD_PROCESS_BUFFER );
        if (result != RET_SUCCESS)
        {
            TRACE(DOM_CTRL_API_ERROR, "%s (send command failed -> RESULT=%d)\n", __FUNCTION__, result);
        }
    }
    else
    {
        pDomCtrlCtx->FpsSkipped++;//FIXME mutex
        if ( pBuffer->pNext != NULL )
        {
            MediaBufUnlockBuffer( pBuffer->pNext );
        }
        MediaBufUnlockBuffer( pBuffer );
    }

    TRACE( DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_PENDING );
}


/******************************************************************************
 * domCtrlStartDraw()
 *****************************************************************************/
RESULT domCtrlStartDraw
(
    domCtrlHandle_t         hDomContext,
    domCtrlDrawConfig_t     *pDrawConfig
)
{
    domCtrlContext_t *pDomCtrlCtx = (domCtrlContext_t *)hDomContext;

    TRACE( DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pDomCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( edomCtrlStateIdle    != pDomCtrlCtx->State )
      && ( edomCtrlStateRunning != pDomCtrlCtx->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    // Create draw context
    domCtrlDrawContext_t* pDrawCtx = malloc( sizeof( domCtrlDrawContext_t ) );
    if ( pDrawCtx == NULL )
    {
        return ( RET_OUTOFMEM );
    }
    MEMSET( pDrawCtx, 0, sizeof( domCtrlDrawContext_t ) );

    osMutexLock( &pDomCtrlCtx->drawMutex );

    // Add to list
    ListAddTail( pDomCtrlCtx->pDrawContextList, ((void *)pDrawCtx) );

    osMutexUnlock( &pDomCtrlCtx->drawMutex );

    // Return draw context
    pDrawConfig->hDrawContext = (domCtrlDrawHandle_t)pDrawCtx;

    TRACE( DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

//FIXME
static int FindQueue( List *pList, void *key )
{
    return ( (pList == key) ? 1 : 0 );
}

/******************************************************************************
 * domCtrlStopDraw()
 *****************************************************************************/
RESULT domCtrlStopDraw
(
    domCtrlHandle_t         hDomContext,
    domCtrlDrawHandle_t     hDrawContext
)
{
    domCtrlContext_t *pDomCtrlCtx = (domCtrlContext_t *)hDomContext;

    TRACE( DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pDomCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    domCtrlDrawContext_t *pDrawCtx = (domCtrlDrawContext_t *)hDrawContext;

    if( pDrawCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( edomCtrlStateIdle    != pDomCtrlCtx->State )
      && ( edomCtrlStateRunning != pDomCtrlCtx->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    osMutexLock( &pDomCtrlCtx->drawMutex );

    // Remove from list
    ListRemoveItem( pDomCtrlCtx->pDrawContextList, FindQueue, ((void *)pDrawCtx) );

    osMutexUnlock( &pDomCtrlCtx->drawMutex );

    // Release memory
    if( NULL != pDrawCtx->pIbdDrawCmds )
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

    TRACE( DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

/******************************************************************************
 * domCtrlDraw()
 *****************************************************************************/
RESULT domCtrlDraw
(
    domCtrlHandle_t         hDomContext,
    domCtrlDrawHandle_t     hDrawContext,
    uint32_t                numCmds,
    domCtrlDrawCmd_t        *pDrawCmds
)
{
    domCtrlContext_t *pDomCtrlCtx = (domCtrlContext_t *)hDomContext;

    TRACE( DOM_CTRL_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if( pDomCtrlCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    domCtrlDrawContext_t *pDrawCtx = (domCtrlDrawContext_t *)hDrawContext;

    if( pDrawCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( edomCtrlStateIdle    != pDomCtrlCtx->State )
      && ( edomCtrlStateRunning != pDomCtrlCtx->State ) )
    {
        return ( RET_WRONG_STATE );
    }

    osMutexLock( &pDomCtrlCtx->drawMutex );

    if( NULL != pDrawCtx->pIbdDrawCmds )
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
    pDrawCtx->NumDrawCmds  = numCmds;
    pDrawCtx->pIbdDrawCmds = (ibdCmd_t*)pDrawCmds;

    osMutexUnlock( &pDomCtrlCtx->drawMutex );

    TRACE( DOM_CTRL_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}
