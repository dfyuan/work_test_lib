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
 * @file acc.c
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
#include <common/cam_types.h>
#include <common/misc.h>

#include "awb.h"
#include "awb_ctrl.h"

#include "acc.h"
#include "interpolate.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( AWB_INFO  );
USE_TRACER( AWB_WARN  );
USE_TRACER( AWB_ERROR );

USE_TRACER( AWB_DEBUG );



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
 *          InterpolateMatrix()
 *
 * @brief   This function interpolates 2 CC-Matrices regarding the saturation
 *          level.
 *
 * @param   fSat            input saturation level
 *          pSatCcMatrixA   pointer to Matrix A
 *          pSatCcMatrixB   pointer to Matrix B
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT InterpolateMatrix
(
    const float             fSat,
    const CamCcProfile_t    *pCcProfileA,
    const CamCcProfile_t    *pCcProfileB,

    Cam3x3FloatMatrix_t     *pResMatrix
)
{
    RESULT result = RET_NULL_POINTER;

    if ( (pCcProfileA != NULL) && (pCcProfileB != NULL) && (pResMatrix != NULL) )
    {
        const Cam3x3FloatMatrix_t *pMatrixA = &pCcProfileA->CrossTalkCoeff;
        const Cam3x3FloatMatrix_t *pMatrixB = &pCcProfileB->CrossTalkCoeff;

        float fSatA = pCcProfileA->saturation;
        float fSatB = pCcProfileB->saturation;

        float f1 = ( fSatB - fSat ) / ( fSatB - fSatA ); // test: if fSat == fSatA => f1 = 1 => choose A: ok
        float f2 = 1.0f - f1;

        int i;

        for ( i=0; i<9; i++)
        {
            pResMatrix->fCoeff[i] = f1 * pMatrixA->fCoeff[i] + f2 * pMatrixB->fCoeff[i];
        }

        result = RET_SUCCESS;
    }

    return ( result );
}



/*****************************************************************************/
/**
 *          CalculateMatrix()
 *
 * @brief   This function searches 
 *
 * @param   fSat            input saturation level
 *          pSatCcMatrixA   pointer to Matrix A
 *          pSatCcMatrixB   pointer to Matrix B
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT CalculateMatrix
(
    const float         fSat,
    const uint32_t      no_cc,
    CamCcProfile_t      *pCcProfiles[],
    Cam3x3FloatMatrix_t *pResMatrix
)
{
    RESULT result = RET_NULL_POINTER;

    if ( (pCcProfiles != NULL) && (no_cc > 0) )
    {
        /* check if saturation smaller than min saturation value => CC_min */
        if ( fSat >= pCcProfiles[0]->saturation )
        {
            memcpy( pResMatrix, &pCcProfiles[0]->CrossTalkCoeff, sizeof(Cam3x3FloatMatrix_t) );
            result = RET_SUCCESS;
        }
        else 
        {
            /* check saturation larger than max value => CC_max */
            uint16_t nLast = (no_cc - 1);

            if ( fSat <= pCcProfiles[nLast]->saturation )
            {
                memcpy( pResMatrix, &pCcProfiles[nLast]->CrossTalkCoeff, sizeof(Cam3x3FloatMatrix_t) );
                result = RET_SUCCESS;
            }
            else
            {
                uint16_t n = 0;

                /* find the segment */
                while( (fSat <= pCcProfiles[n]->saturation) && (n <= nLast) )
                {
                    n++;
                }
                n--;

                /* interpolation of matrices  CC_(n+1) and CC_n */
                result = InterpolateMatrix( fSat, pCcProfiles[n], pCcProfiles[n+1], pResMatrix );
            }
        }
    }

    return ( result );
}



