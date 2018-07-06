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
 * @file cam_engine_imgeffects_api.c
 *
 * @brief
 *   Implementation of the CamEngine Image Effects API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_imgeffects_api.h"

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
 * CamEngineEnableImageEffect()
 *****************************************************************************/
RESULT CamEngineEnableImageEffect
(
    CamEngineHandle_t hCamEngine,
    CamerIcIeConfig_t *pConfig
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

    result = CamerIcIeConfigure( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, pConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamerIcIeEnable( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineDisableImageEffect()
 *****************************************************************************/
RESULT CamEngineDisableImageEffect
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

    result = CamerIcIeDisable( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineImageEffectSetTintCb()
 *****************************************************************************/
RESULT CamEngineImageEffectSetTintCb
(
    CamEngineHandle_t   hCamEngine,
    const uint8_t       tint
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

    result = CamerIcIeSetTintCb( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, tint );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineImageEffectSetTintCr()
 *****************************************************************************/
RESULT CamEngineImageEffectSetTintCr
(
    CamEngineHandle_t   hCamEngine,
    const uint8_t       tint
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

    result = CamerIcIeSetTintCr( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, tint );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineImageEffecSetColorSelection()
 *****************************************************************************/
RESULT CamEngineImageEffectSetColorSelection
(
    CamEngineHandle_t               hCamEngine,
    const CamerIcIeColorSelection_t color,
    const uint8_t                   threshold
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

    result = CamerIcIeSetColorSelection( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, color, threshold );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineImageEffecSetSharpen()
 *****************************************************************************/
RESULT CamEngineImageEffectSetSharpen
(
    CamEngineHandle_t   hCamEngine,
    const uint8_t       factor,
    const uint8_t       threshold
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

    result = CamerIcIeSetSharpen( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, factor, threshold );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
