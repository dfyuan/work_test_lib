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
#ifndef __WB_GAIN_H__
#define __WB_GAIN_H__

/**
 * @file wbgain.h
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
 * @defgroup WBGAIN White Balance Gain Calculation Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include "wbgain_common.h"
#include "awb_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 *          AwbWbGainOutOfRangeCheck()
 *
 * @brief   
 *
 * @param   handle              awb-context
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFRANGE
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
RESULT AwbWbGainOutOfRangeCheck
( 
    AwbContext_t *pAwbCtx
);



/*****************************************************************************/
/**
 *          AwbWbGainClip()
 *
 * @brief   
 *
 * @param   handle              awb-context
 *          pWbGainsOverG       to green gains normalized red/blue-gains
 *          pWbGainsOutOfRange  result of out-of-range-check
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFRANGE
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
RESULT AwbWbGainClip
( 
    AwbContext_t *pAwbCtx
);



/*****************************************************************************/
/**
 *          AwbWbGainProcessFrame()
 *
 * @brief   
 *
 * @param   handle              awb-context
 *          pWbGainsOverG       to green gains normalized red/blue-gains
 *          pWbGainsOutOfRange  result of out-of-range-check
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFRANGE
 * @retval  RET_WRONG_HANDLE
 *
 *****************************************************************************/
RESULT AwbWbGainProcessFrame
(
    AwbContext_t *pAwbCtx
);



#ifdef __cplusplus
}
#endif

/* @} WBGAIN */


#endif /* __WB_GAIN_H__ */
