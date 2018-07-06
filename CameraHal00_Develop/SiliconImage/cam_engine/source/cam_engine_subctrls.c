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
 * @file cam_engine_subctrls.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include <mom_ctrl/mom_ctrl_api.h>
#include <mim_ctrl/mim_ctrl_api.h>
#include <bufsync_ctrl/bufsync_ctrl_api.h>

#include "cam_engine_subctrls.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

USE_TRACER( CAM_ENGINE_INFO  );
USE_TRACER( CAM_ENGINE_WARN  );
USE_TRACER( CAM_ENGINE_ERROR );
USE_TRACER( CAM_ENGINE_DEBUG );

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
 * CamEngineDmaRestartCb()
 *****************************************************************************/
static void CamEngineDmaRestartCb
(
    int32_t             path,
    MediaBuffer_t       *pMediaBuffer,
    void                *pBufferCbCtx
)
{
    CamEngineContext_t  *pCamEngineCtx = (CamEngineContext_t *)pBufferCbCtx;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( ( NULL == pMediaBuffer ) ||
         ( NULL == pMediaBuffer->pOwner ) )
    {
        return;
    }

    if ( ( NULL != pCamEngineCtx ) && 
         ( CAM_ENGINE_PATH_MAIN == path ) )
    {
        OSLAYER_STATUS osStatus;

        MediaBufLockBuffer( pMediaBuffer );

        ulong_t frameId = (ulong_t)&(pMediaBuffer);
        TRACE( CAM_ENGINE_INFO, "%s: (put buffer (id=0x%08x)\n", __FUNCTION__, frameId );

        // store buffer in mainpath-queue of master-pipe
        osStatus = osQueueWrite( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue, &pMediaBuffer );
        DCT_ASSERT( osStatus == OSLAYER_OK );
        if ( OSLAYER_OK != osStatus )
        {
            MediaBufUnlockBuffer( pMediaBuffer );
        }

        // send a CAM_ENGINE_CMD_HW_DMA_FINISHED to trigger an initial dma-start
        //if ( BOOL_TRUE == pCamEngineCtx->startDMA )
        {
            CamEngineCmd_t command;
            RESULT result;
            
            TRACE( CAM_ENGINE_INFO, "%s send command\n", __FUNCTION__ );

            // build dam finished command
            MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
            command.cmdId = CAM_ENGINE_CMD_HW_DMA_FINISHED;

            // send command to cam-engine thread
            result = CamEngineSendCommand( pCamEngineCtx, &command );
            DCT_ASSERT( result == RET_SUCCESS );

            // disable dma-start for next recieved frame, camerIc-hardware does it. 
            pCamEngineCtx->startDMA = BOOL_FALSE;
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );
}



/******************************************************************************
 * CamEngineCreatePictureBufferPool()
 *****************************************************************************/
static RESULT CamEngineCreatePictureBufferPool
(
    CamEngineContext_t  *pCamEngineCtx,
    ChainCtx_t          *pChain,
    const uint32_t      bufNumMainPath,
    const uint32_t      bufSizeMainPath,
    const uint32_t      bufNumSelfPath,
    const uint32_t      bufSizeSelfPath
)
{
    uint32_t i;
    RESULT result = RET_SUCCESS;

    ulong_t bufBaseMainPath = 0UL;
    ulong_t bufBaseSelfPath = 0UL;


    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );


    /******************************************************************************
     * 1.) Main - Path
     *****************************************************************************/
    if ( bufSizeMainPath && bufNumMainPath )
    {
        // 1.) allocate HAL memory (FPGA RAM)
        //modify by zyc
        bufBaseMainPath = (ulong_t)malloc(sizeof(ulong_t)*bufNumMainPath);
        if(!bufBaseMainPath){
            TRACE( CAM_ENGINE_INFO, "failed to alloc bufBaseMainPath\n", __FUNCTION__ );
            return ( RET_FAILURE );

        }
        memset((void*)bufBaseMainPath,0,sizeof(ulong_t)*bufNumMainPath);

        for(i = 0U; i < bufNumMainPath; i++)
        {
            *((ulong_t*)bufBaseMainPath + i) = HalAllocMemory( pCamEngineCtx->hHal, bufSizeMainPath );
            if(*((ulong_t*)bufBaseMainPath + i) == RET_NULL_POINTER)
                break;
        }
        
        if(i < bufNumMainPath){
            TRACE( CAM_ENGINE_INFO, "failed to alloc bufBaseMainPath from ion\n", __FUNCTION__ );
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseMainPath + (i-1)) );
                i--;
            }
            free((void*)bufBaseMainPath);
            return ( RET_FAILURE );
        }

        // 2.) set configuration of buffer-pool
        pChain->BufPoolConfigMain.bufNum                = bufNumMainPath;
        pChain->BufPoolConfigMain.maxBufNum             = bufNumMainPath;
        pChain->BufPoolConfigMain.bufSize               = bufSizeMainPath;
        pChain->BufPoolConfigMain.bufAlign              = PIC_BUFFER_ALIGN;            /* 1K alignment */
        pChain->BufPoolConfigMain.metaDataSizeMediaBuf  = sizeof( PicBufMetaData_t );
        pChain->BufPoolConfigMain.flags                 = 0;

        // 3.) determine buffer pool memory requirements
        if ( RET_SUCCESS != MediaBufPoolGetSize( &(pChain->BufPoolConfigMain) ) )
        {
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseMainPath + (i-1)) );
                i--;
            }
            free((void*)bufBaseMainPath);
            return ( RET_FAILURE );
        }

        // 4.) allocate buffer pool memory (meta data + data) for main path
        pChain->BufPoolMemMain.pMetaDataMemory = malloc( pChain->BufPoolConfigMain.metaDataMemSize );
        pChain->BufPoolMemMain.pBufferMemory   = (void *)bufBaseMainPath;

        // 5.) create buffer pool instance
        if ( RET_SUCCESS !=  MediaBufPoolCreate( &pChain->BufPoolMain, &pChain->BufPoolConfigMain, pChain->BufPoolMemMain ) )
        {
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseMainPath + (i-1)) );
                i--;
            }
            free((void*)bufBaseMainPath);
            return ( RET_FAILURE );
        }
    }

