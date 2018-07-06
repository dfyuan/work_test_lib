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
 * @file cam_engine_cb.h
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

#ifndef __CAM_ENGINE_CB_H__
#define __CAM_ENGINE_CB_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>

#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>



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
RESULT CamEngineCamerIcDrvMeasureCbRestart
(
    CamEngineContext_t  *pCamEngineCtx,
    uint32_t            numFramesToSkip
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
void CamEngineCamerIcDrvLockCb
(
    const CamerIcEventId_t  evtId,
    void                    *pParam,
    void                    *pUserContext
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
void CamEngineCamerIcDrvJpeCb
(
    const CamerIcEventId_t  evtId,
    void                    *pParam,
    void                    *pUserContext
);


/*****************************************************************************/
/**
 *             CamEngineCamerIcDrvMeasureCb
 *
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
void CamEngineCamerIcDrvMeasureCb
(
    const CamerIcEventId_t  evtId,
    void                    *pParam,
    void                    *pUserContext
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
void CamEngineCamerIcDrvCommandCb
(
    const uint32_t          cmdId,
    const RESULT            result,
    void                    *pParam,
    void                    *pUserContext
);


/* @} module_name*/

#endif /* __CAM_ENGINE_CB_H__ */

