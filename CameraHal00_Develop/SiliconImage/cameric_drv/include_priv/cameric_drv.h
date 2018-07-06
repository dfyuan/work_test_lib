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
#ifndef __CAMERIC_DRV_H__
#define __CAMERIC_DRV_H__

/**
 * @file cameric_drv.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup cameric_drv CamerIc Driver Internal
 * @{
 *
 */

#include <ebase/types.h>

#include <hal/hal_api.h>

#include <common/return_codes.h>

#include "cameric_isp_drv.h"                // isp driver module
#include "cameric_isp_bls_drv.h"            // isp bls driver module
#include "cameric_isp_degamma_drv.h"        // isp sensor degamma (gamma in) driver module
#include "cameric_isp_exp_drv.h"            // isp exposure driver module
#include "cameric_isp_awb_drv.h"            // isp white-balance driver module
#include "cameric_isp_elawb_drv.h"          // isp eliptic white-balance driver module
#include "cameric_isp_afm_drv.h"            // isp auto focus driver module
#include "cameric_isp_hist_drv.h"           // isp histogram measurement driver module
#include "cameric_isp_wdr_drv.h"            // isp wide dynamic range driver module
#include "cameric_isp_cac_drv.h"            // isp chromatic abberation driver module
#include "cameric_isp_lsc_drv.h"            // isp lens shade correction driver module
#include "cameric_isp_flt_drv.h"            // isp filter driver module
#include "cameric_isp_dpf_drv.h"            // isp denoising filter driver module
#include "cameric_isp_dpcc_drv.h"           // isp defect pixel cluster correction driver module
#include "cameric_isp_vsm_drv.h"            // isp video stabilization driver module
#include "cameric_isp_is_drv.h"             // isp iamge stabiliation driver module
#include "cameric_isp_cnr_drv.h"            // isp color noise reduction driver module
#include "cameric_cproc_drv.h"              // color processing driver module
#include "cameric_ie_drv.h"                 // image effects driver module
#include "cameric_simp_drv.h"               // super-impose driver module
#include "cameric_jpe_drv.h"                // jpeg encoder driver module
#include "cameric_mi_drv.h"                 // memory interface driver module
#include "cameric_mipi_drv.h"               // mipi driver module
#include "cameric_isp_flash_drv.h"              // flash driver module


/******************************************************************************/
/**
 * @brief   Enumeration type which represents the internal state of the CamerIc
 *          software driver.
 *
 *****************************************************************************/
typedef enum CamerIcDriverState_e
{
    CAMERIC_DRIVER_STATE_INVALID    = 0,    /**< invalid state */
    CAMERIC_DRIVER_STATE_INIT       = 1,    /**< driver is initialized */
    CAMERIC_DRIVER_STATE_RUNNING    = 2,    /**< driver is started and running */
    CAMERIC_DRIVER_STATE_STOPPED    = 3,    /**< driver is stopped */
    CAMERIC_DRIVER_STATE_MAX
} CamerIcDriverState_t;



/******************************************************************************/
/**
 * @brief   Internal context of the CamerIc software driver.
 *
 *****************************************************************************/
