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
 * @file mom_ctrl.h
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup module_name Module Name
 * @{
 *
 */
#ifndef __MOM_CTRL_H__
#define __MOM_CTRL_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>
#include <hal/hal_api.h>

#include <common/return_codes.h>
#include <common/list.h>

#include <cameric_drv/cameric_drv_api.h>

#include "mom_ctrl_common.h"

/**
 * @brief   Internal states of the mom control.
 *
 */
typedef enum MomCtrlState_e
{
    eMomCtrlStateInvalid    = 0x0000,   /**< FSM state is invalid since mom instance does not exist. */
    eMomCtrlStateInitialize = 0x0001,   /**< FSM is in state initialized. */
    eMomCtrlStateRunning    = 0x0002,   /**< FSM is in state running. */
    eMomCtrlStateStopped    = 0x0003,   /**< FSM is in state stopped. */
} MomCtrlState_t;


/**
 * @brief
 *
 * @note
 *
 */
typedef struct MomCtrlContext_s
{
    MomCtrlState_t          State;

    uint32_t                MaxCommands;
    uint32_t                NumBuffersMainPath;
    uint32_t                NumBuffersSelfPath;

    MediaBufPool_t          *pPicBufPoolMainPath;   /**< picture buffer pool */
    MediaBufPool_t          *pPicBufPoolSelfPath;   /**< picture buffer pool */

    MomCtrlCompletionCb_t   momCbCompletion;
    void                    *pUserContext;

    osQueue                 CommandQueue;

    osQueue                 EmptyBufQueue[MOM_CTRL_PATH_MAX-1U];    /**< empty buffer queue main path */
    osQueue                 FullBufQueue[MOM_CTRL_PATH_MAX-1U];     /**< full buffer queue main path */

    osMutex                 PathLock[MOM_CTRL_PATH_MAX-1U];
    List                    PathQueues[MOM_CTRL_PATH_MAX-1U];

    osThread                Thread;

    CamerIcDrvHandle_t      hCamerIc;               /**< CamerIc Driver handle */
    HalHandle_t             HalHandle;

    osMutex                 BufferLock;
    MomCtrlBuffer_t         BufferCbMainPath;
    MomCtrlBuffer_t         BufferCbSelfPath;
} MomCtrlContext_t;



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT MomCtrlCreate
(
    MomCtrlContext_t  *pMomCtrlCtx
);



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT MomCtrlDestroy
(
    MomCtrlContext_t *pMomCtrlCtx
);



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT MomCtrlSendCommand
(
    MomCtrlContext_t    *pMomCtrlCtx,
    MomCtrlCmdId_t      Command
);



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
void MomCtrlCompleteCommand
(
    MomCtrlContext_t    *pMomCtrlCtx,
    MomCtrlCmdId_t      Command,
    RESULT              result
);



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
static inline MomCtrlState_t MomCtrlGetState
(
    MomCtrlContext_t *pMomCtrlCtx
)
{
    return ( pMomCtrlCtx->State );
}



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
static inline void MomCtrlSetState
(
    MomCtrlContext_t        *pMomCtrlCtx,
    const MomCtrlState_t    newState
)
{
    pMomCtrlCtx->State = newState;
}



/* @} module_name*/

#endif /* __MOM_CTRL_H__ */

