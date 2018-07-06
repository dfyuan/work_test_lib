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
#ifndef __CAMERIC_ISP_ELAWB_DRV_H__
#define __CAMERIC_ISP_ELAWB_DRV_H__

/**
 * @file cameric_isp_elawb_drv.h
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
 * @defgroup cameric_isp_elawb_drv CamerIc ISP Eliptic AWB Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_elawb_drv_api.h>


/******************************************************************************
 * Is the hardware eliptic WB measuring module available ?
 *****************************************************************************/
#if defined(MRV_ELAWB_VERSION)

/******************************************************************************
 * eliptic WB measuring module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   Structure of a measuring elipsis
 *
 *****************************************************************************/
typedef struct CamerIcIspElAwbElipse_s
{
    uint16_t    x;              /**< ellipsoid center x */
    uint16_t    y;              /**< ellipsoid center y */

    uint16_t    a1;             /**< rotation matrix coefficient (1,1) =  sin(angle) */
    uint16_t    a2;             /**< rotation matrix coefficient (1,2) =  cos(angle) */
    uint16_t    a3;             /**< rotation matrix coefficient (2,1) = -sin(angle) */
    uint16_t    a4;             /**< rotation matrix coefficient (2,2) =  cos(angle) */

    uint32_t    r_max_sqr;      /**< max radius square of ellipsiod */
} CamerIcIspElAwbElipse_t;


/*****************************************************************************/
/**
 * @brief CamerIc ISP AF module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspElAwbContext_s
{
    bool_t                  enabled;                                        /**< measuring enabled */
    bool_t                  MedianFilter;                                   /**< use median filter in pre filter module */

    CamerIcEventCb_t        EventCb;                                        /**< Event Callback */
    CamerIcWindow_t         Window;                                         /**< measuring window */

    CamerIcIspElAwbElipse_t Elipsis[CAMERIC_ISP_AWB_ELIPSIS_ID_MAX - 1U];
} CamerIcIspElAwbContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP Auto white balance driver
 *          context 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspElAwbInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP Auto white balance driver
 *          context 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspElAwbRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP ElAwb module 
 *          driver (It 's called internally from ISP main driver module when
 *          measurment ready or error interrupt is raised). 
 *
 * @param   handle              CamerIc driver handle
 *
 *****************************************************************************/
void CamerIcIspElAwbSignal
(
    CamerIcDrvHandle_t handle
);


#else  /* #if defined(MRV_ELAWB_VERSION) */

/******************************************************************************
 * AF module is not available.
 *****************************************************************************/

#define CamerIcIspElAwbInit( hnd )      ( RET_NOTSUPP )
#define CamerIcIspElAwbRelease( hnd )   ( RET_NOTSUPP )
#define CamerIcIspElAwbSignal( hnd )

#endif /* #if defined(MRV_ELAWB_VERSION) */


/* @} cameric_isp_elawb_drv */

#endif /* __CAMERIC_ISP_ELAWB_DRV_H__ */

