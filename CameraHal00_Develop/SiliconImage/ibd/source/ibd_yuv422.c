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
 * @ibd.c
 *
 * @brief
 *   Implementation of ibd.
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
 */

#include <stdlib.h>

#ifdef IBD_PPS
#include <sys/time.h>
#endif // IBD_PPS


#include <ebase/trace.h>

#include <common/return_codes.h>
#include <common/align.h>
#include <common/misc.h>

#include "ibd.h"
#include "ibd_common.h"
#include "font.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(IBD_YUV422_INFO , "IBD-YUV422: ", INFO,  0);
CREATE_TRACER(IBD_YUV422_ERROR, "IBD-YUV422: ", ERROR, 1);

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/******************************************************************************
 * ibdSetPixelYUV422SemiRaw()
 *****************************************************************************/
INLINE void ibdSetPixelYUV422SemiRaw
(
    uint8_t         *pY,
    uint8_t         *pCbCr,
    ibdColor_t      color
);

/******************************************************************************
 * ibdConfColorYUV422Semi()
 *****************************************************************************/
INLINE ibdColor_t ibdConfColorYUV422Semi
(
    ibdColor_t  color //!< Input color (ARGB)
);

/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * ibdMapBufferYUV422Semi()
 *****************************************************************************/
RESULT ibdMapBufferYUV422Semi
(
    ibdContext_t        *pibdContext,
    PicBufMetaData_t    *pPicBufMetaDataRaw
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pibdContext == NULL) || (pPicBufMetaDataRaw == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // copy buffer meta data; clear mapped buffer pointers for easier unmapping on errors below
    pibdContext->bufferMetaData = *pPicBufMetaDataRaw;
    pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y.pBuffer = NULL;
    pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer = NULL;
    // note: implementation which assumes that on-board memory is used for buffers!

    // get sizes & base addresses of planes
    uint32_t YCPlaneSize  = pPicBufMetaDataRaw->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBufMetaDataRaw->Data.YCbCr.semiplanar.Y.PicHeightPixel;
    ulong_t  YBaseAddr    = (ulong_t) (pPicBufMetaDataRaw->Data.YCbCr.semiplanar.Y.pBuffer);
    ulong_t  CbCrBaseAddr = (ulong_t) (pPicBufMetaDataRaw->Data.YCbCr.semiplanar.CbCr.pBuffer);

    #ifdef IBD_PPS
        struct timeval startTV, stopTV;
        gettimeofday( &startTV, NULL );
    #endif // IBD_PPS

    // map luma plane
    lres = HalMapMemory( pibdContext->halHandle, YBaseAddr,    YCPlaneSize, HAL_MAPMEM_READWRITE, (void**)&(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y.pBuffer)    );
    UPDATE_RESULT( result, lres );

    // map combined chroma plane
    lres = HalMapMemory( pibdContext->halHandle, CbCrBaseAddr, YCPlaneSize, HAL_MAPMEM_READWRITE, (void**)&(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer) );
    UPDATE_RESULT( result, lres );

    #ifdef IBD_PPS
        gettimeofday( &stopTV, NULL );
        uint32_t startUs = (startTV.tv_sec*1000000)+(startTV.tv_usec);
        uint32_t stopUs  = (stopTV.tv_sec *1000000)+(stopTV.tv_usec);
        uint32_t deltaUs = stopUs - startUs;
        TRACE( IBD_YUV422_ERROR, " Mapping speed = %4.1fMBps (bytes=%u  time=%dus).\n", ((float)(YCPlaneSize*2)) / deltaUs, YCPlaneSize*2, deltaUs );
    #endif // IBD_PPS

    // check for errors
    if (result != RET_SUCCESS)
    {
        TRACE(IBD_YUV422_ERROR, "%s mapping buffer failed (RESULT=%d)\n", __FUNCTION__, result);
        // unmap partially mapped buffer
        ibdUnMapBufferYUV422Semi( pibdContext );
    }

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * ibdUnMapBufferYUV422Semi()
 *****************************************************************************/
RESULT ibdUnMapBufferYUV422Semi
(
    ibdContext_t        *pibdContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if (pibdContext == NULL)
    {
        return RET_NULL_POINTER;
    }
    // note: implementation which assumes that on-board memory is used for buffers!

    #ifdef IBD_PPS
        struct timeval startTV, stopTV;
        uint32_t YCPlaneSize = pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y.PicWidthBytes * pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y.PicHeightPixel;
        gettimeofday( &startTV, NULL );
    #endif // IBD_PPS

    // unmap (partially) mapped buffer
    if (pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y.pBuffer)
    {
        // unmap luma plane
        lres = HalUnMapMemory( pibdContext->halHandle, pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y.pBuffer    );
        UPDATE_RESULT( result, lres );
    }
    if (pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer)
    {
        // unmap combined chroma plane
        lres = HalUnMapMemory( pibdContext->halHandle, pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr.pBuffer );
        UPDATE_RESULT( result, lres );
    }

    #ifdef IBD_PPS
        gettimeofday( &stopTV, NULL );
        uint32_t startUs = (startTV.tv_sec*1000000)+(startTV.tv_usec);
        uint32_t stopUs  = (stopTV.tv_sec *1000000)+(stopTV.tv_usec);
        uint32_t deltaUs = stopUs - startUs;
        TRACE( IBD_YUV422_ERROR, " Unmapping speed = %4.1fMBps (bytes=%u  time=%dus).\n", ((float)(YCPlaneSize*2)) / deltaUs, YCPlaneSize*2, deltaUs );
    #endif // IBD_PPS

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * ibdUnScaleCoordsYUV422Semi()
 *****************************************************************************/
RESULT ibdUnScaleCoordsYUV422Semi(
    ibdContext_t    *pibdContext,
    int32_t         x,
    int32_t         y,
    int32_t         *pXout,
    int32_t         *pYout
)
{
    if ( (pibdContext == NULL) || (pXout == NULL) || (pYout == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get plane info
    PicBufPlane_t *pPbpY = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y);

    // unscale coords
    *pXout = ibdUnscaleCoord( x, pPbpY->PicWidthPixel  );
    *pYout = ibdUnscaleCoord( y, pPbpY->PicHeightPixel );

    return RET_SUCCESS;
}


/******************************************************************************
 * ibdDrawPixelYUV422Semi()
 *****************************************************************************/
RESULT ibdDrawPixelYUV422Semi
(
    ibdContext_t    *pibdContext,
    ibdPixelParam_t *pParams
)
{
    RESULT result = RET_SUCCESS;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if (pibdContext == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get plane info
    PicBufPlane_t *pPbpY    = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y);
    PicBufPlane_t *pPbpCbCr = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr);

    // check limits
    if ( (pParams->x < 0) || ((uint32_t)(pParams->x) >= pPbpY->PicWidthPixel )
      || (pParams->y < 0) || ((uint32_t)(pParams->y) >= pPbpY->PicHeightPixel) )
    {
        return RET_OUTOFRANGE;
    }

    // do the real work
    {
        // convert color to our color space
        ibdColor_t color = ibdConfColorYUV422Semi( pParams->color );

        // get pixel addresses
        uint32_t idxYCbCr = (pParams->y * pPbpY->PicWidthBytes) + pParams->x; // common index into planes
        uint8_t  *pY    = &(pPbpY->pBuffer[idxYCbCr]);    // get pointers to pixel for both planes
        uint8_t  *pCbCr = &(pPbpCbCr->pBuffer[idxYCbCr]); // no need to take care of alignment here

        // draw pixel
        ibdSetPixelYUV422SemiRaw( pY, pCbCr, color );
    }

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * ibdDrawLineYUV422Semi()
 *****************************************************************************/
RESULT ibdDrawLineYUV422Semi
(
    ibdContext_t    *pibdContext,
    ibdLineParam_t  *pParams
)
{
    RESULT result = RET_SUCCESS;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if ((pibdContext == NULL) || (pParams == NULL))
    {
        return RET_NULL_POINTER;
    }

    // get plane info
    PicBufPlane_t *pPbpY    = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y);
    PicBufPlane_t *pPbpCbCr = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr);

    // check limits
    if ( (pParams->x  < 0) || ((uint32_t)(pParams->x ) >= pPbpY->PicWidthPixel )
      || (pParams->y  < 0) || ((uint32_t)(pParams->y ) >= pPbpY->PicHeightPixel)
      || (pParams->x2 < 0) || ((uint32_t)(pParams->x2) >= pPbpY->PicWidthPixel )
      || (pParams->y2 < 0) || ((uint32_t)(pParams->y2) >= pPbpY->PicHeightPixel) )
    {
        return RET_OUTOFRANGE;
    }

    // do the real work
    // fast version of Bresenham's algorithm, for our semiplanar YCbCr pixel addressing
    {
        int32_t i, dx, dy, incx, incy, par, diag, es, el, err;
        uint32_t idxYCbCr;
        uint8_t *pY, *pCbCr;

        // convert color to our color space
        ibdColor_t color = ibdConfColorYUV422Semi( pParams->color );

        // determine address steps
        incx = 1;                    // x-direction
        incy = pPbpY->PicWidthBytes; // y-direction

        // get pointers to first pixel in both Y & CbCr planes
        idxYCbCr = (pParams->y * incy) + (pParams->x * incx); // common index into planes
        pY    = &(pPbpY->pBuffer[idxYCbCr]);
        pCbCr = &(pPbpCbCr->pBuffer[idxYCbCr]); // no need to take care of alignment right now

        // calc distances for both directions
        dx = pParams->x2 - pParams->x;
        dy = pParams->y2 - pParams->y;

        // respect signs of the increments
        if (dx < 0)
        {
            incx = -incx;
            dx = -dx;
        }
        if (dy < 0)
        {
            incy = -incy;
            dy = -dy;
        }

        // draw line
        if (dx == 0) // vertical line?
        {
            TRACE(IBD_YUV422_INFO, "%s vertical dy=%d, incy=%d\n", __FUNCTION__, dy, incy);

            ibdSetPixelYUV422SemiRaw( pY, pCbCr, color ); // draw the first pixel
            for (i=dy; i; i--)
            {
                pY    += incy;  // step in y direction
                pCbCr += incy;  // step in y direction
                ibdSetPixelYUV422SemiRaw( pY, pCbCr, color ); // draw the next pixel
            }
        }
        else if (dy == 0) // horizontal line?
        {
            TRACE(IBD_YUV422_INFO, "%s horizontal dx=%d, incx=%d\n", __FUNCTION__, dx, incx);

            ibdSetPixelYUV422SemiRaw( pY, pCbCr, color ); // draw the first pixel
            for (i=dx; i; i--)
            {
                pY    += incx;  // step in x direction
                pCbCr += incx;  // step in x direction
                ibdSetPixelYUV422SemiRaw( pY, pCbCr, color ); // draw the next pixel
            }
        }
        else // any other line!
        {
            TRACE(IBD_YUV422_INFO, "%s other dx=%d, incx=%d, dy=%d, incy=%d\n", __FUNCTION__, dx, incx, dy, incy);

            // determine address offsets and error step sizes
            if (dx>dy)
            {
                // x is the faster direction
                par = incx; // address offset for parallel step
                es = dy;    // error step small
                el = dx;    // error step large
            }
            else
            {
                // y is the faster direction
                par = incy; // address offset for parallel step
                es = dx;    // error step small
                el = dy;    // error step large
            }
            diag = incx + incy; // address offset for diagonal step

            // init loop
            err = el/2;
            ibdSetPixelYUV422SemiRaw( pY, pCbCr, color ); // draw the first pixel

            // loop drawing line pixelwise
            for (i=el; i; i--) // i counts the pixels, el is the total count
            {
                err -= es;          // update the error term
                if(err<0)
                {
                    err   += el;    // make the error term positive (>=0) again
                    pY    += diag;  // step in the slow direction, diagonal step
                    pCbCr += diag;  // step in the slow direction, diagonal step
                }
                else
                {
                    pY    += par;   // step in the fast direction, parallel step
                    pCbCr += par;   // step in the fast direction, parallel step
                }
                ibdSetPixelYUV422SemiRaw( pY, pCbCr, color );  // draw the next pixel
            }
        }
    }

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * ibdDrawBoxYUV422Semi()
 *****************************************************************************/
RESULT ibdDrawBoxYUV422Semi
(
    ibdContext_t    *pibdContext,
    ibdBoxParam_t   *pParams
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if ((pibdContext == NULL) || (pParams == NULL))
    {
        return RET_NULL_POINTER;
    }

    // no more parameter checking here, we leave this up to ibdDrawLineYUV422Semi()

    // do the real work
    {
        ibdLineParam_t lineParams;
        lineParams.color = pParams->color;

        // top line
        lineParams.x  = pParams->x;
        lineParams.y  = pParams->y;
        lineParams.x2 = pParams->x2;
        lineParams.y2 = pParams->y;

        lres = ibdDrawLineYUV422Semi( pibdContext, &lineParams );
        UPDATE_RESULT( result, lres );

        // left line
        lineParams.x  = pParams->x;
        lineParams.y  = pParams->y;
        lineParams.x2 = pParams->x;
        lineParams.y2 = pParams->y2;

        lres = ibdDrawLineYUV422Semi( pibdContext, &lineParams );
        UPDATE_RESULT( result, lres );

        // right line
        lineParams.x  = pParams->x2;
        lineParams.y  = pParams->y;
        lineParams.x2 = pParams->x2;
        lineParams.y2 = pParams->y2;

        lres = ibdDrawLineYUV422Semi( pibdContext, &lineParams );
        UPDATE_RESULT( result, lres );

        // bottom line
        lineParams.x  = pParams->x;
        lineParams.y  = pParams->y2;
        lineParams.x2 = pParams->x2;
        lineParams.y2 = pParams->y2;

        lres = ibdDrawLineYUV422Semi( pibdContext, &lineParams );
        UPDATE_RESULT( result, lres );
    }

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * ibdDrawRectYUV422Semi()
 *****************************************************************************/
RESULT ibdDrawRectYUV422Semi
(
    ibdContext_t    *pibdContext,
    ibdRectParam_t  *pParams
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if ((pibdContext == NULL) || (pParams == NULL))
    {
        return RET_NULL_POINTER;
    }

    // get plane info
    PicBufPlane_t *pPbpY    = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y);
    PicBufPlane_t *pPbpCbCr = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr);

    // check limits
    if ( (pParams->x  < 0) || ((uint32_t)(pParams->x ) >= pPbpY->PicWidthPixel )
      || (pParams->y  < 0) || ((uint32_t)(pParams->y ) >= pPbpY->PicHeightPixel)
      || (pParams->x2 < 0) || ((uint32_t)(pParams->x2) >= pPbpY->PicWidthPixel )
      || (pParams->y2 < 0) || ((uint32_t)(pParams->y2) >= pPbpY->PicHeightPixel) )
    {
        return RET_OUTOFRANGE;
    }

    // do the real work
    {
        int32_t i, dy, incy;

        ibdLineParam_t lineParams;
        lineParams.color = pParams->color;

        // init loop
        dy = pParams->y2 - pParams->y;
        incy = (dy<0) ? -1 : 1;
        lineParams.x  = pParams->x;
        lineParams.y  = pParams->y;
        lineParams.x2 = pParams->x2;
        lineParams.y2 = pParams->y;
        lres = ibdDrawLineYUV422Semi( pibdContext, &lineParams ); // draw first line
        UPDATE_RESULT( result, lres );

        // loop drawing rect linewise
        while (lineParams.y != pParams->y2)
        {
            lineParams.y  += incy;
            lineParams.y2 += incy;
            lres = ibdDrawLineYUV422Semi( pibdContext, &lineParams ); // draw next line
            UPDATE_RESULT( result, lres );
        }
    }

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * ibdDrawTextYUV422Semi()
 *****************************************************************************/
RESULT ibdDrawTextYUV422Semi
(
    ibdContext_t    *pibdContext,
    ibdTextParam_t  *pParams
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

    if ((pibdContext == NULL) || (pParams == NULL))
    {
        return RET_NULL_POINTER;
    }

    // get plane info
    PicBufPlane_t *pPbpY    = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.Y);
    PicBufPlane_t *pPbpCbCr = &(pibdContext->bufferMetaData.Data.YCbCr.semiplanar.CbCr);

    // check limits
    if ( (pParams->x  < 0) || ((uint32_t)(pParams->x ) >= pPbpY->PicWidthPixel )
      || (pParams->y  < 0) || ((uint32_t)(pParams->y ) >= pPbpY->PicHeightPixel)
      || (pParams->x2 < 0) || ((uint32_t)(pParams->x2) >= pPbpY->PicWidthPixel )
      || (pParams->y2 < 0) || ((uint32_t)(pParams->y2) >= pPbpY->PicHeightPixel) )
    {
        return RET_OUTOFRANGE;
    }

    if ( (pParams->x  == pParams->x2 )
      || (pParams->y  == pParams->y2 ) )
    {
        return RET_INVALID_PARM;
    }

    if (pParams->fontID >= num_fonts)
    {
        return RET_OUTOFRANGE;
    }

    // do the real work
    {
        const font_t *pFont = fonts[pParams->fontID];

        int32_t i, dx, dy, incx, incy, par, orth, par_remain, orth_remain;
        uint32_t idxYCbCr;
        uint8_t *pY, *pCbCr;
        char *pChar;

        // convert color to our color space
        ibdColor_t color  = ibdConfColorYUV422Semi( pParams->color );
        ibdColor_t colorB = ibdConfColorYUV422Semi( pParams->colorB );

        // determine address steps
        incx = 1;                    // x-direction
        incy = pPbpY->PicWidthBytes; // y-direction

        // get pointers to first pixel in both Y & CbCr planes
        idxYCbCr = (pParams->y * incy) + (pParams->x * incx); // common index into planes
        pY    = &(pPbpY->pBuffer[idxYCbCr]);
        pCbCr = &(pPbpCbCr->pBuffer[idxYCbCr]); // no need to take care of alignment right now

        // calc distances for both directions
        dx = pParams->x2 - pParams->x;
        dy = pParams->y2 - pParams->y;

        // determine address offsets with respect to text drawing direction
        if ( (dx >= 0) && (dy >= 0) )
        {
            // par: left -> right; orth: top -> bottom
            par  = +incx; // address offset for parallel step
            orth = +incy; // address offset for orthogonal step
            par_remain  = +dx; // remaining steps in parallel direction
            orth_remain = +dy; // remaining steps in orthogonal direction
        }
        else
        if ( (dx >= 0) && (dy < 0) )
        {
            // par: bottom -> top; orth: left -> right
            par  = -incy; // address offset for parallel step
            orth = +incx; // address offset for orthogonal step
            par_remain  = -dy; // remaining steps in parallel direction
            orth_remain = +dx; // remaining steps in orthogonal direction
        }
        else
        if ( (dx < 0) && (dy >= 0) )
        {
            // par: top -> bottom; orth: right -> left
            par  = +incy; // address offset for parallel step
            orth = -incx; // address offset for orthogonal step
            par_remain  = +dy; // remaining steps in parallel direction
            orth_remain = -dx; // remaining steps in orthogonal direction
        }
        else
        if ( (dx < 0) && (dy < 0) )
        {
            // par: right -> left; orth: bottom -> top
            par  = -incx; // address offset for parallel step
            orth = -incy; // address offset for orthogonal step
            par_remain  = -dx; // remaining steps in parallel direction
            orth_remain = -dy; // remaining steps in orthogonal direction
        }
        else
        {
            TRACE(IBD_YUV422_ERROR, "%s: fatal situation in dx&dy check (dx=%d, dy=%d)\n", __FUNCTION__, dx, dy);

            return RET_FAILURE;
        }

        // deal with last pixel
        ++par_remain;
        ++orth_remain;

        // loop over all chars
        pChar = pParams->pcText;
        for (i=0; (i < pParams->len) && (par_remain > 0) && (orth_remain > 0); i++)
        {
            // get glyph
            int32_t glyphIdx = (*pChar) - pFont->firstchar;

            // is glyph in font?
            if ( (glyphIdx < 0) || (glyphIdx >= pFont->size) )
            {
                // no, so use default char instead
                glyphIdx = pFont->defaultchar - pFont->firstchar;
            }

            // get glyph data; note that we ignore ascend & descend from baseline, thus drawing always top aligned
            int32_t height = pFont->height;
            int32_t width  = pFont->widths  ? pFont->widths[glyphIdx]  : pFont->maxwidth;
            int32_t offset = pFont->offsets ? pFont->offsets[glyphIdx] : (glyphIdx * pFont->height);
            const bitmap_t* pBits  = &pFont->bits[offset];

            // now draw glyph
            {
                int32_t par_steps  = MIN( width,  par_remain );
                int32_t orth_steps = MIN( height, orth_remain );
                int32_t orth_par = orth - par_steps * par;
                uint8_t *pY_tmp    = pY;
                uint8_t *pCbCr_tmp = pCbCr;
                int32_t o, b, p;
                bitmap_t bits;

                // loop glyph lines
                for (o = orth_steps; o; --o)
                {
                    int32_t par_steps2 = par_steps;
                    const bitmap_t *pBits2 = pBits;

                    // loop glyph bitmaps in line
                    for (b=BITMAP_WORDS(par_steps); (b>0); --b)
                    {
                        // get pixel bitmap
                        bits = *pBits2;

                        // loop glyph pixel in bitmap
                        for (p = MIN( par_steps2, (int32_t)BITMAP_BITSPERIMAGE ); p; --p)
                        {
                            // draw pixel
                            ibdSetPixelYUV422SemiRaw( pY_tmp, pCbCr_tmp, BITMAP_TESTBIT(bits) ? color : colorB );

                            // advance one pixel
                            pY_tmp    += par;
                            pCbCr_tmp += par;
                            bits = BITMAP_SHIFTBIT(bits);
                        }

                        // advance one bitmap
                        par_steps2 -= BITMAP_BITSPERIMAGE;
                        ++pBits2;
                    }

                    // advance one line
                    pY_tmp    += orth_par;
                    pCbCr_tmp += orth_par;
                    pBits     += BITMAP_WORDS(width);
                }
            }

            // advance one char
            pY    += width * par;
            pCbCr += width * par;
            par_remain  -= width;
            ////orth_remain -= 0;
            ++pChar;
        }
    }

    TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * ibdSetPixelYUV422SemiRaw()
 *
 * Unsafe internal function to set a pixel (no range check, raw coordinates).
 *****************************************************************************/
INLINE void ibdSetPixelYUV422SemiRaw
(
    uint8_t         *pY,
    uint8_t         *pCbCr,
    ibdColor_t      color
)
{
    //TRACE(IBD_YUV422_INFO, "%s (enter)\n", __FUNCTION__);

#if 1 // local memory implementation
    // set luma
    *pY = ibdBlend( *pY, color.compAYCbCr.Y, color.compAYCbCr.A );

    // set combined chroma
    pCbCr = (uint8_t*)(((ulong_t)pCbCr) & ~1ul); // align address
    *pCbCr = ibdBlend( *pCbCr, color.compAYCbCr.Cb, color.compAYCbCr.A );
    pCbCr++;
    *pCbCr = ibdBlend( *pCbCr, color.compAYCbCr.Cr, color.compAYCbCr.A );
#else // emulated directly mapped hardware memory implementation
    // set luma
    AlteraFPGABoard_WriteReg( 0x1f0, ibdBlend( AlteraFPGABoard_ReadReg(0x1f0), color.compAYCbCr.Y, color.compAYCbCr.A ) );

    // set combined chroma
    pCbCr = (uint8_t*)(((ulong_t)pCbCr) & ~1ul); // align address
    AlteraFPGABoard_WriteReg( 0x1f4, ibdBlend( AlteraFPGABoard_ReadReg(0x1f4), color.compAYCbCr.Cb, color.compAYCbCr.A ) );
    pCbCr++;
    AlteraFPGABoard_WriteReg( 0x1f8, ibdBlend( AlteraFPGABoard_ReadReg(0x1f8), color.compAYCbCr.Cr, color.compAYCbCr.A ) );
#endif

    //TRACE(IBD_YUV422_INFO, "%s (exit)\n", __FUNCTION__);
}


/******************************************************************************
 * ibdConfColorYUV422Semi()
 *****************************************************************************/
INLINE ibdColor_t ibdConfColorYUV422Semi
(
    ibdColor_t  color
)
{
    int32_t R = color.compARGB.R;
    int32_t G = color.compARGB.G;
    int32_t B = color.compARGB.B;
    ibdColor_t  colorOut = color; // copy alpha; we don't change it

    // convert to YCbCr
////#define USE_FLOAT
#if (1)
        // Standard Definition TV (BT.601) as in VideoDemystified 3; page 18f; RGB(0..255) to YCbCr
    #ifdef USE_FLOAT
        float Y  =  0.257*R + 0.504*G + 0.098*B +  16;
        float Cb = -0.148*R – 0.291*G + 0.439*B + 128;
        float Cr =  0.439*R – 0.368*G – 0.071*B + 128;
    #else
        int32_t Y  = ( ( ((int32_t)( 0.257*1024))*R + ((int32_t)( 0.504*1024))*G + ((int32_t)( 0.098*1024))*B ) >> 10 ) +  16;
        int32_t Cb = ( ( ((int32_t)(-0.148*1024))*R + ((int32_t)(-0.291*1024))*G + ((int32_t)( 0.439*1024))*B ) >> 10 ) + 128;
        int32_t Cr = ( ( ((int32_t)( 0.439*1024))*R + ((int32_t)(-0.368*1024))*G + ((int32_t)(-0.071*1024))*B ) >> 10 ) + 128;
    #endif
#else
        // High Definition TV (BT.709) as in VideoDemystified 3; page 19; RGB(0..255) to YCbCr
    #ifdef USE_FLOAT
        float Y  =  0.183*R + 0.614*G + 0.062*B;
        float Cb = -0.101*R - 0.338*G + 0.439*B;
        float Cr =  0.439*R - 0.399*G - 0.040*B;
    #else
        int32_t Y  = ( ( ((int32_t)( 0.183*1024))*R + ((int32_t)( 0.614*1024))*G + ((int32_t)( 0.062*1024))*B ) >> 10 ) +  16;
        int32_t Cb = ( ( ((int32_t)(-0.101*1024))*R + ((int32_t)(-0.338*1024))*G + ((int32_t)( 0.439*1024))*B ) >> 10 ) + 128;
        int32_t Cr = ( ( ((int32_t)( 0.439*1024))*R + ((int32_t)(-0.399*1024))*G + ((int32_t)(-0.040*1024))*B ) >> 10 ) + 128;
    #endif
#endif
    // clip
    if (Y <0) Y =0; else if (Y >255) Y =255;
    if (Cb<0) Cb=0; else if (Cb>255) Cb=255;
    if (Cr<0) Cr=0; else if (Cr>255) Cr=255;

    colorOut.compAYCbCr.Y  = (uint8_t) Y;
    colorOut.compAYCbCr.Cb = (uint8_t) Cb;
    colorOut.compAYCbCr.Cr = (uint8_t) Cr;

    return colorOut;
}
