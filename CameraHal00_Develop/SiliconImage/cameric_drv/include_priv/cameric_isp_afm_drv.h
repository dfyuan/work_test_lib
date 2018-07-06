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
#ifndef __CAMERIC_ISP_AFM_DRV_H__
#define __CAMERIC_ISP_AFM_DRV_H__

/**
 * @file cameric_isp_afm_drv.h
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
 * @defgroup cameric_isp_afm_drv CamerIc ISP AFM Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_afm_drv_api.h>


/******************************************************************************
 * Is the hardware AF measuring module available ?
 *****************************************************************************/
#if defined(MRV_AUTOFOCUS_VERSION)

/******************************************************************************
 * Auto Focus Module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP AF signals delivered from ISP main driver module
 *          into AF module.
 *
 *****************************************************************************/
typedef enum CamerIcIspAfmSignal_e
{
    CAMERIC_ISP_AFM_SIGNAL_INVALID          = 0,
    CAMERIC_ISP_AFM_SIGNAL_MEASURMENT       = 1,
    CAMERIC_ISP_AFM_SIGNAL_LUMA_OVERFLOW    = 2,
    CAMERIC_ISP_AFM_SIGNAL_SUM_OVERFLOW     = 3
} CamerIcIspAfmSignal_t;


/*****************************************************************************/
/**
 * @brief   CamerIc ISP AF module internal driver context.
 *
 *****************************************************************************/
 typedef struct CamerIcIspAfmContext_s
{
    bool_t                          enabled;        /**< measuring enabled */
    CamerIcEventCb_t                EventCb;

	CamerIcWindow_t                 WindowA;		/**< measuring window A */
	uint32_t                        PixelCntA;
	bool_t                          EnabledWdwA;
	CamerIcWindow_t                 WindowB;        /**< measuring window B */
	uint32_t                        PixelCntB;
	bool_t                          EnabledWdwB;
	CamerIcWindow_t                 WindowC;        /**< measuring window C */
	uint32_t                        PixelCntC;
	bool_t                          EnabledWdwC;

	uint32_t                        Threshold;
	uint32_t                        lum_shift;
	uint32_t                        afm_shift;
	uint32_t                        MaxPixelCnt;

	CamerIcAfmMeasuringResult_t     MeasResult;
} CamerIcIspAfmContext_t;



/*****************************************************************************/
/**
 * @brief   This function initializes CamerIc ISP Auto focus module driver
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
RESULT CamerIcIspAfmInit
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   This function releases/frees CamerIc ISP Auto focus module driver
 *          context 
 *
 * @param   handle              CamerIc driver handle
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully deleted / Memory freed
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcIspAfmRelease
(
    CamerIcDrvHandle_t handle
);


/*****************************************************************************/
/**
 * @brief   This function transfers an event to the CamerIc ISP AF module 
 *          driver (It 's called internally from ISP main driver module when
 *          measurment ready or error interrupt is raised). 
 *
 * @param   handle              CamerIc driver handle
 * @param   signal              Signal to deliver
 *
 *****************************************************************************/
void CamerIcIspAfmSignal
(
    const CamerIcIspAfmSignal_t signal,
    CamerIcDrvHandle_t          handle
);


#else  /* #if defined(MRV_AUTOFOCUS_VERSION) */

/******************************************************************************
 * AF module is not available.
 *****************************************************************************/

#define CamerIcIspAfmInit( hnd )                ( RET_NOTSUPP )
#define CamerIcIspAfmRelease( hnd )             ( RET_NOTSUPP )
#define CamerIcIspAfmSignal( signal, hnd )

#endif /* #if defined(MRV_AUTOFOCUS_VERSION) */


/* @} cameric_isp_afm_drv */

#endif /* __CAMERIC_ISP_AFM_DRV_H__ */

