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
 * @file cameric.c
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
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( CAMERIC_DRV_INFO  , "CAMERIC-DRV: ", INFO     , 0 );
CREATE_TRACER( CAMERIC_DRV_WARN  , "CAMERIC-DRV: ", WARNING  , 1 );
CREATE_TRACER( CAMERIC_DRV_ERROR , "CAMERIC-DRV: ", ERROR    , 1 );



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
 * CamerIcDriverFeastureMask()
 *****************************************************************************/
RESULT CamerIcDriverFeastureMask
(
    uint32_t *CamerIcFeatureMask
)
{
    RESULT result = RET_SUCCESS;

    *CamerIcFeatureMask = 0;
    *CamerIcFeatureMask |= ( CAMERIC_FEATURE_ID_MASK_PARALLEL_ITF
                                | CAMERIC_FEATURE_ID_MASK_SMIA_ITF
                                | CAMERIC_FEATURE_ID_MASK_MIPI0_ITF
                                | CAMERIC_FEATURE_ID_MASK_MIPI1_ITF);

    return ( result );
}


/******************************************************************************
 * CamerIcDriverInit()
 *****************************************************************************/
RESULT CamerIcDriverInit
(
    CamerIcDrvConfig_t  *pConfig
)
{
    RESULT result = RET_SUCCESS;

    CamerIcDrvContext_t *pDrvContext;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    pDrvContext = (CamerIcDrvContext_t *)malloc( sizeof(CamerIcDrvContext_t) );
    if ( pDrvContext == NULL )
    {
        TRACE( CAMERIC_DRV_ERROR,  "%s: Can't allocate CamerIC context\n",  __FUNCTION__ );
        return ( RET_OUTOFMEM );
    }
    MEMSET( pDrvContext, 0, sizeof(CamerIcDrvContext_t) );

    /* setup marvin software HAL */
    result = HalAddRef( pConfig->HalHandle );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: Can't reference HAL (%d)\n",  __FUNCTION__ );
        return ( result );
    }
    pDrvContext->HalHandle  = pConfig->HalHandle;
    pDrvContext->base       = pConfig->base;

    /* initialization of cameric sub modules */

    /* setup isp software */
    result = CamerIcIspInit( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup ISP driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }

#if defined(MRV_MIPI_VERSION)
    /* we've a MIPI module, so check if the upper layer wants to
     * use it and create the sw module context in this case */
    {
        uint32_t i = 0UL;

        for (i = 0UL; i<MIPI_ITF_ARR_SIZE; i++ )
        {
            if ( ( (i == 0) && (pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_MIPI)   ) ||
                 ( (i == 1) && (pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_MIPI_2) ) )
            {
                result = CamerIcMipiInit( pDrvContext, i );
                if ( result != RET_SUCCESS )
                {
                    TRACE( CAMERIC_DRV_ERROR,
                            "%s: Can't setup MIPI driver module %d (%d)\n",  __FUNCTION__, i, result);
                    return ( result );
                }
            }
            else
            {
                result = CamerIcMipiDisableClock( pDrvContext, i );
                if ( result != RET_SUCCESS )
                {
                    TRACE( CAMERIC_DRV_ERROR,
                            "%s: Failed to disable clock for MIPI module %d (%d)\n",  __FUNCTION__, i, result);
                    return ( result );
                }
            }

        }
    }
#endif /* MRV_MIPI_VERSION */

#if defined(MRV_BLACK_LEVEL_VERSION)
    /* we've a BLS module, so check if the upper layer wants to
     * use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_BLS )
    {
        result = CamerIcIspBlsInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP BLS driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_BLACK_LEVEL_VERSION */

#if defined(MRV_GAMMA_IN_VERSION)
    /* we've a Degamma module, so check if the upper layer wants to
     * use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_DEGAMMA )
    {
        result = CamerIcIspDegammaInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP DEGAMMA driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_GAMMA_IN_VERSION */    

#if defined(MRV_LSC_VERSION)
    /* we've a Lense Shade Correction module, so 
     * create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_LSC )
    {
        result = CamerIcIspLscInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP LSC driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_LSC_VERSION */

#if defined(MRV_DPCC_VERSION)
    /* we've a Defect Pixel Cluster Correction module, so 
     * create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_DPCC )
    {
        /* setup isp dpcc software */
        result = CamerIcIspDpccInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup ISP DPCC driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_DPCC_VERSION */

