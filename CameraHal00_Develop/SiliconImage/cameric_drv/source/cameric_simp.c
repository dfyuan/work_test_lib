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
 * @cond    cameric_simp
 *
 * @file    cameric_simp.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"


/******************************************************************************
 * Is the hardware Super-Impose module available ?
 *****************************************************************************/
#if defined(MRV_SUPER_IMPOSE_VERSION)

/******************************************************************************
 * Super-Impose Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_SIMP_DRV_INFO  , "CAMERIC-SIMP-DRV: ", INFO     , 0 );
CREATE_TRACER( CAMERIC_SIMP_DRV_WARN  , "CAMERIC-SIMP-DRV: ", WARNING  , 1 );
CREATE_TRACER( CAMERIC_SIMP_DRV_ERROR , "CAMERIC-SIMP-DRV: ", ERROR    , 1 );


/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************/
/**
 * @brief   This functions resets the CamerIC Super-Impose module. 
 *
 * @param   ctx                 Driver contecxt for getting HalHandle to access
 *                              hardware
 * @param   stay_in_reset       if TRUE than the IE hardware module stays in 
 *                              reset state
 *
 * @return                      Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeded
 * @retval  RET_WRONG_HANDLE    handle is invalid
 *
 *****************************************************************************/
static RESULT CamerIcSimpReset
(
    CamerIcDrvContext_t *ctx,
    const bool_t        stay_in_reset
)
{
    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t vi_ircl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl) );
        REG_SET_SLICE( vi_ircl, MRV_VI_SIMP_SOFT_RST, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );

        if ( BOOL_FALSE == stay_in_reset )
        {
            REG_SET_SLICE( vi_ircl, MRV_VI_SIMP_SOFT_RST, 0 );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_ircl), vi_ircl );
        }
    }

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcSimpEnableClock()
 *****************************************************************************/
RESULT CamerIcSimpEnableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) || (ctx->pSimpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_SIMP_CLK_ENABLE, 1 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );
        
        result = CamerIcSimpReset( ctx, BOOL_FALSE );
    }

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcSimpDisableClock()
 *****************************************************************************/
RESULT CamerIcSimpDisableClock
(
    CamerIcDrvHandle_t  handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t vi_iccl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl) );

        REG_SET_SLICE( vi_iccl, MRV_VI_SIMP_CLK_ENABLE, 0 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_iccl), vi_iccl );

        result = CamerIcSimpReset( ctx, BOOL_TRUE );
    }

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcSimpInit()
 *****************************************************************************/
RESULT CamerIcSimpInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pSimpContext = (CamerIcSimpContext_t *)malloc( sizeof(CamerIcSimpContext_t) );
    if (  ctx->pSimpContext == NULL )
    {
        TRACE( CAMERIC_SIMP_DRV_ERROR,
                "%s: Can't allocate CamerIC SIMP context.\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pSimpContext, 0, sizeof(CamerIcSimpContext_t) );

    result = CamerIcSimpEnableClock( ctx );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_SIMP_DRV_ERROR,
                "%s: Failed to enable clock for CamerIC SIMP module.\n",  __FUNCTION__ );
        return ( result );
    }

    ctx->pSimpContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcSimpRelease()
 *****************************************************************************/
RESULT CamerIcSimpRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pSimpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pSimpContext, 0, sizeof(CamerIcSimpContext_t) );
    free ( ctx->pSimpContext );
    ctx->pSimpContext = NULL;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcSimpSignal()
 *****************************************************************************/
