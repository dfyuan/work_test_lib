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
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>

#include "clm.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CLM_INFO  , "CLM: ", INFO     , 0 );
CREATE_TRACER( CLM_WARN  , "CLM: ", WARNING  , 0 );
CREATE_TRACER( CLM_ERROR , "CLM: ", ERROR    , 1 );

CREATE_TRACER( CLM_DEBUG , "CLM: ", INFO     , 0 );


#define CLM_HIST_NUM_BINS   ( 3 * CAMERIC_ISP_HIST_NUM_BINS )


/******************************************************************************
 * local type definitions
 *****************************************************************************/
const  float BlackLevel             = 0.0f;
const  float LoopAccuracy           = 0.001f;
const  uint32_t MaxLoopIterations   = 100UL;

static float MeanLumaMinusBL        = 0.0f;
static float MeanHistogramMinusBL   = 0.0f;
static float SetPointMinusBL        = 0.0f;



/******************************************************************************
 * local variable declarations
 *****************************************************************************/



/******************************************************************************
 * local function prototypes
 *****************************************************************************/



/******************************************************************************
 * ClmCalcLumaDeviation()
 *****************************************************************************/
static float ClmCalcLumaDeviation
(
    const float SetPoint,
    const float MeanLuma
)
{
    float LumaDeviation = 0.0f;

    TRACE( CLM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( SetPoint > MeanLuma )
    {
        LumaDeviation = ( SetPoint - MeanLuma ) / SetPoint;
    }
    else
    {
        LumaDeviation = ( MeanLuma - SetPoint ) / SetPoint;
    }

    TRACE( CLM_DEBUG, "%s: dLuma = %f\n", __FUNCTION__, LumaDeviation );

    TRACE( CLM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( LumaDeviation );
}



/******************************************************************************
 * ClmExtrapolateHistogram()
 *****************************************************************************/
static RESULT ClmExtrapolateHistogram
(
    AecContext_t        *pAecCtx,
    CamerIcHistBins_t   bins
)
{

    uint32_t BinQm1 = bins[ (CAMERIC_ISP_HIST_NUM_BINS-2) ];    /**< h14 */
    uint32_t BinQ   = bins[ (CAMERIC_ISP_HIST_NUM_BINS-1) ];    /**< h15 */

    uint32_t NumOfBinsToAdd = 0U;
    uint32_t Height         = 0U;

    uint32_t i;

    TRACE( CLM_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* transfer the original bins, except for the last one from h to h+ */
    for ( i = 0U; i < (CAMERIC_ISP_HIST_NUM_BINS - 1U); i++ )
    {
        pAecCtx->ClmHistogram[i] = bins[i];
    }

    /* do extrapolation if h15 > h14, to avoid division by 0 h14 > 0 */
    if ( (BinQ > BinQm1) && (BinQm1 > 0) )
    {
        uint32_t value;

        /* 2 * h15/h14 +1 ( NumOfBinsToAdd is at least 3 since h15 > h14 ) */
        NumOfBinsToAdd = ( 2 * BinQ ) / BinQm1;
        ++NumOfBinsToAdd;

	//oyyf
        //if ( NumOfBinsToAdd > ( CAMERIC_ISP_HIST_NUM_BINS - 14) )
        //{
           // NumOfBinsToAdd = CAMERIC_ISP_HIST_NUM_BINS - 14;
        //}

	if ( NumOfBinsToAdd > ( CLM_HIST_NUM_BINS - 14) )
        {
            NumOfBinsToAdd = CLM_HIST_NUM_BINS - 14;
        }

        Height = ( 2U * BinQ ) / NumOfBinsToAdd;

        /* skip last bin since it is always 0 anyway */
        --NumOfBinsToAdd;

        /* calculate triangle
         * H * (1-(i-15)/(W-1)), i-15 = ucBin, W-1 = NumOfBinsToAdd
         * ((i-15)/(W-1) always < 1 since i < NumOfBinsToAdd)
         */
        for( i = 0U; i < NumOfBinsToAdd; i++ )
        {
            value =  ( Height * i ) / NumOfBinsToAdd;
            value = Height - value;
            pAecCtx->ClmHistogram[ (i + (CAMERIC_ISP_HIST_NUM_BINS - 1U)) ] = value;
        }

        pAecCtx->ClmHistogramSize = ((CAMERIC_ISP_HIST_NUM_BINS - 1U) + NumOfBinsToAdd);
    }
    else
    {
        pAecCtx->ClmHistogramSize = CAMERIC_ISP_HIST_NUM_BINS;
 		pAecCtx->ClmHistogram[15] = bins[15]; //oyyf 
    }

    TRACE( CLM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * ClmCalcMeanHistogram()
 *****************************************************************************/
static RESULT ClmCalcMeanHistogram
(
    AecContext_t *pAecCtx
)
{
    TRACE( CLM_INFO, "%s: (enter)\n", __FUNCTION__ );

    uint32_t i;

    pAecCtx->SumHistogram = 0U;
    for ( i = 0U;  i < pAecCtx->ClmHistogramSize; i++ )
    {
        pAecCtx->SumHistogram += pAecCtx->ClmHistogram[i];
    }

    /* avoid division by zero */
    if ( pAecCtx->SumHistogram == 0U )
    {
        TRACE( CLM_WARN, "%s: SumHistogram == 0, avoid division by zero, correcting to 1\n", __FUNCTION__ );
        pAecCtx->SumHistogram = 1U;
    }

    pAecCtx->MeanHistogram = 0.0f;
    for ( i = 0U;  i < pAecCtx->ClmHistogramSize; i++ )
    {
        pAecCtx->MeanHistogram += ( (float)(16U * pAecCtx->ClmHistogram[i]) / pAecCtx->SumHistogram ) *  ( MIN( (i + 0.5f), 15.5f ) );
    }

    /* avoid division by zero. */
    if ( !(pAecCtx->MeanHistogram > 0.0f) )
    {
        TRACE( CLM_WARN, "%s: MeanHistogram == 0, avoid division by zero, correcting to 1\n", __FUNCTION__ );
        pAecCtx->MeanHistogram = 1.0f;
    }

    TRACE( CLM_DEBUG, "%s: SumHistogram=%lu, MeanHistogram=%f\n", __FUNCTION__, pAecCtx->SumHistogram, pAecCtx->MeanHistogram );

    TRACE( CLM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * ClmLoop()
 *****************************************************************************/
static RESULT ClmLoop
(
    AecContext_t    *pAecCtx,
    const float     SetPoint,
    float           *NewExposure
)
{
    uint32_t LoopCnt = 0U;

    float MeanRatio;
    float LoopValue;

    *NewExposure = pAecCtx->Exposure;

    SetPointMinusBL         = MAX( FLT_EPSILON, (SetPoint               - BlackLevel) );
    MeanLumaMinusBL         = MAX( FLT_EPSILON, (pAecCtx->MeanLuma      - BlackLevel) );
    MeanHistogramMinusBL    = MAX( FLT_EPSILON, (pAecCtx->MeanHistogram - BlackLevel) );

    TRACE( CLM_DEBUG, "%s: SetPointMinusBL = %f, MeanLumaMinusBL = %f, MeanHistogramMinusBL = %f\n",
                        __FUNCTION__, SetPointMinusBL, MeanLumaMinusBL, MeanHistogramMinusBL );

    MeanRatio = MeanLumaMinusBL / MeanHistogramMinusBL;
    LoopValue = ( ( SetPointMinusBL > MeanHistogramMinusBL )
                        ? ( SetPointMinusBL - MeanHistogramMinusBL )
                        : ( MeanHistogramMinusBL - SetPointMinusBL )
                        )
                        / SetPointMinusBL;

    while ( LoopValue > LoopAccuracy )
    {
        uint32_t i;

        float CorrectedExposureRatio = ( *NewExposure * MeanRatio) / pAecCtx->Exposure;

        /**
         * Stretch or shrink the histogram considering saturation and calculate
         * the new expected mean. Here the ulSumOverBins from above is used again.
         * So it can't be zero and the test for zero can be omitted.
         */
        for ( i=0; i<pAecCtx->ClmHistogramSize; i++ )
        {
            pAecCtx->ClmHistogramX[i] = MIN ( (CorrectedExposureRatio * ( i + 0.5f )), 15.5f );
        }

        pAecCtx->MeanHistogram = 0.0f;
        for ( i=0; i<pAecCtx->ClmHistogramSize; i++ )
        {
            /* avoid division by zero */
            if ( !(pAecCtx->ClmHistogramX[i] > 0.0f) )
            {
                TRACE( CLM_WARN, "%s: ClmHistogramX[%d] == 0 (%f), avoid division by zero\n", __FUNCTION__, i, pAecCtx->ClmHistogramX[i] );
                return ( RET_OUTOFRANGE );
            }
            pAecCtx->MeanHistogram +=  ( (float)(pAecCtx->ClmHistogram[i] * 16) / pAecCtx->SumHistogram ) * pAecCtx->ClmHistogramX[i];
        }

        /* compensate for black level offset */
        /* avoid rMeanHistMinusBL to become negative */
        MeanHistogramMinusBL = MAX( FLT_EPSILON, (pAecCtx->MeanHistogram - MIN( 248, ((BlackLevel * *NewExposure) / pAecCtx->Exposure) )) );

        /* calculate the new exposure disregarding saturation effects */
        *NewExposure *= SetPointMinusBL / MeanHistogramMinusBL;

        /* if the newly calculated exposure is out of range, correct it and
         * leave the loop */
        if ( *NewExposure < pAecCtx->MinExposure )
        {
            TRACE( CLM_WARN, "%s: Exposure too small, correcting from %f to %f\n", __FUNCTION__, *NewExposure, pAecCtx->MinExposure );
            *NewExposure = pAecCtx->MinExposure;
            TRACE( CLM_WARN, "%s: Leaving iteration loop\n", __FUNCTION__ );
            break;
        }

        if ( *NewExposure > pAecCtx->MaxExposure )
        {
            TRACE( CLM_WARN, "%s: Exposure too large, correcting from %f to %f\n", __FUNCTION__, *NewExposure, pAecCtx->MaxExposure );
            *NewExposure = pAecCtx->MaxExposure;
            TRACE( CLM_WARN, "%s: Leaving iteration loop\n", __FUNCTION__ );
            break;
        }

        if ( ++LoopCnt > MaxLoopIterations )
        {
            TRACE( CLM_WARN, "%s: To much iterations (%lu). Leaving iteration loop\n", __FUNCTION__, LoopCnt );
            break;
        }

        LoopValue = ( ( SetPointMinusBL > MeanHistogramMinusBL )
                            ? ( SetPointMinusBL - MeanHistogramMinusBL )
                            : ( MeanHistogramMinusBL - SetPointMinusBL )
                            )
                            / SetPointMinusBL;
    }

    return ( RET_SUCCESS );
}


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/
RESULT ClmExecute
(
    AecContext_t        *pAecCtx,
    const float         SetPoint,
    CamerIcHistBins_t   bins,
    float               *NewExposure
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CLM_INFO, "%s: (enter)\n", __FUNCTION__ );

    TRACE( CLM_DEBUG, "%s: SetPoint = %f, MeanLuma = %f, %f < Exposure=%f <= %f, \n",
                         __FUNCTION__, SetPoint, pAecCtx->MeanLuma, pAecCtx->MinExposure, pAecCtx->Exposure, pAecCtx->MaxExposure );
    TRACE( CLM_DEBUG, "%s: Bins = %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n",
                         __FUNCTION__, bins[0], bins[1], bins[2], bins[3], bins[4], bins[5], bins[6], bins[7],
                              bins[8], bins[9], bins[10], bins[11], bins[12], bins[13], bins[14], bins[15] );

    if ( (bins == NULL) || (NewExposure == NULL) )
    {
        return ( RET_NULL_POINTER );
    }

    pAecCtx->LumaDeviation = ClmCalcLumaDeviation( SetPoint, pAecCtx->MeanLuma );
    if ( pAecCtx->LumaDeviation < 0.0f )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( pAecCtx->LumaDeviation > ( pAecCtx->ClmTolerance / 100.0f ) )
    {
        result = ClmExtrapolateHistogram( pAecCtx, bins );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        result = ClmCalcMeanHistogram( pAecCtx );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }

        result = ClmLoop( pAecCtx, SetPoint, NewExposure );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }
    else
    {
        *NewExposure = pAecCtx->Exposure;
    }

    TRACE( CLM_DEBUG, "%s: NewExposure: %f dL = %f\n", __FUNCTION__, *NewExposure, pAecCtx->LumaDeviation  );

    TRACE( CLM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}


