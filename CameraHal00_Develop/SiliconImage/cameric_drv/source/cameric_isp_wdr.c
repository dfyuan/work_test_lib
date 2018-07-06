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
 * @file cameric_isp_wdr.c
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
 * Is the hardware WDR module available ?
 *****************************************************************************/
#if defined(MRV_WDR_VERSION)

/******************************************************************************
 * WDR Module is available.
 *****************************************************************************/

/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_ISP_WDR_DRV_INFO  , "CAMERIC-ISP-WDR-DRV: ", INFO    , 0 );
CREATE_TRACER( CAMERIC_ISP_WDR_DRV_WARN  , "CAMERIC-ISP-WDR-DRV: ", WARNING , 1 );
CREATE_TRACER( CAMERIC_ISP_WDR_DRV_ERROR , "CAMERIC-ISP-WDR-DRV: ", ERROR   , 1 );


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
 * CamerIcIspWdrInit()
 *****************************************************************************/
RESULT CamerIcIspWdrInit
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    ctx->pIspWdrContext= ( CamerIcIspWdrContext_t *)malloc( sizeof(CamerIcIspWdrContext_t) );
    if (  ctx->pIspWdrContext == NULL )
    {
        TRACE( CAMERIC_ISP_WDR_DRV_ERROR,  "%s: Can't allocate CamerIC ISP WDR context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( ctx->pIspWdrContext, 0, sizeof( CamerIcIspWdrContext_t ) );

    ctx->pIspWdrContext->enabled = BOOL_FALSE;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcIspWdrRelease()
 *****************************************************************************/
RESULT CamerIcIspWdrRelease
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->pIspWdrContext == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    MEMSET( ctx->pIspWdrContext, 0, sizeof( CamerIcIspWdrContext_t ) );
    free ( ctx->pIspWdrContext);
    ctx->pIspWdrContext= NULL;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}



/******************************************************************************
 * Implementation of Driver API Functions
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspWdrEnable()
 *****************************************************************************/
RESULT CamerIcIspWdrEnable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspWdrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_wdr_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl) );
        REG_SET_SLICE( isp_wdr_ctrl, MRV_WDR_ENABLE, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl), isp_wdr_ctrl );

        ctx->pIspWdrContext->enabled = BOOL_TRUE;
    }

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspWdrDisable()
 *****************************************************************************/
RESULT CamerIcIspWdrDisable
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspWdrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_wdr_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl) );
        REG_SET_SLICE( isp_wdr_ctrl, MRV_WDR_ENABLE, 0U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl), isp_wdr_ctrl );

        ctx->pIspWdrContext->enabled = BOOL_FALSE;
    }

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspWdrSetRgbOffset()
 *****************************************************************************/
RESULT CamerIcIspWdrSetRgbOffset
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      RgbOffset
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspWdrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (RgbOffset != 0U) && !((RgbOffset << MRV_WDR_RGB_OFFSET_SHIFT) & MRV_WDR_RGB_OFFSET_MASK) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_wdr_offset= HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_offset) );
        REG_SET_SLICE( isp_wdr_offset, MRV_WDR_RGB_OFFSET, RgbOffset );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_offset), isp_wdr_offset );

        ctx->pIspWdrContext->RgbOffset = RgbOffset;
    }

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspWdrSetLumOffset()
 *****************************************************************************/
RESULT CamerIcIspWdrSetLumOffset
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      LumOffset
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspWdrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (LumOffset != 0) && !((LumOffset << MRV_WDR_LUM_OFFSET_SHIFT) & MRV_WDR_LUM_OFFSET_MASK) )
    {
        return ( RET_OUTOFRANGE );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;

        uint32_t isp_wdr_offset= HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_offset) );
        REG_SET_SLICE( isp_wdr_offset, MRV_WDR_LUM_OFFSET, LumOffset );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_offset), isp_wdr_offset );

        ctx->pIspWdrContext->LumOffset = LumOffset;
    }

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * CamerIcIspWdrSetCurve()
 *****************************************************************************/
RESULT CamerIcIspWdrSetCurve
(
    CamerIcDrvHandle_t  handle, 
    uint16_t            *Ym,
    uint8_t             *dY
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    int32_t i, j;

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (ctx == NULL) || (ctx->pIspWdrContext == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl), 0x638 );

    for ( i=0; i<4; i++ )
    {
        uint32_t dYi = 0U;
        for ( j=8; j>0; j-- )
        {
            dYi <<= 4;
            dYi += dY[ (i*8 + j) ];
        }

        if (i == 0)
        {
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_tonecurve_1 ), dYi);
        }
        else if (i == 1)
        {
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_tonecurve_2 ), dYi);
        }
        else if (i == 2)
        {
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_tonecurve_3 ), dYi);
        }
        else  /* if (i == 3) */
        {
            HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_tonecurve_4 ), dYi);
        }
    }

    for ( i=0; i<33; i++ )
    {
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->wdr_tone_mapping_curve_y_block_arr[i] ), Ym[i] );
    }

    uint32_t dYi = 0x00000000;
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_offset    ), dYi );
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_deltamin  ), 0x00100000 );

    /* enable wdr module again */
    if ( BOOL_TRUE == ctx->pIspWdrContext->enabled )
    {
        uint32_t isp_wdr_ctrl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl) );
        REG_SET_SLICE( isp_wdr_ctrl, MRV_WDR_ENABLE, 1U );
        HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->isp_wdr_ctrl), isp_wdr_ctrl );
    }

    TRACE( CAMERIC_ISP_WDR_DRV_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


#else  /* #if defined(MRV_WDR_VERSION) */

/******************************************************************************
 * WDR Module is not available.
 *
 * @note Please remove this stub function to reduce code-size, but we need
 *       this in our validation environment
 * 
 *****************************************************************************/

/******************************************************************************
 * CamerIcIspWdrEnable()
 *****************************************************************************/
RESULT CamerIcIspWdrEnable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspWdrDisable()
 *****************************************************************************/
RESULT CamerIcIspWdrDisable
(
    CamerIcDrvHandle_t handle
)
{
    (void)handle;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspWdrSetRgbOffset()
 *****************************************************************************/
RESULT CamerIcIspWdrSetRgbOffset
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      RgbOffset
)
{
    (void)handle;
    (void)RgbOffset;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspWdrSetLumOffset()
 *****************************************************************************/
RESULT CamerIcIspWdrSetLumOffset
(
    CamerIcDrvHandle_t  handle,
    const uint32_t      LumOffset
)
{
    (void)handle;
    (void)LumOffset;
    return ( RET_NOTSUPP );
}


/******************************************************************************
 * CamerIcIspWdrSetCurve()
 *****************************************************************************/
RESULT CamerIcIspWdrSetCurve
(
    CamerIcDrvHandle_t  handle, 
    uint16_t            *Ym,
    uint8_t             *dY
)
{
    (void)handle;
    (void)Ym;
    (void)dY;
    return ( RET_NOTSUPP );
}


#endif /* #if defined(MRV_WDR_VERSION) */

