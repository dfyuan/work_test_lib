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
 * @file cameric_isp.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_scale_drv.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_SCALE_DRV_INFO  , "CAMERIC-SCALE-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_SCALE_DRV_WARN  , "CAMERIC-SCALE-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_SCALE_DRV_ERROR , "CAMERIC-SCALE-DRV: ", ERROR   , 1 );
                                                 

/******************************************************************************
 * flag to set in scalefactor values to enable upscaling
 *****************************************************************************/
#define RSZ_UPSCALE_ENABLE  0x08000U



/******************************************************************************
 * flag to set in scalefactor values to bypass the scaler block
 *****************************************************************************/
#define RSZ_SCALER_BYPASS   0x10000U



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
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcCalcScaleFactor()
 *****************************************************************************/
RESULT CamerIcCalcScaleFactor
(
    const uint16_t  in,
    const uint16_t  out,
    uint32_t        *factor

)
{
    TRACE( CAMERIC_SCALE_DRV_INFO, "%s: (enter) \n", __FUNCTION__ );

    if ( factor == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        if ( in > out )
        {
            /* down scaling */
            *factor = ((((uint32_t)out - 1UL) * RSZ_SCALER_BYPASS) / ((uint32_t)in - 1UL)) + 1UL;
        }
        else if ( in < out )
        {
            /* up scaling */
            *factor = ((((uint32_t)in - 1UL) * RSZ_SCALER_BYPASS) / ((uint32_t)out - 1UL)) | RSZ_UPSCALE_ENABLE;
        }
        else
        {
            /* bypass (no scaling) */
            *factor = RSZ_SCALER_BYPASS;
        }
    }

    TRACE( CAMERIC_SCALE_DRV_INFO, "%s: (exit) \n", __FUNCTION__ );

    return ( RET_SUCCESS );
}
 
