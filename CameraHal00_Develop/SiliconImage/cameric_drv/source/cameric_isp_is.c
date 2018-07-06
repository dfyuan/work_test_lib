/******************************************************************************
 *
 * Copyright 2012, Dream Chip Technologies GmbH. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Dream Chip Technologies GmbH, Steinriede 10, 30827 Garbsen / Berenbostel,
 * Germany
 *
 *****************************************************************************/
/**
 * @file cameric_isp_vs.c
 *
 * @brief
 *   Implementation of the video stabilization measurement driver.
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
 * Is the hardware Video Stabilization Measurement module available ?
 *****************************************************************************/
#if defined(MRV_IS_VERSION)

/******************************************************************************
 * Video Stabilization module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_IS_DRV_INFO  , "CAMERIC-ISP-IS-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_IS_DRV_WARN  , "CAMERIC-ISP-IS-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_IS_DRV_ERROR , "CAMERIC-ISP-IS-DRV: ", ERROR   , 1 );

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
 * CamerIcIspIsInit()
 *****************************************************************************/
RESULT CamerIcIspIsInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspIsContext = ( CamerIcIspIsContext_t *)malloc( sizeof(CamerIcIspIsContext_t) );
    if ( ctx->pIspIsContext == NULL )
    {
        TRACE( CAMERIC_ISP_IS_DRV_ERROR,  "%s: Can't allocate CamerIC ISP IS context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspIsContext, 0, sizeof(CamerIcIspIsContext_t) );

    ctx->pIspIsContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsRelease()
 *****************************************************************************/
RESULT CamerIcIspIsRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspIsContext, 0, sizeof(CamerIcIspIsContext_t) );
    free ( ctx->pIspIsContext );
    ctx->pIspIsContext = NULL;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/


/******************************************************************************
 * CamerIcIspIsEnable()
 *****************************************************************************/
RESULT CamerIcIspIsEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_ctrl;

        /* enable image stabilization functionality;
         * note that input/output functionality is always enabled, independent
         * of the content of the control register */

        isp_is_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_ctrl) );

        REG_SET_SLICE( isp_is_ctrl, MRV_IS_IS_EN, 1 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_ctrl), isp_is_ctrl);

        ctx->pIspIsContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsDisable()
 *****************************************************************************/
RESULT CamerIcIspIsDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_ctrl;

        /* disable image stabilization functionality;
         * note that input/output functionality is always enabled, independent
         * of the content of the control register */

        isp_is_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_ctrl) );

        REG_SET_SLICE( isp_is_ctrl, MRV_IS_IS_EN, 0 );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_ctrl), isp_is_ctrl);

        ctx->pIspIsContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspIsIsEnabled
(
    CamerIcDrvHandle_t      handle,
    bool_t                  *pIsEnabled
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pIsEnabled )
    {
        return ( RET_NULL_POINTER );
    }

    *pIsEnabled = ctx->pIspIsContext->enabled;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsSetRecenterExponent()
 *****************************************************************************/
RESULT CamerIcIspIsSetRecenterExponent
(
    CamerIcDrvHandle_t handle,
    uint8_t            recenterExp
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( recenterExp >= MRV_IS_IS_RECENTER_MAX )
    {
        return ( RET_WRONG_CONFIG );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_recenter;

        isp_is_recenter = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_recenter) );

        REG_SET_SLICE( isp_is_recenter, MRV_IS_IS_RECENTER, recenterExp );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_recenter), isp_is_recenter);

        ctx->pIspIsContext->recenterExp = recenterExp;
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsGetRecenterExponent()
 *****************************************************************************/
RESULT CamerIcIspIsGetRecenterExponent
(
    CamerIcDrvHandle_t  handle,
    uint8_t            *pRecenterExp
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pRecenterExp )
    {
        return ( RET_NULL_POINTER );
    }

    *pRecenterExp = ctx->pIspIsContext->recenterExp;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsSetOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsSetOutputWindow
