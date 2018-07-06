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
#ifndef __ILLUEST_H__
#define __ILLUEST_H__

/**
 * @file illuest.h
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

#include "illuest_common.h"
#include "awb_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif


RESULT AwbIlluEstProcessFrame
(
    AwbContext_t *pAwbCtx
);


#ifdef __cplusplus
}
#endif

/* @} ILLUEST */


#endif /* __ILLUEST_H__ */
