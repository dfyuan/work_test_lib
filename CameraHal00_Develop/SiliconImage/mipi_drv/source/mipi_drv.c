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
 * @file mipi_drv.c
 *
 * @brief   Implementation of MIPI driver.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include "mipi_drv.h"
#include "mipi_drv_phy.h"

////#include "mrv_all_bits.h"

////#include "cameric_drv_cb.h"
////#include "cameric_drv.h"


CREATE_TRACER( MIPI_DRV_INFO  , "MIPI-DRV: ", INFO   , 0 );
CREATE_TRACER( MIPI_DRV_WARN  , "MIPI-DRV: ", WARNING, 1 );
CREATE_TRACER( MIPI_DRV_ERROR , "MIPI-DRV: ", ERROR  , 1 );


/******************************************************************************
 * local macro definitions
 *****************************************************************************/


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
 * MipiDrvCheckConfig()
 *****************************************************************************/
RESULT MipiDrvCheckConfig
(
    MipiDrvContext_t    *pMipiDrvCtx,
    MipiConfig_t        *pMipiConfig
);

/******************************************************************************
 * MipiDrvDoConfig()
 *****************************************************************************/
RESULT MipiDrvDoConfig
(
    MipiDrvContext_t    *pMipiDrvCtx,
    MipiConfig_t        *pMipiConfig
);

/******************************************************************************
 * MipiDrvDoStart()
 *****************************************************************************/
RESULT MipiDrvDoStart
(
    MipiDrvContext_t    *pMipiDrvCtx
);

/******************************************************************************
 * MipiDrvDoStop()
 *****************************************************************************/
RESULT MipiDrvDoStop
(
    MipiDrvContext_t    *pMipiDrvCtx
);


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * MipiDrvCreate()
 *****************************************************************************/