#if 0
    //self path is not used now ,so needn't create self patch buffer ,zyc
    /******************************************************************************
     * Self - Path
     *****************************************************************************/
    if ( bufSizeSelfPath && bufNumSelfPath )
    {
        // 1.)  allocate HAL memory (FPGA RAM)
        //modify by zyc

        bufBaseSelfPath = (uint32_t)malloc(sizeof(uint32_t)*bufNumSelfPath);
        if(!bufBaseSelfPath){
            TRACE( CAM_ENGINE_INFO, "failed to alloc bufBaseSelfPath\n", __FUNCTION__ );
            return ( RET_FAILURE );

        }
        memset((void*)bufBaseSelfPath,0,sizeof(uint32_t)*bufNumSelfPath);

        for(i = 0U; i < bufNumSelfPath; i++)
        {
            *((uint32_t*)bufBaseSelfPath + i) = HalAllocMemory( pCamEngineCtx->hHal, bufSizeSelfPath );
            if(*((uint32_t*)bufBaseSelfPath + i) == RET_NULL_POINTER)
                break;
        }
        
        if(i < bufNumSelfPath){
            TRACE( CAM_ENGINE_INFO, "failed to alloc bufBaseSelfPath from ion\n", __FUNCTION__ );
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((uint32_t*)bufBaseSelfPath + (i-1)) );
                i--;
            }
            free((void*)bufBaseMainPath);
            return ( RET_FAILURE );
        }

        // 2.) set configuration of buffer-pool
        pChain->BufPoolConfigSelf.bufNum                = bufNumSelfPath;
        pChain->BufPoolConfigSelf.maxBufNum             = bufNumSelfPath;
        pChain->BufPoolConfigSelf.bufSize               = bufSizeSelfPath;
        pChain->BufPoolConfigSelf.bufAlign              = PIC_BUFFER_ALIGN;            /* 1K alignment */
        pChain->BufPoolConfigSelf.metaDataSizeMediaBuf  = sizeof( PicBufMetaData_t );
        pChain->BufPoolConfigSelf.flags                 = 0;

        // 3.) determine buffer pool memory requirements
        if ( RET_SUCCESS != MediaBufPoolGetSize(  &(pChain->BufPoolConfigSelf) ) )
        {
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((uint32_t*)bufBaseSelfPath + (i-1)) );
                i--;
            }
            free((void*)bufBaseSelfPath);
            return ( RET_FAILURE );
        }

        // 4.) allocate buffer pool memory (meta data + data) for main path
        pChain->BufPoolMemSelf.pMetaDataMemory = malloc( pChain->BufPoolConfigSelf.metaDataMemSize );
        pChain->BufPoolMemSelf.pBufferMemory   = (void *)bufBaseSelfPath;

        // 5.) create buffer pool instance
        if ( RET_SUCCESS !=  MediaBufPoolCreate( &pChain->BufPoolSelf, &pChain->BufPoolConfigSelf, pChain->BufPoolMemSelf ) )
        {
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((uint32_t*)bufBaseSelfPath + (i-1)) );
                i--;
            }
            free((void*)bufBaseSelfPath);

            return ( RET_FAILURE );
        }
    }
#endif

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineDestroyPictureBufferPool()
 *****************************************************************************/
static RESULT CamEngineDestroyPictureBufferPool
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;    
    uint32_t        i;


    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    /******************************************************************************
     * Self - Path
     *****************************************************************************/

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pMetaDataMemory )
    {
        /* 1.)  destroy self path buffer pool instance */
        if ( RET_SUCCESS != MediaBufPoolDestroy ( &(pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolSelf) ) )
        {
            return ( RET_FAILURE );
        }

        /* 2.) free buffer pool memory (meta data + data) */
        free( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pMetaDataMemory );
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pMetaDataMemory = NULL;
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pBufferMemory )
    {
        //HalFreeMemory( pCamEngineCtx->hHal, (uint32_t)pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pBufferMemory );
        //modify by zyc
        ulong_t bufBaseSelfPath = (ulong_t)pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pBufferMemory;
        i = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolConfigSelf.maxBufNum;
        while(i > 0){
            HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseSelfPath + (i-1)) );
            i--;
        }
        free((void*)bufBaseSelfPath);

        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemSelf.pBufferMemory   = NULL;
    }


    /******************************************************************************
     * Main - Path
     *****************************************************************************/

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pMetaDataMemory )
    {
        /* 1.)  destroy main path buffer pool instance */
        if ( RET_SUCCESS != MediaBufPoolDestroy ( &(pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMain) ) )
        {
            return ( RET_SUCCESS );
        }

        /* 2.) free buffer pool memory (meta data + data) */
        free( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pMetaDataMemory );

        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pMetaDataMemory = NULL;
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pBufferMemory )
    {
       // HalFreeMemory( pCamEngineCtx->hHal, (uint32_t)pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pBufferMemory );

        //modify by zyc

        ulong_t bufBaseMainPath = (ulong_t)pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pBufferMemory;
        i = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolConfigMain.maxBufNum;
        while(i > 0){
            HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseMainPath + (i-1)) );
            i--;
        }
        free((void*)bufBaseMainPath);
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMemMain.pBufferMemory   = NULL;
    }


    /******************************************************************************
     * Self - Path
     *****************************************************************************/

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemSelf.pMetaDataMemory )
    {
        /* 1.)  destroy self path buffer pool instance */
        if ( RET_SUCCESS != MediaBufPoolDestroy ( &(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolSelf) ) )
        {
            return ( RET_FAILURE );
        }

        /* 2.) free buffer pool memory (meta data + data) */
        free( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemSelf.pMetaDataMemory );
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemSelf.pMetaDataMemory = NULL;
    }


    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemSelf.pBufferMemory )
    {
        ulong_t bufBaseSelfPath = (ulong_t)pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemSelf.pBufferMemory;
        i = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolConfigSelf.maxBufNum;
        while(i > 0){
            HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseSelfPath + (i-1)) );
            i--;
        }
        free((void*)bufBaseSelfPath);
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemSelf.pBufferMemory   = NULL;
    }


    /******************************************************************************
     * Main - Path
     *****************************************************************************/

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemMain.pMetaDataMemory )
    {
        /* 1.)  destroy main path buffer pool instance */
        if ( RET_SUCCESS != MediaBufPoolDestroy ( &(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMain) ) )
        {
            return ( RET_SUCCESS );
        }

        /* 2.) free buffer pool memory (meta data + data) */
        free( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemMain.pMetaDataMemory );
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemMain.pMetaDataMemory = NULL;
    }


    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemMain.pBufferMemory )
    {
        
        ulong_t bufBaseMainPath = (ulong_t)pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemMain.pBufferMemory;
        i = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolConfigMain.maxBufNum;
        while(i > 0){
            HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseMainPath + (i-1)) );
            i--;
        }
        free((void*)bufBaseMainPath);
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMemMain.pBufferMemory   = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineCreateInputPictureBufferPool()
 *****************************************************************************/
