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
 * @file cameric_mipi.c
 *
 * @brief   Implementation of MIPI driver API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "mipi_drv_api.h"
#include "mipi_drv.h"


CREATE_TRACER( MIPI_DRV_API_INFO  , "MIPI_DRV_API: ", INFO    , 0 );
CREATE_TRACER( MIPI_DRV_API_WARN  , "MIPI_DRV_API: ", WARNING , 1 );
CREATE_TRACER( MIPI_DRV_API_ERROR , "MIPI_DRV_API: ", ERROR   , 1 );


/******************************************************************************
 * local macro definitions
 *****************************************************************************/


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
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * MipiDrvInit()
 *****************************************************************************/
RESULT MipiDrvInit
(
    MipiDrvConfig_t  *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_API_INFO, "%s: (enter)\n", __FUNCTION__ );

    // check API params
    if( (pConfig == NULL) || (pConfig->HalHandle == NULL) )
    {
        result = RET_NULL_POINTER;
        goto error_exit;
    }

#ifdef MIPI_USE_CAMERIC
    if(pConfig->CamerIcDrvHandle == NULL)
    {
        result = RET_NULL_POINTER;
        goto error_exit;
    }
#endif

    if (pConfig->InstanceNum > 1)
    {
        result = RET_INVALID_PARM;
        goto error_exit;
    }

    // allocate context
    MipiDrvContext_t *pMipiDrvCtx = malloc( sizeof(MipiDrvContext_t) );
    if (pMipiDrvCtx == NULL)
    {
        result = RET_OUTOFMEM;
        goto error_exit;
    }

    // pre initialize context
    memset( pMipiDrvCtx, 0, sizeof(*pMipiDrvCtx) );
    switch(pConfig->InstanceNum)
    {
        case 0: pMipiDrvCtx->pBase = (void*)HAL_BASEADDR_MARVIN; break;
        case 1: pMipiDrvCtx->pBase = (void*)HAL_BASEADDR_MARVIN_2; break;
        default:
            result = RET_INVALID_PARM;
            goto cleanup_1;
    }
    pMipiDrvCtx->Config = *pConfig;
    pMipiDrvCtx->MipiConfig.NumLanes = 0;
    pMipiDrvCtx->State = MIPI_DRV_STATE_NOT_CONFIGURED;

    // initialize driver
    result = MipiDrvCreate( pMipiDrvCtx );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: MipiDrvCreate() failed (result=%d)\n", __FUNCTION__, result );
        goto cleanup_1;
    }

    // success, so return handle
    pConfig->MipiDrvHandle = (MipiDrvHandle_t)pMipiDrvCtx;

    TRACE( MIPI_DRV_API_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result ;

cleanup_1: // free context
    free( pMipiDrvCtx );

error_exit: // just return with error
    TRACE( MIPI_DRV_API_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvRelease()
 *****************************************************************************/
RESULT MipiDrvRelease
(
    MipiDrvHandle_t MipiDrvHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_API_INFO, "%s: (enter)\n", __FUNCTION__ );

    // check API params
    if( MipiDrvHandle == NULL )
    {
        result = RET_NULL_POINTER;
        goto error_exit;
    }

    // get context
    MipiDrvContext_t *pMipiDrvCtx = (MipiDrvContext_t *)MipiDrvHandle;

    // check state
    if ( (pMipiDrvCtx->State != MIPI_DRV_STATE_STOPPED) && (pMipiDrvCtx->State != MIPI_DRV_STATE_NOT_CONFIGURED) )
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // destroy driver
    result = MipiDrvDestroy( pMipiDrvCtx );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: MipiDrvDestroy() failed (result=%d)\n", __FUNCTION__, result );
    }

    // free context
    free( pMipiDrvCtx );

    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_DRV_API_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_API_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvConfig()
 *****************************************************************************/
RESULT MipiDrvConfig
(
    MipiDrvHandle_t MipiDrvHandle,
    MipiConfig_t    *pMipiConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_API_INFO, "%s: (enter)\n", __FUNCTION__ );

    // check API params
    if( (MipiDrvHandle == NULL) || (pMipiConfig == NULL) )
    {
        result = RET_NULL_POINTER;
        goto error_exit;
    }

    // get context
    MipiDrvContext_t *pMipiDrvCtx = (MipiDrvContext_t *)MipiDrvHandle;

    // check state
    if ( (pMipiDrvCtx->State != MIPI_DRV_STATE_STOPPED) && (pMipiDrvCtx->State != MIPI_DRV_STATE_NOT_CONFIGURED) )
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // configure driver
    MipiDrvCmd_t cmd;
    cmd.ID = MIPI_DRV_CMD_CONFIG;
    cmd.params.config.pMipiConfig = pMipiConfig;
    result = MipiDrvCmd( pMipiDrvCtx, &cmd );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: MipiDrvCmd('MIPI_DRV_CMD_CONFIG') failed\n", __FUNCTION__, result );

        // set new state
        pMipiDrvCtx->State = MIPI_DRV_STATE_NOT_CONFIGURED;

        goto error_exit;
    }

    // success, so set new state
    pMipiDrvCtx->State = MIPI_DRV_STATE_STOPPED;

    TRACE( MIPI_DRV_API_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_API_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvStart()
 *****************************************************************************/
RESULT MipiDrvStart
(
    MipiDrvHandle_t MipiDrvHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_API_INFO, "%s: (enter)\n", __FUNCTION__ );

    // check API params
    if( MipiDrvHandle == NULL )
    {
        result = RET_NULL_POINTER;
        goto error_exit;
    }

    // get context
    MipiDrvContext_t *pMipiDrvCtx = (MipiDrvContext_t *)MipiDrvHandle;

    // check state
    if ( pMipiDrvCtx->State != MIPI_DRV_STATE_STOPPED )
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // start driver
    MipiDrvCmd_t cmd;
    cmd.ID = MIPI_DRV_CMD_START;
    result = MipiDrvCmd( pMipiDrvCtx, &cmd );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: MipiDrvCmd('MIPI_DRV_CMD_START') failed (result=%d)\n", __FUNCTION__, result );
        goto error_exit;
    }

    // success, so set new state
    pMipiDrvCtx->State = MIPI_DRV_STATE_RUNNING;

    TRACE( MIPI_DRV_API_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_API_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvStop()
 *****************************************************************************/
RESULT MipiDrvStop
(
    MipiDrvHandle_t MipiDrvHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_API_INFO, "%s: (enter)\n", __FUNCTION__ );

    // check API params
    if( MipiDrvHandle == NULL )
    {
        result = RET_NULL_POINTER;
        goto error_exit;
    }

    // get context
    MipiDrvContext_t *pMipiDrvCtx = (MipiDrvContext_t *)MipiDrvHandle;

    // check state
    if ( pMipiDrvCtx->State != MIPI_DRV_STATE_RUNNING )
    {
        // already stopped
        result = RET_SUCCESS;
        goto success_exit;
    }

    // stop driver
    MipiDrvCmd_t cmd;
    cmd.ID = MIPI_DRV_CMD_STOP;
    result = MipiDrvCmd( pMipiDrvCtx, &cmd );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_API_ERROR, "%s: MipiDrvCmd(MIPI_DRV_CMD_STOP) failed (result=%d)\n", __FUNCTION__, result );
        goto error_exit;
    }

    // success, so set new state
    pMipiDrvCtx->State = MIPI_DRV_STATE_STOPPED;

success_exit:
    TRACE( MIPI_DRV_API_INFO, "%s: (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_API_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}
