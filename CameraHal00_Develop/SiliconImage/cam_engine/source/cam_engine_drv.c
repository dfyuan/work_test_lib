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
 * @file cam_engine_drv.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <common/utl_fixfloat.h>
#include <common/cam_types.h>

#include "cam_engine_drv.h"
#include "cam_engine_cb.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( CAM_ENGINE_INFO  );
USE_TRACER( CAM_ENGINE_WARN  );
USE_TRACER( CAM_ENGINE_ERROR );



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
 * CamEngineInitCamerIcDrv()
 *****************************************************************************/
RESULT CamEngineInitCamerIcDrv
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    CamerIcDrvConfig_t CamerIcDrvConfig;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    // clear driver configuration context
    MEMSET( &CamerIcDrvConfig, 0, sizeof( CamerIcDrvConfig ) );

    CamerIcDrvConfig.DrvHandle  = NULL;
    CamerIcDrvConfig.base       = HAL_BASEADDR_MARVIN;
    //CamerIcDrvConfig.base       = HAL_BASEADDR_MARVIN_2;
    CamerIcDrvConfig.HalHandle  = pCamEngineCtx->hHal;
    CamerIcDrvConfig.ModuleMask = CAMERIC_MODULE_ID_MASK_MIPI
                                    | CAMERIC_MODULE_ID_MASK_MIPI_2
                                    | CAMERIC_MODULE_ID_MASK_ISP
                                    | CAMERIC_MODULE_ID_MASK_CPROC
                                    | CAMERIC_MODULE_ID_MASK_IE
                                    | CAMERIC_MODULE_ID_MASK_SIMP
                                    | CAMERIC_MODULE_ID_MASK_MI
                                    | CAMERIC_MODULE_ID_MASK_BLS
                                    | CAMERIC_MODULE_ID_MASK_DEGAMMA
                                    | CAMERIC_MODULE_ID_MASK_LSC
                                    | CAMERIC_MODULE_ID_MASK_DPCC
                                    | CAMERIC_MODULE_ID_MASK_DPF
                                    | CAMERIC_MODULE_ID_MASK_IS
                                    | CAMERIC_MODULE_ID_MASK_CNR
                                    | CAMERIC_MODULE_ID_MASK_AWB
                                    | CAMERIC_MODULE_ID_MASK_HIST
                                    | CAMERIC_MODULE_ID_MASK_EXPOSURE
                                    | CAMERIC_MODULE_ID_MASK_AFM
                                    | CAMERIC_MODULE_ID_MASK_VSM
                                    | CAMERIC_MODULE_ID_MASK_JPE;
    result = CamerIcDriverInit( &CamerIcDrvConfig );
    if ( (RET_SUCCESS != result) || (NULL == CamerIcDrvConfig.DrvHandle) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't initialize CamerIc Driver (%d)\n", __FUNCTION__, result );
        return ( RET_FAILURE );
    }
    pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc = CamerIcDrvConfig.DrvHandle;

    // do we have a second pipe ?
    if ( BOOL_TRUE == pCamEngineCtx->isSystem3D )
    {
        // create a second pipe
        MEMSET( &CamerIcDrvConfig, 0, sizeof( CamerIcDrvConfig ) );
        CamerIcDrvConfig.DrvHandle  = NULL;
        CamerIcDrvConfig.base       = HAL_BASEADDR_MARVIN_2;
        //CamerIcDrvConfig.base       = HAL_BASEADDR_MARVIN;
        CamerIcDrvConfig.HalHandle  = pCamEngineCtx->hHal;
        CamerIcDrvConfig.ModuleMask = CAMERIC_MODULE_ID_MASK_MIPI
                                        | CAMERIC_MODULE_ID_MASK_ISP
                                        | CAMERIC_MODULE_ID_MASK_CPROC
                                        | CAMERIC_MODULE_ID_MASK_IE
                                        | CAMERIC_MODULE_ID_MASK_SIMP
                                        | CAMERIC_MODULE_ID_MASK_MI
                                        | CAMERIC_MODULE_ID_MASK_BLS
                                        | CAMERIC_MODULE_ID_MASK_DEGAMMA
                                        | CAMERIC_MODULE_ID_MASK_LSC
                                        | CAMERIC_MODULE_ID_MASK_DPCC
                                        | CAMERIC_MODULE_ID_MASK_DPF
                                        | CAMERIC_MODULE_ID_MASK_IS
                                        | CAMERIC_MODULE_ID_MASK_CNR
                                        | CAMERIC_MODULE_ID_MASK_AWB
                                        | CAMERIC_MODULE_ID_MASK_HIST
                                        | CAMERIC_MODULE_ID_MASK_EXPOSURE
                                        | CAMERIC_MODULE_ID_MASK_AFM
                                        | CAMERIC_MODULE_ID_MASK_VSM;
        result = CamerIcDriverInit( &CamerIcDrvConfig );
        if ( (RET_SUCCESS != result) || (NULL == CamerIcDrvConfig.DrvHandle) )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't initialize CamerIc Driver (%d)\n", __FUNCTION__, result );
            (void)CamEngineReleaseCamerIcDrv( pCamEngineCtx );
            return ( RET_FAILURE );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc = CamerIcDrvConfig.DrvHandle;
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineReleaseCamerIcDrv()
 *****************************************************************************/
RESULT CamEngineReleaseCamerIcDrv
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: MIPI Driver still not released!\n", __FUNCTION__ );
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc )
    {
        result = CamerIcDriverRelease( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release CamerIc Driver (%d)\n", __FUNCTION__, result );
            return ( RET_FAILURE );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc = NULL;
    }

    // do we have a second sensor ?
    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: MIPI Driver still not released!\n", __FUNCTION__ );
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc )
    {
        result = CamerIcDriverRelease( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't release CamerIc Driver (%d)\n", __FUNCTION__, result );
            return ( result );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineStartCamerIcDrv()
 *****************************************************************************/
RESULT CamEngineStartCamerIcDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    /* start the driver */
    result = CamerIcDriverStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( RET_SUCCESS != result )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't initialize CamerIc Driver (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* do we use a second pipe */
    if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
         /* start the driver */
        result = CamerIcDriverStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( RET_SUCCESS != result )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't initialize CamerIc Driver (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineStopCamerIcDrv()
 *****************************************************************************/
RESULT CamEngineStopCamerIcDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcDriverStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't stop CamerIc Driver (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    /* do we use a second pipe */
    if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        result = CamerIcDriverStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't stop CamerIc Driver (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineSetupCamerIcDrv()
 *****************************************************************************/
RESULT CamEngineSetupCamerIcDrv
(
    CamEngineContext_t                  *pCamEngineCtx,
    const CamerIcIspDemosaicBypass_t    demosaicMode,
    const uint8_t                       demosaicThreshold,
    const bool_t                        activateGammaIn,
    const bool_t                        activateGammaOut,
    const bool_t                        activateWB
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    /* ISP default configuration */
    result = CamerIcIspSetDemosaic( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, demosaicMode, demosaicThreshold );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Unsupported Demosaic Mode (%d)\n", __FUNCTION__, result);
        return ( result );
    }

    result = (activateGammaIn == BOOL_TRUE)
                    ? CamerIcIspDegammaEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc )
                    : CamerIcIspDegammaDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't activate gamma in (%d)\n", __FUNCTION__, result);
        return ( result );
    }

    result = (activateGammaOut == BOOL_TRUE)
                    ? CamerIcIspGammaOutEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc )
                    : CamerIcIspGammaOutDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't activate gamma out (%d)\n", __FUNCTION__, result);
        return ( result );
    }

    result = CamerIcIspActivateWB( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, activateWB );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't activate white balance (%d)\n", __FUNCTION__, result);
        return ( result );
    }

    if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        result = CamerIcIspSetDemosaic( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, demosaicMode, demosaicThreshold );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Unsupported Demosaic Mode (%d)\n", __FUNCTION__, result);
            return ( result );
        }

        result = (activateGammaIn == BOOL_TRUE)
                        ? CamerIcIspDegammaEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc )
                        : CamerIcIspDegammaDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't activate gamma in (%d)\n", __FUNCTION__, result);
            return ( result );
        }

        result = (activateGammaOut == BOOL_TRUE)
                        ? CamerIcIspGammaOutEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc )
                        : CamerIcIspGammaOutDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't activate gamma out (%d)\n", __FUNCTION__, result);
            return ( result );
        }

        result = CamerIcIspActivateWB( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, activateWB );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't activate white balance (%d)\n", __FUNCTION__, result);
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReStartEventCbCamerIcDrv()
 *****************************************************************************/
