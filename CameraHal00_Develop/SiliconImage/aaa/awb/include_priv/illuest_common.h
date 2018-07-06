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
#ifndef __ILLUEST_COMMON_H__
#define __ILLUEST_COMMON_H__

/**
 * @file illuest_common.h
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
 * @defgroup ILLUEST Illumination Estimation Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>



#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 *  @brief   Region type 
 *
 *  @anchor  IlluEstRegion_t
 */
/*****************************************************************************/
typedef enum IlluEstRegion_e
{
    ILLUEST_REGION_INVALID  = 0,    /**< Region undefined (initialization value) */
    ILLUEST_REGION_A        = 1,    /**< Region A */
    ILLUEST_REGION_B        = 2,    /**< Region B */
    ILLUEST_REGION_C        = 3,    /**< Region C */
} IlluEstRegion_t;



#ifdef __cplusplus
}
#endif

/* @} ILLUEST */


#endif /* __ILLUEST_COMMON_H__ */
