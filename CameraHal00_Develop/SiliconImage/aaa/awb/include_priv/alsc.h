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
#ifndef __ALSC_H__
#define __ALSC_H__

/**
 * @file alsc.h
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
 * @defgroup ALSC Adaptive Lense Shade Correction 
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


RESULT AwbAlscProcessFrame
(
    AwbContext_t *pAwbCtx
);


#ifdef __cplusplus
}
#endif

/* @} ALSC */


#endif /* __ALSC_H__ */
