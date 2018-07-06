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
 * @file cam_engine_drv.h
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup module_name Module Name
 * @{
 *
 */

#ifndef __CAM_ENGINE_DRV_H__
#define __CAM_ENGINE_DRV_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>

#include <common/return_codes.h>

#include <cameric_drv/cameric_drv_api.h>
#include <cameric_drv/cameric_isp_drv_api.h>
#include <cameric_drv/cameric_isp_bls_drv_api.h>
#include <cameric_drv/cameric_isp_degamma_drv_api.h>
#include <cameric_drv/cameric_isp_dpf_drv_api.h>
#include <cameric_drv/cameric_isp_wdr_drv_api.h>
#include <cameric_drv/cameric_isp_dpcc_drv_api.h>
#include <cameric_drv/cameric_isp_is_drv_api.h>
#include <cameric_drv/cameric_isp_hist_drv_api.h>
#include <cameric_drv/cameric_isp_exp_drv_api.h>
#include <cameric_drv/cameric_isp_awb_drv_api.h>
#include <cameric_drv/cameric_isp_afm_drv_api.h>
#include <cameric_drv/cameric_isp_vsm_drv_api.h>
#include <cameric_drv/cameric_isp_cac_drv_api.h>
#include <cameric_drv/cameric_isp_cnr_drv_api.h>
#include <cameric_drv/cameric_isp_flt_drv_api.h>
#include <cameric_drv/cameric_isp_lsc_drv_api.h>
#include <cameric_drv/cameric_jpe_drv_api.h>
#include <cameric_drv/cameric_mi_drv_api.h>
#include <cameric_drv/cameric_dual_cropping_drv_api.h>
#include <cameric_drv/cameric_mipi_drv_api.h>

#include "cam_engine.h"


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineInitCamerIcDrv
(
    CamEngineContext_t              *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseCamerIcDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineStartCamerIcDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineStopCamerIcDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupCamerIcDrv
(
    CamEngineContext_t                  *pCamEngineCtx,
    const CamerIcIspDemosaicBypass_t    demosaicMode,
    const uint8_t                       demosaicThreshold,
    const bool_t                        activateGammaIn,
    const bool_t                        activateGammaOut,
    const bool_t                        activateWB
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReStartEventCbCamerIcDrv
(
    CamEngineContext_t              *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupJpeDrv
(
    CamEngineContext_t *pCamEngineCtx
);



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseJpeDrv
(
    CamEngineContext_t  *pCamEngineCtx
);



/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupHistogramDrv
(
    CamEngineContext_t          *pCamEngineCtx,
    const bool_t                enable,
    const CamerIcIspHistMode_t  mode
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseHistogramDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupExpDrv
(
    CamEngineContext_t                  *pCamEngineCtx,
    const bool_t                        enable,
    const CamerIcIspExpMeasuringMode_t  mode
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseExpDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupAwbDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseAwbDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupAfmDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseAfmDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupVsmDrv
(
    CamEngineContext_t                  *pCamEngineCtx,
    const bool_t                        enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseVsmDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupLscDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupFltDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupBlsDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    const bool_t        enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupCacDrv
(
    CamEngineContext_t      *pCamEngineCtx,
    const bool_t            enable
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineStartMipiDrv
(
    CamEngineContext_t *pCamEngineCtx,
    MipiDataType_t      mipiMode,
    int mipiLaneNum,
    uint32_t mipiFreq ,
    uint32_t attachedDevId
);


/*****************************************************************************/
/**
 * @brief   Short description.
 *
 * Some detailed description goes here ...
 *
 * @param   param1      Describe the parameter 1.
 * @param   param2      Describe the parameter 2
 *
 * @return              Return the result of the function call.
 * @retval              RET_VAL1
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineStopMipiDrv
(
    CamEngineContext_t *pCamEngineCtx
);


/* @} module_name*/

#endif /*__CAM_ENGINE_DRV_H__*/

