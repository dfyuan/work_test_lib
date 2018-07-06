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
#ifndef __CAMERIC_SCALE_DRV_H__
#define __CAMERIC_SCALE_DRV_H__

/**
 * @file cameric_scale_drv.h
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
 * @defgroup cameric_scale_drv CamerIc Scaler Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

/*****************************************************************************/
/**
 *          CamerIcCalcScaleFactor()
 *
 * @brief   Calculates the scaling factor to program into the scaler registers
 * 			to scale from in- to out resolutions.
 * 			
 * @param	in
 * @param	out
 * @param	factor
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcCalcScaleFactor
(
    const uint16_t  in,
    const uint16_t  out,
    uint32_t        *factor

);

/* @} cameric_scale_drv */

#endif /* __CAMERIC_SCALE_DRV_H__ */

