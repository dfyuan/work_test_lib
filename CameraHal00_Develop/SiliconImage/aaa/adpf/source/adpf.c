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
 * @file adpf.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <math.h>

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/utl_fixfloat.h>
#include <common/misc.h>

#include <cam_calibdb/cam_calibdb_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_dpf_drv_api.h>
#include <cameric_drv/cameric_isp_flt_drv_api.h>

#include "adpf.h"
#include "adpf_ctrl.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( ADPF_INFO  , "ADPF: ", INFO     , 0 );
CREATE_TRACER( ADPF_WARN  , "ADPF: ", WARNING  , 1 );
CREATE_TRACER( ADPF_ERROR , "ADPF: ", ERROR    , 1 );

CREATE_TRACER( ADPF_DEBUG , "ADPF: ", INFO     , 0 );



/*****************************************************************************/
/**
 * @brief   This array defines the green square radius for the spatial
 *          weight calculation. The square radius values are calculated
 *          by the following formula:  r^2 = (x^2 + y^2)
 */
/*****************************************************************************/
const float fSpatialRadiusSqrG[CAMERIC_DPF_MAX_SPATIAL_COEFFS]  =
{
     2.0f,      /**< r^2 = (1^2 + 1^2) */
     4.0f,      /**< r^2 = (2^2 + 0^2) */
     9.0f,      /**< r^2 = ((2^2 + 2^2) + (3^2 + 1^2)) / 2 */
    16.0f,      /**< r^2 = (4^2 + 0^2) */
    19.0f,      /**< r^2 = ((3^2 + 3^2) + (4^2 + 2^2)) / 2 */
    32.0f       /**< r^2 = (4^2 + 4^2) */
};


/*****************************************************************************/
/**
 * @brief   This array defines the red/blue square radius for the spatial
 *          weight calculation. The square radius values are calculated
 *          by the following formula:  r^2 = (x^2 + y^2)
 */
/*****************************************************************************/
const float fSpatialRadiusSqrRB[CAMERIC_DPF_MAX_SPATIAL_COEFFS] =
{
     4.0f,      /**< r^2 = (2^2 + 0^2) */
     8.0f,      /**< r^2 = (2^2 + 2^2) */
    16.0f,      /**< r^2 = (4^2 + 0^2) */
    20.0f,      /**< r^2 = (4^2 + 2^2) */
    38.0f,      /**< r^2 = ((6^2 + 0^2) + (6^2 + 2^2)) / 2 */
    52.0f       /**< r^2 = (6^2 + 4^2) */
};



/*****************************************************************************/
/**
 * @brief   This is an example/default NLL coefficient configuration for a
 *          static setup of DPF module. The NLL coefficients are selected by
 *          the gain, means is gain in the range of fMinGain to fMaxGain the
 *          coressponding NLL coefficients are programmed.
 */
/*****************************************************************************/
typedef struct CamerIcDpfNoiseLevelLookUpConfig_s
{
    CamerIcDpfNoiseLevelLookUp_t    NLL;
    float                           fMinGain;
    float                           fMaxGain;
} CamerIcDpfNoiseLevelLookUpConfig_t;


