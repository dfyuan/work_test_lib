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
 * @file cam_engine_isp_api.c
 *
 * @brief
 *   Implementation of the CamEngine ISP API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/utl_fixfloat.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>
#include <cameric_drv/cameric_isp_bls_drv_api.h>
#include <cameric_drv/cameric_isp_lsc_drv_api.h>
#include <cameric_drv/cameric_isp_wdr_drv_api.h>
#include <cameric_drv/cameric_isp_cac_drv_api.h>
#include <cameric_drv/cameric_isp_flt_drv_api.h>
#include <cameric_drv/cameric_isp_cnr_drv_api.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_isp_api.h"

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
 * CamEngineBlsSet()
 *****************************************************************************/
RESULT CamEngineBlsSet
(
    CamEngineHandle_t   hCamEngine,
    uint16_t const      Red,
    uint16_t const      GreenR,
    uint16_t const      GreenB,
    uint16_t const      Blue
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspBlsSetBlackLevel(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, Red, GreenR, GreenB, Blue );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIc Driver set BLS failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspBlsSetBlackLevel(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, Red, GreenR, GreenB, Blue );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR, "%s: CamerIc Driver set BLS failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineBlsGet()
 *****************************************************************************/
RESULT CamEngineBlsGet
(
    CamEngineHandle_t   hCamEngine,
    uint16_t * const    pRed,
    uint16_t * const    pGreenR,
    uint16_t * const    pGreenB,
    uint16_t * const    pBlue
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pRed) || (NULL == pGreenR) || (NULL == pGreenB) || (NULL == pBlue) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspBlsGetBlackLevel(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pRed, pGreenR, pGreenB, pBlue );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWbSetGains()
 *****************************************************************************/
RESULT CamEngineWbSetGains
(
    CamEngineHandle_t                   hCamEngine,
    const CamEngineWbGains_t * const    pGains
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcGains_t drvGains;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pGains )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* compute fix point values */
    drvGains.Red      = UtlFloatToFix_U0208( pGains->Red );
    drvGains.GreenR   = UtlFloatToFix_U0208( pGains->GreenR );
    drvGains.GreenB   = UtlFloatToFix_U0208( pGains->GreenB );
    drvGains.Blue     = UtlFloatToFix_U0208( pGains->Blue );
    
    /* call cameric driver */
    result = CamerIcIspAwbSetGains(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvGains );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver set WB-Gains failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspAwbSetGains(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &drvGains );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver set WB-Gains failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWbGetGains()
 *****************************************************************************/
RESULT CamEngineWbGetGains
(
    CamEngineHandle_t           hCamEngine,
    CamEngineWbGains_t * const  pGains
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcGains_t drvGains;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pGains )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspAwbGetGains(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvGains );
    if ( RET_SUCCESS == result )
    {
        /* compute float values */
        pGains->Red     = UtlFixToFloat_U0208( drvGains.Red );
        pGains->GreenR  = UtlFixToFloat_U0208( drvGains.GreenR );
        pGains->GreenB  = UtlFixToFloat_U0208( drvGains.GreenB );
        pGains->Blue    = UtlFixToFloat_U0208( drvGains.Blue );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWbSetCcMatrix()
 *****************************************************************************/
RESULT CamEngineWbSetCcMatrix
(
    CamEngineHandle_t                   hCamEngine,
    const CamEngineCcMatrix_t * const   pCcMatrix
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIc3x3Matrix_t drvCcMatrix;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pCcMatrix )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* compute fix point values */
    for ( uint8_t i=0U; i<9U; i++ )
    {
        drvCcMatrix.Coeff[i] = UtlFloatToFix_S0407( pCcMatrix->Coeff[i] );
    }

    /* call cameric driver */
    result = CamerIcIspSetCrossTalkCoefficients(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvCcMatrix );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver set XTalk failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetCrossTalkCoefficients(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &drvCcMatrix );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver set XTalk failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWbGetCcMatrix()
 *****************************************************************************/
RESULT CamEngineWbGetCcMatrix
(
    CamEngineHandle_t           hCamEngine,
    CamEngineCcMatrix_t * const pCcMatrix
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIc3x3Matrix_t drvCcMatrix;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pCcMatrix )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspGetCrossTalkCoefficients( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvCcMatrix );
    if ( RET_SUCCESS == result )
    {
        /* compute float values */
        for ( uint8_t i=0U; i<9U; i++ )
        {
            pCcMatrix->Coeff[i] = UtlFixToFloat_S0407( drvCcMatrix.Coeff[i] );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWbSetCcOffset()
 *****************************************************************************/
RESULT CamEngineWbSetCcOffset
(
    CamEngineHandle_t                   hCamEngine,
    const CamEngineCcOffset_t * const   pCcOffset
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcXTalkOffset_t drvCcOffset;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pCcOffset )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* compute fix point values */
    drvCcOffset.Red     = UtlFloatToFix_S1200( (float)pCcOffset->Red );
    drvCcOffset.Green   = UtlFloatToFix_S1200( (float)pCcOffset->Green );
    drvCcOffset.Blue    = UtlFloatToFix_S1200( (float)pCcOffset->Blue );

    /* call cameric driver */
    result = CamerIcIspSetCrossTalkOffset(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvCcOffset );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver set XTalk-Offsets failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetCrossTalkOffset(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &drvCcOffset );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver set XTalk-Offsets failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWbGetCcOffset()
 *****************************************************************************/
RESULT CamEngineWbGetCcOffset
(
    CamEngineHandle_t           hCamEngine,
    CamEngineCcOffset_t * const pCcOffset
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcXTalkOffset_t drvCcOffset;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pCcOffset )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspGetCrossTalkOffset( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvCcOffset );
    if ( RET_SUCCESS == result )
    {
        /* compute float values */
        pCcOffset->Red   = (int16_t)UtlFixToFloat_S1200( drvCcOffset.Red );
        pCcOffset->Green = (int16_t)UtlFixToFloat_S1200( drvCcOffset.Green );
        pCcOffset->Blue  = (int16_t)UtlFixToFloat_S1200( drvCcOffset.Blue );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineDemosaicSet()
 *****************************************************************************/
RESULT CamEngineDemosaicSet
(
    CamEngineHandle_t   hCamEngine,
    bool_t const        Bypass,
    uint8_t const       Threshold
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* compute value */
    CamerIcIspDemosaicBypass_t  BypassMode =
        (BOOL_TRUE != Bypass ) ? CAMERIC_ISP_DEMOSAIC_NORMAL_OPERATION : CAMERIC_ISP_DEMOSAIC_BYPASS;

    /* call cameric driver */
    result = CamerIcIspSetDemosaic(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, BypassMode, Threshold );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver Demosaicing mode failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetDemosaic( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, BypassMode, Threshold );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver Demosaicing mode failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineDemosaicGet()
 *****************************************************************************/
RESULT CamEngineDemosaicGet
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pBypass,
    uint8_t * const     pThreshold
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pBypass) || (NULL == pThreshold) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    CamerIcIspDemosaicBypass_t  BypassMode = CAMERIC_ISP_DEMOSAIC_INVALID;
    result = CamerIcIspGetDemosaic( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &BypassMode, pThreshold );
    if ( RET_SUCCESS == result )
    {
        /* compute value */
        *pBypass = (CAMERIC_ISP_DEMOSAIC_BYPASS != BypassMode ) ? BOOL_FALSE : BOOL_TRUE;
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineLscStatus()
 *****************************************************************************/
RESULT CamEngineLscStatus
(
    CamEngineHandle_t               hCamEngine,
    bool_t * const                  pRunning,
    CamEngineLscConfig_t * const    pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcIspLscConfig_t drvLscConfig;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pRunning) || (NULL == pConfig) ) 
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* get activation status of LSC module */
    result = CamerIcIspLscIsEnabled(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pRunning );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver LSC check status failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* get configuration of LSC module */
    result = CamerIcIspLscGetLenseShadeCorrection(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvLscConfig );
    if ( RET_SUCCESS == result )
    {
        /* copy grid configuration */
        MEMCPY( pConfig->grid.LscXGradTbl, drvLscConfig.LscXGradTbl, sizeof(pConfig->grid.LscXGradTbl) );
        MEMCPY( pConfig->grid.LscYGradTbl, drvLscConfig.LscYGradTbl, sizeof(pConfig->grid.LscYGradTbl) );
        MEMCPY( pConfig->grid.LscXSizeTbl, drvLscConfig.LscXSizeTbl, sizeof(pConfig->grid.LscXSizeTbl) );
        MEMCPY( pConfig->grid.LscYSizeTbl, drvLscConfig.LscYSizeTbl, sizeof(pConfig->grid.LscYSizeTbl) );

        /* copy gain configuration */
        MEMCPY( pConfig->gain.LscRDataTbl,  drvLscConfig.LscRDataTbl, sizeof(pConfig->gain.LscRDataTbl) );
        MEMCPY( pConfig->gain.LscGRDataTbl, drvLscConfig.LscGRDataTbl, sizeof(pConfig->gain.LscGRDataTbl) );
        MEMCPY( pConfig->gain.LscGBDataTbl, drvLscConfig.LscGBDataTbl, sizeof(pConfig->gain.LscGBDataTbl) );
        MEMCPY( pConfig->gain.LscBDataTbl,  drvLscConfig.LscBDataTbl, sizeof(pConfig->gain.LscBDataTbl) );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );
}


/******************************************************************************
 * CamEngineLscEnable()
 *****************************************************************************/
RESULT CamEngineLscEnable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspLscEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver LSC enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspLscEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver LSC enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineLscDisable()
 *****************************************************************************/
RESULT CamEngineLscDisable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspLscDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver LSC disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspLscDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver LSC disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineLscGetConfig()
 *****************************************************************************/
RESULT CamEngineLscGetConfig
(
    CamEngineHandle_t               hCamEngine,
    CamEngineLscConfig_t * const    pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcIspLscConfig_t drvLscConfig;

    RESULT result = RET_SUCCESS;

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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspLscGetLenseShadeCorrection(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvLscConfig );
    if ( RET_SUCCESS == result )
    {
        /* copy grid configuration */
        MEMCPY( pConfig->grid.LscXGradTbl, drvLscConfig.LscXGradTbl, sizeof(pConfig->grid.LscXGradTbl) );
        MEMCPY( pConfig->grid.LscYGradTbl, drvLscConfig.LscYGradTbl, sizeof(pConfig->grid.LscYGradTbl) );
        MEMCPY( pConfig->grid.LscXSizeTbl, drvLscConfig.LscXSizeTbl, sizeof(pConfig->grid.LscXSizeTbl) );
        MEMCPY( pConfig->grid.LscYSizeTbl, drvLscConfig.LscYSizeTbl, sizeof(pConfig->grid.LscYSizeTbl) );

        /* copy gain configuration */
        MEMCPY( pConfig->gain.LscRDataTbl,  drvLscConfig.LscRDataTbl, sizeof(pConfig->gain.LscRDataTbl) );
        MEMCPY( pConfig->gain.LscGRDataTbl, drvLscConfig.LscGRDataTbl, sizeof(pConfig->gain.LscGRDataTbl) );
        MEMCPY( pConfig->gain.LscGBDataTbl, drvLscConfig.LscGBDataTbl, sizeof(pConfig->gain.LscGBDataTbl) );
        MEMCPY( pConfig->gain.LscBDataTbl,  drvLscConfig.LscBDataTbl, sizeof(pConfig->gain.LscBDataTbl) );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWdrEnable()
 *****************************************************************************/
RESULT CamEngineWdrEnable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* get current flags of white balance */
    uint32_t AwbFlags = 0;
    result = AwbGetFlags( pCamEngineCtx->hAwb, &AwbFlags );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: Failed to read AWB flags (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* we want to enable the WDR, so inform AWB to not change the xtalk offset */
    AwbFlags &= ~AWB_WORKING_FLAG_USE_CC_OFFSET;
    result = AwbSetFlags( pCamEngineCtx->hAwb, AwbFlags );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: Failed to set AWB flags (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver */
    result = CamerIcIspWdrEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver WDR enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspWdrEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver WDR enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }
    
    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineWdrDisable()
 *****************************************************************************/
RESULT CamEngineWdrDisable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* get current flags of white balance */
    uint32_t AwbFlags = 0;
    result = AwbGetFlags( pCamEngineCtx->hAwb, &AwbFlags );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: Failed to read AWB flags (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* we want to enable the WDR, so allow AWB to change the xtalk offset */
    AwbFlags |= AWB_WORKING_FLAG_USE_CC_OFFSET;
    result = AwbSetFlags( pCamEngineCtx->hAwb, AwbFlags );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: Failed to set AWB flags (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver */
    result = CamerIcIspWdrDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver WDR disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspWdrDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver WDR disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineWdrSetCurve()
 *****************************************************************************/
RESULT CamEngineWdrSetCurve
(
    CamEngineHandle_t hCamEngine,
    CamEngineWdrCurve_t WdrCurve
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspWdrSetCurve(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, WdrCurve.Ym, WdrCurve.dY );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: booting tonemap-ecurve failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspWdrSetCurve(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, WdrCurve.Ym, WdrCurve.dY );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: booting tonemap-ecurve failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineGammaEnable()
 *****************************************************************************/
RESULT CamEngineGammaStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pRunning
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pRunning ) 
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspIsGammaOutActivated( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pRunning );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );
}


/******************************************************************************
 * CamEngineGammaEnable()
 *****************************************************************************/
RESULT CamEngineGammaEnable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspGammaOutEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver Gamma enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspGammaOutEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver Gamma enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );
}


/******************************************************************************
 * CamEngineGammaDisable()
 *****************************************************************************/
RESULT CamEngineGammaDisable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspGammaOutDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver Gamma disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspGammaOutDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver Gamma disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );
}


/******************************************************************************
 * CamEngineGammaDisable()
 *****************************************************************************/
RESULT CamEngineGammaSetCurve
(
    CamEngineHandle_t           hCamEngine,
    CamEngineGammaOutCurve_t    GammaCurve
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    CamerIcIspGammaSegmentationMode_t mode;
    CamerIcIspGammaCurve_t            curve;
    int32_t i;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }
   
    /* check if cam-engine running */
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* compute value */
    mode = ( GammaCurve.xScale == CAM_ENGINE_GAMMAOUT_XSCALE_LOG )
                ? CAMERIC_ISP_SEGMENTATION_MODE_LOGARITHMIC
                : CAMERIC_ISP_SEGMENTATION_MODE_EQUIDISTANT;

    for (i=0; i<CAMERIC_ISP_GAMMA_CURVE_SIZE; i++ )
    {
        curve.GammaY[i] = GammaCurve.GammaY[i];
    }

    /* call cameric driver */
    result = CamerIcIspSetGammOutCurve( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, mode, &curve );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver setting gamma correction curve failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetGammOutCurve( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, mode, &curve );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver setting gamma correction curve failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );
}

/******************************************************************************
 * CamEngineCacStatus()
 *****************************************************************************/
RESULT CamEngineCacStatus
(
    CamEngineHandle_t               hCamEngine,
    bool_t * const                  pRunning,
    CamEngineCacConfig_t * const    pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcIspCacConfig_t drvCacConfig;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pRunning) || (NULL == pConfig) ) 
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspCacIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pRunning );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CAC check status failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver */
    result = CamerIcIspCacGetConfig( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &drvCacConfig );
    if ( RET_SUCCESS == result )
    {
        /* compute float values */
        pConfig->width          = drvCacConfig.width;
        pConfig->height         = drvCacConfig.height;
        pConfig->hCenterOffset  = drvCacConfig.hCenterOffset;
        pConfig->vCenterOffset  = drvCacConfig.vCenterOffset;
        pConfig->hClipMode      = drvCacConfig.hClipMode;
        pConfig->vClipMode      = drvCacConfig.vClipMode;
        pConfig->aBlue          = UtlFixToFloat_S0504( drvCacConfig.aBlue );
        pConfig->aRed           = UtlFixToFloat_S0504( drvCacConfig.aRed );
        pConfig->bBlue          = UtlFixToFloat_S0504( drvCacConfig.bBlue );
        pConfig->bRed           = UtlFixToFloat_S0504( drvCacConfig.bRed );
        pConfig->cBlue          = UtlFixToFloat_S0504( drvCacConfig.cBlue );
        pConfig->cRed           = UtlFixToFloat_S0504( drvCacConfig.cRed );

        pConfig->Xns            = drvCacConfig.Xns;
        pConfig->Xnf            = drvCacConfig.Xnf;

        pConfig->Yns            = drvCacConfig.Yns;
        pConfig->Ynf            = drvCacConfig.Ynf;
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );
}

