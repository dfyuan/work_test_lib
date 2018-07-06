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
 * @file cameric_isp_dpcc.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Does CamerIC contain this module ?
 *****************************************************************************/
#if defined(MRV_DPCC_VERSION)

/******************************************************************************
 * Defect Pixel Cluster Correction Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_DPCC_DRV_INFO  , "CAMERIC-ISP-DPCC-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_DPCC_DRV_WARN  , "CAMERIC-ISP-DPCC-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_DPCC_DRV_ERROR , "CAMERIC-ISP-DPCC-DRV: ", ERROR   , 1 );


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
 * local functions 
 *****************************************************************************/


/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDpccInit()
 *****************************************************************************/
RESULT CamerIcIspDpccInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspDpccContext = ( CamerIcIspDpccContext_t *)malloc( sizeof(CamerIcIspDpccContext_t) );
    if (  ctx->pIspDpccContext == NULL )
    {
        TRACE( CAMERIC_ISP_DPCC_DRV_ERROR,  "%s: Can't allocate CamerIC ISP DPCC context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspDpccContext, 0, sizeof( CamerIcIspDpccContext_t ) );

    ctx->pIspDpccContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspDpccRelease()
 *****************************************************************************/
RESULT CamerIcIspDpccRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspDpccContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspDpccContext, 0, sizeof( CamerIcIspDpccContext_t ) );
    free ( ctx->pIspDpccContext );
    ctx->pIspDpccContext = NULL;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDpccEnable()
 *****************************************************************************/
RESULT CamerIcIspDpccEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpccContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_dpcc_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_mode) );
        REG_SET_SLICE( isp_dpcc_mode, MRV_DPCC_ISP_DPCC_ENABLE, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_mode), isp_dpcc_mode);

        ctx->pIspDpccContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpccDisable()
 *****************************************************************************/
RESULT CamerIcIspDpccDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspDpccContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_dpcc_mode = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_mode) );
        REG_SET_SLICE( isp_dpcc_mode, MRV_DPCC_ISP_DPCC_ENABLE, 0U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_mode), isp_dpcc_mode);

        ctx->pIspDpccContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticConfig()
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticConfig
(
    CamerIcDrvHandle_t          handle,
    CamerIcDpccStaticConfig_t   *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) 
            || (ctx->pIspDpccContext == NULL)
            || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( ctx->pIspDpccContext->enabled == BOOL_TRUE )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_mode),           pConfig->isp_dpcc_mode          );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_output_mode),    pConfig->isp_dpcc_output_mode   );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_set_use),        pConfig->isp_dpcc_set_use       );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_methods_set_1),  pConfig->isp_dpcc_methods_set_1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_methods_set_2),  pConfig->isp_dpcc_methods_set_2 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_methods_set_3),  pConfig->isp_dpcc_methods_set_3 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_line_thresh_1),  pConfig->isp_dpcc_line_thresh_1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_line_mad_fac_1), pConfig->isp_dpcc_line_mad_fac_1);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_pg_fac_1),       pConfig->isp_dpcc_pg_fac_1      );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_thresh_1),   pConfig->isp_dpcc_rnd_thresh_1  );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rg_fac_1),       pConfig->isp_dpcc_rg_fac_1      );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_line_thresh_2),  pConfig->isp_dpcc_line_thresh_2 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_line_mad_fac_2), pConfig->isp_dpcc_line_mad_fac_2);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_pg_fac_2),       pConfig->isp_dpcc_pg_fac_2      );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_thresh_2),   pConfig->isp_dpcc_rnd_thresh_2  );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rg_fac_2),       pConfig->isp_dpcc_rg_fac_2      );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_line_thresh_3),  pConfig->isp_dpcc_line_thresh_3 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_line_mad_fac_3), pConfig->isp_dpcc_line_mad_fac_3);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_pg_fac_3),       pConfig->isp_dpcc_pg_fac_3      );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_thresh_3),   pConfig->isp_dpcc_rnd_thresh_3  );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rg_fac_3),       pConfig->isp_dpcc_rg_fac_3      );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_ro_limits),      pConfig->isp_dpcc_ro_limits     );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_offs),       pConfig->isp_dpcc_rnd_offs      );
    }
 
    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticDemoConfigGain12()
 * for gains between 1..2
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticDemoConfigGain12
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspDpccContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_set_use),        0x00000006 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_thresh_1),   0x00000A0A );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rg_fac_1),       0x00002020 );

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticDemoConfigGain24()
 * for gains between 2..4
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticDemoConfigGain24
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspDpccContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_set_use),        0x00000007 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_thresh_1),   0x00000A0A );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rg_fac_1),       0x00002020 );

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticDemoConfigGain48()
 * for gains between 4..8
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticDemoConfigGain48
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspDpccContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_set_use),        0x0000000F );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rnd_thresh_1),   0x00000804 );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_dpcc_rg_fac_1),       0x00000802 );

    TRACE( CAMERIC_ISP_DPCC_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}

#else  /* #if defined(MRV_DPCC_VERSION) */

/******************************************************************************
 * Defect Pixel Cluster Correction Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 *
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDpccEnable()
 *****************************************************************************/
RESULT CamerIcIspDpccEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpccDisable()
 *****************************************************************************/
RESULT CamerIcIspDpccDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticConfig()
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticConfig
(
    CamerIcDrvHandle_t          handle,
    CamerIcDpccStaticConfig_t   *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticDemoConfigGain12()
 * for gains between 1..2
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticDemoConfigGain12
(
    CamerIcDrvHandle_t  handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticDemoConfigGain24()
 * for gains between 2..4
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticDemoConfigGain24
(
    CamerIcDrvHandle_t  handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDpccSetStaticDemoConfigGain48()
 * for gains between 4..8
 *****************************************************************************/
RESULT CamerIcIspDpccSetStaticDemoConfigGain48
(
    CamerIcDrvHandle_t  handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}

#endif /* #if defined(MRV_DPCC_VERSION) */

