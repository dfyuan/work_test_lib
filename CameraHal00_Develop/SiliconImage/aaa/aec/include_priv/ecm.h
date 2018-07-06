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
#ifndef __EXPOSURE_CONVERSION_H__
#define __EXPOSURE_CONVERSION_H__

/**
 * @file ecm.h
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
 * @defgroup ECM Exposure Conversion Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include "aec_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 * predefined flicker period values for ECM module
 */
/*****************************************************************************/
#define ECM_TFLICKER_OFF   ((ECM_TFLICKER_100HZ+ECM_TFLICKER_120HZ)/2)  //!< predefined flicker period value for ECM module
#define ECM_TFLICKER_100HZ (1.0/100.0)                                  //!< predefined flicker period value for ECM module
#define ECM_TFLICKER_120HZ (1.0/120.0)                                  //!< predefined flicker period value for ECM module
#define ECM_DOT_NO         (6)



/*****************************************************************************/
/**
 *          EcmExecute
 *
 * @brief   Performs EC algorithm
 *
 * @param   pAecCtx                 Reference of AEC context.
 * @param   Exposure                Current exposure value.
 * @param   pSplitGain              Reference to storage for calculated gain.
 * @param   pSplitIntegrationTime   Reference to storage for calculated integration time
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT EcmExecute
(
    AecContext_t        *pAecCtx,
    float               Exposure,
    float               *pSplitGain,
    float               *pSplitIntegrationTime
);
RESULT EcmExecuteDirect
(
    AecContext_t        *pAecCtx,
    float               Exposure,
    float               *pSplitGain,
    float               *pSplitIntegrationTime
);

/*****************************************************************************/
/**
 *          AfpsExecute
 *
 * @brief   Performs AFPS algorithm.
 *
 * @param   pAecCtx         Reference of AEC context
 * @param   IntegrationTime Current integration time
 * @param   pNewResolution  Reference to storage for chosen resolution;
 *                          set to new resolution if a change is required,
 *                          set to 0 (zero) otherwise
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 *
 *****************************************************************************/
RESULT AfpsExecute
(
    AecContext_t        *pAecCtx,
    float               IntegrationTime,
    uint32_t            *pNewResolution
);



#ifdef __cplusplus
}
#endif


/* @} ECM */


#endif /* __EXPOSURE_CONVERSION_H__ */
