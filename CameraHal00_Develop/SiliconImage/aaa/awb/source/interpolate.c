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
 * @file interpolate.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

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
 * Interpolate()
 *****************************************************************************/
RESULT Interpolate
(
    InterpolateCtx_t *pInterpolationCtx
)
{
    uint16_t n    = 0U;
    uint16_t nMax = 0U;

    // TRACE( AWB_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( pInterpolationCtx == NULL )
    {
        return ( RET_NULL_POINTER );
    }

    nMax = (pInterpolationCtx->size - 1U);

    /* lower range check */
    if ( pInterpolationCtx->x_i < pInterpolationCtx->pX[0] )
    {
        pInterpolationCtx->y_i = pInterpolationCtx->pY[0];
        return ( RET_OUTOFRANGE );
    }

    /* upper range check */
    if( pInterpolationCtx->x_i > pInterpolationCtx->pX[nMax] )
    {
        pInterpolationCtx->y_i = pInterpolationCtx->pY[nMax];
        return ( RET_OUTOFRANGE );
    }

    /* find x area */
    n = 0;
    while ( (pInterpolationCtx->x_i >= pInterpolationCtx->pX[n]) && (n <= nMax) )
    {
        ++n;
    }
    --n;

    /**
     * If n was larger than nMax, which means f_xi lies exactly on the 
     * last interval border, we count f_xi to the last interval and 
     * have to decrease n one more time */
    if ( n == nMax )
    {
        --n;
    }

    pInterpolationCtx->y_i
        = ( (pInterpolationCtx->pY[n+1] - pInterpolationCtx->pY[n]) / (pInterpolationCtx->pX[n+1] - pInterpolationCtx->pX[n]) )
            * ( pInterpolationCtx->x_i - pInterpolationCtx->pX[n] ) 
            + ( pInterpolationCtx->pY[n] );

    // TRACE( AWB_INFO, "%s: (exit)\n", __FUNCTION__);
    
    return ( RET_SUCCESS );
}


