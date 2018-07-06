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
 * @file cameric_mi_drv.h
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
 * @defgroup cameric_mi_drv CamerIc MI Driver Internal API
 * @{
 *
 */
#ifndef __CAMERIC_MI_DRV_H__
#define __CAMERIC_MI_DRV_H__

#include <ebase/types.h>

#include <common/align.h>
#include <common/list.h>
#include <common/picture_buffer.h>

#include <hal/hal_api.h>

#include <bufferpool/media_buffer.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_mi_drv_api.h>

/*******************************************************************************
 *          CamerIcMiDataPathContext_t
 *
 * @brief
 *
 * @note
 */
typedef struct CamerIcMiDataPathContext_s
{
    CamerIcMiDataMode_t             out_mode;                   /**< output format */
    CamerIcMiDataMode_t             in_mode;                    /**< input format */
    CamerIcMiDataLayout_t           datalayout;                 /**< layout of data */
    bool_t                          swap_UV;                    /**< swap color (U/V) channels */

    uint32_t                        in_width;                   /**< incomming image width (in front of scaler) */
    uint32_t                        in_height;                  /**< incomming image height (in front of scaler) */
    uint32_t                        out_width;                  /**< scaler output picture width */
    uint32_t                        out_height;                 /**< scaler output picture height */

    CamerIcMiOrientation_t          orientation;
    bool_t                          prepare_rotation;

    bool_t                          hscale;
    bool_t                          vscale;

    MediaBuffer_t                   *pShdBuffer;
    MediaBuffer_t                   *pBuffer;
} CamerIcMiDataPathContext_t;



/*******************************************************************************
 *
 *          CamerIcMiContext_t
 *
 * @brief   Internal MI driver context structure.
 *
 *****************************************************************************/
typedef struct CamerIcMiContext_s
{
    HalIrqCtx_t                     HalIrqCtx;

    CamerIcMiBurstLength_t          y_burstlength;
    CamerIcMiBurstLength_t          c_burstlength;

    CamerIcRequestCb_t              RequestCb;
    CamerIcEventCb_t                EventCb;

    CamerIcMiDataPathContext_t      PathCtx[CAMERIC_MI_PATH_MAX];

    uint32_t                        numFramesToSkip;
    osMutex 						buffer_mutex;
    uint32_t 						miFrameNum;
} CamerIcMiContext_t;


/*****************************************************************************/
/**
 *          CamerIcMiInit()
 *
 * @brief   Initialize CamerIc MI driver context
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcMiInit
(
    CamerIcDrvHandle_t  handle
);



/*****************************************************************************/
/**
 *          CamerIcMiRelease()
 *
 * @brief   Release/Free CamerIc MI driver context
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcMiRelease
(
    CamerIcDrvHandle_t  handle
);



/*****************************************************************************/
/**
 *          CamerIcMiStart()
 *
 * @brief   Initialize and start MI interrupt handling
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcMiStart
(
    CamerIcDrvHandle_t  handle
);



/*****************************************************************************/
/**
 *          CamerIcMiStop()
 *
 * @brief   Stop and release MI interrupt handling
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcMiStop
(
    CamerIcDrvHandle_t  handle
);



/*****************************************************************************/
/**
 *          CamerIcMiStartLoadPicture()
 *
 * @brief
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcMiStartLoadPicture
(
    CamerIcDrvHandle_t      handle,
    PicBufMetaData_t        *pPicBuffer,
    bool_t                  cont
);



/*****************************************************************************/
/**
 * @brief   Set/Program picture buffer addresses to CamerIc registers.
 *
 * @param   handle          CamerIc driver handle
 * @param   path            Path Index (main- or selfpath)
 * @param   pPicBuffer      Pointer to picture buffer
 *
 * @return                  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
ulong_t CamerIcMiGetBufferId
(
    CamerIcDrvHandle_t      handle,
    const CamerIcMiPath_t   path
);


/* @} cameric_mi_drv */

#endif /* __MRV_MI_DRV_H__ */
