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
#ifndef __CAMERIC_ISP_CAC_DRV_H__
#define __CAMERIC_ISP_CAC_DRV_H__

/**
 * @file cameric_isp_cac_drv.h
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
 * @defgroup cameric_isp_cac_drv CamerIc ISP CAC Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_cac_drv_api.h>


/******************************************************************************
 * Is the hardware CAC module available ?
 *****************************************************************************/
#if defined(MRV_CAC_VERSION)

/******************************************************************************
 * Chromatic Aberation Correction Module is available.
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   CamerIc ISP CAC module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspCacContext_s
{
    bool_t                              enabled;        /**< CAC enabled */

    uint16_t                            width;          /**< width of the input image in pixel */
    uint16_t                            height;         /**< height of the input image in pixel */

    int16_t                             hCenterOffset;  /**< horizontal offset between image center and optical enter of the input image in pixels */
    int16_t                             vCenterOffset;  /**< vertical offset between image center and optical enter of the input image in pixels */

    CamerIcIspCacHorizontalClipMode_t   hClipMode;
    CamerIcIspCacVerticalClipMode_t     vClipMode;

    uint16_t                            aBlue;
    uint16_t                            aRed;
    uint16_t                            bBlue;
    uint16_t                            bRed;
    uint16_t                            cBlue;
    uint16_t                            cRed;

    uint8_t                             Xns;            /**< horizontal normal shift parameter */
    uint8_t                             Xnf;            /**< horizontal scaling factor */
    
    uint8_t                             Yns;            /**< vertical normal shift parameter */
    uint8_t                             Ynf;            /**< vertical scaling factor */
} CamerIcIspCacContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP Chromatic Aberration 
 *          Correction driver context 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspCacInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP Chromatic Aberration
 *          Correction driver context 
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspCacRelease
(
    CamerIcDrvHandle_t handle
);


#else  /* #if defined(MRV_CAC_VERSION) */

/******************************************************************************
 * Chromatic Aberation Correction Module is not available.
 *****************************************************************************/

#define CamerIcIspCacInit( hnd )                ( RET_NOTSUPP )
#define CamerIcIspCacRelease( hnd )             ( RET_NOTSUPP )

#endif /* #if defined(MRV_CAC_VERSION) */


/* @} cameric_isp_cac_drv */

#endif /* __CAMERIC_ISP_WDR_DRV_H__ */

