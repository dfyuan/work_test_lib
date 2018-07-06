/******************************************************************************
 *
 * Copyright 2011, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file mipi_drv_rec_cameric.c
 *
 * @brief   Implementation of MIPI Rec driver.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/misc.h>

#include <hal/hal_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_mipi_drv_api.h>

#include "mipi_drv_rec.h"

CREATE_TRACER( MIPI_REC_DEBUG, "MIPI-REC-CAMERIC: ", INFO,    0 );
CREATE_TRACER( MIPI_REC_INFO , "MIPI-REC-CAMERIC: ", INFO,    0 );
CREATE_TRACER( MIPI_REC_WARN , "MIPI-REC-CAMERIC: ", WARNING, 1 );
CREATE_TRACER( MIPI_REC_ERROR, "MIPI-REC-CAMERIC: ", ERROR,   1 );


/******************************************************************************
 * local macro definitions
 *****************************************************************************/


/******************************************************************************
 * local type definitions
 *****************************************************************************/
typedef struct MipiRecContext_s
{
    HalHandle_t         HalHandle;          //!< Handle of HAL session to use.
    uint32_t            HalDevID;           //!< HalDeviceID of the REC.
    MipiConfig_t        MipiConfig;         //!< MIPI config currently in use.
    int                 itfIdx;
    bool_t              Running;            //!< Running state of REC.
    CamerIcDrvHandle_t  CamerIcDrvHandle;   //!< Handle to existing CamerIc driver instance, only required if CamerIc MIPI module is in use, set to NULL otherwise; @note Must be valid for the whole lifetime of this driver instance!
} MipiRecContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/******************************************************************************
 * MipiRecConfigureCamerIc()
 *****************************************************************************/
static RESULT MipiRecConfigureCamerIc
(
    MipiRecContext_t    *pMipiRecCtx,
    MipiConfig_t        *pMipiConfig
);

/////******************************************************************************
//// * MipiRecXxx()
//// *****************************************************************************/
////static RESULT MipiRecXxx
////(
////    MipiRecContext_t    *pMipiRecCtx
////);


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * MipiRecInit()
 *****************************************************************************/
RESULT MipiRecInit
(
    MipiRecHandle_t     *pMipiRecHandle,
    MipiDrvConfig_t     *pMipiDrvConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( (pMipiRecHandle == NULL) || (pMipiDrvConfig == NULL) || (pMipiDrvConfig->HalHandle == NULL) )
    {
        return RET_NULL_POINTER;
    }

    uint32_t HalDevID = 0;
    switch(pMipiDrvConfig->InstanceNum)
    {
        case 0:
            //HalDevID = (pMipiDrvConfig->ItfNum == 0) ? HAL_DEVID_CAM_1A_REC : HAL_DEVID_CAM_1B_REC;
            break;
        case 1:
            //HalDevID = HAL_DEVID_CAM_2_REC;
            break;
        default:
            result = RET_INVALID_PARM;
            goto error_exit;
    }

    // allocate context
    MipiRecContext_t *pMipiRecCtx = malloc( sizeof(MipiRecContext_t) );
    if (pMipiRecCtx == NULL)
    {
        result = RET_OUTOFMEM;
        goto error_exit;
    }

    // pre initialize context
    memset( pMipiRecCtx, 0, sizeof(*pMipiRecCtx) );
    pMipiRecCtx->MipiConfig.NumLanes = 0;
    pMipiRecCtx->Running = false;
    pMipiRecCtx->HalHandle = pMipiDrvConfig->HalHandle;
    pMipiRecCtx->HalDevID = HalDevID;
    pMipiRecCtx->CamerIcDrvHandle = pMipiDrvConfig->CamerIcDrvHandle;
    pMipiRecCtx->itfIdx = pMipiDrvConfig->ItfNum;

    // add HAL reference
    result = HalAddRef( pMipiRecCtx->HalHandle );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_REC_ERROR, "%s: HalAddRef() failed\n", __FUNCTION__ );
        goto cleanup_1;
    }

    // initialize CamerIc MIPI REC
    // -> already done by CamerIcDriverInit() (CAMERIC_MODULE_ID_MASK_MIPI must be specified in CamerIcDrvConfig_t.ModuleMask)

    // success, so return handle
    *pMipiRecHandle = (MipiRecHandle_t)pMipiRecCtx;

    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

cleanup_2: // remove HAL reference
    HalDelRef( pMipiRecCtx->HalHandle );

cleanup_1: // free REC context
    free( pMipiRecCtx );

error_exit: // just return with error
    TRACE( MIPI_REC_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}

/******************************************************************************
 * MipiRecDestroy()
 *****************************************************************************/
