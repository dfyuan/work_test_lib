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
 * @file cam_engine_jpe_api.c
 *
 * @brief
 *   Implementation of the CamEngine JPE API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_jpe_api.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

USE_TRACER( CAM_ENGINE_API_INFO  );
USE_TRACER( CAM_ENGINE_API_WARN  );
USE_TRACER( CAM_ENGINE_API_ERROR );

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
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * CamEngineEnableJpe()
 *****************************************************************************/
RESULT CamEngineEnableJpe
(
    CamEngineHandle_t hCamEngine,
    CamerIcJpeConfig_t *pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pConfig->width == 0U )
    {
        pConfig->width = ( 0 != pCamEngineCtx->outWidth[CAMERIC_MI_PATH_MAIN] ) ?
                pCamEngineCtx->outWidth[CAMERIC_MI_PATH_MAIN] :
                (uint16_t)( pCamEngineCtx->chain[CHAIN_MASTER].isWindow.width - pCamEngineCtx->chain[CHAIN_MASTER].isWindow.hOffset );
    }

    if ( pConfig->height == 0U )
    {
        pConfig->height = ( 0 != pCamEngineCtx->outHeight[CAMERIC_MI_PATH_MAIN] ) ?
                pCamEngineCtx->outHeight[CAMERIC_MI_PATH_MAIN] :
                (uint16_t)( pCamEngineCtx->chain[CHAIN_MASTER].isWindow.height - pCamEngineCtx->chain[CHAIN_MASTER].isWindow.vOffset );
    }

    result = CamerIcJpeConfigure( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, pConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamerIcJpeEnable( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pCamEngineCtx->enableJPE = BOOL_TRUE;

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineDisableJpe()
 *****************************************************************************/
RESULT CamEngineDisableJpe
(
    CamEngineHandle_t hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamerIcJpeDisable( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pCamEngineCtx->enableJPE = BOOL_FALSE;

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

