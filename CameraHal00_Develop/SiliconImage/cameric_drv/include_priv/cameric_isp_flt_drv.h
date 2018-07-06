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
/**
 * @file cameric_isp_flt_drv.h
 *
 * @brief
 *  Internal interface of FLT Module.
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup cameric_isp_flt_drv CamerIc ISP FLT Driver Internal API
 * @{
 *
 */

#ifndef __CAMERIC_ISP_FLT_DRV_H__
#define __CAMERIC_ISP_FLT_DRV_H__

#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_flt_drv_api.h>


/******************************************************************************
 * Is the hardware ISP Filter module available ?
 *****************************************************************************/
#if defined(MRV_FILTER_VERSION)

/******************************************************************************
 * Auto Focus Module is available.
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   CamerIc ISP Filter module internal driver context.
 *
 *****************************************************************************/
typedef struct CamerIcIspFltContext_s
{
    bool_t                          enabled;

    CamerIcIspFltDeNoiseLevel_t     DeNoiseLevel;
    CamerIcIspFltSharpeningLevel_t  SharpeningLevel;
} CamerIcIspFltContext_t;



/*****************************************************************************/
/**
 * @brief   Initialize CamerIc ISP FLT driver context.
 *
 * @param   handle          CamerIc driver handle.
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspFltInit
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   Release/Free CamerIc ISP FLT driver context.
 *
 * @param   handle          CamerIc driver handle.
 *
 * @return                  Return the result of the function call.
 * @retval                  RET_SUCCESS
 * @retval                  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspFltRelease
(
    CamerIcDrvHandle_t handle
);


#else  /* #if defined(MRV_FILTER_VERSION) */

/******************************************************************************
 * ISP Filter module is not available.
 *****************************************************************************/

#define CamerIcIspFltInit( hnd )                ( RET_NOTSUPP )
#define CamerIcIspFltRelease( hnd )             ( RET_NOTSUPP )

#endif /* #if defined(MRV_FILTER_VERSION) */


/* @} cameric_isp_flt_drv */

#endif /* __CAMERIC_ISP_FLT_DRV_H__ */
