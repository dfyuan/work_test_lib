/******************************************************************************
 *
 * Copyright 2012, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
#ifndef __CAMERIC_ISP_VSM_DRV_H__
#define __CAMERIC_ISP_VSM_DRV_H__

/**
 * @file cameric_isp_vs_drv.h
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
 * @defgroup cameric_isp_vs_drv CamerIc ISP VSM Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include "cameric_isp_vsm_drv_api.h"


/******************************************************************************
 * Is the hardware video stabilization measurement module available ?
 *****************************************************************************/
#if defined(MRV_VSM_VERSION)

/******************************************************************************
 * Video stabilization measurement module is available.
 *****************************************************************************/


/*****************************************************************************/
/**
 * @brief   CamerIc ISP video stabiliation measurement module
 *          internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspVsmContext_s
{
    bool_t                          enabled;    /**< measuring enabled */

    CamerIcEventCb_t                EventCb;    /**< callback event (e.g. called if a new displacement vector is available) */

    CamerIcWindow_t                 MeasureWin; /**< measuring window */

    uint8_t                         horSegments; /**< Number of 16 point wide segments enclosed by
                                                      the first iteration sample points in horizontal direction.
                                                      Range: 1 ... 128. See also @ref CamerIcIspVsmSetMeasuringWindow. */
    uint8_t                         verSegments; /**< Number of 16 point wide segments enclosed by
                                                      the first iteration sample points in vertical direction.
                                                      Range: 1 ... 128. See also @ref CamerIcIspVsmSetMeasuringWindow.  */

    CamerIcIspVsmEventData_t        eventData;   /**< data to pass in the event callback */
} CamerIcIspVsmContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP VSM driver context.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcIspVsmInit
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP VSM driver context.
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspVsmRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP VSM
 *          driver context (called internally from ISP main driver module
 *          when measurement done interrupt is raised).
 *
 * @param   handle       CamerIc driver handle
 * @param   currFrameId  Id if the current frame to pass along with any
 *                       measured results
 *
 *****************************************************************************/
void CamerIcIspVsmSignal
(
    CamerIcDrvHandle_t handle,
    ulong_t            currFrameId
);

#else  /* #if defined(MRV_VSM_VERSION)  */

/******************************************************************************
 * Video stabilization measurement module is not available.
 *****************************************************************************/
#define CamerIcIspVsmInit( hnd )    ( RET_NOTSUPP )
#define CamerIcIspVsmRelease( hnd ) ( RET_NOTSUPP )
#define CamerIcIspVsmSignal( hnd )

#endif /* #if defined(MRV_VSM_VERSION)  */

/* @} cameric_isp_vs_drv */

#endif /* __CAMERIC_ISP_VSM_DRV_H__ */

