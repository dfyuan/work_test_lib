/******************************************************************************
 *
 * Copyright 2010, Silicon Image, Inc.  All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of: Silicon Image, Inc., 1060
 * East Arques Avenue, Sunnyvale, California 94085
 *
 *****************************************************************************/
/**
 * @file media_buffer_queue_ex.c
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
#include "media_buffer_queue_ex.h"


/***** local functions ***********************************************/

/*****************************************************************************/
/**
 * @brief Inserts new media buffer at head of buffer queue.
 *
 * @param pBufQueue   Pointer to Media Buffer Queue object.
 * @param pBuf        Pointer to media buffer.
 *****************************************************************************/
static void MediaBufQueueExInsertInHead(MediaBufQueueEx_t *pBufQueue, MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);
    DCT_ASSERT(pBuf != NULL);

    DCT_ASSERT(!MediaBufQueueExIsFull(pBufQueue));
    DCT_ASSERT(pBufQueue->pBufArray[pBufQueue->head] == NULL);

    DCT_ASSERT(pBufQueue->fullBufsAvail < pBufQueue->pBufPool->fillLevel); // same as !MediaBufQueueExIsFull()...

    pBufQueue->pBufArray[pBufQueue->head] = pBuf;

    /* advance head */
    pBufQueue->head++;
    if(pBufQueue->head >= pBufQueue->pBufPool->bufNum)
    {
        pBufQueue->head = 0U; /* wrap */
    }

    /* one more full buffer is available */
    pBufQueue->fullBufsAvail++;

    if (!pBufQueue->isExtPool)
    {
        /* inform media buffer pool (and its registered users) about filled buffer */
        /* if buffer pool is external the producer has to call this function
         * prior putting the buffers into (possibly multiple) output queue(s);
         * after queueing the buffer he has to call MediaBufQueueExPutFullBufferDone()
         * to finalize this step                                                       */
        MediaBufPoolBufferFilled(pBufQueue->pBufPool, pBuf);
    }
    else
    {
        uint32_t i;

        /* inform registered queue users about new full buffer */
        for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
        {
            if(pBufQueue->notify[i].fpCallback != NULL)
            {
                (pBufQueue->notify[i].fpCallback)(FULL_BUFFER_ADDED, pBufQueue->notify[i].pUserContext, pBuf);
            }
        }

        /* inform registered queue users about entering of the high watermark critical region.
         * If the last buffer flag is set we have to inform the user too.
         * Otherwise he would wait infinitely for the hit of the high watermark.*/
        if(pBufQueue->highWatermark && ((uint32_t)(pBufQueue->highWatermark) == pBufQueue->fullBufsAvail)) 
        {
            for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
            {
                if(pBufQueue->notify[i].fpCallback != NULL)
                {
                    (pBufQueue->notify[i].fpCallback)(HIGH_WATERMARK_CRITICAL_REGION_ENTERED, pBufQueue->notify[i].pUserContext, pBuf);
                }
            }
        }

        /* inform registered queue users about leaving of the low watermark critical region */
        if(pBufQueue->lowWatermark && ((uint32_t)(pBufQueue->lowWatermark + 1) == pBufQueue->fullBufsAvail))
        {
            for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
            {
                if(pBufQueue->notify[i].fpCallback != NULL)
                {
                    (pBufQueue->notify[i].fpCallback)(LOW_WATERMARK_CRITICAL_REGION_LEFT, pBufQueue->notify[i].pUserContext, pBuf);
                }
            }
        }
    }
}


/*****************************************************************************/
/**
 * @brief Removes media buffer from tail of buffer queue.
 *
 * @param pBufQueue   Pointer to Media Buffer Queue object.
 *
 * @return pBuf       Pointer to media buffer.
 *****************************************************************************/
