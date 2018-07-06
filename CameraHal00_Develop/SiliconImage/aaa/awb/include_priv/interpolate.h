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
#ifndef __INTERPOLATE_H__
#define __INTERPOLATE_H__

/**
 * @file interpolate.h
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
 * @defgroup Common AWB Common functions
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
 *          InterpolateCtx_t
 *
 * @brief   context structure for function interpolation function Interpolate
 *
 ******************************************************************************/
typedef struct InterpolateCtx_s
{
    float*  	pX;       	/**< x-array, lookup table */
    float*  	pY;       	/**< y-array			   */
    uint16_t	size; 		/**< size of above arrays  */
    float  		x_i;        /**< value to find matching interval for in x-array */
    float		y_i;        /**< result calculated from x_i, pfx and pfy */
} InterpolateCtx_t;



/*****************************************************************************/
/**
 *          Interpolate()
 *
 * @brief   This function performs 1D data linear interpolation (table 
 *          lookup) using the same syntax as the MATLAB function 
 *          yi = interp1(x,Y,xi,'linear'). For more details go to
 *          http://www.mathworks.com/access/helpdesk/help/techdoc/ref/interp1.html
 *
 * @param   pInterpolationCtx
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_NULL_POINTER
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
RESULT Interpolate
(
    InterpolateCtx_t *pInterpolationCtx
);



#ifdef __cplusplus
}
#endif

/* @} Common */


#endif /* __INTERPOLATE_H__ */
