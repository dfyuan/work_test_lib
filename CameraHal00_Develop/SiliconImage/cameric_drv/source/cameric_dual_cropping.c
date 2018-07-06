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
 * @cond    cameric_dual_cropping
 *
 * @file    cameric_dual_cropping.c
 *
 * @brief   This is the implementation of the dual cropping unit implemented 
 *          in the Y/C splitter module.
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>

#include "mrv_all_bits.h"

#include "cameric_drv_cb.h"
#include "cameric_drv.h"
#include "cameric_mi_drv.h"

#include <cameric_drv/cameric_dual_cropping_drv_api.h>

/******************************************************************************
 * Is the dual cropping unit available in Y/C splitter ?
 *****************************************************************************/
#if defined(MRV_DUAL_CROP_VERSION)

/******************************************************************************
 * Dual cropping unit is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_DCROP_DRV_INFO  , "CAMERIC_DCROP_DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_DCROP_DRV_WARN  , "CAMERIC_DCROP_DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_DCROP_DRV_ERROR , "CAMERIC_DCROP_DRV: ", ERROR   , 1 );

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
 * Implementation of Driver Internal API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcDualCropingIsAvailable()
 *****************************************************************************/
RESULT CamerIcDualCropingIsAvailable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_SUCCESS );
}

/******************************************************************************
 * CamerIcIspIsGetOutputWindow()
 *****************************************************************************/
RESULT CamerIcDualCropingGetOutputWindow
(
    CamerIcDrvHandle_t                  handle,
    CamerIcMiPath_t const               path,
    CamerIcDualCropingMode_t * const    pMode,
    CamerIcWindow_t * const             pWin
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_DCROP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (NULL == ctx) || (NULL == ctx->HalHandle) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check path */
    if ( ((CAMERIC_MI_PATH_MAIN != path) && (CAMERIC_MI_PATH_SELF != path)) ||
         (NULL == pWin) || (NULL == pMode)                                  )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow cropping changes if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        
        uint32_t dual_crop_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_ctrl) );

        uint32_t dual_crop_h_offs = 0u;
        uint32_t dual_crop_v_offs = 0u;
        uint32_t dual_crop_h_size = 0u;
        uint32_t dual_crop_v_size = 0u;

        if ( CAMERIC_MI_PATH_MAIN == path )
        {
            dual_crop_h_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_offs) );
            dual_crop_v_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_offs) );
            dual_crop_h_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_size) );
            dual_crop_v_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_size) );
        
            dual_crop_h_offs = REG_GET_SLICE( dual_crop_h_offs, MRV_dual_crop_DUAL_CROP_M_H_OFFS);
            dual_crop_v_offs = REG_GET_SLICE( dual_crop_v_offs, MRV_dual_crop_DUAL_CROP_M_V_OFFS);
            dual_crop_h_size = REG_GET_SLICE( dual_crop_h_size, MRV_dual_crop_DUAL_CROP_M_H_SIZE);
            dual_crop_v_size = REG_GET_SLICE( dual_crop_v_size, MRV_dual_crop_DUAL_CROP_M_V_SIZE);

            *pMode = (CamerIcDualCropingMode_t)(REG_GET_SLICE( dual_crop_h_offs, MRV_dual_crop_CROP_MP_MODE ) + 1u);
        }
        else
        {
            dual_crop_h_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_h_offs) );
            dual_crop_v_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_v_offs) );
            dual_crop_h_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_h_size) );
            dual_crop_v_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_v_size) );
        
            dual_crop_h_offs = REG_GET_SLICE( dual_crop_h_offs, MRV_dual_crop_DUAL_CROP_S_H_OFFS);
            dual_crop_v_offs = REG_GET_SLICE( dual_crop_v_offs, MRV_dual_crop_DUAL_CROP_S_V_OFFS);
            dual_crop_h_size = REG_GET_SLICE( dual_crop_h_size, MRV_dual_crop_DUAL_CROP_S_H_SIZE);
            dual_crop_v_size = REG_GET_SLICE( dual_crop_v_size, MRV_dual_crop_DUAL_CROP_S_V_SIZE);
            
            *pMode = (CamerIcDualCropingMode_t)(REG_GET_SLICE( dual_crop_h_offs, MRV_dual_crop_CROP_SP_MODE ) + 1u);
        }

        pWin->hOffset   = dual_crop_h_offs;
        pWin->vOffset   = dual_crop_v_offs;
        pWin->width     = dual_crop_h_size;
        pWin->height    = dual_crop_v_size;
    }

    TRACE( CAMERIC_DCROP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
} 

/******************************************************************************
 * CamerIcIspIsSetOutputWindow()
 *****************************************************************************/
