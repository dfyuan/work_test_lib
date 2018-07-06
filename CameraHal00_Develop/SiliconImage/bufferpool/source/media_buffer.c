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
 * @file media_buffer.c
 *
 * @brief
 *          Media Buffer implementation
 *
 * <pre>
 *
 *   Principal Author: Joerg Detert
 *   Creation date:    Feb 28, 2008
 *
 * </pre>
 *
 *****************************************************************************/
#include <oslayer/oslayer.h>

#include "media_buffer.h"
#include "media_buffer_pool.h"


/******************************************************************************
 * MediaBufInit
 *****************************************************************************/
void MediaBufInit(MediaBuffer_t *pBuf)
{
    DCT_ASSERT(pBuf != NULL);
    
    pBuf->syncPoint        = BOOL_FALSE;
    pBuf->last             = BOOL_FALSE;
    pBuf->lockCount        = 0U;
    pBuf->pOwner           = NULL;
}


/******************************************************************************
 * MediaBufLockBuffer
 *****************************************************************************/
RESULT MediaBufLockBuffer(MediaBuffer_t* pBuf)
{
    DCT_ASSERT(pBuf != NULL);
    DCT_ASSERT(pBuf->pOwner != NULL);

    osAtomicIncrement( &pBuf->lockCount );

    return RET_SUCCESS;
}


/******************************************************************************
 * MediaBufUnlockBuffer
 *****************************************************************************/
RESULT MediaBufUnlockBuffer(MediaBuffer_t* pBuf)
{
    DCT_ASSERT(pBuf != NULL);
	if (pBuf->pOwner == NULL)
	{
		return RET_FAILURE;
	}

    uint32_t val = osAtomicDecrement( &pBuf->lockCount );

    if(val == 0U)
    {
        MediaBufPoolFreeBuffer(pBuf->pOwner, pBuf);
    }

    return RET_SUCCESS;
}