RESULT MipiRecDestroy
(
    MipiRecHandle_t     RecHandle
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( RecHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiRecContext_t *pMipiRecCtx = (MipiRecContext_t*) RecHandle;

    // check state
    if (pMipiRecCtx->Running)
    {
        TRACE( MIPI_REC_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // release CamerIc MIPI REC
    // -> will done by CamerIcDriverRelease() later on

    // remove HAL reference
    lres = HalDelRef( pMipiRecCtx->HalHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_REC_ERROR, "%s: HalDelRef() failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_REC_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiRecConfig()
 *****************************************************************************/
extern RESULT MipiRecConfig
(
    MipiRecHandle_t     RecHandle,
    MipiConfig_t        *pMipiConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( (RecHandle == NULL) || (pMipiConfig == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiRecContext_t *pMipiRecCtx = (MipiRecContext_t*) RecHandle;

    // check state
    if (pMipiRecCtx->Running)
    {
        TRACE( MIPI_REC_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // configure CamerIc MIPI REC
    result = MipiRecConfigureCamerIc( pMipiRecCtx , pMipiConfig );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_REC_ERROR, "%s: MipiRecConfigureCamerIc() failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // remember new config
    pMipiRecCtx->MipiConfig = *pMipiConfig;

    // that's it
    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_REC_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiRecStart()
 *****************************************************************************/
RESULT MipiRecStart
(
    MipiRecHandle_t     RecHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( RecHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiRecContext_t *pMipiRecCtx = (MipiRecContext_t*) RecHandle;

    // check state
    if ( pMipiRecCtx->Running || (pMipiRecCtx->MipiConfig.NumLanes == 0) )
    {
        TRACE( MIPI_REC_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // start CamerIc MIPI REC
    result = CamerIcMipiEnable( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiEnable() failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // set new state
    pMipiRecCtx->Running = true;

    // that's it
    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_REC_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiRecStop()
 *****************************************************************************/
RESULT MipiRecStop
(
    MipiRecHandle_t     RecHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    if ( RecHandle == NULL )
    {
        return RET_NULL_POINTER;
    }

    // get context
    MipiRecContext_t *pMipiRecCtx = (MipiRecContext_t*) RecHandle;

    // check state
    if (!pMipiRecCtx->Running)
    {
        TRACE( MIPI_REC_ERROR, "%s: wrong state\n", __FUNCTION__ );
        result = RET_WRONG_STATE;
        goto error_exit;
    }

    // stop CamerIc MIPI REC
    result = CamerIcMipiDisable( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiDisable() failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // set new state
    pMipiRecCtx->Running = false;

    // that's it
    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_REC_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * local functions
 *****************************************************************************/

/******************************************************************************
 * MipiRecConfigureCamerIc()
 *****************************************************************************/
static RESULT MipiRecConfigureCamerIc
(
    MipiRecContext_t    *pMipiRecCtx,
    MipiConfig_t        *pMipiConfig
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );

    // check params
    DCT_ASSERT( pMipiRecCtx != NULL );
    DCT_ASSERT( pMipiConfig != NULL );

    // set configuration
    lres = CamerIcMipiSetNumberOfLanes( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx, pMipiConfig->NumLanes );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiSetNumberOfLanes() failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    lres = CamerIcMipiSetVirtualChannelAndDataType( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx, pMipiConfig->VirtChannel, pMipiConfig->DataType );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiSetVirtualChannelAndDataType() failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    if (pMipiRecCtx->MipiConfig.CompScheme != MIPI_DATA_COMPRESSION_SCHEME_NONE)
    {
        lres = CamerIcMipiSetCompressionSchemeAndPredictorBlock( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx, pMipiConfig->CompScheme, pMipiConfig->PredBlock );
        if (RET_SUCCESS != lres)
        {
            TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiSetCompressionSchemeAndPredictorBlock() failed\n", __FUNCTION__ );
            UPDATE_RESULT( result, lres );
        }

        lres = CamerIcMipiEnableCompressedMode( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx );
        if (RET_SUCCESS != lres)
        {
            TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiEnableCompressedMode() failed\n", __FUNCTION__ );
            UPDATE_RESULT( result, lres );
        }
    }
    else
    {
        lres = CamerIcMipiDisableCompressedMode( pMipiRecCtx->CamerIcDrvHandle, pMipiRecCtx->itfIdx );
        if (RET_SUCCESS != lres)
        {
            TRACE( MIPI_REC_ERROR, "%s: CamerIcMipiDisableCompressedMode() failed\n", __FUNCTION__ );
            UPDATE_RESULT( result, lres );
        }
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit:
    TRACE( MIPI_REC_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/////******************************************************************************
//// * MipiRecXxx()
//// *****************************************************************************/
////static RESULT MipiRecXxx
////(
////    MipiRecContext_t    *pMipiRecCtx
////)
////{
////    RESULT result = RET_SUCCESS;
////
////    TRACE( MIPI_REC_INFO, "%s (enter)\n", __FUNCTION__ );
////
////    // check params
////    DCT_ASSERT( pMipiRecCtx != NULL );
////
////    // do some work
////    //...
////
////
////    TRACE( MIPI_REC_INFO, "%s (exit)\n", __FUNCTION__ );
////
////    return result;
////}
