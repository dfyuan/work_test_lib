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
 * @file cameric_isp_lsc_drv.h
 *
 * @brief
 *  Internal interface of LSC Module.
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup cameric_isp_lsc_drv CamerIc ISP LSC Driver Internal API
 * @{
 *
 */
#ifndef __CAMERIC_ISP_LSC_DRV_H__
#define __CAMERIC_ISP_LSC_DRV_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_lsc_drv_api.h>


/******************************************************************************
 * Is the hardware LSC module available ?
 *****************************************************************************/
#if defined(MRV_LSC_VERSION) 

/******************************************************************************
 * LSC module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP LSC module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspLscContext_s
{
    bool_t      enabled;
} CamerIcIspLscContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes the CamerIc ISP LSC driver context.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspLscInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   Release/Free CamerIc ISP LSC driver context.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspLscRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_LSC_VERSION) */

/******************************************************************************
 * LSC module is not available.
 *****************************************************************************/

#define CamerIcIspLscInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspLscRelease( hnd )     ( RET_NOTSUPP )

#endif /* #if defined(MRV_LSC_VERSION) */

/* @} cameric_isp_lsc_drv */

#endif /* __CAMERIC_ISP_LSC_DRV_H__ */
