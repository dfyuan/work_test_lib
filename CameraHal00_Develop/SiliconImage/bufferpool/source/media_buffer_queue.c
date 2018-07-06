/******************************************************************************
 *
 * Copyright 2007, Silicon Image, Inc.  All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc., 1060
 * East Arques Avenue, Sunnyvale, California 94085
 *
 *****************************************************************************/
/**
 * @file media_buffer_queue.c
 *
 * @brief
 *          Buffer Pool implementation
 *
 * <pre>
 *
 *   Principal Author: Joerg Detert
 *   Creation date:    Feb 28, 2008
 *
 * </pre>
 *
 *****************************************************************************/

#include <ebase/builtins.h>

#include "media_buffer_pool.h"
#include "media_buffer_queue.h"


/***** local functions ***********************************************/

/*****************************************************************************/
/**
 * @brief Inserts new media buffer at head of buffer queue.
 * 
 * @param pBufQueue   Pointer to Media Buffer Queue object.
 * @param pBuf        Pointer to media buffer.
 *****************************************************************************/
static void MediaBufQueueInsertInHead(MediaBufQueue_t *pBufQueue, MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    DCT_ASSERT(!MediaBufQueueIsFull(pBufQueue));
    DCT_ASSERT(pBufQueue->pBufArray[pBufQueue->head] == NULL);

    pBufQueue->pBufArray[pBufQueue->head] = pBuf;

    /* advance head */
    pBufQueue->head++;

    /* one more full buffer is available */
    pBufQueue->fullBufsAvail++;

    if(pBufQueue->head >= pBufQueue->bufPool.bufNum)
    {
        pBufQueue->head = 0U; /* wrap */
    }

    /* inform media buffer pool about filled buffer */
    MediaBufPoolBufferFilled(&pBufQueue->bufPool, pBuf);

    DCT_ASSERT(pBufQueue->fullBufsAvail <= pBufQueue->bufPool.fillLevel);
    
}


/*****************************************************************************/
/**
 * @brief Removes media buffer from tail of buffer queue.
 * 
 * @param pBufQueue   Pointer to Media Buffer Queue object.
 * 
 * @return pBuf       Pointer to media buffer.
 *****************************************************************************/
static MediaBuffer_t* MediaBufQueueGetFromTail(MediaBufQueue_t *pBufQueue)
{
    MediaBuffer_t *pBuf;

    DCT_ASSERT(pBufQueue != NULL);
    
    DCT_ASSERT(MediaBufQueueFullBuffersAvailable(pBufQueue));
    DCT_ASSERT(pBufQueue->pBufArray[pBufQueue->tail] != NULL);

    pBuf = pBufQueue->pBufArray[pBufQueue->tail];

    pBufQueue->pBufArray[pBufQueue->tail] = NULL;

    /* advance tail */
    pBufQueue->tail++;

    if(pBufQueue->tail >= pBufQueue->bufPool.bufNum)
    {
        pBufQueue->tail = 0U;
    }
     
    DCT_ASSERT(pBuf != NULL);
    
    /* one full buffer less available now */
    DCT_ASSERT(pBufQueue->fullBufsAvail > 0);
    pBufQueue->fullBufsAvail--;
    
    return pBuf;
}


/******************************************************************************
 * MediaBufQueueGetBufQueueMemSize
 *****************************************************************************/
static uint32_t MediaBufQueueGetBufQueueMemSize(const MediaBufQueueConfig_t* pConfig)
{
    return (pConfig->maxBufNum * sizeof(MediaBuffer_t*));
}





/***** API implementation ***********************************************/


/******************************************************************************
 * MediaBufQueueGetSize
 *****************************************************************************/
RESULT MediaBufQueueGetSize(MediaBufQueueConfig_t* pConfig)
{
    RESULT ret = RET_SUCCESS;
    
    DCT_ASSERT(pConfig != NULL);
    
    /* Get needed size of memory  for buffer pool */
    ret = MediaBufPoolGetSize(pConfig);
    if(ret != RET_SUCCESS)
    {
        return ret;
    }
    
    /* Add size of memmory needed by queue to meta data memory size */
    pConfig->metaDataMemSize += MediaBufQueueGetBufQueueMemSize(pConfig);
    
    return ret;
}


/******************************************************************************
 * MediaBufQueueCreate
 *****************************************************************************/
