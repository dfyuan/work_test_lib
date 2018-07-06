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
 * @mim_ctrl.h
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


#ifndef __MIM_CTRL_H__
#define __MIM_CTRL_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>

#include <cameric_drv/cameric_drv_api.h>

#include "mim_ctrl_common.h"

/**
 * @brief   Internal states of the mim control.
 *
 */
typedef enum
{
    eMimCtrlStateInvalid    = 0x0000,   /**< FSM state is invalid since mim instance does not exist. */
    eMimCtrlStateInitialize,            /**< FSM is in state initialized. */
    eMimCtrlStateRunning,               /**< FSM is in state running. */
    eMimCtrlStateStopped,               /**< FSM is in state running. */
    eMimCtrlStateWaitForDma,            /**< FSM is in state waiting for completion of DMA transfer */
} MimCtrlState_t;


/**
 * @brief
 *
 * @note
 *
 */
typedef struct MimCtrlContext_s
{
	MimCtrlState_t          State;

    uint32_t                MaxCommands;
    uint32_t                MaxBuffers;

	osQueue                 CommandQueue;
	osThread	            Thread;

	MimCtrlCompletionCb_t   mimCbCompletion;
	void                    *pUserContext;
	
	osQueue                 *pInQueue;

    CamerIcCompletionCb_t   DmaCompletionCb;
    PicBufMetaData_t        *pDmaPicBuffer;
    RESULT                  dmaResult;

	CamerIcDrvHandle_t      hCamerIc;               /**< CamerIc Driver handle */
} MimCtrlContext_t;



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
RESULT MimCtrlCreate
(
    MimCtrlContext_t  *pMimCtrlCtx
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
RESULT MimCtrlDestroy
(
    MimCtrlContext_t *pMimCtrlCtx
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
RESULT MimCtrlSendCommand
(
    MimCtrlContext_t    *pMimCtrlCtx,
    MimCtrlCmdId_t      Command
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
MimCtrlCmdId_t MimCtrlGetPendingCommand
(
    MimCtrlContext_t    *pMimCtrlCtx
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
void MimCtrlCompleteCommand
(
    MimCtrlContext_t    *pMimCtrlCtx,
    MimCtrlCmdId_t      Command,
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
static inline MimCtrlState_t MimCtrlGetState
(
    MimCtrlContext_t *pMimCtrlCtx
)
{
    return ( pMimCtrlCtx->State );
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
static inline void MimCtrlSetState
(
    MimCtrlContext_t        *pMimCtrlCtx,
    const MimCtrlState_t    newState
)
{
    pMimCtrlCtx->State = newState;
}



/* @} module_name*/

#endif /* __MIM_CTRL_H__ */
