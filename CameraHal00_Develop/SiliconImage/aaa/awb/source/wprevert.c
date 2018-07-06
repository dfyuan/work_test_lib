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
#include <common/cam_types.h>

#include "awb.h"
#include "awb_ctrl.h"
#include "wprevert.h"



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

//! constants for conversion from Y,Cb,Cr to R,G,B
static const float Yuv2RgbCoeff[] =
{
    1.1636025f,  -0.0622839f,   1.6007823f,
    1.1636025f,  -0.4045270f,  -0.7949191f,
    1.1636025f,   1.9911744f,  -0.0250092f
};



/******************************************************************************
 * local function prototypes
 *****************************************************************************/



/******************************************************************************
 * local functions
 *****************************************************************************/

/******************************************************************************
 * GetInverseMatrix
 *****************************************************************************/
static RESULT GetInverseMatrix
(
    const Cam3x3FloatMatrix_t   *pCtMatrix,
    Cam3x3FloatMatrix_t         *pInvCtMatrix
)
{
    RESULT result = RET_NULL_POINTER;

    /* initial check */
    if ( ( pCtMatrix != NULL ) 
            && ( pInvCtMatrix != NULL ) )
    {
        float det;

        /* calculate inverse xtalk matrix
         *
         *   -1              | a33a22-a32a23 a32a13-a33a12 a23a12-a22a13 |
         * A         = 1/|A| | a31a23-a33a21 a33a11-a31a13 a21a13-a23a11 |
         *                   | a32a21-a31a22 a31a12-a32a11 a22a11-a21a12 |
         *
         *
         *  determinant |A| = a11(a33a22-a32a23)-a12(a33a21-a31a23)+a13(a32a21-a31a22)
         */
        det = ( pCtMatrix->fCoeff[0] * ((pCtMatrix->fCoeff[8] * pCtMatrix->fCoeff[4])-(pCtMatrix->fCoeff[7] * pCtMatrix->fCoeff[5])) )
                - ( pCtMatrix->fCoeff[1] * ((pCtMatrix->fCoeff[8] * pCtMatrix->fCoeff[3])-(pCtMatrix->fCoeff[6] * pCtMatrix->fCoeff[5])) )
                + ( pCtMatrix->fCoeff[2] * ((pCtMatrix->fCoeff[7] * pCtMatrix->fCoeff[3])-(pCtMatrix->fCoeff[6] * pCtMatrix->fCoeff[4])) );

        if ( fabsf(det) > DIVMIN )
        {
            pInvCtMatrix->fCoeff[0] = ( (pCtMatrix->fCoeff[8] * pCtMatrix->fCoeff[4]) - (pCtMatrix->fCoeff[7] * pCtMatrix->fCoeff[5]) ) / det;
            pInvCtMatrix->fCoeff[1] = ( (pCtMatrix->fCoeff[7] * pCtMatrix->fCoeff[2]) - (pCtMatrix->fCoeff[8] * pCtMatrix->fCoeff[1]) ) / det;
            pInvCtMatrix->fCoeff[2] = ( (pCtMatrix->fCoeff[5] * pCtMatrix->fCoeff[1]) - (pCtMatrix->fCoeff[4] * pCtMatrix->fCoeff[2]) ) / det;
            pInvCtMatrix->fCoeff[3] = ( (pCtMatrix->fCoeff[6] * pCtMatrix->fCoeff[5]) - (pCtMatrix->fCoeff[8] * pCtMatrix->fCoeff[3]) ) / det;
            pInvCtMatrix->fCoeff[4] = ( (pCtMatrix->fCoeff[8] * pCtMatrix->fCoeff[0]) - (pCtMatrix->fCoeff[6] * pCtMatrix->fCoeff[2]) ) / det;
            pInvCtMatrix->fCoeff[5] = ( (pCtMatrix->fCoeff[3] * pCtMatrix->fCoeff[2]) - (pCtMatrix->fCoeff[5] * pCtMatrix->fCoeff[0]) ) / det;
            pInvCtMatrix->fCoeff[6] = ( (pCtMatrix->fCoeff[7] * pCtMatrix->fCoeff[3]) - (pCtMatrix->fCoeff[6] * pCtMatrix->fCoeff[4]) ) / det;
            pInvCtMatrix->fCoeff[7] = ( (pCtMatrix->fCoeff[6] * pCtMatrix->fCoeff[1]) - (pCtMatrix->fCoeff[7] * pCtMatrix->fCoeff[0]) ) / det;
            pInvCtMatrix->fCoeff[8] = ( (pCtMatrix->fCoeff[4] * pCtMatrix->fCoeff[0]) - (pCtMatrix->fCoeff[3] * pCtMatrix->fCoeff[1]) ) / det;
            result = RET_SUCCESS;
        }
        else
        {
            result = RET_OUTOFRANGE;
        }
    }

    return ( result );
}



/******************************************************************************
 * ConvertMeansToRgb
 *****************************************************************************/