#if defined(MRV_DPF_VERSION)    
    /* we've a Denoising Pre-Filter module, so 
     * create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_DPF )
    {
        /* setup isp dpf software */
        result = CamerIcIspDpfInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP DPF driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_DPF_VERSION */

#if defined(MRV_FILTER_VERSION)
    /* we've the ISP Filter module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    result = CamerIcIspFltInit( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup ISP FLT driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }
#endif /* MRV_FILTER_VERSION */

#if defined(MRV_CAC_VERSION)
    /* setup isp cac software */
    result = CamerIcIspCacInit( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup ISP CAC driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }
#endif /* MRV_CAC_VERSION */

#if defined(MRV_CNR_VERSION)
    /* setup isp cnr software */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_CNR )
    {
        result = CamerIcIspCnrInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                   "%s: Can't setup ISP CNR driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_CNR_VERSION */

#if defined(MRV_AUTO_EXPOSURE_VERSION)
    /* we've an Exposure measuring module, so check if the upper layer wants
     * create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_EXPOSURE )
    {
        result = CamerIcIspExpInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP EXPOSURE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_AUTO_EXPOSURE_VERSION */    

#if defined(MRV_HISTOGRAM_VERSION)    
    /* we've a histogram measuring module, so check if the upper layer wants
     * create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_HIST )
    {
        result = CamerIcIspHistInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup ISP HIST driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_HISTOGRAM_VERSION */    

#if defined(MRV_AWB_VERSION)
    /* we've a WB measuring module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_AWB )
    {
         /* setup isp awb software */
        result = CamerIcIspAwbInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP AWB driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_AWB_VERSION */

#if defined(MRV_ELAWB_VERSION)
    /* we've an eliptic WB measuring module, so check if the upper layer
     * wants to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_ELAWB )
    {
         /* setup isp awb software */
        result = CamerIcIspElAwbInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP EL-AWB driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_ELAWB_VERSION */

#if defined(MRV_AUTOFOCUS_VERSION)
    /* we've a AF measuring module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_AFM )
    {
        result = CamerIcIspAfmInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP AF driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_AUTOFOCUS_VERSION */
    
#if defined(MRV_WDR_VERSION)
    /* we've a WDR module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    result = CamerIcIspWdrInit( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup ISP WDR driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }
#endif /* MRV_WDR_VERSION */

#if defined(MRV_COLOR_PROCESSING_VERSION)
    /* we've a Color Processing module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_CPROC )
    {
        result = CamerIcCprocInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup color processing driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
    else
    {
        result = CamerIcCprocDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for color processing module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_COLOR_PROCESSING_VERSION */

#if defined(MRV_IMAGE_EFFECTS_VERSION)
    /* we've a Image Effects module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_IE )
    {
        result = CamerIcIeInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup IE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
    else
    {
        result = CamerIcIeDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for IE module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_IMAGE_EFFECTS_VERSION */

#if defined(MRV_SUPER_IMPOSE_VERSION)
    /* we've a Super Impose module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_SIMP )
    {
        result = CamerIcSimpInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup SIMP driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
    else
    {
        result = CamerIcSimpDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for SIMP module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_SUPER_IMPOSE_VERSION */

#if defined(MRV_JPE_VERSION)
    /* we've a JPE module, so check if the upper layer wants
     * to use it and create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_JPE )
    {
        result = CamerIcJpeInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup JPE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
    else
    {
        result = CamerIcJpeDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for JPE module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_JPE_VERSION */

#if defined(MRV_VSM_VERSION)
    /* we've an video stabilization measuring module, so check if the upper
     * layer wants to create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_VSM )
    {
        result = CamerIcIspVsmInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP VSM driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_VSM_VERSION */

#if defined(MRV_IS_VERSION)
    /* we've an image stabilization module, so check if the upper
     * layer wants to create the sw module context in this case */
    if ( pConfig->ModuleMask & CAMERIC_MODULE_ID_MASK_IS )
    {
        result = CamerIcIspIsInit( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't setup ISP IS driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_IS_VERSION */

    /* setup mi software */
    result = CamerIcMiInit( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't setup MI driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }


    CamerIcIspFlashInit(pDrvContext);

    /* driver initialized */
    pDrvContext->DriverState = CAMERIC_DRIVER_STATE_INIT;

    pConfig->DrvHandle = (CamerIcDrvHandle_t)pDrvContext;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}


/******************************************************************************
 * CamerIcDriverRelease()
 *****************************************************************************/
RESULT CamerIcDriverRelease
(
    CamerIcDrvHandle_t *handle
)
{
    CamerIcDrvContext_t *pDrvContext= ( CamerIcDrvContext_t *)*handle;

    RESULT result = RET_SUCCESS;


    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( pDrvContext == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* release driver only if initialized or stopped before */
    if ( (pDrvContext->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (pDrvContext->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    result = CamerIcMiRelease( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't release MI driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }

#if defined(MRV_IS_VERSION)
    if ( pDrvContext->pIspIsContext )
    {
        result = CamerIcIspIsRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release IS driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_IS_VERSION */

#if defined(MRV_VSM_VERSION)
    if ( pDrvContext->pIspVsmContext )
    {
        result = CamerIcIspVsmRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release VSM driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_VSM_VERSION */

#if defined(MRV_JPE_VERSION)
    if ( pDrvContext->pJpeContext )
    {
        result = CamerIcJpeRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release JPE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_JPE_VERSION */

#if defined(MRV_SUPER_IMPOSE_VERSION)
    if ( pDrvContext->pSimpContext != NULL )
    {
        result = CamerIcSimpDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for SIMP module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }

        result = CamerIcSimpRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release SIMP driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_SUPER_IMPOSE_VERSION */

#if defined(MRV_IMAGE_EFFECTS_VERSION)
    if ( pDrvContext->pIeContext != NULL )
    {
        result = CamerIcIeDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for IE module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }

        result = CamerIcIeRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release IE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_IMAGE_EFFECTS_VERSION */

#if defined(MRV_COLOR_PROCESSING_VERSION)
    if ( pDrvContext->pCprocContext )
    {
        result = CamerIcCprocDisableClock( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Failed to disable clock for CPROC module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }

        result = CamerIcCprocRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release color processing driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_COLOR_PROCESSING_VERSION */

#if defined(MRV_WDR_VERSION)
    if ( pDrvContext->pIspWdrContext )
    {
        result = CamerIcIspWdrRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP WDR driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_WDR_VERSION */

#if defined(MRV_AUTOFOCUS_VERSION)
    if ( pDrvContext->pIspAfmContext )
    {
        result = CamerIcIspAfmRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP AF driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_AUTOFOCUS_VERSION */

#if defined(MRV_ELAWB_VERSION)
    /* setup awb eliptic measuring software */
    if ( pDrvContext->pIspElAwbContext )
    {
        /* setup isp awb software */
        result = CamerIcIspElAwbRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP EL-AWB driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_ELAWB_VERSION */

#if defined(MRV_AWB_VERSION)
    if ( pDrvContext->pIspAwbContext )
    {
        result = CamerIcIspAwbRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP AWB driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_AWB_VERSION */

#if defined(MRV_HISTOGRAM_VERSION)
    if ( pDrvContext->pIspHistContext )
    {
        result = CamerIcIspHistRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP HIST driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_HISTOGRAM_VERSION */

#if defined(MRV_AUTO_EXPOSURE_VERSION)
    if ( pDrvContext->pIspExpContext )
    {
        result = CamerIcIspExpRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP EXPOSURE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_AUTO_EXPOSURE_VERSION */

#ifdef MRV_CNR_VERSION
    if ( pDrvContext->pIspCnrContext )
    {
        result = CamerIcIspCnrRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP CNR driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_CNR_VERSION */

#ifdef MRV_CAC_VERSION
    if ( pDrvContext->pIspCacContext )
    {
        result = CamerIcIspCacRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP CAC driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_CAC_VERSION */

#if defined(MRV_FILTER_VERSION)
    if ( pDrvContext->pIspFltContext )
    {
        result = CamerIcIspFltRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP FLT driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_FILTER_VERSION */    

#if defined(MRV_DPF_VERSION)    
    if ( pDrvContext->pIspDpfContext != NULL )
    {
        result = CamerIcIspDpfRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't release ISP DPF driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_DPF_VERSION */    

#if defined(MRV_DPCC_VERSION)
    if ( pDrvContext->pIspDpccContext != NULL )
    {
        result = CamerIcIspDpccRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't release ISP DPCC driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_DPCC_VERSION */

#if defined(MRV_LSC_VERSION)
    if ( pDrvContext->pIspLscContext != NULL )
    {
        result = CamerIcIspLscRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP LSC driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_LSC_VERSION */

#if defined(MRV_GAMMA_IN_VERSION)
    if ( pDrvContext->pIspDegammaContext != NULL )
    {
        result = CamerIcIspDegammaRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP DEGAMMA driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_GAMMA_IN_VERSION */    

#if defined(MRV_BLACK_LEVEL_VERSION)
    if ( pDrvContext->pIspBlsContext != NULL )
    {
        result = CamerIcIspBlsRelease( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't release ISP BLS driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_BLACK_LEVEL_VERSION */

#if defined(MRV_MIPI_VERSION)
    {
        uint32_t i = 0UL;

        for (i = 0UL; i<MIPI_ITF_ARR_SIZE; i++ )
        {
            if ( pDrvContext->pMipiContext[i] )
            {
                result = CamerIcMipiDisableClock( pDrvContext, i );
                if ( result != RET_SUCCESS )
                {
                    TRACE( CAMERIC_DRV_ERROR,
                            "%s: Failed to disable clock for MIPI module (%d)\n",  __FUNCTION__, result);
                    return ( result );
                }

                result = CamerIcMipiRelease( pDrvContext, i );
                if ( result != RET_SUCCESS )
                {
                    TRACE( CAMERIC_DRV_ERROR,
                            "%s: Can't release MIPI driver module (%d)\n",  __FUNCTION__, result);
                    return ( result );
                }
            }
        }
    }
#endif /* MRV_MIPI_VERSION */

    if (pDrvContext->pFlashContext != NULL)
        CamerIcIspFlashRelease(pDrvContext);

    result = CamerIcIspRelease( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR,
                "%s: Can't release ISP driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }

    result = HalDelRef( pDrvContext->HalHandle );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: Can't dereference HAL (%d)\n",  __FUNCTION__ );
        return ( result );
    }
    pDrvContext->HalHandle = NULL;

    pDrvContext->DriverState = CAMERIC_DRIVER_STATE_INVALID;

    MEMSET( *handle, 0, sizeof( CamerIcDrvContext_t ) );
    free ( *handle );
    *handle = NULL;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcDriverStart()
 *****************************************************************************/
RESULT CamerIcDriverStart
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *pDrvContext= ( CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( handle == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* start driver only if initialized or stopped before */
    if ( (pDrvContext->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (pDrvContext->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    /* start isp software */
    result = CamerIcIspStart( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: Can't start ISP driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }

    /* start mi software */
    result = CamerIcMiStart( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        (void)CamerIcIspStop( pDrvContext );

        TRACE( CAMERIC_DRV_ERROR, "%s: Can't start MI driver module (%d)\n",  __FUNCTION__, result);

        return ( result );
    }

#if defined(MRV_MIPI_VERSION)
    {
        uint32_t i = 0UL;

        for (i = 0UL; i<MIPI_ITF_ARR_SIZE; i++ )
        {
            if ( pDrvContext->pMipiContext[i] != NULL )
            {
                result = CamerIcMipiStart( pDrvContext, i );
                if ( result != RET_SUCCESS )
                {
                    (void)CamerIcMiStop( pDrvContext );
                    (void)CamerIcIspStop( pDrvContext );
                    TRACE( CAMERIC_DRV_ERROR,
                            "%s: Can't start MIPI driver module %d (%d)\n",  __FUNCTION__, i, result);
                    return ( result );
                }
            }
        }
    }
#endif /* MRV_MIPI_VERSION */

#if defined(MRV_JPE_VERSION)
    if ( pDrvContext->pJpeContext != NULL )
    {
        result = CamerIcJpeStart( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            (void)CamerIcMipiStop( pDrvContext, 0 );
            (void)CamerIcMiStop( pDrvContext );
            (void)CamerIcIspStop( pDrvContext );
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't start JPE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_JPE_VERSION */

    pDrvContext->pDmaCompletionCb = NULL;

    pDrvContext->DriverState = CAMERIC_DRIVER_STATE_RUNNING;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcDriverStop()
 *****************************************************************************/
RESULT CamerIcDriverStop
(
    CamerIcDrvHandle_t handle
)
{
    CamerIcDrvContext_t *pDrvContext= ( CamerIcDrvContext_t *)handle;

    RESULT result = RET_SUCCESS;


    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( pDrvContext == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* start driver only if initialized or stopped before */
    if ( pDrvContext->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

#if defined(MRV_JPE_VERSION)
    if ( pDrvContext->pJpeContext != NULL )
    {
        result = CamerIcJpeStop( pDrvContext );
        if ( result != RET_SUCCESS )
        {
            TRACE( CAMERIC_DRV_ERROR,
                    "%s: Can't stop JPE driver module (%d)\n",  __FUNCTION__, result);
            return ( result );
        }
    }
#endif /* MRV_JPE_VERSION */

#if defined(MRV_MIPI_VERSION)
    {
        uint32_t i = 0UL;

        for (i = 0UL; i<MIPI_ITF_ARR_SIZE; i++ )
        {
            if ( pDrvContext->pMipiContext[i] != NULL )
            {
                result = CamerIcMipiStop( pDrvContext, i );
                if ( result != RET_SUCCESS )
                {
                    (void)CamerIcMiStop( pDrvContext );
                    (void)CamerIcIspStop( pDrvContext );
                    TRACE( CAMERIC_DRV_ERROR,
                            "%s: Can't stop MIPI driver module %d (%d)\n",  __FUNCTION__, i, result);
                    return ( result );
                }
            }
        }
    }
#endif /* MRV_MIPI_VERSION */

    /* stop CamerIc MI software driver */
    result = CamerIcMiStop( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: Can't stop MI driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }

    /* stop CamerIc ISP software driver */
    result = CamerIcIspStop( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: Can't stop ISP driver module (%d)\n",  __FUNCTION__, result);
        return ( result );
    }

    pDrvContext->DriverState = CAMERIC_DRIVER_STATE_STOPPED;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcDriverGetRevision()
 *****************************************************************************/
RESULT CamerIcDriverGetRevision
(
    CamerIcDrvHandle_t      handle,
    uint32_t                *revision
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( (ctx == NULL) || (ctx->HalHandle == NULL) )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( revision == NULL )
    {
        return ( RET_INVALID_PARM );
    }
    else
    {
        volatile MrvAllRegister_t *ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
        *revision = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_id) );
    }

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverGetIfSelectSwitch()
 *****************************************************************************/
RESULT CamerIcDriverGetIfSelect
(
    CamerIcDrvHandle_t          handle,
    CamerIcInterfaceSelect_t    *IfSelect
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( IfSelect == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    *IfSelect = ctx->IfSelect;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverSetIfSelect()
 *****************************************************************************/
RESULT CamerIcDriverSetIfSelect
(
    CamerIcDrvHandle_t              handle,
    const CamerIcInterfaceSelect_t  IfSelect
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    uint32_t vi_dpcl     = 0UL;
    uint32_t if_select   = 0UL;


    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    switch ( IfSelect )
    {
        case CAMERIC_ITF_SELECT_PARALLEL:
            {
                if_select = MRV_IF_SELECT_PAR;
                break;
            }

        case CAMERIC_ITF_SELECT_SMIA:
            {
                if_select = MRV_IF_SELECT_SMIA;
                break;
            }

        case CAMERIC_ITF_SELECT_MIPI:
            {
                if_select = MRV_IF_SELECT_MIPI;
                break;
            }

        case CAMERIC_ITF_SELECT_MIPI_2:
            {
                if_select = MRV_IF_SELECT_MIPI_2;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid Interface selected (%d)\n",  __FUNCTION__, IfSelect );
                return ( RET_NOTSUPP );
            }
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    
    vi_dpcl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl) );
    REG_SET_SLICE(vi_dpcl, MRV_IF_SELECT, if_select);
    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl), vi_dpcl );
    
    TRACE( CAMERIC_DRV_INFO, "%s: set path: 0x%08x\n", __FUNCTION__, vi_dpcl);

    ctx->IfSelect = IfSelect;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}




/******************************************************************************
 * CamerIcDriverGetViDmaSwitch()
 *****************************************************************************/
RESULT CamerIcDriverGetViDmaSwitch
(
    CamerIcDrvHandle_t      handle,
    CamerIcDmaReadPath_t    *DmaRead
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( DmaRead == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    *DmaRead = ctx->DmaRead;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverSetViDmaSwitch()
 *****************************************************************************/
RESULT CamerIcDriverSetViDmaSwitch
(
    CamerIcDrvHandle_t          handle,
    const CamerIcDmaReadPath_t  DmaRead
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    uint32_t vi_dpcl        = 0UL;
    uint32_t vi_dma_switch  = 0UL;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    vi_dpcl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl) );

    switch ( DmaRead )
    {
        case CAMERIC_DMA_READ_SPMUX:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_SELF;
                break;
            }

        case CAMERIC_DMA_READ_SUPERIMPOSE:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_SIMP;
                break;
            }

        case CAMERIC_DMA_READ_IMAGE_EFFECTS:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_IE;
                break;
            }

        case CAMERIC_DMA_READ_JPEG:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_JPG;
                break;
            }

        case CAMERIC_DMA_READ_ISP:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_ISP;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid DMA-Read datapath (%d)\n",  __FUNCTION__, DmaRead );
                return ( RET_NOTSUPP );
            }
    }

    REG_SET_SLICE( vi_dpcl, MRV_VI_DMA_SWITCH, vi_dma_switch );

    TRACE( CAMERIC_DRV_INFO, "%s: set path: 0x%08x\n", __FUNCTION__, vi_dpcl);

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl), vi_dpcl );

    ctx->DmaRead = DmaRead;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverGetViMpMux()
 *****************************************************************************/
RESULT CamerIcDriverGetViMpMux
(
    CamerIcDrvHandle_t      handle,
    CamerIcMainPathMux_t    *MpMux
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( MpMux == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    *MpMux = ctx->MpMux;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverSetViMpMux()
 *****************************************************************************/
RESULT CamerIcDriverSetViMpMux
(
    CamerIcDrvHandle_t          handle,
    const CamerIcMainPathMux_t  MpMux
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;

    uint32_t vi_dpcl    = 0UL;
    uint32_t vi_mp_mux  = 0UL;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_RUNNING) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    vi_dpcl = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl) );

    switch ( MpMux )
    {
        case CAMERIC_MP_MUX_JPEG_DIRECT:
            {
                vi_mp_mux = MRV_VI_MP_MUX_JPGDIRECT;
                break;
            }

        case CAMERIC_MP_MUX_MI:
            {
                vi_mp_mux = MRV_VI_MP_MUX_MP;
                break;
            }

        case CAMERIC_MP_MUX_JPEG:
            {
                vi_mp_mux = MRV_VI_MP_MUX_JPEG;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid main-path (%d)\n",  __FUNCTION__, MpMux);
                return ( RET_NOTSUPP );
            }
    }

    REG_SET_SLICE( vi_dpcl, MRV_VI_MP_MUX, vi_mp_mux );

    TRACE( CAMERIC_DRV_INFO, "%s: set path: 0x%08x\n", __FUNCTION__, vi_dpcl);

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl), vi_dpcl );

    ctx->MpMux = MpMux;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}




/******************************************************************************
 * CamerIcDriverSetDataPath()
 *****************************************************************************/
RESULT CamerIcDriverSetDataPath
(
    CamerIcDrvHandle_t                      handle,

    const CamerIcMainPathMux_t              MpMux,
    const CamerIcSelfPathMux_t              SpMux,
    const CamerIcYcSplitterChannelMode_t    YcSplitter,
    const CamerIcImgEffectsMux_t            IeMux,
    const CamerIcDmaReadPath_t              DmaRead,
    const CamerIcInterfaceSelect_t          IfSelect
)
{
    CamerIcDrvContext_t *ctx = (CamerIcDrvContext_t *)handle;

    volatile MrvAllRegister_t *ptCamerIcRegMap;
    uint32_t vi_dpcl;

    uint32_t vi_mp_mux;
    uint32_t vi_dma_spmux;
    uint32_t vi_dma_switch;
    uint32_t vi_chan_mode;
    uint32_t vi_dma_iemux;
    uint32_t if_select;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check precondition */
    if ( ctx == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* don't allow mode changing if running */
    if ( (ctx->DriverState != CAMERIC_DRIVER_STATE_INIT)
            && (ctx->DriverState != CAMERIC_DRIVER_STATE_STOPPED) )
    {
        return ( RET_WRONG_STATE );
    }

    ptCamerIcRegMap = (MrvAllRegister_t *)ctx->base;
    vi_dpcl         = HalReadReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl) );

    /* get desired setting for vi_dp_switch (or vi_dp_mux) bits */
    switch ( MpMux )
    {
        case CAMERIC_MP_MUX_JPEG_DIRECT:
            {
                vi_mp_mux = MRV_VI_MP_MUX_JPGDIRECT;
                break;
            }

        case CAMERIC_MP_MUX_MI:
            {
                vi_mp_mux = MRV_VI_MP_MUX_MP;
                break;
            }

        case CAMERIC_MP_MUX_JPEG:
            {
                vi_mp_mux = MRV_VI_MP_MUX_JPEG;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid main-path (%d)\n",  __FUNCTION__, MpMux);
                return ( RET_NOTSUPP );
            }
    }

    switch ( SpMux )
    {
        case CAMERIC_SP_MUX_CAMERA:
            {
                vi_dma_spmux = MRV_VI_DMA_SPMUX_CAM;
                break;
            }

        case CAMERIC_SP_MUX_DMA_READ:
            {
                vi_dma_spmux = MRV_VI_DMA_SPMUX_DMA;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid self-path (%d)\n",  __FUNCTION__, SpMux);
                return ( RET_NOTSUPP );
            }
    }

    /* get desired setting for ycs_chan_mode (or vi_chan_mode) bits */
    switch ( YcSplitter )
    {
        case CAMERIC_YCSPLIT_CHMODE_OFF:
            {
                vi_chan_mode = MRV_VI_CHAN_MODE_OFF;
                break;
            }

        case CAMERIC_YCSPLIT_CHMODE_MP:
            {
                vi_chan_mode = MRV_VI_CHAN_MODE_MP;
                break;
            }

        case CAMERIC_YCSPLIT_CHMODE_SP:
            {
                vi_chan_mode = MRV_VI_CHAN_MODE_SP;
                break;
            }

        case CAMERIC_YCSPLIT_CHMODE_MP_SP:
            {
                vi_chan_mode = MRV_VI_CHAN_MODE_MP_SP;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid YC-Splitter setting (%d)\n",  __FUNCTION__, YcSplitter );
                return ( RET_NOTSUPP );
            }
    }

    switch ( IeMux )
    {
        case CAMERIC_IE_MUX_CAMERA:
            {
                vi_dma_iemux = MRV_VI_DMA_IEMUX_CAM;
                break;
            }

        case CAMERIC_IE_MUX_DMA_READ:
            {
                vi_dma_iemux = MRV_VI_DMA_IEMUX_DMA;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid image effects muxer setting (%d)\n",  __FUNCTION__, IeMux );
                return ( RET_NOTSUPP );
            }

    }

    switch ( DmaRead )
    {
        case CAMERIC_DMA_READ_SPMUX:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_SELF;
                break;
            }

        case CAMERIC_DMA_READ_SUPERIMPOSE:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_SIMP;
                break;
            }

        case CAMERIC_DMA_READ_IMAGE_EFFECTS:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_IE;
                break;
            }

        case CAMERIC_DMA_READ_JPEG:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_JPG;
                break;
            }

        case CAMERIC_DMA_READ_ISP:
            {
                vi_dma_switch = MRV_VI_DMA_SWITCH_ISP;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid DMA-Read datapath (%d)\n",  __FUNCTION__, DmaRead );
                return ( RET_NOTSUPP );
            }
    }

    switch ( IfSelect )
    {
        case CAMERIC_ITF_SELECT_PARALLEL:
            {
                if_select = MRV_IF_SELECT_PAR;
                break;
            }

        case CAMERIC_ITF_SELECT_SMIA:
            {
                if_select = MRV_IF_SELECT_SMIA;
                break;
            }

        case CAMERIC_ITF_SELECT_MIPI:
            {
                if_select = MRV_IF_SELECT_MIPI;
                break;
            }

        case CAMERIC_ITF_SELECT_MIPI_2:
            {
                if_select = MRV_IF_SELECT_MIPI_2;
                break;
            }

        default:
            {
                TRACE( CAMERIC_DRV_WARN,
                        "%s: Invalid Interface selected (%d)\n",  __FUNCTION__, IfSelect );
                return ( RET_NOTSUPP );
            }
    }

    /*  program settings into MARVIN vi_dpcl register */
    REG_SET_SLICE(vi_dpcl, MRV_VI_MP_MUX, vi_mp_mux);
    REG_SET_SLICE(vi_dpcl, MRV_VI_DMA_SPMUX, vi_dma_spmux);
    REG_SET_SLICE(vi_dpcl, MRV_VI_CHAN_MODE, vi_chan_mode);
    REG_SET_SLICE(vi_dpcl, MRV_VI_DMA_IEMUX, vi_dma_iemux);
    REG_SET_SLICE(vi_dpcl, MRV_VI_DMA_SWITCH, vi_dma_switch);
    REG_SET_SLICE(vi_dpcl, MRV_IF_SELECT, if_select);

    TRACE( CAMERIC_DRV_INFO, "%s: set path: 0x%08x\n", __FUNCTION__, vi_dpcl);

    HalWriteReg( ctx->HalHandle, ((ulong_t)&ptCamerIcRegMap->vi_dpcl), vi_dpcl );

    ctx->MpMux          = MpMux;
    ctx->SpMux          = SpMux;
    ctx->YcSplitter     = YcSplitter;
    ctx->IeMux          = IeMux;
    ctx->DmaRead        = DmaRead;
    ctx->IfSelect       = IfSelect;

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverLoadPicture()
 *****************************************************************************/
RESULT CamerIcDriverLoadPicture
(
    CamerIcDrvHandle_t      handle,
    PicBufMetaData_t        *pPicBuffer,
    CamerIcCompletionCb_t   *pCompletionCb,
    bool_t                  cont
)
{
    CamerIcDrvContext_t *pDrvContext = ( CamerIcDrvContext_t *)handle;

    RESULT result;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( pDrvContext == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( (pCompletionCb == NULL) || (pPicBuffer == NULL) )
    {
        return ( RET_INVALID_PARM );
    }

    if ( pDrvContext->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    if ( pDrvContext->pDmaCompletionCb != NULL )
    {
        return ( RET_BUSY );
    }

    pDrvContext->pDmaCompletionCb = pCompletionCb;

    result = CamerIcMiStartLoadPicture( pDrvContext, pPicBuffer, cont );
    if ( result != RET_PENDING )
    {
        return ( result );
    }

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( result );
}



/******************************************************************************
 * CamerIcDriverCaptureFrames()
 *****************************************************************************/
RESULT CamerIcDriverCaptureFrames
(
    CamerIcDrvHandle_t      handle,
    const uint32_t          numFrames,
    CamerIcCompletionCb_t   *pCompletionCb
)
{
    CamerIcDrvContext_t *pDrvContext = ( CamerIcDrvContext_t *)handle;

    RESULT result;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( NULL == pDrvContext )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: wrong handle\n", __FUNCTION__);
        return ( RET_WRONG_HANDLE );
    }

    if ( NULL == pCompletionCb )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: invalid parameter\n", __FUNCTION__);
        return ( RET_INVALID_PARM );
    }

    /* release driver only if initialized or stopped before */
    if ( pDrvContext->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        TRACE( CAMERIC_DRV_ERROR, "%s: wrong state\n", __FUNCTION__);
        return ( RET_WRONG_STATE );
    }

    pDrvContext->pCapturingCompletionCb = pCompletionCb;

    result = CamerIcIspStartCapturing ( pDrvContext, numFrames );

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( (result == RET_SUCCESS) ? RET_PENDING : result );
}



/******************************************************************************
 * CamerIcDriverStartInput()
 *****************************************************************************/
RESULT CamerIcDriverStartInput
(
    CamerIcDrvHandle_t      handle
)
{
    CamerIcDrvContext_t *pDrvContext = ( CamerIcDrvContext_t *)handle;

    RESULT result;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( pDrvContext == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    /* release driver only if initialized or stopped before */
    if ( pDrvContext->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    pDrvContext->pStopInputCompletionCb = NULL;

    result = CamerIcIspEnable( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_SUCCESS );
}



/******************************************************************************
 * CamerIcDriverStopInput()
 *****************************************************************************/
RESULT CamerIcDriverStopInput
(
    CamerIcDrvHandle_t      handle,
    CamerIcCompletionCb_t   *pCompletionCb
)
{
    CamerIcDrvContext_t *pDrvContext = ( CamerIcDrvContext_t *)handle;

    RESULT result;

    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( pDrvContext == NULL )
    {
        return ( RET_WRONG_HANDLE );
    }

    if ( pCompletionCb == NULL )
    {
        return ( RET_INVALID_PARM );
    }

    /* release driver only if initialized or stopped before */
    if ( pDrvContext->DriverState != CAMERIC_DRIVER_STATE_RUNNING )
    {
        return ( RET_WRONG_STATE );
    }

    pDrvContext->pStopInputCompletionCb = pCompletionCb;

    result = CamerIcIspDisable( pDrvContext );
    if ( result != RET_SUCCESS )
    {
        return ( result );
    }

    TRACE( CAMERIC_DRV_INFO, "%s: (exit)\n", __FUNCTION__);

    return ( RET_PENDING );
}

void CamerIcDriverSetIsSwapByte
(
    CamerIcDrvHandle_t      handle,
    bool_t  swapbyte
)
{
    CamerIcDrvContext_t *pDrvContext = ( CamerIcDrvContext_t *)handle;


    TRACE( CAMERIC_DRV_INFO, "%s: (enter)\n", __FUNCTION__);

    /* check pre-condition */
    if ( pDrvContext == NULL )
    {
        return ;
    }

    pDrvContext->isSwapByte = swapbyte;
}

