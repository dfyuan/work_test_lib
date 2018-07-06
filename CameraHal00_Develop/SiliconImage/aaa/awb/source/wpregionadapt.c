/******************************************************************************
 * * Copyright 2010, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted, * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel, * Germany *
 *****************************************************************************/
/**
 * @file wprevert.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <math.h>
#include <float.h>

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "awb.h"
#include "awb_ctrl.h"

#include "wpregionadapt.h"
#include "interpolate.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( AWB_INFO  );
USE_TRACER( AWB_WARN  );
USE_TRACER( AWB_ERROR );

USE_TRACER( AWB_DEBUG );

#define MIN( a, b )         ( (a < b) ? ( a ) : ( b ) )
#define MAX( a, b )         ( (a > b) ? ( a ) : ( b ) )

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
 * local functions
 *****************************************************************************/
static RESULT AwbWpCalculateMaxY
(
    CamerIcAwbMeasuringConfig_t *pMeasConfig
)
{
    int i;
    int j;

    float fMaxY = FLT_MAX;  

    float af_inv_fixed_CSM[3][3] =
    {
        { 1.16360251338143f, -0.0622839361692239f,  1.60078228623829f  },
        { 1.16360251338143f, -0.404526997191162f,  -0.794919140915279f },
        { 1.16360251338143f,  1.9911744299624f,    -0.0250091508403276 }
    };

    // determine the coordinates for the polygon corners
    float af_cb[4] = { 0.0f };
    float af_cr[4] = { 0.0f };

    float af_ymaxTmp[3];
    float af_ymax[4];

    if( pMeasConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    af_cb[0] = (float)( -pMeasConfig->MinC + (pMeasConfig->RefCb_MaxB - 128) );
    af_cb[1] = (float)(  pMeasConfig->MinC - pMeasConfig->MaxCSum + (pMeasConfig->RefCb_MaxB - 128) );
    af_cb[2] = (float)(  pMeasConfig->MaxCSum + pMeasConfig->MinC + (pMeasConfig->RefCb_MaxB - 128) );
    af_cb[3] = (float)( -pMeasConfig->MinC + ( pMeasConfig->RefCb_MaxB - 128 ) );

    af_cr[0] = (float)(  pMeasConfig->MinC - pMeasConfig->MaxCSum + (pMeasConfig->RefCr_MaxR - 128) );
    af_cr[1] = (float)( -pMeasConfig->MinC + (pMeasConfig->RefCr_MaxR - 128) );
    af_cr[2] = (float)( -pMeasConfig->MinC + (pMeasConfig->RefCr_MaxR - 128) );
    af_cr[3] = (float)(  pMeasConfig->MaxCSum + pMeasConfig->MinC + (pMeasConfig->RefCr_MaxR - 128) );


    // for each point calculate the optimal YMAX right before going into clipping in RGB space
    for( i = 0; i < 4; i++ )
    {
        for( j = 0; j < 3; j++ )
        {
            af_ymaxTmp[j] =   -(   af_inv_fixed_CSM[j][1] * af_cb[i]
                                 + af_inv_fixed_CSM[j][2] * af_cr[i]
                                 - 254.0f )
                            / af_inv_fixed_CSM[j][0]
                            + 16.0f;
        }
        af_ymax[i] = MIN( af_ymaxTmp[0], MIN( af_ymaxTmp[1], af_ymaxTmp[2] ) );

        // since the polygon is convex, determine the minimum YMAX of all polygon points
        if( af_ymax[i] < fMaxY ) //fMaxY is initialised to FLT_MAX, so we surely catch the first array element
        {
            fMaxY = af_ymax[i];
        }
    }

    // for programming the hardware
    pMeasConfig->MaxY = (uint8_t)fMaxY;

    return ( RET_SUCCESS );
}



static RESULT AwbWpRegionAdapt
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    InterpolateCtx_t InterpolateCtx;

    float f_CbMin_regionMax     = 0.0f;
    float f_CrMin_regionMax     = 0.0f;
    float f_MaxCSum_regionMax   = 0.0f;
    float f_CbMin_regionMin     = 0.0f;
    float f_CrMin_regionMin     = 0.0f;
    float f_MaxCSum_regionMin   = 0.0f;
    float f_CbMin               = 0.0f;
    float f_CrMin               = 0.0f;
    float f_MaxCSum             = 0.0f;
    float f_shift               = 0.0f;
	float f_MinC_regionMax      = 0.0f;
	float f_MinC_regionMin      = 0.0f;
	float f_MinC				= 0.0f;
	float f_MaxY_regionMax		= 0.0f;
	float f_MaxY_regionMin		= 0.0f;
	float f_MaxY				= 0.0f;
	float f_MinY_MaxG_regionMax = 0.0f;
	float f_MinY_MaxG_regionMin = 0.0f;
	float f_MinY_MaxG			= 0.0f;
	float f_RefCb				= 0.0f;
	float f_RefCr				= 0.0f;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pAwbCtx->WhitePixelEvaluation == AWB_NUM_WHITE_PIXEL_LTMIN )
    {
        pAwbCtx->RegionSize = MIN( 1.0f, (pAwbCtx->RegionSize + pAwbCtx->RegionSizeInc) );
    }
    else if ( pAwbCtx->WhitePixelEvaluation == AWB_NUM_WHITE_PIXEL_GTMAX )
    {
        /**
         * if GTMAX && "out of range" we use the old region size:
         * decrement is not good because the white pixels aren't
         * "really white"
         */
        if ( pAwbCtx->WbGainsOutOfRange )
        {
            /* otherwise: decrease region size */
            pAwbCtx->RegionSize = MAX( 0.0f, (pAwbCtx->RegionSize - pAwbCtx->RegionSizeDec) );
        }
    }

    InterpolateCtx.size = pAwbCtx->pFadeParam->ArraySize;
    InterpolateCtx.pX   = pAwbCtx->pFadeParam->pFade;
    InterpolateCtx.pY   = pAwbCtx->pFadeParam->pCbMinRegionMax;
    InterpolateCtx.x_i  = pAwbCtx->RgProj;
    result = Interpolate( &InterpolateCtx );
    UPDATE_RESULT( result, RET_SUCCESS );
    f_CbMin_regionMax   = InterpolateCtx.y_i;

    InterpolateCtx.pY   = pAwbCtx->pFadeParam->pCrMinRegionMax;
    InterpolateCtx.x_i  = pAwbCtx->RgProj;
    result = Interpolate( &InterpolateCtx );
    UPDATE_RESULT( result, RET_SUCCESS );
    f_CrMin_regionMax   = InterpolateCtx.y_i;

    InterpolateCtx.pY   = pAwbCtx->pFadeParam->pMaxCSumRegionMax;
    InterpolateCtx.x_i  = pAwbCtx->RgProj;
    result = Interpolate( &InterpolateCtx );
    UPDATE_RESULT( result, RET_SUCCESS );
    f_MaxCSum_regionMax = InterpolateCtx.y_i;

    InterpolateCtx.pY   = pAwbCtx->pFadeParam->pCbMinRegionMin;
    InterpolateCtx.x_i  = pAwbCtx->RgProj;
    result = Interpolate( &InterpolateCtx );
    UPDATE_RESULT( result, RET_SUCCESS );
    f_CbMin_regionMin   = InterpolateCtx.y_i;

    InterpolateCtx.pY   = pAwbCtx->pFadeParam->pCrMinRegionMin;
    InterpolateCtx.x_i  = pAwbCtx->RgProj;
    result = Interpolate( &InterpolateCtx );
    UPDATE_RESULT( result, RET_SUCCESS );
    f_CrMin_regionMin   = InterpolateCtx.y_i;

    InterpolateCtx.pY   = pAwbCtx->pFadeParam->pMaxCSumRegionMin;
    InterpolateCtx.x_i  = pAwbCtx->RgProj;
    result = Interpolate( &InterpolateCtx );
    UPDATE_RESULT( result, RET_SUCCESS );
    f_MaxCSum_regionMin = InterpolateCtx.y_i;

	if(pAwbCtx->pFadeParam->regionAdjustEnable){
		InterpolateCtx.pY   = pAwbCtx->pFadeParam->pMinCRegionMax;
	    InterpolateCtx.x_i  = pAwbCtx->RgProj;
	    result = Interpolate( &InterpolateCtx );
	    UPDATE_RESULT( result, RET_SUCCESS );
	    f_MinC_regionMax = InterpolateCtx.y_i;

		InterpolateCtx.pY   = pAwbCtx->pFadeParam->pMinCRegionMin;
	    InterpolateCtx.x_i  = pAwbCtx->RgProj;
	    result = Interpolate( &InterpolateCtx );
	    UPDATE_RESULT( result, RET_SUCCESS );
	    f_MinC_regionMin = InterpolateCtx.y_i;

		
		InterpolateCtx.pY	= pAwbCtx->pFadeParam->pMaxYRegionMax;
		InterpolateCtx.x_i	= pAwbCtx->RgProj;
		result = Interpolate( &InterpolateCtx );
		UPDATE_RESULT( result, RET_SUCCESS );
		f_MaxY_regionMax = InterpolateCtx.y_i;

		InterpolateCtx.pY	= pAwbCtx->pFadeParam->pMaxYRegionMin;
		InterpolateCtx.x_i	= pAwbCtx->RgProj;
		result = Interpolate( &InterpolateCtx );
		UPDATE_RESULT( result, RET_SUCCESS );
		f_MaxY_regionMin = InterpolateCtx.y_i;

		InterpolateCtx.pY	= pAwbCtx->pFadeParam->pMinYMaxGRegionMax;
		InterpolateCtx.x_i	= pAwbCtx->RgProj;
		result = Interpolate( &InterpolateCtx );
		UPDATE_RESULT( result, RET_SUCCESS );
		f_MinY_MaxG_regionMax = InterpolateCtx.y_i;

		InterpolateCtx.pY	= pAwbCtx->pFadeParam->pMinYMaxGRegionMin;
		InterpolateCtx.x_i	= pAwbCtx->RgProj;
		result = Interpolate( &InterpolateCtx );
		UPDATE_RESULT( result, RET_SUCCESS );
		f_MinY_MaxG_regionMin = InterpolateCtx.y_i;

		
		InterpolateCtx.pY	= pAwbCtx->pFadeParam->pRefCb;
		InterpolateCtx.x_i	= pAwbCtx->RgProj;
		result = Interpolate( &InterpolateCtx );
		UPDATE_RESULT( result, RET_SUCCESS );
		f_RefCb = InterpolateCtx.y_i;

		InterpolateCtx.pY	= pAwbCtx->pFadeParam->pRefCr;
		InterpolateCtx.x_i	= pAwbCtx->RgProj;
		result = Interpolate( &InterpolateCtx );
		UPDATE_RESULT( result, RET_SUCCESS );
		f_RefCr = InterpolateCtx.y_i;

		f_MinC   	= rint( (double)( pAwbCtx->RegionSize * f_MinC_regionMax + (1.0f - pAwbCtx->RegionSize) * f_MinC_regionMin ) );
		f_MaxY   	= rint( (double)( pAwbCtx->RegionSize * f_MaxY_regionMax + (1.0f - pAwbCtx->RegionSize) * f_MaxY_regionMin ) );
		f_MinY_MaxG = rint( (double)( pAwbCtx->RegionSize * f_MinY_MaxG_regionMax + (1.0f - pAwbCtx->RegionSize) * f_MinY_MaxG_regionMin ) );
		pAwbCtx->MeasWdw.MaxY		= (uint8_t)f_MaxY;	
		pAwbCtx->MeasWdw.MinY_MaxG	= (uint8_t)f_MinY_MaxG;
	}
	else {
		f_RefCr = 132.0f;
		f_RefCb = 128.0f;
	}

    f_CbMin     = rint( (double)( pAwbCtx->RegionSize * f_CbMin_regionMax   + (1.0f - pAwbCtx->RegionSize) * f_CbMin_regionMin ) );
    f_CrMin     = rint( (double)( pAwbCtx->RegionSize * f_CrMin_regionMax   + (1.0f - pAwbCtx->RegionSize) * f_CrMin_regionMin ) );
    f_MaxCSum   = rint( (double)( pAwbCtx->RegionSize * f_MaxCSum_regionMax + (1.0f - pAwbCtx->RegionSize) * f_MaxCSum_regionMin ) );

    /* diagonal shift of the near white region with respect to the middle */
    f_shift = -( f_CrMin + f_CbMin ) / 2.0f;

    // parameters for the AWB measurement with white-point discrimination (near white model)
    pAwbCtx->MeasWdw.RefCr_MaxR = (uint8_t)( f_CrMin + f_shift +  f_RefCr );
    pAwbCtx->MeasWdw.RefCb_MaxB = (uint8_t)( f_CbMin + f_shift + f_RefCb );
    pAwbCtx->MeasWdw.MinC       = (uint8_t)( (float)pAwbCtx->MeasWdw.RefCb_MaxB - f_CbMin );
    pAwbCtx->MeasWdw.MaxCSum    = (uint8_t)f_MaxCSum;


    result = AwbWpCalculateMaxY( &pAwbCtx->MeasWdw );

	#if 1
	if (!pAwbCtx->pFadeParam->regionAdjustEnable){
		pAwbCtx->MeasWdw.MaxY = 230;
		if(pAwbCtx->MeasWdw.MinC  < 12)
			pAwbCtx->MeasWdw.MinC  = 12;

		if(pAwbCtx->MeasWdw.MinC > 15)
			pAwbCtx->MeasWdw.MinC  = 15;
	}
	#endif

    TRACE( AWB_INFO, "%s: fproj(%f)refCr(%d) refCb(%d) minC(%d) MaxCsum(%d) MaxY(%d) MinY(%d) RegionSize(%f)\n", __FUNCTION__,pAwbCtx->RgProj,
    pAwbCtx->MeasWdw.RefCr_MaxR,pAwbCtx->MeasWdw.RefCb_MaxB,pAwbCtx->MeasWdw.MinC,pAwbCtx->MeasWdw.MaxCSum,
    pAwbCtx->MeasWdw.MaxY, pAwbCtx->MeasWdw.MinY_MaxG,pAwbCtx->RegionSize);

    return ( result );
}

 
/******************************************************************************
 * AwbWpRegionAdaptProcessFrame()
 *****************************************************************************/
RESULT AwbWpRegionAdaptProcessFrame
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1. initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    result = AwbWpRegionAdapt( pAwbCtx );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

