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
#ifndef __CAMERIC_ISP_DEGAMMA_DRV_H__
#define __CAMERIC_ISP_DEGAMMA_DRV_H__

/**
 * @file cameric_isp_degamma_drv.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @defgroup cameric_isp_degamma_drv CamerIc ISP DEGAMMA Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_degamma_drv_api.h>


/******************************************************************************
 * Is the hardware Degamma module available ?
 *****************************************************************************/
#if defined(MRV_GAMMA_IN_VERSION) 

/******************************************************************************
 * Degamma module is available.
 *****************************************************************************/

/******************************************************************************/
/**
 * @brief driver internal context of ISP Degamma Module.
 *
 *****************************************************************************/
typedef struct CamerIcIspDegammaContext_s
{
    bool_t                      enabled;
    CamerIcIspDegammaCurve_t    curve;
} CamerIcIspDegammaContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP DEGAMMA driver context 
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspDegammaInit
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
RESULT CamerIcIspDegammaRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_GAMMA_IN_VERSION) */

/******************************************************************************
 * Degamma module is not available.
 *****************************************************************************/

#define CamerIcIspDegammaInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspDegammaRelease( hnd )     ( RET_NOTSUPP )

#endif /* #if defined(MRV_GAMMA_IN_VERSION) */

/* @} cameric_isp_degamma_drv */

#endif /* __CAMERIC_ISP_DEGAMMA_DRV_H__ */

