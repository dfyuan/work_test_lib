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
 * @file cam_engine_drv_api.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <cameric_drv/cameric_drv_api.h>

#include "cam_engine.h"
#include "cam_engine_drv_api.h"

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

/******************************************************************************
 * CamEngineCamerIcMasterId()
 *****************************************************************************/
RESULT CamEngineCamerIcMasterId
(
    CamEngineHandle_t   hCamEngine,
    uint32_t            *revision
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == revision )
    {
        return ( RET_INVALID_PARM );
    }

    result = CamerIcDriverGetRevision( pCamEngineCtx->chain[CHAIN_MASTER].hCamerIc, revision );

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
 
/******************************************************************************
 * CamEngineCamerIcSlaveId()
 *****************************************************************************/
RESULT CamEngineCamerIcSlaveId
(
    CamEngineHandle_t   hCamEngine,
    uint32_t            *revision
)
{
    CamEngineContext_t *pCamEngineCtx = (CamEngineContext_t *)hCamEngine;

    RESULT result = RET_SUCCESS;

    TRACE( CAM_ENGINE_API_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pCamEngineCtx )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == revision )
    {
        return ( RET_INVALID_PARM );
    }

    if ( pCamEngineCtx->chain[CHAIN_SLAVE].hCamerIc )
    {
        result = CamerIcDriverGetRevision( pCamEngineCtx->chain[CHAIN_SLAVE].hCamerIc, revision );
    }
    else
    {
        result = RET_NOTSUPP;
    }

    TRACE( CAM_ENGINE_API_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}
 

