/******************************************************************************
 *
 * Copyright 2012, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
#ifndef __CAMERIC_ISP_IS_DRV_H__
#define __CAMERIC_ISP_IS_DRV_H__

/**
 * @file cameric_isp_is_drv.h
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
 * @defgroup cameric_isp_vs_drv CamerIc ISP IS Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include "cameric_isp_is_drv_api.h"


/******************************************************************************
 * Is the hardware image stabilization module available ?
 *****************************************************************************/
#if defined(MRV_IS_VERSION)

/******************************************************************************
 * Image stabilization module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP image stabilization module
 *          internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspIsContext_s
{
    bool_t      enabled;     /**< image stabilization feature enabled */
    uint8_t     recenterExp; /**< configured recenter exponent */

    CamerIcWindow_t outWin;  /**< configured output window which is regarded
                                  as "centered" */

    uint16_t  maxDisplHor;   /**< configured maximum horizontal displacement
                                  from outWin */
    uint16_t  maxDisplVer;   /**< configured maximum vertical displacement
                                  from outWin */
    int16_t   displHor;      /**< configured horizontal displacement
                                  from outWin */
    int16_t   displVer;      /**< configured vertical displacement
                                  from outWin */
} CamerIcIspIsContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP IS driver context.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspIsInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP IS driver context.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspIsRelease
(
    CamerIcDrvHandle_t handle
);


#else  /* #if defined(MRV_IS_VERSION)  */

/******************************************************************************
 * Video stabilization measurement module is not available.
 *****************************************************************************/
#define CamerIcIspIsInit( hnd )    ( RET_NOTSUPP )
#define CamerIcIspIsRelease( hnd ) ( RET_NOTSUPP )

#endif /* #if defined(MRV_IS_VERSION)  */

/* @} cameric_isp_vs_drv */

#endif /* __CAMERIC_ISP_IS_DRV_H__ */

