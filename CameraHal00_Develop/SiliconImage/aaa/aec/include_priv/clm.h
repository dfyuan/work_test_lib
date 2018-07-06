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
#ifndef __CLM_H__
#define __CLM_H__

/**
 * @file clm.h
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
 * @defgroup CLM Control Loop Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>

#include "aec_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif


RESULT ClmExecute
(
    AecContext_t        *pAecCtx,
    const float         SetPoint,
    CamerIcHistBins_t   bins,
    float               *NewExposure 
);


#ifdef __cplusplus
}
#endif

/* @} CLM */


#endif /* __CLM_H__ */