RESULT MediaBufQueueCreate(MediaBufQueue_t*       pBufQueue,
                                   MediaBufQueueConfig_t* pConfig,
                                   MediaBufQueueMemory_t  queueMemory)
{
    RESULT  ret = RET_SUCCESS;

    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }
    
    if(pConfig == NULL)
    {
        return RET_INVALID_PARM;
    }
    
    if((queueMemory.pBufferMemory    == NULL) ||
       (queueMemory.pMetaDataMemory  == NULL) )
    {
        return RET_INVALID_PARM;
    }

    /* initialize buffer queue object*/
    (void)MEMSET(pBufQueue, 0, (size_t) sizeof(MediaBufQueue_t));

    /* initialize SW memory depending on buffer count */
    {
        uint32_t bufQueueMemSize = MediaBufQueueGetBufQueueMemSize(pConfig);
        
        /* Take first chunk of memory for queue (pointer array) */
        pBufQueue->pBufArray = (MediaBuffer_t**) queueMemory.pMetaDataMemory;
        (void) MEMSET(pBufQueue->pBufArray, 0, (size_t) bufQueueMemSize);
    
        /* Adapt p_buffer_pool_mem and buf_pool_mem_size before handing them to MediaBufPoolCreate */
        queueMemory.pMetaDataMemory = ((uint8_t*) queueMemory.pMetaDataMemory) + bufQueueMemSize;
    }

    /* Create media buffer pool */
    ret = MediaBufPoolCreate(&pBufQueue->bufPool,
                             pConfig,
                             queueMemory);
    
    return ret;
}


/******************************************************************************
 * MediaBufQueueDestroy
 *****************************************************************************/
RESULT MediaBufQueueDestroy(MediaBufQueue_t* pBufQueue)
{
    RESULT ret = RET_SUCCESS;
    
    DCT_ASSERT(pBufQueue != NULL);
    
    ret = MediaBufPoolDestroy(&pBufQueue->bufPool);
    
    (void) MEMSET(pBufQueue, 0, sizeof(MediaBufQueue_t));

    return ret;
}


/******************************************************************************
 * MediaBufQueueGetEmptyBuffer
 *****************************************************************************/
MediaBuffer_t* MediaBufQueueGetEmptyBuffer(MediaBufQueue_t *pBufQueue)
{
    MediaBuffer_t* pBuf;
    
    DCT_ASSERT(pBufQueue != NULL);
    
    pBuf = MediaBufPoolGetBuffer(&pBufQueue->bufPool);
    
    if (pBuf != NULL)
    {
        pBuf->isFull = BOOL_FALSE;
    }
    
    return pBuf;
}


/******************************************************************************
 * MediaBufQueueUpdateAvgFillLevel
 * 
 * Note: avgFillLevel is an exponential moving average 
 *       expressed as a 16.16 fixed point number.
 *****************************************************************************/
static void MediaBufQueueUpdateAvgFillLevel(MediaBufQueue_t *pBufQueue)
{    
    int32_t newFillLevelFixedPoint    = (int32_t) (pBufQueue->bufPool.fillLevel << 16);
    int32_t oldAvgFillLevelFixedPoint = (int32_t) pBufQueue->avgFillLevel;
    int32_t newAvgFillLevelFixedPoint;
    
    newAvgFillLevelFixedPoint = oldAvgFillLevelFixedPoint + ((newFillLevelFixedPoint - oldAvgFillLevelFixedPoint) >> 4);
    
    pBufQueue->avgFillLevel = (uint32_t) newAvgFillLevelFixedPoint;
}


/******************************************************************************
 * MediaBufQueuePutFullBuffer
 *****************************************************************************/
RESULT MediaBufQueuePutFullBuffer(MediaBufQueue_t *pBufQueue, MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);
    
    if(MediaBufQueueIsFull(pBufQueue))
    {
        return RET_NOTAVAILABLE;
    }

    pBuf->isFull = BOOL_TRUE;
    
    MediaBufQueueInsertInHead(pBufQueue, pBuf);

    /* Update maximum fill level. */
    if (pBufQueue->bufPool.fillLevel > pBufQueue->maxFillLevel)
    {
        pBufQueue->maxFillLevel = pBufQueue->bufPool.fillLevel;
    }

    /* Update average fill level. */
    MediaBufQueueUpdateAvgFillLevel(pBufQueue);

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueGetFullBuffer
 *****************************************************************************/
MediaBuffer_t* MediaBufQueueGetFullBuffer(MediaBufQueue_t *pBufQueue)
{
    MediaBuffer_t *pBuf;

    DCT_ASSERT(pBufQueue != NULL);

    while(!MediaBufQueueFullBuffersAvailable(pBufQueue))
    {
        return  NULL;
    }

    pBuf = MediaBufQueueGetFromTail(pBufQueue);
    
    DCT_ASSERT(pBuf != NULL);

    return pBuf;
}


/******************************************************************************
 * MediaBufQueueReleaseBuffer
 *****************************************************************************/
RESULT MediaBufQueueReleaseBuffer(MediaBufQueue_t* pBufQueue,
                                          MediaBuffer_t*   pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);
    
    if (pBuf->lockCount == 0U)
    {
        return RET_FAILURE;
    }

    pBuf->lockCount--;

    if(pBuf->lockCount == 0U)
    {
        MediaBufPoolFreeBuffer(&pBufQueue->bufPool, pBuf);
        
        /* Update average fill level. */
        MediaBufQueueUpdateAvgFillLevel(pBufQueue);
    }

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueFlush
 *****************************************************************************/
