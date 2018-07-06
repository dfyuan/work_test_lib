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
 * @file cam_engine_modules.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <aec/aec.h>
#include <awb/awb.h>
#include <af/af.h>
#include <adpf/adpf.h>
#include <adpcc/adpcc.h>

#include "cam_engine.h"
#include "cam_engine_aaa_api.h"
#include "cam_engine_modules.h"
#include "cam_engine_subctrls.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( CAM_ENGINE_INFO  );
USE_TRACER( CAM_ENGINE_WARN  );
USE_TRACER( CAM_ENGINE_ERROR );
USE_TRACER( CAM_ENGINE_DEBUG );

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
const CamerIcAwbMeasuringConfig_t MeasConfig =
{
    .MaxY           = 200U,
    .RefCr_MaxR     = 128U,
    .MinY_MaxG      =  30U,
    .RefCb_MaxB     = 128U,
    .MaxCSum        =  20U,
    .MinC           =  20U
};



/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * local function
 *****************************************************************************/


/******************************************************************************
 * CamEngineAecResChangeCb
 *****************************************************************************/
static void CamEngineAecResChangeCb
(
    void        *pPrivateContext,               /**< reference to user context as handed in via AecInstanceConfig_t */
    uint32_t    NewResolution                   /**< new resolution to switch to */
)
{
    CamEngineContext_t  *pCamEngineCtx = (CamEngineContext_t*) pPrivateContext;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if (pCamEngineCtx->cbAfpsResChange != NULL)
    {
        TRACE( CAM_ENGINE_INFO, "%s: calling cbAfpsResChange()\n", __FUNCTION__ );
        pCamEngineCtx->cbAfpsResChange( NewResolution, pCamEngineCtx->pUserCbCtx );
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return;
}



/******************************************************************************
 * CamEngineInitAec
 *****************************************************************************/
RESULT CamEngineInitAec
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AecInstanceConfig_t AecInstConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* setup config */
    MEMSET( &AecInstConfig, 0, sizeof(AecInstConfig) );
    AecInstConfig.pResChangeCbFunc    = CamEngineAecResChangeCb;
    AecInstConfig.pResChangeCbContext = (void*) pCamEngineCtx;

    /* init */
    result = AecInit( &AecInstConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Initializaion of AEC failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* return handle */
    pCamEngineCtx->hAec = AecInstConfig.hAec;

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAec
 *****************************************************************************/
RESULT CamEngineReleaseAec
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( NULL != pCamEngineCtx->hAec )
    {
        RESULT lres;

        lres = AecStop( pCamEngineCtx->hAec );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop AEC (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        lres = AecRelease( pCamEngineCtx->hAec );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release AEC (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        pCamEngineCtx->hAec = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineConfAec
 *****************************************************************************/
RESULT CamEngineConfAec
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor == NULL )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAec )
    {
        AecConfig_t AecConfig;

        /* setup config */
        MEMSET( &AecConfig, 0, sizeof(AecConfig) );

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            AecConfig.hSensor       = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor;
            AecConfig.hSubSensor    = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor;
        }
        else
        {
            AecConfig.hSensor       = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor;
            AecConfig.hSubSensor    = NULL;
        }

        AecConfig.hCamCalibDb       = pCamEngineCtx->hCamCalibDb;
        AecConfig.SemMode           = AEC_SCENE_EVALUATION_DISABLED;
        // AecConfig.SemMode        = AEC_SCENE_EVALUATION_ADAPTIVE;
        // AecConfig.SemMode        = AEC_SCENE_EVALUATION_FIX;
        AecConfig.SetPoint          = 92.0f;
        AecConfig.ClmTolerance      = 7.0f;
        AecConfig.DampingMode       = AEC_DAMPING_MODE_STILL_IMAGE;
        AecConfig.DampOverStill     = 0.2f;
        AecConfig.DampUnderStill    = 0.3f;
        AecConfig.DampOverVideo     = 0.6f;
        AecConfig.DampUnderVideo    = 0.9f;

        // this value must be user selected; we don't support automatic flicker detection
        switch( pCamEngineCtx->flickerPeriod )
        {
            default:
                TRACE( CAM_ENGINE_ERROR, "%s: Invalid flicker period; defaulting to CAM_ENGINE_FLICKER_OFF\n", __FUNCTION__ );
                // fall through!
            case CAM_ENGINE_FLICKER_OFF  : AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_OFF  ; break;
            case CAM_ENGINE_FLICKER_100HZ: AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_100HZ; break;
            case CAM_ENGINE_FLICKER_120HZ: AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_120HZ; break;
        }

        ////TODO: take these ECM values from CamCalibDB
        ////                               // fast  normal  slow ( @100Hz )
        //AecConfig.EcmT0fac      = 1.0  ; // 1.0    1.0    2.0 ( EcmT0 = EcmT0fac * EcmTflicker )
        //AecConfig.EcmA0         = 1.0;   // 2.0    1.0    1.0

        // this value is user selected
        AecConfig.AfpsEnabled = pCamEngineCtx->enableAfps;
        AecConfig.AfpsMaxGain = 10;

        /* configure */
        result = AecConfigure( pCamEngineCtx->hAec, &AecConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuration of AEC failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReConfAec
 *****************************************************************************/
RESULT CamEngineReConfAec
(
    CamEngineContext_t  *pCamEngineCtx,
    uint32_t            *pNumFramesToSkip
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor == NULL )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAec )
    {
        AecConfig_t AecConfig;

        /* get current config */
        result = AecGetCurrentConfig( pCamEngineCtx->hAec, &AecConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Retrieving of current AEC configuration failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        /* update */
        switch( pCamEngineCtx->flickerPeriod )
        {
            default:
                TRACE( CAM_ENGINE_ERROR, "%s: Invalid flicker period; defaulting to CAM_ENGINE_FLICKER_OFF\n", __FUNCTION__ );
                // fall through!
            case CAM_ENGINE_FLICKER_OFF  : AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_OFF  ; break;
            case CAM_ENGINE_FLICKER_100HZ: AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_100HZ; break;
            case CAM_ENGINE_FLICKER_120HZ: AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_120HZ; break;
        }
        AecConfig.AfpsEnabled = pCamEngineCtx->enableAfps;

        /* re-configure */
        result = AecReConfigure( pCamEngineCtx->hAec, &AecConfig, pNumFramesToSkip );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-configuration of AEC failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}
/******************************************************************************
 * CamEngineSetAecPoint
 *****************************************************************************/
RESULT CamEngineModulesSetAecPoint
(
	CamEngineContext_t	*pCamEngineCtx,
	float point
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor == NULL )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAec )
    {
        AecConfig_t AecConfig;

        /* get current config */
        result = AecGetCurrentConfig( pCamEngineCtx->hAec, &AecConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Retrieving of current AEC configuration failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        /* update */
        switch( pCamEngineCtx->flickerPeriod )
        {
            default:
                TRACE( CAM_ENGINE_ERROR, "%s: Invalid flicker period; defaulting to CAM_ENGINE_FLICKER_OFF\n", __FUNCTION__ );
                // fall through!
            case CAM_ENGINE_FLICKER_OFF  : AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_OFF  ; break;
            case CAM_ENGINE_FLICKER_100HZ: AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_100HZ; break;
            case CAM_ENGINE_FLICKER_120HZ: AecConfig.EcmFlickerSelect = AEC_EXPOSURE_CONVERSION_FLICKER_120HZ; break;
        }
        AecConfig.AfpsEnabled = pCamEngineCtx->enableAfps;
		
		AecConfig.SetPoint = point;
        /* re-configure */
		uint32_t numFramesToSkipModules = 0;
        result = AecReConfigure( pCamEngineCtx->hAec, &AecConfig, &numFramesToSkipModules );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-configuration of AEC failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );


}

/******************************************************************************
 * CamEngineInitAwb
 *****************************************************************************/
RESULT CamEngineInitAwb
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AwbInstanceConfig_t AwbInstConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* setup config */
    MEMSET( &AwbInstConfig, 0, sizeof(AwbInstConfig) );

    /* init */
    result = AwbInit( &AwbInstConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Initializaion of AWB failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* return handle */
    pCamEngineCtx->hAwb = AwbInstConfig.hAwb;

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineReleaseAwb
 *****************************************************************************/
RESULT CamEngineReleaseAwb
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( NULL != pCamEngineCtx->hAwb )
    {
        RESULT lres;

        lres = AwbStop( pCamEngineCtx->hAwb );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop AWB (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        lres = AwbRelease( pCamEngineCtx->hAwb );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release AWB (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        pCamEngineCtx->hAwb = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineConfAwb
 *****************************************************************************/
RESULT CamEngineConfAwb
(
    CamEngineContext_t                  *pCamEngineCtx,
    const CamerIcAwbMeasuringConfig_t   *pMeasConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMeasConfig == NULL )
    {
        return RET_NULL_POINTER;
    }

    if ( NULL == pCamEngineCtx->hCamCalibDb )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAwb )
    {
        AwbConfig_t AwbConfig;
    
        // setup awb configuration structure
        MEMSET( &AwbConfig, 0, sizeof(AwbConfig) );

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            AwbConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AwbConfig.hSubCamerIc  = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        }
        else
        {
            AwbConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AwbConfig.hSubCamerIc  = NULL;
        }

        //AwbConfig.Mode       = AWB_MODE_MANUAL;
        AwbConfig.Mode              = AWB_MODE_AUTO;
        AwbConfig.width             = pCamEngineCtx->outWindow.width;
        AwbConfig.height            = pCamEngineCtx->outWindow.height;
        AwbConfig.framerate         = 0.0f;
        AwbConfig.hCamCalibDb       = pCamEngineCtx->hCamCalibDb;

        AwbConfig.fStableDeviation  = 0.1f;     // 10 %
        AwbConfig.fRestartDeviation = 0.2f;     // 20 %

        AwbConfig.MeasMode          = CAMERIC_ISP_AWB_MEASURING_MODE_YCBCR;
        AwbConfig.MeasConfig        = *pMeasConfig;
        AwbConfig.Flags             = (AWB_WORKING_FLAG_USE_DAMPING | AWB_WORKING_FLAG_USE_CC_OFFSET);

        /* configure */
        result = AwbConfigure( pCamEngineCtx->hAwb, &AwbConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuration of AWB failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReConfAwb
 *****************************************************************************/
RESULT CamEngineReConfAwb
(
    CamEngineContext_t                  *pCamEngineCtx,
    const CamerIcAwbMeasuringConfig_t   *pMeasConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pMeasConfig == NULL )
    {
        return RET_NULL_POINTER;
    }

    if ( NULL == pCamEngineCtx->hCamCalibDb )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAwb )
    {
        AwbConfig_t AwbConfig;

        /* setup config */
        MEMSET( &AwbConfig, 0, sizeof(AwbConfig) );
        
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            AwbConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AwbConfig.hSubCamerIc  = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        }
        else
        {
            AwbConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AwbConfig.hSubCamerIc  = NULL;
        }

        AwbConfig.Mode              = AWB_MODE_AUTO;
        AwbConfig.width             = pCamEngineCtx->outWindow.width;
        AwbConfig.height            = pCamEngineCtx->outWindow.height;
        AwbConfig.framerate         = 0;
        AwbConfig.hCamCalibDb       = pCamEngineCtx->hCamCalibDb;
        AwbConfig.fStableDeviation  = 0.1f;     // 10 %
        AwbConfig.fRestartDeviation = 0.2f;     // 20 %

        AwbConfig.MeasMode          = CAMERIC_ISP_AWB_MEASURING_MODE_YCBCR;
        AwbConfig.MeasConfig        = *pMeasConfig;
        AwbConfig.Flags             = (AWB_WORKING_FLAG_USE_DAMPING | AWB_WORKING_FLAG_USE_CC_OFFSET);

        /* re-configure */
        result = AwbReConfigure( pCamEngineCtx->hAwb, &AwbConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-configuration of AWB failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineInitAf
 *****************************************************************************/
RESULT CamEngineInitAf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AfInstanceConfig_t AfInstConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* setup config */
    MEMSET( &AfInstConfig, 0, sizeof(AfInstConfig) );

    /* init */
    result = AfInit( &AfInstConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Initializaion of AF failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* return handle */
    pCamEngineCtx->hAf = AfInstConfig.hAf;

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAf
 *****************************************************************************/
RESULT CamEngineReleaseAf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( NULL != pCamEngineCtx->hAf )
    {
        RESULT lres;

        lres = AfStop( pCamEngineCtx->hAf );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop AF (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        lres = AfRelease( pCamEngineCtx->hAf );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release AF (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT ( result, lres );
        }

        pCamEngineCtx->hAf = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineConfAf
 *****************************************************************************/
RESULT CamEngineConfAf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AfConfig_t AfConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor == NULL )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAf )
    {
        pCamEngineCtx->availableAf = BOOL_TRUE;

        // setup auto focus configuration
        MEMSET( &AfConfig, 0, sizeof(AfConfig) );

        AfConfig.hCamerIc = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            AfConfig.hSensor       = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor;
            AfConfig.hSubSensor    = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hSensor;
        }
        else
        {
            AfConfig.hSensor       = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor;
            AfConfig.hSubSensor    = NULL;
        }

        AfConfig.Afss    = AFM_FSS_ADAPTIVE_RANGE;
        
        /* configure */
        result = AfConfigure( pCamEngineCtx->hAf, &AfConfig );
        if ( result == RET_NOTSUPP )
        {
            pCamEngineCtx->availableAf = BOOL_FALSE;
            // sensor told us, there's no support for Af
            result = RET_SUCCESS;
        }
        else if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuration of AF failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReConfAf
 *****************************************************************************/
RESULT CamEngineReConfAf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hSensor == NULL )
    {
        return ( RET_NOTSUPP );
    }

    if ( NULL != pCamEngineCtx->hAf )
    {
        /* re-configure */
        result = AfReConfigure( pCamEngineCtx->hAf );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-configuration of AF failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineInitAdpf
 *****************************************************************************/
RESULT CamEngineInitAdpf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AdpfInstanceConfig_t AdpfInstConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* setup config */
    MEMSET( &AdpfInstConfig, 0, sizeof(AdpfInstConfig) );

    /* init */
    result = AdpfInit( &AdpfInstConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Initialization of ADPF failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* return handle */
    pCamEngineCtx->hAdpf = AdpfInstConfig.hAdpf;

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAdpf
 *****************************************************************************/
RESULT CamEngineReleaseAdpf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( NULL != pCamEngineCtx->hAdpf )
    {
        RESULT lres;

        lres = AdpfStop( pCamEngineCtx->hAdpf );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop ADPF (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        lres = AdpfRelease( pCamEngineCtx->hAdpf );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release ADPF (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        pCamEngineCtx->hAdpf = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineConfAdpf
 *****************************************************************************/
RESULT CamEngineConfAdpf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pCamEngineCtx->hAdpf )
    {
        AdpfConfig_t AdpfConfig;

        // setup config configuration structure
        MEMSET( &AdpfConfig, 0, sizeof(AdpfConfig) );

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

        if ( NULL != pCamEngineCtx->hCamCalibDb )
        {
            // calibration database is available => use it
            AdpfConfig.type                 = ADPF_USE_CALIB_DATABASE;
            AdpfConfig.fSensorGain          = 1.0f;

            AdpfConfig.data.db.width        = pCamEngineCtx->outWindow.width;
            AdpfConfig.data.db.height       = pCamEngineCtx->outWindow.height;
            AdpfConfig.data.db.framerate    = 0;
            AdpfConfig.data.db.hCamCalibDb  = pCamEngineCtx->hCamCalibDb;

        }
        else
        {
            // calibration database not available => use a static default configuration
            AdpfConfig.type                     = ADPF_USE_DEFAULT_CONFIG;
            AdpfConfig.fSensorGain              = 1.0f;

            AdpfConfig.data.def.SigmaGreen      = 4UL;
            AdpfConfig.data.def.SigmaRedBlue    = 4UL;
            AdpfConfig.data.def.fGradient       = 1.0f;
            AdpfConfig.data.def.fOffset         = 0.0f;
            AdpfConfig.data.def.NfGains.fRed    = 1.0f;
            AdpfConfig.data.def.NfGains.fGreenR = 1.0f;
            AdpfConfig.data.def.NfGains.fGreenB = 1.0f;
            AdpfConfig.data.def.NfGains.fBlue   = 1.0f;
        }

        // configure
        result = AdpfConfigure( pCamEngineCtx->hAdpf, &AdpfConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuration of ADPF failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReConfAdpf
 *****************************************************************************/
RESULT CamEngineReConfAdpf
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pCamEngineCtx->hAdpf )
    {
        AdpfConfig_t AdpfConfig;

        /* setup config */
        MEMSET( &AdpfConfig, 0, sizeof(AdpfConfig) );
        
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

        if ( NULL != pCamEngineCtx->hCamCalibDb )
        {
            // calibration database is available => use it
            AdpfConfig.type                 = ADPF_USE_CALIB_DATABASE;
            AdpfConfig.fSensorGain          = 1.0f;

            AdpfConfig.data.db.width        = pCamEngineCtx->outWindow.width;
            AdpfConfig.data.db.height       = pCamEngineCtx->outWindow.height;
            AdpfConfig.data.db.framerate    = 0;
            AdpfConfig.data.db.hCamCalibDb  = pCamEngineCtx->hCamCalibDb;

        }
        else
        {
            // calibration database not available => use a static default configuration
            AdpfConfig.type                 = ADPF_USE_DEFAULT_CONFIG;
            AdpfConfig.fSensorGain          = 1.0f;

            AdpfConfig.data.def.SigmaGreen      = 4UL;
            AdpfConfig.data.def.SigmaRedBlue    = 4UL;
            AdpfConfig.data.def.fGradient       = 1.0f;
            AdpfConfig.data.def.fOffset         = 0.0f;
            AdpfConfig.data.def.NfGains.fRed    = 1.0f;
            AdpfConfig.data.def.NfGains.fGreenR = 1.0f;
            AdpfConfig.data.def.NfGains.fGreenB = 1.0f;
            AdpfConfig.data.def.NfGains.fBlue   = 1.0f;
        }

        /* re-configure */
        result = AdpfReConfigure( pCamEngineCtx->hAdpf, &AdpfConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-onfiguration of ADPF failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineInitAdpcc
 *****************************************************************************/
RESULT CamEngineInitAdpcc
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AdpccInstanceConfig_t AdpccInstConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* setup config */
    MEMSET( &AdpccInstConfig, 0, sizeof(AdpccInstConfig) );

    /* init */
    result = AdpccInit( &AdpccInstConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Initializaion of ADPF failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* return handle */
    pCamEngineCtx->hAdpcc = AdpccInstConfig.hAdpcc;

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAdpcc
 *****************************************************************************/
RESULT CamEngineReleaseAdpcc
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( NULL != pCamEngineCtx->hAdpcc )
    {
        RESULT lres;

        lres = AdpccStop( pCamEngineCtx->hAdpcc );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop ADPF (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        lres = AdpccRelease( pCamEngineCtx->hAdpcc );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release ADPF (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        pCamEngineCtx->hAdpcc = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineConfAdpcc
 *****************************************************************************/
RESULT CamEngineConfAdpcc
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pCamEngineCtx->hAdpcc )
    {
        AdpccConfig_t AdpccConfig;

        // setup config configuration structure
        MEMSET( &AdpccConfig, 0, sizeof(AdpccConfig) );

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            AdpccConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AdpccConfig.hSubCamerIc  = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        }
        else
        {
            AdpccConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AdpccConfig.hSubCamerIc  = NULL;
        }

        if ( NULL != pCamEngineCtx->hCamCalibDb )
        {
            // calibration database is available => use it
            AdpccConfig.type                = ADPCC_USE_CALIB_DATABASE;
            AdpccConfig.fSensorGain         = 1.0f;

            AdpccConfig.data.db.width       = pCamEngineCtx->outWindow.width;
            AdpccConfig.data.db.height      = pCamEngineCtx->outWindow.height;
            AdpccConfig.data.db.framerate   = 0;
            AdpccConfig.data.db.hCamCalibDb = pCamEngineCtx->hCamCalibDb;
        }
        else
        {
            // calibration database not available => use a static default configuration
            AdpccConfig.type                                = ADPCC_USE_DEFAULT_CONFIG;
            AdpccConfig.fSensorGain                         = 1.0f;

            AdpccConfig.data.def.isp_dpcc_mode              = 0x00000004UL;
            AdpccConfig.data.def.isp_dpcc_output_mode       = 0x00000003UL;
            AdpccConfig.data.def.isp_dpcc_set_use           = 0x00000007UL;

            AdpccConfig.data.def.isp_dpcc_methods_set_1     = 0x00001D1DUL;
            AdpccConfig.data.def.isp_dpcc_methods_set_2     = 0x00000707UL;
            AdpccConfig.data.def.isp_dpcc_methods_set_3     = 0x00001F1FUL;

            AdpccConfig.data.def.isp_dpcc_line_thresh_1     = 0x00000808UL;
            AdpccConfig.data.def.isp_dpcc_line_mad_fac_1    = 0x00000404UL;
            AdpccConfig.data.def.isp_dpcc_pg_fac_1          = 0x00000403UL;
            AdpccConfig.data.def.isp_dpcc_rnd_thresh_1      = 0x00000A0AUL;
            AdpccConfig.data.def.isp_dpcc_rg_fac_1          = 0x00002020UL;

            AdpccConfig.data.def.isp_dpcc_line_thresh_2     = 0x0000100CUL;
            AdpccConfig.data.def.isp_dpcc_line_mad_fac_2    = 0x00001810UL;
            AdpccConfig.data.def.isp_dpcc_pg_fac_2          = 0x00000403UL;
            AdpccConfig.data.def.isp_dpcc_rnd_thresh_2      = 0x00000808UL;
            AdpccConfig.data.def.isp_dpcc_rg_fac_2          = 0x00000808UL;

            AdpccConfig.data.def.isp_dpcc_line_thresh_3     = 0x00002020UL;
            AdpccConfig.data.def.isp_dpcc_line_mad_fac_3    = 0x00000404UL;
            AdpccConfig.data.def.isp_dpcc_pg_fac_3          = 0x00000403UL;
            AdpccConfig.data.def.isp_dpcc_rnd_thresh_3      = 0x00000806UL;
            AdpccConfig.data.def.isp_dpcc_rg_fac_3          = 0x00000404UL;

            AdpccConfig.data.def.isp_dpcc_ro_limits         = 0x00000AFAUL;
            AdpccConfig.data.def.isp_dpcc_rnd_offs          = 0x00000FFFUL;
         }

        // configure
        result = AdpccConfigure( pCamEngineCtx->hAdpcc, &AdpccConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuration of ADPF failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReConfAdpcc
 *****************************************************************************/
RESULT CamEngineReConfAdpcc
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pCamEngineCtx->hAdpcc )
    {
        AdpccConfig_t AdpccConfig;

        // setup config configuration structure
        MEMSET( &AdpccConfig, 0, sizeof(AdpccConfig) );
        
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            AdpccConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AdpccConfig.hSubCamerIc  = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        }
        else
        {
            AdpccConfig.hCamerIc     = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
            AdpccConfig.hSubCamerIc  = NULL;
        }

        if ( NULL != pCamEngineCtx->hCamCalibDb )
        {
            // calibration database is available => use it
            AdpccConfig.type                = ADPCC_USE_CALIB_DATABASE;
            AdpccConfig.fSensorGain         = 1.0f;

            AdpccConfig.data.db.width       = pCamEngineCtx->outWindow.width;
            AdpccConfig.data.db.height      = pCamEngineCtx->outWindow.height;
            AdpccConfig.data.db.framerate   = 0;
            AdpccConfig.data.db.hCamCalibDb = pCamEngineCtx->hCamCalibDb;
        }
        else
        {
            // calibration database not available => use a static default configuration
            AdpccConfig.type                                = ADPCC_USE_DEFAULT_CONFIG;
            AdpccConfig.fSensorGain                         = 1.0f;

            AdpccConfig.data.def.isp_dpcc_mode              = 0x00000004UL;
            AdpccConfig.data.def.isp_dpcc_output_mode       = 0x00000003UL;
            AdpccConfig.data.def.isp_dpcc_set_use           = 0x00000007UL;

            AdpccConfig.data.def.isp_dpcc_methods_set_1     = 0x00001D1DUL;
            AdpccConfig.data.def.isp_dpcc_methods_set_2     = 0x00000707UL;
            AdpccConfig.data.def.isp_dpcc_methods_set_3     = 0x00001F1FUL;

            AdpccConfig.data.def.isp_dpcc_line_thresh_1     = 0x00000808UL;
            AdpccConfig.data.def.isp_dpcc_line_mad_fac_1    = 0x00000404UL;
            AdpccConfig.data.def.isp_dpcc_pg_fac_1          = 0x00000403UL;
            AdpccConfig.data.def.isp_dpcc_rnd_thresh_1      = 0x00000A0AUL;
            AdpccConfig.data.def.isp_dpcc_rg_fac_1          = 0x00002020UL;

            AdpccConfig.data.def.isp_dpcc_line_thresh_2     = 0x0000100CUL;
            AdpccConfig.data.def.isp_dpcc_line_mad_fac_2    = 0x00001810UL;
            AdpccConfig.data.def.isp_dpcc_pg_fac_2          = 0x00000403UL;
            AdpccConfig.data.def.isp_dpcc_rnd_thresh_2      = 0x00000808UL;
            AdpccConfig.data.def.isp_dpcc_rg_fac_2          = 0x00000808UL;

            AdpccConfig.data.def.isp_dpcc_line_thresh_3     = 0x00002020UL;
            AdpccConfig.data.def.isp_dpcc_line_mad_fac_3    = 0x00000404UL;
            AdpccConfig.data.def.isp_dpcc_pg_fac_3          = 0x00000403UL;
            AdpccConfig.data.def.isp_dpcc_rnd_thresh_3      = 0x00000806UL;
            AdpccConfig.data.def.isp_dpcc_rg_fac_3          = 0x00000404UL;

            AdpccConfig.data.def.isp_dpcc_ro_limits         = 0x00000AFAUL;
            AdpccConfig.data.def.isp_dpcc_rnd_offs          = 0x00000FFFUL;
        }

        // re-configure
        result = AdpccReConfigure( pCamEngineCtx->hAdpcc, &AdpccConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-configuration of ADPF failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineInitAvs
 *****************************************************************************/
RESULT CamEngineInitAvs
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    AvsInstanceConfig_t AvsInstConfig;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* setup config */
    MEMSET( &AvsInstConfig, 0, sizeof(AvsInstConfig) );

    /* init */
    result = AvsInit( &AvsInstConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Initializaion of AVS failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* return handle */
    pCamEngineCtx->hAvs = AvsInstConfig.hAvs;

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAvs
 *****************************************************************************/
RESULT CamEngineReleaseAvs
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pCamEngineCtx != NULL );

    if ( NULL != pCamEngineCtx->hAvs )
    {
        RESULT lres;

        lres = AvsStop( pCamEngineCtx->hAvs );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop AVS (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        lres = AvsRelease( pCamEngineCtx->hAvs );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release AVS (%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT ( result, lres );
        }

        pCamEngineCtx->hAvs = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineConfAvs
 *****************************************************************************/
RESULT CamEngineConfAvs
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pCamEngineCtx->hAvs )
    {
        /* setup config */
        MEMSET(&pCamEngineCtx->avsConfig, 0x00, sizeof(AvsConfig_t));
        
        pCamEngineCtx->avsConfig.hCamerIc    = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
        pCamEngineCtx->avsConfig.hSubCamerIc = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        pCamEngineCtx->avsConfig.offsetDataArraySize = PIC_BUFFER_NUM_SELF_SENSOR_IMGSTAB;
        pCamEngineCtx->avsConfig.srcWindow = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].isWindow;
        pCamEngineCtx->avsConfig.dstWindow = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].isWindow;

        /* configure */
        result = AvsConfigure( pCamEngineCtx->hAvs, &pCamEngineCtx->avsConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuration of AVS failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReConfAvs
 *****************************************************************************/
RESULT CamEngineReConfAvs
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL != pCamEngineCtx->hAvs )
    {
        /* only change windows (this function is called in case of resolution
         * changes), but leave everything else the way it is */
        pCamEngineCtx->avsConfig.hCamerIc    = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
        pCamEngineCtx->avsConfig.hSubCamerIc = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        pCamEngineCtx->avsConfig.offsetDataArraySize = PIC_BUFFER_NUM_SELF_SENSOR_IMGSTAB;
        pCamEngineCtx->avsConfig.srcWindow = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].isWindow;
        pCamEngineCtx->avsConfig.dstWindow = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].isWindow;

        /* configure */
        result = AvsReConfigure( pCamEngineCtx->hAvs, &pCamEngineCtx->avsConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Re-Configuration of AVS failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}




/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * CamEngineModulesInit()
 *****************************************************************************/
RESULT CamEngineModulesInit
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

#if 1 //zyc ,for test
    /* init ADPCC module */
    lres = CamEngineInitAdpcc( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initializing ADPCC failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* init ADPF module */
    lres = CamEngineInitAdpf( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initializing ADPF failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* init AF module */
    lres = CamEngineInitAf( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initializing AF failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* init AWB module */
    lres = CamEngineInitAwb( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initializing AWB failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* init AEC module */
    lres = CamEngineInitAec( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (initializing AEC failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* init AVS module */
    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        lres = CamEngineInitAvs( pCamEngineCtx );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (initializing AVS failed -> RESULT=%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }
    }
#endif
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineModulesRelease()
 *****************************************************************************/
RESULT CamEngineModulesRelease
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* release ADPCC module */
    lres = CamEngineReleaseAdpcc( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing ADPCC failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* release ADPF module */
    lres = CamEngineReleaseAdpf( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing ADPF failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* release AF module */
    lres = CamEngineReleaseAf( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing AF failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* release AWB module */
    lres = CamEngineReleaseAwb( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing AWB failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* release AEC module */
    lres = CamEngineReleaseAec( pCamEngineCtx );
    if ( lres != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (releasing AEC failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    /* release AVS module */
    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        lres = CamEngineReleaseAvs( pCamEngineCtx );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (releasing AVS failed -> RESULT=%d)\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * CamEngineModulesConfigure
 *****************************************************************************/
RESULT CamEngineModulesConfigure
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );
    //soc camera test
    //zyh@rock-chips.com: v0.0x20.0
    if(!pCamEngineCtx->isSOCsensor){
		if ( pCamEngineCtx == NULL )
		{
			return ( RET_WRONG_HANDLE );
		}

		/* configure AEC module */
		result = CamEngineConfAec( pCamEngineCtx );
		if ( (RET_SUCCESS != result) && (RET_NOTSUPP != result) )
		{
			TRACE( CAM_ENGINE_ERROR, "%s (configuring AEC failed -> RESULT=%d)\n", __FUNCTION__, result );
			return ( result );
		}

		/* configure AWB module */
		result = CamEngineConfAwb( pCamEngineCtx, &MeasConfig );
		if ( (RET_SUCCESS != result) && (RET_NOTSUPP != result) )
		{
			TRACE( CAM_ENGINE_ERROR, "%s (configuring AWB failed -> RESULT=%d)\n", __FUNCTION__, result );
			return ( result );
		}

		/* configure ADPF module */
		result = CamEngineConfAdpf( pCamEngineCtx );
		if ( RET_SUCCESS != result )
		{
			TRACE( CAM_ENGINE_ERROR, "%s (configuring ADPF failed -> RESULT=%d)\n", __FUNCTION__, result );
			return ( result );
		}

		/* configure ADPCC module */
		result = CamEngineConfAdpcc( pCamEngineCtx );
		if ( RET_SUCCESS != result )
		{
			TRACE( CAM_ENGINE_ERROR, "%s (configuring ADPCC failed -> RESULT=%d)\n", __FUNCTION__, result );
			return ( result );
		}

		/* configure AVS module */
		if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
		{
			result = CamEngineConfAvs( pCamEngineCtx );
			if ( RET_SUCCESS != result )
			{
				TRACE( CAM_ENGINE_ERROR, "%s (configuring AVS failed -> RESULT=%d)\n", __FUNCTION__, result );
				return ( result );
			}
		}

	}
	/* configure AF module */
	result = CamEngineConfAf( pCamEngineCtx );
	if ( (RET_SUCCESS != result) && (RET_NOTSUPP != result) )
	{
		TRACE( CAM_ENGINE_ERROR, "%s (configuring AF failed -> RESULT=%d)\n", __FUNCTION__, result );
		return ( result );
	}


    
    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineModulesReConfigure
 *****************************************************************************/
RESULT CamEngineModulesReConfigure
(
    CamEngineContext_t  *pCamEngineCtx,
    uint32_t            *pNumFramesToSkip
)
{
    RESULT result = RET_SUCCESS;

    //soc camera test
    if(pCamEngineCtx->isSOCsensor){
        return ( RET_SUCCESS );
    }

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* re-configure AEC module */
    result = CamEngineReConfAec( pCamEngineCtx, pNumFramesToSkip );
    if ( (RET_SUCCESS != result) && (RET_NOTSUPP != result) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (re-configuring AEC failed -> RESULT=%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* re-configure AWB module */
    result = CamEngineReConfAwb( pCamEngineCtx, &MeasConfig );
    if ( (RET_SUCCESS != result) && (RET_NOTSUPP != result) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (re-configuring AWB failed -> RESULT=%d)\n", __FUNCTION__, result );
        return ( result );
    }

#if 0
    /* re-configure AF module */
    lres = CamEngineReConfAf( pCamEngineCtx );
    if ( (lres != RET_SUCCESS) && (lres != RET_NOTSUPP) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (re-configuring AF failed -> RESULT=%d)\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }
#endif

    /* re-configure ADPF module */
    result = CamEngineReConfAdpf( pCamEngineCtx );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (re-configuring ADPF failed -> RESULT=%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* re-configure ADPCC module */
    result = CamEngineReConfAdpcc( pCamEngineCtx );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (re-configuring ADPCC failed -> RESULT=%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* re-configure AVS module */
    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        result = CamEngineReConfAvs( pCamEngineCtx );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (re-configuring AVS failed -> RESULT=%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


