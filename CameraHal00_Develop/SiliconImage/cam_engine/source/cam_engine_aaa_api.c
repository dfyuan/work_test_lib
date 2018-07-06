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
 * @file cam_engine_aaa_api.c
 *
 * @brief
 *   Implementation of the CamEngine Auto Algorithms API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/utl_fixfloat.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_aaa_api.h"

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
 * CamEngineAwbStart()
 *****************************************************************************/
RESULT CamEngineAwbStart
(
    CamEngineHandle_t           hCamEngine,
    const CamEngineAwbMode_t    mode,
    const uint32_t              index,
    const bool_t                damp
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    uint32_t  AwbFlags = 0;
    AwbMode_t AwbMode;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if awb available */
    if ( ( NULL == pCamEngineCtx->hAwb ) ||
         ( NULL == pCamEngineCtx->hCamCalibDb ) )
    {
        return ( RET_NOTSUPP );
    }

    switch ( mode )
    {
        case CAM_ENGINE_AWB_MODE_AUTO:
            {
                AwbMode = AWB_MODE_AUTO;
                break;
            }

        case CAM_ENGINE_AWB_MODE_MANUAL:
            {
                AwbMode = AWB_MODE_MANUAL;
                break;
            }
		case CAM_ENGINE_AWB_MODE_MANUAL_CT:
            {
                AwbMode = AWB_MODE_MANUAL_CT;
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    if ( (mode <= CAM_ENGINE_AWB_MODE_INVALID) || (mode >= CAM_ENGINE_AWB_MODE_MAX) )
    {
        return ( RET_INVALID_PARM );
    }

    result = AwbGetFlags( pCamEngineCtx->hAwb, &AwbFlags );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    if ( BOOL_TRUE == damp )
    {
        AwbFlags |= AWB_WORKING_FLAG_USE_DAMPING;
    }
    else
    {
        AwbFlags &= ~AWB_WORKING_FLAG_USE_DAMPING;
    }
    result = AwbSetFlags( pCamEngineCtx->hAwb, AwbFlags );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = AwbStart( pCamEngineCtx->hAwb, AwbMode, index );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s can't start white-balance (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAwbStop()
 *****************************************************************************/
RESULT CamEngineAwbStop
(
    CamEngineHandle_t hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    // check if cam-engine running
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    // check if awb available
    if ( ( NULL == pCamEngineCtx->hAwb ) ||
         ( NULL == pCamEngineCtx->hCamCalibDb ) )
    {
        return ( RET_NOTSUPP );
    }

    // stop awb now
    result = AwbStop( pCamEngineCtx->hAwb );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s can't stop white-balance (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAwbReset()
 *****************************************************************************/
RESULT CamEngineAwbReset
(
    CamEngineHandle_t hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if awb available */
    if ( ( NULL == pCamEngineCtx->hAwb ) ||
         ( NULL == pCamEngineCtx->hCamCalibDb ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AwbReset( pCamEngineCtx->hAwb );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s can't reset white-balance\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
/******************************************************************************
 * CamEngineAwbGetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAwbGetMeasuringWindow
(
    CamEngineHandle_t               hCamEngine,
    CamEngineWindow_t               *pWindow
 
)
{
    RESULT lres = RET_SUCCESS;
    bool_t isEnabled = BOOL_FALSE;
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
    CamerIcDrvHandle_t pCamerIcHandle;
    
    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if awb available with configured sensor */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAwb  ) )
    {
        return ( RET_NOTSUPP );
    }

    pCamerIcHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    lres = CamerIcIspAwbGetMeasuringWindow(pCamerIcHandle,(CamerIcWindow_t*)pWindow);
    
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    return lres;
    
}

/******************************************************************************
 * CamEngineAwbSetMeasuringWindow
 *****************************************************************************/
RESULT CamEngineAwbSetMeasuringWindow
(   
    CamEngineHandle_t               hCamEngine,
    int16_t                  x,
    int16_t                  y,
    uint16_t                  width,
    uint16_t                  height  
)
{
    RESULT result = RET_SUCCESS;
	RESULT lres;
    bool_t isEnabled = BOOL_FALSE;
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
    CamerIcDrvHandle_t pCamerIcHandle;
    
    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if awb available with configured sensor */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAwb  ) )
    {
        return ( RET_NOTSUPP );
    }

    isEnabled = BOOL_FALSE;
    lres = CamerIcIspAwbIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == lres )
    {
        if ( BOOL_TRUE == isEnabled )
        {
            result = CamerIcIspAwbDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s: Can't disable auto white balance (%d)\n", __FUNCTION__, result );
                return ( result );
            }
			
            result = CamerIcIspAwbSetMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc ,
												x,
												y,
												width,
												height );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIc Driver AWB window configuration failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            result = CamerIcIspAwbEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s: Can't enable auto white balance (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
 
    }
    else
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIcIspAwbIsEnabled failed!",__FUNCTION__ );
        result = lres;
    }
    
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    return result;

}


/******************************************************************************
 * CamEngineAwbGetTemperature()
 *****************************************************************************/