/*****************************************************************************/
/**
 *          InterpolateVector()
 *
 * @brief   This function interpolates 2 CC-Offset vectors regarding the 
 *          saturation level.
 *
 * @param   fSat            input saturation level
 *          pSatCcMatrixA   pointer to Matrix A
 *          pSatCcMatrixB   pointer to Matrix B
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT InterpolateVector
(
    const float             fSat,
    const CamCcProfile_t    *pCcProfileA,
    const CamCcProfile_t    *pCcProfileB,
    Cam1x3FloatMatrix_t     *pResOffset
)
{
    RESULT result = RET_NULL_POINTER;

    if ( (pCcProfileA != NULL) && (pCcProfileB != NULL) && (pResOffset != NULL) )
    {
        const Cam1x3FloatMatrix_t *pOffsetA = &pCcProfileA->CrossTalkOffset;
        const Cam1x3FloatMatrix_t *pOffsetB = &pCcProfileB->CrossTalkOffset;

        float fSatA = pCcProfileA->saturation;
        float fSatB = pCcProfileB->saturation;

        float f1 = ( fSatB - fSat ) / ( fSatB - fSatA ); // test: if fSat == fSatA => f1 = 1 => choose A: ok
        float f2 = 1.0f - f1;

        pResOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_RED]
                = f1 * pOffsetA->fCoeff[CAM_3CH_COLOR_COMPONENT_RED] + f2 * pOffsetB->fCoeff[CAM_3CH_COLOR_COMPONENT_RED];
        pResOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]
                = f1 * pOffsetA->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] + f2 * pOffsetB->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN];
        pResOffset->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]
                = f1 * pOffsetA->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] + f2 * pOffsetB->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE];

        result = RET_SUCCESS;
    }

    return ( result );
}



/*****************************************************************************/
/**
 *          CalculateOffset()
 *
 * @brief   This function interpolates 2 CC-Offset vectors regarding the 
 *          saturation level.
 *
 * @param   fSat            input saturation level
 *          pSatCcMatrixA   pointer to Matrix A
 *          pSatCcMatrixB   pointer to Matrix B
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
static RESULT CalculateOffset
(
    const float         fSat,
    const uint32_t      no_cc,
    CamCcProfile_t      *pCcProfiles[],
    Cam1x3FloatMatrix_t *pResOffset
)
{
    RESULT result = RET_NULL_POINTER;

    if ( (pCcProfiles != NULL) && (no_cc > 0) )
    {
        /* check if saturation smaller than min saturation value => CC_min */
        if ( fSat >= pCcProfiles[0]->saturation )
        {
            memcpy( pResOffset, &pCcProfiles[0]->CrossTalkOffset, sizeof(Cam1x3FloatMatrix_t) );
            result = RET_SUCCESS;
        }
        else 
        {
            /* check saturation larger than max value => CC_max */
            uint16_t nLast = (no_cc - 1);

            if ( fSat <= pCcProfiles[nLast]->saturation )
            {
                memcpy( pResOffset, &pCcProfiles[nLast]->CrossTalkOffset, sizeof(Cam1x3FloatMatrix_t) );
                result = RET_SUCCESS;
            }
            else
            {
                uint16_t n = 0;

                /* find the segment */
                while( (fSat <= pCcProfiles[n]->saturation) && (n <= nLast) )
                {
                    n++;
                }
                n--;

                /* interpolation of matrices  CC_(n+1) and CC_n */
                result = InterpolateVector( fSat, pCcProfiles[n], pCcProfiles[n+1], pResOffset );
            }
        }
    }

    return ( result );
}


/******************************************************************************
 * CalculateOffsetScalingFactor
 *****************************************************************************/
static RESULT CalculateOffsetScalingFactor
(
    AwbContext_t *pAwbCtx,
    float        *pScalingFactor
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1. initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pScalingFactor == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pScalingFactor = 1.0f;

    uint32_t SumHistogram = 0U;
    for ( uint32_t i = 0U;  i < CAMERIC_ISP_HIST_NUM_BINS; i++ )
    {
        SumHistogram += pAwbCtx->Histogram[i];
    }

    /* avoid division by zero */
    if ( SumHistogram == 0U )
    {
        TRACE( AWB_WARN, "%s: SumHistogram == 0, avoid division by zero, correcting to 1\n", __FUNCTION__ );
        SumHistogram = 1U;
    }

    float h0 = (float)pAwbCtx->Histogram[0] / (float)SumHistogram;

    DCT_ASSERT( ( pAwbCtx->ScalingThreshold1 - pAwbCtx->ScalingThreshold0 ) > 0 );
    *pScalingFactor = 1.0f - MIN( 1, MAX( 0, ( h0 - pAwbCtx->ScalingThreshold0 ) / ( pAwbCtx->ScalingThreshold1 - pAwbCtx->ScalingThreshold0 ) ) );

    TRACE( AWB_DEBUG, "bin[0]=%d, sum=%d, h0=%f, f=%f\n", pAwbCtx->Histogram[0], SumHistogram, h0, *pScalingFactor );
    
    return ( RET_SUCCESS );
}


