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
#ifndef __CAMERIC_IE_DRV_H__
#define __CAMERIC_IE_DRV_H__

/**
 * @cond    cameric_ie_drv
 *
 * @file    cameric_ie_drv.h
 *
 * @brief   This file contains driver internal definitions for the CamerIC 
 *          driver IE (image effects) module.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_ie_drv CamerIc IE Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_ie_drv_api.h>


/******************************************************************************
 * Is the hardware Image Effects module available ?
 *****************************************************************************/
#if defined(MRV_IMAGE_EFFECTS_VERSION)

/******************************************************************************
 * Image Effects Module is available.
 *****************************************************************************/

/******************************************************************************/
/**
 * @brief driver internal context of Image Effects Module.
 *
 *****************************************************************************/
typedef struct CamerIcIeContext_s
{
    bool_t                              enabled;            /**< IE enabled */

    CamerIcIeConfig_t                   config;             /**< current configuration of IE module */
} CamerIcIeContext_t;


/*****************************************************************************/
/**
 * @brief   This function enables the clock for the CamerIC IE (Image
 *          Effects) module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIeEnableClock
(
    CamerIcDrvHandle_t  handle
);


/*****************************************************************************/
/**
 * @brief   This function disables the clock for the CamerIC IE (Image
 *          effects) module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIeDisableClock
(
    CamerIcDrvHandle_t  handle
);


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIC IE (Image Effects) driver context. 
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIeInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees the  CamerIC IE (Image Effects)
 *          driver context .
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIeRelease
(
    CamerIcDrvHandle_t handle
);


#else  /* #if defined(MRV_IMAGE_EFFECTS_VERSION) */

/******************************************************************************
 * Image Effects Module is not available.
 *****************************************************************************/

#define CamerIcIeEnableClock( hnd )       ( RET_NOTSUPP )
#define CamerIcIeDisableClock( hnd )      ( RET_NOTSUPP )
#define CamerIcIeInit( hnd )              ( RET_NOTSUPP )
#define CamerIcIeRelease( hnd )           ( RET_NOTSUPP )

#endif /* #if defined(MRV_IMAGE_EFFECTS_VERSION) */

/* @} cameric_ie_drv */

#endif /* __CAMERIC_IE_DRV_H__ */