static RESULT CamEngineCreateInputPictureBufferPool
(
    CamEngineContext_t  *pCamEngineCtx,
    const uint32_t      bufNumImg,
    const uint32_t      bufSizeImg
)
{
    RESULT result = RET_SUCCESS;
    ulong_t bufBaseInputPic = 0UL;
    uint32_t i = 0;


    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( bufSizeImg && bufNumImg )
    {
        /* 1.) set configuration of buffer-pool */
        pCamEngineCtx->BufferPoolConfigInput.bufNum                = bufNumImg;
        pCamEngineCtx->BufferPoolConfigInput.maxBufNum             = bufNumImg;
        pCamEngineCtx->BufferPoolConfigInput.bufSize               = bufSizeImg;
        pCamEngineCtx->BufferPoolConfigInput.bufAlign              = PIC_BUFFER_ALIGN;            /* 1K alignment */
        pCamEngineCtx->BufferPoolConfigInput.metaDataSizeMediaBuf  = sizeof( PicBufMetaData_t );
        pCamEngineCtx->BufferPoolConfigInput.flags                 = BUFPOOL_RINGBUFFER;


        bufBaseInputPic = (ulong_t)malloc(sizeof(ulong_t)*bufNumImg);
        if(!bufBaseInputPic){
            TRACE( CAM_ENGINE_INFO, "failed to alloc bufBaseInputPic\n", __FUNCTION__ );
            return ( RET_FAILURE );

        }
        memset((void*)bufBaseInputPic,0,sizeof(ulong_t)*bufNumImg);

        for(i = 0U; i < bufNumImg; i++)
        {
            *((ulong_t*)bufBaseInputPic + i) = HalAllocMemory( pCamEngineCtx->hHal, bufSizeImg );
            if(*((ulong_t*)bufBaseInputPic + i) == RET_NULL_POINTER)
                break;
        }
        
        if(i < bufNumImg){
            TRACE( CAM_ENGINE_INFO, "failed to alloc bufBaseSelfPath from ion\n", __FUNCTION__ );
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseInputPic + (i-1)) );
                i--;
            }
            free((void*)bufBaseInputPic);
            return ( RET_FAILURE );
        }

        /* 2.) determine buffer pool memory requirements */
        if ( RET_SUCCESS != MediaBufPoolGetSize( &pCamEngineCtx->BufferPoolConfigInput ) )
        {
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseInputPic + (i-1)) );
                i--;
            }
            free((void*)bufBaseInputPic);
            return ( RET_FAILURE );
        }

        /* 3.) allocate buffer pool memory (meta data + data) for main path */
        pCamEngineCtx->BufferPoolMemInput.pMetaDataMemory = malloc( pCamEngineCtx->BufferPoolConfigInput.metaDataMemSize );

        
        pCamEngineCtx->BufferPoolMemInput.pBufferMemory   = (void *)bufBaseInputPic;

        /* 4.) create buffer pool instance */
        if ( RET_SUCCESS !=  MediaBufPoolCreate( &pCamEngineCtx->BufferPoolInput,
                                                 &pCamEngineCtx->BufferPoolConfigInput,
                                                  pCamEngineCtx->BufferPoolMemInput ) )
        {
            while(i > 0){
                HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseInputPic + (i-1)) );
                i--;
            }
            free((void*)bufBaseInputPic);
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineDestroyInputPictureBufferPool()
 *****************************************************************************/
