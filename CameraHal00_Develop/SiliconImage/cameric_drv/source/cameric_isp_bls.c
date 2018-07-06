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
 * @file cameric_isp_bls.c
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
 * Is Black Level Module available ?
 *****************************************************************************/
#if defined(MRV_BLACK_LEVEL_VERSION)

/******************************************************************************
 * Black Level Substraction Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_BLS_DRV_INFO  , "CAMERIC-ISP-BLS-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_BLS_DRV_WARN  , "CAMERIC-ISP-BLS-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_BLS_DRV_ERROR , "CAMERIC-ISP-BLS-DRV: ", ERROR   , 1 );


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
 * Implementation of Driver internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspBlsInit()
 *****************************************************************************/
RESULT CamerIcIspBlsInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_bls_ctrl;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check driver context (precondition) */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* create a bls module context */
    ctx->pIspBlsContext = ( CamerIcIspBlsContext_t *)malloc( sizeof(CamerIcIspBlsContext_t) );
    if (  ctx->pIspBlsContext == NULL )
    {
        TRACE( CAMERIC_ISP_BLS_DRV_ERROR,  "%s: Can't allocate CamerIC ISP BLS context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspBlsContext, 0, sizeof( CamerIcIspBlsContext_t ) );
    
    /* make sure that hardware module is disabled */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );
    REG_SET_SLICE( isp_bls_ctrl, MRV_BLS_BLS_ENABLE, MRV_BLS_BLS_ENABLE_BYPASS );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );

    ctx->pIspBlsContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsRelease()
 *****************************************************************************/
RESULT CamerIcIspBlsRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_bls_ctrl;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* make sure that hardware module is disabled */
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );
    REG_SET_SLICE( isp_bls_ctrl, MRV_BLS_BLS_ENABLE, MRV_BLS_BLS_ENABLE_BYPASS );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );

    /* release the module context */
    MEMSET( ctx->pIspBlsContext, 0, sizeof( CamerIcIspBlsContext_t ) );
    free ( ctx->pIspBlsContext );
    ctx->pIspBlsContext = NULL;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspBlsEnable()
 *****************************************************************************/
RESULT CamerIcIspBlsEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );
        REG_SET_SLICE( isp_bls_ctrl, MRV_BLS_BLS_ENABLE, MRV_BLS_BLS_ENABLE_PROCESS );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );

        ctx->pIspBlsContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsDisable()
 *****************************************************************************/
RESULT CamerIcIspBlsDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );
        REG_SET_SLICE( isp_bls_ctrl, MRV_BLS_BLS_ENABLE, MRV_BLS_BLS_ENABLE_BYPASS );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );

        ctx->pIspBlsContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspBlsIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspBlsContext->enabled;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsSetSubstractionMode()
 *****************************************************************************/
RESULT CamerIcIspBlsSetSubstractionMode
(
    CamerIcDrvHandle_t          handle,
    const CamerIcIspBlsMode_t   submode
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        uint32_t isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );

        switch ( submode )
        {
            case CAMERIC_ISP_BLS_MODE_USE_FIX_VALUES:
                {
                    REG_SET_SLICE( isp_bls_ctrl, MRV_BLS_BLS_MODE, MRV_BLS_BLS_MODE_FIX );
                    break;
                }

            case CAMERIC_ISP_BLS_MODE_USE_MEASURED_VALUES:
                {
                    REG_SET_SLICE( isp_bls_ctrl, MRV_BLS_BLS_MODE, MRV_BLS_BLS_MODE_MEAS );
                    break;
                }

            default:
                {
                    return ( RET_OUTOFRANGE );
                }
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsSetBlackLevel()
 *****************************************************************************/
RESULT CamerIcIspBlsSetBlackLevel
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      isp_bls_a_fixed,
    const uint16_t      isp_bls_b_fixed,
    const uint16_t      isp_bls_c_fixed,
    const uint16_t      isp_bls_d_fixed
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (isp_bls_a_fixed & ~MRV_BLS_BLS_A_FIXED_MASK)
            || (isp_bls_b_fixed & ~MRV_BLS_BLS_B_FIXED_MASK)
            || (isp_bls_c_fixed & ~MRV_BLS_BLS_C_FIXED_MASK)
            || (isp_bls_d_fixed & ~MRV_BLS_BLS_D_FIXED_MASK) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        if ( BOOL_TRUE == ctx->pIspBlsContext->enabled )
        {
            CamerIcIspBlsDisable( handle );
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_a_fixed), (uint32_t)isp_bls_a_fixed );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_b_fixed), (uint32_t)isp_bls_b_fixed );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_c_fixed), (uint32_t)isp_bls_c_fixed );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_d_fixed), (uint32_t)isp_bls_d_fixed );
        
        ctx->pIspBlsContext->isp_bls_a_fixed = isp_bls_a_fixed; 
        ctx->pIspBlsContext->isp_bls_b_fixed = isp_bls_b_fixed;
        ctx->pIspBlsContext->isp_bls_c_fixed = isp_bls_c_fixed;
        ctx->pIspBlsContext->isp_bls_d_fixed = isp_bls_d_fixed;

        if ( BOOL_FALSE == ctx->pIspBlsContext->enabled )
        {
            CamerIcIspBlsEnable( handle );
        }
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsGetBlackLevel()
 *****************************************************************************/
