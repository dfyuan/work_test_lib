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
#ifndef __CAMERIC_ISP_DPF_DRV_H__
#define __CAMERIC_ISP_DPF_DRV_H__

/**
 * @file cameric_isp_dpf_drv.h
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
 * @defgroup cameric_isp_dpf_drv CamerIc ISP DPF Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_dpf_drv_api.h>


/******************************************************************************
 * Is the hardware DPF module available ?
 *****************************************************************************/
#if defined(MRV_DPF_VERSION) 

/******************************************************************************
 * DPF module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP DPF module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspDpfContext_s
{
    bool_t                          enabled;                /**< module enabled / disabled */

    CamerIcDpfGainUsage_t           GainUsage;              /**< which gains shall be used in preprocessing stage of dpf module */

    CamerIcDpfRedBlueFilterSize_t   RBFilterSize;           /**< size of filter kernel for red/blue pixel */

    bool_t                          ProcessRedPixel;        /**< enable filter processing for red pixel */
    bool_t                          ProcessGreenRPixel;     /**< enable filter processing for green pixel in red lines */
    bool_t                          ProcessGreenBPixel;     /**< enable filter processing for green pixel in blue lines */
    bool_t                          ProcessBluePixel;       /**< enable filter processing for blux pixel */

    CamerIcDpfSpatial_t             SpatialG;               /**< spatial weights for green pixel */
    CamerIcDpfSpatial_t             SpatialRB;              /**< spatial weights for red/blue pixel */
} CamerIcIspDpfContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP DPF driver context 
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspDpfInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP DPF driver context 
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspDpfRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_DPF_VERSION) */

/******************************************************************************
 * DPF module is not available.
 *****************************************************************************/

#define CamerIcIspDpfInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspDpfRelease( hnd )     ( RET_NOTSUPP )

#endif /* #if defined(MRV_DPF_VERSION) */

/* @} cameric_isp_dpf_drv */

#endif /* __CAMERIC_ISP_DPF_DRV_H__ */

