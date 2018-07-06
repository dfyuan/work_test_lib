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
 * @file vom_ctrl_mvdu.h
 *
 * @brief
 *   Definition of MVDU driver API.
 *
 *****************************************************************************/
/**
 * @page vom_ctrl_page VOM Ctrl
 * The Video Output Module displays image buffers handed in via QuadMVDU_FX on
 * a connected HDMI display device.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref vom_ctrl_api
 * - @ref vom_ctrl_common
 * - @ref vom_ctrl
 * - @ref vom_ctrl_mvdu
 *
 * @defgroup vom_ctrl_mvdu VOM Ctrl MVDU
 * @{
 *
 */

#ifndef __VOM_CTRL_MVDU_H__
#define __VOM_CTRL_MVDU_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/picture_buffer.h>
#include <common/cea_861.h>
#include <common/hdmi_3d.h>

#include <bufferpool/media_buffer.h>

#include <hal/hal_api.h>

/**
 * @brief Handle to vom ctrl mvdu context.
 *
 */
typedef struct vomCtrlMvduContext_s *vomCtrlMvduHandle_t;


/**
 *  @brief Buffer displayed signalling callback
 *
 *  Callback for signalling that a buffer was displayed and can be released by callee.
 *
 */
typedef void (* vomCtrlMvduBufferReleaseCb)
(
    void            *pUserContext,  //!< Opaque user data pointer that was passed in on creation.
    MediaBuffer_t   *pBuffer,       //!< Pointer to buffer that is to be released.
    RESULT          DelayedResult   //!< Result of delayed buffer processing.
);

/*****************************************************************************/
/**
 * @brief   Initializes MVDU and HDMI transmitter.
 *
 * @param   pMvduHandle         Reference to MVDU context handle. A valid handle
 *                              will be returned upon successfull initialization,
 *                              otherwise the handle is undefined.
 * @param   pVideoFormat        Reference to CEA style video format description.
 * @param   Enable3D            Enable 3D display mode.
 * @param   VideoFormat3D       The HDMI 3D display mode to use; undefined if 3D not enabled.
 * @param   BufReleaseCB        Callback funktion to release a displayed buffer.
 * @param   pUserContext        Opaque user context passed on to @ref BufReleaseCB.
 * @param   HalHandle           Handle of HAL session to use.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MVDU & HDMI successfully initialized.
 * @retval  RET_FAILURE et al.  Error initializing MVDU and/or HDMI.
 *
 *****************************************************************************/
extern RESULT vomCtrlMvduInit
(
    vomCtrlMvduHandle_t                 *pMvduHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D,
    vomCtrlMvduBufferReleaseCb          BufReleaseCB,
    void                                *pUserContext,
    HalHandle_t                         HalHandle
);


/*****************************************************************************/
/**
 * @brief   Shuts down MVDU and HDMI transmitter.
 *
 * @param   MvduHandle          MVDU context handle as returned by @ref vomCtrlMvduInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         MVDU & HDMI successfully shutdown.
 * @retval  RET_FAILURE et al.  Error shutting down MVDU and/or HDMI.
 *
 *****************************************************************************/
extern RESULT vomCtrlMvduDestroy
(
    vomCtrlMvduHandle_t MvduHandle
);


/*****************************************************************************/
/**
 * @brief   Displays the provided image via MVDU and HDMI transmitter.
 *
 * @note    The image buffer must contain @ref PicBufMetaData_t as MediaBufferMetaData.
 *          The meta data will be used for on the fly format conversion and resolution
 *          change.
 *
 * @param   MvduHandle          MVDU context handle as returned by @ref vomCtrlMvduInit.
 * @param   pBuffer             Reference to MediaBuffer containing the new image to display.
 *
 * @return  Return the result of the function call.
 * @retval  RET_PENDING         Image buffer successfully passed on to MVDU. The
 *                              buffer will be returned after it was displayed
 *                              using the registered callback.
 * @retval  RET_FAILURE et al.  Error passing image buffer to MVDU.
 *
 *****************************************************************************/
extern RESULT vomCtrlMvduDisplay
(
    vomCtrlMvduHandle_t MvduHandle,
    MediaBuffer_t       *pBuffer
);


/* @} vom_ctrl_mvdu */

#ifdef __cplusplus
}
#endif

#endif /* __VOM_CTRL_MVDU_H__ */
