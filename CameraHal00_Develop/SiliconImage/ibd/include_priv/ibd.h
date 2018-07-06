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
/**
 * @ibd.h
 *
 * @brief
 *   Internal stuff used by ibd implementation.
 *
 *****************************************************************************/
/**
 * @page ibd_page IBD
 * The In-Buffer Display module allows to perform some simple graphics stuff on image buffers.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref ibd_api
 * - @ref ibd_common
 * - @ref ibd
 *
 * @defgroup ibd IBD
 * @{
 *
 */


#ifndef __IBD_H__
#define __IBD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cea_861.h>
#include <common/picture_buffer.h>
#include <hal/hal_api.h>

#include "ibd_common.h"
#include "ibd_api.h"


/******************************************************************************
 * buffer type & layout dependent buffer handling & drawing function types
 *****************************************************************************/
struct ibdContext_s;
typedef RESULT (*pfMapBuffer)     ( struct ibdContext_s *pibdContext, PicBufMetaData_t *pRawBuffer );
typedef RESULT (*pfUnMapBuffer)   ( struct ibdContext_s *pibdContext );
typedef RESULT (*pfUnScaleCoords) ( struct ibdContext_s *pibdContext, int32_t x, int32_t y, int32_t *pXout, int32_t *pYout );
typedef RESULT (*pfDrawPixel_t)   ( struct ibdContext_s *pibdContext, ibdPixelParam_t *pParams );
typedef RESULT (*pfDrawLine_t)    ( struct ibdContext_s *pibdContext, ibdLineParam_t *pParams );
typedef RESULT (*pfDrawBox_t)     ( struct ibdContext_s *pibdContext, ibdBoxParam_t *pParams );
typedef RESULT (*pfDrawRect_t)    ( struct ibdContext_s *pibdContext, ibdRectParam_t *pParams );
typedef RESULT (*pfDrawText_t)    ( struct ibdContext_s *pibdContext, ibdTextParam_t *pParams );


/******************************************************************************
 * buffer type & layout dependent drawing function types
 *****************************************************************************/
typedef struct ibdContext_s
{
    HalHandle_t         halHandle;      //!< Handle to HAL session to use.
    PicBufMetaData_t    bufferMetaData; //!< Media buffer meta data descriptor for mapped buffer.

    pfMapBuffer         MapBuffer;      //!< Suitable handling function for type & layout of buffer.
    pfUnMapBuffer       UnMapBuffer;    //!< Suitable handling function for type & layout of buffer.

    pfUnScaleCoords     UnScaleCoords;  //!< Suitable handling function for type & layout of buffer.

    pfDrawPixel_t       DrawPixel;      //!< Suitable drawing function for type & layout of mapped buffer.
    pfDrawLine_t        DrawLine;       //!< Suitable drawing function for type & layout of mapped buffer.
    pfDrawBox_t         DrawBox;        //!< Suitable drawing function for type & layout of mapped buffer.
    pfDrawRect_t        DrawRect;       //!< Suitable drawing function for type & layout of mapped buffer.
    pfDrawText_t        DrawText;       //!< Suitable drawing function for type & layout of mapped buffer.
} ibdContext_t;


/******************************************************************************
 * buffer type & layout dependent buffer handling & drawing functions
 *****************************************************************************/
extern RESULT ibdMapBufferYUV422Semi    ( ibdContext_t *pibdContext, PicBufMetaData_t *pRawBuffer );
extern RESULT ibdUnMapBufferYUV422Semi  ( ibdContext_t *pibdContext );
extern RESULT ibdUnScaleCoordsYUV422Semi( ibdContext_t *pibdContext, int32_t x, int32_t y, int32_t *pXout, int32_t *pYout );
extern RESULT ibdDrawPixelYUV422Semi    ( ibdContext_t *pibdContext, ibdPixelParam_t *pParams );
extern RESULT ibdDrawLineYUV422Semi     ( ibdContext_t *pibdContext, ibdLineParam_t *pParams );
extern RESULT ibdDrawBoxYUV422Semi      ( ibdContext_t *pibdContext, ibdBoxParam_t *pParams );
extern RESULT ibdDrawRectYUV422Semi     ( ibdContext_t *pibdContext, ibdRectParam_t *pParams );
extern RESULT ibdDrawTextYUV422Semi     ( ibdContext_t *pibdContext, ibdTextParam_t *pParams );


/*****************************************************************************/
/**
 * @brief   Create IBD driver context.
 *
 * @param   halHandle           Handle to HAL session to use.
 * @param   pPicBufMetaData     Picture buffer meta data describing the buffer to use for drawing.
 *
 * @return  Reference to IBD driver context; NULL on failure
 *
 *****************************************************************************/
extern ibdContext_t* ibdCreateContext
(
    HalHandle_t         halHandle,
    PicBufMetaData_t    *pPicBufMetaData
);

/*****************************************************************************/
/**
 * @brief   Destroy IBD driver context.
 *
 * @param   pIbdContext         Reference to driver context as returned by @ref ibdCreateContext().
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_NULL_POINTER
 *
 *****************************************************************************/
extern RESULT ibdDestroyContext
(
    ibdContext_t        *pibdContext
);


/*****************************************************************************/
/**
 * @brief   Execute given number of draw commands, using the given driver context.
 *
 * @param   pIbdContext         Reference to driver context as returned by @ref ibdCreateContext().
 * @param   numCmds             Number of commands in command array.
 * @param   pIbdCmd             Reference to command(s).
 * @param   scaledCoords        Coords are given as scale factors in 0.32 fixed point
 *                              representation rather than as absolut pixel coords.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_NULL_POINTER
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFRANGE
 *
 *****************************************************************************/
extern RESULT ibdDrawCmds
(
    ibdContext_t    *pIbdContext,
    uint32_t        numCmds,
    ibdCmd_t        *pIbdCmds,
    bool_t          scaledCoords
);


/*****************************************************************************/
/**
 * @brief   Helper function to limit against a range.
 *
 * @param   n       Value to clip.
 * @param   min     The lower limit.
 * @param   max     The upper limit.
 *
 * @return  The clipped value.
 *
 *****************************************************************************/
INLINE int32_t ibdClip(int32_t n, int32_t min, int32_t max)
{
	return (n<min) ? min : (n>max) ? max : n;
}


/*****************************************************************************/
/**
 * @brief   Helper function to blend two color components.
 *
 * @param   colBack     Background color.
 * @param   colFore     Foreground color.
 * @param   alphaFore   Alpha of foreground color (0..255 = transparent..opaque).
 *
 * @return  The blended color.
 *
 *****************************************************************************/
INLINE int32_t ibdBlend(int32_t colBack, int32_t colFore, int32_t alphaFore)
{
	// new = (1-alpha)*back + alpha*fore
	// reordered into a single multiply form:
	//     = (fore-back)*alpha + back
	return ( ( (colFore - colBack) * alphaFore ) >> 8 ) + colBack;
}


/* @} ibd */

#ifdef __cplusplus
}
#endif

#endif /* __IBD_H__ */