static MediaBuffer_t* MediaBufQueueExGetFromTail(MediaBufQueueEx_t *pBufQueue)
{
    MediaBuffer_t *pBuf;

    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    DCT_ASSERT(MediaBufQueueExFullBuffersAvailable(pBufQueue));
    DCT_ASSERT(pBufQueue->pBufArray[pBufQueue->tail] != NULL);

    DCT_ASSERT(pBufQueue->fullBufsAvail > 0);

    pBuf = pBufQueue->pBufArray[pBufQueue->tail];

    pBufQueue->pBufArray[pBufQueue->tail] = NULL;

    /* advance tail */
    pBufQueue->tail++;
    if(pBufQueue->tail >= pBufQueue->pBufPool->bufNum)
    {
        pBufQueue->tail = 0U;
    }

    DCT_ASSERT(pBuf != NULL);

    /* one full buffer less available now */
    pBufQueue->fullBufsAvail--;

    if (pBufQueue->isExtPool)
    {
        uint32_t i;

        /* inform registered queue users about removed full buffer */
        for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
        {
            if(pBufQueue->notify[i].fpCallback != NULL)
            {
                (pBufQueue->notify[i].fpCallback)(FULL_BUFFER_REMOVED, pBufQueue->notify[i].pUserContext, pBuf);
            }
        }

        /* inform registered queue users about entering of the low watermark critical region */
        if(pBufQueue->lowWatermark &&
           ((uint32_t)(pBufQueue->lowWatermark) == pBufQueue->fullBufsAvail))
        {
            for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
            {
                if(pBufQueue->notify[i].fpCallback != NULL)
                {
                    (pBufQueue->notify[i].fpCallback)(LOW_WATERMARK_CRITICAL_REGION_ENTERED, pBufQueue->notify[i].pUserContext, pBuf);
                }
            }
        }

        /* inform registered queue users about leaving of the high watermark critical region */
        if(pBufQueue->highWatermark &&
           ((uint32_t)(pBufQueue->highWatermark - 1) == pBufQueue->fullBufsAvail) )
        {
            for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
            {
                if(pBufQueue->notify[i].fpCallback != NULL)
                {
                    (pBufQueue->notify[i].fpCallback)(HIGH_WATERMARK_CRITICAL_REGION_LEFT, pBufQueue->notify[i].pUserContext, pBuf);
                }
            }
        }
    }

    return pBuf;
}


/******************************************************************************
 * MediaBufQueueExGetBufQueueMemSize
 *****************************************************************************/
static uint32_t MediaBufQueueExGetBufQueueMemSize(const MediaBufQueueExConfig_t* pConfig)
{
    return (pConfig->maxBufNum * sizeof(MediaBuffer_t*));
}


/******************************************************************************
 * MediaBufQueueExReleaseBufferInt
 *****************************************************************************/
static RESULT MediaBufQueueExReleaseBufferInt(MediaBufQueueEx_t* pBufQueue, MediaBuffer_t* pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    MediaBufPool_t *pBufPool = pBufQueue->pBufPool;
    DCT_ASSERT(pBufPool != NULL);

    if (pBuf->lockCount == 0U)
    {
        return RET_FAILURE;
    }

    //
    //TODO/FIX: this assumes the buffer was added to the queue with a lock count of 1
    if (pBuf->lockCount == 1U)
    {
        int i;

        /* inform registered queue users about buffer to be released */
        /* e.g. to release chained ancillary buffers as well */
        for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
        {
            if(pBufQueue->notify[i].fpCallback != NULL)
            {
                (pBufQueue->notify[i].fpCallback)(EMPTY_BUFFER_ADDED, pBufQueue->notify[i].pUserContext, pBuf);
            }
        }
    }

    pBuf->lockCount--;

    if (pBuf->lockCount == 0U)
    {
        /* inform media buffer pool (and its registered users) about released buffer */
        /* always do this, not matter if the pool is external or not */
        /* (hard resets lockCount to 0 for historical reasons) */
        MediaBufPoolFreeBuffer(pBufPool, pBuf);
    }

    return RET_SUCCESS;
}


/***** API implementation ***********************************************/


/******************************************************************************
 * MediaBufQueueExGetSize
 *****************************************************************************/
RESULT MediaBufQueueExGetSize(MediaBufQueueExConfig_t* pConfig)
{
    RESULT ret = RET_SUCCESS;

    DCT_ASSERT(pConfig != NULL);

    /* Get needed size of memory for buffer pool */
    ret = MediaBufPoolGetSize(pConfig);
    if(ret != RET_SUCCESS)
    {
        return ret;
    }

    /* Add size of memmory needed by queue to meta data memory size */
    pConfig->metaDataMemSize += MediaBufQueueExGetBufQueueMemSize(pConfig);

    return ret;
}


