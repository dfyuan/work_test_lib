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
#include <common/misc.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_awb_drv_api.h>

#include "awb.h"
#include "awb_ctrl.h"
#include "expprior.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
USE_TRACER( AWB_INFO  );
USE_TRACER( AWB_WARN  );
USE_TRACER( AWB_ERROR );

USE_TRACER( AWB_DEBUG );


#define LOGMIN     0.0001f   //!< lowest value for logarithmic calculations


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
 * ExpPriorMeanValue()
 *****************************************************************************/
static float ExpPriorMeanValue
(
    const float *pItem,
    const uint16_t no
)
{
    float sum = 0.0f;
    float *pCurPtr = (float *)pItem;

    uint16_t i;

    for ( i = 0U; i<no; i++ )
    {
        sum += *pCurPtr;
        pCurPtr ++;
    }

    return ( sum / no );
}



/******************************************************************************
 * ExpPriorAddToIIRFilter()
 *****************************************************************************/
static RESULT ExpPriorAddToIIRFilter
(
    AwbExpPriorContext_t *pExpPriorCtx,
    const float          value
)
{
    pExpPriorCtx->pIIRFilterItems[pExpPriorCtx->IIRCurFilterItem] = value;
    ++pExpPriorCtx->IIRCurFilterItem;

    if ( pExpPriorCtx->IIRCurFilterItem >= pExpPriorCtx->IIRFilterSize )
    {
        pExpPriorCtx->IIRCurFilterItem = 0U;
    }

    return ( RET_SUCCESS );
}




/******************************************************************************
 * AwbExpPriorInit()
 *****************************************************************************/