RESULT MediaBufQueueFlush(MediaBufQueue_t *pBufQueue)
{
    MediaBuffer_t *pBuf;

    DCT_ASSERT(pBufQueue != NULL);

    /* Free all full buffers which have not yet been retrieved */
    while(MediaBufQueueFullBuffersAvailable(pBufQueue))
    {
        pBuf = MediaBufQueueGetFullBuffer(pBufQueue);
        while (pBuf->lockCount > 0U)
        {
            MediaBufQueueReleaseBuffer(pBufQueue, pBuf);
        }
    }
    
    pBufQueue->maxFillLevel = 0;
    pBufQueue->avgFillLevel = 0;
    
    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueReset
 *****************************************************************************/
RESULT MediaBufQueueReset(MediaBufQueue_t* pBufQueue)
{
    /* Note: this function applies a flush by reset of variables,
       but there's no need to explicitely call the MediaBufQueueFlush
       function */

    DCT_ASSERT(pBufQueue != NULL);

    /* reset state variables */    
    pBufQueue->fullBufsAvail = 0;
    pBufQueue->maxFillLevel  = 0;
    pBufQueue->avgFillLevel  = 0;
    
    /* reset buffer queue array */
    (void) MEMSET(pBufQueue->pBufArray, 0, (size_t) (pBufQueue->bufPool.maxBufNum * sizeof(MediaBuffer_t*)));
    pBufQueue->head = 0;
    pBufQueue->tail = 0;

    /* reset buffer pool */
    return MediaBufPoolReset(&pBufQueue->bufPool);
}


/******************************************************************************
 * MediaBufQueueSetParameter
 *****************************************************************************/
RESULT MediaBufQueueSetParameter(MediaBufQueue_t*       pBufQueue, 
                                 MediaBufQueueParamId_t paramId, 
                                 int32_t                paramValue)
{
    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }
    
    return MediaBufPoolSetParameter(&pBufQueue->bufPool, paramId, paramValue);
}


/******************************************************************************
 * MediaBufQueueGetParameter
 *****************************************************************************/
RESULT MediaBufQueueGetParameter(MediaBufQueue_t*       pBufQueue, 
                                 MediaBufQueueParamId_t paramId, 
                                 int32_t*               pParamValue)
{
    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }
    
    return MediaBufPoolGetParameter(&pBufQueue->bufPool, paramId, pParamValue);
}


/******************************************************************************
 * MediaBufQueueLockBuffer
 *****************************************************************************/
RESULT MediaBufQueueLockBuffer(MediaBufQueue_t* pBufQueue,
                               MediaBuffer_t*   pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    (void) pBufQueue;

    pBuf->lockCount++;
    
    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueUnlockBuffer
 *****************************************************************************/
RESULT MediaBufQueueUnlockBuffer(MediaBufQueue_t* pBufQueue,
                                 MediaBuffer_t*   pBuf)
{
    /* Unlock is not more than an alias for Release,
       because if the lock drops down to 0, buffer must
       be freed, and this is handled in Release */
    return MediaBufQueueReleaseBuffer(pBufQueue, pBuf);
}


/******************************************************************************
 * MediaBufQueueGetStats
 *****************************************************************************/
RESULT MediaBufQueueGetStats(MediaBufQueue_t*     pBufQueue,
                             MediaBufQueueStat_t* pStat)
{
    pStat->currFillLevel  = pBufQueue->bufPool.fillLevel;
    pStat->maxFillLevel   = pBufQueue->maxFillLevel;
    pStat->avgFillLevel   = pBufQueue->avgFillLevel;
    /* meanTargetArea is expressed as a 16.16 fixed point number, and is lwm + (hwm - lwm)/2. */
    pStat->meanTargetArea = (pBufQueue->bufPool.lowWatermark << 16) + ((pBufQueue->bufPool.highWatermark - pBufQueue->bufPool.lowWatermark) << 15);
    
    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueRegisterCb
 *****************************************************************************/
RESULT MediaBufQueueRegisterCb(MediaBufQueue_t*        pBufQueue,
                               MediaBufQueueCbNotify_t fpCallback,
                               void*                   pUserContext)
{
    if(pBufQueue == NULL)
    {
        return RET_INVALID_PARM;
    }
    
    return MediaBufPoolRegisterCb(&pBufQueue->bufPool, fpCallback, pUserContext);
}


/******************************************************************************
 * MediaBufQueueDeregisterCb
 *****************************************************************************/
RESULT MediaBufQueueDeregisterCb(MediaBufQueue_t*        pBufQueue,
                                 MediaBufQueueCbNotify_t fpCallback)
{
    if(pBufQueue == NULL)
    {
        return RET_INVALID_PARM;
    }
    
    return MediaBufPoolDeregisterCb(&pBufQueue->bufPool, fpCallback);
}