RESULT CamerIcIspBlsGetBlackLevel
(
    CamerIcDrvHandle_t  handle,
    uint16_t            *isp_bls_a_fixed,
    uint16_t            *isp_bls_b_fixed,
    uint16_t            *isp_bls_c_fixed,
    uint16_t            *isp_bls_d_fixed
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (isp_bls_a_fixed == NULL) || (isp_bls_b_fixed == NULL) 
            || (isp_bls_c_fixed == NULL) || (isp_bls_d_fixed == NULL) )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        *isp_bls_a_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_a_fixed) );
        *isp_bls_b_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_b_fixed) );
        *isp_bls_c_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_c_fixed) );
        *isp_bls_d_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_d_fixed) );
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspBlsSetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  width,
    const uint16_t                  height
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    CamerIcWindow_t *pWindow;

    uint32_t isp_bls_h_start = 0UL;
    uint32_t isp_bls_h_stop  = 0UL;
    uint32_t isp_bls_v_start = 0UL;
    uint32_t isp_bls_v_stop  = 0UL;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

    switch ( WdwId )
    {
        case CAMERIC_ISP_BLS_WINDOW_1:
            {
                if ( (x > MRV_BLS_BLS_H1_START_MAX)
                        || (width > MRV_BLS_BLS_H1_STOP_MAX)
                        || (y > MRV_BLS_BLS_V1_START_MAX)
                        || (height  > MRV_BLS_BLS_V1_STOP_MAX) )
                {
                    return ( RET_OUTOFRANGE );
                }

                REG_SET_SLICE( isp_bls_h_start, MRV_BLS_BLS_H1_START, x );
                REG_SET_SLICE( isp_bls_h_stop,  MRV_BLS_BLS_H1_STOP , width );
                REG_SET_SLICE( isp_bls_v_start, MRV_BLS_BLS_V1_START, y );
                REG_SET_SLICE( isp_bls_v_stop,  MRV_BLS_BLS_V1_STOP , height );

                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_h1_start), isp_bls_h_start );
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_h1_stop ), isp_bls_h_stop );
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_v1_start), isp_bls_v_start );
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_v1_stop ), isp_bls_v_stop );
        
                pWindow = &ctx->pIspBlsContext->Window1;

                break;
            }
        
        case CAMERIC_ISP_BLS_WINDOW_2:
            {
                if ( (x > MRV_BLS_BLS_H2_START_MAX)
                        || (width > MRV_BLS_BLS_H2_STOP_MAX)
                        || (y > MRV_BLS_BLS_V2_START_MAX)
                        || (height  > MRV_BLS_BLS_V2_STOP_MAX) )
                {
                    return ( RET_OUTOFRANGE );
                }
                
                REG_SET_SLICE( isp_bls_h_start, MRV_BLS_BLS_H2_START, x );
                REG_SET_SLICE( isp_bls_h_stop,  MRV_BLS_BLS_H2_STOP , width );
                REG_SET_SLICE( isp_bls_v_start, MRV_BLS_BLS_V2_START, y );
                REG_SET_SLICE( isp_bls_v_stop,  MRV_BLS_BLS_V2_STOP , height );
 
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_h2_start), isp_bls_h_start );
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_h2_stop ), isp_bls_h_stop );
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_v2_start), isp_bls_v_start );
                HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_v2_stop ), isp_bls_v_stop );
        
                pWindow = &ctx->pIspBlsContext->Window2;
                
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }

    SetCamerIcWindow( pWindow, x, y, width, height );

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsEnableMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspBlsEnableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_bls_ctrl;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );

    switch ( WdwId )
    {
        case CAMERIC_ISP_BLS_WINDOW_1:
            {
                if ( (ctx->pIspBlsContext->Window1.hOffset > MRV_BLS_BLS_H1_START_MAX)
                        || (ctx->pIspBlsContext->Window1.width > MRV_BLS_BLS_H1_STOP_MAX)
                        || (ctx->pIspBlsContext->Window1.vOffset > MRV_BLS_BLS_V1_START_MAX)
                        || (ctx->pIspBlsContext->Window1.height  > MRV_BLS_BLS_V1_STOP_MAX) )
                {
                    return ( RET_OUTOFRANGE );
                }
                
                REG_SET_SLICE(isp_bls_ctrl,  MRV_BLS_WINDOW_ENABLE_WND1, 1 );
                break;
            }
        
        case CAMERIC_ISP_BLS_WINDOW_2:
            {
                if ( (ctx->pIspBlsContext->Window2.hOffset > MRV_BLS_BLS_H2_START_MAX)
                        || (ctx->pIspBlsContext->Window2.width > MRV_BLS_BLS_H2_STOP_MAX)
                        || (ctx->pIspBlsContext->Window2.vOffset > MRV_BLS_BLS_V2_START_MAX)
                        || (ctx->pIspBlsContext->Window2.height  > MRV_BLS_BLS_V2_STOP_MAX) )
                {
                    return ( RET_OUTOFRANGE );
                }
                
                REG_SET_SLICE(isp_bls_ctrl,  MRV_BLS_WINDOW_ENABLE_WND2, 1 );
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }
    
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsMeasuringWindowIsEnabled()
 *****************************************************************************/
bool_t CamerIcIspBlsMeasuringWindowIsEnabled
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_bls_ctrl;

    bool_t result = BOOL_FALSE;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( BOOL_FALSE );
    }
    
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );

    switch ( WdwId )
    {
        case CAMERIC_ISP_BLS_WINDOW_1:
            {
                result = REG_GET_SLICE( isp_bls_ctrl,  MRV_BLS_WINDOW_ENABLE_WND1 ) ? BOOL_TRUE : BOOL_FALSE;
                break;
            }
        
        case CAMERIC_ISP_BLS_WINDOW_2:
            {
                result = REG_GET_SLICE( isp_bls_ctrl,  MRV_BLS_WINDOW_ENABLE_WND2 ) ? BOOL_TRUE : BOOL_FALSE;
                break;
            }

        default:
            {
                result = BOOL_FALSE;
                break;
            }
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcIspBlsMeasuringWindowIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspBlsDisableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;
    
    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t isp_bls_ctrl;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    
    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    isp_bls_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl) );

    switch ( WdwId )
    {
        case CAMERIC_ISP_BLS_WINDOW_1:
            {
                REG_SET_SLICE(isp_bls_ctrl,  MRV_BLS_WINDOW_ENABLE_WND1, 0 );
                break;
            }
        
        case CAMERIC_ISP_BLS_WINDOW_2:
            {
                REG_SET_SLICE(isp_bls_ctrl,  MRV_BLS_WINDOW_ENABLE_WND2, 0 );
                break;
            }

        default:
            {
                return ( RET_INVALID_PARM );
            }
    }
    
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_ctrl), isp_bls_ctrl );

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspBlsGetMeasuredBlackLevel()
 *****************************************************************************/
