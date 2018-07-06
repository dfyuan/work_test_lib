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
#ifndef __WB_GAIN_COMMON_H__
#define __WB_GAIN_COMMON_H__

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



#ifdef __cplusplus
extern "C"
{
#endif


/******************************************************************************/
/**
 *          WbGainsOverG_t
 *
 * @brief   context structure for function interpolation function Interpolate
 *
 ******************************************************************************/
typedef struct WbGainsOverG_s
{
    float GainROverG;                           /**< (Gain-Red / Gain-Green) */
    float GainBOverG;                           /**< (Gain-Blue / Gain-Green) */
} WbGainsOverG_t;


#ifdef __cplusplus
}
#endif

/* @} WBGAIN */


#endif /* __WB_GAIN_COMMON_H__ */
