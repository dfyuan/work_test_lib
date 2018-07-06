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
#ifndef __CAMERIC_CPROC_DRV_H__
#define __CAMERIC_CPROC_DRV_H__

/**
 * @cond    cameric_cproc
 *
 * @file    cameric_cproc_drv.h
 *
 * @brief   This file contains driver internal definitions for the CamerIC 
 *          driver CPROC module.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_cproc_drv CamerIc CPROC Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_cproc_drv_api.h>


/******************************************************************************
 * Is the hardware Color Processing module available ?
 *****************************************************************************/
#if defined(MRV_COLOR_PROCESSING_VERSION)

/******************************************************************************
 * Color Processing Module is available.
 *****************************************************************************/

/******************************************************************************/
/**
 * @brief driver internal context of Color Processing Module.
 *
 *****************************************************************************/
typedef struct CamerIcCprocContext_s
{
    bool_t                              enabled;        /**< CPROC enabled */
    CamerIcCprocConfig_t                config;         /**< current configuration of CPROC module */
} CamerIcCprocContext_t;



/*****************************************************************************/
/**
 * @brief   This function enables the clock for the CamerIC CPROC (Color
 *          procssing) module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcCprocEnableClock
(
    CamerIcDrvHandle_t  handle
);



/*****************************************************************************/
/**
 * @brief   This function disables the clock for the CamerIC CPROC (Color
 *          processing) module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcCprocDisableClock
(
    CamerIcDrvHandle_t  handle
);



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIC CPROC (color processing) driver
 *          context. 
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcCprocInit
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function releases/frees the  CamerIC CPROC (color processing)
 *          driver context.
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcCprocRelease
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_COLOR_PROCESSING_VERSION) */

/******************************************************************************
 * Color Processing Module is not available.
 *****************************************************************************/

#define CamerIcCprocEnableClock( hnd )      ( RET_NOTSUPP )
#define CamerIcCprocDisableClock( hnd )     ( RET_NOTSUPP )
#define CamerIcCprocInit( hnd )             ( RET_NOTSUPP )
#define CamerIcCprocRelease( hnd )          ( RET_NOTSUPP )

#endif /* #if defined(MRV_COLOR_PROCESSING_VERSION) */


/* @} cameric_cproc_drv */

/* @endcond */

#endif /* __CAMERIC_CPROC_DRV_H__ */

