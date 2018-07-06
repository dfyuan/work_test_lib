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
 * @file cameric_isp_degamma.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/list.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is Degamma (Gamma In) Module available ?
 *****************************************************************************/
#if defined(MRV_GAMMA_IN_VERSION)

/******************************************************************************
 * Degamma (Gamma In) Module is available
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_DEGAMMA_DRV_INFO  , "CAMERIC-ISP-DEGAMMA-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_DEGAMMA_DRV_WARN  , "CAMERIC-ISP-DEGAMMA-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_DEGAMMA_DRV_ERROR , "CAMERIC-ISP-DEGAMMA-DRV: ", ERROR   , 1 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/
#define REG_GET_DEGAMMA_DX_( reg, i )   REG_GET_SLICE( reg, MRV_ISP_GAMMA_DX_##i )


/******************************************************************************
 * local functions 
 *****************************************************************************/
static void CamerIcIspDegammaReadOutDeGammaCurve
(
    CamerIcDrvContext_t *ctx
)
{
    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( ctx->pIspDegammaContext != NULL )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        CamerIcIspDegammaCurve_t *pCurve = &ctx->pIspDegammaContext->curve;

        uint32_t i;

        uint32_t isp_gamma_dx_lo = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_dx_lo) );
        uint32_t isp_gamma_dx_hi = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_dx_hi) );
        
        pCurve->segment[0U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_1 );
        pCurve->segment[1U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_2 );
        pCurve->segment[2U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_3 );
        pCurve->segment[3U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_4 );
        pCurve->segment[4U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_5 );
        pCurve->segment[5U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_6 );
        pCurve->segment[6U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_7 );
        pCurve->segment[7U] = REG_GET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_8 );
        
        pCurve->segment[ 8U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_9 );
        pCurve->segment[ 9U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_10 );
        pCurve->segment[10U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_11 );
        pCurve->segment[11U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_12 );
        pCurve->segment[12U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_13 );
        pCurve->segment[13U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_14 );
        pCurve->segment[14U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_15 );
        pCurve->segment[15U] = REG_GET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_16 );

        for ( i=0L; i<CAMERIC_DEGAMMA_CURVE_SIZE; i++ )
        {
            uint32_t red    = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->degamma_r_y_block_arr[i]) );
            uint32_t green  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->degamma_g_y_block_arr[i]) );
            uint32_t blue   = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->degamma_b_y_block_arr[i]) );

            pCurve->red[i]      = (uint16_t)red;
            pCurve->green[i]    = (uint16_t)green;
            pCurve->blue[i]     = (uint16_t)blue;
        }
    }
        
    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);
}



/******************************************************************************
 * Implementation of Driver internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDegammaInit()
 *****************************************************************************/
RESULT CamerIcIspDegammaInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspDegammaContext = ( CamerIcIspDegammaContext_t *)malloc( sizeof(CamerIcIspDegammaContext_t) );
    if (  ctx->pIspDegammaContext == NULL )
    {
        TRACE( CAMERIC_ISP_DEGAMMA_DRV_ERROR,  "%s: Can't allocate CamerIC ISP DEGAMMA context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspDegammaContext, 0, sizeof( CamerIcIspDegammaContext_t ) );

    ctx->pIspDegammaContext->enabled = BOOL_FALSE;

    CamerIcIspDegammaReadOutDeGammaCurve( ctx );

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDegammaRelease()
 *****************************************************************************/
RESULT CamerIcIspDegammaRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspDegammaContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspDegammaContext, 0, sizeof( CamerIcIspDegammaContext_t ) );
    free ( ctx->pIspDegammaContext );
    ctx->pIspDegammaContext = NULL;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDegammaEnable()
 *****************************************************************************/
RESULT CamerIcIspDegammaEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspDegammaContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIspDegammaContext->enabled == BOOL_TRUE )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_GAMMA_IN_ENABLE, MRV_ISP_ISP_GAMMA_IN_ENABLE_ON );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );
    
        ctx->pIspDegammaContext->enabled = BOOL_TRUE;
    }


    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDegammaDisable()
 *****************************************************************************/
