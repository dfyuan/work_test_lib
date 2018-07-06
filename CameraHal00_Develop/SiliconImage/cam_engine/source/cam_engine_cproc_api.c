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
 * @file cam_engine_cproc_api.c
 *
 * @brief
 *   Implementation of the CamEngine Color Processing API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/utl_fixfloat.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_cproc_drv_api.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_cproc_api.h"

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
 * CamEngineCnrIsAvailable()
 *****************************************************************************/
RESULT CamEngineCprocIsAvailable
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
    result = CamerIcCprocIsAvailable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCprocEnable()
 *****************************************************************************/
RESULT CamEngineCprocEnable
(
    CamEngineHandle_t               hCamEngine,
    CamEngineCprocConfig_t * const  pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    CamerIcCprocConfig_t Config;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* compute fix point values */
    Config.ChromaOut    = pConfig->ChromaOut;
    Config.LumaIn       = pConfig->LumaIn;
    Config.LumaOut      = pConfig->LumaOut;
    Config.contrast     = UtlFloatToFix_U0107( pConfig->contrast );
    Config.saturation   = UtlFloatToFix_U0107( pConfig->saturation );
    Config.brightness   = UtlFloatToFix_S0800( (float)pConfig->brightness );

    if ( pConfig->hue < 0.0f )
    {
        Config.hue = (uint8_t)( pConfig->hue * -128.0f / 90.0f );
    }
    else
    {
        Config.hue = (uint8_t)( pConfig->hue * 128.0f / 90.0f );
    }

    /* call cameric driver */
    result = CamerIcCprocConfigure( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &Config );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC configure failed (%d)\n", __FUNCTION__, result );
         return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocConfigure( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &Config );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC configure failed (%d)\n", __FUNCTION__, result );
             return ( result );
        }
    }

    /* call cameric driver */
    result = CamerIcCprocEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC enable failed (%d)\n", __FUNCTION__, result );
         return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC enable failed (%d)\n", __FUNCTION__, result );
             return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCprocDisable()
 *****************************************************************************/
RESULT CamEngineCprocDisable
(
    CamEngineHandle_t hCamEngine
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
    result = CamerIcCprocDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC disable failed (%d)\n", __FUNCTION__, result );
         return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC disable failed (%d)\n", __FUNCTION__, result );
             return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCprocStatus()
 *****************************************************************************/
RESULT CamEngineCprocStatus
(
    CamEngineHandle_t               hCamEngine,
    bool_t * const                  pRunning,
    CamEngineCprocConfig_t * const  pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;
    uint8_t value = 0U;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ( NULL == pRunning ) || ( NULL == pConfig ) )
    {
        return ( RET_NULL_POINTER );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check enable status */
    result = CamerIcCprocIsEnabled(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pRunning );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC check status failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* get contrast */
    result = CamerIcCprocGetContrast(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &value );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC get contrast failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* compute float value */
    pConfig->contrast = UtlFixToFloat_U0107( (uint32_t)value );

    /* get brightness */
    result = CamerIcCprocGetBrightness(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &value );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC get brightness failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* compute value */
    pConfig->brightness = (int8_t)value;

    /* get saturation */
    result = CamerIcCprocGetSaturation(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &value );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC get saturation failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }
    
    /* compute float value */
    pConfig->saturation = UtlFixToFloat_U0107( (uint32_t)value );

    /* get hue */
    result = CamerIcCprocGetHue(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &value );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC get hue failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }
    
    /* compute float value */
    pConfig->hue = ((float)((int8_t)(value) * 90) / 128.0f);

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCprocSetContrast()
 *****************************************************************************/
RESULT CamEngineCprocSetContrast
(
    CamEngineHandle_t   hCamEngine,
    float const         contrast
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

    /* compute fix point values */
    uint8_t drvContrast = UtlFloatToFix_U0107( contrast ); 
    
    /* call cameric driver */
    result = CamerIcCprocSetContrast(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, drvContrast );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC set contrast failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }
    
    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocSetContrast(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, drvContrast );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC set contrast failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCprocSetBrightness()
 *****************************************************************************/
RESULT CamEngineCprocSetBrightness
(
    CamEngineHandle_t   hCamEngine,
    int8_t const        brightness
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
    
    /* compute fix point values */
    uint8_t drvBrightness = UtlFloatToFix_S0800( (float)brightness );

    /* call cameric driver */
    result = CamerIcCprocSetBrightness(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, drvBrightness );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC set brightness failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocSetBrightness(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, drvBrightness );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC set brightness failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCprocSetSaturation()
 *****************************************************************************/
RESULT CamEngineCprocSetSaturation
(
    CamEngineHandle_t   hCamEngine,
    float const         saturation
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

    /* compute fix point values */
    uint8_t drvSaturation = UtlFloatToFix_U0107( (float)saturation );

    /* call cameric driver */
    result = CamerIcCprocSetSaturation(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, drvSaturation );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC set saturation failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocSetSaturation(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, drvSaturation );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC set saturation failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineCprocSetHue()
 *****************************************************************************/
RESULT CamEngineCprocSetHue
(
    CamEngineHandle_t   hCamEngine,
    float const         hue
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

    /* compute fix point values */
    uint8_t drvHue = (uint8_t)( hue * 128.0f / 90.0f );

    /* call cameric driver */
    result = CamerIcCprocSetHue(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, drvHue );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CPROC set hue failed (%d)\n", __FUNCTION__, result );
         return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcCprocSetHue(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, drvHue );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CPROC set hue failed (%d)\n", __FUNCTION__, result );
             return ( result );
        }
    }
    
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