static RESULT CamEngineDestroyInputPictureBufferPool
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__ );

    /* free existing buffer */
    if ( pCamEngineCtx->pDmaMediaBuffer != NULL )
    {
        MediaBufPoolFreeBuffer( &pCamEngineCtx->BufferPoolInput, pCamEngineCtx->pDmaMediaBuffer );
        pCamEngineCtx->pDmaMediaBuffer = NULL;
        pCamEngineCtx->pDmaBuffer = NULL;
    }

    if ( NULL != pCamEngineCtx->BufferPoolMemInput.pMetaDataMemory )
    {
        /* 1.)  destroy dma input buffer pool instance */
        if ( RET_SUCCESS != MediaBufPoolDestroy ( &(pCamEngineCtx->BufferPoolInput) ) )
        {
            return ( RET_FAILURE );
        }

        /* 2.) free buffer pool memory (meta data + data) */
        free( pCamEngineCtx->BufferPoolMemInput.pMetaDataMemory );
        pCamEngineCtx->BufferPoolMemInput.pMetaDataMemory = NULL;
    }

    if ( NULL != pCamEngineCtx->BufferPoolMemInput.pBufferMemory )
    {
        //HalFreeMemory( pCamEngineCtx->hHal, (uint32_t)pCamEngineCtx->BufferPoolMemInput.pBufferMemory );
        ulong_t bufBaseInputPic = (ulong_t)pCamEngineCtx->BufferPoolMemInput.pBufferMemory;
        uint32_t i = pCamEngineCtx->BufferPoolConfigInput.bufNum;
        while(i > 0){
            HalFreeMemory( pCamEngineCtx->hHal, *((ulong_t*)bufBaseInputPic + i) );
            i--;
        }
        free((void*)bufBaseInputPic);
        pCamEngineCtx->BufferPoolMemInput.pBufferMemory   = NULL;
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 *
 *          CamEngineMimCtrlCb
 *
 * @brief   MIM-Control completion callback
 *
 *****************************************************************************/
static void CamEngineMimCtrlCb
(
    MimCtrlCmdId_t  CmdId,
    RESULT          result,
    const void      *pUserContext
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)pUserContext;

    TRACE( CAM_ENGINE_INFO, "%s (enter cmd=%u, res=%d)\n", __FUNCTION__, CmdId, result );

    if ( pCamEngineCtx != NULL )
    {
        switch ( CmdId )
        {
            case MIM_CTRL_CMD_START:
                {
                    TRACE( CAM_ENGINE_INFO, "%s: EventCmdStart (RESULT=%d)\n", __FUNCTION__, result );
                    (void)osEventSignal( &pCamEngineCtx->MimEventCmdStart );
                    break;
                }

            case MIM_CTRL_CMD_STOP:
                {
                    TRACE( CAM_ENGINE_INFO, "%s: EventCmdStop (RESULT=%d)\n", __FUNCTION__, result);
                    (void)osEventSignal( &pCamEngineCtx->MimEventCmdStop );
                    break;
                }

            case MIM_CTRL_CMD_DMA_TRANSFER:
                {
                    CamEngineCmd_t command;

                    TRACE( CAM_ENGINE_INFO, "%s: EventCmdDma (RESULT=%d)\n", __FUNCTION__, result );

                    // unlock the dma-buffer
                    if ( ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) &&
                         ( NULL != pCamEngineCtx->pDmaMediaBuffer ) )
                    {
                        MediaBufUnlockBuffer( pCamEngineCtx->pDmaMediaBuffer );
                        pCamEngineCtx->pDmaMediaBuffer = NULL;
                        pCamEngineCtx->pDmaBuffer      = NULL;
                    }

                    // build dma finished comand
                    MEMSET( &command, 0, sizeof( CamEngineCmd_t ) );
                    command.cmdId = CAM_ENGINE_CMD_HW_DMA_FINISHED;

                    // send command to cam-engine thread via command-queue
                    result = CamEngineSendCommand( pCamEngineCtx, &command );
                    DCT_ASSERT( result == RET_SUCCESS );

                    break;
                }

            default:
                {
                    TRACE( CAM_ENGINE_WARN, "%s: invalid event (%d)\n", __FUNCTION__, CmdId );
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );
}


/******************************************************************************
 *
 *          CamEngineMimCtrlSetup
 *
 * @brief   Create and initialize a MIM-Control instance
 *
 *****************************************************************************/
static RESULT CamEngineMimCtrlSetup
(
    CamEngineContext_t  *pCamEngineCtx,
    const uint32_t      bufNum
)
{
    RESULT result = RET_SUCCESS;
    
    MimCtrlConfig_t MimCtrlConfig;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( NULL == pCamEngineCtx->hMimCtrl );

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->MimEventCmdStart, 1, 0 ) )
    {
        return ( RET_FAILURE );
    }

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->MimEventCmdStop, 1, 0 ) )
    {
        (void)osEventDestroy( &pCamEngineCtx->MimEventCmdStart );
        return ( RET_FAILURE );
    }

    MimCtrlConfig.MaxPendingCommands   = bufNum;
    MimCtrlConfig.mimCbCompletion      = CamEngineMimCtrlCb;
    MimCtrlConfig.pUserContext         = pCamEngineCtx;
    MimCtrlConfig.hMimContext          = NULL;
    if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
    {
        MimCtrlConfig.hCamerIc         = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc; 
    }
    else
    {
        MimCtrlConfig.hCamerIc         = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    }
    result = MimCtrlInit( &MimCtrlConfig );
    if ( result != RET_SUCCESS )
    {
        (void)osEventDestroy( &pCamEngineCtx->MimEventCmdStart );
        (void)osEventDestroy( &pCamEngineCtx->MimEventCmdStop );
        return ( result );
    }
    pCamEngineCtx->hMimCtrl = MimCtrlConfig.hMimContext;

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 *
 *          CamEngineMimCtrlRelease
 *
 * @brief   Start a MIM-Control instance
 *
 *****************************************************************************/