RESULT CamerIcIspDegammaDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspDegammaContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIspDegammaContext->enabled == BOOL_FALSE )
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
        REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_GAMMA_IN_ENABLE, MRV_ISP_ISP_GAMMA_IN_ENABLE_OFF );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl );
    
        ctx->pIspDegammaContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDegammaIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspDegammaIsEnabled
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->pIspDegammaContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pIsEnabled == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    *pIsEnabled = ctx->pIspDegammaContext->enabled;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDegammaGetCurve()
 *****************************************************************************/
RESULT CamerIcIspDegammaGetCurve
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspDegammaCurve_t    *pCurve
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check preconditions */
    if ( (ctx == NULL) || (ctx->pIspDegammaContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCurve == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        CamerIcIspDegammaReadOutDeGammaCurve( ctx );
        MEMCPY( pCurve, &ctx->pIspDegammaContext->curve, sizeof( CamerIcIspDegammaCurve_t ) ); 
    }

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspDegammaSetCurve()
 *****************************************************************************/
RESULT CamerIcIspDegammaSetCurve
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspDegammaCurve_t    *pCurve
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check preconditions */
    if ( pCurve == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    if ( (ctx == NULL) || (ctx->pIspDegammaContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( ctx->pIspDegammaContext->enabled == BOOL_TRUE )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        
        uint32_t i;
    
        uint32_t isp_gamma_dx_lo = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_dx_lo) );
        uint32_t isp_gamma_dx_hi = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_dx_hi) );
        
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_1,  pCurve->segment[0U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_2,  pCurve->segment[1U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_3,  pCurve->segment[2U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_4,  pCurve->segment[3U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_5,  pCurve->segment[4U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_6,  pCurve->segment[5U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_7,  pCurve->segment[6U] );
        REG_SET_SLICE( isp_gamma_dx_lo, MRV_ISP_GAMMA_DX_8,  pCurve->segment[7U] );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_dx_lo), isp_gamma_dx_lo );
                                                        
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_9,  pCurve->segment[ 8U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_10, pCurve->segment[ 9U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_11, pCurve->segment[10U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_12, pCurve->segment[11U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_13, pCurve->segment[12U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_14, pCurve->segment[13U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_15, pCurve->segment[14U] );
        REG_SET_SLICE( isp_gamma_dx_hi, MRV_ISP_GAMMA_DX_16, pCurve->segment[15U] );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_gamma_dx_hi), isp_gamma_dx_hi );

        for ( i=0L; i<CAMERIC_DEGAMMA_CURVE_SIZE; i++ )
        {
            uint32_t red    = (uint32_t)pCurve->red[i];
            uint32_t green  = (uint32_t)pCurve->green[i];
            uint32_t blue   = (uint32_t)pCurve->blue[i];

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->degamma_r_y_block_arr[i]), red );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->degamma_g_y_block_arr[i]), green );        
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->degamma_b_y_block_arr[i]), blue );
        }

        MEMCPY( &ctx->pIspDegammaContext->curve, pCurve, sizeof( CamerIcIspDegammaCurve_t ) ); 
    }

    TRACE( CAMERIC_ISP_DEGAMMA_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_GAMMA_IN_VERSION) */ 

/******************************************************************************
 * Degamma (Gamma In) Module is not available
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 *
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspDegammaEnable()
 *****************************************************************************/
RESULT CamerIcIspDegammaEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDegammaDisable()
 *****************************************************************************/
RESULT CamerIcIspDegammaDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDegammaIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspDegammaIsEnabled
(
    CamerIcDrvHandle_t  handle,
    bool_t              *pIsEnabled
)
{
    (void)handle;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDegammaGetCurve()
 *****************************************************************************/
RESULT CamerIcIspDegammaGetCurve
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspDegammaCurve_t    *pCurve
)
{
    (void)handle;
    (void)pCurve;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspDegammaSetCurve()
 *****************************************************************************/
RESULT CamerIcIspDegammaSetCurve
(
    CamerIcDrvHandle_t          handle,
    CamerIcIspDegammaCurve_t    *pCurve
)
{
    (void)handle;
    (void)pCurve;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_GAMMA_IN_VERSION) */

