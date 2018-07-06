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
#ifndef __EXPPRIOR_H__
#define __EXPPRIOR_H__

/**
 * @file expprior.h
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
 * @defgroup EXPPRIOR
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include "expprior_common.h"
#include "awb_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/**
 *  @brief  Configuration structure of the Exposure and Prior Probability Module
 */
/*****************************************************************************/
typedef struct AwbExpPriorConfig_s
{
    float       IIRDampCoefAdd;             /**< incrementer of damping coefficient */
    float       IIRDampCoefSub;             /**< decrementer of damping coefficient */
    float       IIRDampFilterThreshold;     /**< threshold for incrementing or decrementing of damping coefficient */

    float       IIRDampingCoefMin;          /**< minmuim value of damping coefficient */
    float       IIRDampingCoefMax;          /**< maximum value of damping coefficient */
    float       IIRDampingCoefInit;         /**< initial value of damping coefficient */

    uint16_t    IIRFilterSize;              /**< number of filter items */
    float       IIRFilterInitValue;         /**< initial value of the filter items */
} AwbExpPriorConfig_t;



/*****************************************************************************/
/**
 * @brief   Initialize Exposure Prior Probability Module ( EPPM )
 *
 * This functions initializes the Memory-Input-Modul .
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
RESULT AwbExpPriorInit
(
    AwbContext_t        *pAwbCtx,
    AwbExpPriorConfig_t *pConfig
);



/*****************************************************************************/
/**
 * @brief   Initialize Exposure Prior Probability Module ( EPPM )
 *
 * This functions initializes the Memory-Input-Modul .
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
RESULT AwbExpPriorReset
(
    AwbContext_t        *pAwbCtx,
    AwbExpPriorConfig_t *pConfig
);



/*****************************************************************************/
/**
 * @brief   Initialize Exposure Prior Probability Module ( EPPM )
 *
 * This functions initializes the Memory-Input-Modul .
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
RESULT AwbExpPriorRelease
(
    AwbContext_t *pAwbCtx
);



/*****************************************************************************/
/**
 * @brief   Initialize Exposure Prior Probability Module ( EPPM )
 *
 * This functions initializes the Memory-Input-Modul .
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
RESULT AwbExpResizeIIRFilter
(
    AwbContext_t    *pAwbCtx,
    const uint16_t  size,
    const float     initValue
);



/*****************************************************************************/
/**
 * @brief   Initialize Exposure Prior Probability Module ( EPPM )
 *
 * This functions initializes the Memory-Input-Modul .
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
RESULT AwbExpPriorProcessFrame
(
    AwbContext_t *pAwbCtx
);


#ifdef __cplusplus
}
#endif

/* @} EXPPRIOR */

#endif /* __EXPPRIOR_H__ */