/******************************************************************************
 * Damping
 *****************************************************************************/
static RESULT Damping
(
    const float         damp,                /**< damping coefficient */
    Cam3x3FloatMatrix_t *pMatrixUndamped,   /**< undamped new computed matrices */
    Cam3x3FloatMatrix_t *pMatrixDamped,     /**< old matrices and result */
    Cam1x3FloatMatrix_t *pOffsetUndamped,   /**< undamped new computed offsets */
    Cam1x3FloatMatrix_t *pOffsetDamped      /**< old offset and result */
)
{
    RESULT result = RET_NULL_POINTER;

    if ( (pMatrixUndamped != NULL) && (pMatrixDamped != NULL) 
            && (pOffsetUndamped != NULL) && (pOffsetDamped != NULL) )
    {
        int32_t i;
        float f = (1.0f - damp);

        /* calc. damped cc matrix */
        for( i=0; i<9; i++ )
        {
            pMatrixDamped->fCoeff[i] = (damp * pMatrixDamped->fCoeff[i]) + (f *  pMatrixUndamped->fCoeff[i]);
        }

        /* calc. damped cc offsets */
        pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_RED]
                        = (damp * pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_RED])
                        + (f * pOffsetUndamped->fCoeff[CAM_3CH_COLOR_COMPONENT_RED]);
        pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]
                        = (damp * pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN])
                        + (f * pOffsetUndamped->fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]);
        pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]
                        = (damp * pOffsetDamped->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE])
                        + (f * pOffsetUndamped->fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]);

        result = RET_SUCCESS;
    }

    return ( result );
}




/******************************************************************************
 * AwbAccProcessFrame()
 *****************************************************************************/
RESULT AwbAccProcessFrame
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

    /* 2. reset values */
    memset( pAwbCtx->fSaturation,   0, (AWB_MAX_ILLUMINATION_PROFILES * sizeof(float)) );
    memset( pAwbCtx->CcMatrix,      0, (AWB_MAX_ILLUMINATION_PROFILES * sizeof(Cam3x3FloatMatrix_t)) );
    memset( pAwbCtx->CcOffset,      0, (AWB_MAX_ILLUMINATION_PROFILES * sizeof(Cam1x3FloatMatrix_t)) );

    memset( &pAwbCtx->UndampedCcMatrix, 0, sizeof(Cam3x3FloatMatrix_t) );
    memset( &pAwbCtx->UndampedCcOffset, 0, sizeof(Cam1x3FloatMatrix_t) );

    /* 3. calc new matrices regarding the region */
    if ( pAwbCtx->Region == ILLUEST_REGION_A )
    {
        CamIlluProfile_t *pDomIlluProfile =  pAwbCtx->pIlluProfiles[pAwbCtx->DominateIlluProfileIdx];

        InterpolateCtx_t InterpolateCtx;

        InterpolateCtx.size = pDomIlluProfile->SaturationCurve.ArraySize;
        InterpolateCtx.pX   = pDomIlluProfile->SaturationCurve.pSensorGain;
        InterpolateCtx.pY   = pDomIlluProfile->SaturationCurve.pSaturation;
        InterpolateCtx.x_i  = pAwbCtx->SensorGain;
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

        pAwbCtx->fSaturation[pAwbCtx->DominateIlluProfileIdx] = InterpolateCtx.y_i;

        /* calculate resulting matrix */
        result = CalculateMatrix( InterpolateCtx.y_i, pDomIlluProfile->cc_no,
                                    pAwbCtx->pCcProfiles[pAwbCtx->DominateIlluProfileIdx],
                                    &pAwbCtx->CcMatrix[pAwbCtx->DominateIlluProfileIdx] );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
            
        /* calculate resulting offset */
        result = CalculateOffset( InterpolateCtx.y_i, pDomIlluProfile->cc_no,
                                    pAwbCtx->pCcProfiles[pAwbCtx->DominateIlluProfileIdx],
                                    &pAwbCtx->CcOffset[pAwbCtx->DominateIlluProfileIdx] );
        RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

        /* copy to interim result */
        memcpy( &pAwbCtx->UndampedCcMatrix, &pAwbCtx->CcMatrix[pAwbCtx->DominateIlluProfileIdx], sizeof(Cam3x3FloatMatrix_t) );
        memcpy( &pAwbCtx->UndampedCcOffset, &pAwbCtx->CcOffset[pAwbCtx->DominateIlluProfileIdx], sizeof(Cam1x3FloatMatrix_t) );

    }
    else if ( (pAwbCtx->Region == ILLUEST_REGION_B) || (pAwbCtx->Region == ILLUEST_REGION_C) )
    {
        InterpolateCtx_t InterpolateCtx;

        int32_t i = 0L;
        int32_t j = 0L;
 
        for( i=0; i<pAwbCtx->NoIlluProfiles; i++)
        {
            CamIlluProfile_t *pIlluProfile = pAwbCtx->pIlluProfiles[i];

            InterpolateCtx.size = pIlluProfile->SaturationCurve.ArraySize;
            InterpolateCtx.pX   = pIlluProfile->SaturationCurve.pSensorGain;
            InterpolateCtx.pY   = pIlluProfile->SaturationCurve.pSaturation;
            InterpolateCtx.x_i  = pAwbCtx->SensorGain;
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
        
            pAwbCtx->fSaturation[i] = InterpolateCtx.y_i;

            /* calculate resulting matrix */
            result = CalculateMatrix( InterpolateCtx.y_i, pIlluProfile->cc_no, pAwbCtx->pCcProfiles[i], &pAwbCtx->CcMatrix[i] );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

 
            /* weighting this cc-matrix in the overall resulting cc-matrix */
            for ( j=0; j<9; j++ )
            {
                pAwbCtx->UndampedCcMatrix.fCoeff[j] += ( pAwbCtx->WeightTrans[i] * pAwbCtx->CcMatrix[i].fCoeff[j] );
            }

            /* calculate resulting offset */
            result = CalculateOffset( InterpolateCtx.y_i, pIlluProfile->cc_no, pAwbCtx->pCcProfiles[i], &pAwbCtx->CcOffset[i] );
            RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );

            /* weighting cc-offset-vector in the overall resulting cc-offset-vector */
            pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_RED]
                        += ( pAwbCtx->WeightTrans[i] * pAwbCtx->CcOffset[i].fCoeff[CAM_3CH_COLOR_COMPONENT_RED] );
            pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN]
                        += ( pAwbCtx->WeightTrans[i] * pAwbCtx->CcOffset[i].fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] );
            pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]
                        += ( pAwbCtx->WeightTrans[i] * pAwbCtx->CcOffset[i].fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE] );
        }
    }

