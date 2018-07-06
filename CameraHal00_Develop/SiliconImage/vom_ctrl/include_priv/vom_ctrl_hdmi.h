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
 * @file vom_ctrl_hdmi.h
 *
 * @brief
 *   Definition of HDMI driver API.
 *
 *****************************************************************************/
/**
 * @page vom_ctrl_page VOM Ctrl
 * The Video Output Module displays image buffers handed in via QuadHDMI_FX on
 * a connected HDMI display device.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref vom_ctrl_api
 * - @ref vom_ctrl_common
 * - @ref vom_ctrl
 * - @ref vom_ctrl_hdmi
 *
 * @defgroup vom_ctrl_hdmi VOM Ctrl HDMI
 * @{
 *
 */

#ifndef __VOM_CTRL_HDMI_H__
#define __VOM_CTRL_HDMI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cea_861.h>
#include <common/hdmi_3d.h>

/**
 * @brief Handle to vom ctrl hdmi context.
 *
 */
typedef struct vomCtrlHdmiContext_s *vomCtrlHdmiHandle_t;


/*****************************************************************************/
/**
 * @brief   Initializes HDMI transmitter to default settings
 *
 * @param   pHdmiHandle         Reference to HDMI context handle. A valid handle
 *                              will be returned upon successfull initialization,
 *                              otherwise the handle is undefined.
 * @param   pVideoFormat        Reference to CEA style video format description.
 * @param   Enable3D            Enable 3D display mode.
 * @param   VideoFormat3D       The HDMI 3D display mode to use; undefined if 3D not enabled.
 * @param   HalHandle           Handle of HAL session to use.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         HDMI successfully initialized.
 * @retval  RET_FAILURE et al.  Error initializing HDMI.
 *
 *****************************************************************************/
extern RESULT vomCtrlHdmiInit
(
    vomCtrlHdmiHandle_t                 *pHdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D,
    HalHandle_t                         HalHandle
);


/*****************************************************************************/
/**
 * @brief   Shuts down HDMI transmitter.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         HDMI successfully shutdown.
 * @retval  RET_FAILURE et al.  Error shutting down HDMI.
 *
 *****************************************************************************/
extern RESULT vomCtrlHdmiDestroy
(
    vomCtrlHdmiHandle_t HdmiHandle
);


/*****************************************************************************/
/**
 * @brief   Displays the provided image via HDMI transmitter.
 *
 * @note    The image buffer must contain @ref PicBufMetaData_t as MediaBufferMetaData.
 *          The meta data will be used for on the fly format conversion and resolution
 *          change.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 * @param   pVideoFormat        Reference to CEA style video format description.
 * @param   Enable3D            Enable 3D display mode.
 * @param   VideoFormat3D       The HDMI 3D display mode to use; undefined if 3D not enabled.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         New format, layout & frame rate set.
 * @retval  RET_FAILURE et al.  Error setting new format, layout & frame rate.
 *
 *****************************************************************************/
extern RESULT vomCtrlHdmiChangeDisplay
(
    vomCtrlHdmiHandle_t                 HdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
);


/*****************************************************************************/
/**
 * @brief   Check if HDMI transmitter has detected valid sync signals.
 *
 * @note    This function is just temporarely to help detect MVDU startup problems
 *          by an automated checker.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Valid syncs detected.
 * @retval  RET_OUTOFRANGE      No valid syncs detected.
 * @retval  RET_FAILURE et al.  Error checking for valid syncs.
 *
 *****************************************************************************/
extern RESULT vomCtrlHdmiCheckValidSync
(
    vomCtrlHdmiHandle_t HdmiHandle
);


/* @} vom_ctrl_hdmi */

#ifdef __cplusplus
}
#endif

#endif /* __VOM_CTRL_HDMI_H__ */