void CamerIcSimpSignal
(
    const CamerIcIspSimpSignal_t    signal,
    CamerIcDrvHandle_t              handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pSimpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return;
    }
    else
    {
        switch ( signal )
        {
            case CAMERIC_SIMP_SIGNAL_END_OF_FRAME:
                {
                    volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
                    uint32_t super_imp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_ctrl) );

                    if ( ctx->pSimpContext->enabled == BOOL_TRUE  )
                    {
                        if ( (super_imp_ctrl & MRV_SI_BYPASS_MODE_MASK) == MRV_SI_BYPASS_MODE_BYPASS )
                        {
                            REG_SET_SLICE( super_imp_ctrl, MRV_SI_BYPASS_MODE, MRV_SI_BYPASS_MODE_PROCESS );
                            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_ctrl), super_imp_ctrl );
                        }
                    }
                    else
                    {
                        if ( (super_imp_ctrl & MRV_SI_BYPASS_MODE_MASK) == MRV_SI_BYPASS_MODE_PROCESS )
                        {
                            REG_SET_SLICE( super_imp_ctrl, MRV_SI_BYPASS_MODE, MRV_SI_BYPASS_MODE_BYPASS );
                            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_ctrl), super_imp_ctrl );
                        }
                    }
                }

            default:
                {
                    break;
                }

        }
    }               

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );
}


/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcSimpEnable()
 *****************************************************************************/
RESULT CamerIcSimpEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pSimpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t super_imp_ctrl     = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_ctrl) );
        uint32_t super_imp_offset_x = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_offset_x) );
        uint32_t super_imp_offset_y = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_offset_y) );
        uint32_t super_imp_color_y  = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_color_y) );
        uint32_t super_imp_color_cb = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_color_cb) );
        uint32_t super_imp_color_cr = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_color_cr) );

        switch ( ctx->pSimpContext->config.TransparencyMode )
        {
            case CAMERIC_SIMP_TRANSPARENCY_MODE_ENABLED:
                {
                    REG_SET_SLICE( super_imp_ctrl, MRV_SI_TRANSPARENCY_MODE, MRV_SI_TRANSPARENCY_MODE_ENABLED );
                    break;
                }

            case CAMERIC_SIMP_TRANSPARENCY_MODE_DISABLED:
                {
                    REG_SET_SLICE( super_imp_ctrl, MRV_SI_TRANSPARENCY_MODE, MRV_SI_TRANSPARENCY_MODE_DISABLED );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_SIMP_DRV_ERROR, "%s: unsopported transparency mode(%d)\n",
                            __FUNCTION__, ctx->pSimpContext->config.TransparencyMode );
                    return ( RET_NOTSUPP );
                }
        }

        switch ( ctx->pSimpContext->config.ReferenceMode )
        {
            case CAMERIC_SIMP_REFERENCE_MODE_FROM_MEMORY:
                {
                    REG_SET_SLICE( super_imp_ctrl, MRV_SI_REF_IMAGE, MRV_SI_REF_IMAGE_MEM );
                    break;
                }

            case CAMERIC_SIMP_REFERENCE_MODE_FROM_CAMERA:
                {
                    REG_SET_SLICE( super_imp_ctrl, MRV_SI_REF_IMAGE, MRV_SI_REF_IMAGE_IE );
                    break;
                }

            default:
                {
                    TRACE( CAMERIC_SIMP_DRV_ERROR, "%s: unsopported reference mode(%d)\n",
                            __FUNCTION__, ctx->pSimpContext->config.ReferenceMode );
                    return ( RET_NOTSUPP );
                }
        }

       //  REG_SET_SLICE( super_imp_ctrl, MRV_SI_BYPASS_MODE, MRV_SI_BYPASS_MODE_PROCESS );

        REG_SET_SLICE( super_imp_offset_x, MRV_SI_OFFSET_X, ctx->pSimpContext->config.OffsetX );
        REG_SET_SLICE( super_imp_offset_y, MRV_SI_OFFSET_Y, ctx->pSimpContext->config.OffsetY );
        REG_SET_SLICE( super_imp_color_y,  MRV_SI_Y_COMP,   ctx->pSimpContext->config.Y );
        REG_SET_SLICE( super_imp_color_cb, MRV_SI_CB_COMP,  ctx->pSimpContext->config.Cb );
        REG_SET_SLICE( super_imp_color_cr, MRV_SI_CR_COMP,  ctx->pSimpContext->config.Cr );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_offset_x), super_imp_offset_x );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_offset_y), super_imp_offset_y );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_color_y), super_imp_color_y );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_color_cb), super_imp_color_cb );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_color_cr), super_imp_color_cr );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->super_imp_ctrl), super_imp_ctrl );

        ctx->pSimpContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcSimpDisable()
 *****************************************************************************/
