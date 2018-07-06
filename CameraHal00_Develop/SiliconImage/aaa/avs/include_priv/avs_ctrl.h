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
#ifndef __AVS_CTRL_H__
#define __AVS_CTRL_H__

/**
 * @file avs_ctrl.h
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
 * @defgroup AVS Auto Video Stabilizaton Control
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include "avs.h"


#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************************************/
/**
 * @brief   States of AVS module.
 *
 ******************************************************************************/
typedef enum AvsState_e
{
    AVS_STATE_INVALID        = 0,
    AVS_STATE_INITIALIZED    = 1,
    AVS_STATE_STOPPED        = 2,
    AVS_STATE_RUNNING        = 3,
    AVS_STATE_MAX
} AvsState_t;



/******************************************************************************/
/**
 * @brief   AVS context structure.
 *
 ******************************************************************************/
typedef struct AvsOffsetData_s
{
    CamEngineVector_t   offsVec; /**< horizontal/vertical offset of
                                      the cropping window from the center
                                      position for the frame identified by
                                      frameId */
    ulong_t             frameId; /**< id of the frame which the offset
                                      vector relates to */
    bool_t              used;   /**< whether this structure is currently used */
} AvsOffsetData_t;



/******************************************************************************/
/**
 * @brief   AVS context structure.
 *
 ******************************************************************************/
typedef struct AvsContext_s
{
    AvsState_t          state;      /**< state of module */

    CamerIcDrvHandle_t  hCamerIc;    /**< cameric handle */
    CamerIcDrvHandle_t  hSubCamerIc; /**< sub cameric handle */
    AvsConfig_t         config;     /**< current configuration of module */
    CamEngineVector_t   displVec;   /**< last reported displacement vector */
    CamEngineVector_t   offsVec;    /**< current horizontal/vertical offset of
                                         the cropping window from the center
                                         position */
    AvsOffsetData_t    *pOffsetData; /**< pointer to array of offset data */
    CamEngineWindow_t   cropWin;    /**< current cropping window */
    CamerIcWindow_t     cropWinIs;  /**< cropping window structure needed to
                                         call IS. */

    uint16_t maxAbsOffsetHor;       /**< maximum absolute  horizontal offset
                                         of the cropping window from the center
                                         position */
    uint16_t maxAbsOffsetVer;       /**< maximum absolute  horizontal offset
                                         of the cropping window from the center
                                         position */

    uint16_t           dampLutHorSize; /**< Number of elements pointed to by
                                            pDampLutHor. */
    uint16_t           dampLutVerSize; /**< Number of elements pointed to by
                                            pDampLutVer. */

    AvsDampFuncParams_t dampHorParams; /**< Potentially used parameters for
                                            horizontal damping function. */
    AvsDampFuncParams_t dampVerParams; /**< Potentially used parameters for
                                            vertical damping function. */
    AvsDampingPoint_t *pDampLutHor; /**< Lookup table for damping function in
                                         horizontal direction. */
    AvsDampingPoint_t *pDampLutVer; /**< Lookup table for damping function in
                                         vertical direction. */
} AvsContext_t;


#ifdef __cplusplus
}
#endif

/* @} AVS */


#endif /* __AVS_CTRL_H__*/
