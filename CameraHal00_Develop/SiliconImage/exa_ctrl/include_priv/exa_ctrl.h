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
 * @exa_ctrl.h
 *
 * @brief
 *   Internal stuff used by exa ctrl implementation.
 *
 *****************************************************************************/
/**
 * @page exa_ctrl_page EXA Ctrl
 * The External Algorithm Module captures image and calls external algorithm.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref exa_ctrl_api
 * - @ref exa_ctrl_common
 * - @ref exa_ctrl
 *
 * @defgroup exa_ctrl EXA Ctrl
 * @{
 *
 */


#ifndef __EXA_CTRL_H__
#define __EXA_CTRL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cea_861.h>
#include <common/picture_buffer.h>

#include <oslayer/oslayer.h>

#include <hal/hal_api.h>

#include "exa_ctrl_common.h"

/**
 * @brief   Internal states of the exa control.
 *
 */
typedef enum
{
    eExaCtrlStateInvalid      = 0,  //!< FSM state is invalid since EXA instance does not exist, is currently being created or is currently being shutdown.
    eExaCtrlStateIdle,              //!< FSM is in state Idle.
    eExaCtrlStatePaused,            //!< FSM is in state Paused.
    eExaCtrlStateRunning            //!< FSM is in state Running.
} exaCtrlState_t;

/**
 * @brief   Buffer type & layout dependent buffer handling.
 *
 * @note
 *
 */
typedef RESULT (* exaCtrlMapBuffer_t)
(
    struct exaCtrlContext_s    *pExaContext,
    PicBufMetaData_t           *pPicBufMetaData
);

typedef RESULT (* exaCtrlUnMapBuffer_t)
(
    struct exaCtrlContext_s    *pExaContext
);

/**
 * @brief   Context of exa control instance. Holds all information required for operation.
 *
 * @note
 *
 */
typedef struct exaCtrlContext_s
{
    exaCtrlState_t             State;              //!< Holds internal state.

    uint32_t                   MaxCommands;        //!< Max number of commands that can be queued.
    uint32_t                   MaxBuffers;
    exaCtrlCompletionCb_t      exaCbCompletion;    //!< Buffer completion callback.
    void                       *pUserContext;      //!< User context to pass in to buffer completion callback.
    HalHandle_t                HalHandle;          //!< HAL session to use for HW access.

    osQueue                    CommandQueue;       //!< Command queue; holds elements of type @ref exaCtrlCmd_t.
    osThread                   Thread;             //!< Command processing thread.

    osQueue                    FullBufQueue;

    exaCtrlSampleCb_t          exaCbSample;        //!< External algorithm callback.
    void                       *pSampleContext;    //!< Sample context passed on to sample callback.
    uint8_t                    SampleSkip;         //!< Skip consecutive samples.

    uint8_t                    SampleIdx;          //!< Sample index.

    PicBufMetaData_t           MappedMetaData;     //!< Media buffer meta data descriptor for mapped buffer.
    exaCtrlMapBuffer_t         MapBuffer;          //!< Suitable handling function for type & layout of buffer.
    exaCtrlUnMapBuffer_t       UnMapBuffer;        //!< Suitable handling function for type & layout of buffer.
} exaCtrlContext_t;



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
extern RESULT exaCtrlCreate
(
    exaCtrlContext_t    *pExaContext
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
extern RESULT exaCtrlDestroy
(
    exaCtrlContext_t    *pExaContext
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
extern RESULT exaCtrlSendCommand
(
    exaCtrlContext_t    *pExaContext,
    exaCtrlCmd_t        *pCommand
);


/* @} exa_ctrl */

#ifdef __cplusplus
}
#endif

#endif /* __EXA_CTRL_H__ */