/******************************************************************************
 * MediaBufQueueExGetSizeExtPool
 *****************************************************************************/
RESULT MediaBufQueueExGetSizeExtPool(MediaBufQueueExConfig_t* pConfig)
{
    RESULT ret = RET_SUCCESS;

    DCT_ASSERT(pConfig != NULL);

    /* Put needed size of memory for buffer pool into buffer memory size */
    pConfig->bufMemSize = 0;

    /* Put size of memmory needed by queue into meta data memory size */
    pConfig->metaDataMemSize = MediaBufQueueExGetBufQueueMemSize(pConfig);

    return ret;
}


/******************************************************************************
 * MediaBufQueueExCreate
 *****************************************************************************/
RESULT MediaBufQueueExCreate(MediaBufQueueEx_t*       pBufQueue,
                                     MediaBufQueueExConfig_t* pConfig,
                                     MediaBufQueueExMemory_t  queueMemory)
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
    (void)MEMSET(pBufQueue, 0, (size_t) sizeof(MediaBufQueueEx_t));

    /* initialize SW memory depending on buffer count */
    {
        uint32_t bufQueueMemSize = MediaBufQueueExGetBufQueueMemSize(pConfig);

        /* Take first chunk of memory for queue (pointer array) */
        pBufQueue->pBufArray = (MediaBuffer_t**) queueMemory.pMetaDataMemory;
        (void) MEMSET(pBufQueue->pBufArray, 0, (size_t) bufQueueMemSize);

        /* Adapt p_buffer_pool_mem and buf_pool_mem_size before handing them to MediaBufPoolCreate */
        queueMemory.pMetaDataMemory = ((uint8_t*) queueMemory.pMetaDataMemory) + bufQueueMemSize;
    }

    /* Create media buffer pool */
    pBufQueue->pBufPool = malloc( sizeof( MediaBufPool_t ) );
    if (pBufQueue->pBufPool == NULL)
    {
        return RET_OUTOFMEM;
    }

    ret = MediaBufPoolCreate(pBufQueue->pBufPool,
                             pConfig,
                             queueMemory);

    return ret;
}


/******************************************************************************
 * MediaBufQueueExCreateExtPool
 *****************************************************************************/
RESULT MediaBufQueueExCreateExtPool(MediaBufQueueEx_t*       pBufQueue,
                                            MediaBufQueueExConfig_t* pConfig,
                                            MediaBufQueueExMemory_t  queueMemory,
                                            MediaBufPool_t*          pExtBufPool)
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

    if(queueMemory.pMetaDataMemory  == NULL)
    {
        return RET_INVALID_PARM;
    }

    /* initialize buffer queue object*/
    (void)MEMSET(pBufQueue, 0, (size_t) sizeof(MediaBufQueueEx_t));

    /* initialize SW memory depending on buffer count */
    {
        uint32_t bufQueueMemSize = MediaBufQueueExGetBufQueueMemSize(pConfig);

        /* Take first chunk of memory for queue (pointer array) */
        pBufQueue->pBufArray = (MediaBuffer_t**) queueMemory.pMetaDataMemory;
        (void) MEMSET(pBufQueue->pBufArray, 0, (size_t) bufQueueMemSize);
    }

    /* Connect media buffer pool */
    pBufQueue->pBufPool = pExtBufPool;
    pBufQueue->highWatermark = pExtBufPool->highWatermark; // may be overwritten later on
    pBufQueue->lowWatermark  = pExtBufPool->lowWatermark;  // may be overwritten later on

    /* remember that we're using an external buffer pool */
    pBufQueue->isExtPool = BOOL_TRUE;

    return ret;
}


/******************************************************************************
 * MediaBufQueueExDestroy
 *****************************************************************************/
RESULT MediaBufQueueExDestroy(MediaBufQueueEx_t* pBufQueue)
{
    RESULT ret = RET_SUCCESS;

    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    if (!pBufQueue->isExtPool)
    {
        ret = MediaBufPoolDestroy(pBufQueue->pBufPool);
        free(pBufQueue->pBufPool);
    }
    (void) MEMSET(pBufQueue, 0, sizeof(MediaBufQueueEx_t));

    return ret;
}


/******************************************************************************
 * MediaBufQueueExGetEmptyBuffer
 *****************************************************************************/