static RESULT CamEngineMimCtrlStart
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    if ( ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        result = MimCtrlStart( pCamEngineCtx->hMimCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->MimEventCmdStart ) )
        {
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 *
 *          CamEngineMimCtrlStop
 *
 * @brief   Stop a MIM-Control instance
 *
 *****************************************************************************/
static RESULT CamEngineMimCtrlStop
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    if ( ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        result = MimCtrlStop( pCamEngineCtx->hMimCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->MimEventCmdStop ) )
        {
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 *
 *          CamEngineMimCtrlRelease
 *
 * @brief   Release a MIM-Control instance
 *
 *****************************************************************************/
static RESULT CamEngineMimCtrlRelease
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    if ( NULL != pCamEngineCtx->hMimCtrl )
    {
        result = MimCtrlShutDown( pCamEngineCtx->hMimCtrl );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        pCamEngineCtx->hMimCtrl = NULL;

        (void)osEventDestroy( &pCamEngineCtx->MimEventCmdStart );
        (void)osEventDestroy( &pCamEngineCtx->MimEventCmdStop );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 *
 *          BufSyncCtrlCb
 *
 * @brief   BUFSYNC-Control completion callback
 *
 *****************************************************************************/
static void BufSyncCtrlCb
(
    BufSyncCtrlCmdId_t  CmdId,
    RESULT              result,
    const void*         pUserContext
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)pUserContext;

    TRACE( CAM_ENGINE_INFO, "%s (enter cmd=%u, res=%d)\n", __FUNCTION__, CmdId, result );

    if ( pCamEngineCtx != NULL )
    {

        switch ( CmdId )
        {
            case BUFSYNC_CTRL_CMD_START:
                {
                    (void)osEventSignal( &pCamEngineCtx->BufSyncEventStart );
                    break;
                }

            case BUFSYNC_CTRL_CMD_STOP:
                {
                    (void)osEventSignal( &pCamEngineCtx->BufSyncEventStop );
                    break;
                }

            default:
                {
                    TRACE( CAM_ENGINE_WARN, "%s: unknown command #%d (RESULT=%d)\n", __FUNCTION__, (uint32_t)CmdId, result);
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );
}


/******************************************************************************
 * CamEngineBufSyncCtrlSetup()
 ******************************************************************************/
RESULT CamEngineBufSyncCtrlSetup
(
    CamEngineContext_t *pCamEngineCtx,
    const uint32_t      bufNumMainPath,
    const uint32_t      bufNumSelfPath
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s: (enter)\n", __FUNCTION__ );

    DCT_ASSERT( NULL == pCamEngineCtx->hBufSyncCtrl );

    BufSyncCtrlConfig_t BufSyncCtrlConfig;

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->BufSyncEventStart, 1, 0 ) )
    {
        return ( RET_FAILURE );
    }

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->BufSyncEventStop, 1, 0 ) )
    {
        (void)osEventDestroy(  &pCamEngineCtx->BufSyncEventStart );
        return ( RET_FAILURE );
    }

    MEMSET( &BufSyncCtrlConfig, 0, sizeof( BufSyncCtrlConfig ) );
    BufSyncCtrlConfig.MaxPendingCommands   = 2*bufNumMainPath;
    BufSyncCtrlConfig.bufsyncCbCompletion  = BufSyncCtrlCb;
    BufSyncCtrlConfig.pUserContext         = (void *)pCamEngineCtx;
    BufSyncCtrlConfig.pPicBufQueue1        = &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue;
    BufSyncCtrlConfig.pPicBufQueue2        = &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MainPathQueue;
    result = BufSyncCtrlInit( &BufSyncCtrlConfig );
    if ( result != RET_SUCCESS )
    {
        (void)osEventDestroy( &pCamEngineCtx->BufSyncEventStop );
        (void)osEventDestroy( &pCamEngineCtx->BufSyncEventStart );
        return ( result );
    }
    pCamEngineCtx->hBufSyncCtrl = BufSyncCtrlConfig.hBufSyncCtrl;

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * CamEngineBufSyncCtrlStart()
 *****************************************************************************/
static RESULT CamEngineBufSyncCtrlStart
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = BufSyncCtrlStart( pCamEngineCtx->hBufSyncCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->BufSyncEventStart ) )
        {
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineBufSyncCtrlStop()
 *****************************************************************************/
static RESULT CamEngineBufSyncCtrlStop
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = BufSyncCtrlStop( pCamEngineCtx->hBufSyncCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->BufSyncEventStop ) )
        {
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineBufSyncCtrlRelease()
 *****************************************************************************/
static RESULT CamEngineBufSyncCtrlRelease
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    if ( NULL != pCamEngineCtx->hBufSyncCtrl )
    {
        result = BufSyncCtrlShutDown( pCamEngineCtx->hBufSyncCtrl );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        pCamEngineCtx->hBufSyncCtrl = NULL;

        (void)osEventDestroy( &pCamEngineCtx->BufSyncEventStart );
        (void)osEventDestroy( &pCamEngineCtx->BufSyncEventStop );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 *
 *          CamEngineMomCtrlCb
 *
 * @brief   MOM-Control completion callback
 *
 *****************************************************************************/
static void CamEngineMomCtrlCb
(
    MomCtrlCmdId_t      CmdId,
    RESULT              result,
    const void*         pUserContext
)
{
    ChainCtx_t *pChainCtx = (ChainCtx_t *)pUserContext;

    TRACE( CAM_ENGINE_INFO, "%s (enter cmd=%u, res=%d)\n", __FUNCTION__, CmdId, result );

    if ( pChainCtx != NULL )
    {

        switch ( CmdId )
        {
            case MOM_CTRL_CMD_START:
                {
                    (void)osEventSignal( &pChainCtx->MomEventCmdStart );
                    break;
                }

            case MOM_CTRL_CMD_STOP:
                {
                    (void)osEventSignal( &pChainCtx->MomEventCmdStop );
                    break;
                }

            default:
                {
                    TRACE( CAM_ENGINE_WARN, "%s: unknown command #%d (RESULT=%d)\n", __FUNCTION__, (uint32_t)CmdId, result);
                    break;
                }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );
}


/******************************************************************************
 * CamEngineMomCtrlSetup()
 *****************************************************************************/
static RESULT CamEngineMomCtrlSetup
(
    CamEngineContext_t  *pCamEngineCtx,
    const uint32_t      bufNumMainPath,
    const uint32_t      bufNumSelfPath
)
{
    RESULT result = RET_SUCCESS;

    MomCtrlConfig_t MomCtrlConfig;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
    DCT_ASSERT( NULL == pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart, 1, 0 ) )
    {
        return ( RET_FAILURE );
    }

    if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop, 1, 0 ) )
    {
        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
        return ( RET_FAILURE );
    }

    MomCtrlConfig.MaxPendingCommands   = bufNumMainPath + bufNumSelfPath;
    MomCtrlConfig.NumBuffersMainPath   = bufNumMainPath;
    MomCtrlConfig.NumBuffersSelfPath   = bufNumSelfPath;
    MomCtrlConfig.pPicBufPoolMainPath  = &(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolMain);
    MomCtrlConfig.pPicBufPoolSelfPath  = &(pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].BufPoolSelf);
    MomCtrlConfig.momCbCompletion      = CamEngineMomCtrlCb;
    MomCtrlConfig.pUserContext         = &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0];
    MomCtrlConfig.hCamerIc             = pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hCamerIc;
    MomCtrlConfig.HalHandle            = pCamEngineCtx->hHal;
    MomCtrlConfig.hMomContext          = NULL;

    result = MomCtrlInit( &MomCtrlConfig );
    if ( result != RET_SUCCESS )
    {
        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
        return ( result );
    }
    pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = MomCtrlConfig.hMomContext;

    /* do we have a second pipeline ? */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart, 1, 0 ) )
        {
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( RET_FAILURE );
        }

        if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStop, 1, 0 ) )
        {
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart );
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( RET_FAILURE );
        }

        MomCtrlConfig.MaxPendingCommands   = bufNumMainPath + bufNumSelfPath;
        MomCtrlConfig.NumBuffersMainPath   = bufNumMainPath;
        MomCtrlConfig.NumBuffersSelfPath   = bufNumSelfPath;
        MomCtrlConfig.pPicBufPoolMainPath  = &( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMain );
        MomCtrlConfig.pPicBufPoolSelfPath  = NULL;
        MomCtrlConfig.momCbCompletion      = CamEngineMomCtrlCb;
        MomCtrlConfig.pUserContext         = &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1];
        MomCtrlConfig.hCamerIc             = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        MomCtrlConfig.HalHandle            = pCamEngineCtx->hHal;
        MomCtrlConfig.hMomContext          = NULL;

        result = MomCtrlInit( &MomCtrlConfig );
        if ( result != RET_SUCCESS )
        {
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart );
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( result );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl = MomCtrlConfig.hMomContext;

        result = MomCtrlAttachQueueToPath( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl,
                            MOM_CTRL_PATH_MAINPATH, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue );
        if ( result != RET_SUCCESS )
        {
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl = NULL;
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( result );
        }

        result = MomCtrlAttachQueueToPath( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl,
                            MOM_CTRL_PATH_MAINPATH, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MainPathQueue );
        if ( result != RET_SUCCESS )
        {
            (void)MomCtrlDetachQueueToPath( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl,
                            MOM_CTRL_PATH_MAINPATH, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue );
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl = NULL;
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( result );
        }
    }
    else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
    {
        if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart, 1, 0 ) )
        {
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( RET_FAILURE );
        }

        if ( OSLAYER_OK != osEventInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStop, 1, 0 ) )
        {
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart );
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( RET_FAILURE );
        }

        MomCtrlConfig.MaxPendingCommands   = bufNumMainPath + bufNumSelfPath;
        MomCtrlConfig.NumBuffersMainPath   = bufNumMainPath;
        MomCtrlConfig.NumBuffersSelfPath   = bufNumSelfPath;
        MomCtrlConfig.pPicBufPoolMainPath  = &( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].BufPoolMain );
        MomCtrlConfig.pPicBufPoolSelfPath  = NULL;
        MomCtrlConfig.momCbCompletion      = CamEngineMomCtrlCb;
        MomCtrlConfig.pUserContext         = &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1];
        MomCtrlConfig.hCamerIc             = pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hCamerIc;
        MomCtrlConfig.HalHandle            = pCamEngineCtx->hHal;
        MomCtrlConfig.hMomContext          = NULL;

        result = MomCtrlInit( &MomCtrlConfig );
        if ( result != RET_SUCCESS )
        {
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart );
            (void)MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
            pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
            (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
            return ( result );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl = MomCtrlConfig.hMomContext;
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineMomCtrlStart()
 *****************************************************************************/
static RESULT CamEngineMomCtrlStart
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    result = MomCtrlStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
    if ( result != RET_PENDING )
    {
        return ( result );
    }

    if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart ) )
    {
        return ( RET_FAILURE );
    }

    /* do we have a second pipeline ? */
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = MomCtrlStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart ) )
        {
            return ( RET_FAILURE );
        }
    }
    else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
    {
        // DMA needs to be started
        pCamEngineCtx->startDMA = BOOL_TRUE;
     
        result = MomCtrlStart( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart ) )
        {
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineMomCtrlStop()
 *****************************************************************************/
static RESULT CamEngineMomCtrlStop
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    result = MomCtrlStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
    if ( result != RET_PENDING )
    {
        return ( result );
    }

    if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop ) )
    {
        return ( RET_FAILURE );
    }

    /* do we have a second pipeline ? */
    if ( ( CAM_ENGINE_MODE_SENSOR_3D         == pCamEngineCtx->mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode ) )
    {
        result = MomCtrlStop( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );
        if ( result != RET_PENDING )
        {
            return ( result );
        }

        if ( OSLAYER_OK != osEventWait( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStop ) )
        {
            return ( RET_FAILURE );
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineMomCtrlRelease()
 *****************************************************************************/
static RESULT CamEngineMomCtrlRelease
(
    CamEngineContext_t *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    /* do we have a second pipeline ? */
    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl )
    {
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = MomCtrlDetachQueueToPath( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl,
                            MOM_CTRL_PATH_MAINPATH, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue );
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }

            result = MomCtrlDetachQueueToPath( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl,
                            MOM_CTRL_PATH_MAINPATH, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MainPathQueue );
            if ( result != RET_SUCCESS )
            {
                return ( result );
            }
        }

        result = MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl = NULL;

        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStart );
        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MomEventCmdStop );
    }

    if ( NULL != pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl )
    {
        result = MomCtrlShutDown( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl );
        if ( result != RET_SUCCESS )
        {
            return ( result );
        }
        pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl = NULL;

        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStart );
        (void)osEventDestroy(  &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MomEventCmdStop );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * CamEngineSubCtrlsSetup()
 *****************************************************************************/