static inline RESULT ConvertMeansToRgb
( 
    const CamerIcAwbMeasuringResult_t *pMeasResult,
    AwbComponent_t                    *pMeanRGB
)
{
    // test if pointer is NULL
    if ( (pMeasResult == NULL ) || (pMeanRGB == NULL ) )
    {
        return ( RET_INVALID_PARM );
    }

    /***
     *
     *  Conversion from YUV to RGB:
     *
     *  R = Coeff0 * (Y - 16) + Coeff1 * (Cb - 128) + Coeff2 * (Cr - 128)
     *  G = Coeff3 * (Y - 16) + Coeff4 * (Cb - 128) + Coeff5 * (Cr - 128)
     *  B = Coeff6 * (Y - 16) + Coeff7 * (Cb - 128) + Coeff8 * (Cr - 128)
     *
     */
    pMeanRGB->fRed   = (Yuv2RgbCoeff[0] * ((float)pMeasResult->MeanY__G - 16.0f))
            + (Yuv2RgbCoeff[1] * ((float)pMeasResult->MeanCb__B - 128.0f))
            + (Yuv2RgbCoeff[2] * ((float)pMeasResult->MeanCr__R - 128.0f));

    pMeanRGB->fGreen = (Yuv2RgbCoeff[3] * ((float)pMeasResult->MeanY__G - 16.0f))
            + (Yuv2RgbCoeff[4] * ((float)pMeasResult->MeanCb__B - 128.0f))
            + (Yuv2RgbCoeff[5] * ((float)pMeasResult->MeanCr__R- 128.0f));

    pMeanRGB->fBlue  = (Yuv2RgbCoeff[6] * ((float)pMeasResult->MeanY__G - 16.0f))
            + (Yuv2RgbCoeff[7] * ((float)pMeasResult->MeanCb__B - 128.0f))
            + (Yuv2RgbCoeff[8] * ((float)pMeasResult->MeanCr__R - 128.0f));

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SetMeansToRgb
 *****************************************************************************/
static inline RESULT SetMeansToRgb
( 
    const CamerIcAwbMeasuringResult_t *pMeasResult,
    AwbComponent_t                    *pMeanRGB
)
{
    // test if pointer is NULL
    if ( (pMeasResult == NULL ) || (pMeanRGB == NULL ) )
    {
        return ( RET_INVALID_PARM );
    }

    pMeanRGB->fRed   = (float)pMeasResult->MeanCr__R;
    pMeanRGB->fGreen = (float)pMeasResult->MeanY__G;
    pMeanRGB->fBlue  = (float)pMeasResult->MeanCb__B;
 
    return ( RET_SUCCESS );
}



/******************************************************************************
 * WpRevertProcessFrame()
 *****************************************************************************/
RESULT AwbWpRevertProcessFrame
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    AwbComponent_t      MeansRgb;
    Cam3x3FloatMatrix_t InvCtMatrix;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1. initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* 2. means to RGB  */
    switch ( pAwbCtx->MeasMode )
    {
        case CAMERIC_ISP_AWB_MEASURING_MODE_YCBCR:
            {
                result = ConvertMeansToRgb( &pAwbCtx->MeasuredMeans, &MeansRgb ); 
     
                break;
            }

        case CAMERIC_ISP_AWB_MEASURING_MODE_RGB:
            {
                result = SetMeansToRgb( &pAwbCtx->MeasuredMeans, &MeansRgb );
                break;
            }

        default:
            {
                return ( RET_OUTOFRANGE );
            }
    }

    /* 3. revert calculation of means */
    result = GetInverseMatrix( &pAwbCtx->CtMatrix, &InvCtMatrix );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 2. transform the RGB values with the inverse xtalk matrix
     *
     *   | fAwbMeanR | = | fAwbMeanR |         -1
     *   | fAwbMeanG | = | fAwbMeanG | x Xtalk
     *   | fAwbMeanB | = | fAwbMeanB |
     */
    pAwbCtx->RevertedMeansRgb.fRed   = InvCtMatrix.fCoeff[0] * MeansRgb.fRed  
                                        + InvCtMatrix.fCoeff[1] * MeansRgb.fGreen
                                        + InvCtMatrix.fCoeff[2] * MeansRgb.fBlue;

    pAwbCtx->RevertedMeansRgb.fGreen = InvCtMatrix.fCoeff[3] * MeansRgb.fRed
                                        + InvCtMatrix.fCoeff[4] * MeansRgb.fGreen
                                        + InvCtMatrix.fCoeff[5] * MeansRgb.fBlue;

    pAwbCtx->RevertedMeansRgb.fBlue  = InvCtMatrix.fCoeff[6] * MeansRgb.fRed
                                        + InvCtMatrix.fCoeff[7] * MeansRgb.fGreen
                                        + InvCtMatrix.fCoeff[8] * MeansRgb.fBlue;

    /* 3. normalize the mean value to gains
     *
     * divide the awb mean values by the AWB gains
     *
     *    fAwbMeanR = fAwbMeanR / fAwbGainR
     *    fAwbMeanG = fAwbMeanG / fAwbGainG
     *    fAwbMeanB = fAwbMeanB / fAwbGainB
     */
    if ( (pAwbCtx->Gains.fRed > DIVMIN)
            && (pAwbCtx->Gains.fGreenR > DIVMIN)
            && (pAwbCtx->Gains.fGreenB > DIVMIN)
            && (pAwbCtx->Gains.fBlue > DIVMIN) )
    {
        pAwbCtx->RevertedMeansRgb.fRed   /= pAwbCtx->Gains.fRed;
        pAwbCtx->RevertedMeansRgb.fGreen /= ( (pAwbCtx->Gains.fGreenR +  pAwbCtx->Gains.fGreenB) / 2 );
        pAwbCtx->RevertedMeansRgb.fBlue  /= pAwbCtx->Gains.fBlue;

        result = RET_SUCCESS;
    }
    else
    {
        result = RET_OUTOFRANGE;
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