MediaBuffer_t* MediaBufQueueExGetEmptyBuffer(MediaBufQueueEx_t *pBufQueue)
{
    MediaBuffer_t* pBuf;

    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBufQueue->pBufPool != NULL);

    pBuf = MediaBufPoolGetBuffer(pBufQueue->pBufPool);

    if (pBuf != NULL)
    {
        pBuf->isFull = BOOL_FALSE;
    }

    return pBuf;
}


/******************************************************************************
 * MediaBufQueueExUpdateAvgFillLevel
 *
 * Note: avgFillLevel is an exponential moving average
 *       expressed as a 16.16 fixed point number.
 *****************************************************************************/
static void MediaBufQueueExUpdateAvgFillLevel(MediaBufQueueEx_t *pBufQueue)
{
    int32_t newFillLevelFixedPoint    = (!pBufQueue->isExtPool) ?
                                        (int32_t) (pBufQueue->pBufPool->fillLevel << 16) :
                                        (int32_t) (pBufQueue->fullBufsAvail << 16);
    int32_t oldAvgFillLevelFixedPoint = (int32_t) pBufQueue->avgFillLevel;
    int32_t newAvgFillLevelFixedPoint;

    newAvgFillLevelFixedPoint = oldAvgFillLevelFixedPoint + ((newFillLevelFixedPoint - oldAvgFillLevelFixedPoint) >> 4);

    pBufQueue->avgFillLevel = (uint32_t) newAvgFillLevelFixedPoint;
}


/******************************************************************************
 * MediaBufQueueExPutFullBuffer
 *****************************************************************************/
