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
#ifndef __BUFSYNC_CTRL_H__
#define __BUFSYNC_CTRL_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>

#include <common/return_codes.h>
#include <common/list.h>

#include "bufsync_ctrl_common.h"



/**
 * @brief   prototype declaration of the bufsync-control context
 */
typedef struct BufSyncCtrlContext_s BufSyncCtrlContext_t; 



/**
 *          BufSyncCmd_t
 *
 * @brief   generic buffersync command type
 *
 */
typedef struct BufSyncCmd_s
{
    BufSyncCtrlCmdId_t  CmdId;
    void                *pCmdCtx;
} BufSyncCmd_t;



/**
 * @brief   Internal states of the bufsync-control.
 *
 */
typedef enum BufSyncCtrlState_e
{
    eBufSyncCtrlStateInvalid    = 0x0000,   /**< FSM state is invalid since bufsync instance does not exist. */
    eBufSyncCtrlStateInitialize = 0x0001,   /**< FSM is in state initialized. */
    eBufSyncCtrlStateRunning    = 0x0002,   /**< FSM is in state running. */
    eBufSyncCtrlStateStopped    = 0x0003,   /**< FSM is in state stopped. */
} BufSyncCtrlState_t;



/**
 * @brief   Internal context of the bufsync-queue.
 *
 */
#define BUFSYNC_MAX_QUEUES          2
typedef struct BufSysncQueueContext_s
{
    ulong_t                     id;
    osQueue                     *pBufQueue;         /**< buffer queue */
    osThread                    Thread;             /**< queue - thread */
    
    MediaBuffer_t               *pBuffer;           /**< current buffer */
    osMutex                     BufferLock;

    BufSyncCtrlContext_t        *pOwner;
    bool_t                      bExit;
} BufSysncQueueContext_t;



/**
 * @brief   Internal context of the bufsync-control.
 */
struct BufSyncCtrlContext_s
{
    BufSyncCtrlState_t          State;

    uint32_t                    MaxCommands;
    osQueue                     CommandQueue;
    osThread                    Thread;

    BufSyncCtrlCompletionCb_t   bufsyncCbCompletion;    /**< Callback function for command completion. */
    void                        *pUserContext;          /**< User context passed on to completion callback. */

    BufSysncQueueContext_t      QueueCtx[BUFSYNC_MAX_QUEUES];

    osMutex                     BufferLock;
    BufSyncCtrlBuffer_t         BufferCb;
};



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
RESULT BufSyncCtrlCreate
(
    BufSyncCtrlContext_t *pBufSyncCtrlCtx
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
RESULT BufSyncCtrlDestroy
(
    BufSyncCtrlContext_t *pBufSyncCtrlCtx
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
RESULT BufSyncCtrlSendCommand
(
    BufSyncCtrlContext_t    *pBufSyncCtrlCtx,
    BufSyncCmd_t            *pCommand
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
void BufSyncCtrlCompleteCommand
(
    BufSyncCtrlContext_t    *pBufSyncCtrlCtx,
    BufSyncCtrlCmdId_t      Command,
    RESULT                  result 
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
static inline BufSyncCtrlState_t BufSyncCtrlGetState
(
    BufSyncCtrlContext_t    *pBufSyncCtrlCtx
)
{
    return ( pBufSyncCtrlCtx->State );
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
static inline void BufSyncCtrlSetState
(
    BufSyncCtrlContext_t        *pBufSyncCtrlCtx,
    const BufSyncCtrlState_t    newState
)
{
    pBufSyncCtrlCtx->State = newState;
}


/* @} module_name*/

#endif /* __BUFSYNC_CTRL_H__ */

