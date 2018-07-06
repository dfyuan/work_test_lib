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
#ifndef __CAMERIC_SIMP_DRV_H__
#define __CAMERIC_SIMP_DRV_H__

/**
 * @cond    cameric_simp_drv
 *
 * @file    cameric_simp_drv.h
 *
 * @brief   This file contains driver internal definitions for the CamerIC 
 *          driver SIMP (super impose) module.
 *
 *****************************************************************************/
/**
 * @defgroup cameric_simp_drv CamerIc SIMP Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_simp_drv_api.h>


/******************************************************************************
 * Is the hardware Super-Impose module available ?
 *****************************************************************************/
#if defined(MRV_SUPER_IMPOSE_VERSION)

/******************************************************************************
 * Super-Impose Module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc Sumper-Impose signals delivered from MI driver module
 *          into AF module.
 *
 *****************************************************************************/
typedef enum CamerIcIspSimpSignal_e
{
    CAMERIC_SIMP_SIGNAL_INVALID         = 0,
    CAMERIC_SIMP_SIGNAL_END_OF_FRAME    = 1,
    CAMERIC_SIMP_SIGNAL_LAST
} CamerIcIspSimpSignal_t;


/******************************************************************************/
/**
 * @brief driver internal context of Super-Impose module.
 *
 *****************************************************************************/
typedef struct CamerIcSimpContext_s
{
    bool_t              enabled;            /**< SIMP enabled */
    CamerIcSimpConfig_t config;            
} CamerIcSimpContext_t;


/*****************************************************************************/
/**
 * @brief   This function enables the clock for the CamerIC SIMP (Super-
 *          Impose) module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcSimpEnableClock
(
    CamerIcDrvHandle_t  handle
);


/*****************************************************************************/
/**
 * @brief   This function disables the clock for the CamerIC SIMP (Super-
 *          Impose) module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcSimpDisableClock
(
    CamerIcDrvHandle_t  handle
);


/*****************************************************************************/
/**
 * @brief   This function initialize CamerIC SIMP (Super-Impose) driver context. 
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcSimpInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees the  CamerIC SIMP (Super-Impose)
 *          driver context.
 *
 * @return                  	Return the result of the function call.
 * @retval	RET_SUCCESS			operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcSimpRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP AF module 
 *          driver (It 's called internally from MI driver module when
 *          measurment ready or error interrupt is raised). 
 *
 * @param   handle              CamerIc driver handle
 * @param   signal              Signal to deliver
 *
 *****************************************************************************/
void CamerIcSimpSignal
(
    const CamerIcIspSimpSignal_t    signal,
    CamerIcDrvHandle_t              handle
);


#else  /* #if defined(MRV_SUPER_IMPOSE_VERSION) */

/******************************************************************************
 * Super-Impose Module is not available.
 *****************************************************************************/

#define CamerIcSimpEnableClock( hnd )       ( RET_NOTSUPP )
#define CamerIcSimpDisableClock( hnd )      ( RET_NOTSUPP )
#define CamerIcSimpInit( hnd )              ( RET_NOTSUPP )
#define CamerIcSimpRelease( hnd )           ( RET_NOTSUPP )
#define CamerIcSimpSignal( signal, hnd )

#endif /* #if defined(MRV_SUPER_IMPOSE_VERSION) */


/* @} cameric_simp_drv */

#endif /* __CAMERIC_SIMP_DRV_H__ */