RESULT CamerIcIspBlsGetMeasuredBlackLevel
(
    CamerIcDrvHandle_t  handle,
    uint16_t            *isp_bls_a_fixed,
    uint16_t            *isp_bls_b_fixed,
    uint16_t            *isp_bls_c_fixed,
    uint16_t            *isp_bls_d_fixed
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspBlsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (isp_bls_a_fixed == NULL) || (isp_bls_b_fixed == NULL) 
            || (isp_bls_c_fixed == NULL) || (isp_bls_d_fixed == NULL) )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        *isp_bls_a_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_a_measured) );
        *isp_bls_b_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_b_measured) );
        *isp_bls_c_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_c_measured) );
        *isp_bls_d_fixed = (uint16_t)HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_bls_d_measured) );
    }

    TRACE( CAMERIC_ISP_BLS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}

#else  /* if defined(MRV_BLACK_LEVEL_VERSION) */

/******************************************************************************
 * Black Level Substraction Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspBlsDisable()
 *****************************************************************************/
RESULT CamerIcIspBlsEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspBlsDisable()
 *****************************************************************************/
RESULT CamerIcIspBlsDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspBlsIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspBlsIsEnabled
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
 * CamerIcIspBlsSetSubstractionMode()
 *****************************************************************************/
RESULT CamerIcIspBlsSetSubstractionMode
(
    CamerIcDrvHandle_t          handle,
    const CamerIcIspBlsMode_t   submode
)
{
    (void)handle;
    (void)submode;
    return ( RET_NOTSUPP );
}