RESULT CamEngineSubCtrlsSetup
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS ;

    uint32_t bufNumImg    = 0U;
    uint32_t bufSizeImg   = 0U;

    uint32_t bufNumMain   = 0U;
    uint32_t bufNumSelf   = 0U;
    uint32_t bufSizeMain  = 0U;
    uint32_t bufSizeSelf  = 0U;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* set bufferpool parameter */
    switch ( pCamEngineCtx->mode )
    {
        case CAM_ENGINE_MODE_SENSOR_2D:
            {
                bufNumImg   = PIC_BUFFER_NUM_SIMP_INPUT;
                bufSizeImg  = PIC_BUFFER_SIZE_SIMP_INPUT;

				bufNumMain  = pCamEngineCtx->bufNum;
                //bufNumMain  = PIC_BUFFER_NUM_MAIN_SENSOR;
                bufNumSelf  = PIC_BUFFER_NUM_SELF_SENSOR;
				bufSizeMain = pCamEngineCtx->bufSize;
                //bufSizeMain = PIC_BUFFER_SIZE_MAIN_SENSOR;
                bufSizeSelf = PIC_BUFFER_SIZE_SELF_SENSOR;

                break;
            }

        case CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB:
            {
                bufNumImg   = PIC_BUFFER_NUM_SIMP_INPUT;
                bufSizeImg  = PIC_BUFFER_SIZE_SIMP_INPUT;

                bufNumMain  = PIC_BUFFER_NUM_MAIN_SENSOR_IMGSTAB;
                bufSizeMain = PIC_BUFFER_SIZE_MAIN_SENSOR_IMGSTAB;
                break;
            }

        case CAM_ENGINE_MODE_SENSOR_3D:
            {
                bufNumImg   = 0;
                bufSizeImg  = 0;

                bufNumMain  = PIC_BUFFER_NUM_MAIN_SENSOR_3D;
                bufSizeMain = PIC_BUFFER_SIZE_MAIN_SENSOR_3D;
                break;
            }
        
        case CAM_ENGINE_MODE_IMAGE_PROCESSING:
            {
                bufNumImg   = PIC_BUFFER_NUM_INPUT;
                bufSizeImg  = PIC_BUFFER_SIZE_INPUT;

                bufNumMain  = PIC_BUFFER_NUM_MAIN_IMAGE;
                bufNumSelf  = PIC_BUFFER_NUM_SELF_IMAGE;
                bufSizeMain = PIC_BUFFER_SIZE_MAIN_IMAGE;
                bufSizeSelf = PIC_BUFFER_SIZE_SELF_IMAGE;
                break;
            }

        default:
            {
                TRACE( CAM_ENGINE_ERROR, "%s (buffer size initialization failed, unsupported mode(%d))\n", __FUNCTION__, pCamEngineCtx->mode );
                return ( RET_NOTSUPP );
            }
    }

