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
#ifndef __WP_REVERT_H__
#define __WP_REVERT_H__

/**
 * @file wprevert.h
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
 * @defgroup WPREVERT White Pixel Revert Calculation Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#ifdef __cplusplus
extern "C"
{
#endif


RESULT AwbWpRevertProcessFrame
(
    AwbContext_t *pAwbCtx
);


#ifdef __cplusplus
}
#endif

/* @} WPREVERT */


#endif /* __WP_REVERT_H__ */