#define CAMERIC_DPF_MAX_NLL     8
const CamerIcDpfNoiseLevelLookUpConfig_t CamerIcDpfNllDefault[CAMERIC_DPF_MAX_NLL] =
{
    {
        .fMinGain    = 1.0,
        .fMaxGain    = 1.5,
        .NLL         =
        {
            .NllCoeff   = { 0x3FF, 0x1FF, 0x16B, 0x129, 0x102, 0x0D3,
                            0x0B7, 0x0A3, 0x095, 0x081, 0x074, 0x06A,
                            0x05B, 0x052, 0x04B, 0x045, 0x041 },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 1.5,
        .fMaxGain    = 2.5,
        .NLL         =
        {
            .NllCoeff   = { 0x2D4, 0x2D4, 0x2D4, 0x2D3, 0x292, 0x238,
                            0x1FB, 0x1CE, 0x1AB, 0x178, 0x154, 0x139,
                            0x111, 0x0F6, 0x0E1, 0x0D1, 0x0C4 },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 2.5,
        .fMaxGain    = 3.5,
        .NLL         =
        {
            .NllCoeff   = { 0x24F, 0x24F, 0x24F, 0x24E, 0x219, 0x1D0,
                            0x19E, 0x179, 0x15D, 0x133, 0x116, 0x0FF,
                            0x0DF, 0x0C9, 0x0B8, 0x0AB, 0x0A0 },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 3.5,
        .fMaxGain    = 4.5,
        .NLL         =
        {
            .NllCoeff   = { 0x200, 0x200, 0x200, 0x1FF, 0x1D1, 0x191,
                            0x166, 0x147, 0x12E, 0x10A, 0x0F1, 0x0DD,
                            0x0C1, 0x0AE, 0x09F, 0x094, 0x08B },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 4.5,
        .fMaxGain    = 5.5,
        .NLL         =
        {
            .NllCoeff   = { 0x1CA, 0x1CA, 0x1CA, 0x1C9, 0x1A0, 0x167,
                            0x141, 0x124, 0x10E, 0x0EE, 0x0D7, 0x0C6,
                            0x0AD, 0x09B, 0x08E, 0x084, 0x07C },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 5.5,
        .fMaxGain    = 6.5,
        .NLL         =
        {
            .NllCoeff   = { 0x1A2, 0x1A2, 0x1A2, 0x1A1, 0x17C, 0x148,
                            0x125, 0x10B, 0x0F7, 0x0D9, 0x0C4, 0x0B5,
                            0x09E, 0x08E, 0x082, 0x079, 0x071 },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 6.5,
        .fMaxGain    = 7.5,
        .NLL         =
        {
            .NllCoeff   = { 0x183, 0x183, 0x183, 0x182, 0x160, 0x12F,
                            0x10F, 0x0F7, 0x0E4, 0x0C9, 0x0B6, 0x0A7,
                            0x092, 0x083, 0x078, 0x070, 0x069 },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    },
    {
        .fMinGain    = 7.5,
        .fMaxGain    = 8.5,
        .NLL         =
        {
            .NllCoeff   = { 0x16A, 0x16A, 0x16A, 0x169, 0x149, 0x11C,
                            0x0FD, 0x0E7, 0x0D6, 0x0BC, 0x0AA, 0x09C,
                            0x089, 0x07B, 0x071, 0x069, 0x062 },
            .xScale     = CAMERIC_NLL_SCALE_LOGARITHMIC
        }
    }
};


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

/*****************************************************************************/
/**
 * @brief   This local function prepares a resolution access identifier for
 *          calibration database access.
 *
 * @param   pAdpfCtx        adpf context
 * @param   hCamCalibDb     handle to calibration database to use
 * @param   width           image resolution ( width )
 * @param   height          image resolution ( height )
 * @param   framerate       framerate per second

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpfPrepareCalibDbAccess
(
    AdpfContext_t               *pAdpfCtx,
    const CamCalibDbHandle_t    hCamCalibDb,
    const uint16_t              width,
    const uint16_t              height,
    const uint16_t              framerate
)
{
    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    result = CamCalibDbGetResolutionNameByWidthHeight( hCamCalibDb, width, height, &pAdpfCtx->ResName );
    if ( RET_SUCCESS != result )
    {
        TRACE( ADPF_ERROR, "%s: resolution (%dx%d@%d) not found in database\n", __FUNCTION__, width, height, framerate );
        return ( result );
    }
    TRACE( ADPF_INFO, "%s: resolution = %s\n", __FUNCTION__, pAdpfCtx->ResName );

    pAdpfCtx->hCamCalibDb = hCamCalibDb;

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


/*****************************************************************************/
/**
 * @brief   This local function prepares a resolution access identifier for
 *          calibration database access.
 *
 * @param   pAdpfCtx        adpf context
 * @param   pNll            resulted NLL coefficients

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpfCalculateNllCoefficients
(
    AdpfContext_t                   *pAdpfCtx,

    const float                     fSensorGain,
    CamerIcDpfNoiseLevelLookUp_t    *pNll
)
{
    (void) pAdpfCtx;

    RESULT result = RET_SUCCESS;

    int32_t i;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    // initial check
    if ( fSensorGain < 1.0f )
    {
        return ( RET_INVALID_PARM );
    }

    // loop over the default array
    for ( i = 0; i < CAMERIC_DPF_MAX_NLL; i++ )
    {
        if ( (CamerIcDpfNllDefault[i].fMinGain <= fSensorGain)
                && (fSensorGain < CamerIcDpfNllDefault[i].fMaxGain) )
        {
            int32_t size = sizeof( CamerIcDpfNllDefault[i].NLL.NllCoeff );

            pNll->xScale = CamerIcDpfNllDefault[i].NLL.xScale;
            memcpy( pNll->NllCoeff, CamerIcDpfNllDefault[i].NLL.NllCoeff, size );

            return ( RET_SUCCESS );
        }
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/*****************************************************************************/
/**
 * @brief   This local function calculates the spatial weights for Green
 *          and Red/Blue Pixel of the filter cluster.
 *
 *          s_weight = exp( -1 * (x^2 + y^2) / ( 2 * sigma^2) )
 *
 * @param   pAdpfCtx        adpf context
 * @param   SigmaGreen      sigma value for green pixel
 * @param   SigmaRedBlue    sigma value for red/blue pixel
 * @param   pSpatialG       resulted spatial weights for green
 * @param   pSpatialRB      resulted spatial weights for blue

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpfCalculateSpatialWeights
(
     AdpfContext_t          *pAdpfCtx,
     const uint32_t         SigmaGreen,
     const uint32_t         SigmaRedBlue,

     CamerIcDpfSpatial_t    *pSpatialG,
     CamerIcDpfSpatial_t    *pSpatialRB
)
{
    (void) pAdpfCtx;

    double dSigmaSqr = 0.0f;
    float fExpResult = 0.0f;
    uint32_t i       = 0UL;

    uint32_t sqr1    = 0UL;
    uint32_t sqr2    = 0UL;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    sqr1 = (SigmaGreen * SigmaGreen);
    sqr2 = (SigmaRedBlue * SigmaRedBlue);
    if ( !sqr1 || !sqr2 )
    {
        return ( RET_DIVISION_BY_ZERO );
    }

    // spatial weights for green channel
    dSigmaSqr = (double)(sqr1);
    for ( i=0UL; i<CAMERIC_DPF_MAX_SPATIAL_COEFFS; i++ )
    {
        fExpResult = (float)( 16.0f * exp( -1.0f * (double)fSpatialRadiusSqrG[i] / (2.0f * dSigmaSqr) ) );
        if ( fExpResult > 16.0f )
        {
            // clip to max value
            fExpResult = 16.0f;
        }
        pSpatialG->WeightCoeff[i] = (uint8_t)UtlFloatToFix_U0800( fExpResult );
    }

    // spatial weights for red/blue channel
    dSigmaSqr = (double)(sqr2);
    for ( i=0UL; i<CAMERIC_DPF_MAX_SPATIAL_COEFFS; i++ )
    {
       fExpResult = (float)( 16.0f * exp( -1.0f * (double)fSpatialRadiusSqrRB[i] / (2.0f * dSigmaSqr) ) );
       if ( fExpResult > 16.0f )
       {
           // clip to max value
           fExpResult = 16.0f;
       }
       pSpatialRB->WeightCoeff[i] = (uint8_t)UtlFloatToFix_U0800( fExpResult );
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/*****************************************************************************/
/**
 * @brief   This local function calculates the strength value.
 *
 * @param   pAdpfCtx        adpf context
 * @param   fSensorGain     current sensor gain
 * @param   fGradient       gradient / slope
 * @param   fOffset         additve offset

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpfCalculateStrength
(
    AdpfContext_t           *pAdpfCtx,
    const float             fSensorGain,
    const float             fGradient,
    const float             fOffset,
    const float             fMin,
    const float             fDiv,

    CamerIcDpfInvStrength_t *pDynInvStrength
)
{
    (void) pAdpfCtx;

    float fStrength = 0.0f ;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__ );

    // initial check
    if ( fSensorGain < 1.0f )
    {
        return ( RET_INVALID_PARM );
    }

    fStrength = sqrtf( fGradient * fSensorGain ) + fOffset;
    fStrength = MIN( fMin , fStrength );

    /* The following values show examples:          */
    /* weight=0.251 -> 255, weight=0.5 -> 128,      */
    /* weight=1     ->  64, weight=1.0 *default*    */
    /* weight=1.25  ->  51, weight=1.5 -> 42,       */
    /* weight=1.75  ->  37, weight=2   -> 32        */

    if ( fStrength <= 0.251f )
    {
        /* set to min. strength */
        pDynInvStrength->WeightR = 0x7FU;
        pDynInvStrength->WeightG = 0xFFU;
        pDynInvStrength->WeightB = 0x7FU;
    }
    else if (fStrength >= 128.0f)
    {
        /* set to max. strength - low bandpass */
        pDynInvStrength->WeightR = 0x00U;
        pDynInvStrength->WeightG = 0x00U;
        pDynInvStrength->WeightB = 0x00U;
    }
    else
    {
        /* fStrength is never 0.0f else division by null                */
        /* half the InvStrength of B/R, because no lost of sharpening   */
        pDynInvStrength->WeightR = (uint8_t)UtlFloatToFix_U0800( (fDiv / fStrength) );
        pDynInvStrength->WeightG = (uint8_t)UtlFloatToFix_U0800( (fDiv / fStrength) );
        pDynInvStrength->WeightB = (uint8_t)UtlFloatToFix_U0800( (fDiv / fStrength) );

    }

    TRACE( ADPF_DEBUG, "%s: (gain=%f fStrength=%f, R:%u, G:%u, B:%u)\n",
                __FUNCTION__, fSensorGain, fStrength,
                pDynInvStrength->WeightR, pDynInvStrength->WeightG, pDynInvStrength->WeightB );

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/*****************************************************************************/
/**
 * @brief   This local function calculates the denoise level.
 *
 * @param   pAdpfCtx        adpf context
 * @param   fSensorGain     current sensor gain
 * @param   pDenoiseLevelCurve       denoise  level curve
 * @param   deNoiseLevel         denoise  level

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpfCalculateDenoiseLevel
(
    AdpfContext_t           *pAdpfCtx,
    const float             fSensorGain,
    CamDenoiseLevelCurve_t  * pDenoiseLevelCurve,
    CamerIcIspFltDeNoiseLevel_t * deNoiseLevel
)
{
    (void) pAdpfCtx;

    TRACE( ADPF_INFO, "%s:(enter) fSensorGain(%f) size(%d)\n", __FUNCTION__ ,fSensorGain, pDenoiseLevelCurve->ArraySize);

    // initial check
    if(pDenoiseLevelCurve == NULL)
	{
	  TRACE( ADPF_ERROR, "%s: pDenoiseLevelCurve == NULL \n", __FUNCTION__ );
	  return ( RET_INVALID_PARM );
	}
	
    if ( fSensorGain < 1.0f )
    {
    	
		TRACE( ADPF_ERROR, "%s: fSensorGain(%f)<1.0f\n", __FUNCTION__,fSensorGain );
        return ( RET_INVALID_PARM );
    }
	
	uint16_t n	  = 0U;
	uint16_t nMax = 0U; 		
	float Dgain =	fSensorGain;//Denoise level gain	
	nMax = (pDenoiseLevelCurve->ArraySize- 1U); 		
	/* lower range check */
	if ( Dgain < pDenoiseLevelCurve->pSensorGain[0] )
	{
		Dgain = pDenoiseLevelCurve->pSensorGain[0]; 			
	}
				
	/* upper range check */
	if( Dgain > pDenoiseLevelCurve->pSensorGain[nMax] )
	{
		Dgain = pDenoiseLevelCurve->pSensorGain[nMax];			
	}
				
	/* find x area */
	n = 0;
	while ( (Dgain >=  pDenoiseLevelCurve->pSensorGain[n]) && (n <= nMax) )
	{
		++n;
	}
	--n;

	/**
	 * If n was larger than nMax, which means fSensorGain lies exactly on the 
	* last interval border, we count fSensorGain to the last interval and 
	 * have to decrease n one more time */
	if ( n == nMax )
	{
		--n;
	}
	float sub1=ABS(pDenoiseLevelCurve->pSensorGain[n]-Dgain);			
	float sub2=ABS(pDenoiseLevelCurve->pSensorGain[n+1]-Dgain);
	n	=	sub1<sub2?n:n+1;
	
	*deNoiseLevel	=	pDenoiseLevelCurve->pDlevel[n];
	if(*deNoiseLevel >=  CAMERIC_ISP_FLT_DENOISE_LEVEL_MAX)
		*deNoiseLevel = CAMERIC_ISP_FLT_DENOISE_LEVEL_TEST;

	if(*deNoiseLevel <=  CAMERIC_ISP_FLT_DENOISE_LEVEL_INVALID)
		*deNoiseLevel = CAMERIC_ISP_FLT_DENOISE_LEVEL_0;

	TRACE( ADPF_INFO, "%s: gain=%f,dLelvel=%d\n", __FUNCTION__,Dgain,*deNoiseLevel);
    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/*****************************************************************************/
/**
 * @brief   This local function calculates the strength value.
 *
 * @param   pAdpfCtx        adpf context
 * @param   fSensorGain     current sensor gain
 * @param   pSharpeningLevelCurve       sharpen level curve
 * @param   sharpeningLevel         sharpen level

 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS     function succeed
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
static RESULT AdpfCalculateSharpeningLevel
(
    AdpfContext_t           *pAdpfCtx,
    const float             fSensorGain,
    CamSharpeningLevelCurve_t  * pSharpeningLevelCurve,
    CamerIcIspFltSharpeningLevel_t * sharpeningLevel
)
{
    (void) pAdpfCtx;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__ );

    // initial check
    if(pSharpeningLevelCurve == NULL)
	{
	  TRACE( ADPF_ERROR, "%s: pSharpeningLevelCurve == NULL \n", __FUNCTION__ );
	  return ( RET_INVALID_PARM );
	}
	
    if ( fSensorGain < 1.0f )
    {
    	TRACE( ADPF_ERROR, "%s: fSensorGain  < 1.0f  \n", __FUNCTION__ );
        return ( RET_INVALID_PARM );
    }
	uint16_t n	  = 0U;
	uint16_t nMax = 0U; 		
	float Dgain =	fSensorGain;//sharping level gain	
	nMax = (pSharpeningLevelCurve->ArraySize- 1U); 		
	/* lower range check */
	if ( Dgain < pSharpeningLevelCurve->pSensorGain[0] )
	{
		Dgain = pSharpeningLevelCurve->pSensorGain[0]; 			
	}
				
	/* upper range check */
	if( Dgain > pSharpeningLevelCurve->pSensorGain[nMax] )
	{
		Dgain = pSharpeningLevelCurve->pSensorGain[nMax];			
	}
				
	/* find x area */
	n = 0;
	while ( (Dgain >=  pSharpeningLevelCurve->pSensorGain[n]) && (n <= nMax) )
	{
		++n;
	}
	--n;
				
	/**
	 * If n was larger than nMax, which means fSensorGain lies exactly on the 
	* last interval border, we count fSensorGain to the last interval and 
	 * have to decrease n one more time */
	if ( n == nMax )
	{
		--n;
	}
	float sub1=ABS(pSharpeningLevelCurve->pSensorGain[n]-Dgain);			
	float sub2=ABS(pSharpeningLevelCurve->pSensorGain[n+1]-Dgain);
	n	=	sub1<sub2?n:n+1;
	
	*sharpeningLevel	=	pSharpeningLevelCurve->pSlevel[n];
	if(*sharpeningLevel >  CAMERIC_ISP_FLT_SHARPENING_LEVEL_MAX)
		*sharpeningLevel = CAMERIC_ISP_FLT_SHARPENING_LEVEL_10;

	if(*sharpeningLevel <  CAMERIC_ISP_FLT_SHARPENING_LEVEL_INVALID)
		*sharpeningLevel = CAMERIC_ISP_FLT_SHARPENING_LEVEL_0;

	TRACE( ADPF_INFO, "%s: gain=%f,sLelvel=%d\n", __FUNCTION__,Dgain, *sharpeningLevel);
    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AdpfApplyConfiguration()
 *****************************************************************************/
static RESULT AdpfApplyConfiguration
(
    AdpfContext_t   *pAdpfCtx,
    AdpfConfig_t    *pConfig
)
{
    RESULT result = RET_SUCCESS;

    CamerIcDpfConfig_t  DpfConfig;
    CamerIcGains_t      NfGains;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    // clear
    MEMSET( &DpfConfig, 0, sizeof( DpfConfig ) );
    MEMSET( &NfGains, 0, sizeof( NfGains ) );

    pAdpfCtx->hCamerIc      = pConfig->hCamerIc;
    pAdpfCtx->hSubCamerIc   = pConfig->hSubCamerIc;

    // configuration with data from calibration database
    if ( pConfig->type == ADPF_USE_CALIB_DATABASE )
    {
        CamDpfProfile_t *pDpfProfile = NULL;

        // initial check
        if ( NULL == pConfig->data.db.hCamCalibDb )
        {
            return ( RET_INVALID_PARM );
        }

        // initialize calibration database access
        result = AdpfPrepareCalibDbAccess( pAdpfCtx,
                                            pConfig->data.db.hCamCalibDb,
                                            pConfig->data.db.width,
                                            pConfig->data.db.height,
                                            pConfig->data.db.framerate );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPF_ERROR,  "%s: Can't prepare database access\n",  __FUNCTION__ );
            return ( result );
        }

        // get dpf-profile from calibration database
        result = CamCalibDbGetDpfProfileByResolution( pAdpfCtx->hCamCalibDb, pAdpfCtx->ResName, &pDpfProfile );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPF_ERROR,  "%s: Getting DPF profile for resolution %s from calibration database failed (%d)\n",
                                    __FUNCTION__, pAdpfCtx->ResName, result );
            return ( result );
        }
        DCT_ASSERT( NULL != pDpfProfile );

        // initialize Adpf context with values from calibration database
        pAdpfCtx->gain                  = pConfig->fSensorGain;
        pAdpfCtx->SigmaGreen            = pDpfProfile->SigmaGreen;
        pAdpfCtx->SigmaRedBlue          = pDpfProfile->SigmaRedBlue;
        pAdpfCtx->fGradient             = pDpfProfile->fGradient;
        pAdpfCtx->fOffset               = pDpfProfile->fOffset;
        pAdpfCtx->fMin                  = 2.0f;
        pAdpfCtx->fDiv                  = 64.0f;

        pAdpfCtx->NfGains.fRed          = pDpfProfile->NfGains.fCoeff[CAM_4CH_COLOR_COMPONENT_RED];
        pAdpfCtx->NfGains.fGreenR       = pDpfProfile->NfGains.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENR];
        pAdpfCtx->NfGains.fGreenB       = pDpfProfile->NfGains.fCoeff[CAM_4CH_COLOR_COMPONENT_GREENB];
        pAdpfCtx->NfGains.fBlue         = pDpfProfile->NfGains.fCoeff[CAM_4CH_COLOR_COMPONENT_BLUE];

        pAdpfCtx->Mfd.enable           	= pDpfProfile->Mfd.enable;
		pAdpfCtx->Mfd.gain[0]           = pDpfProfile->Mfd.gain[0];
		pAdpfCtx->Mfd.gain[1]           = pDpfProfile->Mfd.gain[1];
		pAdpfCtx->Mfd.gain[2]           = pDpfProfile->Mfd.gain[2];

        pAdpfCtx->Mfd.frames[0]			= pDpfProfile->Mfd.frames[0];
		pAdpfCtx->Mfd.frames[1]         = pDpfProfile->Mfd.frames[1];
		pAdpfCtx->Mfd.frames[2]         = pDpfProfile->Mfd.frames[2];

        pAdpfCtx->Uvnr.enable           = pDpfProfile->Uvnr.enable;
        pAdpfCtx->Uvnr.gain[0]          = pDpfProfile->Uvnr.gain[0];
        pAdpfCtx->Uvnr.gain[1]          = pDpfProfile->Uvnr.gain[1];
		pAdpfCtx->Uvnr.gain[2]          = pDpfProfile->Uvnr.gain[2];

        pAdpfCtx->Uvnr.ratio[0]			= pDpfProfile->Uvnr.ratio[0];
        pAdpfCtx->Uvnr.ratio[1]         = pDpfProfile->Uvnr.ratio[1];
		pAdpfCtx->Uvnr.ratio[2]         = pDpfProfile->Uvnr.ratio[2];

        pAdpfCtx->Uvnr.distances[0]		= pDpfProfile->Uvnr.distances[0];
        pAdpfCtx->Uvnr.distances[1]     = pDpfProfile->Uvnr.distances[1];
		pAdpfCtx->Uvnr.distances[2]     = pDpfProfile->Uvnr.distances[2];

		pAdpfCtx->DenoiseLevelCurve 	= pDpfProfile->DenoiseLevelCurve;
		pAdpfCtx->SharpeningLevelCurve 	= pDpfProfile->SharpeningLevelCurve;

		if(pDpfProfile->FilterEnable >= 1.0){
			pAdpfCtx->FlterEnable = 1.0;
		}else{
			pAdpfCtx->FlterEnable = 0.0;
		}

        switch ( pDpfProfile->nll_segmentation )
        {
            case 0U:
            {
                pAdpfCtx->Nll.xScale = CAMERIC_NLL_SCALE_LINEAR;
                break;
            }

            case 1U:
            {
                pAdpfCtx->Nll.xScale = CAMERIC_NLL_SCALE_LOGARITHMIC;
                break;
            }

            default:
            {
                TRACE( ADPF_ERROR,  "%s: NLL x-scale not supported (%d)\n", __FUNCTION__, pDpfProfile->nll_segmentation );
                return ( RET_OUTOFRANGE );
            }
        }

        for ( int32_t i = 0; i<CAMERIC_DPF_MAX_NLF_COEFFS; i++ )
        {
            pAdpfCtx->Nll.NllCoeff[i] = ( pDpfProfile->nll_coeff.uCoeff[i] >> 2 );
        }
    }
    // configuration with default configuration data
    else if ( pConfig->type == ADPF_USE_DEFAULT_CONFIG )
    {
        // initialize Adpf context with values from calibration database
        pAdpfCtx->gain                  = pConfig->fSensorGain;
        pAdpfCtx->SigmaGreen            = pConfig->data.def.SigmaGreen;
        pAdpfCtx->SigmaRedBlue          = pConfig->data.def.SigmaRedBlue;
        pAdpfCtx->fGradient             = pConfig->data.def.fGradient;
        pAdpfCtx->fOffset               = pConfig->data.def.fOffset;
        pAdpfCtx->fMin                  = pConfig->data.def.fMin;
        pAdpfCtx->fDiv                  = pConfig->data.def.fDiv;

        pAdpfCtx->NfGains.fRed          = pConfig->data.def.NfGains.fRed;
        pAdpfCtx->NfGains.fGreenR       = pConfig->data.def.NfGains.fGreenR;
        pAdpfCtx->NfGains.fGreenB       = pConfig->data.def.NfGains.fGreenB;
        pAdpfCtx->NfGains.fBlue         = pConfig->data.def.NfGains.fBlue;

        result = AdpfCalculateNllCoefficients( pAdpfCtx, pConfig->fSensorGain, &pAdpfCtx->Nll );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        int32_t i;
        for ( i = 0; i<CAMERIC_DPF_MAX_NLF_COEFFS; i++ )
        {
            pAdpfCtx->Nll.NllCoeff[i] >>= 2;
        }
    }
    else
    {
        TRACE( ADPF_ERROR,  "%s: unsupported ADPF configuration\n",  __FUNCTION__ );
        return ( RET_OUTOFRANGE );
    }

    // initialize CamerIc driver dpf-config
    DpfConfig.GainUsage             = CAMERIC_DPF_GAIN_USAGE_AWB_LSC_GAINS;
    DpfConfig.RBFilterSize          = CAMERIC_DPF_RB_FILTERSIZE_13x9;
    DpfConfig.ProcessRedPixel       = BOOL_TRUE;
    DpfConfig.ProcessGreenRPixel    = BOOL_TRUE;
    DpfConfig.ProcessGreenBPixel    = BOOL_TRUE;
    DpfConfig.ProcessBluePixel      = BOOL_TRUE;

    // compute initial spatial weights
    result = AdpfCalculateSpatialWeights( pAdpfCtx,
                                            pAdpfCtx->SigmaGreen,
                                            pAdpfCtx->SigmaRedBlue,
                                            &DpfConfig.SpatialG,
                                            &DpfConfig.SpatialRB );
    if ( result != RET_SUCCESS )
    {
        TRACE( ADPF_ERROR,  "%s: Initial calcultion of spatial weights failed (%d)\n",  __FUNCTION__, result );
        return ( result );
    }

    // apply dpf config to cameric dpf driver
    result =  CamerIcIspDpfConfig( pAdpfCtx->hCamerIc, &DpfConfig );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    if ( NULL != pAdpfCtx->hSubCamerIc )
    {
        result =  CamerIcIspDpfConfig( pAdpfCtx->hSubCamerIc, &DpfConfig );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    // convert noise function gains to fix point
    result = AdpfGains2CamerIcGains( &pAdpfCtx->NfGains, &NfGains );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    // apply dpf nl-gains to cameric dpf driver
    result = CamerIcIspDpfSetNoiseFunctionGain( pAdpfCtx->hCamerIc, &NfGains );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    if ( NULL != pAdpfCtx->hSubCamerIc )
    {
        result = CamerIcIspDpfSetNoiseFunctionGain( pAdpfCtx->hSubCamerIc, &NfGains );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    // apply dpf nl-lockup to cameric dpf driver
    result = CamerIcIspDpfSetNoiseLevelLookUp( pAdpfCtx->hCamerIc, &pAdpfCtx->Nll );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    if ( NULL != pAdpfCtx->hSubCamerIc )
    {
        result = CamerIcIspDpfSetNoiseLevelLookUp( pAdpfCtx->hSubCamerIc, &pAdpfCtx->Nll );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    // caluclate init strength
    result = AdpfCalculateStrength( pAdpfCtx,
                                        pConfig->fSensorGain,
                                        pAdpfCtx->fGradient,
                                        pAdpfCtx->fOffset,
                                        pAdpfCtx->fMin,
                                        pAdpfCtx->fDiv,
                                        &pAdpfCtx->DynInvStrength );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    // apply dynamic strength to cameric dpf driver
    result = CamerIcIspDpfSetStrength( pAdpfCtx->hCamerIc, &pAdpfCtx->DynInvStrength);
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    if ( NULL != pAdpfCtx->hSubCamerIc )
    {
        result = CamerIcIspDpfSetStrength( pAdpfCtx->hSubCamerIc, &pAdpfCtx->DynInvStrength);
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    // save configuration
    pAdpfCtx->Config = *pConfig;

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * Implementation of AEC API Functions
 *****************************************************************************/

/******************************************************************************
 * AdpfGains2CamerIcGains()
 *****************************************************************************/
RESULT AdpfGains2CamerIcGains
(
    AdpfGains_t     *pAdpfGains,
    CamerIcGains_t  *pCamerIcGains
)
{
    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (pAdpfGains != NULL) && (pCamerIcGains != NULL) )
    {
        if ( (pAdpfGains->fRed > 15.995f)
                || (pAdpfGains->fBlue > 15.995f)
                || (pAdpfGains->fGreenR > 15.995f)
                || (pAdpfGains->fGreenB > 15.995f) )
        {
            result = RET_OUTOFRANGE;
        }
        else
        {
            pCamerIcGains->Red      = UtlFloatToFix_U0408( pAdpfGains->fRed );
            pCamerIcGains->GreenR   = UtlFloatToFix_U0408( pAdpfGains->fGreenR );
            pCamerIcGains->GreenB   = UtlFloatToFix_U0408( pAdpfGains->fGreenB );
            pCamerIcGains->Blue     = UtlFloatToFix_U0408( pAdpfGains->fBlue );
        }
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcGains2AdpfGains()
 *****************************************************************************/
RESULT CamerIcGains2AdpfGains
(
    CamerIcGains_t  *pCamerIcGains,
    AdpfGains_t     *pAdpfGains
)
{
    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (pAdpfGains != NULL) && (pCamerIcGains != NULL) )
    {
        pAdpfGains->fRed      = UtlFixToFloat_U0408( pCamerIcGains->Red );
        pAdpfGains->fGreenR   = UtlFixToFloat_U0408( pCamerIcGains->GreenR );
        pAdpfGains->fGreenB   = UtlFixToFloat_U0408( pCamerIcGains->GreenB );
        pAdpfGains->fBlue     = UtlFixToFloat_U0408( pCamerIcGains->Blue );
    }
    else
    {
        result = RET_NULL_POINTER;
    }

    TRACE( ADPF_INFO, "%s: (exit %d)\n", __FUNCTION__, result );

    return ( result );
}



/******************************************************************************
 * AdpfInit()
 *****************************************************************************/
RESULT AdpfInit
(
    AdpfInstanceConfig_t *pInstConfig
)
{
    RESULT result = RET_SUCCESS;

    AdpfContext_t *pAdpfCtx;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pInstConfig )
    {
        return ( RET_INVALID_PARM );
    }

    /* allocate auto exposure control context */
    pAdpfCtx = (AdpfContext_t *)malloc( sizeof(AdpfContext_t) );
    if ( NULL == pAdpfCtx )
    {
        TRACE( ADPF_ERROR,  "%s: Can't allocate ADPF context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    /* pre-initialize context */
    MEMSET( pAdpfCtx, 0, sizeof(*pAdpfCtx) );
    pAdpfCtx->state         = ADPF_STATE_INITIALIZED;

    /* return handle */
    pInstConfig->hAdpf = (AdpfHandle_t)pAdpfCtx;

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfRelease()
 *****************************************************************************/
RESULT AdpfRelease
(
    AdpfHandle_t handle
)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpfCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check state */
    if ( (ADPF_STATE_RUNNING == pAdpfCtx->state) ||
         (ADPF_STATE_LOCKED == pAdpfCtx->state)  )
    {
        return ( RET_BUSY );
    }

    MEMSET( pAdpfCtx, 0, sizeof(AdpfContext_t) );
    free ( pAdpfCtx );

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AdpfConfigure()
 *****************************************************************************/
RESULT AdpfConfigure
(
    AdpfHandle_t handle,
    AdpfConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AdpfContext_t *pAdpfCtx = (AdpfContext_t*) handle;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpfCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (ADPF_STATE_INITIALIZED != pAdpfCtx->state) &&
         (ADPF_STATE_STOPPED != pAdpfCtx->state)     )
    {
        return ( RET_WRONG_STATE );
    }

    /* apply new configuration */
    result = AdpfApplyConfiguration( pAdpfCtx, pConfig );
    if ( result != RET_SUCCESS )
    {
        TRACE( ADPF_ERROR,  "%s: Can't configure CamerIc DPF (%d)\n",  __FUNCTION__, result );
        return ( result );
    }

    /* save configuration into context */
    pAdpfCtx->Config = *pConfig;
    pAdpfCtx->state = ADPF_STATE_STOPPED;

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfReConfigure()
 *****************************************************************************/
RESULT AdpfReConfigure
(
    AdpfHandle_t handle,
    AdpfConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpfCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pConfig )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (ADPF_STATE_LOCKED != pAdpfCtx->state)  && 
         (ADPF_STATE_RUNNING != pAdpfCtx->state) )
    {
        // check if we switched the resolution
        if ( ( ADPF_USE_CALIB_DATABASE == pConfig->type ) && 
                ( ( pConfig->data.db.width != pAdpfCtx->Config.data.db.width )            ||
                  ( pConfig->data.db.height != pAdpfCtx->Config.data.db.height )          ||
                  ( pConfig->data.db.hCamCalibDb != pAdpfCtx->Config.data.db.hCamCalibDb) )
           )
        {
            // apply config
            result = AdpfApplyConfiguration( pAdpfCtx, pConfig );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPF_ERROR,  "%s: Can't reconfigure CamerIc DPF (%d)\n",  __FUNCTION__, result );
                return ( result );
            }

            // save configuration
            pAdpfCtx->Config = *pConfig;
        }
        else if ( ADPF_USE_DEFAULT_CONFIG == pConfig->type )
        {
            // apply config
            result = AdpfApplyConfiguration( pAdpfCtx, pConfig );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPF_ERROR,  "%s: Can't reconfigure CamerIc DPF (%d)\n",  __FUNCTION__, result );
                return ( result );
            }

            // save configuration
            pAdpfCtx->Config = *pConfig;
        }

        result = RET_SUCCESS;
    }
    else if ( ADPF_STATE_STOPPED != pAdpfCtx->state )
    {
        result = RET_SUCCESS;
    }
    else
    {
        result = RET_WRONG_STATE;
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfStart()
 *****************************************************************************/
RESULT AdpfStart
(
    AdpfHandle_t handle
)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    RESULT result;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpfCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (ADPF_STATE_RUNNING == pAdpfCtx->state) ||
         (ADPF_STATE_LOCKED == pAdpfCtx->state)  )
    {
        result = RET_BUSY;
    }
    else if ( (ADPF_STATE_STOPPED == pAdpfCtx->state)     ||
              (ADPF_STATE_INITIALIZED == pAdpfCtx->state) )
    {
        /* enable cameric driver instance(s) */
        result = CamerIcIspDpfEnable( pAdpfCtx->hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPF_ERROR,  "%s: Can't enable CamerIc DPF (%d)\n",  __FUNCTION__, result );
            return ( result );
        }
        if ( NULL != pAdpfCtx->hSubCamerIc )
        {
            result = CamerIcIspDpfEnable( pAdpfCtx->hSubCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPF_ERROR,  "%s: Can't enable 2nd CamerIc DPF (%d)\n",  __FUNCTION__, result );
                return ( result );
            }
        }

        pAdpfCtx->state = ADPF_STATE_RUNNING;
    }
    else
    {
        result = RET_WRONG_STATE;
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfStop()
 *****************************************************************************/
RESULT AdpfStop
(
    AdpfHandle_t handle
)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    RESULT result;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( NULL == pAdpfCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* before stopping, unlock the ADPF if locked */
    if ( ADPF_STATE_LOCKED == pAdpfCtx->state )
    {
        result = RET_BUSY;
    }
    else if ( ADPF_STATE_RUNNING == pAdpfCtx->state )
    {
        /* disable cameric driver instance(s) */
        result = CamerIcIspDpfDisable( pAdpfCtx->hCamerIc );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPF_ERROR,  "%s: Can't disable CamerIc DPF (%d)\n",  __FUNCTION__, result );
            return ( result );
        }
        if ( NULL != pAdpfCtx->hSubCamerIc )
        {
            result = CamerIcIspDpfDisable( pAdpfCtx->hSubCamerIc );
            if ( result != RET_SUCCESS )
            {
                TRACE( ADPF_ERROR,  "%s: Can't disable 2nd CamerIc DPF (%d)\n",  __FUNCTION__, result );
                return ( result );
            }
        }
    
        pAdpfCtx->state = ADPF_STATE_STOPPED;
    }
    else if ( (ADPF_STATE_STOPPED == pAdpfCtx->state)     ||
              (ADPF_STATE_INITIALIZED == pAdpfCtx->state) )

    {
        result = RET_SUCCESS;
    }
    else
    {
        result = RET_WRONG_STATE;
    }


    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfGetCurrentConfig()
 *****************************************************************************/
RESULT AdpfGetCurrentConfig
(
    AdpfHandle_t handle,
    AdpfConfig_t *pConfig
)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if (NULL == pConfig)
    {
        return ( RET_NULL_POINTER );
    }

    // return config
    MEMSET( pConfig, 0, sizeof( AdpfConfig_t ) );

    pConfig->fSensorGain = pAdpfCtx->gain;
    pConfig->type = ADPF_USE_DEFAULT_CONFIG;

    pConfig->data.def.SigmaGreen = pAdpfCtx->SigmaGreen;
    pConfig->data.def.SigmaRedBlue = pAdpfCtx->SigmaRedBlue;
    pConfig->data.def.fGradient = pAdpfCtx->fGradient;
    pConfig->data.def.fOffset = pAdpfCtx->fOffset;
    pConfig->data.def.fMin = pAdpfCtx->fMin;
    pConfig->data.def.fDiv = pAdpfCtx->fDiv;

    pConfig->data.def.NfGains.fRed = pAdpfCtx->NfGains.fRed;
    pConfig->data.def.NfGains.fGreenR = pAdpfCtx->NfGains.fGreenR;
    pConfig->data.def.NfGains.fGreenB = pAdpfCtx->NfGains.fGreenB;
    pConfig->data.def.NfGains.fBlue = pAdpfCtx->NfGains.fBlue;

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfStatus()
 *****************************************************************************/
RESULT AdpfStatus
(
    AdpfHandle_t handle,
    bool_t       *pRunning
)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pRunning == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pRunning = ( (pAdpfCtx->state == ADPF_STATE_RUNNING)
                    || (pAdpfCtx->state == ADPF_STATE_LOCKED) ) ? BOOL_TRUE : BOOL_FALSE;

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AdpfProcessFrame()
 *****************************************************************************/
RESULT AdpfProcessFrame
(
    AdpfHandle_t    handle,
    const float     gain
)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;

    RESULT result = RET_SUCCESS;

    float dgain = 0.0f; /* gain difference */

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ADPF_STATE_RUNNING == pAdpfCtx->state )
    {
        dgain = ( gain > pAdpfCtx->gain ) ? (gain - pAdpfCtx->gain) : ( pAdpfCtx->gain -gain);
        if ( dgain > 0.15f )
        {
            /* caluclate new strength */
            result = AdpfCalculateStrength( pAdpfCtx, gain, pAdpfCtx->fGradient, pAdpfCtx->fOffset, pAdpfCtx->fMin, pAdpfCtx->fDiv, &pAdpfCtx->DynInvStrength );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

            /* program strength */
            result = CamerIcIspDpfSetStrength( pAdpfCtx->hCamerIc, &pAdpfCtx->DynInvStrength);
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

            if ( NULL != pAdpfCtx->hSubCamerIc )
            {
                result = CamerIcIspDpfSetStrength( pAdpfCtx->hSubCamerIc, &pAdpfCtx->DynInvStrength);
                RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
            }

            pAdpfCtx->gain = gain;
        }
        else
        {
            result = RET_CANCELED;
        }

		if(pAdpfCtx->FlterEnable >= 1.0)
		{
			/*caluclate denoise level*/
			CamerIcIspFltDeNoiseLevel_t deNoiseLevel = pAdpfCtx->denoise_level;
			result = AdpfCalculateDenoiseLevel(pAdpfCtx,gain,&pAdpfCtx->DenoiseLevelCurve,&pAdpfCtx->denoise_level);
			RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );	
				
			/*calucate sharnping level*/
			CamerIcIspFltSharpeningLevel_t sharpningLevel = pAdpfCtx->sharp_level;
			result = AdpfCalculateSharpeningLevel(pAdpfCtx,gain,&pAdpfCtx->SharpeningLevelCurve,&pAdpfCtx->sharp_level);
			RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

			TRACE( ADPF_INFO, "%s:filterenable(%f) gain(%f) denoise %d, sharp %d", __func__,pAdpfCtx->FlterEnable, gain, pAdpfCtx->denoise_level, pAdpfCtx->sharp_level);
			if ((deNoiseLevel != pAdpfCtx->denoise_level) || (sharpningLevel != pAdpfCtx->sharp_level)){
				result = CamerIcIspFltSetFilterParameter(pAdpfCtx->hCamerIc, pAdpfCtx->denoise_level, pAdpfCtx->sharp_level);
				RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
			}
		}			
    }
    else
    {
        result = RET_CANCELED ;
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

/******************************************************************************
 * AdpfGetMfdGain()
 *****************************************************************************/
RESULT AdpfGetMfdGain
(
    AdpfHandle_t    handle,

    char			*mfd_enable,
    float			mfd_gain[],
    float			mfd_frames[]

)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;
	CamDpfProfile_t *pDpfProfile = NULL;

    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ADPF_STATE_RUNNING == pAdpfCtx->state )
    {
        result = CamCalibDbGetDpfProfileByResolution( pAdpfCtx->hCamCalibDb, pAdpfCtx->ResName, &pDpfProfile );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPF_ERROR,  "%s: Getting DPF profile for resolution %s from calibration database failed (%d)\n",
                                    __FUNCTION__, pAdpfCtx->ResName, result );
            return ( result );
        }
        DCT_ASSERT( NULL != pDpfProfile );

		*mfd_enable = pDpfProfile->Mfd.enable;
        mfd_gain[0] = pDpfProfile->Mfd.gain[0];
		mfd_gain[1] = pDpfProfile->Mfd.gain[1];
		mfd_gain[2] = pDpfProfile->Mfd.gain[2];
		mfd_frames[0] = pDpfProfile->Mfd.frames[0];
		mfd_frames[1] = pDpfProfile->Mfd.frames[1];
		mfd_frames[2] = pDpfProfile->Mfd.frames[2];

		TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);
    }
    else
    {
        result = RET_CANCELED ;
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );

}