(
    CamerIcDrvHandle_t    handle,
    CamerIcWindow_t *     pOutWin,
    bool_t                force_upd
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pOutWin )
    {
        return ( RET_NULL_POINTER );
    }

    if ( (pOutWin->hOffset > MRV_IS_IS_H_OFFS_MAX) ||
         (pOutWin->vOffset > MRV_IS_IS_V_OFFS_MAX) ||
         (pOutWin->width   > MRV_IS_IS_H_SIZE_MAX) ||
         (pOutWin->height  > MRV_IS_IS_V_SIZE_MAX) )
    {
        return ( RET_WRONG_CONFIG );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_h_offs;
        uint32_t isp_is_v_offs;
        uint32_t isp_is_h_size;
        uint32_t isp_is_v_size;
        
        isp_is_h_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_offs) );
        isp_is_v_offs = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_offs) );
        isp_is_h_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_size) );
        isp_is_v_size = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_size) );

        REG_SET_SLICE( isp_is_h_offs, MRV_IS_IS_H_OFFS, pOutWin->hOffset );
        REG_SET_SLICE( isp_is_v_offs, MRV_IS_IS_V_OFFS, pOutWin->vOffset );
        REG_SET_SLICE( isp_is_h_size, MRV_IS_IS_H_SIZE, pOutWin->width   );
        REG_SET_SLICE( isp_is_v_size, MRV_IS_IS_V_SIZE, pOutWin->height  );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_offs), isp_is_h_offs);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_offs), isp_is_v_offs);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_size), isp_is_h_size);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_size), isp_is_v_size);

        if ( BOOL_TRUE == force_upd )
        {
            uint32_t isp_ctrl;

            isp_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl) );
            REG_SET_SLICE( isp_ctrl, MRV_ISP_ISP_CFG_UPD, 1 );
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_ctrl), isp_ctrl);
        }

        ctx->pIspIsContext->outWin = *pOutWin;
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsGetOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsGetOutputWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pOutWin
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pOutWin )
    {
        return ( RET_NULL_POINTER );
    }

    *pOutWin = ctx->pIspIsContext->outWin;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsGetDisplacedOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsGetDisplacedOutputWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pDisplWin
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pDisplWin )
    {
        return ( RET_NULL_POINTER );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_h_offs_shd;
        uint32_t isp_is_v_offs_shd;
        uint32_t isp_is_h_size_shd;
        uint32_t isp_is_v_size_shd;

        isp_is_h_offs_shd = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_offs_shd) );
        isp_is_v_offs_shd = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_offs_shd) );
        isp_is_h_size_shd = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_h_size_shd) );
        isp_is_v_size_shd = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_v_size_shd) );

        pDisplWin->hOffset = (uint16_t) REG_GET_SLICE( isp_is_h_offs_shd, MRV_IS_IS_H_OFFS_SHD );
        pDisplWin->vOffset = (uint16_t) REG_GET_SLICE( isp_is_v_offs_shd, MRV_IS_IS_V_OFFS_SHD );
        pDisplWin->width   = (uint16_t) REG_GET_SLICE( isp_is_h_size_shd, MRV_IS_ISP_H_SIZE_SHD );
        pDisplWin->height  = (uint16_t) REG_GET_SLICE( isp_is_v_size_shd, MRV_IS_ISP_V_SIZE_SHD );
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsSetMaxDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsSetMaxDisplacement
(
    CamerIcDrvHandle_t    handle,
    uint16_t              maxDisplHor,
    uint16_t              maxDisplVer
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (maxDisplHor > MRV_IS_IS_MAX_DX_MAX) ||
         (maxDisplVer > MRV_IS_IS_MAX_DY_MAX) )
    {
        return ( RET_WRONG_CONFIG );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_max_dx;
        uint32_t isp_is_max_dy;

        isp_is_max_dx = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_max_dx) );
        isp_is_max_dy = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_max_dy) );

        REG_SET_SLICE( isp_is_max_dx, MRV_IS_IS_MAX_DX, maxDisplHor );
        REG_SET_SLICE( isp_is_max_dy, MRV_IS_IS_MAX_DY, maxDisplVer );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_max_dx), isp_is_max_dx);
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_max_dy), isp_is_max_dy);

        ctx->pIspIsContext->maxDisplHor = maxDisplHor;
        ctx->pIspIsContext->maxDisplVer = maxDisplVer;
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsGetMaxDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsGetMaxDisplacement
(
    CamerIcDrvHandle_t    handle,
    uint16_t             *pMaxDisplHor,
    uint16_t             *pMaxDisplVer
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pMaxDisplHor) ||
         (NULL == pMaxDisplVer) )
    {
        return ( RET_NULL_POINTER );
    }

    *pMaxDisplHor = ctx->pIspIsContext->maxDisplHor;
    *pMaxDisplVer = ctx->pIspIsContext->maxDisplVer;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}




