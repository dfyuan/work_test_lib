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
 * @file sem.c
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
#include <cameric_drv/cameric_isp_exp_drv_api.h>

#include "sem.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( SEM_INFO  , "SEM: ", INFO     , 0 );
CREATE_TRACER( SEM_WARN  , "SEM: ", WARNING  , 1 );
CREATE_TRACER( SEM_ERROR , "SEM: ", ERROR    , 1 );

CREATE_TRACER( SEM_DEBUG , "SEM: ", INFO     , 0 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/
static const float c1  =  10.0f;
static const float c2  = 100.0f;


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * SemCalcRegionLuminaces()
 *****************************************************************************/
static RESULT SemCalcRegionLuminaces
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
)
{
    TRACE( SEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( luma == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    TRACE( SEM_DEBUG, "( 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x )\n",
                luma[CAMERIC_ISP_EXP_GRID_00],
                luma[CAMERIC_ISP_EXP_GRID_10],
                luma[CAMERIC_ISP_EXP_GRID_20],
                luma[CAMERIC_ISP_EXP_GRID_30],
                luma[CAMERIC_ISP_EXP_GRID_40] );

    pAecCtx->SemCtx.sem.MeanLumaRegion0 = ((float)((uint32_t)luma[CAMERIC_ISP_EXP_GRID_00]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_10]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_20]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_30]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_40])) / 5.0f ;

    pAecCtx->SemCtx.sem.MeanLumaRegion1 = ((float)((uint32_t)luma[CAMERIC_ISP_EXP_GRID_04]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_14]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_24]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_34]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_44])) / 5.0f;

    pAecCtx->SemCtx.sem.MeanLumaRegion2 = ((float)((uint32_t)luma[CAMERIC_ISP_EXP_GRID_01]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_11]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_02]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_03])) / 4.0f;

    pAecCtx->SemCtx.sem.MeanLumaRegion3 = ((float)((uint32_t)luma[CAMERIC_ISP_EXP_GRID_31]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_41]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_42]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_43])) / 4.0f;

    pAecCtx->SemCtx.sem.MeanLumaRegion4 = ((float)((uint32_t)luma[CAMERIC_ISP_EXP_GRID_21] 
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_12]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_22]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_32]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_13]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_23]
                                + (uint32_t)luma[CAMERIC_ISP_EXP_GRID_33])) / 7.0f;

    TRACE( SEM_DEBUG, "rg0=%f, rg1=%f, rg2=%f, rg3=%f, rg4=%f\n",
            pAecCtx->SemCtx.sem.MeanLumaRegion0, 
            pAecCtx->SemCtx.sem.MeanLumaRegion1, 
            pAecCtx->SemCtx.sem.MeanLumaRegion2, 
            pAecCtx->SemCtx.sem.MeanLumaRegion3,
            pAecCtx->SemCtx.sem.MeanLumaRegion4 );

    TRACE( SEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcMeanLuminace()
 *****************************************************************************/
static RESULT SemCalcMeanLuminace
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
)
{
    TRACE( SEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( luma == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    pAecCtx->MeanLumaObject = (( 5.0f * pAecCtx->SemCtx.sem.MeanLumaRegion1 ) + ( 7.0f * pAecCtx->SemCtx.sem.MeanLumaRegion4 )) / 12.0f;

    TRACE( SEM_DEBUG, "mean = %f, mean_object = %f\n", pAecCtx->MeanLuma, pAecCtx->MeanLumaObject );

    TRACE( SEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcLuminaceDifference()
 *****************************************************************************/
static RESULT SemCalcLuminaceDifference
(
    AecContext_t *pAecCtx
)
{
    uint32_t sum1;
    uint32_t sum2;

    TRACE( SEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    sum1 = pAecCtx->SemCtx.sem.MeanLumaRegion0 + MAX( pAecCtx->SemCtx.sem.MeanLumaRegion2, pAecCtx->SemCtx.sem.MeanLumaRegion3 );
    sum2 = pAecCtx->SemCtx.sem.MeanLumaRegion1 + pAecCtx->SemCtx.sem.MeanLumaRegion4;

    pAecCtx->d = (sum1 > sum2) ? (sum1 - sum2) : (sum2 - sum1);

    TRACE( SEM_DEBUG, "d = %f\n", pAecCtx->d );

    TRACE( SEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcDegreeOfBackLight()
 *****************************************************************************/
static RESULT SemCalcDegreeOfBackLight
(
    AecContext_t *pAecCtx
)
{
    TRACE( SEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( pAecCtx->d < c1 )
    {
        pAecCtx->z = 0.0f;
    }
    else
    {
        if ( pAecCtx->d > c2 )
        {
            pAecCtx->z = 1.0f;
        }
        else
        {
            pAecCtx->z = ( pAecCtx->d - c1 ) / ( c2 - c1 );
        }
    }

    TRACE( SEM_DEBUG, "z = %f\n", pAecCtx->z );

    TRACE( SEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * SemCalcTargetLuminace()
 *****************************************************************************/
static RESULT SemCalcTargetLuminace
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
)
{
    uint32_t i;

    float value;
    float mean = 0.0f;

    TRACE( SEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    for ( i=0U; i<CAMERIC_ISP_EXP_GRID_ITEMS; i++ )
    {
        value = ( ((float)luma[i]) * pAecCtx->SetPoint ) / pAecCtx->MeanLumaObject;
        mean += ( value > 255.0f ) ? 255.0f : value;
    }

    mean /= (float)CAMERIC_ISP_EXP_GRID_ITEMS;

    pAecCtx->m0 = ( ( 1.0f - pAecCtx->z ) * pAecCtx->SetPoint ) + ( pAecCtx->z * mean );

    TRACE( SEM_DEBUG, "m0 = %f\n", pAecCtx->m0 );

    TRACE( SEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/
RESULT SemExecute
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
)
{
    RESULT result;

    TRACE( SEM_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( luma == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    /* 1.) calculate mean luminaces for frame-regions */
    result = SemCalcRegionLuminaces( pAecCtx, luma );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 2.) calculate mean luminace for current frame */
    result = SemCalcMeanLuminace( pAecCtx, luma );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 3.) calculate difference d for current frame */
    result = SemCalcLuminaceDifference( pAecCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 4.) calculate degree of back/front-light */
    result = SemCalcDegreeOfBackLight( pAecCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    /* 5.) calculate m0 (object luminace to hit) */
    result = SemCalcTargetLuminace( pAecCtx, luma );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    pAecCtx->SemSetPoint = pAecCtx->m0;

    TRACE( SEM_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( result );
}