RESULT AwbExpPriorInit
(
    AwbContext_t        *pAwbCtx,
    AwbExpPriorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* initial checks */
    if ( pConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (pConfig->IIRFilterSize < AWB_EXPPRIOR_FILTER_SIZE_MIN)
            || (pConfig->IIRFilterSize > AWB_EXPPRIOR_FILTER_SIZE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    MEMSET( &pAwbCtx->ExpPriorCtx, 0, sizeof(AwbExpPriorContext_t) );

    pAwbCtx->ExpPriorCtx.IIRFilterSize          = pConfig->IIRFilterSize;
    pAwbCtx->ExpPriorCtx.IIRFilterInitValue     = pConfig->IIRFilterInitValue;

    pAwbCtx->ExpPriorCtx.IIRDampCoefAdd         = pConfig->IIRDampCoefAdd;
    pAwbCtx->ExpPriorCtx.IIRDampCoefSub         = pConfig->IIRDampCoefSub;
    pAwbCtx->ExpPriorCtx.IIRDampFilterThreshold = pConfig->IIRDampFilterThreshold;

    pAwbCtx->ExpPriorCtx.IIRDampingCoefMin      = pConfig->IIRDampingCoefMin;
    pAwbCtx->ExpPriorCtx.IIRDampingCoefMax      = pConfig->IIRDampingCoefMax;
    pAwbCtx->ExpPriorCtx.IIRDampingCoefInit     = pConfig->IIRDampingCoefInit;

    result = AwbExpResizeIIRFilter( pAwbCtx, pConfig->IIRFilterSize, pConfig->IIRFilterInitValue );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbExpPriorReset()
 *****************************************************************************/
RESULT AwbExpPriorReset
(
    AwbContext_t        *pAwbCtx,
    AwbExpPriorConfig_t *pConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* initial checks */
    if ( pConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (pConfig->IIRFilterSize < AWB_EXPPRIOR_FILTER_SIZE_MIN)
            || (pConfig->IIRFilterSize > AWB_EXPPRIOR_FILTER_SIZE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    MEMSET( &pAwbCtx->ExpPriorCtx, 0, sizeof(AwbExpPriorContext_t) );

    pAwbCtx->ExpPriorCtx.IIRFilterSize          = pConfig->IIRFilterSize;
    pAwbCtx->ExpPriorCtx.IIRFilterInitValue     = pConfig->IIRFilterInitValue;

    pAwbCtx->ExpPriorCtx.IIRDampCoefAdd         = pConfig->IIRDampCoefAdd;
    pAwbCtx->ExpPriorCtx.IIRDampCoefSub         = pConfig->IIRDampCoefSub;
    pAwbCtx->ExpPriorCtx.IIRDampFilterThreshold = pConfig->IIRDampFilterThreshold;

    pAwbCtx->ExpPriorCtx.IIRDampingCoefMin      = pConfig->IIRDampingCoefMin;
    pAwbCtx->ExpPriorCtx.IIRDampingCoefMax      = pConfig->IIRDampingCoefMax;
    pAwbCtx->ExpPriorCtx.IIRDampingCoefInit     = pConfig->IIRDampingCoefInit;

    free( pAwbCtx->ExpPriorCtx.pIIRFilterItems );
    pAwbCtx->ExpPriorCtx.pIIRFilterItems = NULL;

    result = AwbExpResizeIIRFilter( pAwbCtx, pConfig->IIRFilterSize, pConfig->IIRFilterInitValue );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * AwbExpPriorRelease()
 *****************************************************************************/
RESULT AwbExpPriorRelease
(
    AwbContext_t *pAwbCtx
)
{
    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( NULL == pAwbCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pAwbCtx->ExpPriorCtx.pIIRFilterItems != NULL )
    {
        free( pAwbCtx->ExpPriorCtx.pIIRFilterItems );
        pAwbCtx->ExpPriorCtx.pIIRFilterItems = NULL;
    }

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbExpPriorRelease()
 *****************************************************************************/
void AwbExpDumpIIRFilter
(
    AwbContext_t *pAwbCtx
)
{
    uint16_t i;

    for (i=0U; i<pAwbCtx->ExpPriorCtx.IIRFilterSize; i++ )
    {
        TRACE( AWB_INFO, "pIIRFilterItems[%d] =  %f\n", i, pAwbCtx->ExpPriorCtx.pIIRFilterItems[i] );
    }
}



/******************************************************************************
 * local function prototypes
 *****************************************************************************/
RESULT AwbExpResizeIIRFilter
(
    AwbContext_t    *pAwbCtx,
    const uint16_t  size,
    const float     initValue
)
{
    AwbExpPriorContext_t *pExpPriorCtx;

    float *pOldIIRFilterItems       = NULL;
    uint16_t OldIIRFilterSize       = 0U;
    uint16_t OldIIRFilterCurrent    = 0U;

    int16_t i;
    int16_t j;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    pExpPriorCtx  = &pAwbCtx->ExpPriorCtx;
    if ( pExpPriorCtx == NULL  )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (size < AWB_EXPPRIOR_FILTER_SIZE_MIN) || (size > AWB_EXPPRIOR_FILTER_SIZE_MAX) )
    {
        return ( RET_OUTOFRANGE );
    }

    if ( pExpPriorCtx->pIIRFilterItems != NULL )
    {
        pOldIIRFilterItems  = pExpPriorCtx->pIIRFilterItems;
        OldIIRFilterSize    = pExpPriorCtx->IIRFilterSize;
        OldIIRFilterCurrent = pExpPriorCtx->IIRCurFilterItem;
    }

    pExpPriorCtx->pIIRFilterItems = (float *)malloc( size * sizeof(float) );
    if ( pExpPriorCtx->pIIRFilterItems == NULL )
    {
        TRACE( AWB_ERROR,  "%s: Can't allocate EPPM-IIRFilter\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }

    pExpPriorCtx->IIRFilterSize         = size;
    pExpPriorCtx->IIRFilterInitValue    = initValue;

    /* preinit with init-value */
    for (i=0; i<size; i++)
    {
        pExpPriorCtx->pIIRFilterItems[i] = initValue;
    }

    /* resize filter ? */
    if ( pOldIIRFilterItems != NULL )
    {
        uint16_t OldIIRFilterStart;
        uint16_t IIRFilterStart;

        /* move read pointer to new position */

        /* Only youngest elements will be copied.
         * Note: Current pointer points to oldest element */
        if ( OldIIRFilterCurrent == 0U )
        {
            OldIIRFilterStart = OldIIRFilterSize - 1U;
        }
        else
        {
            OldIIRFilterStart = OldIIRFilterCurrent - 1U;
        }

        /* move write pointer to new position */
        IIRFilterStart =  pExpPriorCtx->IIRFilterSize - 1U;

        /* copy data */
        for(  j=(int16_t)IIRFilterStart, i=(int16_t)OldIIRFilterStart; (j>=0 && i>=0); j--, i-- )
        {
            /* current downto 0 or full */
            pExpPriorCtx->pIIRFilterItems[j] = pOldIIRFilterItems[i];
        }

        for( i=(OldIIRFilterSize-1); (j>=0 && i>OldIIRFilterStart); j--, i--) /* note that j is not re-initialized */
        {
            /* old size-1 downto current+1 or full */
            pExpPriorCtx->pIIRFilterItems[j] = pOldIIRFilterItems[i];
        }

        free( pOldIIRFilterItems );

        pOldIIRFilterItems = NULL;
    }

    pExpPriorCtx->IIRCurFilterItem =  0U;             /* oldest remaining element */

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * AwbExpPriorProcessFrame()
 *****************************************************************************/
RESULT AwbExpPriorProcessFrame
(
    AwbContext_t *pAwbCtx
)
{
    RESULT result = RET_SUCCESS;

    float p_out = 0.0f;
    float fGExp = 0.0f;
    float mean  = 0.0f;

    TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    /* 1. initial checks */
    if ( pAwbCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* 2. calc. exposure normalized with K */
    fGExp = ( pAwbCtx->SensorGain * pAwbCtx->IntegrationTime * pAwbCtx->pKFactor->fCoeff[0] );
    if ( fGExp < LOGMIN )
    {
        return ( RET_OUTOFRANGE );
    }

    /* 3. calc. the exposure prior probability outdoor {log(0.04) = 3.21888f} */

    /* Calculate the prior probabilities for the Bayes classifier
     *
     * The original implementation based on Gaussian distributions has been replaced by the
     * simpler approximation along three straight lines. Moreover, prior probabilities are 
     * clipped to fExpPriorIn = fExpPriorOut = 0.5, thus in indoor situations all profiles
     * (also outdoor) are equally probable.
     */
    p_out =  ( 0.9f * ( -(float)log( (double)fGExp ) - 3.21888f ) + 0.5f );

    /* 4. range-check and evaluate indoor / outdoor */
    if ( p_out >= 1.0f )
    {
        p_out = 1.0f;
        pAwbCtx->DoorType = EXPPRIOR_OUTDOOR;
    }
    else if ( p_out <= 0.5f )
    {
        p_out = 0.5f;
        pAwbCtx->DoorType = EXPPRIOR_INDOOR;
    }
    else
    {
        pAwbCtx->DoorType = EXPPRIOR_TRANSITION_RANGE;
    }

    /* 5. calc. indoor probability and set in context */
    pAwbCtx->ExpPriorIn  = 1.0f - p_out;
    pAwbCtx->ExpPriorOut = p_out;

    /* 6. calc. mean value of all old indoor probabilities */
    mean = ExpPriorMeanValue( pAwbCtx->ExpPriorCtx.pIIRFilterItems, pAwbCtx->ExpPriorCtx.IIRFilterSize );

    /* 7. add new indoor probability to filter */
    result = ExpPriorAddToIIRFilter( &pAwbCtx->ExpPriorCtx, pAwbCtx->ExpPriorIn );
    RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
    
    /* 8. calc. new damping value */
    if ( fabsf (mean - pAwbCtx->ExpPriorIn) > pAwbCtx->ExpPriorCtx.IIRDampFilterThreshold )
    {
        pAwbCtx->AwbIIRDampCoef -= pAwbCtx->ExpPriorCtx.IIRDampCoefSub;
    }
    else
    {
        pAwbCtx->AwbIIRDampCoef += pAwbCtx->ExpPriorCtx.IIRDampCoefAdd;
    }

    /* 9. keep damping coefficient within allowed range */
    pAwbCtx->AwbIIRDampCoef = MAX( pAwbCtx->ExpPriorCtx.IIRDampingCoefMin, pAwbCtx->AwbIIRDampCoef );
    pAwbCtx->AwbIIRDampCoef = MIN( pAwbCtx->ExpPriorCtx.IIRDampingCoefMax, pAwbCtx->AwbIIRDampCoef );

    TRACE( AWB_DEBUG, "p_out = %f, fGExp = %f\n", p_out, fGExp );

    TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}

