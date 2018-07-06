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
#ifndef __ACC_H__
#define __ACC_H__

/**
 * @file acc.h
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
 * @defgroup ACC Adaptive Color Correction 
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include "awb_ctrl.h"

#ifdef __cplusplus
extern "C"
{
#endif


RESULT AwbAccProcessFrame
(
    AwbContext_t *pAwbCtx
);


#ifdef __cplusplus
}
#endif

/* @} ACC */


#endif /* __ACC_H__ */