RESULT MipiDrvCreate
(
    MipiDrvContext_t    *pMipiDrvCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    if ((pMipiDrvCtx == NULL) || (pMipiDrvCtx->Config.HalHandle == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // add HAL reference
    result = HalAddRef( pMipiDrvCtx->Config.HalHandle );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: adding HAL reference failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // connect to MIPI REC
    result = MipiRecInit( &pMipiDrvCtx->RecHandle, &pMipiDrvCtx->Config );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: initializing MIPI REC failed\n", __FUNCTION__ );
        goto cleanup_1;
    }

    // connect to MIPI PHY
    result = MipiPhyInit( &pMipiDrvCtx->PhyHandle, &pMipiDrvCtx->Config );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: initializing MIPI PHY failed\n", __FUNCTION__ );
        goto cleanup_2;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

cleanup_2: // destroy REC instance
    MipiRecDestroy( pMipiDrvCtx->RecHandle );

cleanup_1: // remove HAL reference
    HalDelRef( pMipiDrvCtx->Config.HalHandle );

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvDestroy()
 *****************************************************************************/
RESULT MipiDrvDestroy
(
    MipiDrvContext_t    *pMipiDrvCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMipiDrvCtx != NULL );

    // disconnect from MIPI PHY
    lres = MipiPhyDestroy( pMipiDrvCtx->PhyHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_DRV_ERROR, "%s: shutting down MIPI PHY failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // disconnect from MIPI REC
    lres = MipiRecDestroy( pMipiDrvCtx->RecHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_DRV_ERROR, "%s: shutting down MIPI REC failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // remove HAL reference
    lres = HalDelRef( pMipiDrvCtx->Config.HalHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_DRV_ERROR, "%s: removing HAL reference failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvCmd()
 *****************************************************************************/
RESULT MipiDrvCmd
(
    MipiDrvContext_t    *pMipiDrvCtx,
    MipiDrvCmd_t        *pCmd
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMipiDrvCtx != NULL );
    DCT_ASSERT( pCmd != NULL );

    switch (pCmd->ID)
    {
        case MIPI_DRV_CMD_START:
            // do something
            result = MipiDrvDoStart( pMipiDrvCtx );
            if (RET_SUCCESS != result)
            {
                TRACE( MIPI_DRV_ERROR, "%s: starting MIPI failed\n", __FUNCTION__ );
                goto error_exit;
            }
            break;
        case MIPI_DRV_CMD_STOP:
            // do something
            result = MipiDrvDoStop( pMipiDrvCtx );
            if (RET_SUCCESS != result)
            {
                TRACE( MIPI_DRV_ERROR, "%s: stopping MIPI failed\n", __FUNCTION__ );
                goto error_exit;
            }
            break;
        case MIPI_DRV_CMD_CONFIG:
            // do something
            result = MipiDrvDoConfig( pMipiDrvCtx, pCmd->params.config.pMipiConfig );
            if (RET_SUCCESS != result)
            {
                TRACE( MIPI_DRV_ERROR, "%s: stopping MIPI failed\n", __FUNCTION__ );
                goto error_exit;
            }
            break;
        default:
            result = RET_INVALID_PARM;
            goto error_exit;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * local functions
 *****************************************************************************/

/******************************************************************************
 * MipiDrvDoConfig()
 *****************************************************************************/
RESULT MipiDrvDoConfig
(
    MipiDrvContext_t    *pMipiDrvCtx,
    MipiConfig_t        *pMipiConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMipiDrvCtx != NULL );
    DCT_ASSERT( pMipiConfig != NULL );

    // common sanity checks for new config
    result = MipiDrvIsConfigValid( pMipiConfig );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: invalid MIPI config\n", __FUNCTION__, result );
        goto error_exit;
    }

    // store new config
    pMipiDrvCtx->MipiConfig = *pMipiConfig;

    // configure MIPI REC
    result = MipiRecConfig( pMipiDrvCtx->RecHandle, pMipiConfig );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: configuring MIPI REC failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // configure MIPI PHY
    result = MipiPhyConfig( pMipiDrvCtx->PhyHandle, pMipiConfig );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: configuring MIPI PHY faile)\n", __FUNCTION__ );
        goto cleanup_1;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

cleanup_1: // stop REC
    MipiRecStop( pMipiDrvCtx->RecHandle );

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvDoStart()
 *****************************************************************************/
RESULT MipiDrvDoStart
(
    MipiDrvContext_t    *pMipiDrvCtx
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMipiDrvCtx != NULL );

    // start MIPI REC
    result = MipiRecStart( pMipiDrvCtx->RecHandle );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: starting MIPI REC failed\n", __FUNCTION__ );
        goto error_exit;
    }

    // start MIPI PHY
    result = MipiPhyStart( pMipiDrvCtx->PhyHandle );
    if (RET_SUCCESS != result)
    {
        TRACE( MIPI_DRV_ERROR, "%s: starting MIPI PHY faile)\n", __FUNCTION__ );
        goto cleanup_1;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

cleanup_1: // stop REC
    MipiRecStop( pMipiDrvCtx->RecHandle );

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvDoStop()
 *****************************************************************************/
RESULT MipiDrvDoStop
(
    MipiDrvContext_t    *pMipiDrvCtx
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMipiDrvCtx != NULL );

    // stop MIPI PHY
    lres = MipiPhyStop( pMipiDrvCtx->PhyHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_DRV_ERROR, "%s: stopping MIPI PHY failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // stop MIPI REC
    lres = MipiRecStop( pMipiDrvCtx->RecHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE( MIPI_DRV_ERROR, "%s: stopping MIPI REC failed\n", __FUNCTION__ );
        UPDATE_RESULT( result, lres );
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}


/******************************************************************************
 * MipiDrvIsConfigValid()
 *****************************************************************************/
RESULT MipiDrvIsConfigValid
(
    MipiConfig_t *pMipiConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE( MIPI_DRV_INFO, "%s (enter)\n", __FUNCTION__ );

    DCT_ASSERT( pMipiConfig != NULL );

    // check NumLanes
    if (pMipiConfig->NumLanes > MAX_NUMBER_OF_MIPI_LANES)
    {
        TRACE( MIPI_DRV_ERROR, "%s: invalid number of lanes\n", __FUNCTION__ );
        result = RET_INVALID_PARM;
        goto error_exit;
    }

    // check VirtChannel
    switch (pMipiConfig->VirtChannel)
    {
        case MIPI_VIRTUAL_CHANNEL_0:
        case MIPI_VIRTUAL_CHANNEL_1:
        case MIPI_VIRTUAL_CHANNEL_2:
        case MIPI_VIRTUAL_CHANNEL_3:
            break;

        default:
            TRACE( MIPI_DRV_ERROR, "%s: invalid virtual channel\n", __FUNCTION__ );
            result = RET_INVALID_PARM;
            goto error_exit;
    }

        // check DataType
    switch (pMipiConfig->DataType)
    {
        case MIPI_DATA_TYPE_FSC:
        case MIPI_DATA_TYPE_FEC:
        case MIPI_DATA_TYPE_LSC:
        case MIPI_DATA_TYPE_LEC:
            TRACE( MIPI_DRV_ERROR, "%s: un-supported data type\n", __FUNCTION__ );
            result = RET_NOTSUPP;
            goto error_exit;

        case MIPI_DATA_TYPE_GSPC1:
        case MIPI_DATA_TYPE_GSPC2:
        case MIPI_DATA_TYPE_GSPC3:
        case MIPI_DATA_TYPE_GSPC4:
        case MIPI_DATA_TYPE_GSPC5:
        case MIPI_DATA_TYPE_GSPC6:
        case MIPI_DATA_TYPE_GSPC7:
        case MIPI_DATA_TYPE_GSPC8:
            TRACE( MIPI_DRV_ERROR, "%s: un-supported data type\n", __FUNCTION__ );
            result = RET_NOTSUPP;
            goto error_exit;

        case MIPI_DATA_TYPE_NULL:
        case MIPI_DATA_TYPE_BLANKING:
        case MIPI_DATA_TYPE_EMBEDDED:
            TRACE( MIPI_DRV_ERROR, "%s: un-supported data type\n", __FUNCTION__ );
            result = RET_NOTSUPP;
            goto error_exit;

        case MIPI_DATA_TYPE_YUV420_8:
        case MIPI_DATA_TYPE_YUV420_10:
        case MIPI_DATA_TYPE_LEGACY_YUV420_8:
        case MIPI_DATA_TYPE_YUV420_8_CSPS:
        case MIPI_DATA_TYPE_YUV420_10_CSPS:
        case MIPI_DATA_TYPE_YUV422_8:
        case MIPI_DATA_TYPE_YUV422_10:
            break;

        case MIPI_DATA_TYPE_RGB444:
        case MIPI_DATA_TYPE_RGB555:
        case MIPI_DATA_TYPE_RGB565:
        case MIPI_DATA_TYPE_RGB666:
        case MIPI_DATA_TYPE_RGB888:
            break;

        case MIPI_DATA_TYPE_RAW_6:
        case MIPI_DATA_TYPE_RAW_7:
        case MIPI_DATA_TYPE_RAW_8:
        case MIPI_DATA_TYPE_RAW_10:
        case MIPI_DATA_TYPE_RAW_12:
        case MIPI_DATA_TYPE_RAW_14:
            break;

        case MIPI_DATA_TYPE_USER_1:
        case MIPI_DATA_TYPE_USER_2:
        case MIPI_DATA_TYPE_USER_3:
        case MIPI_DATA_TYPE_USER_4:
        case MIPI_DATA_TYPE_USER_5:
        case MIPI_DATA_TYPE_USER_6:
        case MIPI_DATA_TYPE_USER_7:
        case MIPI_DATA_TYPE_USER_8:
            TRACE( MIPI_DRV_ERROR, "%s: un-supported data type\n", __FUNCTION__ );
            result = RET_NOTSUPP;
            goto error_exit;

        default:
            TRACE( MIPI_DRV_ERROR, "%s: invalid data type\n", __FUNCTION__ );
            result = RET_INVALID_PARM;
            goto error_exit;
    }

    // check CompScheme
    switch (pMipiConfig->CompScheme)
    {
        case MIPI_DATA_COMPRESSION_SCHEME_NONE:
            break;

        case MIPI_DATA_COMPRESSION_SCHEME_12_8_12:
        case MIPI_DATA_COMPRESSION_SCHEME_12_7_12:
        case MIPI_DATA_COMPRESSION_SCHEME_12_6_12:
        case MIPI_DATA_COMPRESSION_SCHEME_10_8_10:
        case MIPI_DATA_COMPRESSION_SCHEME_10_7_10:
        case MIPI_DATA_COMPRESSION_SCHEME_10_6_10:
        {
            // check PredBlock
            switch (pMipiConfig->PredBlock)
            {
                case MIPI_PREDICTOR_BLOCK_INVALID:
                    TRACE( MIPI_DRV_ERROR, "%s: invalid predictor block\n", __FUNCTION__ );
                    result = RET_INVALID_PARM;
                    goto error_exit;

                case MIPI_PREDICTOR_BLOCK_1:
                case MIPI_PREDICTOR_BLOCK_2:
                    break;

                default:
                    TRACE( MIPI_DRV_ERROR, "%s: invalid predictor block\n", __FUNCTION__ );
                    result = RET_INVALID_PARM;
                    goto error_exit;
            }

            break;
        }

        default:
            TRACE( MIPI_DRV_ERROR, "%s: invalid compression scheme\n", __FUNCTION__ );
            result = RET_INVALID_PARM;
            goto error_exit;
    }

    // that's it
    if (RET_SUCCESS != result)
    {
        goto error_exit;
    }

    TRACE( MIPI_DRV_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;

error_exit: // just return with error
    TRACE( MIPI_DRV_INFO, "%s: (exit, result=%d)\n", __FUNCTION__, result );

    return result;
}
