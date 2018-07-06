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
#ifndef __CAMERIC_ISP_BLS_DRV_H__
#define __CAMERIC_ISP_BLS_DRV_H__

/**
 * @file cameric_isp_bls_drv.h
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
 * @defgroup cameric_isp_bls_drv CamerIc ISP BLS Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_bls_drv_api.h>


/******************************************************************************
 * Is the hardware BLS module available ?
 *****************************************************************************/
#if defined(MRV_BLACK_LEVEL_VERSION) 

/******************************************************************************
 * BLS module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP BLS module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspBlsContext_s
{
    bool_t                  enabled;            /**< module enabled */

    uint16_t                isp_bls_a_fixed;    /**< black-level-substraction value */
    uint16_t                isp_bls_b_fixed;    /**< black-level-substraction value */
    uint16_t                isp_bls_c_fixed;    /**< black-level-substraction value */
    uint16_t                isp_bls_d_fixed;    /**< black-level-substraction value */

    CamerIcWindow_t         Window1;            /**< measuring window 1 */ 
    CamerIcWindow_t         Window2;            /**< measuring window 2 */ 
} CamerIcIspBlsContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP BLS driver context. 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspBlsInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP BLS driver context. 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspBlsRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_BLACK_LEVEL_VERSION) */

/******************************************************************************
 * BLS module is not available.
 *****************************************************************************/

#define CamerIcIspBlsInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspBlsRelease( hnd )     ( RET_NOTSUPP )

#endif /* #if defined(MRV_BLACK_LEVEL_VERSION) */


/* @} cameric_isp_bls_drv */


#endif /* __CAMERIC_ISP_BLS_DRV_H__ */

