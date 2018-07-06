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
#ifndef __ADPF_CTRL_H__
#define __ADPF_CTRL_H__

/**
 * @file adpf_ctrl.h
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
 * @defgroup ADPF Auto Denoising Pre-Filter
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include "adpf.h"

#ifdef __cplusplus
extern "C"
{
#endif



/*****************************************************************************/
/**
 * @brief   This enum type specifies the different possible states of
 *          the ADPF module.
 */
/*****************************************************************************/
typedef enum AdpfState_e
{
    ADPF_STATE_INVALID           = 0,                   /**< initialization value */
    ADPF_STATE_INITIALIZED       = 1,                   /**< instance is created, but not initialized */
    ADPF_STATE_STOPPED           = 2,                   /**< instance is confiured (ready to start) or stopped */
    ADPF_STATE_RUNNING           = 3,                   /**< instance is running (processes frames) */
    ADPF_STATE_LOCKED            = 4,                   /**< instance is locked (for taking snapshots) */
    ADPF_STATE_MAX                                      /**< max */
} AdpfState_t;



/*****************************************************************************/
/**
 * @brief   Context of the ADPF module.
 */
/*****************************************************************************/
typedef struct AdpfContext_s
{
    AdpfState_t                     state;

    CamerIcDrvHandle_t              hCamerIc;           /**< cameric driver handle */
    CamerIcDrvHandle_t              hSubCamerIc;        /**< cameric driver handle */

    CamResolutionName_t             ResName;            /**< identifier for accessing resolution depended calibration data */
    CamCalibDbHandle_t              hCamCalibDb;        /**< calibration database handle */

    uint16_t                        SigmaGreen;         /**< sigma value for green pixel */
    uint16_t                        SigmaRedBlue;       /**< sigma value for red/blue pixel */

    float                           fGradient;          /**< */
    float                           fOffset;            /**< */
    float                           fMin;          /**< */
    float                           fDiv;            /**< */
    AdpfGains_t                     NfGains;            /**< */

    AdpfConfig_t                    Config;

    CamerIcDpfInvStrength_t         DynInvStrength;     /**< */
    CamerIcDpfNoiseLevelLookUp_t    Nll;                /**< noise level lookup */

    float                           gain;               /**< current sensor gain */

	CamMfdProfile_t					Mfd;			    /**< mfd struct*/
	CamUvnrProfile_t				Uvnr;			    /**< uvnr struct*/

	//oyyf add
    CamDenoiseLevelCurve_t        DenoiseLevelCurve;   
    CamSharpeningLevelCurve_t     SharpeningLevelCurve;  	
	CamerIcIspFltDeNoiseLevel_t denoise_level;
	CamerIcIspFltSharpeningLevel_t sharp_level;
	float FlterEnable;
} AdpfContext_t;



#ifdef __cplusplus
}
#endif

/* @} ADPF */


#endif /* __ADPF_CTRL_H__*/
