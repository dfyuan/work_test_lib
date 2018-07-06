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
 * @file cam_engine_simp_api.c
 *
 * @brief
 *   Implementation of the CamEngine Super Impose API.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_simp_drv_api.h>

#include "cam_engine.h"
#include "cam_engine_api.h"
#include "cam_engine_simp_api.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

USE_TRACER( CAM_ENGINE_API_INFO  );
USE_TRACER( CAM_ENGINE_API_WARN  );
USE_TRACER( CAM_ENGINE_API_ERROR );

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
 * See header file for detailed comment.
 *****************************************************************************/
static CamerIcCompletionCb_t  CompletionCb;



/******************************************************************************
 * CamEnginePreloadImageSimp()
 *****************************************************************************/
static RESULT CamEnginePreloadImageSimp
(
    CamEngineContext_t      *pCamEngineCtx,
    const PicBufMetaData_t  *pSimPicBuffer
)
{
    PicBufMetaData_t *pPicBuffer = NULL;
    MediaBuffer_t *pMediaBuffer  = NULL;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    /* check buffer data */
    DCT_ASSERT( pCamEngineCtx != NULL );
    DCT_ASSERT( pSimPicBuffer != NULL );

    /* get a picture buffer */
    pMediaBuffer = MediaBufPoolGetBuffer( &pCamEngineCtx->BufferPoolInput );
    if ( pMediaBuffer == NULL )
    {
        return ( RET_OUTOFMEM );
    }

    /* set picture buffer meta-data */
    pPicBuffer = (PicBufMetaData_t *)pMediaBuffer->pMetaData;

    /* copy all buffer data */
    *pPicBuffer = *pSimPicBuffer;

    if ( pSimPicBuffer->Layout == PIC_BUF_LAYOUT_PLANAR )
    {
        void *pMapBufferY;
        void *pMapBufferCb;
        void *pMapBufferCr;

        pPicBuffer->Data.YCbCr.planar.Y.pBuffer  = (uint8_t *)pMediaBuffer->pBaseAddress;
        pPicBuffer->Data.YCbCr.planar.Cb.pBuffer = (uint8_t *)( pPicBuffer->Data.YCbCr.planar.Y.pBuffer
                + ((pPicBuffer->Data.YCbCr.planar.Y.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel)) );
        pPicBuffer->Data.YCbCr.planar.Cr.pBuffer = (uint8_t *)( pPicBuffer->Data.YCbCr.planar.Cb.pBuffer
                + ((pPicBuffer->Data.YCbCr.planar.Cb.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel)) );

        /* Y - Plane */
        result = HalMapMemory( pCamEngineCtx->hHal,
                    (unsigned long)( pPicBuffer->Data.YCbCr.planar.Y.pBuffer ),
                    (pPicBuffer->Data.YCbCr.planar.Y.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel),
                    HAL_MAPMEM_WRITEONLY, &pMapBufferY ) ;
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        MEMCPY( pMapBufferY, pSimPicBuffer->Data.YCbCr.planar.Y.pBuffer,
                    (pPicBuffer->Data.YCbCr.planar.Y.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Y.PicHeightPixel) );

        result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBufferY );
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        /* Cb - Plane */
        result = HalMapMemory( pCamEngineCtx->hHal,
                    (unsigned long)( pPicBuffer->Data.YCbCr.planar.Cb.pBuffer ),
                    (pPicBuffer->Data.YCbCr.planar.Cb.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel),
                    HAL_MAPMEM_WRITEONLY, &pMapBufferCb ) ;
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        MEMCPY( pMapBufferCb, pSimPicBuffer->Data.YCbCr.planar.Cb.pBuffer,
                    (pPicBuffer->Data.YCbCr.planar.Cb.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Cb.PicHeightPixel) );

        result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBufferCb );
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        /* Cr - Plane */
        result = HalMapMemory( pCamEngineCtx->hHal,
                    (unsigned long)( pPicBuffer->Data.YCbCr.planar.Cr.pBuffer ),
                    (pPicBuffer->Data.YCbCr.planar.Cr.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Cr.PicHeightPixel),
                    HAL_MAPMEM_WRITEONLY, &pMapBufferCr ) ;
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        MEMCPY( pMapBufferCr, pSimPicBuffer->Data.YCbCr.planar.Cr.pBuffer,
                    (pPicBuffer->Data.YCbCr.planar.Cr.PicWidthBytes * pPicBuffer->Data.YCbCr.planar.Cr.PicHeightPixel) );

        result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBufferCr );
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }
    }
    else if ( pSimPicBuffer->Layout == PIC_BUF_LAYOUT_SEMIPLANAR )
    {
        void *pMapBufferY;
        void *pMapBufferCbCr;
        void *pMapBufferCr;

        pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer  = (uint8_t *)pMediaBuffer->pBaseAddress;
        pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer = (uint8_t *)( pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer
                + ((pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel)) );

        /* Y - Plane */
        result = HalMapMemory( pCamEngineCtx->hHal,
                    (unsigned long)( pPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer ),
                    (pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel),
                    HAL_MAPMEM_WRITEONLY, &pMapBufferY ) ;
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        MEMCPY( pMapBufferY, pSimPicBuffer->Data.YCbCr.semiplanar.Y.pBuffer,
                    (pPicBuffer->Data.YCbCr.semiplanar.Y.PicWidthBytes * pPicBuffer->Data.YCbCr.semiplanar.Y.PicHeightPixel) );

        result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBufferY );
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        /* CbCr - Plane */
        result = HalMapMemory( pCamEngineCtx->hHal,
                    (unsigned long)( pPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer ),
                    (pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthBytes * pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicHeightPixel),
                    HAL_MAPMEM_WRITEONLY, &pMapBufferCbCr ) ;
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        MEMCPY( pMapBufferCbCr, pSimPicBuffer->Data.YCbCr.semiplanar.CbCr.pBuffer,
                    (pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicWidthBytes * pPicBuffer->Data.YCbCr.semiplanar.CbCr.PicHeightPixel) );

        result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBufferCbCr );
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }
    }
    else
    {
        void *pMapBuffer;

        pPicBuffer->Data.YCbCr.combined.pBuffer = (uint8_t *)pMediaBuffer->pBaseAddress;

        /* 1.) get a free write-only memory chunk from HAL */
        result = HalMapMemory( pCamEngineCtx->hHal,
                    (unsigned long)( pPicBuffer->Data.YCbCr.combined.pBuffer ),
                    (pPicBuffer->Data.YCbCr.combined.PicWidthBytes * pPicBuffer->Data.YCbCr.combined.PicHeightPixel),
                    HAL_MAPMEM_WRITEONLY, &pMapBuffer ) ;
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }

        /* 2.) copy data to mapped memory chunk */
        MEMCPY( pMapBuffer, pSimPicBuffer->Data.YCbCr.combined.pBuffer,
                    (pPicBuffer->Data.YCbCr.combined.PicWidthBytes * pPicBuffer->Data.YCbCr.combined.PicHeightPixel) );

        /* 3.) initiate DMA transfer */
        result = HalUnMapMemory( pCamEngineCtx->hHal, pMapBuffer );
        if ( result != RET_SUCCESS )
        {
            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pMediaBuffer );
            return ( result );
        }
    }

    pCamEngineCtx->pDmaBuffer = pPicBuffer;
    pCamEngineCtx->pDmaMediaBuffer = pMediaBuffer;

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}



