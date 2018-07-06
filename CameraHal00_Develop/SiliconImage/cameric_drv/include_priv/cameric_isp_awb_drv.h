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
#ifndef __CAMERIC_ISP_AWB_DRV_H__
#define __CAMERIC_ISP_AWB_DRV_H__

/**
 * @file cameric_isp_awb_drv.h
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
 * @defgroup cameric_isp_awb_drv CamerIc ISP AWB Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_awb_drv_api.h>


/******************************************************************************
 * Is the hardware WB measuring module available ?
 *****************************************************************************/
#if defined(MRV_AWB_VERSION) 

/******************************************************************************
 * WB measuring module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP WB module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspAwbContext_s
{
    bool_t                          enabled;        /**< measuring enabled */
    bool_t                          autostop;       /**< stop measuring after a complete frame */
    CamerIcIspAwbMeasuringMode_t    mode;           /**< measuring mode */

    CamerIcEventCb_t                EventCb;

    CamerIcWindow_t                 Window;         /**< measuring window */
    CamerIcAwbMeasuringResult_t     MeasResult;     /**< currently measured values */
} CamerIcIspAwbContext_t;


/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP Auto white balance driver
 *          context 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspAwbInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP Auto white balance driver
 *          context 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspAwbRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP WB module
 *          driver (called internally from ISP main driver module when
 *          measurment ready interrupt is raised). 
 *
 * @param   handle              CamerIc driver handle
 *
 *****************************************************************************/
void CamerIcIspAwbSignal
(
    CamerIcDrvHandle_t handle
);


#else  /* #if defined(MRV_AWB_VERSION) */

/******************************************************************************
 * WB module is not available.
 *****************************************************************************/

#define CamerIcIspAwbInit( hnd )        ( RET_NOTSUPP )
#define CamerIcIspAwbRelease( hnd )     ( RET_NOTSUPP )
#define CamerIcIspAwbSignal( hnd )

#endif /* #if defined(MRV_AWB_VERSION) */


/* @} cameric_isp_awb_drv */

#endif /* __CAMERIC_ISP_AWB_DRV_H__ */