/******************************************************************************
 * CamerIcIspBlsSetBlackLevel()
 *****************************************************************************/
RESULT CamerIcIspBlsSetBlackLevel
(
    CamerIcDrvHandle_t  handle,
    const uint16_t      isp_bls_a_fixed,
    const uint16_t      isp_bls_b_fixed,
    const uint16_t      isp_bls_c_fixed,
    const uint16_t      isp_bls_d_fixed
)
{
    (void)handle;
    (void)isp_bls_a_fixed;
    (void)isp_bls_b_fixed;
    (void)isp_bls_c_fixed;
    (void)isp_bls_d_fixed;
    return ( RET_NOTSUPP );
}



/******************************************************************************
 * CamerIcIspBlsGetBlackLevel()
 *****************************************************************************/
RESULT CamerIcIspBlsGetBlackLevel
(
    CamerIcDrvHandle_t  handle,
    uint16_t            *isp_bls_a_fixed,
    uint16_t            *isp_bls_b_fixed,
    uint16_t            *isp_bls_c_fixed,
    uint16_t            *isp_bls_d_fixed
)
{
    (void)handle;
    (void)isp_bls_a_fixed;
    (void)isp_bls_b_fixed;
    (void)isp_bls_c_fixed;
    (void)isp_bls_d_fixed;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspBlsSetMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspBlsSetMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId,
    const uint16_t                  x,
    const uint16_t                  y,
    const uint16_t                  width,
    const uint16_t                  height
)
{
    (void)handle;
    (void)WdwId;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspBlsEnableMeasuringWindow()
 *****************************************************************************/
RESULT CamerIcIspBlsEnableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId
)
{
    (void)handle;
    (void)WdwId;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspBlsMeasuringWindowIsEnabled()
 *****************************************************************************/
bool_t CamerIcIspBlsMeasuringWindowIsEnabled
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId
)
{
    (void)handle;
    (void)WdwId;
    return ( BOOL_FALSE );
}


/******************************************************************************
 * CamerIcIspBlsMeasuringWindowIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspBlsDisableMeasuringWindow
(
    CamerIcDrvHandle_t              handle,
    const CamerIcIspBlsWindowId_t   WdwId
)
{
    (void)handle;
    (void)WdwId;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspBlsGetMeasuredBlackLevel()
 *****************************************************************************/
RESULT CamerIcIspBlsGetMeasuredBlackLevel
(
    CamerIcDrvHandle_t  handle,
    uint16_t            *isp_bls_a_fixed,
    uint16_t            *isp_bls_b_fixed,
    uint16_t            *isp_bls_c_fixed,
    uint16_t            *isp_bls_d_fixed
)
{
    (void)handle;
    (void)isp_bls_a_fixed;
    (void)isp_bls_b_fixed;
    (void)isp_bls_c_fixed;
    (void)isp_bls_d_fixed;
    return ( RET_NOTSUPP );
}

#endif /* MRV_BLACK_LEVEL_VERSION */

