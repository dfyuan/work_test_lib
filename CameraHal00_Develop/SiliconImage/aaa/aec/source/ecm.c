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
 * @file clm.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <math.h>

#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include "ecm.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( ECM_INFO  , "ECM: ", INFO     , 0 );
CREATE_TRACER( ECM_WARN  , "ECM: ", WARNING  , 1 );
CREATE_TRACER( ECM_ERROR , "ECM: ", ERROR    , 1 );
CREATE_TRACER( ECM_NOTICE0 , "ECM: ", TRACE_NOTICE0, 1 );
CREATE_TRACER( ECM_DEBUG , "ECM: ", INFO     , 0 );

#define ABS_DIFF( a, b )     ( (a > b) ? (a-b) : (b-a) )
#define ECM_LOCK_RANGE       0.0001f //seconds
#define HUGE_VAL_GAIN        (65535.0f)

/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * EcmExecute()
 *****************************************************************************/
RESULT EcmExecuteDirect
(
    AecContext_t        *pAecCtx,
    float               Exposure,
    float               *pSplitGain,
    float               *pSplitIntegrationTime
)
{
    RESULT result = RET_SUCCESS;
	if (pAecCtx->EcmDotEnable >= 1.0){
		// rename exposure param to match formulas in ECM paper, although therein Alpha is said to be the
		 // exposure normalized against the flicker period (Alpha=Exposure/EcmTflicker),whereas here it is not
		 float Alpha = Exposure;

		 // our internal results
		 float g	 = 0.0f;
		 float Tint  = 0.0f;
		 float g0	 = 0.0f;
		 float Tint_temp  = 0.0f;

		 float *TimeDot=pAecCtx->EcmTimeDot;//{ET_min,......,ET_max}
		 TimeDot[ECM_DOT_NO -1] = pAecCtx->MaxIntegrationTime;//{AG_min,......,AG_max}
		 float *GainDot = pAecCtx->EcmGainDot;
		 GainDot[ECM_DOT_NO -1] = pAecCtx->MaxGain;
		 int dot=0;
		 float ExDot=0;

		 TRACE( ECM_INFO, "%s: (enter) \n", __FUNCTION__);
		 // TRACE( ECM_ERROR, "%s: MaxIntegrationTime=%f  MaxGain=%f \n", __FUNCTION__,pAecCtx->MaxIntegrationTime,pAecCtx->MaxGain );
		 DCT_ASSERT(pAecCtx != NULL);
		 DCT_ASSERT(pSplitGain != NULL);
		 DCT_ASSERT(pSplitIntegrationTime != NULL);

		 if ( Alpha < 0.0f )
		 {
			 return RET_OUTOFRANGE;
		 }

		 for(dot = 0;dot < ECM_DOT_NO ;dot++)
		 {
			 if (GainDot[dot] > pAecCtx->MaxGain)
			 {
				 GainDot[dot] = pAecCtx->MaxGain;
			 }
			 if (TimeDot[dot] > pAecCtx->MaxIntegrationTime)
			 {
				 TimeDot[dot] = pAecCtx->MaxIntegrationTime;
			 }
			 TimeDot[dot] = floorf( TimeDot[dot]  / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker;//round
		 }

		 for(dot = 0;dot < ECM_DOT_NO ;dot++)
		 {
			 ExDot = TimeDot[dot]*GainDot[dot];

			 if(ExDot > Alpha)
				 break;
		 }
		 //1<=dot<=6
		 if (0 < dot && dot < ECM_DOT_NO)
		 {
			 if (TimeDot[dot-1] == TimeDot[dot])//adjust gain
			 {
				 Tint = TimeDot[dot];
				 g = Alpha/Tint;
			 }
			 else if(GainDot[dot-1] == GainDot[dot]) //adjust time
			 {
				 g = GainDot[dot];
				 Tint = Alpha/g;
				 if(Tint > pAecCtx->EcmTflicker)//
				 {
					 Tint_temp = roundf( Tint / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker;//round
					 g = Alpha / Tint_temp;//adjust gain again
					 if(g > GainDot[dot])
					 {
						 Tint_temp = ceilf( Tint / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker;//round
						 g = Alpha / Tint_temp;
					 }
					 if(g < pAecCtx->MinGain)
					 {
						 Tint_temp = floorf( Tint / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker;//round
						 g = Alpha / Tint_temp;
					 }
					 
					 Tint = Tint_temp;
				 }	  
			 } else {
				 Tint = TimeDot[dot];
				 g = Alpha/Tint;
			 }
		 } else if(dot == ECM_DOT_NO)
		 {
			  dot = dot-1;
			  g = GainDot[dot];
			  Tint = TimeDot[dot];
		 }

		 // TRACE( ECM_ERROR, "%s: EXPOSURE=%f	not modified dot=%d g=%f t=%f\n", __FUNCTION__,Alpha,dot,g,Tint );
		 // if (g < pAecCtx->MinGain) {
		 // 	g = pAecCtx->MinGain;
		 // 	Tint = Alpha/g;
		 // } else	 if (g > pAecCtx->MaxGain) {
		 // 	g = g*8/9;
		 // 	Tint = Alpha/g;
		 // }
		 if (g < pAecCtx->MinGain) {
			 g = pAecCtx->MinGain;
			 Tint = Alpha/g;
		 } else  if (g > pAecCtx->MaxGain) {
			 g = pAecCtx->MaxGain;
			 Tint = Alpha/g;
		 }

		 // TRACE( ECM_ERROR, "%s: EXPOSURE=%f	modified dot=%d g=%f t=%f\n", __FUNCTION__,Alpha,dot,g,Tint );


		 pAecCtx->EcmOldAlpha = Alpha;
		 pAecCtx->EcmOldGain  = g;
		 pAecCtx->EcmOldTint  = Tint;

		 TRACE( ECM_INFO, "%s: In/Split-Exposure: %f/%f (Split-Gain/-IntTime: %f/%f)\n",
			 __FUNCTION__, Alpha, g*Tint, g,Tint);

		 // return split gain & integration time
		 *pSplitGain = g;
		 *pSplitIntegrationTime = Tint;
		 TRACE( ECM_INFO, "%s: (exit)\n", __FUNCTION__ );
	}else {

	    // rename exposure param to match formulas in ECM paper, although therein Alpha is said to be the
	    // exposure normalized against the flicker period (Alpha=Exposure/EcmTflicker),whereas here it is not
	    float Alpha = Exposure;

	    // our internal results
	    float g     = 0.0f;
	    float Tint  = 0.0f;
	    float g0    = 0.0f;


	    TRACE( ECM_INFO, "%s: (enter) \n", __FUNCTION__);

	    DCT_ASSERT(pAecCtx != NULL);
	    DCT_ASSERT(pSplitGain != NULL);
	    DCT_ASSERT(pSplitIntegrationTime != NULL);

	    if ( Alpha < 0.0f )
	    {
	        return RET_OUTOFRANGE;
	    }

	    DCT_ASSERT( pAecCtx->EcmT0 > 0.0f);
	    if(pAecCtx->EcmT0 > pAecCtx->MaxIntegrationTime){
	        pAecCtx->EcmT0 = pAecCtx->MaxIntegrationTime;
	    }
            if(pAecCtx->EcmT0 < 0.01f)
                pAecCtx->EcmT0 = 0.01f;
	    g0 = ( Alpha / pAecCtx->EcmT0 );

	    DCT_ASSERT( pAecCtx->MinGain > 0.0f);
	    if ( g0 < pAecCtx->MinGain )
	    {
	       // don't try to avoid flicker in brigth scenes
	       TRACE( ECM_DEBUG, "%s: skipping flicker avoidance\n", __FUNCTION__ );

	       // set final values directly
	       g    = pAecCtx->MinGain;
	       Tint = Alpha / g;
	    }
	    else
	    {
	       // try to avoid flicker while splitting exposure into integration time & gain
	       float gc    = 0.0f;
	       float Tc    = 0.0f;
	       float gTemp = 0.0f;

	       // align max integration time to an integer multiple of the flicker period
	       float Tmax = floorf( pAecCtx->MaxIntegrationTime / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker;
	       if(Tmax<= 0.0f){
	            Tmax = pAecCtx->MaxIntegrationTime ;
	        }

	       // calc continuous gain
	       gc = powf( g0, (pAecCtx->EcmA0 / (pAecCtx->EcmA0 + 1.0f)) ); // linear approach

	       // calc corresponding continuous integration time
	       DCT_ASSERT( gc > 0.0f );
	       Tc = Alpha / gc;

	       // calc final integration time (align & limit continuous integration time to an integer multiple of the flicker period)
	       Tint = MIN( Tmax, roundf ( Tc / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker );

	       // calc preliminary gain
	       if ( Tint == 0.0 ) //ok here, avoid division by 0
	       {
	           gTemp = HUGE_VAL_GAIN; // force 'gTemp > pAecCtx->MaxGain' case below
	       }
	       else
	       {
	           gTemp = Alpha / Tint;
	       }

	       // do some limit checking on the preliminary gain and recalc integration time if necessary
	       if ( gTemp < pAecCtx->MinGain )
	       {
	           Tint = MIN( Tmax, floorf ( Tc / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker );
	       }
	       else if ( gTemp > pAecCtx->MaxGain )
	       {
	           Tint = MIN( Tmax, ceilf ( (Alpha / pAecCtx->MaxGain) / pAecCtx->EcmTflicker ) * pAecCtx->EcmTflicker );
	       }

	       // calc final gain
	       DCT_ASSERT ( Tint > 0.0f );
	       g = Alpha / Tint;

	       // limit final gain once more
	       if( g > pAecCtx->MaxGain )
	       {
	           g = pAecCtx->MaxGain;
	       }

	       
	       TRACE( ECM_DEBUG,"    EcmT0: %f  Tc: %f  Tint: %f \n"
	                        "        EcmA0: %f  g0: %f  gc: %f \n",
	                    pAecCtx->EcmT0,Tc,Tint,
	                    pAecCtx->EcmA0,g0,gc);
	    }

	    pAecCtx->EcmOldAlpha = Alpha;
	    pAecCtx->EcmOldGain  = g;
	    pAecCtx->EcmOldTint  = Tint; 

	    TRACE( ECM_DEBUG, "%s: In/Split-Exposure: %f/%f (Split-Gain/-IntTime: %f/%f)\n",
	        __FUNCTION__, Alpha, g*Tint, g,Tint);

	    // return split gain & integration time
	    *pSplitGain = g;
	    *pSplitIntegrationTime = Tint;

	    TRACE( ECM_INFO, "%s: (exit)\n", __FUNCTION__ );

	}
    return ( result );
}
RESULT EcmExecute
(
    AecContext_t        *pAecCtx,
    float               Exposure,
    float               *pSplitGain,
    float               *pSplitIntegrationTime
)
{
    RESULT result = RET_SUCCESS;

    // rename exposure param to match formulas in ECM paper, although therein Alpha is said to be the
    // exposure normalized against the flicker period (Alpha=Exposure/EcmTflicker),whereas here it is not
    float Alpha = Exposure;

    // our internal results
    float g     = 0.0f;
    float Tint  = 0.0f;
    float g0    = 0.0f;


    TRACE( ECM_DEBUG, "%s: (enter) OldAlpha: %f  Alpha: %f\n", __FUNCTION__, pAecCtx->EcmOldAlpha, Alpha);

    DCT_ASSERT(pAecCtx != NULL);
    DCT_ASSERT(pSplitGain != NULL);
    DCT_ASSERT(pSplitIntegrationTime != NULL);

    if ( Alpha < 0.0f )
    {
        return RET_OUTOFRANGE;
    }

    //lock range to avoid oscillations caused mainly due to granularity of sensor gain and integration time
    if( ABS_DIFF(pAecCtx->EcmOldAlpha, Alpha) < ECM_LOCK_RANGE )
    {
       g    = pAecCtx->EcmOldGain;
       Tint = pAecCtx->EcmOldTint;
    }
    else
    {       
       result = EcmExecuteDirect(pAecCtx,Exposure,&g,&Tint);
    }

    TRACE( ECM_DEBUG, "%s: In/Split-Exposure: %f/%f (Split-Gain/-IntTime: %f/%f  diff:%f)\n",
        __FUNCTION__, Alpha, g*Tint, g,Tint,ABS_DIFF(pAecCtx->EcmOldAlpha, Alpha) );

    // return split gain & integration time
    *pSplitGain = g;
    *pSplitIntegrationTime = Tint;

    TRACE( ECM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * AfpsExecute()
 *****************************************************************************/
RESULT AfpsExecute
(
    AecContext_t        *pAecCtx,
    float               IntegrationTime,
    uint32_t            *pNewResolution
)
{
    RESULT result = RET_SUCCESS;
    uint32_t curRes;

    TRACE( ECM_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT(pAecCtx != NULL);
    DCT_ASSERT(pNewResolution != NULL);

    uint32_t idx = 0;

    // default to no resolution change requested
    *pNewResolution = 0;

    // AFPS enabled?
    if (!pAecCtx->AfpsEnabled)
    {
        return RET_SUCCESS;
    }

    // do we have AFPS info present?
    if (pAecCtx->AfpsInfo.Stage[0].Resolution == 0)
    {
        TRACE( ECM_INFO, "AfpsInfo empty\n" );
        return RET_SUCCESS;
    }

    // find first/fastest resolution supporting the given integration time
    for (idx=0; idx<ISI_NUM_AFPS_STAGES; idx++)
    {
        if (pAecCtx->AfpsInfo.Stage[idx].Resolution == 0)
        {
            --idx; // unwind to use last resolution; won't underflow as Stage[0].Resolution must be non-zero due to explicit check above
            break;
        }

        if (pAecCtx->AfpsInfo.Stage[idx].MaxIntTime > IntegrationTime)
        {
            break; // OK, we've found a suitable resolution
        }
    }
    if (idx>=ISI_NUM_AFPS_STAGES)
    {
        idx = ISI_NUM_AFPS_STAGES-1; // clip to last stage
    }

    // request resolution change?    
    if (IsiGetResolutionIss(pAecCtx->hSensor,&curRes) == RET_SUCCESS)
        pAecCtx->AfpsCurrResolution = curRes;
    if (pAecCtx->AfpsInfo.Stage[idx].Resolution != pAecCtx->AfpsCurrResolution)
    {
        char *szOldResName = "<old resolution>";
        char *szNewResName = "<new resolution>";
        (void)IsiGetResolutionName( pAecCtx->AfpsCurrResolution, &szOldResName );
        (void)IsiGetResolutionName( pAecCtx->AfpsInfo.Stage[idx].Resolution, &szNewResName );
        TRACE( ECM_NOTICE0, " AfpsCurrResolution(0x%x  %dx%d@%dfps) -> (idx:%d  0x%x  %dx%d@%dfps) ",
            pAecCtx->AfpsCurrResolution,ISI_RES_W_GET(pAecCtx->AfpsCurrResolution),
            ISI_RES_H_GET(pAecCtx->AfpsCurrResolution),
            ISI_FPS_GET(pAecCtx->AfpsCurrResolution),
            idx,
            pAecCtx->AfpsInfo.Stage[idx].Resolution,
            ISI_RES_W_GET(pAecCtx->AfpsInfo.Stage[idx].Resolution),
            ISI_RES_H_GET(pAecCtx->AfpsInfo.Stage[idx].Resolution),
            ISI_FPS_GET(pAecCtx->AfpsInfo.Stage[idx].Resolution));
            
        TRACE( ECM_DEBUG, "%s: resolution change requested (%s -> %s)\n", __FUNCTION__, szOldResName, szNewResName );
        *pNewResolution = pAecCtx->AfpsCurrResolution = pAecCtx->AfpsInfo.Stage[idx].Resolution;
    }

    TRACE( ECM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}