/******************************************************************************
 * CamerIcIspIsSetDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsSetDisplacement
(
    CamerIcDrvHandle_t    handle,
    int16_t               displHor,
    int16_t               displVer
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (abs(displHor) > ctx->pIspIsContext->maxDisplHor) ||
         (abs(displVer) > ctx->pIspIsContext->maxDisplVer) )
    {
        return ( RET_WRONG_CONFIG );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_is_displace;
        int32_t displHor32, displVer32;

        /* Sign extension to 32 bit */
        displHor32 = displHor;
        displVer32 = displVer;

        isp_is_displace = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_displace) );

        REG_SET_SLICE( isp_is_displace, MRV_IS_DX, (uint32_t) displHor32 );
        REG_SET_SLICE( isp_is_displace, MRV_IS_DY, (uint32_t) displVer32 );

        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_is_max_dx), isp_is_displace);

        ctx->pIspIsContext->displHor = displHor;
        ctx->pIspIsContext->displVer = displVer;
    }

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspIsGetDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsGetDisplacement
(
    CamerIcDrvHandle_t    handle,
    int16_t              *pDisplHor,
    int16_t              *pDisplVer
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    if ( (ctx == NULL) || (ctx->pIspIsContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (NULL == pDisplHor) ||
         (NULL == pDisplVer) )
    {
        return ( RET_NULL_POINTER );
    }

    *pDisplHor = ctx->pIspIsContext->displHor;
    *pDisplVer = ctx->pIspIsContext->displVer;

    TRACE( CAMERIC_ISP_IS_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



#else  /* #if defined(MRV_IS_VERSION)  */


/******************************************************************************
 * CamerIcIspIsEnable()
 *****************************************************************************/
RESULT CamerIcIspIsEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsDisable()
 *****************************************************************************/
RESULT CamerIcIspIsDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsIsEnabled()
 *****************************************************************************/
RESULT CamerIcIspIsIsEnabled
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
 * CamerIcIspIsSetRecenterExponent()
 *****************************************************************************/
RESULT CamerIcIspIsSetRecenterExponent
(
    CamerIcDrvHandle_t handle,
    uint8_t            recenterExp
)
{
    (void)handle;
    (void)recenterExp;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsGetRecenterExponent()
 *****************************************************************************/
RESULT CamerIcIspIsGetRecenterExponent
(
    CamerIcDrvHandle_t  handle,
    uint8_t            *pRecenterExp
)
{
    (void)handle;
    (void)pRecenterExp;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsSetOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsSetOutputWindow
(
    CamerIcDrvHandle_t    handle,
    CamerIcWindow_t      *pOutWin
)
{
    (void)handle;
    (void)pOutWin;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsGetOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsGetOutputWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pOutWin
)
{
    (void)handle;
    (void)pOutWin;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsGetDisplacedOutputWindow()
 *****************************************************************************/
RESULT CamerIcIspIsGetDisplacedOutputWindow
(
    CamerIcDrvHandle_t  handle,
    CamerIcWindow_t    *pDisplWin
)
{
    (void)handle;
    (void)pDisplWin;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsSetMaxDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsSetMaxDisplacement
(
    CamerIcDrvHandle_t    handle,
    uint16_t              maxDisplHor,
    uint16_t              maxDisplVer
)
{
    (void)handle;
    (void)maxDisplHor;
    (void)maxDisplVer;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsGetMaxDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsGetMaxDisplacement
(
    CamerIcDrvHandle_t    handle,
    uint16_t             *pMaxDisplHor,
    uint16_t             *pMaxDisplVer
)
{
    (void)handle;
    (void)pMaxDisplHor;
    (void)pMaxDisplVer;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsSetDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsSetDisplacement
(
    CamerIcDrvHandle_t    handle,
    int16_t               displHor,
    int16_t               displVer
)
{
    (void)handle;
    (void)displHor;
    (void)displVer;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspIsGetDisplacement()
 *****************************************************************************/
RESULT CamerIcIspIsGetDisplacement
(
    CamerIcDrvHandle_t    handle,
    int16_t              *pDisplHor,
    int16_t              *pDisplVer
)
{
    (void)handle;
    (void)pDisplHor;
    (void)pDisplVer;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_IS_VERSION)  */