/******************************************************************************
 * CamEngineCacEnable()
 *****************************************************************************/
RESULT CamEngineCacEnable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspCacEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CAC enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspCacEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CAC enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCacDisable()
 *****************************************************************************/
RESULT CamEngineCacDisable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspCacDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CAC disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspCacDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CAC disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineFilterStatus()
 *****************************************************************************/
RESULT CamEngineFilterStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pRunning,
    uint8_t * const     pDenoiseLevel,
    uint8_t * const     pSharpenLevel
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcIspFltDeNoiseLevel_t     denoiseLevel;
    CamerIcIspFltSharpeningLevel_t  sharpenLevel;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pRunning) || (NULL == pDenoiseLevel) || (NULL == pSharpenLevel) )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspFltIsEnabled(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pRunning );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver FLT check status failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspFltGetFilterParameter(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &denoiseLevel, &sharpenLevel );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver getting FLT parameter failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    *pDenoiseLevel = ((uint8_t)denoiseLevel) - 1;
    *pSharpenLevel = ((uint8_t)sharpenLevel) - 1;

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineFilterEnable()
 *****************************************************************************/
RESULT CamEngineFilterEnable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspFltEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver FLT enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspFltEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver FLT enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineFilterDisable()
 *****************************************************************************/
