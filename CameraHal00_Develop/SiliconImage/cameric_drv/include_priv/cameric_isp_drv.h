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
#ifndef __CAMERIC_ISP_DRV_H__
#define __CAMERIC_ISP_DRV_H__

/**
 * @file cameric_isp_drv.h
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
 * @defgroup cameric_isp_drv CamerIc ISP Driver Internal API
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <hal/hal_api.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>



/*******************************************************************************
 *
 *          CamerIcIspContext_t
 *  
 * @brief   Internal ISP driver context structure.
 *
 *****************************************************************************/
typedef struct CamerIcIspContext_s
{
    CamerIcIspMode_t                IspMode;                /**< current isp working mode */

    bool_t                          FltEnabled;
    bool_t                          LscEnabled;

    CamerIcIspSampleEdge_t          sampleEdge;
    CamerIcIspPolarity_t            hSyncPol;
    CamerIcIspPolarity_t            vSyncPol;
    CamerIcIspBayerPattern_t        bayerPattern;
    CamerIcIspColorSubsampling_t    subSampling;
    CamerIcIspCCIRSequence_t        seqCCIR;
    CamerIcIspFieldSelection_t      fieldSelection;
    CamerIcIspInputSelection_t      inputSelection;
    CamerIcIspLatencyFifo_t         latencyFifo;

    CamerIcColorConversionRange_t   YConvRange;             /**< Color conversion luminance clipping range */
    CamerIcColorConversionRange_t   CrConvRange;            /**< Color conversion chrominace clippping range */ 

    CamerIcWindow_t                 acqWindow;              /**< acquisition window */
    CamerIcWindow_t                 ofWindow;               /**< output formatter window */
    CamerIcWindow_t                 isWindow;               /**< image stabilization output window */

    CamerIcIspDemosaicBypass_t      bypassMode;
    uint8_t                         demosaicThreshold;

	CamerIcRequestCb_t              RequestCb;              /**< request callback */
	CamerIcEventCb_t        	    EventCb;				/**< event callback */

    HalIrqCtx_t                     HalIrqCtx;

	CamerIcCompletionCb_t   	    *pCapturingCompletionCb;
} CamerIcIspContext_t;



/*****************************************************************************/
/**
 *          CamerIcIspInit()
 *
 * @brief   Initialize CamerIc ISP driver context 
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspInit
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 *          CamerIcIspRelease()
 *
 * @brief   Release/Free CamerIc ISP driver context 
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspRelease
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 *          CamerIcIspStart()
 *
 * @brief   Initialize and start ISP interrupt handling  
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspStart
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 *          CamerIcIspStop()
 *
 * @brief  	Stop and release ISP interrupt handling
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspStop
(
    CamerIcDrvHandle_t handle
);


extern RESULT CamerIcIspEnable
(
    CamerIcDrvHandle_t handle
);



extern RESULT CamerIcIspDisable
(
    CamerIcDrvHandle_t handle
);



/*****************************************************************************/
/**
 *          CamerIcIspStartCapturing()
 *
 * @brief  	Start Frame Capturing 
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_FAILURE
 *
 *****************************************************************************/
RESULT CamerIcIspStartCapturing
(
    CamerIcDrvHandle_t	handle,
    const uint32_t      numFrames
);



/* @} cameric_isp_drv */

#endif /* __CAMERIC_ISP_DRV_H__ */
