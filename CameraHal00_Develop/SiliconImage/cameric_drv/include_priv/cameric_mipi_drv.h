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
 * @file cameric_mipi_drv.h
 *
 * @brief
 *  Internal interface of MIPI module.
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 *
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup cameric_mipi_drv CamerIc MIPI Driver Internal API
 * @{
 *
 */

#ifndef __CAMERIC_MIPI_DRV_H__
#define __CAMERIC_MIPI_DRV_H__

#include <ebase/types.h>

#include <common/align.h>
#include <common/list.h>
#include <common/mipi.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_mipi_drv_api.h>


/******************************************************************************
 * Is the hardware MIPI module available ?
 *****************************************************************************/
#if defined(MRV_MIPI_VERSION)

/******************************************************************************
 * MIPI Module is available.
 *****************************************************************************/


/******************************************************************************/
/**
 * @brief driver internal context of MIPI Module.
 *
 *****************************************************************************/
typedef struct CamerIcMipiContext_s
{
    int32_t                     idx;                    /**< instance index */
    HalIrqCtx_t                 HalIrqCtx;              /**< interrupt context */

    uint32_t                    no_lanes;               /**< number of used lanes */
    MipiVirtualChannel_t        virtual_channel;        /**< current virtual channel id */
    MipiDataType_t              data_type;              /**< curreent data type  */

    bool_t                      compression_enabled;    /**< compression enabled */
    MipiDataCompressionScheme_t compression_scheme;     /**< currently used compression sheme */
    MipiPredictorBlock_t        predictor_block;        /**< currently used predictor block */

    CamerIcEventCb_t            EventCb;                /**< event callback */

    bool_t                      enabled;                /**< mipi module currently enabled */
} CamerIcMipiContext_t;



/*****************************************************************************/
/**
 * @brief   This function enables the clock for the CamerIC MIPI module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         Context successfully generated/initialized
 * @retval  RET_WRONG_HANDLE    driver handle is invalid
 * @retval  RET_OUTOFMEM        out of memory (memory allocation failed)
 *
 *****************************************************************************/
RESULT CamerIcMipiEnableClock
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);



/*****************************************************************************/
/**
 * @brief   This function disables the clock for the CamerIC MIPI module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcMipiDisableClock
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);


/*****************************************************************************/
/**
 * @brief   This function initializes the  CamerIc MIPI driver context.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcMipiInit
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);


/*****************************************************************************/
/**
 * @brief   This function releases/frees the  CamerIc MIPI driver context.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcMipiRelease
(
    CamerIcDrvHandle_t handle,
    const int32_t      idx
);


/*****************************************************************************/
/**
 * @brief   This function starts interrupt handling the  CamerIc MIPI driver
 *          module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcMipiStart
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);


/*****************************************************************************/
/**
 * @brief   This function stops interrupt handling the  CamerIc MIPI driver
 *          module.
 *
 * @param   handle              CamerIc driver handle.
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
RESULT CamerIcMipiStop
(
    CamerIcDrvHandle_t  handle,
    const int32_t       idx
);


#else  /* #if defined(MRV_MIPI_VERSION) */

/******************************************************************************
 * MIPI module is not available.
 *****************************************************************************/

#define CamerIcMipiEnableClock( hnd, idx )      ( RET_NOTSUPP )
#define CamerIcMipiDisableClock( hnd, idx )     ( RET_NOTSUPP )
#define CamerIcMipiInit( hnd, idx )             ( RET_NOTSUPP )
#define CamerIcMipiRelease( hnd, idx )          ( RET_NOTSUPP )
#define CamerIcMipiStart( hnd, idx )            ( RET_NOTSUPP )
#define CamerIcMipiStop( hnd, idx )             ( RET_NOTSUPP )

#endif /* #if defined(MRV_MIPI_VERSION) */


/* @} cameric_mipi_drv */

#endif /* __CAMERIC_MIPI_DRV_H__ */

