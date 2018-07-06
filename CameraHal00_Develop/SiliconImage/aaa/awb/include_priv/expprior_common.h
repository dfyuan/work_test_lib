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
#ifndef __EXPPRIOR_COMMON_H__
#define __EXPPRIOR_COMMON_H__

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

#ifdef __cplusplus
extern "C"
{
#endif

#define AWB_EXPPRIOR_FILTER_SIZE_MAX   50       /**< AWB prior exp filter count max. It should be higher than the min count */
#define AWB_EXPPRIOR_FILTER_SIZE_MIN   1        /**< AWB prior exp filter count min */

#define AWB_EXPPRIOR_MIDDLE             0.5f    /**< value to initialize the gexp prior filter */



/*****************************************************************************/
/**
 *  @brief  Evaluation of the calculated probability
 */
/*****************************************************************************/
typedef enum ExpPriorCalcEvaluation_e
{
    EXPPRIOR_INVALID            = 0,
    EXPPRIOR_INDOOR             = 1,    /**< evaluated to indoor */
    EXPPRIOR_OUTDOOR            = 2,    /**< evaluated to outdoor */    
    EXPPRIOR_TRANSITION_RANGE   = 3,    /**< evaluated to transition range between indoor and outdoor */
    EXPPRIOR_MAX                = 4,
} ExpPriorCalcEvaluation_t;



/*****************************************************************************/
/**
 *  @brief  Configuration structure of the Exposure and Prior Probability Module
 */
/*****************************************************************************/
typedef struct AwbExpPriorContext_s
{
    float       IIRDampCoefAdd;             /**< incrementer of damping coefficient */
    float       IIRDampCoefSub;             /**< decrementer of damping coefficient */
    float       IIRDampFilterThreshold;     /**< threshold for incrementing or decrementing of damping coefficient */

    float       IIRDampingCoefMin;          /**< minmuim value of damping coefficient */
    float       IIRDampingCoefMax;          /**< maximum value of damping coefficient */
    float       IIRDampingCoefInit;         /**< initial value of damping coefficient */

    uint32_t    IIRFilterSize;              /**< number of filter items */
    float       IIRFilterInitValue;         /**< initial value of the filter items */

    float       *pIIRFilterItems;           /**< */
    uint16_t    IIRCurFilterItem;           /**< */
} AwbExpPriorContext_t;



#ifdef __cplusplus
}
#endif

/* @} EXPPRIOR */

#endif /* __EXPPRIOR_COMMON_H__ */
