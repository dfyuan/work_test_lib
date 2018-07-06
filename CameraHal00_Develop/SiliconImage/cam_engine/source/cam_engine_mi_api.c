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
 * @file cam_engine_mi_api.c
 *
 * @brief
 *   Implementation of the CamEngine MI API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_mi_drv_api.h>
#include <cameric_drv/cameric_dual_cropping_drv_api.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_mi_api.h"

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
 * CamEngineDualCroppingIsAvailable()
 *****************************************************************************/
RESULT CamEngineDualCroppingIsAvailable
(
    CamEngineHandle_t   hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING  ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_INITIALIZED) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcDualCropingIsAvailable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineMiSwapUV()
 *****************************************************************************/
RESULT CamEngineMiSwapUV
(
    CamEngineHandle_t   hCamEngine,
    bool_t const        swap
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcMiSwapColorChannel(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                CAMERIC_MI_PATH_MAIN, swap );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver MI set color channel swapping failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcMiSwapColorChannel(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                CAMERIC_MI_PATH_SELF, swap );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver MI set color channel swapping failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcMiSwapColorChannel(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc,
                    CAMERIC_MI_PATH_MAIN, swap );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver MI set color channel swapping failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        result = CamerIcMiSwapColorChannel(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc,
                    CAMERIC_MI_PATH_SELF, swap );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver MI set color channel swapping failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineIsPictureOrientationAllowed()
 *****************************************************************************/
RESULT CamEngineIsPictureOrientationAllowed
(
    CamEngineHandle_t           hCamEngine,
    CamEngineMiOrientation_t    orientation
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamerIcMiIsPictureOrientationAllowed(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                    CAMERIC_MI_PATH_SELF, orientation );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetPictureOrientation()
 *****************************************************************************/
RESULT CamEngineSetPictureOrientation
(
    CamEngineHandle_t           hCamEngine,
    CamEngineMiOrientation_t    orientation
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pCamEngineCtx->orient != orientation )
    {
        result = CamerIcMiSetPictureOrientation(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                    CAMERIC_MI_PATH_SELF,
                    orientation );
        if ( RET_SUCCESS == result )
        {
            pCamEngineCtx->orient = orientation ;
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