RESULT CamEngineFilterDisable
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspFltDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver FLT disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspFltDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver FLT disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineFilterSetLevel()
 *****************************************************************************/
RESULT CamEngineFilterSetLevel
(
    CamEngineHandle_t   hCamEngine,
    uint8_t const       DenoiseLevel,
    uint8_t const       SharpenLevel
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspFltSetFilterParameter( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
            (CamerIcIspFltDeNoiseLevel_t)(DenoiseLevel+1), (CamerIcIspFltSharpeningLevel_t)(SharpenLevel+1) );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver setting FLT parameter failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspFltSetFilterParameter( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc,
                (CamerIcIspFltDeNoiseLevel_t)(DenoiseLevel+1), (CamerIcIspFltSharpeningLevel_t)(SharpenLevel+1) );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver setting FLT parameter failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCnrIsAvailable()
 *****************************************************************************/
RESULT CamEngineCnrIsAvailable
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
    result = CamerIcIspCnrIsAvailable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCnrEnable()
 *****************************************************************************/
RESULT CamEngineCnrEnable
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
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspCnrEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CNR enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspCnrEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CNR enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCnrDisable()
 *****************************************************************************/
RESULT CamEngineCnrDisable
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
    result = CamerIcIspCnrDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CNR disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspCnrDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CNR disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCnrStatus()
 *****************************************************************************/
RESULT CamEngineCnrStatus
(
    CamEngineHandle_t   hCamEngine,
    bool_t * const      pRunning,
    uint32_t * const    pThreshold1,
    uint32_t * const    pThreshold2
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

    if ( NULL == pRunning )
    {
        return ( RET_INVALID_PARM );
    }

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspCnrIsActivated(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,pRunning );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CNR check status failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspCnrGetThresholds(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pThreshold1, pThreshold2 );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CNR get thresholds failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCnrSetThresholds()
 *****************************************************************************/
RESULT CamEngineCnrSetThresholds
(
    CamEngineHandle_t   hCamEngine,
    uint32_t const      Threshold1,
    uint32_t const      Threshold2
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

    /* check if cam-engine running */
    if ( ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING   ) &&
         ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING ) )
    {
        return ( RET_WRONG_STATE );
    }

    /* call cameric driver */
    result = CamerIcIspCnrSetThresholds(
                pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, Threshold1,Threshold2 );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver CNR set thresholds failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspCnrSetThresholds(
                    pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, Threshold1, Threshold2 );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver CNR set thresholds failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}




