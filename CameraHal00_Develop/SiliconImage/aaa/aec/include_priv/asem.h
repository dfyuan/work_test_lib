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
#ifndef __ADAPTIVE_SCENE_EVALUATION_H__
#define __ADAPTIVE_SCENE_EVALUATION_H__

/**
 * @file asem.h
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
 * @defgroup ASEM Adaptive Scene Evaluation Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_exp_drv_api.h>

#include "aec_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 *          AdaptSemExecute
 *
 * @brief
 *
 * @param
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT AdaptSemExecute
(
    AecContext_t        *pAecCtx,
    CamerIcMeanLuma_t   luma
);



#ifdef __cplusplus
}
#endif


/* @} SEM */


#endif /* __ADAPTIVE_SCENE_EVALUATION_H__ */