RESULT CamerIcDualCropingSetOutputWindow
(
    CamerIcDrvHandle_t              handle,
    CamerIcMiPath_t const           path,
    CamerIcDualCropingMode_t const  mode,
    CamerIcWindow_t * const         pWin
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_DCROP_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( (NULL == ctx) || (NULL == ctx->HalHandle) )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* check path */
    if ( ((CAMERIC_MI_PATH_MAIN != path) && (CAMERIC_MI_PATH_SELF != path)) ||
         ((CAMERIC_DCROP_INVALID >= mode) || (CAMERIC_DCROP_MAX <= mode))   ||
         ( NULL == pWin)                                                    )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow cropping changes if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)    &&
         (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t dual_crop_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_ctrl) );

        uint32_t dual_crop_h_offs = 0u;
        uint32_t dual_crop_v_offs = 0u;
        uint32_t dual_crop_h_size = 0u;
        uint32_t dual_crop_v_size = 0u;

        if ( CAMERIC_MI_PATH_MAIN == path )
        {
            dual_crop_h_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_offs) );
            dual_crop_v_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_offs) );
            dual_crop_h_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_size) );
            dual_crop_v_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_size) );
        
            REG_SET_SLICE( dual_crop_h_offs, MRV_dual_crop_DUAL_CROP_M_H_OFFS, pWin->hOffset );
            REG_SET_SLICE( dual_crop_v_offs, MRV_dual_crop_DUAL_CROP_M_V_OFFS, pWin->vOffset );
            REG_SET_SLICE( dual_crop_h_size, MRV_dual_crop_DUAL_CROP_M_H_SIZE, pWin->width   );
            REG_SET_SLICE( dual_crop_v_size, MRV_dual_crop_DUAL_CROP_M_V_SIZE, pWin->height  );

            switch ( mode )
            {
                case CAMERIC_DCROP_YUV:
                    REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_CROP_MP_MODE, 1u  );
                    break;
                
                case CAMERIC_DCROP_RAW:
                    REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_CROP_MP_MODE, 2u  );
                    break;
                
                case CAMERIC_DCROP_BYPASS:
                default:
                    REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_CROP_MP_MODE, 0u  );
                    break;
            }

            // REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_DUAL_CROP_CFG_UPD_PERMANENT, 1u  );
            
            // update with next frame-end
            REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_DUAL_CROP_CFG_UPD, 1u  );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_offs), dual_crop_h_offs);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_offs), dual_crop_v_offs);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_h_size), dual_crop_h_size);
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_m_v_size), dual_crop_v_size);
        }
        else
        {
            dual_crop_h_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_h_offs) );
            dual_crop_v_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_v_offs) );
            dual_crop_h_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_h_size) );
            dual_crop_v_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_v_size) );
        
            REG_SET_SLICE( dual_crop_h_offs, MRV_dual_crop_DUAL_CROP_S_H_OFFS, pWin->hOffset );
            REG_SET_SLICE( dual_crop_v_offs, MRV_dual_crop_DUAL_CROP_S_V_OFFS, pWin->vOffset );
            REG_SET_SLICE( dual_crop_h_size, MRV_dual_crop_DUAL_CROP_S_H_SIZE, pWin->width   );
            REG_SET_SLICE( dual_crop_v_size, MRV_dual_crop_DUAL_CROP_S_V_SIZE, pWin->height  );
      
            switch ( mode )
            {
                case CAMERIC_DCROP_YUV:
                    REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_CROP_SP_MODE, 1u  );
                    break;
                
                case CAMERIC_DCROP_RAW:
                    REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_CROP_SP_MODE, 2u  );
                    break;
                
                case CAMERIC_DCROP_BYPASS:
                default:
                    REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_CROP_SP_MODE, 0u  );
                    break;
            }
            
            // REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_DUAL_CROP_CFG_UPD_PERMANENT, 1u  );

            // update with next frame-end
            REG_SET_SLICE( dual_crop_ctrl, MRV_dual_crop_DUAL_CROP_CFG_UPD, 1u  );

            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_h_offs), dual_crop_h_offs );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_v_offs), dual_crop_v_offs );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_h_size), dual_crop_h_size );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_s_v_size), dual_crop_v_size );
        }

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->dual_crop_ctrl), dual_crop_ctrl );
    }

    TRACE( CAMERIC_DCROP_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
} 

#else  /* #if defined(MRV_DUAL_CROP_VERSION) */

/******************************************************************************
 * Color Processing Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcDualCropingIsAvailable()
 *****************************************************************************/
RESULT CamerIcDualCropingIsAvailable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}

/******************************************************************************
 * CamerIcIspIsGetOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsGetOutputWindow
(
    CamerIcDrvHandle_t      handle,
    CamerIcMiPath_t const   path,
    CamerIcWindow_t * const pWin
)
{
    (void)handle;
    (void)path;
    (void)pWin;
    return ( RET_NOTSUPP );
}

/******************************************************************************
 * CamerIcIspIsSetOutputWindow()
 *****************************************************************************/
RESULT CamerIcDualCropingSetOutputWindow
(
    CamerIcDrvHandle_t      handle,
    CamerIcMiPath_t const   path,
    CamerIcWindow_t * const pWin
)
{
    (void)handle;
    (void)path;
    (void)pWin;
    return ( RET_NOTSUPP );
}

#endif /* #if defined(MRV_DUAL_CROP_VERSION) */