RESULT CamEngineSetColorConversionRange
(
     CamEngineHandle_t           hCamEngine,
    CamerIcColorConversionRange_t YConvRange,
    CamerIcColorConversionRange_t CrConvRange
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
    if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
         ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
    {
        return ( RET_WRONG_STATE );
    }

	 /* call cameric driver */
    result = CamerIcIspSetColorConversionRange( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, YConvRange, CrConvRange );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver setting ColorConversionRange failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetColorConversionRange( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, YConvRange, CrConvRange );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver setting ColorConversionRange failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );

	
}


RESULT CamEngineSetColorConversionCoefficients
(
     CamEngineHandle_t           hCamEngine,
    CamerIc3x3Matrix_t    *pCConvCoefficients
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
	if ( ( CAM_ENGINE_STATE_RUNNING   != pCamEngineCtx->state ) &&
		 ( CAM_ENGINE_STATE_STREAMING != pCamEngineCtx->state ) )
	{
		return ( RET_WRONG_STATE );
	}

	 /* call cameric driver */
    result = CamerIcIspSetColorConversionCoefficients( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, pCConvCoefficients );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_API_ERROR,
                "%s: CamerIc Driver setting ColorConversion Coefficients failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* call cameric driver for second pipe */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspSetColorConversionCoefficients( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc,pCConvCoefficients );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_API_ERROR,
                    "%s: CamerIc Driver setting ColorConversion Coefficients failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );
    
    return ( result );

}


