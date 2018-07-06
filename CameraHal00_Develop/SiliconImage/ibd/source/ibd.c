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

#ifdef IBD_PPS
#include <sys/time.h>
#endif // IBD_PPS

#include <string.h> // for memset()

#include <ebase/trace.h>
#include <ebase/dct_assert.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include "ibd.h"
#include "ibd_common.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(IBD_INFO , "IBD: ", INFO,     0);
CREATE_TRACER(IBD_WARN , "IBD: ", WARNING,  1);
CREATE_TRACER(IBD_ERROR, "IBD: ", ERROR,    1);

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
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * ibdCreateContext()
 *****************************************************************************/
ibdContext_t* ibdCreateContext
(
    HalHandle_t         halHandle,
    PicBufMetaData_t    *pPicBufMetaData
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    ibdContext_t *pIbdContext;

    TRACE(IBD_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pPicBufMetaData != NULL );

    // allocate context
    pIbdContext = (ibdContext_t *)malloc( sizeof(ibdContext_t) );

    // init context
    memset( pIbdContext, 0, sizeof(ibdContext_t) );
    pIbdContext->halHandle = halHandle;

    if ( NULL != pIbdContext->halHandle )
    {
        // reference HAL
        result = HalAddRef( pIbdContext->halHandle );
        if (RET_SUCCESS != result)
        {
            TRACE( IBD_ERROR, "%s: HalAddRef() failed (RESULT=%d)\n", __FUNCTION__, result);
            goto cleanup_0;
        }
    }

    // pre-check buffer meta data
    result = PicBufIsConfigValid( pPicBufMetaData );
    if (RET_SUCCESS != result)
    {
        TRACE(IBD_ERROR, "%s PicBufIsConfigValid() failed (RESULT=%d)\n", __FUNCTION__, result);
        goto cleanup_1;
    }

    // depending on data format & layout determine subroutines to:
    // ...handle buffers (get/map data into local buffers)
    // ...draw in buffers
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        case PIC_BUF_TYPE_JPEG:
        case PIC_BUF_TYPE_YCbCr444:
        case PIC_BUF_TYPE_YCbCr420:
        case PIC_BUF_TYPE_RGB888:
            UPDATE_RESULT( result, RET_NOTSUPP );
            break;
        case PIC_BUF_TYPE_YCbCr422:
            switch (pPicBufMetaData->Layout)
            {
                case PIC_BUF_LAYOUT_PLANAR:
                case PIC_BUF_LAYOUT_COMBINED:
                    UPDATE_RESULT( result, RET_NOTSUPP );
                    break;
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                {
                    // set format/layout dependent function pointers
                    pIbdContext->MapBuffer     = pIbdContext->halHandle ? ibdMapBufferYUV422Semi : NULL;
                    pIbdContext->UnMapBuffer   = pIbdContext->halHandle ? ibdUnMapBufferYUV422Semi : NULL;
                    pIbdContext->UnScaleCoords = ibdUnScaleCoordsYUV422Semi;
                    pIbdContext->DrawPixel     = ibdDrawPixelYUV422Semi;
                    pIbdContext->DrawLine      = ibdDrawLineYUV422Semi;
                    pIbdContext->DrawBox       = ibdDrawBoxYUV422Semi;
                    pIbdContext->DrawRect      = ibdDrawRectYUV422Semi;
                    pIbdContext->DrawText      = ibdDrawTextYUV422Semi;
                    break;
                }
                default:
                    UPDATE_RESULT( result, RET_NOTSUPP );
            };
            break;
        default:
            UPDATE_RESULT( result, RET_NOTSUPP );
    }
    if (RET_SUCCESS != result)
    {
        TRACE(IBD_ERROR, "%s unsupported buffer config (RESULT=%d)\n", __FUNCTION__, result);
        goto cleanup_1;
    }

    if ( NULL != pIbdContext->halHandle )
    {
        // map buffer
        result = pIbdContext->MapBuffer( pIbdContext, pPicBufMetaData );
        if (RET_SUCCESS != result)
        {
            TRACE(IBD_ERROR, "%s MapBuffer() failed (RESULT=%d)\n", __FUNCTION__, result);
            goto cleanup_1;
        }
    }
    else
    {
        // just copy buffer meta data
        pIbdContext->bufferMetaData = *pPicBufMetaData;
    }

    TRACE(IBD_INFO, "%s (exit)\n", __FUNCTION__ );

    // success
    return pIbdContext;

    // failure cleanup
cleanup_1:
    if ( NULL != pIbdContext->halHandle )
    {
        // release HAL
        HalDelRef( pIbdContext->halHandle );
    }

cleanup_0: // release context structure
    free( pIbdContext );

    return NULL;
}

/******************************************************************************
 * ibdDestroyContext()
 *****************************************************************************/