RESULT CamerIcSimpDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pSimpContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        //volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        //uint32_t super_imp_ctrl     = HalReadReg( ctx->HalHandle, ((uint32_t)&ptCamerIcRegMap->super_imp_ctrl) );
        //REG_SET_SLICE( super_imp_ctrl, MRV_SI_BYPASS_MODE, MRV_SI_BYPASS_MODE_BYPASS );
        //HalWriteReg( ctx->HalHandle, ((uint32_t)&ptCamerIcRegMap->super_imp_ctrl), super_imp_ctrl );

        ctx->pSimpContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcSimpIsEnabled()
 *****************************************************************************/
RESULT CamerIcSimpIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pSimpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pSimpContext->enabled;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcSimpConfigure()
 *****************************************************************************/
RESULT CamerIcSimpConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcSimpConfig_t     *pConfig
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pSimpContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pConfig == NULL )
    {
        return ( RET_NULL_POINTER );
    }
    
    if ( ctx->pSimpContext->enabled == BOOL_TRUE )
    {
        return ( RET_BUSY );
    }

    if ( (pConfig->TransparencyMode <= CAMERIC_SIMP_TRANSPARENCY_MODE_INVALID) 
            || (pConfig->TransparencyMode >= CAMERIC_SIMP_TRANSPARENCY_MODE_MAX) )
    {
        TRACE( CAMERIC_SIMP_DRV_ERROR,
                "%s: unsopported overlay mode(%d)\n", __FUNCTION__, pConfig->TransparencyMode );
        return ( RET_NOTSUPP );
    }

    if ( (pConfig->ReferenceMode <= CAMERIC_SIMP_REFERENCE_MODE_INVALID)
            || (pConfig->ReferenceMode >= CAMERIC_SIMP_REFERENCE_MODE_MAX) )
    {
        TRACE( CAMERIC_SIMP_DRV_ERROR,
                "%s: unsopported reference image(%d)\n", __FUNCTION__, pConfig->ReferenceMode );
        return ( RET_NOTSUPP );
    }

    if ( pConfig->OffsetX > MRV_SI_OFFSET_X_MASK )
    {
        TRACE( CAMERIC_SIMP_DRV_ERROR, "%s: x-offset out of range (%d)\n", __FUNCTION__, pConfig->OffsetX );
        return ( RET_OUTOFRANGE );
    }

    if ( pConfig->OffsetY > MRV_SI_OFFSET_Y_MASK )
    {
        TRACE( CAMERIC_SIMP_DRV_ERROR, "%s: y-offset out of range (%d)\n", __FUNCTION__, pConfig->OffsetY );
        return ( RET_OUTOFRANGE );
    }

    MEMCPY( &ctx->pSimpContext->config, pConfig, sizeof( CamerIcSimpConfig_t ) );

    TRACE( CAMERIC_SIMP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_SUPER_IMPOSE_VERSION) */

/******************************************************************************
 * Super-Impose Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcSimpEnable()
 *****************************************************************************/
RESULT CamerIcSimpEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcSimpDisable()
 *****************************************************************************/
RESULT CamerIcSimpDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcSimpIsEnabled()
 *****************************************************************************/
RESULT CamerIcSimpIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    (void)handle;
    (void)pIsEnabled;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcSimpConfigure()
 *****************************************************************************/
RESULT CamerIcSimpConfigure
(
    CamerIcDrvHandle_t      handle,
    CamerIcSimpConfig_t     *pConfig
)
{
    (void)handle;
    (void)pConfig;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_SUPER_IMPOSE_VERSION) */

