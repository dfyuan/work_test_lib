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
 * @file alsc.c
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

#include "alsc.h"
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

/******************************************************************************
 * VignSelectLscMatrices
 *****************************************************************************/
static RESULT VignSelectLscProfiles
(
    const float     fVignetting,
    int32_t         no_lsc,
    CamLscProfile_t *pLscProfiles[],
    CamLscProfile_t **pLscProfile1,
    CamLscProfile_t **pLscProfile2
)
{
    RESULT result = RET_SUCCESS;

    if ( (no_lsc == 0) || (pLscProfiles == NULL)
            || (pLscProfile1 == NULL) || (pLscProfile2 == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( fVignetting >= pLscProfiles[0]->vignetting )
    {
        *pLscProfile1 = pLscProfiles[0];
        *pLscProfile2 = NULL;

        result = RET_OUTOFRANGE;
    }
    else
    {
        int32_t nLast = no_lsc - 1;
        if ( fVignetting <= pLscProfiles[nLast]->vignetting )
        {
            *pLscProfile1 = pLscProfiles[nLast];
            *pLscProfile2 = NULL;

            result = RET_OUTOFRANGE;
        }
        else
        {
            uint16_t n = 0;

            /* find the segment */
            while( (fVignetting <= pLscProfiles[n]->vignetting) && (n <= nLast) )
            {
                n++;
            }
            n--;

            *pLscProfile1 = pLscProfiles[n];
            *pLscProfile2 = pLscProfiles[n+1];
        }
    }

    return ( result );
}



/******************************************************************************
 * InterpolateMatrices
 *****************************************************************************/
static RESULT VignInterpolateMatrices
(
    const float             fVignetting,
    const CamLscProfile_t   *pLscProfile1,
    const CamLscProfile_t   *pLscProfile2,
    CamLscMatrix_t          *pResMatrix
)
{
    RESULT iResult = RET_NULL_POINTER;

    if ( (pLscProfile1 != NULL) && (pLscProfile2 != NULL) && (pResMatrix != NULL) )
    {
        float fVigA = pLscProfile1->vignetting;
        float fVigB = pLscProfile2->vignetting;

        float f1 = ( fVigB - fVignetting ) / ( fVigB - fVigA );
        /* FLOAT f2 = ( fVigB - fVignetting ) / ( fVigB - fVigA ); */
        float f2 = 1.0f - f1;

        /* left shift 16 */
        uint32_t f1_ = (uint32_t)(f1 * 65536.0f);
        uint32_t f2_ = (uint32_t)(f2 * 65536.0f);

        int16_t i;

        uint32_t red;
        uint32_t greenr;
        uint32_t greenb;
        uint32_t blue;

        for ( i=0; i<(17*17); i++ )
        {
            red     = (f1_ * (uint32_t)pLscProfile1->LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[i])
                        + (f2_ * (uint32_t)pLscProfile2->LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[i]);

            greenr  = (f1_ * (uint32_t)pLscProfile1->LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[i])
                        + (f2_ * (uint32_t)pLscProfile2->LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[i]);

            greenb  = (f1_ * (uint32_t)pLscProfile1->LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[i])
                        + (f2_ * (uint32_t)pLscProfile2->LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[i]);

            blue    = (f1_ * (uint32_t)pLscProfile1->LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[i])
                        + (f2_ * (uint32_t)pLscProfile2->LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[i]);

            /* with round up (add 65536/2 <=> 0.5) before right shift */
            pResMatrix->LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[i]    = (uint16_t)((red + (65536>>1)) >> 16);
            pResMatrix->LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[i] = (uint16_t)((greenr + (65536>>1)) >> 16);
            pResMatrix->LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[i] = (uint16_t)((greenb + (65536>>1)) >> 16);
            pResMatrix->LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[i]   = (uint16_t)((blue + (65536>>1)) >> 16);
        }

        iResult = RET_SUCCESS;
    }

    return ( iResult );
}


/******************************************************************************
 * Damping
 *****************************************************************************/
static RESULT Damping
(
    const float     damp,               /**< damping coefficient */
    CamLscMatrix_t  *pMatrixUndamped,   /**< undamped new computed matrices */
    CamLscMatrix_t  *pMatrixDamped      /**< old matrices and result */
)
{
    RESULT result = RET_NULL_POINTER;

    if ( (pMatrixUndamped != NULL) && (pMatrixDamped != NULL) )
    {
        /* left shift 16 */
        uint32_t f1_ = (uint32_t)(damp * 65536.0f);
        uint32_t f2_ = (uint32_t)(65536U - f1_);

        int16_t i;

        uint32_t red;
        uint32_t greenr;
        uint32_t greenb;
        uint32_t blue;

        for ( i=0; i<(17*17); i++ )
        {
            red     = (f1_ * (uint32_t)pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[i])
                        + (f2_ * (uint32_t)pMatrixUndamped->LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[i]);

            greenr  = (f1_ * (uint32_t)pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[i])
                        + (f2_ * (uint32_t)pMatrixUndamped->LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[i]);

            greenb  = (f1_ * (uint32_t)pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[i])
                        + (f2_ * (uint32_t)pMatrixUndamped->LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[i]);

            blue    = (f1_ * (uint32_t)pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[i])
                        + (f2_ * (uint32_t)pMatrixUndamped->LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[i]);

            /* with round up (add 65536/2 <=> 0.5) before right shift */
            pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_RED].uCoeff[i]    = (uint16_t)((red    + (65536>>1)) >> 16);
            pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_GREENR].uCoeff[i] = (uint16_t)((greenr + (65536>>1)) >> 16);
            pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_GREENB].uCoeff[i] = (uint16_t)((greenb + (65536>>1)) >> 16);
            pMatrixDamped->LscMatrix[ISI_COLOR_COMPONENT_BLUE].uCoeff[i]   = (uint16_t)((blue   + (65536>>1)) >> 16);
        }

        result = RET_SUCCESS;
    }


    return ( result );
}



