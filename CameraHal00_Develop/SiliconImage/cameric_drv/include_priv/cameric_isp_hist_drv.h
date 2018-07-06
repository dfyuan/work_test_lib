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
#ifndef __CAMERIC_ISP_HIST_DRV_H__
#define __CAMERIC_ISP_HIST_DRV_H__

/**
 * @file cameric_isp_hist_drv.h
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
 * @defgroup cameric_isp_hist_drv CamerIc ISP HIST Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>


/******************************************************************************
 * Is the hardware Histogram measuring module available ?
 *****************************************************************************/
#if defined(MRV_HISTOGRAM_VERSION)

/******************************************************************************
 * Histogram Measuring Module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP Histogram module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspHistContext_s
{
    bool_t                  enabled;                            /**< measuring enabled */
    CamerIcIspHistMode_t    mode;                               /**< histogram mode */
    uint16_t                StepSize;                           /**< stepsize calculated from measuirng window */

    CamerIcEventCb_t        EventCb;

    CamerIcWindow_t         Window;                             /**< measuring window */
    CamerIcWindow_t         Grid;                               /**< measuring window */
    CamerIcHistWeights_t    Weights;
    CamerIcHistBins_t       Bins;                               /**< measured values */
} CamerIcIspHistContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP HIST driver module context.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspHistInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP HIST driver context. 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspHistRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP hist module 
 *          driver (It 's called internally from ISP main driver module when
 *          measurment ready or error interrupt is raised). 
 *
 * @param   handle              CamerIc driver handle
 *
 *****************************************************************************/
void CamerIcIspHistSignal
(
    CamerIcDrvHandle_t handle
);

#else  /* #if defined(MRV_HISTOGRAM_VERSION) */

/******************************************************************************
 * Histogram Measuring Module is not available.
 *****************************************************************************/

#define CamerIcIspHistInit( hnd )                ( RET_NOTSUPP )
#define CamerIcIspHistRelease( hnd )             ( RET_NOTSUPP )
#define CamerIcIspHistSignal( hnd )

#endif /* #if defined(MRV_HISTOGRAM_VERSION) */


/* @} cameric_isp_hist_drv */

#endif /* __CAMERIC_ISP_HIST_DRV_H__ */

