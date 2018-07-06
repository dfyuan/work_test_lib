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
 * @file wbgain.c
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

#include "awb.h"
#include "awb_ctrl.h"

#include "wbgain.h"
#include "interpolate.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( AWB_INFO  );
USE_TRACER( AWB_WARN  );
USE_TRACER( AWB_ERROR );

USE_TRACER( AWB_DEBUG );


/**< limits for out of range tests. */
#define DIVMIN     0.00001f  /**< lowest denominator for divisions  */
#define LOGMIN     0.0001f   /**< lowest value for logarithmic calculations */


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

/******************************************************************************
 * AwbWbGainOutOfRangeCheck()
 *****************************************************************************/
RESULT AwbWbGainOutOfRangeCheck
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    InterpolateCtx_t InterpolateCtx;

    bool_t *pWbGainsOutOfRange     = &pAwbCtx->WbGainsOutOfRange;

    float f_globalGainDistance1 = 0.0f;
    float f_globalGainDistance2 = 0.0f;

    float f_Rg = 0.0f;          /**< shortcur for red-gain/green-gain */
    float f_Bg = 0.0f;          /**< shortcur for red-gain/green-gain */
    float f_s  = 0.0f;
    float f_RgProj = 0.0f;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1.) initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE ); 
    }

    if ( (pAwbCtx->pCenterLine == NULL) || (pAwbCtx->pGlobalFadeParam == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    /* 2.) initialize shortcut */
    f_Rg = pAwbCtx->WbGainsOverG.GainROverG; 
    f_Bg = pAwbCtx->WbGainsOverG.GainBOverG; 
 
    /* 3.) calculate distance of current gain point from line
     *     scalar product of center line normal with (f_Rg, f_Bg) 
     *     minus distance of centerline to (0,0) */
    f_s = pAwbCtx->pCenterLine->f_N0_Rg * f_Rg + pAwbCtx->pCenterLine->f_N0_Bg * f_Bg - pAwbCtx->pCenterLine->f_d;
      
    /* 4.) calculate Rg coordinate of projected point by substraction
     *     of distance in center line normal direction from original f_Rg */
    f_RgProj = -f_s * pAwbCtx->pCenterLine->f_N0_Rg + f_Rg;
 
    /* 5.1) get distance of upper out-of-range curve from center line */
    InterpolateCtx.size = pAwbCtx->pGlobalFadeParam->ArraySize1;
    InterpolateCtx.pX   = pAwbCtx->pGlobalFadeParam->pGlobalFade1;
    InterpolateCtx.pY   = pAwbCtx->pGlobalFadeParam->pGlobalGainDistance1;
    InterpolateCtx.x_i  = f_RgProj;
    result = Interpolate( &InterpolateCtx );
    if ( result == RET_OUTOFRANGE )
    {
        *pWbGainsOutOfRange = BOOL_TRUE;
        return ( RET_SUCCESS );
    }
    else if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    f_globalGainDistance1 = InterpolateCtx.y_i;

    /* 5.2) get distance of lower out-of-range curve from center line use same f_RgProj for f_xi */
    InterpolateCtx.size = pAwbCtx->pGlobalFadeParam->ArraySize2;
    InterpolateCtx.pX   = pAwbCtx->pGlobalFadeParam->pGlobalFade2;
    InterpolateCtx.pY   = pAwbCtx->pGlobalFadeParam->pGlobalGainDistance2;
    InterpolateCtx.x_i  = f_RgProj;
    result = Interpolate( &InterpolateCtx );
    if ( result == RET_OUTOFRANGE )
    {
        *pWbGainsOutOfRange = BOOL_TRUE;
        return ( RET_SUCCESS );
    }
    else if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    f_globalGainDistance2 = -InterpolateCtx.y_i;

    /* 6. evaluate results from above */
    /* do not rely on AWB measurements for dominant colored scenes like grass, sky etc.
     * compare distance of current gain with upper/lower out-of-range curve. also compare
     * projection point against MaxSky threshold. */
    TRACE( AWB_DEBUG, "f_s=%f, (%f %f), f_RgProj=%f  f_Rg(%f) f_Bg(%f)\n", f_s, f_globalGainDistance1, f_globalGainDistance2, f_RgProj,f_Rg,f_Bg );
    if ( (f_s > f_globalGainDistance1) || (f_s < f_globalGainDistance2) || (f_RgProj > pAwbCtx->RgProjMaxSky) )
    {
        /* out of range */
        *pWbGainsOutOfRange = BOOL_TRUE;

        if ( f_RgProj > pAwbCtx->RgProjMaxSky )
        {
            TRACE( AWB_DEBUG, "WB OOR check: SKY THRESHOLD EXCEEDED (%f > %f)\n", f_RgProj, pAwbCtx->RgProjMaxSky );
        }
        else
        {
            TRACE( AWB_DEBUG, "WB OOR check: OBTAINED AWB GAINS TOO FAR FROM TEMP. CURVE\n" );
            TRACE( AWB_DEBUG, "f_s=%f, (%f %f), f_RgProj=%f \n", f_s, f_globalGainDistance1, f_globalGainDistance2, f_RgProj );
        }
    }
    else
    {
        /* inside range, Wb can go on. */
        *pWbGainsOutOfRange = BOOL_FALSE;
    }
 
    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbWbGainClip()
 *****************************************************************************/
RESULT AwbWbGainClip
( 
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    InterpolateCtx_t InterpolateCtx;

    float f_s_Max1      = 0.0f;     /**< maximal allowed distance in upper right plane of current gain point from line -> calculated */
    float f_s_Max2      = 0.0f;     /**< maximal allowed distance in lower left plane of current gain point from line -> calculated */
    float f_RgProj_Min  = 0.0f;     /**< minimal allowed Rg value for projected point -> calculated */
    float f_s           = 0.0f;

    float f_Rg          = 0.0f;
    float f_Bg          = 0.0f;

    float f_RgProj      = 0.0f;

    bool_t *pRgProjClippedToOutDoorMin = &pAwbCtx->RgProjClippedToOutDoorMin;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1.) initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE ); 
    }

    if ( (pAwbCtx->pCenterLine == NULL) || (pAwbCtx->pGlobalFadeParam == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    /* 2.) initialize shortcut */
    f_Rg = pAwbCtx->WbGainsOverG.GainROverG; 
    f_Bg = pAwbCtx->WbGainsOverG.GainBOverG; 

    *pRgProjClippedToOutDoorMin = BOOL_FALSE;
 
    /* 3.) calculate distance of current gain point from line
     *     scalar product of center line normal with (f_Rg, f_Bg) minus distance of centerline to (0,0) 
     *     Note: that the result is different from OOR-check because of intermediate damping step. */
    f_s = pAwbCtx->pCenterLine->f_N0_Rg * f_Rg + pAwbCtx->pCenterLine->f_N0_Bg * f_Bg - pAwbCtx->pCenterLine->f_d;
  
    /* 4.) calculate Rg coordinate of projected point by substraction of distance 
     * in center line normal direction from original f_Rg */
    f_RgProj = -f_s * pAwbCtx->pCenterLine->f_N0_Rg + f_Rg;

    /* the projection is compared to a minimum and a maximum threshold and clipped to these thresholds if smaller/greater */

    /* 5.) Lower Limit */
    /* for define of lower Rg limit for projected point expPrior probability for outdoor is taken into account. */
    switch ( pAwbCtx->DoorType )
    {
        case EXPPRIOR_INDOOR:
            {
                /* if INDOOR is very likely use the pre-calibrated indoor projection minimum. */
                f_RgProj_Min = pAwbCtx->RgProjIndoorMin;
                break;
            }

        case EXPPRIOR_OUTDOOR:
            {
                /* if OUTDOOR is certain the projection cannot be smaller than the pre-calibrated outdoor projection minimum. */
                f_RgProj_Min = pAwbCtx->RgProjOutdoorMin;
                break;
            }

        case EXPPRIOR_TRANSITION_RANGE:
            {
                 /* transition range */
                 /* in transition range use weighted value of outdoor and indoor projection minimum as minimum. */
                 /* exposure prior probability serves for weighting */
                 float fWeight = 2.0f * pAwbCtx->ExpPriorOut - 1.0f; // fExpPriorOut ranges from 1...0.5
                 f_RgProj_Min = (fWeight * pAwbCtx->RgProjOutdoorMin) + ( 1.0f - fWeight ) * pAwbCtx->RgProjIndoorMin;
                 break;
            }

        default:
            {
                return ( RET_WRONG_CONFIG );
            }
     }

	TRACE( AWB_DEBUG, "AWB Clipping: f_RgProj(%f)  f_RgProj_Min(%f)  pAwbCtx->RgProjMax(%f)\n", f_RgProj, f_RgProj_Min,pAwbCtx->RgProjMax);
	  
    /* 5.1) compare Rg projection with lower limit. If too large clip projection to lower limit. */
    if ( f_RgProj < f_RgProj_Min )
    {
        TRACE( AWB_DEBUG, "AWB Clipping: Rg coordinate smaller than minimum threshold -> clipping  f_RgProj_Min(%f)\n",  f_RgProj_Min);
        f_RgProj = f_RgProj_Min;
        if ( f_RgProj == pAwbCtx->RgProjOutdoorMin ) 
        {
            *pRgProjClippedToOutDoorMin = BOOL_TRUE;
        }	  
    }      

    /* 6.) Upper Limit */
    /* compare Rg projection with pre-calibrated upper limit. If too large clip projection to
     * upper limit. note: no considerations concerning expPrior for upper limit. */
    if ( f_RgProj > pAwbCtx->RgProjMax )
    {
        TRACE( AWB_DEBUG, "AWB Clipping: Rg coordinate greater than maximum threshold -> clipping\n" );
        f_RgProj = pAwbCtx->RgProjMax;
    }

    /* 7.) get distance of upper clipping curve from projection point */
    InterpolateCtx.size = pAwbCtx->pGainClipCurve->ArraySize1;
    InterpolateCtx.pX   = pAwbCtx->pGainClipCurve->pRg1;
    InterpolateCtx.pY   = pAwbCtx->pGainClipCurve->pMaxDist1;
    InterpolateCtx.x_i  = f_RgProj;
    result = Interpolate( &InterpolateCtx );
    if ( result == RET_OUTOFRANGE )
    {
        result = RET_SUCCESS;
        TRACE( AWB_WARN, "AWB Clipping: Rg coordinate greater than maximum threshold -> clipping\n" );
    }
    else if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    f_s_Max1 = InterpolateCtx.y_i;

	#if 0
	/* ddl@rock-chips.com: v0.5.0x00 */
  	if(strcmp(pAwbCtx->pIlluProfiles[pAwbCtx->DominateIlluProfileIdx]->name, "A")== 0)
	{
		TRACE( AWB_DEBUG, "AWB Clipping: Rg:%f  weght:%f\n",  f_RgProj, pAwbCtx->Weight[pAwbCtx->DominateIlluProfileIdx]);
		if(f_RgProj > pAwbCtx->fRgProjALimit || pAwbCtx->Weight[pAwbCtx->DominateIlluProfileIdx] < pAwbCtx->fRgProjAWeight)
			f_RgProj = pAwbCtx->fRgProjYellowLimit;
		
	}else{
		if(f_RgProj < pAwbCtx->fRgProjYellowLimit)
			f_RgProj = pAwbCtx->fRgProjYellowLimit;
	}
	#endif
	
    /* 8.) get distance of lower clipping curve from projection point use same projection point */
    InterpolateCtx.size = pAwbCtx->pGainClipCurve->ArraySize2;
    InterpolateCtx.pX   = pAwbCtx->pGainClipCurve->pRg2;
    InterpolateCtx.pY   = pAwbCtx->pGainClipCurve->pMaxDist2;
    InterpolateCtx.x_i  = f_RgProj;
    result = Interpolate( &InterpolateCtx );
    if ( result == RET_OUTOFRANGE )
    {
        result = RET_SUCCESS;
        TRACE( AWB_WARN, "WBGAIN_CALC Clipping: Out of range of clipping curve. Choosing max/min clipping value.\n" );
    }
    else if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    f_s_Max2 = -InterpolateCtx.y_i;
    
    TRACE( AWB_DEBUG, "f_RgProj: %f, %f, %f \n", f_RgProj, f_s_Max1, f_s_Max2);
	
	#if 0
	/* ddl@rock-chips.com: v0.5.0x00 */
	if(f_s < pAwbCtx->fRgProjIllToCwf) {
	    if ((pAwbCtx->DominateIlluProfileIdx == pAwbCtx->D50IlluProfileIdx) && (pAwbCtx->Weight[pAwbCtx->D50IlluProfileIdx] < pAwbCtx->fRgProjIllToCwfWeight)) 
		{
		    pAwbCtx->DominateIlluProfileIdx = pAwbCtx->CwfIlluProfileIdx;	
		    TRACE( AWB_ERROR, "illuminate: D65  -->  CWF \n", __FUNCTION__);
		}
	}
	#endif
	
	pAwbCtx->WbRg = f_Rg;
	pAwbCtx->WbBg = f_Bg;
	pAwbCtx->Wb_s = f_s;
	pAwbCtx->Wb_s_max1 = f_s_Max1;
	pAwbCtx->Wb_s_max2 = f_s_Max2;
	TRACE( AWB_DEBUG, "f_RgProj: %f,  f_s:%f (%f, %f)  f_Rg:%f	f_Bg:%f\n", f_RgProj, f_s, f_s_Max1, f_s_Max2, f_Rg, f_Bg);
    /* 9.) compare current distance with upper distance value. If >: use clipping curve distance */
    if ( f_s > f_s_Max1 )
    {
        TRACE( AWB_DEBUG, "AWB Clipping: Gain distance value above upper clipping curve distance value -> clipping to upper clipping curve. \n" );
        f_s = f_s_Max1;
    }

    /* 10.) compare current distance with lower distance value. If <: use clipping curve distance */
    if ( f_s < f_s_Max2 )
    {
        TRACE( AWB_DEBUG, "AWB Clipping:  Gain distance value below lower clipping curve distance value -> clipping to lower clipping curve \n" );
        f_s = f_s_Max2;
    }
  
    /* 11.) calculate clipped Rg position with clipped distance value
     *      Rg projection coordinate + distance * Rg coordinate of center line normal vector */
    f_Rg = f_RgProj + f_s * pAwbCtx->pCenterLine->f_N0_Rg;

    /* this:     f_Bg = f_BgProj + f_s * f_N0_Bg
     * and this: f_BgProj =  (f_d - g_f_RgProj * f_N0_Rg) / f_N0_Bg
     * put together lead to this: */
    f_Bg = ( pAwbCtx->pCenterLine->f_d - pAwbCtx->pCenterLine->f_N0_Rg * f_RgProj ) / pAwbCtx->pCenterLine->f_N0_Bg + f_s * pAwbCtx->pCenterLine->f_N0_Bg;

    pAwbCtx->WbClippedGainsOverG.GainROverG = f_Rg;
    pAwbCtx->WbClippedGainsOverG.GainBOverG = f_Bg;
    pAwbCtx->RgProj = f_RgProj;  	

    TRACE( AWB_DEBUG, "%s: New damped and clipped, green-normalized gains: R: %f, G: 1.0, B: %f\n", __FUNCTION__, f_Rg, f_Bg);

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbNormalizeGain()
 *****************************************************************************/
static RESULT AwbNormalizeGain
(
    AwbGains_t  *pGains
)
{
    float fGainMin = 0.0f;

    if ( pGains == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    fGainMin = (pGains->fGreenR + pGains->fGreenB) / 2.0f;
    if ( pGains->fRed < fGainMin )
    {
        fGainMin = pGains->fRed;
    }
    if ( pGains->fBlue < fGainMin )
    {
        fGainMin = pGains->fBlue;
    }

    if( fGainMin < (float)DIVMIN )
    {
         return ( RET_OUTOFRANGE );
    }

    pGains->fRed    /= fGainMin;
    pGains->fGreenR /= fGainMin;
    pGains->fGreenB /= fGainMin;
    pGains->fBlue   /= fGainMin;

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbWbGainProcessFrame()
 *****************************************************************************/
RESULT AwbWbGainProcessFrame
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1.) initial checks and presets */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE ); 
    }

    pAwbCtx->WbGainsOverG.GainROverG = ( pAwbCtx->WbGains.fRed / pAwbCtx->WbGains.fGreenR );
    pAwbCtx->WbGainsOverG.GainBOverG = ( pAwbCtx->WbGains.fBlue / pAwbCtx->WbGains.fGreenB );

    result = AwbWbGainOutOfRangeCheck( pAwbCtx );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    // calculate new damped gains and store them in global variables
    pAwbCtx->WbDampedGains.fRed    = pAwbCtx->AwbIIRDampCoef * pAwbCtx->WbDampedGains.fRed + ( 1.0f - pAwbCtx->AwbIIRDampCoef) * pAwbCtx->WbGains.fRed;
    pAwbCtx->WbDampedGains.fGreenR = pAwbCtx->AwbIIRDampCoef * pAwbCtx->WbDampedGains.fGreenR + ( 1.0f - pAwbCtx->AwbIIRDampCoef) * pAwbCtx->WbGains.fGreenR;
    pAwbCtx->WbDampedGains.fGreenB = pAwbCtx->AwbIIRDampCoef * pAwbCtx->WbDampedGains.fGreenB + ( 1.0f - pAwbCtx->AwbIIRDampCoef) * pAwbCtx->WbGains.fGreenB;
    pAwbCtx->WbDampedGains.fBlue   = pAwbCtx->AwbIIRDampCoef * pAwbCtx->WbDampedGains.fBlue + ( 1.0f - pAwbCtx->AwbIIRDampCoef) * pAwbCtx->WbGains.fBlue;

    pAwbCtx->WbGainsOverG.GainROverG = ( pAwbCtx->WbDampedGains.fRed / pAwbCtx->WbDampedGains.fGreenR );
    pAwbCtx->WbGainsOverG.GainBOverG = ( pAwbCtx->WbDampedGains.fBlue / pAwbCtx->WbDampedGains.fGreenB );

    result = AwbWbGainClip( pAwbCtx );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

    pAwbCtx->WbGains.fRed       = pAwbCtx->WbClippedGainsOverG.GainROverG;
    pAwbCtx->WbGains.fGreenR    = 1.0f; 
    pAwbCtx->WbGains.fGreenB    = 1.0f;
    pAwbCtx->WbGains.fBlue      = pAwbCtx->WbClippedGainsOverG.GainBOverG;

    result = AwbNormalizeGain( &pAwbCtx->WbGains );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