#if 0
    /* 3.1 */
    float fRed      = pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_RED];
    float fBlue     = pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE];
    float fGreen    = pAwbCtx->UndampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN];

    float fMax = fRed; 

    if ( fMax > fGreen )
    {
        fMax = fGreen;
    }

    if ( fMax > fBlue )
    {
        fMax = fBlue;
    }

    float s = 4095.0f / ( 4095.0f + fMax );

    int32_t j = 0L;
    for ( j=0; j<9; j++ )
    {
        pAwbCtx->UndampedCcMatrix.fCoeff[j] = ( s *  pAwbCtx->UndampedCcMatrix.fCoeff[j] );
    }
#endif

    float fOffsetScaling = 1.0f;
    result = CalculateOffsetScalingFactor( pAwbCtx, &fOffsetScaling );
    if ( result != RET_SUCCESS )
    {
        TRACE( AWB_WARN, "Clipped offset scaling factor to 1.\n" );
    }

    pAwbCtx->ScalingFactor = (pAwbCtx->DampingFactorScaling * pAwbCtx->ScalingFactor) + (1-pAwbCtx->DampingFactorScaling)*fOffsetScaling;

    /* 4. Damping */
    result = Damping( (AWB_WORKING_FLAG_USE_DAMPING & pAwbCtx->Flags) ? pAwbCtx->AwbIIRDampCoef : 0,
                            &pAwbCtx->UndampedCcMatrix, &pAwbCtx->DampedCcMatrix, 
                            &pAwbCtx->UndampedCcOffset, &pAwbCtx->DampedCcOffset );

    pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_RED]   *= pAwbCtx->ScalingFactor;
    pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_BLUE]  *= pAwbCtx->ScalingFactor;
    pAwbCtx->DampedCcOffset.fCoeff[CAM_3CH_COLOR_COMPONENT_GREEN] *= pAwbCtx->ScalingFactor;

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