RESULT MediaBufQueueExPutFullBuffer(MediaBufQueueEx_t *pBufQueue, MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    if(MediaBufQueueExIsFull(pBufQueue))
    {
        return RET_NOTAVAILABLE;
    }

    pBuf->isFull = BOOL_TRUE;

    if (pBufQueue->isExtPool)
    {
        /* Lock the buffer right here, so that it won't get lost due to activities on other extended queues */
        RESULT lRet = MediaBufQueueExLockBuffer(pBufQueue, pBuf);
        DCT_ASSERT( lRet == RET_SUCCESS);
    }

    MediaBufQueueExInsertInHead(pBufQueue, pBuf);

    /* Update maximum fill level. */
    if (!pBufQueue->isExtPool)
    {
        if (pBufQueue->pBufPool->fillLevel > pBufQueue->maxFillLevel)
        {
            pBufQueue->maxFillLevel = pBufQueue->pBufPool->fillLevel;
        }
    }
    else
    {
        if (pBufQueue->fullBufsAvail > pBufQueue->maxFillLevel)
        {
            pBufQueue->maxFillLevel = pBufQueue->fullBufsAvail;
        }
    }

    /* Update average fill level. */
    MediaBufQueueExUpdateAvgFillLevel(pBufQueue);

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueExPutFullBufferDone
 *****************************************************************************/
RESULT MediaBufQueueExPutFullBufferDone(MediaBufQueueEx_t *pBufQueue, MediaBuffer_t *pBuf)
{
    RESULT result = RET_FAILURE;

    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    if (!pBufQueue->isExtPool)
    {
        result = RET_NOTSUPP;
    }
    else
    {
        /* remove the initial lock by MediaBufQueueExGetEmptyBuffer() */
        result = MediaBufQueueExReleaseBufferInt(pBufQueue, pBuf);
    }

    return result;
}


/******************************************************************************
 * MediaBufQueueExGetFullBuffer
 *****************************************************************************/
MediaBuffer_t* MediaBufQueueExGetFullBuffer(MediaBufQueueEx_t *pBufQueue)
{
    MediaBuffer_t *pBuf;

    DCT_ASSERT(pBufQueue != NULL);

    if(!MediaBufQueueExFullBuffersAvailable(pBufQueue))
    {
        return  NULL;
    }

    pBuf = MediaBufQueueExGetFromTail(pBufQueue);

    DCT_ASSERT(pBuf != NULL);

    if (pBufQueue->isExtPool)
    {
        /* Update average fill level. */
        MediaBufQueueExUpdateAvgFillLevel(pBufQueue);
    }

    return pBuf;
}


/******************************************************************************
 * MediaBufQueueExReleaseBuffer
 *****************************************************************************/
RESULT MediaBufQueueExReleaseBuffer(MediaBufQueueEx_t* pBufQueue,
                                            MediaBuffer_t*     pBuf)
{
    RESULT result;

    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    result = MediaBufQueueExReleaseBufferInt(pBufQueue, pBuf);

    if (result == RET_SUCCESS)
    {
        /* Update average fill level. */
        MediaBufQueueExUpdateAvgFillLevel(pBufQueue);
    }

    return result;
}


/******************************************************************************
 * MediaBufQueueExFlush
 *****************************************************************************/
RESULT MediaBufQueueExFlush(MediaBufQueueEx_t *pBufQueue)
{
    MediaBuffer_t *pBuf;

    DCT_ASSERT(pBufQueue != NULL);

    /* Free all full buffers which have not yet been retrieved */
    while(MediaBufQueueExFullBuffersAvailable(pBufQueue))
    {
        pBuf = MediaBufQueueExGetFullBuffer(pBufQueue);
        if (!pBufQueue->isExtPool)
        {
            while (pBuf->lockCount > 0U)
            {
                MediaBufQueueExReleaseBuffer(pBufQueue, pBuf);
            }
        }
        else
        {
            // if an external buffer pool is used, we can safely remove our own lock
            // and after all queues holding this buffer are flushed the buffer will be
            // fully released thereby as well - except when it is still being locked
            // by processes currently dealing with that buffer. In that case these
            // processes will finally release the buffer by unlocking it.
            DCT_ASSERT( pBuf->lockCount > 0U );
            MediaBufQueueExReleaseBuffer(pBufQueue, pBuf);
        }
    }

    pBufQueue->maxFillLevel = 0;
    pBufQueue->avgFillLevel = 0;

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueExReset
 *****************************************************************************/
RESULT MediaBufQueueExReset(MediaBufQueueEx_t* pBufQueue)
{
    /* Note: this function applies a flush by reset of variables,
       but there's no need to explicitely call the MediaBufQueueExFlush
       function */

    DCT_ASSERT(pBufQueue != NULL);

    /* reset state variables */
    pBufQueue->fullBufsAvail = 0;
    pBufQueue->maxFillLevel  = 0;
    pBufQueue->avgFillLevel  = 0;

    /* reset buffer queue array */
    (void) MEMSET(pBufQueue->pBufArray, 0, (size_t) (pBufQueue->pBufPool->maxBufNum * sizeof(MediaBuffer_t*)));
    pBufQueue->head = 0;
    pBufQueue->tail = 0;

    if (!pBufQueue->isExtPool)
    {
        /* reset buffer pool */
        return MediaBufPoolReset(pBufQueue->pBufPool);
    }
    else
    {
        return RET_SUCCESS;
    }
}


/******************************************************************************
 * MediaBufQueueExSetParameter
 *****************************************************************************/
RESULT MediaBufQueueExSetParameter(MediaBufQueueEx_t*       pBufQueue,
                                           MediaBufQueueExParamId_t paramId,
                                           int32_t                  paramValue)
{
    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }

    if (!pBufQueue->isExtPool)
    {
        return MediaBufPoolSetParameter(pBufQueue->pBufPool, paramId, paramValue);
    }
    else
    {
        RESULT ret = RET_SUCCESS;

        switch (paramId)
        {
            case PARAM_HIGH_WATERMARK:
                pBufQueue->highWatermark = (uint16_t) paramValue;
                break;

            case PARAM_LOW_WATERMARK:
                pBufQueue->lowWatermark  = (uint16_t) paramValue;
                break;

            default:
                ret = RET_INVALID_PARM;
                break;
        }/* end of "switch(param)" */

        return ret;
    }
}


/******************************************************************************
 * MediaBufQueueExGetParameter
 *****************************************************************************/
RESULT MediaBufQueueExGetParameter(MediaBufQueueEx_t*       pBufQueue,
                                           MediaBufQueueExParamId_t paramId,
                                           int32_t*                 pParamValue)
{
    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }

    if (!pBufQueue->isExtPool)
    {
        return MediaBufPoolGetParameter(pBufQueue->pBufPool, paramId, pParamValue);
    }
    else
    {
        RESULT ret = RET_SUCCESS;

        switch (paramId)
        {
            case PARAM_HIGH_WATERMARK:
                *pParamValue = (int32_t) pBufQueue->highWatermark;
                break;

            case PARAM_LOW_WATERMARK:
                *pParamValue = (int32_t) pBufQueue->lowWatermark;
                break;

            default:
                ret = RET_INVALID_PARM;
                break;
        }/* end of "switch(param)" */

        return ret;
    }
}