typedef struct CamerIcDrvContext_s
{
    ulong_t                         base;                       /**< base address of CamerIc */

    CamerIcDriverState_t            DriverState;                /**< internal driver state */

    CamerIcMainPathMux_t            MpMux;                      /**< main path muxer (vi_mp_mux) */
    CamerIcSelfPathMux_t            SpMux;                      /**< self path muxer (vi_dma_spmux) */
    CamerIcYcSplitterChannelMode_t  YcSplitter;                 /**< y/c-spliiter (vi_chan_mode) */
    CamerIcImgEffectsMux_t          IeMux;                      /**< image effects muxer (vi_dma_iemux) */
    CamerIcDmaReadPath_t            DmaRead;                    /**< dma read switch (vi_dma_switch) */
    CamerIcInterfaceSelect_t        IfSelect;                   /**< interface selector (if_select) */

    HalHandle_t                     HalHandle;                  /**< HAL handle given by CamerIcDriverInit */

    CamerIcCompletionCb_t           *pCapturingCompletionCb;    /**< capturing completion calback (called when no of frames reached) */
    CamerIcCompletionCb_t           *pStopInputCompletionCb;    /**< stop completion callback (called when stop completed) */
    CamerIcCompletionCb_t           *pDmaCompletionCb;          /**< dma completion callback (called when a dma read completed) */

    CamerIcIspContext_t             *pIspContext;               /**< reference to CamerIc ISP driver context */
    CamerIcMiContext_t              *pMiContext;                /**< reference to CamerIc MI driver context */

    bool_t                          isSwapByte;
#if defined(MRV_MIPI_VERSION)
    CamerIcMipiContext_t            *pMipiContext[MIPI_ITF_ARR_SIZE];
                                                                /**< references to CamerIc MIPI driver context */
#endif /* MRV_MIPI_VERSION */

#if defined(MRV_BLACK_LEVEL_VERSION)
    CamerIcIspBlsContext_t          *pIspBlsContext;            /**< reference to CamerIc ISP BLS driver context */
#endif /* MRV_BLACK_LEVEL_VERSION */

#if defined(MRV_GAMMA_IN_VERSION)
    CamerIcIspDegammaContext_t      *pIspDegammaContext;        /**< reference to CamerIc ISP DEGAMMA driver context */
#endif /* MRV_GAMMA_IN_VERSION */

#if defined(MRV_LSC_VERSION)
    CamerIcIspLscContext_t          *pIspLscContext;            /**< reference to CamerIc ISP LSC driver context */
#endif /* MRV_LSC_VERSION */    

#if defined(MRV_DPCC_VERSION)
    CamerIcIspDpccContext_t         *pIspDpccContext;           /**< reference to CamerIc ISP DPCC driver context */
#endif /* MRV_DPCC_VERSION */    

#if defined(MRV_DPF_VERSION)
    CamerIcIspDpfContext_t          *pIspDpfContext;            /**< reference to CamerIc ISP DPF driver context */
#endif /* MRV_DPF_VERSION */     
    
#if defined(MRV_FILTER_VERSION)
    CamerIcIspFltContext_t          *pIspFltContext;            /**< reference to CamerIc ISP FLT driver context */
#endif /* MRV_FILTER_VERSION */     

#if defined(MRV_CAC_VERSION)
    CamerIcIspCacContext_t          *pIspCacContext;            /**< reference to CamerIc ISP CAC driver context */
#endif /* MRV_CAC_VERSION */    

#if defined(MRV_AUTO_EXPOSURE_VERSION)
    CamerIcIspExpContext_t          *pIspExpContext;            /**< reference to CamerIc ISP EXPOSURE driver context */
#endif /* MRV_AUTO_EXPOSURE_VERSION */
    
#if defined(MRV_HISTOGRAM_VERSION)
    CamerIcIspHistContext_t         *pIspHistContext;           /**< reference to CamerIc ISP HIST driver context */
#endif /* MRV_HISTOGRAM_VERSION */

#if defined(MRV_AWB_VERSION)
    CamerIcIspAwbContext_t          *pIspAwbContext;            /**< reference to CamerIc ISP Auto white balance driver context */
#endif /* MRV_AWB_VERSION */    

#if defined(MRV_ELAWB_VERSION)
    CamerIcIspElAwbContext_t        *pIspElAwbContext;          /**< reference to CamerIc ISP elliptic auto white balance driver context */
#endif /* MRV_AWB_VERSION */    

#if defined(MRV_AUTOFOCUS_VERSION)
    CamerIcIspAfmContext_t          *pIspAfmContext;            /**< reference to CamerIc ISP Auto white balance driver context */
#endif /* MRV_AUTOFOCUS_VERSION */
    
#if defined(MRV_WDR_VERSION)
    CamerIcIspWdrContext_t          *pIspWdrContext;            /**< reference to CamerIc ISP WDR driver context */
#endif /* MRV_WDR_VERSION */

#if defined(MRV_CNR_VERSION)
    CamerIcIspCnrContext_t          *pIspCnrContext;            /**< reference to CamerIc ISP Cnr driver context */
#endif /* MRV_CNR_VERSION */

#ifdef MRV_COLOR_PROCESSING_VERSION
    CamerIcCprocContext_t           *pCprocContext;             /**< reference to CamerIc CPROC driver context */
#endif /* MRV_COLOR_PROCESSING_VERSION */

#ifdef MRV_IMAGE_EFFECTS_VERSION
    CamerIcIeContext_t              *pIeContext;                /**< reference to CamerIc IE driver context */
#endif /* MRV_IMAGE_EFFECTS_VERSION */    

#if defined(MRV_SUPER_IMPOSE_VERSION)
    CamerIcSimpContext_t            *pSimpContext;              /**< reference to CamerIc Super-Impose driver context */
#endif /* MRV_SUPER_IMPOSE_VERSION */    

#if defined(MRV_JPE_VERSION)
    CamerIcJpeContext_t             *pJpeContext;               /**< reference to CamerIc JPE driver context */
#endif /* MRV_JPE_VERSION */

#if defined(MRV_VSM_VERSION)
    CamerIcIspVsmContext_t          *pIspVsmContext;            /**< reference to CamerIc ISP VSM driver context */
#endif

#if defined(MRV_IS_VERSION)
    CamerIcIspIsContext_t          *pIspIsContext;              /**< reference to CamerIc ISP IS driver context */
#endif

    CamerIcIspFlashContext_t       *pFlashContext;

	uint32_t                        invalFrame;

} CamerIcDrvContext_t;



/* @} cameric_drv */

#endif /* __CAMERIC_DRV_H__ */