RESULT ibdDestroyContext
(
    ibdContext_t        *pIbdContext
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pIbdContext != NULL );

    if ( NULL != pIbdContext->halHandle )
    {
        // unmap buffer
        lres = pIbdContext->UnMapBuffer( pIbdContext );
        UPDATE_RESULT( result, lres );
        if (RET_SUCCESS != lres)
        {
            TRACE(IBD_ERROR, "%s UnMapBuffer() failed (RESULT=%d)\n", __FUNCTION__, lres);
        }

        // release HAL
        lres = HalDelRef( pIbdContext->halHandle );
        UPDATE_RESULT( result, lres );
        if (RET_SUCCESS != lres)
        {
            TRACE(IBD_ERROR, "%s HalDelRef() failed (RESULT=%d)\n", __FUNCTION__, lres);
        }
    }

    // release context structure
    free( pIbdContext );

    TRACE(IBD_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}

/******************************************************************************
 * ibdDrawCmds()
 *****************************************************************************/
RESULT ibdDrawCmds
(
    ibdContext_t        *pIbdContext,
    uint32_t            numCmds,
    ibdCmd_t            *pIbdCmds,
    bool_t              scaledCoords
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE(IBD_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pIbdContext != NULL );

    // process commands
    uint32_t cmdIdx;
    for (cmdIdx=0; cmdIdx<numCmds; cmdIdx++)
    {
        ibdCmd_t *pCmd = &pIbdCmds[cmdIdx];

        switch (pCmd->cmdId)
        {
            case IBD_DRAW_PIXEL:
                if (scaledCoords)
                {
                    ibdPixelParam_t Params = pCmd->params.pixel;

                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x, Params.y, &Params.x, &Params.y );
                    DCT_ASSERT( lres == RET_SUCCESS );

                    lres = pIbdContext->DrawPixel( pIbdContext, &Params );
                }
                else
                {
                    lres = pIbdContext->DrawPixel( pIbdContext, &pCmd->params.pixel );
                }
                UPDATE_RESULT( result, lres );
                if (lres != RET_SUCCESS)
                {
                    TRACE(IBD_WARN, "%s DrawPixel() failed (RESULT=%d)\n", __FUNCTION__, lres);
                }
                #ifdef IBD_PPS
                else
                {
                    uint32_t loop, loops = 1000;
                    struct timeval start, stop;

                    gettimeofday( &start, NULL );
                    for (loop=loops; loop; --loop)
                    {
                        pIbdContext->DrawPixel( pIbdContext, &pCmd->params.pixel );
                    }
                    gettimeofday( &stop, NULL );

                    uint32_t pixel   = loops;
                    uint32_t startUs = (start.tv_sec*1000000)+(start.tv_usec);
                    uint32_t stopUs  = (stop.tv_sec *1000000)+(stop.tv_usec);
                    uint32_t deltaUs = stopUs - startUs;

                    TRACE( IBD_ERROR, " Drawing speed PIXEL = %4.1fMPps (pixel=%u  time=%dus).\n", ((float)pixel) / deltaUs, pixel, deltaUs );
                }
                #endif // IBD_PPS

                break;

            case IBD_DRAW_LINE:
                //TODO: swap coords to assure memory get's accessed with increasing addresses -> usually speeds up things
                if (scaledCoords)
                {
                    ibdLineParam_t Params = pCmd->params.line;

                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x, Params.y, &Params.x, &Params.y );
                    DCT_ASSERT( lres == RET_SUCCESS );
                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x2, Params.y2, &Params.x2, &Params.y2 );
                    DCT_ASSERT( lres == RET_SUCCESS );

                    lres = pIbdContext->DrawLine( pIbdContext, &Params );
                }
                else
                {
                    lres = pIbdContext->DrawLine( pIbdContext, &pCmd->params.line );
                }
                UPDATE_RESULT( result, lres );
                if (lres != RET_SUCCESS)
                {
                    TRACE(IBD_WARN, "%s DrawLine() failed (RESULT=%d)\n", __FUNCTION__, lres);
                }
                #ifdef IBD_PPS
                else
                {
                    uint32_t loop, loops = 100;
                    struct timeval start, stop;

                    gettimeofday( &start, NULL );
                    for (loop=loops; loop; --loop)
                    {
                        pIbdContext->DrawLine( pIbdContext, &pCmd->params.line );
                    }
                    gettimeofday( &stop, NULL );

                    uint32_t pixel   = MAX( ABS(pCmd->params.line.x2 - pCmd->params.line.x), ABS(pCmd->params.line.y2 - pCmd->params.line.y) ) * loops;
                    uint32_t startUs = (start.tv_sec*1000000)+(start.tv_usec);
                    uint32_t stopUs  = (stop.tv_sec *1000000)+(stop.tv_usec);
                    uint32_t deltaUs = stopUs - startUs;

                    TRACE( IBD_ERROR, " Drawing speed LINE = %4.1fMPps (pixel=%u  time=%dus).\n", ((float)pixel) / deltaUs, pixel, deltaUs );
                }
                #endif // IBD_PPS

                break;

            case IBD_DRAW_BOX:
                //TODO: swap coords to assure memory get's accessed with increasing addresses -> usually speeds up things
                if (scaledCoords)
                {
                    ibdBoxParam_t Params = pCmd->params.box;

                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x, Params.y, &Params.x, &Params.y );
                    DCT_ASSERT( lres == RET_SUCCESS );
                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x2, Params.y2, &Params.x2, &Params.y2 );
                    DCT_ASSERT( lres == RET_SUCCESS );

                    lres = pIbdContext->DrawBox( pIbdContext, &Params );
                }
                else
                {
                    lres = pIbdContext->DrawBox( pIbdContext, &pCmd->params.box );
                }
                UPDATE_RESULT( result, lres );
                if (lres != RET_SUCCESS)
                {
                    TRACE(IBD_WARN, "%s DrawBox() failed (RESULT=%d)\n", __FUNCTION__, lres);
                }
                #ifdef IBD_PPS
                else
                {
                    uint32_t loop, loops = 100;
                    struct timeval start, stop;

                    gettimeofday( &start, NULL );
                    for (loop=loops; loop; --loop)
                    {
                        pIbdContext->DrawBox( pIbdContext, &pCmd->params.box );
                    }
                    gettimeofday( &stop, NULL );

                    uint32_t pixel   = ABS( 2*(pCmd->params.box.x2 - pCmd->params.box.x) + 2*(pCmd->params.box.y2 - pCmd->params.box.y) ) * loops;
                    uint32_t startUs = (start.tv_sec*1000000)+(start.tv_usec);
                    uint32_t stopUs  = (stop.tv_sec *1000000)+(stop.tv_usec);
                    uint32_t deltaUs = stopUs - startUs;

                    TRACE( IBD_ERROR, " Drawing speed BOX = %4.1fMPps (pixel=%u  time=%dus).\n", ((float)pixel) / deltaUs, pixel, deltaUs );
                }
                #endif // IBD_PPS

                break;

            case IBD_DRAW_RECT:
                //TODO: swap coords to assure memory get's accessed with increasing addresses -> usually speeds up things
                if (scaledCoords)
                {
                    ibdRectParam_t Params = pCmd->params.rect;

                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x, Params.y, &Params.x, &Params.y );
                    DCT_ASSERT( lres == RET_SUCCESS );
                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x2, Params.y2, &Params.x2, &Params.y2 );
                    DCT_ASSERT( lres == RET_SUCCESS );

                    lres = pIbdContext->DrawRect( pIbdContext, &Params );
                }
                else
                {
                    lres = pIbdContext->DrawRect( pIbdContext, &pCmd->params.rect );
                }
                UPDATE_RESULT( result, lres );
                if (lres != RET_SUCCESS)
                {
                    TRACE(IBD_WARN, "%s DrawRect() failed (RESULT=%d)\n", __FUNCTION__, lres);
                }
                #ifdef IBD_PPS
                else
                {
                    uint32_t loop, loops = 10;
                    struct timeval start, stop;

                    gettimeofday( &start, NULL );
                    for (loop=loops; loop; --loop)
                    {
                        pIbdContext->DrawRect( pIbdContext, &pCmd->params.rect );
                    }
                    gettimeofday( &stop, NULL );

                    uint32_t pixel   = ABS( (pCmd->params.rect.x2 - pCmd->params.rect.x) * (pCmd->params.rect.y2 - pCmd->params.rect.y) ) * loops;
                    uint32_t startUs = (start.tv_sec*1000000)+(start.tv_usec);
                    uint32_t stopUs  = (stop.tv_sec *1000000)+(stop.tv_usec);
                    uint32_t deltaUs = stopUs - startUs;

                    TRACE( IBD_ERROR, " Drawing speed RECT = %4.1fMPps (pixel=%u  time=%dus).\n", ((float)pixel) / deltaUs, pixel, deltaUs );
                }
                #endif // IBD_PPS

                break;

            case IBD_DRAW_TEXT:
                // don't swap coords here, as they indirectly specifiy drawing direction as well
                if (scaledCoords)
                {
                    ibdTextParam_t Params = pCmd->params.text;

                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x, Params.y, &Params.x, &Params.y );
                    DCT_ASSERT( lres == RET_SUCCESS );
                    lres = pIbdContext->UnScaleCoords( pIbdContext, Params.x2, Params.y2, &Params.x2, &Params.y2 );
                    DCT_ASSERT( lres == RET_SUCCESS );

                    lres = pIbdContext->DrawText( pIbdContext, &Params );
                }
                else
                {
                    lres = pIbdContext->DrawText( pIbdContext, &pCmd->params.text );
                }
                UPDATE_RESULT( result, lres );
                if (lres != RET_SUCCESS)
                {
                    TRACE(IBD_ERROR, "%s DrawText() failed (RESULT=%d)\n", __FUNCTION__, lres);
                }
                break;

            default:
                TRACE(IBD_ERROR, "%s unknown command %d\n", __FUNCTION__, pCmd->cmdId);
        }
    }

    TRACE(IBD_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * Local functions
 *****************************************************************************/
