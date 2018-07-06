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
 * @file wprevert.c
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
#include "illuest.h"

#include "interpolate.h"


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( AWB_INFO  );
USE_TRACER( AWB_WARN  );
USE_TRACER( AWB_ERROR );

USE_TRACER( AWB_DEBUG );


#define LOGMIN     0.0001f   //!< lowest value for logarithmic calculations


#ifndef MIN
#define MIN(x,y)            ( ((x) < (y)) ? (x) : (y) )
#endif

#ifndef MAX
#define MAX(x,y)            ( ((x) > (y)) ? (x) : (y) )
#endif



/******************************************************************************
 * local type definitions
 *****************************************************************************/



/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
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
 * AwbIlluEstProcessFrame()
 *****************************************************************************/
RESULT AwbIlluEstProcessFrame
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    float fAwbMeanSum = 0.0f;

    float fPcaC1 = 0.0f;
    float fPcaC2 = 0.0f;

    float fPcaC1x = 0.0f;
    float fPcaC2x = 0.0f;

    float fVal1 = 0.0f;
    float fVal2 = 0.0f;

    float fWeightSum    = 0.0f;
    float fLikeHoodSum  = 0.0f;
    float fMaxWeight    = 0.0f;
    float fDistance     = 0.0f;

    float fTransFact    = 0.0f;

    int32_t i;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1.) initial checks and presets */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE ); 
    }

    /* 1.1 ) reset all values */
    #if 0    //ddl@rock-chips.com   v0.4.0
    pAwbCtx->DominateIlluProfileIdx = -1;
    pAwbCtx->Region                 = AWB_NUM_WHITE_PIXEL_INVALID;
    #endif
    
    /* 2.) Normalize the mean values
     *
     *    fAwbMeanSum = fAwbMeanR + fAwbMeanG + fAwbMeanB
     *
     *    fAwbMeanR = fAwbMeanR / fAwbMeanSum
     *    fAwbMeanG = fAwbMeanG / fAwbMeanSum
     *    fAwbMeanB = fAwbMeanB / fAwbMeanSum
     */
    fAwbMeanSum = pAwbCtx->RevertedMeansRgb.fRed + pAwbCtx->RevertedMeansRgb.fGreen + pAwbCtx->RevertedMeansRgb.fBlue;
    if( fAwbMeanSum < DIVMIN )
    {
         TRACE( AWB_ERROR, "(message code 02): AwbMeanSum near zero!\n");
         return ( RET_OUTOFRANGE );
    }
    pAwbCtx->NormalizedMeansRgb.fRed   = pAwbCtx->RevertedMeansRgb.fRed   / fAwbMeanSum;
    pAwbCtx->NormalizedMeansRgb.fGreen = pAwbCtx->RevertedMeansRgb.fGreen / fAwbMeanSum;
    pAwbCtx->NormalizedMeansRgb.fBlue  = pAwbCtx->RevertedMeansRgb.fBlue  / fAwbMeanSum;

    /* 3.) calculate PCA (R,G,B)
     *
     *    |  fPcaC1  |       ^T    | fAwbMeanR  - m1 |
     *    |          |  =  V    x  | fAwbMeanG  - m2 |
     *    |  fPcaC2  |             | fAwbMeanB  - m3 |
     */
    fPcaC1 =
         (pAwbCtx->NormalizedMeansRgb.fRed   - pAwbCtx->pSvdMeanValue->fCoeff[0]) * pAwbCtx->pPcaMatrix->fCoeff[0] +
         (pAwbCtx->NormalizedMeansRgb.fGreen - pAwbCtx->pSvdMeanValue->fCoeff[1]) * pAwbCtx->pPcaMatrix->fCoeff[1] +
         (pAwbCtx->NormalizedMeansRgb.fBlue  - pAwbCtx->pSvdMeanValue->fCoeff[2]) * pAwbCtx->pPcaMatrix->fCoeff[2];

    fPcaC2 =
         (pAwbCtx->NormalizedMeansRgb.fRed   - pAwbCtx->pSvdMeanValue->fCoeff[0]) * pAwbCtx->pPcaMatrix->fCoeff[3] +
         (pAwbCtx->NormalizedMeansRgb.fGreen - pAwbCtx->pSvdMeanValue->fCoeff[1]) * pAwbCtx->pPcaMatrix->fCoeff[4] +
         (pAwbCtx->NormalizedMeansRgb.fBlue  - pAwbCtx->pSvdMeanValue->fCoeff[2]) * pAwbCtx->pPcaMatrix->fCoeff[5];

    /* 4.) calculate the likelihood data */
    for( i=0; i<pAwbCtx->NoIlluProfiles; i++)
    {
        /* MAP Lookup
         *                            1/2 (fPcaC1 - µ01, fPcaC2 - µ02) x | E011 E012 | x | fPcaC1 - µ01 |
         *  fLikeHood0 = Gaussfac.* e                                    | E021 E022 |   | fPcaC2 - µ02 |
         *
         *                            1/2 (fPcaC1 - µ11, fPcaC2 - µ12) x | E111 E112 | x | fPcaC1 - µ11 |
         *  fLikeHood1 = Gaussfac.* e                                    | E121 E122 |   | fPcaC2 - µ12 |
         *
         *                            1/2 (fPcaC1 - µ21, fPcaC2 - µ22) x | E211 E212 | x | fPcaC1 - µ21 |
         *  fLikeHood2 = Gaussfac.* e                                    | E221 E222 |   | fPcaC2 - µ22 |
         *
         */
        fPcaC1x = (fPcaC1 - pAwbCtx->pIlluProfiles[i]->GaussMeanValue.fCoeff[0]);
        fPcaC2x = (fPcaC2 - pAwbCtx->pIlluProfiles[i]->GaussMeanValue.fCoeff[1]);

        fVal1 = (pAwbCtx->pIlluProfiles[i]->CovarianceMatrix.fCoeff[0] * fPcaC1x) 
                        + (pAwbCtx->pIlluProfiles[i]->CovarianceMatrix.fCoeff[1] * fPcaC2x);
        fVal2 = (pAwbCtx->pIlluProfiles[i]->CovarianceMatrix.fCoeff[2] * fPcaC1x)
                        + (pAwbCtx->pIlluProfiles[i]->CovarianceMatrix.fCoeff[3] * fPcaC2x);

        pAwbCtx->LikeHood[i] = (float)( exp( -0.5 * (double)( fPcaC1x * fVal1 + fPcaC2x * fVal2 ) ) ) * pAwbCtx->pIlluProfiles[i]->GaussFactor.fCoeff[0];
    }

    // calculate the likehood sum
    for( i = 0; i < pAwbCtx->NoIlluProfiles; i++ )
    {
        fLikeHoodSum += ( pAwbCtx->LikeHood[i] * ( (pAwbCtx->pIlluProfiles[i]->DoorType == CAM_DOOR_TYPE_OUTDOOR) ? pAwbCtx->ExpPriorOut : pAwbCtx->ExpPriorIn ) );
    }

    if ( fLikeHoodSum < DIVMIN )
    {
        TRACE( AWB_ERROR, "AWB (message code 04): fLikeHoodSum near zero\n" );
        return ( RET_CANCELED );
    }

    /* 5.) normalize the weights and find maximum weight (dominante profile) */
    for( i=0; i < pAwbCtx->NoIlluProfiles; i++)
    {
        pAwbCtx->Weight[i] = ( pAwbCtx->LikeHood[i] * ( (pAwbCtx->pIlluProfiles[i]->DoorType == CAM_DOOR_TYPE_OUTDOOR) ? pAwbCtx->ExpPriorOut : pAwbCtx->ExpPriorIn ) ) / fLikeHoodSum;

        /* 5.1) search the max weight for further calculations */
        if ( pAwbCtx->Weight[i] > fMaxWeight )
        {
            fMaxWeight = pAwbCtx->Weight[i];       /* max weight */
            pAwbCtx->DominateIlluProfileIdx = i;   /* profile number of max weight */
        }

        fWeightSum += pAwbCtx->Weight[i];

        TRACE( AWB_DEBUG, "%015s: liklyhood[%d]=%f weigth[%d]=%f Pca: %f %f Val: %f %f \n",
                pAwbCtx->pIlluProfiles[i]->name, i, pAwbCtx->LikeHood[i], i, pAwbCtx->Weight[i], fPcaC1x, fPcaC2x, fVal1, fVal2 );
    }

    TRACE( AWB_DEBUG, "fLikeHoodSum=%f, %f\n", fLikeHoodSum, fWeightSum );
    TRACE( AWB_DEBUG, "DominateIlluProfileIdx=%d    ExpPriorOut: %f   ExpPriorIn: %f\n", pAwbCtx->DominateIlluProfileIdx, pAwbCtx->ExpPriorOut,pAwbCtx->ExpPriorIn);

    /* 6. check the distance and evaluate region */
    if ( (pAwbCtx->DominateIlluProfileIdx >= 0) && (pAwbCtx->DominateIlluProfileIdx < pAwbCtx->NoIlluProfiles)  )
    {
        int32_t idx = pAwbCtx->DominateIlluProfileIdx;

        fDistance = pAwbCtx->LikeHood[idx];

        if ( fDistance >=  pAwbCtx->pIlluProfiles[idx]->Threshold.fCoeff[1] )
        {
            pAwbCtx->Region = ILLUEST_REGION_A;
        }
        else if ( (fDistance >  pAwbCtx->pIlluProfiles[idx]->Threshold.fCoeff[0])
                && (fDistance <  pAwbCtx->pIlluProfiles[idx]->Threshold.fCoeff[1]) )
        {
            pAwbCtx->Region = ILLUEST_REGION_B;

            /* note: as coef0<dist<coef1, coef0 cannot be equal to coef1 => divide by 0 not possible */
            fTransFact = (fDistance - pAwbCtx->pIlluProfiles[idx]->Threshold.fCoeff[0])
                / (pAwbCtx->pIlluProfiles[idx]->Threshold.fCoeff[1] - pAwbCtx->pIlluProfiles[idx]->Threshold.fCoeff[0]);
        }
        else
        {
            pAwbCtx->Region = ILLUEST_REGION_C;
        }

        /* 7. use fTransFact to calculate fWeightTrans */
        if ( pAwbCtx->Region == ILLUEST_REGION_A )
        {
            /* set weight to 0 but for dominant illu. This weight is set to 1 */
            for ( i=0; i < pAwbCtx->NoIlluProfiles; i++ )
            {
                pAwbCtx->WeightTrans[i] = 0.0f;
            }
            pAwbCtx->WeightTrans[pAwbCtx->DominateIlluProfileIdx] = 1.0f;
        }
        else if ( pAwbCtx->Region == ILLUEST_REGION_C )
        {
            /* mixed illumination shall be used. */
            for( i=0; i < pAwbCtx->NoIlluProfiles; i++)
            {
                pAwbCtx->WeightTrans[i] = pAwbCtx->Weight[i];
            }
        }
        else /* RegionB */
        {
            /* mix of mixed illumination and dominant illumination */
            /* mixed illumination is reduced by (1-fTransFact),    */
            /* dominant illu is increased accordingly              */
            for( i=0; i < pAwbCtx->NoIlluProfiles; i++)
            {
                pAwbCtx->WeightTrans[i] = (1.0f - fTransFact) * pAwbCtx->Weight[i];
            }
            pAwbCtx->WeightTrans[pAwbCtx->DominateIlluProfileIdx] += fTransFact;
        }
    }
    else
    {
        return ( RET_OUTOFRANGE );
    }

    TRACE( AWB_DEBUG, "Region=%d\n", pAwbCtx->Region );

    //****************************************************************
    // Calculate the new AWB gains using the gray-world assumption

    if ( ( pAwbCtx->NormalizedMeansRgb.fRed > DIVMIN )
            && ( pAwbCtx->NormalizedMeansRgb.fGreen > DIVMIN )
            && ( pAwbCtx->NormalizedMeansRgb.fBlue > DIVMIN ) )
    {
        pAwbCtx->WbGains.fRed       = fAwbMeanSum / pAwbCtx->NormalizedMeansRgb.fRed; 
        pAwbCtx->WbGains.fGreenR    = fAwbMeanSum / pAwbCtx->NormalizedMeansRgb.fGreen;
        pAwbCtx->WbGains.fGreenB    = fAwbMeanSum / pAwbCtx->NormalizedMeansRgb.fGreen;
        pAwbCtx->WbGains.fBlue      = fAwbMeanSum / pAwbCtx->NormalizedMeansRgb.fBlue;

        // normalize the gain values
        result = AwbNormalizeGain( &pAwbCtx->WbGains );
    }
    else
    {
         result = RET_OUTOFRANGE;
    }


    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}
