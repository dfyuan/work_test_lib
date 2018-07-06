/******************************************************************************
 *
 * Copyright 2013, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
#ifndef __CAMERIC_ISP_CNR_DRV_H__
#define __CAMERIC_ISP_CNR_DRV_H__

/**
 * @file cameric_isp_cnr_drv.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_cnr_drv CamerIc ISP Color noise reduction Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_cnr_drv_api.h>


/******************************************************************************
 * Is the hardware Color noise reduction module available ?
 *****************************************************************************/
#if defined(MRV_CNR_VERSION) 

/******************************************************************************
 * Color noise reduction module is available.
 *****************************************************************************/

/******************************************************************************/
/**
 * @brief driver internal context of ISP CNR Module.
 *
 *****************************************************************************/
typedef struct CamerIcIspCnrContext_s
{
    bool_t                      enabled;
} CamerIcIspCnrContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP CNR driver context 
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspCnrInit
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Release/Free CamerIc ISP DEGAMMA driver context 
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspCnrRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_CNR_VERSION) */

/******************************************************************************
 * Color noise reduction module is not available.
 *****************************************************************************/

#define CamerIcIspCnrInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspCnrRelease( hnd )     ( RET_NOTSUPP )

#endif /* #if defined(MRV_CNR_VERSION) */

/* @} cameric_isp_degamma_drv */

#endif /* __CAMERIC_ISP_CNR_DRV_H__ */


