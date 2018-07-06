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
#ifndef __BSSA_H__
#define __BSSA_H__

/**
 * @file bssa.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup BSSA   Basic Step Search Algorithm
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>


#ifdef __cplusplus
extern "C"
{
#endif



// /////////////////////////////////////////////
//! context of basic step search algorithm (BSSA)
#define STATE_BSSA_START     0
#define STATE_BSSA_SEARCH    1
#define STATE_BSSA_DONE      2

typedef enum BssaState_e
{
    BSSA_STATE_INVALID  = 0,    
    BSSA_STATE_START    = 1,
    BSSA_STATE_SEARCH   = 2,
    BSSA_STATE_DONE     = 3,
    BSSA_STATE_MAX
} BssaState_t;



typedef struct BssaCtx_s
{ 
   BssaState_t  State;          /**< state of algorithm (one of the STATE_BSSA_xxx defines) */

   // results
   int32_t      LensPosBest;    /**< best lens position */
   int32_t      LensPosWorst;   /**< worst lens position */
   uint32_t     SharpnessMax;   /**< sharpness at best lens position */
   uint32_t     SharpnessMin;   /**< sharpness at worst lens position  */

    // parameters
   int32_t      LensStart;      /**< lens position to start the search */
   int32_t      LensStop;       /**< lens position to stop the search */
   int32_t      Step;           /**< step size (always positive, even if ulLensStop < ulLensStart) */
} BssaCtx_t;



void AfReinitBssa
(
    BssaCtx_t       *pBssaCtx,
    const int32_t   Start,
    const int32_t   Stop
);



RESULT AfSearchBssa
(
    BssaCtx_t       *pCtx,
    const uint32_t  Sharpness,
    int32_t         *pLensPosition
);

int32_t AfIsBestAtRangeBorder
(
    BssaCtx_t   *pBssaCtx
);


#ifdef __cplusplus
}
#endif

/* @} BSSA */


#endif /* __BSSA_H__*/
