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
 * @mom_ctrl.h
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
#ifndef __MOM_CTRL_CB_H__
#define __MOM_CTRL_CB_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>

#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>



/*****************************************************************************/
/**
 * 			MimCtrlDmaCompletionCb
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
void MimCtrlDmaCompletionCb
(
	const CamerIcCommandId_t    cmdId,
	const RESULT                result,
	void                        *pParam,
	void                        *pUserContext
);



/* @} module_name*/

#endif /* __MOM_CTRL_CB_H__ */