/******************************************************************************
 * MediaBufQueueExLockBuffer
 *****************************************************************************/
RESULT MediaBufQueueExLockBuffer(MediaBufQueueEx_t* pBufQueue,
                                         MediaBuffer_t*     pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    (void) pBufQueue;

    pBuf->lockCount++;

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueExUnlockBuffer
 *****************************************************************************/
RESULT MediaBufQueueExUnlockBuffer(MediaBufQueueEx_t* pBufQueue,
                                           MediaBuffer_t*     pBuf)
{
    DCT_ASSERT(pBufQueue != NULL);
    DCT_ASSERT(pBuf != NULL);

    /* we may have to release the buffer from the pool as well,
     * simply use central implementation for this               */
    return MediaBufQueueExReleaseBufferInt(pBufQueue, pBuf);
}


/******************************************************************************
 * MediaBufQueueExGetStats
 *****************************************************************************/
RESULT MediaBufQueueExGetStats(MediaBufQueueEx_t*     pBufQueue,
                                       MediaBufQueueExStat_t* pStat)
{
    pStat->currFillLevel  = pBufQueue->pBufPool->fillLevel;
    pStat->maxFillLevel   = pBufQueue->maxFillLevel;
    pStat->avgFillLevel   = pBufQueue->avgFillLevel;
    /* meanTargetArea is expressed as a 16.16 fixed point number, and is lwm + (hwm - lwm)/2. */
    if (!pBufQueue->isExtPool)
    {
        pStat->meanTargetArea = (pBufQueue->pBufPool->lowWatermark << 16) + ((pBufQueue->pBufPool->highWatermark - pBufQueue->pBufPool->lowWatermark) << 15);
    }
    else
    {
        pStat->meanTargetArea = (pBufQueue->lowWatermark << 16) + ((pBufQueue->highWatermark - pBufQueue->lowWatermark) << 15);
    }

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufQueueExRegisterCb
 *****************************************************************************/
RESULT MediaBufQueueExRegisterCb(MediaBufQueueEx_t*        pBufQueue,
                                         MediaBufQueueExCbNotify_t fpCallback,
                                         void*                     pUserContext)
{
    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }

    if (!pBufQueue->isExtPool)
    {
        return MediaBufPoolRegisterCb(pBufQueue->pBufPool, fpCallback, pUserContext);
    }
    else
    {
        RESULT ret = RET_NOTAVAILABLE;
        int32_t i;

        if(fpCallback == NULL)
        {
            return RET_INVALID_PARM;
        }

        for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
        {
            if(pBufQueue->notify[i].fpCallback == NULL)
            {
                pBufQueue->notify[i].fpCallback    = fpCallback;
                pBufQueue->notify[i].pUserContext  = pUserContext;
                ret = RET_SUCCESS;
                break;
            }
        }

        return ret;
    }
}


/******************************************************************************
 * MediaBufQueueExDeregisterCb
 *****************************************************************************/
RESULT MediaBufQueueExDeregisterCb(MediaBufQueueEx_t*        pBufQueue,
                                           MediaBufQueueExCbNotify_t fpCallback)
{
    if(pBufQueue == NULL)
    {
        return RET_WRONG_HANDLE;
    }

    if (!pBufQueue->isExtPool)
    {
        return MediaBufPoolDeregisterCb(pBufQueue->pBufPool, fpCallback);
    }
    else
    {
        RESULT ret = RET_NOTAVAILABLE;
        int32_t i;

        if(fpCallback == NULL)
        {
            return RET_INVALID_PARM;
        }

        for(i = 0; i < MAX_NUM_REGISTERED_CB; i++)
        {
            if(pBufQueue->notify[i].fpCallback == fpCallback)
            {
                pBufQueue->notify[i].fpCallback   = NULL;
                pBufQueue->notify[i].pUserContext = NULL;
                ret = RET_SUCCESS;
                break;
            }
        }

        return ret;
    }
}