RESULT CamEngineReStartEventCbCamerIcDrv
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
	CamCalibAecGlobal_t *pAecGlobal;
	double wScale, hScale;
    bool_t isEnabled = BOOL_FALSE;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

	result = CamCalibDbGetAecGlobal( pCamEngineCtx->hCamCalibDb, &pAecGlobal );
	if ( result != RET_SUCCESS )
	{
		TRACE( CAM_ENGINE_ERROR, "%s(%d): Access to CalibDB failed. (%d)\n", __FUNCTION__, __LINE__, result );
		return ( result );
	}
	wScale=pAecGlobal->MeasuringWinWidthScale;
	hScale=pAecGlobal->MeasuringWinHeightScale;
	TRACE( CAM_ENGINE_INFO, "%s:MeasuringWinWidthScale=%f,MeasuringWinHeightScale=%f\n",
	__FUNCTION__, pAecGlobal->MeasuringWinWidthScale, pAecGlobal->MeasuringWinHeightScale);

    lres = CamerIcIspHistIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == lres )
    {
        if ( BOOL_TRUE == isEnabled )
        {
            result = CamerIcIspHistSetMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                                                        pCamEngineCtx->outWindow.hOffset,
                                                        pCamEngineCtx->outWindow.vOffset,
                                                        pCamEngineCtx->outWindow.width * wScale,
                                                        pCamEngineCtx->outWindow.height * hScale);
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver HIST window configuration failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            result = CamerIcIspHistEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't enable histogram measuring (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
        else
        {
            result = CamerIcIspHistDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't disable histogram measuring (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of histogram measuring (%d)\n", __FUNCTION__, lres );
        return ( lres );
    }


    isEnabled = BOOL_FALSE;
    lres = CamerIcIspExpIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == lres )
    {
        if ( BOOL_TRUE == isEnabled )
        {
            result = CamerIcIspExpSetMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                                                        pCamEngineCtx->outWindow.hOffset,
                                                        pCamEngineCtx->outWindow.vOffset,
                                                        ( (4096-176) > (pCamEngineCtx->outWindow.width*wScale))  ? (pCamEngineCtx->outWindow.width*wScale) : (4096-177), //-2000,        // @uli- hack for supporting (14MP)
                                                        ( (3072-140) > (pCamEngineCtx->outWindow.height*hScale)) ? (pCamEngineCtx->outWindow.height*hScale): (3072-141) );//FIXME
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver EXP window configuration failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            result = CamerIcIspExpEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't enable exposure (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
        else
        {
            result = CamerIcIspExpDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't disable exposure (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of exposure (%d)\n", __FUNCTION__, lres );
        return ( lres );
    }


    isEnabled = BOOL_FALSE;
    lres = CamerIcIspAwbIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == lres )
    {
        if ( BOOL_TRUE == isEnabled )
        {
        	CamCalibAwbGlobal_t *pAwbGlobal     = NULL;
        	char resName[32];
        	sprintf(resName, "%dx%d", pCamEngineCtx->outWindow.width, pCamEngineCtx->outWindow.height);
        	result = CamCalibDbGetAwbGlobalByResolution(pCamEngineCtx->hCamCalibDb, resName, &pAwbGlobal);
			if ( result != RET_SUCCESS )
			{
				TRACE( CAM_ENGINE_ERROR, "%s(%d): Access to CalibDB failed. (%d)\n", __FUNCTION__, __LINE__, result );
				return ( result );
			}
			
        	TRACE( CAM_ENGINE_ERROR, "%s: >>>>>%f,%f<<<<<<\n", __FUNCTION__, pAwbGlobal->awbMeasWinWidthScale, pAwbGlobal->awbMeasWinHeightScale);
            result = CamerIcIspAwbSetMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                                                        pCamEngineCtx->outWindow.hOffset,
                                                        pCamEngineCtx->outWindow.vOffset,
                                                        pCamEngineCtx->outWindow.width * pAwbGlobal->awbMeasWinWidthScale,
                                                        pCamEngineCtx->outWindow.height * pAwbGlobal->awbMeasWinHeightScale);
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AWB window configuration failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            result = CamerIcIspAwbEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't enable auto white balance (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
        else
        {
            result = CamerIcIspAwbDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't disable auto white balance (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of auto white balance (%d)\n", __FUNCTION__, lres );
        return ( lres );
    }


    isEnabled = BOOL_FALSE;
    lres = CamerIcIspAfmIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == lres )
    {
        if ( BOOL_TRUE == isEnabled )
        {
            uint16_t outWidth  = ( pCamEngineCtx->outWindow.width -  pCamEngineCtx->outWindow.hOffset );
            uint16_t outHeight = ( pCamEngineCtx->outWindow.height - pCamEngineCtx->outWindow.vOffset );

            uint16_t width  = ( outWidth  >= 200U ) ? 200U : outWidth;
            uint16_t height = ( outHeight >= 200U ) ? 200U : outHeight;
            //uint16_t width  =  outWidth/4;
            //uint16_t height =  outHeight/4;
            
            uint16_t x = ( outWidth  >> 1 ) - ( width  >> 1 );
            uint16_t y = ( outHeight >> 1 ) - ( height >> 1 );
            result = CamerIcIspAfmSetMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CAMERIC_ISP_AFM_WINDOW_A, x, y, width, height );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AFM measurement window configuration failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            result = CamerIcIspAfmEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't enable auto focus (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
        else
        {
            result = CamerIcIspAfmDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't disable auto focus (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of auto focus (%d)\n", __FUNCTION__, lres );
        return ( lres );
    }

    isEnabled = BOOL_FALSE;
    lres = CamerIcIspVsmIsEnabled( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &isEnabled );
    if ( RET_SUCCESS == lres )
    {
        if ( BOOL_TRUE == isEnabled )
        {
            CamerIcWindow_t measureWin;
            uint8_t horSegments;
            uint8_t verSegments;

            MEMSET( &measureWin, 0, sizeof(CamerIcWindow_t) );

            measureWin.hOffset = pCamEngineCtx->outWindow.hOffset;
            measureWin.vOffset = pCamEngineCtx->outWindow.vOffset;

            /* Only even values allowed for width. */
            measureWin.width = pCamEngineCtx->outWindow.width & ~0x1;
            /* 1920 width is maximum.
             * TODO: Use well defined Macro. */
            if (measureWin.width > 1920) {
                measureWin.width = 1920;
            }

            /* Choose number of horizontal segments such that horizontal window
             * overlap is at least half of window width. */
            horSegments = measureWin.width >> 4;
            if (horSegments > 128) {
                horSegments = 128;
            }
            if (horSegments < 1) {
                horSegments = 1;
            }

            measureWin.height = pCamEngineCtx->outWindow.height;
            /* Reduce height according to VSM design spec to give it time
             * for correlation calculation at the end of a frame. */
            {
                uint32_t horCorrelSteps = ((horSegments + 2) >> 1) + 6;
                measureWin.height -= horCorrelSteps;
                /* 1088 height is maximum.
                 * TODO: Use well defined Macro. */
                if (measureWin.height > 1088) {
                    measureWin.height = 1088;
                }
                /* Only even values allowed for height. */
                measureWin.height &= ~0x1;
            }

            /* Choose number of vertical segments such that vertical window
             * overlap is at least half of window height. */
            verSegments = measureWin.height >> 4;
            if (verSegments > 128) {
                verSegments = 128;
            }
            if (verSegments < 1) {
                verSegments = 1;
            }

            result = CamerIcIspVsmSetMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc,
                                                      &measureWin, horSegments, verSegments );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver VSM window configuration failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            result = CamerIcIspVsmEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't enable video stabilization measuring (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
        else
        {
            result = CamerIcIspVsmDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't disable video stabilization measuring (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }
    else
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't get status of video stabilization measuring (%d)\n", __FUNCTION__, lres );
        return ( lres );
    }


    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupJpeDrv()
 *****************************************************************************/
RESULT CamEngineSetupJpeDrv
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->JpeEventGenHeader, 1, 0 ) )
    {
        return ( RET_FAILURE );
    }

    result = CamerIcJpeRegisterEventCb( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, CamEngineCamerIcDrvJpeCb, pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver JPE callback registering failed (%d)\n", __FUNCTION__, result );
        (void)osEventDestroy( &pCamEngineCtx->JpeEventGenHeader );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * CamEngineReleaseJpeDrv()
 *****************************************************************************/
RESULT CamEngineReleaseJpeDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcJpeDeRegisterEventCb( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver JPE callback deregistering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    osEventDestroy( &pCamEngineCtx->JpeEventGenHeader );

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupHistogramDrv()
 *****************************************************************************/
RESULT CamEngineSetupHistogramDrv
(
    CamEngineContext_t          *pCamEngineCtx,
    const bool_t                enable,
    const CamerIcIspHistMode_t  mode
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspHistDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable histogram measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspHistSetMeasuringMode( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, mode );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver HIST mode configuration failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspHistRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CamEngineCamerIcDrvMeasureCb, pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver HIST callback registering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( BOOL_TRUE == enable )
    {
        result = CamerIcIspHistEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't enable histogram measuring (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseHistogramDrv()
 *****************************************************************************/
RESULT CamEngineReleaseHistogramDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspHistDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable histogram measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspHistDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver HIST callback deregistering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupExpDrv()
 *****************************************************************************/
RESULT CamEngineSetupExpDrv
(
    CamEngineContext_t                  *pCamEngineCtx,
    const bool_t                        enable,
    const CamerIcIspExpMeasuringMode_t  mode
)
{
    RESULT result;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamerIcIspExpDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable exposure luminance measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspExpSetMeasuringMode( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, mode );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver EXP mode configuration failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspExpRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CamEngineCamerIcDrvMeasureCb, pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver EXP callback registering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( BOOL_TRUE == enable )
    {
        result = CamerIcIspExpEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't enable exposure luminance measuring (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineReleaseExpDrv()
 *****************************************************************************/
RESULT CamEngineReleaseExpDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspExpDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable exposure luminance measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspExpDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver EXP callback deregistering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupAwbDrv()
 *****************************************************************************/
RESULT CamEngineSetupAwbDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspAwbDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable awb measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspAwbRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CamEngineCamerIcDrvMeasureCb, pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AWB callback registering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( BOOL_TRUE == enable )
    {
        result = CamerIcIspAwbEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't enable awb measuring (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAwbDrv()
 *****************************************************************************/
RESULT CamEngineReleaseAwbDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspAwbDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable AWB measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspAwbDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AWB callback deregistering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupAfmDrv()
 *****************************************************************************/
RESULT CamEngineSetupAfmDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspAfmDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable AFM measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( BOOL_TRUE == pCamEngineCtx->availableAf )
    {
        result = CamerIcIspAfmEnableMeasuringWindow( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CAMERIC_ISP_AFM_WINDOW_A );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AFM measurement window enabling failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        result = CamerIcIspAfmSetThreshold( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, 0x4U );        
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AFM threshold configuration failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        result = CamerIcIspAfmRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CamEngineCamerIcDrvMeasureCb, pCamEngineCtx );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AFM callback registering failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        if ( BOOL_TRUE == enable )
        {
            result = CamerIcIspAfmEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Can't enable histogram measuring (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineReleaseAfmDrv()
 *****************************************************************************/
RESULT CamEngineReleaseAfmDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspAfmDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable AFM measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspAfmDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver AFM callback deregistering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupVsmDrv()
 *****************************************************************************/
RESULT CamEngineSetupVsmDrv
(
    CamEngineContext_t                  *pCamEngineCtx,
    const bool_t                        enable
)
{
    RESULT result;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamerIcIspVsmDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable video stabilization measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspVsmRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CamEngineCamerIcDrvMeasureCb, pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver VSM callback registering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( BOOL_TRUE == enable )
    {
        result = CamerIcIspVsmEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't enable video stabilization measuring (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineReleaseVsmDrv()
 *****************************************************************************/
RESULT CamEngineReleaseVsmDrv
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    result = CamerIcIspVsmDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't disable video stabilization measuring (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    result = CamerIcIspVsmDeRegisterEventCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver VSM callback deregistering failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineSetupLscDrv()
 *****************************************************************************/
RESULT CamEngineSetupLscDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
)
{
    RESULT result;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamerIcIspLscDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver LSC enable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( BOOL_TRUE == enable )
    {
        result = CamerIcIspLscEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver LSC enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSetupFltDrv()
 *****************************************************************************/
RESULT CamEngineSetupFltDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
)
{
    RESULT result;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamerIcIspFltDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver FLT disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspFltDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver FLT disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    result = CamerIcIspFltSetFilterParameter( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, CAMERIC_ISP_FLT_DENOISE_LEVEL_2, CAMERIC_ISP_FLT_SHARPENING_LEVEL_3 );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver FLT set parameters failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspFltSetFilterParameter( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, CAMERIC_ISP_FLT_DENOISE_LEVEL_2, CAMERIC_ISP_FLT_SHARPENING_LEVEL_3 );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver FLT set parameters failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    if ( BOOL_TRUE == enable )
    {
        result = CamerIcIspFltEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver FLT enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = CamerIcIspFltEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver FLT enable failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSetupBlsDrv()
 *****************************************************************************/
RESULT CamEngineSetupBlsDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
)
{
    RESULT result;

    uint16_t red;
    uint16_t greenR;
    uint16_t greenB;
    uint16_t blue;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    // disbale blacklevel substraction module before (re)configuring
    result = CamerIcIspBlsDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspBlsDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    // enable blacklevel substraction module after configuration
    if ( BOOL_TRUE == enable )
    {
        // check to load data from calibDB
        if ( NULL != pCamEngineCtx->hCamCalibDb )
        {
            CamBlsProfile_t *pBlsProfile;
            CamerIcIspBayerPattern_t bayerPattern;

            uint16_t fixedA = 0U;
            uint16_t fixedB = 0U;
            uint16_t fixedC = 0U;
            uint16_t fixedD = 0U;

            // get BLS calibration profile from database
            result = CamCalibDbGetBlsProfileByResolution( pCamEngineCtx->hCamCalibDb, pCamEngineCtx->ResName, &pBlsProfile );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: BLS profile %s not available (%d)\n", __FUNCTION__, pCamEngineCtx->ResName, result );
                return ( result );
            }
            DCT_ASSERT( NULL != pBlsProfile );

            red     =  pBlsProfile->level.uCoeff[CAM_4CH_COLOR_COMPONENT_RED];
            greenR  =  pBlsProfile->level.uCoeff[CAM_4CH_COLOR_COMPONENT_GREENR];
            greenB  =  pBlsProfile->level.uCoeff[CAM_4CH_COLOR_COMPONENT_GREENB];
            blue    =  pBlsProfile->level.uCoeff[CAM_4CH_COLOR_COMPONENT_BLUE];

            // read back configured bayer-pattern
            result = CamerIcIspGetAcqBayerPattern( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &bayerPattern );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS read Bayer pattern failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            // bayer pattern check
            switch ( bayerPattern )
            {
                case CAMERIC_ISP_BAYER_PATTERN_RGRGGBGB:
                     {
                         fixedA = red;
                         fixedB = greenR;
                         fixedC = greenB;
                         fixedD = blue;
                         break;
                     }
            
                 case CAMERIC_ISP_BAYER_PATTERN_GRGRBGBG:
                     {
                         fixedA = greenR;
                         fixedB = red;
                         fixedC = blue;
                         fixedD = greenB;
                         break;
                     }
            
                 case CAMERIC_ISP_BAYER_PATTERN_GBGBRGRG:
                     {
                         fixedA = greenB;
                         fixedB = blue;
                         fixedC = red;
                         fixedD = greenR;
                         break;
                     }

                 case CAMERIC_ISP_BAYER_PATTERN_BGBGGRGR:
                     {
                         fixedA = blue;
                         fixedB = greenB;
                         fixedC = greenR;
                         fixedD = red;
                         break;
                     }
            
                 default:
                     {
                         TRACE( CAM_ENGINE_ERROR, "%s: unsopported bayer pattern (%d)\n", __FUNCTION__, bayerPattern );
                          return ( RET_NOTSUPP );
                     }

            }

            // set black level values 
            result = CamerIcIspBlsSetBlackLevel( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, fixedA, fixedB, fixedC, fixedD );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS set Black Level Substraction failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
            {
                result = CamerIcIspBlsSetBlackLevel( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, fixedA, fixedB, fixedC, fixedD );
                if ( result != RET_SUCCESS )
                {
                    TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS set Black Level Substraction failed (%d)\n", __FUNCTION__, result );
                    return ( result );
                }
            }
        }

        // enable
        result = CamerIcIspBlsEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = CamerIcIspBlsEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver BLS enable failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSetupCacDrv()
 *****************************************************************************/
RESULT CamEngineSetupCacDrv
(
    CamEngineContext_t      *pCamEngineCtx,
    const bool_t            enable
)
{
    RESULT result;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    // disable CAC module before (re)configuring
    result = CamerIcIspCacDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver CAC disable failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    // disable slave pipe
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = CamerIcIspCacDisable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver CAC disable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }

    // enable CAC module after configuration
    if ( BOOL_TRUE == enable )
    {
        // check to load data from calibDB
        if ( NULL != pCamEngineCtx->hCamCalibDb )
        {
            CamerIcIspCacConfig_t CacConfig;
            CamCacProfile_t *pCacProfile;

            // get CAC calibration profile from database
            result = CamCalibDbGetCacProfileByResolution( pCamEngineCtx->hCamCalibDb, pCamEngineCtx->ResName, &pCacProfile );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CAC profile %s not available (%d)\n", __FUNCTION__, pCamEngineCtx->ResName, result );
                return ( result );
            }
            DCT_ASSERT( NULL != pCacProfile );

            CacConfig.width         = pCamEngineCtx->outWindow.width;
            CacConfig.height        = pCamEngineCtx->outWindow.height;

            CacConfig.hCenterOffset = pCacProfile->hCenterOffset;
            CacConfig.vCenterOffset = pCacProfile->vCenterOffset;

            CacConfig.hClipMode     = CAMERIC_CAC_H_CLIPMODE_FIX4;
            CacConfig.vClipMode     = CAMERIC_CAC_V_CLIBMODE_DYN4;

            CacConfig.aRed          = UtlFloatToFix_S0504( pCacProfile->Red.fCoeff[0] );
            CacConfig.bRed          = UtlFloatToFix_S0504( pCacProfile->Red.fCoeff[1] );
            CacConfig.cRed          = UtlFloatToFix_S0504( pCacProfile->Red.fCoeff[2] );

            CacConfig.aBlue         = UtlFloatToFix_S0504( pCacProfile->Blue.fCoeff[0] );
            CacConfig.bBlue         = UtlFloatToFix_S0504( pCacProfile->Blue.fCoeff[1] );
            CacConfig.cBlue         = UtlFloatToFix_S0504( pCacProfile->Blue.fCoeff[2] );

            CacConfig.Xns           = pCacProfile->x_ns;
            CacConfig.Xnf           = pCacProfile->x_nf;

            CacConfig.Yns           = pCacProfile->y_ns;
            CacConfig.Ynf           = pCacProfile->y_nf;

            // configure CAC module
            result = CamerIcIspCacConfig( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc, &CacConfig );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver set CAC failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }

            // configure slave pipe
            if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
            {
                result = CamerIcIspCacConfig( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc, &CacConfig );
                if ( result != RET_SUCCESS )
                {
                    TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver set CAC failed (%d)\n", __FUNCTION__, result );
                    return ( result );
                }
            }
        }

        // enable
        result = CamerIcIspCacEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver CAC enable failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        // enable slave pipe
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = CamerIcIspCacEnable( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: CamerIc Driver CAC enable failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineStartMipiDrv
 *****************************************************************************/
RESULT CamEngineStartMipiDrv
(
    CamEngineContext_t *pCamEngineCtx,
    MipiDataType_t      mipiMode,
    int mipiLaneNum,
    uint32_t mipiFreq,
    uint32_t attachedDevId
)
{
    RESULT result;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__);

    result = MipiDrvStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi );
    if ( (result != RET_SUCCESS) && (result != RET_WRONG_STATE) )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Stopping MIPI driver failed (%d)\n", __FUNCTION__, result );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = MipiDrvStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi );
        if ( (result != RET_SUCCESS) && (result != RET_WRONG_STATE) )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Stopping MIPI driver failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }
    }
    /* zyc@rock-chips.com: v0.0x21.0 */
    if ( ( MIPI_DATA_TYPE_RAW_10 == mipiMode ) || 
         ( MIPI_DATA_TYPE_RAW_12 == mipiMode ) ||
         ( MIPI_DATA_TYPE_YUV422_8 == mipiMode ))
    {
        MipiConfig_t MipiConfig;
        MEMSET( &MipiConfig, 0, sizeof( MipiConfig ) );
        MipiConfig.NumLanes    = mipiLaneNum; //use lane 1 and 2;for test ,zyc;
        MipiConfig.VirtChannel = MIPI_VIRTUAL_CHANNEL_0;
        MipiConfig.DataType    = mipiMode;
        MipiConfig.CompScheme  = MIPI_DATA_COMPRESSION_SCHEME_NONE;
        MipiConfig.PredBlock   = MIPI_PREDICTOR_BLOCK_INVALID;
        MipiConfig.Freq        = mipiFreq;
        MipiConfig.DevId       = attachedDevId;

        result = MipiDrvConfig( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi, &MipiConfig );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Configuring MIPI driver failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = MipiDrvConfig( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi, &MipiConfig );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Configuring MIPI driver failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }

        result = MipiDrvStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Starting MIPI driver failed (%d)\n", __FUNCTION__, result );
            return ( result );
        }

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = MipiDrvStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s: Starting MIPI driver failed (%d)\n", __FUNCTION__, result );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineStopMipiDrv
 *****************************************************************************/
RESULT CamEngineStopMipiDrv
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi  )
    {
        lres = MipiDrvStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMipi );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Stopping MIPI driver failed (%d)\n", __FUNCTION__, result );
            UPDATE_RESULT( result, lres );
        }
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi )
    {
        lres = MipiDrvStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMipi );
        if ( lres != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Stopping MIPI driver failed (%d)\n", __FUNCTION__, result );
            UPDATE_RESULT( result, lres );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


