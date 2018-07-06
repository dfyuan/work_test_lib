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
#ifndef __ADCC_CTRL_H__
#define __ADCC_CTRL_H__

/**
 * @file adpcc_ctrl.h
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
 * @defgroup ADPCC Auto defect pixel cluster correction
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include "adpcc.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 * @brief   This enum type specifies the different possible states of
 *          the ADPCC module.
 */
/*****************************************************************************/
typedef enum AdpccState_e
{
    ADPCC_STATE_INVALID           = 0,                  /**< initialization value */
    ADPCC_STATE_INITIALIZED       = 1,                  /**< instance is created, but not initialized */
    ADPCC_STATE_STOPPED           = 2,                  /**< instance is confiured (ready to start) or stopped */
    ADPCC_STATE_RUNNING           = 3,                  /**< instance is running (processes frames) */
    ADPCC_STATE_LOCKED            = 4,                  /**< instance is locked (for taking snapshots) */
    ADPCC_STATE_MAX                                     /**< max */
} AdpccState_t;



/*****************************************************************************/
/**
 * @brief   Context of the ADPCC module.
 */
/*****************************************************************************/
typedef struct AdpccContext_s
{
    AdpccState_t                    state;              /**< current state of ADPCC instance */

    CamerIcDrvHandle_t              hCamerIc;           /**< cameric driver handle */
    CamerIcDrvHandle_t              hSubCamerIc;        /**< cameric driver handle */

    CamResolutionName_t             ResName;            /**< identifier for accessing resolution depended calibration data */
    CamCalibDbHandle_t              hCamCalibDb;        /**< calibration database handle */

    float                           gain;               /**< current sensor gain */
    AdpccConfig_t                   Config;
} AdpccContext_t;


#ifdef __cplusplus
}
#endif

/* @} ADPCC */


#endif /* __ADPCC_CTRL_H__*/