#if 0 //modify by zyc ,input buffer is not neccessary
    // create input buffer pool
    result = CamEngineCreateInputPictureBufferPool( pCamEngineCtx, bufNumImg, bufSizeImg );
    if ( result != RET_SUCCESS )
    {
        CamEngineDestroyInputPictureBufferPool( pCamEngineCtx );
        TRACE( CAM_ENGINE_ERROR, "%s (initialization of input buffer-pool failed)\n", __FUNCTION__);
        return ( result );
    }
#endif
    // create buffer pool for main pipe
    result = CamEngineCreatePictureBufferPool( pCamEngineCtx, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0], bufNumMain, bufSizeMain, bufNumSelf, bufSizeSelf );
    if ( result != RET_SUCCESS )
    {
        CamEngineDestroyPictureBufferPool( pCamEngineCtx );
        TRACE( CAM_ENGINE_ERROR, "%s (initialization of buffer-pools failed)\n", __FUNCTION__);
        return ( result );
    }

    // create buffer pool for slave pipe
    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        // note: selfpath not available on 2nd CamerIc
        result = CamEngineCreatePictureBufferPool( pCamEngineCtx, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1], bufNumMain, bufSizeMain, 0, 0 );
        if ( result != RET_SUCCESS )
        {
            CamEngineDestroyPictureBufferPool( pCamEngineCtx );
            TRACE( CAM_ENGINE_ERROR, "%s (initialization of buffer-pools failed)\n", __FUNCTION__);
            return ( result );
        }

        // create mainpath queue for master pipe
        if ( OSLAYER_OK != osQueueInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue, bufNumMain, sizeof(MediaBuffer_t *) ) )
        {
            return ( RET_FAILURE );
        }

        // create mainpath queue for slave pipe
        if ( OSLAYER_OK != osQueueInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MainPathQueue, bufNumMain, sizeof(MediaBuffer_t *) ) )
        {
            return ( RET_FAILURE );
        }

        // create buffer sync control
        result = CamEngineBufSyncCtrlSetup( pCamEngineCtx, bufNumMain, bufNumSelf );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't setup BufSync-Ctrl\n", __FUNCTION__ );
            return ( result );
        }
    }
    else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
    {
        // note: selfpath not available on 2nd CamerIc
        result = CamEngineCreatePictureBufferPool( pCamEngineCtx, &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1], bufNumMain, bufSizeMain, 0, 0 );
        if ( result != RET_SUCCESS )
        {
            CamEngineDestroyPictureBufferPool( pCamEngineCtx );
            TRACE( CAM_ENGINE_ERROR, "%s (initialization of buffer-pools failed)\n", __FUNCTION__);
            return ( result );
        }
        
        // create mainpath queue for master pipe
        if ( OSLAYER_OK != osQueueInit( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue, bufNumMain, sizeof(MediaBuffer_t *) ) )
        {
            return ( RET_FAILURE );
        }

        result = CamEngineMimCtrlSetup( pCamEngineCtx, bufNumMain );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't setup MIM-Ctrl\n", __FUNCTION__ );
            return ( result );
        }
    }
    else if ( CAM_ENGINE_MODE_IMAGE_PROCESSING == pCamEngineCtx->mode )
    {
        result = CamEngineMimCtrlSetup( pCamEngineCtx, bufNumImg );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s: Can't setup MIM-Ctrl\n", __FUNCTION__ );
            return ( result );
        }
    }

    result = CamEngineMomCtrlSetup( pCamEngineCtx, bufNumMain, bufNumSelf );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't setup MOM-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
    {
        pCamEngineCtx->startDMA = BOOL_TRUE;

        // register dma buffer-callback at CamerIc master 
        result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_MAINPATH,
                (MomCtrlBufferCb_t)CamEngineDmaRestartCb, pCamEngineCtx );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
            return ( result );
        }
    }

    // register callbacks
    if ( NULL != pCamEngineCtx->cbBuffer )
    {
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_SELFPATH,
                    (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }

            // register buffer-callback at buffer-sync ctrl
            result = BufSyncCtrlRegisterBufferCb( pCamEngineCtx->hBufSyncCtrl,
                    (BufSyncCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
        else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
        {
            // register buffer-callback at CamerIc slave
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl, MOM_CTRL_PATH_MAINPATH,
                    (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
        else
        {
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_SELFPATH,
                    (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }

            // register buffer-callback at CamerIc master
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_MAINPATH,
                    (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSubCtrlsStart()
 *****************************************************************************/
RESULT CamEngineSubCtrlsStart
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS ;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pCamEngineCtx != NULL );

    result = CamEngineMomCtrlStart( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't start MOM-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    result = CamEngineBufSyncCtrlStart( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't start BufSync-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    result = CamEngineMimCtrlStart( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't start MIM-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSubCtrlsStop()
 *****************************************************************************/
RESULT CamEngineSubCtrlsStop
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS ;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pCamEngineCtx != NULL );

    result = CamEngineMimCtrlStop( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't stop MIM-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    result = CamEngineBufSyncCtrlStop( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't stop BufSync-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    result = CamEngineMomCtrlStop( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s: Can't stop MOM-Ctrl\n", __FUNCTION__ );
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSubCtrlsRelease()
 *****************************************************************************/
RESULT CamEngineSubCtrlsRelease
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS ;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pCamEngineCtx != NULL );

    /* unregister buffer callback */
    result = MomCtrlDeRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_SELFPATH );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAM_ENGINE_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
        return ( result );
    }

    if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
    {
        result = BufSyncCtrlDeRegisterBufferCb( pCamEngineCtx->hBufSyncCtrl );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
            return ( result );
        }
    }
    else
    {
        result = MomCtrlDeRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_MAINPATH );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
            return ( result );
        }
    }

    result = CamEngineMimCtrlRelease( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamEngineBufSyncCtrlRelease( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamEngineMomCtrlRelease( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    (void)osQueueDestroy( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].MainPathQueue );
    (void)osQueueDestroy( &pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].MainPathQueue );

    result = CamEngineDestroyInputPictureBufferPool( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    result = CamEngineDestroyPictureBufferPool( pCamEngineCtx );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamEngineSubCtrlsRegisterBufferCb
 *****************************************************************************/
RESULT  CamEngineSubCtrlsRegisterBufferCb
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineBufferCb_t fpCallback,
    void                *pBufferCbCtx
)
{
    RESULT result = RET_SUCCESS ;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pCamEngineCtx != NULL );

    pCamEngineCtx->cbBuffer     = fpCallback;
    pCamEngineCtx->pBufferCbCtx = pBufferCbCtx;

    if ( CAM_ENGINE_STATE_RUNNING == pCamEngineCtx->state )
    {
        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_SELFPATH,
                     (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }

            // register buffer-callback at buffer-sync ctrl
            result = BufSyncCtrlRegisterBufferCb( pCamEngineCtx->hBufSyncCtrl,
                    (BufSyncCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
        else if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == pCamEngineCtx->mode )
        {
            // register buffer-callback at CamerIc slave
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx1].hMomCtrl, MOM_CTRL_PATH_MAINPATH,
                    (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
        else
        {
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_SELFPATH,
                     (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }

            // register buffer-callback at CamerIc master
            result = MomCtrlRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_MAINPATH,
                    (MomCtrlBufferCb_t)pCamEngineCtx->cbBuffer, pCamEngineCtx->pBufferCbCtx );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (registering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamEngineSubCtrlsDeRegisterBufferCb
 *****************************************************************************/
RESULT  CamEngineSubCtrlsDeRegisterBufferCb
(
    CamEngineContext_t  *pCamEngineCtx
)
{
    RESULT result = RET_SUCCESS ;

    TRACE( CAM_ENGINE_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT( pCamEngineCtx != NULL );

    pCamEngineCtx->cbBuffer     = NULL;
    pCamEngineCtx->pBufferCbCtx = NULL;

    if ( CAM_ENGINE_STATE_RUNNING == pCamEngineCtx->state )
    {
        result = MomCtrlDeRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_SELFPATH );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAM_ENGINE_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
            return ( result );
        }

        if ( CAM_ENGINE_MODE_SENSOR_3D == pCamEngineCtx->mode )
        {
            result = BufSyncCtrlDeRegisterBufferCb( pCamEngineCtx->hBufSyncCtrl );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
        else
        {
            result = MomCtrlDeRegisterBufferCb( pCamEngineCtx->chain[pCamEngineCtx->chainIdx0].hMomCtrl, MOM_CTRL_PATH_MAINPATH );
            if ( result != RET_SUCCESS )
            {
                TRACE( CAM_ENGINE_ERROR, "%s (unregistering buffer callback failed)\n", __FUNCTION__ );
                return ( result );
            }
        }
    }

    TRACE( CAM_ENGINE_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