/******************************************************************************
 * AdpfGetUvnrPara()
 *****************************************************************************/
RESULT AdpfGetUvnrPara
(
    AdpfHandle_t    handle,

    char			*uvnr_enable,
    float			uvnr_gain[],
    float			uvnr_ratio[],
    float			uvnr_distances[]

)
{
    AdpfContext_t *pAdpfCtx = (AdpfContext_t *)handle;
	CamDpfProfile_t *pDpfProfile = NULL;

    RESULT result = RET_SUCCESS;

    TRACE( ADPF_INFO, "%s: (enter)\n", __FUNCTION__);

    /* initial checks */
    if ( pAdpfCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ADPF_STATE_RUNNING == pAdpfCtx->state )
    {
        result = CamCalibDbGetDpfProfileByResolution( pAdpfCtx->hCamCalibDb, pAdpfCtx->ResName, &pDpfProfile );
        if ( result != RET_SUCCESS )
        {
            TRACE( ADPF_ERROR,  "%s: Getting DPF profile for resolution %s from calibration database failed (%d)\n",
                                    __FUNCTION__, pAdpfCtx->ResName, result );
            return ( result );
        }
        DCT_ASSERT( NULL != pDpfProfile );

		*uvnr_enable = pDpfProfile->Uvnr.enable;
		uvnr_gain[0] = pDpfProfile->Uvnr.gain[0];
		uvnr_gain[1] = pDpfProfile->Uvnr.gain[1];
		uvnr_gain[2] = pDpfProfile->Uvnr.gain[2];

		uvnr_ratio[0] = pDpfProfile->Uvnr.ratio[0];
		uvnr_ratio[1] = pDpfProfile->Uvnr.ratio[1];
		uvnr_ratio[2] = pDpfProfile->Uvnr.ratio[2];

		uvnr_distances[0] = pDpfProfile->Uvnr.distances[0];
		uvnr_distances[1] = pDpfProfile->Uvnr.distances[1];
		uvnr_distances[2] = pDpfProfile->Uvnr.distances[2];

		TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);
    }
    else
    {
        result = RET_CANCELED ;
    }

    TRACE( ADPF_INFO, "%s: (exit)\n", __FUNCTION__);
    return ( result );

}


