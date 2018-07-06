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
#ifndef __CAMERIC_ISP_DPCC_DRV_H__
#define __CAMERIC_ISP_DPCC_DRV_H__

/**
 * @file cameric_isp_dpcc_drv.h
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
 * @defgroup cameric_isp_dpcc_drv CamerIc ISP DPCC Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_dpcc_drv_api.h>


/******************************************************************************
 * Is the hardware DPCC module available ?
 *****************************************************************************/
#if defined(MRV_DPCC_VERSION) 

/******************************************************************************
 * DPCC module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP DPCC module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspDpccContext_s
{
    bool_t      enabled;
} CamerIcIspDpccContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP DPCC driver context 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspDpccInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP DPCC driver context 
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspDpccRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_DPCC_VERSION) */

/******************************************************************************
 * DPCC module is not available.
 *****************************************************************************/

#define CamerIcIspDpccInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspDpccRelease( hnd )     ( RET_NOTSUPP )

#endif /* #if defined(MRV_DPCC_VERSION) */

/* @} cameric_isp_dpcc_drv */

#endif /* __CAMERIC_ISP_DPCC_DRV_H__ */