RESULT CamEngineAwbGetTemperature
(
    CamEngineHandle_t           hCamEngine,
    float*                      ct
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if awb available */
    if ( ( NULL == pCamEngineCtx->hAwb ) ||
         ( NULL == pCamEngineCtx->hCamCalibDb ) )
    {
        return ( RET_NOTSUPP );
    }

	result= AwbGetTemperature( pCamEngineCtx->hAwb,ct);

    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s can't get  white-balance temperature (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAwbStatus()
 *****************************************************************************/
RESULT CamEngineAwbStatus
(
    CamEngineHandle_t       hCamEngine,
    bool_t                  *pRunning,
    CamEngineAwbMode_t      *pMode,
    uint32_t                *pCieProfile,
    CamEngineAwbRgProj_t    *pRgProj,
    bool_t                  *pDamping
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    uint32_t    AwbFlags = 0;
    AwbMode_t   AwbMode;
    AwbRgProj_t AwbRgProj;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check if cam-engine running
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    // check references
    if ( ( pRunning == NULL )    ||
         ( pMode == NULL )       ||
         ( pCieProfile == NULL ) ||
         ( pRgProj == NULL )     ||
         ( pDamping == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    // check if awb available
    if ( ( NULL == pCamEngineCtx->hAwb ) ||
         ( NULL == pCamEngineCtx->hCamCalibDb ) )
    {
        return ( RET_NOTSUPP );
    }

    // get awb working flags
    result = AwbGetFlags( pCamEngineCtx->hAwb, &AwbFlags );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (AwbGetFlags failed with %d)\n", __FUNCTION__, result );
        return ( result );
    }
    *pDamping = (AwbFlags & AWB_WORKING_FLAG_USE_DAMPING) ? BOOL_TRUE : BOOL_FALSE;

    result = AwbStatus( pCamEngineCtx->hAwb, pRunning, &AwbMode, pCieProfile, &AwbRgProj );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (AwbStatus failed with %d)\n", __FUNCTION__, result );
        return ( result );
    }

    switch ( AwbMode )
    {
        case AWB_MODE_AUTO:
            {
                *pMode = CAM_ENGINE_AWB_MODE_AUTO;
                break;
            }

        case AWB_MODE_MANUAL:
            {
                *pMode = CAM_ENGINE_AWB_MODE_MANUAL;
                break;
            }
        case AWB_MODE_MANUAL_CT:
            {
                *pMode = CAM_ENGINE_AWB_MODE_MANUAL_CT;
                break;
            }

        default:
            {
                *pMode = CAM_ENGINE_AWB_MODE_INVALID;
                return ( RET_FAILURE );
            }
    }

    if ( pRgProj )
    {
        pRgProj->fRgProjIndoorMin   = AwbRgProj.fRgProjIndoorMin;
        pRgProj->fRgProjOutdoorMin  = AwbRgProj.fRgProjOutdoorMin;
        pRgProj->fRgProjMax         = AwbRgProj.fRgProjMax;
        pRgProj->fRgProjMaxSky      = AwbRgProj.fRgProjMaxSky;
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

/******************************************************************************
 * CamEngineAwbStable()
 *****************************************************************************/
RESULT CamEngineIsAwbStable
(
    CamEngineHandle_t       hCamEngine,
    bool_t                  *pIsStable,
    uint32_t				*pDNoWhitePixel
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    // check if cam-engine running
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    // check references
    if ( ( pIsStable == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    // check if awb available
    if ( ( NULL == pCamEngineCtx->hAwb ) ||
         ( NULL == pCamEngineCtx->hCamCalibDb ) )
    {
        return ( RET_NOTSUPP );
    }

    // get awb stable state?
    result = AwbSettled( pCamEngineCtx->hAwb, pIsStable, pDNoWhitePixel );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s (AwbSettled failed with %d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineAecStart()
 *****************************************************************************/
RESULT CamEngineAecStart
(
    CamEngineHandle_t           hCamEngine
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AecStart( pCamEngineCtx->hAec );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAecStop()
 *****************************************************************************/
RESULT CamEngineAecStop
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AecStop( pCamEngineCtx->hAec );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAecReset()
 *****************************************************************************/
RESULT CamEngineAecReset
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AecReset( pCamEngineCtx->hAec );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
/* ddl@rock-chips.com: v0.0x29.0 */
/******************************************************************************
 * CamEngine3aLock()   
 *****************************************************************************/
RESULT CamEngine3aLock
(
    CamEngineHandle_t hCamEngine,
    CamEngine3aLock_t lock
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    if (lock & Lock_aec) {
        result = AecTryLock( pCamEngineCtx->hAec );
    }

    if (lock & Lock_awb) {
        result |= AwbTryLock( pCamEngineCtx->hAwb );
    }

    if (lock & Lock_af) {
        result |= AfTryLock( pCamEngineCtx->hAf );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
/******************************************************************************
 * CamEngine3aUnLock()   
 *****************************************************************************/
RESULT CamEngine3aUnLock
(
    CamEngineHandle_t hCamEngine,
    CamEngine3aLock_t unlock
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    if (unlock & Lock_aec) {
        result = AecUnLock( pCamEngineCtx->hAec );
    }

    if (unlock & Lock_awb) {
        result |= AwbUnLock( pCamEngineCtx->hAwb );
    }

    if (unlock & Lock_af) {
        result |= AfUnLock( pCamEngineCtx->hAf );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
/******************************************************************************
 * CamEngineAecConfigure()
 *****************************************************************************/
RESULT CamEngineAecConfigure
(
    CamEngineHandle_t               hCamEngine,
    const CamEngineAecSemMode_t     mode,
    const float                     setPoint,
    const float                     clmTolerance,
    const CamEngineAecDampingMode_t dampingMode,
    const float                     dampOverStill,
    const float                     dampUnderStill,
    const float                     dampOverVideo,
    const float                     dampUnderVideo
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    AecConfig_t AecConfig;

    /* get current config */
    result = AecGetCurrentConfig( pCamEngineCtx->hAec, &AecConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    switch ( dampingMode )
    {
        case CAM_ENGINE_AEC_DAMPING_MODE_STILL_IMAGE:
            {
                AecConfig.DampingMode = AEC_DAMPING_MODE_STILL_IMAGE; 
                break;
            }
        
        case CAM_ENGINE_AEC_DAMPING_MODE_VIDEO:
            {
                AecConfig.DampingMode = AEC_DAMPING_MODE_VIDEO;
                break;
            }
        
        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    switch ( mode )
    {
        case CAM_ENGINE_AEC_SCENE_EVALUATION_DISABLED:
            {
                AecConfig.SemMode = AEC_SCENE_EVALUATION_DISABLED;
                break;
            }

        case CAM_ENGINE_AEC_SCENE_EVALUATION_FIX:
            {
                AecConfig.SemMode = AEC_SCENE_EVALUATION_FIX;
                break;
            }

        case CAM_ENGINE_AEC_SCENE_EVALUATION_ADAPTIVE:
            {
                AecConfig.SemMode = AEC_SCENE_EVALUATION_ADAPTIVE;
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    AecConfig.SetPoint          = setPoint;
    AecConfig.ClmTolerance      = clmTolerance;
    AecConfig.DampingMode       = dampingMode;
    AecConfig.DampOverStill     = dampOverStill;
    AecConfig.DampUnderStill    = dampUnderStill;
    AecConfig.DampOverVideo     = dampOverVideo;
    AecConfig.DampUnderVideo    = dampUnderVideo;

    /* configure */
    result = AecReConfigure( pCamEngineCtx->hAec, &AecConfig, NULL );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAecStatus()
 *****************************************************************************/
RESULT CamEngineAecStatus
(
    CamEngineHandle_t           hCamEngine,
    bool_t                      *pRunning,
    CamEngineAecSemMode_t       *pMode,
    float                       *pSetPoint,
    float                       *pClmTolerance,
    CamEngineAecDampingMode_t   *pDampingMode,
    float                       *pDampOverStill,
    float                       *pDampUnderStill,
    float                       *pDampOverVideo,
    float                       *pDampUnderVideo
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( (pRunning == NULL) || (pMode == NULL)
            || (pSetPoint == NULL) || (pClmTolerance == NULL)
            || (pDampOverStill == NULL) || (pDampUnderStill == NULL)
            || (pDampOverVideo == NULL) || (pDampUnderVideo == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    AecSemMode_t SemMode         = AEC_SCENE_EVALUATION_INVALID;
    AecDampingMode_t DampingMode = AEC_DAMPING_MODE_INVALID;

    result = AecStatus( pCamEngineCtx->hAec, pRunning, &SemMode, pSetPoint, pClmTolerance,
                            &DampingMode, pDampOverStill, pDampUnderStill, pDampOverVideo, pDampUnderVideo );
    
    // evaluate damping mode from AEC driver
    switch ( DampingMode )
    {
        case AEC_DAMPING_MODE_STILL_IMAGE:
            {
                *pDampingMode = CAM_ENGINE_AEC_DAMPING_MODE_STILL_IMAGE;
                break;
            }
        
        case AEC_DAMPING_MODE_VIDEO:
            {
                *pDampingMode = CAM_ENGINE_AEC_DAMPING_MODE_VIDEO;
                break;
            }
        
        default:
            {
                *pDampingMode = CAM_ENGINE_AEC_DAMPING_MODE_INVALID;
                return ( RET_FAILURE );
            }
    }

    // evaluate scene-evaluation mode from AEC driver
    switch ( SemMode )
    {
        case AEC_SCENE_EVALUATION_DISABLED:
            {
                *pMode = CAM_ENGINE_AEC_SCENE_EVALUATION_DISABLED;
                break;
            }

        case AEC_SCENE_EVALUATION_FIX:
            {
                *pMode = CAM_ENGINE_AEC_SCENE_EVALUATION_FIX;
                break;
            }

        case AEC_SCENE_EVALUATION_ADAPTIVE:
            {
                *pMode = CAM_ENGINE_AEC_SCENE_EVALUATION_ADAPTIVE;
                break;
            }

        default:
            {
                *pMode = CAM_ENGINE_AWB_MODE_INVALID;
                return ( RET_FAILURE );
            }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAecGetHistogram()
 *****************************************************************************/
RESULT CamEngineAecGetHistogram
(
    CamEngineHandle_t        hCamEngine,
    CamEngineAecHistBins_t   *pHistogram
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( (pHistogram == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    DCT_ASSERT( sizeof( CamEngineAecHistBins_t ) == sizeof( CamerIcHistBins_t ) );
    result = AecGetCurrentHistogram( pCamEngineCtx->hAec, (CamerIcHistBins_t *)pHistogram );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAecGetLuminance()
 *****************************************************************************/
RESULT CamEngineAecGetLuminance
(
    CamEngineHandle_t        hCamEngine,
    CamEngineAecMeanLuma_t   *pLuma
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( (pLuma == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    DCT_ASSERT( sizeof( CamEngineAecMeanLuma_t ) == sizeof( CamerIcMeanLuma_t ) );
    result = AecGetCurrentLuminance( pCamEngineCtx->hAec, (CamerIcMeanLuma_t *)pLuma );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

/******************************************************************************
 * CamEngineAecGetClmTolerance()
 *****************************************************************************/
RESULT CamEngineAecGetClmTolerance
(
    CamEngineHandle_t        hCamEngine,
    float   *clmTolerance
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }
	if ( (clmTolerance == NULL) )
    {
        return ( RET_INVALID_PARM );
    }
	AecGetClmTolerance(pCamEngineCtx->hAec, clmTolerance);
	return ( result );
}

/******************************************************************************
 * CamEngineSetAecClmTolerance()
 *****************************************************************************/
RESULT CamEngineSetAecClmTolerance
(
	CamEngineHandle_t		 hCamEngine,
	float	clmTolerance
)
{
	CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

	RESULT result = RET_SUCCESS;
	if ( pCamEngineCtx == NULL )
	{
		return ( RET_WRONG_HANDLE );
	}

	AecSetClmTolerance(pCamEngineCtx->hAec, clmTolerance);
	return ( result );
}

/******************************************************************************
 * CamEngineAecGetObjectRegion()
 *****************************************************************************/
RESULT CamEngineAecGetObjectRegion
(
    CamEngineHandle_t        hCamEngine,
    CamEngineAecMeanLuma_t   *pObjectRegion
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( (pObjectRegion == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if aec available */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    DCT_ASSERT( sizeof( CamEngineAecMeanLuma_t ) == sizeof( CamerIcMeanLuma_t ) );
    result = AecGetCurrentObjectRegion( pCamEngineCtx->hAec, (CamerIcMeanLuma_t *)pObjectRegion );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
/******************************************************************************
 * CamEngineAecGetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAecGetMeasuringWindow
(
    CamEngineHandle_t               hCamEngine,
    CamEngineWindow_t               *pWindow,
    CamEngineWindow_t               *pGrid
)
{
    RESULT lres = RET_SUCCESS;
    bool_t isEnabled = BOOL_FALSE;
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
    CamerIcDrvHandle_t pCamerIcHandle;
    
    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if aec available with configured sensor */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }

    pCamerIcHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    lres = CamerIcIspExpGetMeasuringWindow(pCamerIcHandle,(CamerIcWindow_t*)pWindow,(CamerIcWindow_t*)pGrid);
    
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    return lres;
    
}

/******************************************************************************
 * CamEngineAecHistSetMeasureWinAndMode
 *****************************************************************************/
RESULT CamEngineAecHistSetMeasureWinAndMode
(   
    CamEngineHandle_t               hCamEngine,
    int16_t                  x,
    int16_t                  y,
    uint16_t                  width,
    uint16_t                  height,
    CamEngineAecHistMeasureMode_t mode
)
{
    RESULT result = RET_SUCCESS, lres;
    bool_t isEnabled = BOOL_FALSE;
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
	CamCalibAecGlobal_t *pAecGlobal;
    CamerIcDrvHandle_t pCamerIcHandle;
    unsigned char gridWeights[25];
    CamEngineWindow_t curAeWin;
    int16_t hist_x,hist_y;
    uint16_t hist_w,hist_h;
	int16_t point_x,point_y;
	double wScale, hScale;
  
    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if aec available with configured sensor */
    if ( ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAec ) )
    {
        return ( RET_NOTSUPP );
    }
	#if 0/*don't need*/
	result = CamCalibDbGetAecGlobal( pCamEngineCtx->hCamCalibDb, &pAecGlobal );
	if ( result != RET_SUCCESS )
	{
		TRACE( CAM_ENGINE_API_ERROR, "%s: Access to CalibDB failed. (%d)\n", __FUNCTION__, result );
		return ( result );
	}

	wScale=pAecGlobal->MeasuringWinWidthScale;
	hScale=pAecGlobal->MeasuringWinHeightScale;
	TRACE( CAM_ENGINE_API_INFO, "%s:MeasuringWinWidthScale=%f,MeasuringWinHeightScale=%f\n",
	__FUNCTION__, pAecGlobal->MeasuringWinWidthScale, pAecGlobal->MeasuringWinHeightScale);
	#endif
    isEnabled = BOOL_FALSE;
    pCamerIcHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    lres = CamerIcIspExpIsEnabled( pCamerIcHandle, &isEnabled );
    if ( RET_SUCCESS == lres ) {        
        if ( BOOL_TRUE == isEnabled ) {
            
            uint16_t outWidth  = ( pCamEngineCtx->outWindow.width -  pCamEngineCtx->outWindow.hOffset );
            uint16_t outHeight = ( pCamEngineCtx->outWindow.height - pCamEngineCtx->outWindow.vOffset );
            CamerIcWindow_t curAfWin; 

            if (x || y || width || height) {
                x += 1000;
                y += 1000;

				point_x = (x+width/2)*5/2000 + 1;
				point_y = (y+height/2)*5/2000 + 1;

                x = x*outWidth/2000;
                y = y*outHeight/2000;
                width = width*outWidth/2000;
                height = height*outHeight/2000;

                x = (x<16)? 16 : x;
                y = (y<16)? 16 : y;
                width = (width<(outWidth/4))?(outWidth/4):width;
                height = (height<(outHeight/4))?(outHeight/4):height; 

                if ((x+width) > (outWidth-16))
                    x = outWidth-16-width;
                if ((y+height) > (outHeight-16))
                    y = outHeight-16-height;

                hist_x = x;
                hist_y = y;
                hist_w = width;
                hist_h = height;
            } else {
                hist_x = x = pCamEngineCtx->outWindow.hOffset;
                hist_y = y = pCamEngineCtx->outWindow.vOffset;
                width = ( (4096-176) > pCamEngineCtx->outWindow.width)  ? pCamEngineCtx->outWindow.width : (4096-177);
                height = ( (3072-140) > pCamEngineCtx->outWindow.height) ? pCamEngineCtx->outWindow.height: (3072-141);//FIXME
                hist_w = pCamEngineCtx->outWindow.width;
                hist_h = pCamEngineCtx->outWindow.height;
				CamerIcIspHistSetMeasuringWindow(pCamerIcHandle, hist_x, hist_y, hist_w, hist_h);
				CamerIcIspExpSetMeasuringWindow(pCamerIcHandle, x, y, width, height);
            }

            lres = CamerIcIspExpGetMeasuringWindow(pCamerIcHandle,(CamerIcWindow_t*)&curAeWin,NULL);
            if (lres == RET_SUCCESS) {
				#if 0
                if ( (x!=curAeWin.hOffset) ||
                     (y!=curAeWin.vOffset) ||
                     (width!=curAeWin.width) ||
                     (height!=curAeWin.height) )
                #endif
				{

                    CamerIcIspExpDisable(pCamerIcHandle);
                    CamerIcIspHistDisable(pCamerIcHandle);
                    AecStop(pCamEngineCtx->hAec);

                    //CamerIcIspHistSetMeasuringWindow(pCamerIcHandle, hist_x, hist_y, hist_w, hist_h);
                    //CamerIcIspExpSetMeasuringWindow(pCamerIcHandle, x, y, width, height); 
                    if (mode == AverageMetering) {
						//CamerIcIspHistSetMeasuringWindow(pCamerIcHandle, hist_x, hist_y, hist_w, hist_h);
						//CamerIcIspExpSetMeasuringWindow(pCamerIcHandle, x, y, width, height);
                        memset(gridWeights,0x01,sizeof(gridWeights));
                        CamerIcIspHistSetGridWeights(pCamerIcHandle, gridWeights);
                        AecSetMeanLumaGridWeights(pCamEngineCtx->hAec, gridWeights);
                    } else if (mode == CentreWeightMetering) {
                    	AecGetGridWeights(pCamEngineCtx->hAec, gridWeights);
                        CamerIcIspHistSetGridWeights(pCamerIcHandle, gridWeights);
                        AecSetMeanLumaGridWeights(pCamEngineCtx->hAec, gridWeights);
                    }
					else if (mode == AfWeightMetering){
						//add by ethan 2016-01-20
						TRACE( CAM_ENGINE_API_ERROR, "%s entering AfWeightMetering \n", __FUNCTION__);

						#if 0
						hist_x = x = pCamEngineCtx->outWindow.hOffset;
						hist_y = y = pCamEngineCtx->outWindow.vOffset;
						width = ( (4096-176) > pCamEngineCtx->outWindow.width)  ? pCamEngineCtx->outWindow.width : (4096-177);
						height = ( (3072-140) > pCamEngineCtx->outWindow.height) ? pCamEngineCtx->outWindow.height: (3072-141);//FIXME
						hist_w = pCamEngineCtx->outWindow.width;
						hist_h = pCamEngineCtx->outWindow.height;
						memset(gridWeights,0x1,sizeof(gridWeights));
						CamerIcIspHistSetMeasuringWindow(pCamerIcHandle, hist_x, hist_y, hist_w, hist_h);
						CamerIcIspExpSetMeasuringWindow(pCamerIcHandle, x, y, width, height);
						#endif

						#if 1
						memset(gridWeights,0x1,sizeof(gridWeights));

                        if (point_x>1)
							gridWeights[(point_x-2)+(point_y-1)*5] = 0x6;

						if (point_y>1)
							gridWeights[(point_x-1)+(point_y-2)*5] = 0x6;

						if (point_x<5)
							gridWeights[point_x+(point_y-1)*5] = 0x6;

						if (point_y<5)
							gridWeights[point_x-1+point_y*5] = 0x6;

						if ((point_x>1)&&(point_y>1))
							gridWeights[(point_x-2)+(point_y-2)*5] = 0x4;

						if ((point_x<5)&&(point_y>1))
							gridWeights[point_x+(point_y-2)*5] = 0x4;

						if ((point_x>1)&&(point_y<5))
							gridWeights[point_x-2+point_y*5] = 0x4;

						if ((point_x<5)&&(point_y<5))
							gridWeights[point_x+point_y*5] = 0x4;

						gridWeights[point_x-1+(point_y-1)*5] = 0x10;
						#endif
						CamerIcIspHistSetGridWeights(pCamerIcHandle, gridWeights);
                        AecSetMeanLumaGridWeights(pCamEngineCtx->hAec, gridWeights);
					}

                    CamerIcIspHistEnable(pCamerIcHandle);
                    CamerIcIspExpEnable(pCamerIcHandle);
                    AecStart(pCamEngineCtx->hAec);

                }
            } else {
                TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIcIspExpGetMeasuringWindow failed!",__FUNCTION__ );
                result = lres;
            }
            
        }

    } else {
        TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIcIspExpIsEnabled failed!",__FUNCTION__ );
        result = lres;
    }
    

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    return result;

}


/******************************************************************************
 * CamEngineAfAvailable()
 *****************************************************************************/
RESULT CamEngineAfAvailable
(
    CamEngineHandle_t       hCamEngine,
    bool_t                  *pAvailable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pAvailable == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pAvailable = ((BOOL_TRUE == pCamEngineCtx->availableAf) && (NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor) && (NULL != pCamEngineCtx->hAf));

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAfStart()
 *****************************************************************************/
RESULT CamEngineAfStart
(
    CamEngineHandle_t                    hCamEngine,
    const CamEngineAfSearchAlgorithm_t   searchAgoritm
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    AfSearchStrategy_t Afss = AFM_FSS_INVALID;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if ( (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    switch ( searchAgoritm )
    {
        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_FULL_RANGE:
            {
                Afss = AFM_FSS_FULLRANGE;
                break;
            }

        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE:
            {
                Afss = AFM_FSS_ADAPTIVE_RANGE;
                break;
            }

        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_HILL_CLIMBING:
            {
                Afss = AFM_FSS_HILLCLIMBING;
                break;
            }

        default:
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s invalid auto-focus search function\n", __FUNCTION__ );
                return ( RET_INVALID_PARM );
            }
    }

    result = AfStart( pCamEngineCtx->hAf, Afss );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAfOneShot()
 *****************************************************************************/
RESULT CamEngineAfOneShot
(
    CamEngineHandle_t                    hCamEngine,
    const CamEngineAfSearchAlgorithm_t   searchAgoritm
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    AfSearchStrategy_t Afss = AFM_FSS_INVALID;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if (  ( BOOL_TRUE != pCamEngineCtx->availableAf ) ||
          ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) ||
          ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    switch ( searchAgoritm )
    {
        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_FULL_RANGE:
            {
                Afss = AFM_FSS_FULLRANGE;
                break;
            }

        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE:
            {
                Afss = AFM_FSS_ADAPTIVE_RANGE;
                break;
            }

        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_HILL_CLIMBING:
            {
                Afss = AFM_FSS_HILLCLIMBING;
                break;
            }

        default:
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s invalid auto-focus search function\n", __FUNCTION__ );
                return ( RET_INVALID_PARM );
            }
    }

    result = AfOneShot( pCamEngineCtx->hAf, Afss );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}




/******************************************************************************
 * CamEngineAfStop()
 *****************************************************************************/
RESULT CamEngineAfStop
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if (  (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AfStop( pCamEngineCtx->hAf );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAfStatus()
 *****************************************************************************/
RESULT CamEngineAfStatus
(
    CamEngineHandle_t               hCamEngine,
    bool_t                          *pRunning,
    CamEngineAfSearchAlgorithm_t    *pSearchAgoritm,
    float                           *sharpness
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    AfSearchStrategy_t Afss = AFM_FSS_INVALID;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pRunning == NULL) || (pSearchAgoritm == NULL))
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if (  (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AfStatus( pCamEngineCtx->hAf, pRunning, &Afss, sharpness );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    switch ( Afss )
    {
        case AFM_FSS_FULLRANGE:
            {
                *pSearchAgoritm = CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_FULL_RANGE;
                break;
            }

        case AFM_FSS_ADAPTIVE_RANGE:
            {
                *pSearchAgoritm = CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE;
                break;
            }

        case AFM_FSS_HILLCLIMBING:
            {
                *pSearchAgoritm = CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_HILL_CLIMBING;
                break;
            }

        default:
            {
                *pSearchAgoritm = CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_INVALID;
                return ( RET_FAILURE );
            }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineAfShotCheck()
 *****************************************************************************/
RESULT CamEngineAfShotCheck
(
    CamEngineHandle_t               hCamEngine,
    bool_t                          *shot
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    AfSearchStrategy_t Afss = AFM_FSS_INVALID;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (shot == NULL))
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if (  (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    result = AfShotCheck( pCamEngineCtx->hAf, shot );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }    

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

/******************************************************************************
 * CamEngineAfRegisterEvtQue()
 *****************************************************************************/
RESULT CamEngineAfRegisterEvtQue
(
    CamEngineHandle_t               hCamEngine,
    CamEngineAfEvtQue_t             *evtQue
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    AfSearchStrategy_t Afss = AFM_FSS_INVALID;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (evtQue == NULL))
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if (  (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }


    result = AfRegisterEvtQue( pCamEngineCtx->hAf, (AfEvtQue_t*)evtQue);
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }    

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );

}

/******************************************************************************
 * CamEngineAfReset()
 *****************************************************************************/
RESULT CamEngineAfReset
(
    CamEngineHandle_t                    hCamEngine,
    const CamEngineAfSearchAlgorithm_t   searchAgoritm
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    AfSearchStrategy_t Afss = AFM_FSS_INVALID;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if af available with configured sensor */
    if ( (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    switch ( searchAgoritm )
    {
        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_FULL_RANGE:
            {
                Afss = AFM_FSS_FULLRANGE;
                break;
            }

        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_ADAPTIVE_RANGE:
            {
                Afss = AFM_FSS_ADAPTIVE_RANGE;
                break;
            }

        case CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_HILL_CLIMBING:
            {
                Afss = AFM_FSS_HILLCLIMBING;
                break;
            }

        default:
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s invalid auto-focus search function\n", __FUNCTION__ );
                return ( RET_INVALID_PARM );
            }
    }

    result = AfReset( pCamEngineCtx->hAf, Afss );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

/******************************************************************************
 * CamEngineAfGetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAfGetMeasuringWindow
(
    CamEngineHandle_t               hCamEngine,
    CamEngineWindow_t               *pWindow
)
{
    RESULT lres = RET_SUCCESS;
    bool_t isEnabled = BOOL_FALSE;
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
    CamerIcDrvHandle_t pCamerIcHandle;
    CamerIcIspAfmWindowId_t WdwId=CAMERIC_ISP_AFM_WINDOW_INVALID;
    
    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if af available with configured sensor */
    if ( (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    pCamerIcHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    lres = CamerIcIspAfmIsEnabled( pCamerIcHandle, &isEnabled );
    if ( RET_SUCCESS == lres ) {
        if (CamerIcIspAfmMeasuringWindowIsEnabled(pCamerIcHandle, CAMERIC_ISP_AFM_WINDOW_A) == BOOL_TRUE) {
            WdwId = CAMERIC_ISP_AFM_WINDOW_A;
        } else if (CamerIcIspAfmMeasuringWindowIsEnabled(pCamerIcHandle, CAMERIC_ISP_AFM_WINDOW_B) == BOOL_TRUE) {
            WdwId = CAMERIC_ISP_AFM_WINDOW_B;
        } else if (CamerIcIspAfmMeasuringWindowIsEnabled(pCamerIcHandle, CAMERIC_ISP_AFM_WINDOW_C) == BOOL_TRUE) {
            WdwId = CAMERIC_ISP_AFM_WINDOW_C;
        }

        if (WdwId == CAMERIC_ISP_AFM_WINDOW_INVALID) {
            TRACE( CAM_ENGINE_API_ERROR, "%s: Is not measure window is validate!",__FUNCTION__ );
        } else {            
            lres = CamerIcIspAfmGetMeasuringWindow(pCamerIcHandle, WdwId, (CamerIcWindow_t*)pWindow);
        }
    } else {
        TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIcIspAfmIsEnabled failed!",__FUNCTION__ );
    }
    
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    return lres;
    
}

/******************************************************************************
 * CamEngineAfSetMeasuringWindow()
 *****************************************************************************/
RESULT CamEngineAfSetMeasuringWindow
(   
    CamEngineHandle_t               hCamEngine,
    int16_t                  x,
    int16_t                  y,
    uint16_t                  width,
    uint16_t                  height
)
{
    RESULT result = RET_SUCCESS, lres;
    bool_t isEnabled = BOOL_FALSE;
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;
    CamerIcDrvHandle_t pCamerIcHandle;
    CamerIcIspAfmWindowId_t WdwId=CAMERIC_ISP_AFM_WINDOW_INVALID;
    
    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    
    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if af available with configured sensor */
    if ( (BOOL_TRUE != pCamEngineCtx->availableAf) || ( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor ) || ( NULL == pCamEngineCtx->hAf ) )
    {
        return ( RET_NOTSUPP );
    }

    isEnabled = BOOL_FALSE;
    pCamerIcHandle = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    lres = CamerIcIspAfmIsEnabled( pCamerIcHandle, &isEnabled );
    if ( RET_SUCCESS == lres )
    {        
        if ( BOOL_TRUE == isEnabled ) {

            if (CamerIcIspAfmMeasuringWindowIsEnabled(pCamerIcHandle, CAMERIC_ISP_AFM_WINDOW_A) == BOOL_TRUE) {
                WdwId = CAMERIC_ISP_AFM_WINDOW_A;
            } else if (CamerIcIspAfmMeasuringWindowIsEnabled(pCamerIcHandle, CAMERIC_ISP_AFM_WINDOW_B) == BOOL_TRUE) {
                WdwId = CAMERIC_ISP_AFM_WINDOW_B;
            } else if (CamerIcIspAfmMeasuringWindowIsEnabled(pCamerIcHandle, CAMERIC_ISP_AFM_WINDOW_C) == BOOL_TRUE) {
                WdwId = CAMERIC_ISP_AFM_WINDOW_C;
            }

            if (WdwId == CAMERIC_ISP_AFM_WINDOW_INVALID) {
                TRACE( CAM_ENGINE_API_ERROR, "%s: Is not measure window is validate!",__FUNCTION__ );
            } else {

                uint16_t outWidth  = ( pCamEngineCtx->outWindow.width -  pCamEngineCtx->outWindow.hOffset );
                uint16_t outHeight = ( pCamEngineCtx->outWindow.height - pCamEngineCtx->outWindow.vOffset );
                CamerIcWindow_t curAfWin; 

                if (x || y || width || height) {
                    x += 1000;
                    y += 1000;

                    x = x*outWidth/2000;
                    y = y*outHeight/2000;
                    width = width*outWidth/2000;
                    height = height*outHeight/2000;
                } else {
                    width  = ( outWidth  >= 200U ) ? 200U : outWidth;
                    height = ( outHeight >= 200U ) ? 200U : outHeight;
            
                    x = ( outWidth  >> 1 ) - ( width  >> 1 );
                    y = ( outHeight >> 1 ) - ( height >> 1 );
                }
                lres = CamerIcIspAfmGetMeasuringWindow(pCamerIcHandle, WdwId, &curAfWin);
                if (lres != RET_SUCCESS) {
                    TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIcIspAfmGetMeasuringWindow failed!",__FUNCTION__ );
                } else {                
                    if ( (x != curAfWin.hOffset) ||
                         (y != curAfWin.vOffset) ||
                         (width != curAfWin.width) ||
                         (height != curAfWin.height) ) {
                         
                        CamerIcIspAfmDisable(pCamerIcHandle);
                        CamerIcIspAfmDisableMeasuringWindow(pCamerIcHandle, WdwId);
                        CamerIcIspAfmSetMeasuringWindow(pCamerIcHandle, WdwId,x,y,width,height);
                        CamerIcIspAfmEnableMeasuringWindow(pCamerIcHandle, WdwId);
                        CamerIcIspAfmEnable(pCamerIcHandle);
                    }
                }
            }
        }
    } else {
        TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIcIspAfmIsEnabled failed!",__FUNCTION__ );
        result = lres;
    }
    

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    return result;
}
/******************************************************************************
 * CamEngineAdpfStart()
 *****************************************************************************/
RESULT CamEngineAdpfStart
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if adpf available */
    if ( NULL == pCamEngineCtx->hAdpf )
    {
        return ( RET_NOTSUPP );
    }

    result = AdpfStart( pCamEngineCtx->hAdpf );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAdpfStop()
 *****************************************************************************/
RESULT CamEngineAdpfStop
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if adpf available */
    if ( NULL == pCamEngineCtx->hAdpf )
    {
        return ( RET_NOTSUPP );
    }

    result = AdpfStop( pCamEngineCtx->hAdpf );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

/******************************************************************************
 * CamEnginegetMfdgain()
 *****************************************************************************/
RESULT CamEnginegetMfdgain
(
    CamEngineHandle_t           hCamEngine,
	char						*mfd_enable,
	float						mfd_gain[],
	float						mfd_frames[]

)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    /* check if adpf available */
    if ( NULL == pCamEngineCtx->hAdpf )
    {
        return ( RET_NOTSUPP );
    }

	AdpfGetMfdGain(pCamEngineCtx->hAdpf, mfd_enable, mfd_gain, mfd_frames);
	return ( RET_SUCCESS );
}

/******************************************************************************
 * CamEnginegetUvnrpara()
 *****************************************************************************/
RESULT CamEnginegetUvnrpara
(
    CamEngineHandle_t           hCamEngine,
	char						*uvnr_enable,
	float						uvnr_gain[],
	float						uvnr_ratio[],
	float						uvnr_distances[]

)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );
    /* check if adpf available */
    if ( NULL == pCamEngineCtx->hAdpf )
    {
        return ( RET_NOTSUPP );
    }

	AdpfGetUvnrPara(pCamEngineCtx->hAdpf, uvnr_enable, uvnr_gain, uvnr_ratio,uvnr_distances);
	return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineAdpfConfigure()
 *****************************************************************************/
RESULT CamEngineAdpfConfigure
(
    CamEngineHandle_t           hCamEngine,
    const float                 gradient,
    const float                 offset,
    const float                 min,
    const float                 div,
    const uint8_t               sigmaGreen,
    const uint8_t               sigmaRedBlue
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if aec available */
    if ( NULL == pCamEngineCtx->hAdpf )
    {
        return ( RET_NOTSUPP );
    }

    AdpfConfig_t AdpfConfig;

    /* get current config */
    result = AdpfGetCurrentConfig( pCamEngineCtx->hAdpf, &AdpfConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    AdpfConfig.fSensorGain            = 1.0f;

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        AdpfConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
        AdpfConfig.hSubCamerIc  = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
    }
    else
    {
        AdpfConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
        AdpfConfig.hSubCamerIc  = NULL;
    }

    AdpfConfig.data.def.SigmaGreen    = sigmaGreen;
    AdpfConfig.data.def.SigmaRedBlue  = sigmaRedBlue;
    AdpfConfig.data.def.fGradient     = gradient;
    AdpfConfig.data.def.fOffset       = offset;
    AdpfConfig.data.def.fMin          = min;
    AdpfConfig.data.def.fDiv          = div;

    /* configure */
    result = AdpfReConfigure( pCamEngineCtx->hAdpf, &AdpfConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineAdpfStatus()
 *****************************************************************************/
RESULT CamEngineAdpfStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pRunning,
    float               *pGradient,
    float               *pOffset,
    float               *pMin,
    float               *pDiv,
    uint8_t             *pSigmaGreen,
    uint8_t             *pSigmaRedBlue
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRunning == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    if ( (pRunning == NULL) || (pGradient == NULL) || (pOffset == NULL) || (pSigmaGreen == NULL) || (pSigmaRedBlue == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if adpf available */
    if ( NULL == pCamEngineCtx->hAdpf )
    {
        return ( RET_NOTSUPP );
    }

    result = AdpfStatus( pCamEngineCtx->hAdpf, pRunning );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    AdpfConfig_t AdpfConfig;

    /* get current config */
    result = AdpfGetCurrentConfig( pCamEngineCtx->hAdpf, &AdpfConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    *pSigmaGreen   = AdpfConfig.data.def.SigmaGreen;
    *pSigmaRedBlue = AdpfConfig.data.def.SigmaRedBlue;
    *pGradient     = AdpfConfig.data.def.fGradient;
    *pOffset       = AdpfConfig.data.def.fOffset;
    *pMin          = AdpfConfig.data.def.fMin;
    *pDiv          = AdpfConfig.data.def.fDiv;

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAdpccStart()
 *****************************************************************************/
RESULT CamEngineAdpccStart
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if adpcc available */
    if ( NULL == pCamEngineCtx->hAdpcc )
    {
        return ( RET_NOTSUPP );
    }

    result = AdpccStart( pCamEngineCtx->hAdpcc );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAdpccStop()
 *****************************************************************************/
RESULT CamEngineAdpccStop
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if adpcc available */
    if ( NULL == pCamEngineCtx->hAdpcc )
    {
        return ( RET_NOTSUPP );
    }

    result = AdpccStop( pCamEngineCtx->hAdpcc );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAdpccStatus()
 *****************************************************************************/
RESULT CamEngineAdpccStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pRunning
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRunning == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* check if adpcc available */
    if ( NULL == pCamEngineCtx->hAdpcc )
    {
        return ( RET_NOTSUPP );
    }

    result = AdpccStatus( pCamEngineCtx->hAdpcc, pRunning );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}




/******************************************************************************
 * CamEngineAvsStart()
 *****************************************************************************/
RESULT CamEngineAvsStart
(
    CamEngineHandle_t  hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        if ( pCamEngineCtx == NULL )
        {
            return ( RET_WRONG_HANDLE );
        }

        /* check if cam-engine running */
        if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
             ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
        {
            return ( RET_WRONG_STATE );
        }

        /* check if avs available  */
        if ( NULL == pCamEngineCtx->hAvs )
        {
            return ( RET_NOTSUPP );
        }

        result = AvsStart( pCamEngineCtx->hAvs );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAvsStop()
 *****************************************************************************/
RESULT CamEngineAvsStop
(
    CamEngineHandle_t hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        if ( pCamEngineCtx == NULL )
        {
            return ( RET_WRONG_HANDLE );
        }

        /* check if cam-engine running */
        if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
             ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
        {
            return ( RET_WRONG_STATE );
        }

        /* check if avs available */
        if ( NULL == pCamEngineCtx->hAvs )
        {
            return ( RET_NOTSUPP );
        }

        result = AvsStop( pCamEngineCtx->hAvs );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAvsConfigure()
 *****************************************************************************/
RESULT CamEngineAvsConfigure
(
    CamEngineHandle_t   hCamEngine,
    const bool          useParams,
    const uint16_t      numItpPoints,
    const float         theta,
    const float         baseGain,
    const float         fallOff,
    const float         acceleration
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        if ( pCamEngineCtx == NULL )
        {
            return ( RET_WRONG_HANDLE );
        }

        /* check if cam-engine running */
        if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
             ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
        {
            return ( RET_WRONG_STATE );
        }

        /* check if avs available  */
        if ( NULL == pCamEngineCtx->hAvs )
        {
            return ( RET_NOTSUPP );
        }

        {
            if ( useParams )
            {
                AvsDampFuncParams_t *pDampParams = &pCamEngineCtx->avsDampParams;

                MEMSET( pDampParams, 0, sizeof(AvsDampFuncParams_t) );
                pDampParams->numItpPoints = numItpPoints;
                pDampParams->theta        = theta;
                pDampParams->baseGain     = baseGain;
                pDampParams->fallOff      = fallOff;
                pDampParams->acceleration = acceleration;

                pCamEngineCtx->avsConfig.pDampParamsHor = pDampParams;
                pCamEngineCtx->avsConfig.pDampParamsVer = pDampParams;
            }
            else
            {
                pCamEngineCtx->avsConfig.pDampParamsHor = NULL;
                pCamEngineCtx->avsConfig.pDampParamsVer = NULL;
            }
            
            pCamEngineCtx->avsConfig.hCamerIc    = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            pCamEngineCtx->avsConfig.hSubCamerIc = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;

            pCamEngineCtx->avsConfig.dampLutHorSize = 0;
            pCamEngineCtx->avsConfig.dampLutVerSize = 0;
            pCamEngineCtx->avsConfig.pDampLutHor = NULL;
            pCamEngineCtx->avsConfig.pDampLutVer = NULL;

            /* leave the window configurations as they are */

            result = AvsReConfigure( pCamEngineCtx->hAvs, &pCamEngineCtx->avsConfig );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s: Re-Configuration of AVS failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAvsGetConfig()
 *****************************************************************************/
RESULT CamEngineAvsGetConfig
(
    CamEngineHandle_t   hCamEngine,
    bool_t              *pUsingParams,
    uint16_t            *pNumItpPoints,
    float               *pTheta,
    float               *pBaseGain,
    float               *pFallOff,
    float               *pAcceleration,
    int                 *pNumDampData,
    double             **ppDampXData,
    double             **ppDampYData
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        if ( pCamEngineCtx == NULL )
        {
            return ( RET_WRONG_HANDLE );
        }

        /* check if cam-engine running */
        if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
             ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
        {
            return ( RET_WRONG_STATE );
        }

        /* check if avs available  */
        if ( NULL == pCamEngineCtx->hAvs )
        {
            return ( RET_NOTSUPP );
        }

        {
            AvsConfig_t config;
            AvsDampFuncParams_t* pParams;
            uint16_t i;

            MEMSET( &config, 0, sizeof(config) );
            result = AvsGetConfig( pCamEngineCtx->hAvs, &config );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_API_ERROR, "%s: Get config from AVS failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            /* For now, take horizontal parameters. It actually does not matter,
             * since we are configuring the same parameters for both cases anyway.
             */
            pParams = config.pDampParamsHor;
            *pUsingParams = ( pParams == NULL ) ? BOOL_FALSE : BOOL_TRUE;
            if ( pParams )
            {
                *pNumItpPoints = pParams->numItpPoints;
                *pTheta        = pParams->theta;
                *pBaseGain     = pParams->baseGain;
                *pFallOff      = pParams->fallOff;
                *pAcceleration = pParams->acceleration;
            }

            /* Transform lookup table to a form which can be used to
             * pass to damp data out parameters.  */
            if ( pParams->numItpPoints + 1 > pCamEngineCtx->numDampData )
            {
                pCamEngineCtx->numDampData = pParams->numItpPoints + 1;

                if ( pCamEngineCtx->pDampXData != NULL )
                {
                    free( pCamEngineCtx->pDampXData );
                }
                if ( pCamEngineCtx->pDampYData != NULL )
                {
                    free( pCamEngineCtx->pDampYData );
                }

                pCamEngineCtx->pDampXData =
                    malloc( pCamEngineCtx->numDampData * sizeof(double) );
                if ( pCamEngineCtx->pDampXData == NULL )
                {
                    pCamEngineCtx->numDampData = 0;
                    return ( RET_OUTOFMEM );
                }
                pCamEngineCtx->pDampYData =
                    malloc( pCamEngineCtx->numDampData * sizeof(double) );
                if ( pCamEngineCtx->pDampYData == NULL )
                {
                    pCamEngineCtx->numDampData = 0;
                    free( pCamEngineCtx->pDampXData );
                    return ( RET_OUTOFMEM );
                }
            }


            pCamEngineCtx->pDampXData[0] = 0.0;
            if ( pParams->numItpPoints > 0 )
            {
                pCamEngineCtx->pDampYData[0] = config.pDampLutHor[0].value;
            }
            else
            {
                pCamEngineCtx->pDampYData[0] = 1.0;
            }
            for ( i = 0; i < pParams->numItpPoints; i++ )
            {
                pCamEngineCtx->pDampXData[i+1] = config.pDampLutHor[i].offset;
                pCamEngineCtx->pDampYData[i+1] = config.pDampLutHor[i].value;
            }

            *pNumDampData = pCamEngineCtx->numDampData;
            *ppDampXData  = pCamEngineCtx->pDampXData;
            *ppDampYData  = pCamEngineCtx->pDampYData;
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineAvsGetStatus()
 *****************************************************************************/
RESULT CamEngineAvsGetStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t             *pRunning,
    CamEngineVector_t  *pCurrDisplVec,
    CamEngineVector_t  *pCurrOffsetVec
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        if ( pCamEngineCtx == NULL )
        {
            return ( RET_WRONG_HANDLE );
        }

        if ( (pRunning == NULL) || (pCurrDisplVec == NULL) ||
             (pCurrOffsetVec == NULL) )
        {
            return ( RET_INVALID_PARM );
        }

        /* check if cam-engine running */
        if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
             ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
        {
            return ( RET_WRONG_STATE );
        }

        /* check if avs available */
        if ( NULL == pCamEngineCtx->hAvs )
        {
            return ( RET_NOTSUPP );
        }

        result = AvsGetStatus( pCamEngineCtx->hAvs, pRunning, pCurrDisplVec, pCurrOffsetVec );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

RESULT CamEngineAwbGetinfo
(
	CamEngineHandle_t hCamEngine,
	float *f_RgProj, 
	float *f_s, 
	float *f_s_Max1, 
	float *f_s_Max2, 
	float *f_Bg1, 
	float *f_Rg1, 
	float *f_Bg2, 
	float *f_Rg2
)
{
	CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

	AwbGetGainParam(pCamEngineCtx->hAwb, f_RgProj,f_s, f_s_Max1, f_s_Max2, f_Bg1, f_Rg1, f_Bg2, f_Rg2);
	return ( RET_SUCCESS );
}

RESULT CamEngineAwbGetIlluEstInfo
(
	CamEngineHandle_t hCamEngine,
	float *ExpPriorIn,
	float *ExpPriorOut,
	char (*name)[20],
	float likehood[],
	float wight[],
	int *curIdx,
	int *region,
	int *count
)
{
	CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

	return AwbGetIlluEstInfo(pCamEngineCtx->hAwb, ExpPriorIn, ExpPriorOut, name, likehood, wight, curIdx, region, count);
}