/******************************************************************************
 * SimpCompletionCb()
 *****************************************************************************/
void SimpCompletionCb
(
    const CamerIcCommandId_t    cmdId,
    const RESULT                result,
    void                        *pParam,
    void                        *pUserContext
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)pUserContext;

    TRACE( CAM_ENGINE_API_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (pCamEngineCtx != NULL) && (pParam != NULL) )
    {
        switch ( cmdId )
        {
            case CAMERIC_MI_COMMAND_DMA_TRANSFER:
                {
                    RESULT result;
                    bool_t enabled;

                    result = CamerIcSimpIsEnabled( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, &enabled );
                    DCT_ASSERT( (result == RET_SUCCESS) );
                    if ( enabled == BOOL_TRUE )
                    {
                        result = CamerIcDriverLoadPicture( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, pCamEngineCtx->pDmaBuffer, &CompletionCb, BOOL_TRUE );
                        DCT_ASSERT( (result == RET_PENDING) );
                    }
                    else
                    {
                        if ( pCamEngineCtx->pDmaMediaBuffer != NULL )
                        {
                            MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pCamEngineCtx->pDmaMediaBuffer );
                            pCamEngineCtx->pDmaMediaBuffer  = NULL;
                            pCamEngineCtx->pDmaBuffer       = NULL;
                        }
                    }

                    break;
                }

            default:
                {
                     TRACE( CAM_ENGINE_API_WARN, "%s: (unsupported command)\n", __FUNCTION__ );
                     break;
                }
        }
    }

    TRACE( CAM_ENGINE_API_INFO, "%s: (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * CamEngineEnableSimp()
 *****************************************************************************/
RESULT CamEngineEnableSimp
(
    CamEngineHandle_t       hCamEngine,
    CamEngineSimpConfig_t   *pConfig
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    CamerIcDmaReadPath_t DmaRead = CAMERIC_DMA_READ_INVALID;

    RESULT result = RET_SUCCESS;

    CamerIcSimpConfig_t config;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING )
    {
        return ( RET_WRONG_STATE );
    }

    if ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode )
    {
        return ( RET_NOTSUPP );
    }

    /* check configuration */
    if ( pConfig->pPicBuffer == NULL )
    {
        TRACE( CAM_ENGINE_API_ERROR, "%s no picture data \n", __FUNCTION__ );
        return ( RET_WRONG_CONFIG );
    }

    /* check configuration - picture buffer type (YUV422 expected)*/
    if ( (pConfig->pPicBuffer->Type != PIC_BUF_TYPE_YCbCr422)
            || ( (pConfig->pPicBuffer->Layout != PIC_BUF_LAYOUT_PLANAR)
                    && (pConfig->pPicBuffer->Layout != PIC_BUF_LAYOUT_SEMIPLANAR)
                    && (pConfig->pPicBuffer->Layout != PIC_BUF_LAYOUT_COMBINED) ) )
    {
        return ( RET_WRONG_CONFIG );
    }

    /* check configuration */
    switch ( pConfig->Mode )
    {
        case CAM_ENGINE_SIMP_MODE_OVERLAY:
            {
                config.TransparencyMode = CAMERIC_SIMP_TRANSPARENCY_MODE_DISABLED;
                config.ReferenceMode    = CAMERIC_SIMP_REFERENCE_MODE_FROM_CAMERA;
                config.OffsetX          = pConfig->SimpModeConfig.Overlay.OffsetX;
                config.OffsetY          = pConfig->SimpModeConfig.Overlay.OffsetY;
                config.Y                = 0U;
                config.Cb               = 0U;
                config.Cr               = 0U;
                break;
            }

        case CAM_ENGINE_SIMP_MODE_KEYCOLORING:
            {
                config.TransparencyMode = CAMERIC_SIMP_TRANSPARENCY_MODE_ENABLED;
                config.ReferenceMode    = CAMERIC_SIMP_REFERENCE_MODE_FROM_MEMORY;
                config.OffsetX          = 0U;
                config.OffsetY          = 0U;
                config.Y                = pConfig->SimpModeConfig.KeyColoring.Y;
                config.Cb               = pConfig->SimpModeConfig.KeyColoring.Cb;
                config.Cr               = pConfig->SimpModeConfig.KeyColoring.Cr;
                break;
            }

        default:
            {
                TRACE( CAM_ENGINE_API_ERROR,
                        "%s unsupported super impose mode (%d)\n", __FUNCTION__, pConfig->Mode );
                return ( RET_NOTSUPP );
            }
    }

    result = CamEnginePreloadImageSimp( pCamEngineCtx, pConfig->pPicBuffer );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamerIcDriverGetViDmaSwitch( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, &DmaRead );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    if ( DmaRead != CAMERIC_DMA_READ_SUPERIMPOSE )
    {
        result = CamerIcDriverSetViDmaSwitch( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, CAMERIC_DMA_READ_SUPERIMPOSE );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
    }

    CompletionCb.func           = SimpCompletionCb;
    CompletionCb.pUserContext   = pCamEngineCtx;
    CompletionCb.pParam         = (void *)pCamEngineCtx->pDmaBuffer;
    result = CamerIcDriverLoadPicture( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, pCamEngineCtx->pDmaBuffer, &CompletionCb, BOOL_TRUE );

    result = CamerIcSimpConfigure( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, &config );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamerIcSimpEnable( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineDisableSimp()
 *****************************************************************************/
RESULT CamEngineDisableSimp
(
    CamEngineHandle_t   hCamEngine
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( pCamEngineCtx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check if cam-engine running */
    if ( (CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_STREAMING) && 
         (CamEngineGetState( pCamEngineCtx ) != CAM_ENGINE_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamerIcSimpDisable( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc );

    if ( pCamEngineCtx->pDmaMediaBuffer != NULL )
    {
        MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pCamEngineCtx->pDmaMediaBuffer );
        pCamEngineCtx->pDmaMediaBuffer  = NULL;
        pCamEngineCtx->pDmaBuffer       = NULL;
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}