/******************************************************************************
 * AwbAlscProcessFrame()
 *****************************************************************************/
RESULT AwbAlscProcessFrame
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    CamIlluProfile_t *pDomIlluProfile = NULL;
    CamLscProfile_t *pLscProfile1;
    CamLscProfile_t *pLscProfile2;

    InterpolateCtx_t InterpolateCtx;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1. initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pDomIlluProfile = pAwbCtx->pIlluProfiles[pAwbCtx->DominateIlluProfileIdx];

    InterpolateCtx.size = pDomIlluProfile->VignettingCurve.ArraySize;
    InterpolateCtx.pX   = pDomIlluProfile->VignettingCurve.pSensorGain;
    InterpolateCtx.pY   = pDomIlluProfile->VignettingCurve.pVignetting;
    InterpolateCtx.x_i  = pAwbCtx->SensorGain;
    result = Interpolate( &InterpolateCtx );
    if ( result == RET_OUTOFRANGE )
    {
        result = RET_SUCCESS;
        TRACE( AWB_WARN, "aLSC: Gain compensation input out of range, using max/min value.\n" );
    }
    else if ( result != RET_SUCCESS )
    {
        return ( result );
    }
    
    pAwbCtx->fVignetting = InterpolateCtx.y_i;

    result = VignSelectLscProfiles( InterpolateCtx.y_i,
                                        pDomIlluProfile->lsc_no[pAwbCtx->ResIdx],
                                        pAwbCtx->pLscProfiles[pAwbCtx->ResIdx][pAwbCtx->DominateIlluProfileIdx],
                                        &pLscProfile1, &pLscProfile2 ); 
    if ( result == RET_SUCCESS )
    {
        TRACE( AWB_DEBUG, "fVignetting: %f (%f .. %f)\n",  InterpolateCtx.y_i, pLscProfile1->vignetting, pLscProfile2->vignetting );
        result = VignInterpolateMatrices( InterpolateCtx.y_i, pLscProfile1, pLscProfile2, &pAwbCtx->UndampedLscMatrixTable );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }
    else if ( result == RET_OUTOFRANGE )
    {
        /* we don't need to interpolate */
        TRACE( AWB_DEBUG, "fVignetting: %f (%f)\n",  InterpolateCtx.y_i, pLscProfile1->vignetting );
        memcpy( &pAwbCtx->UndampedLscMatrixTable, &pLscProfile1->LscMatrix[0], sizeof(CamLscMatrix_t) );
    }
    else
    {
        return ( result );
    }

    /* 3. Damping */
    result = Damping( (AWB_WORKING_FLAG_USE_DAMPING & pAwbCtx->Flags) ? pAwbCtx->AwbIIRDampCoef : 0,
                        &pAwbCtx->UndampedLscMatrixTable, &pAwbCtx->DampedLscMatrixTable );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
} 
