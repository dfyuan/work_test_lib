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
#ifndef __CAMERIC_ISP_EXP_DRV_H__
#define __CAMERIC_ISP_EXP_DRV_H__

/**
 * @file cameric_isp_exp_drv.h
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
 * @defgroup cameric_isp_exp_drv CamerIc ISP EXP Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_exp_drv_api.h>


/******************************************************************************
 * Is the hardware Exporsure Measurement module available ?
 *****************************************************************************/
#if defined(MRV_AUTO_EXPOSURE_VERSION) 

/******************************************************************************
 * Exporsure module is available.
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   CamerIc ISP Exposure module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspExpContext_s
{
    bool_t                          enabled;    /**< measuring enabled */
    bool_t                          autostop;   /**< stop measuring after a complete frame */
    CamerIcIspExpMeasuringMode_t    mode;       /**< measuring mode */

    CamerIcEventCb_t                EventCb;    /**< callback event (e.g. called if a new measurements available */

    CamerIcWindow_t                 Window;     /**< measuring window */
    CamerIcWindow_t                 Grid;       /**< measuring window */

    CamerIcMeanLuma_t               Luma;       /**< currently measured luma values */
} CamerIcIspExpContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP EXPOSURE driver context.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspExpInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP EXPOSURE driver context. 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspExpRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP EXPOSURE 
 *          driver context (called internally from ISP main driver module
 *          when measurement ready interrupt is raised). 
 *
 * @param   handle              CamerIc driver handle
 *
 *****************************************************************************/
void CamerIcIspExpSignal
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_AUTO_EXPOSURE_VERSION)  */

/******************************************************************************
 * Exporsure module is not available.
 *****************************************************************************/
#define CamerIcIspExpInit( hnd )    ( RET_NOTSUPP )
#define CamerIcIspExpRelease( hnd ) ( RET_NOTSUPP )
#define CamerIcIspExpSignal( hnd ) 

#endif /* #if defined(MRV_AUTO_EXPOSURE_VERSION)  */

/* @} cameric_isp_exp_drv */

#endif /* __CAMERIC_ISP_WDR_DRV_H__ */

