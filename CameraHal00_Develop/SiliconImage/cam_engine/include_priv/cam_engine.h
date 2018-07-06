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
 * @file cam_engine.h
 *
 * @brief
 *   Internal interface of the CamEngine.
 *
 *****************************************************************************/
/**
 *
 * @defgroup cam_engine CamEngine internal interface
 * @{
 *
 */
#ifndef __CAM_ENGINE_H__
#define __CAM_ENGINE_H__

#include <ebase/types.h>
#include <oslayer/oslayer.h>
#include <common/return_codes.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include <cameric_drv/cameric_drv_api.h>
#include <mipi_drv/mipi_drv_api.h>

#include <aec/aec.h>
#include <awb/awb.h>
#include <af/af.h>
#include <adpf/adpf.h>
#include <adpcc/adpcc.h>
#include <avs/avs.h>

#include <mom_ctrl/mom_ctrl_api.h>
#include <mim_ctrl/mim_ctrl_api.h>
#include <bufsync_ctrl/bufsync_ctrl_api.h>

#include "cam_engine_api.h"
#include "cam_engine_mi_api.h"


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @brief Generic command type.
 *
 */
typedef struct CamEngineCmd_s
{
    CamEngineCmdId_t    cmdId;
    void                *pCmdCtx;
} CamEngineCmd_t;


/**
 * @brief   Internal context of the chain
 *
 */
typedef struct ChainCtx_s
{
    IsiSensorHandle_t       hSensor;                        /**< sensor handle for the chain */
    CamerIcDrvHandle_t      hCamerIc;                       /**< caemric driver handle for the chain */
    MipiDrvHandle_t         hMipi;                          /**< mipi input interface handle for the chain */

    CamerIcIspMode_t        inMode;                         /**< image input mode in front of ISP */

    // buffer managment
    MediaBufPoolMemory_t    BufPoolMemMain;                 /**< buffer pool (frame pool) memory of main-path */
    MediaBufPool_t          BufPoolMain;                    /**< buffer pool context of main-path */
    MediaBufPoolConfig_t    BufPoolConfigMain;              /**< buffer pool configuration of main-path */
    MediaBufPoolMemory_t    BufPoolMemSelf;
    MediaBufPool_t          BufPoolSelf;
    MediaBufPoolConfig_t    BufPoolConfigSelf;

    osQueue                 MainPathQueue;                  /**< MainPathQueue */

    osEvent                 MomEventCmdStart;
    osEvent                 MomEventCmdStop;
    MomCtrlContextHandle_t  hMomCtrl;

    CamEngineWindow_t       isWindow;
    CamerIcCompletionCb_t   CamerIcCompletionCb;
} ChainCtx_t;


/**
 * @brief Internal context of the cam-engine
 *
 */
typedef struct CamEngineContext_s
{
    CamEngineState_t            state;                      /**< State of the CamEngine */
    CamEngineModeType_t         mode;                       /**< configured scenario mode of the CamEngine */

    CamEngineCompletionCb_t     cbCompletion;               /**< Completion callback. */
    CamEngineAfpsResChangeCb_t  cbAfpsResChange;            /**< Afps resolution chnage request callback */
    void                        *pUserCbCtx;                /**< User context for completion & Afps callbacks. */

    uint32_t                    maxCommands;                /**< Max pending commands in command queue. */
    osQueue                     commandQueue;               /**< Command queue. */
    osThread                    thread;                     /**< CamEngine thread. */

    CamEngineMiOrientation_t    orient;                     /**< currently configured picture orientation */

    CamerIcCompletionCb_t       camCaptureCompletionCb;     /**< capturing completion callback */
    CamerIcCompletionCb_t       camStopInputCompletionCb;   /**< stop completion callback */ 

    bool_t                      isSystem3D;                 /**< is 3D system */

    //bool_t                      enable3D;
    bool_t                      startDMA;
    ChainCtx_t                  chain[CHAIN_MAX];
    uint32_t                    chainIdx0;
    uint32_t                    chainIdx1;

    HalHandle_t                 hHal;                       /**< handle to HAL */

    CamEngineWindow_t           acqWindow;                  /**< acquistion window size (image size on ISP input) */
    CamEngineWindow_t           outWindow;                  /**< ISP outout image isze */

    CamEngineFlickerPeriod_t    flickerPeriod;
    bool_t                      enableAfps;

    /* output size */
    uint32_t                    outWidth[CAM_ENGINE_PATH_MAX];
    uint32_t                    outHeight[CAM_ENGINE_PATH_MAX];
    bool_t                      dcEnable[CAM_ENGINE_PATH_MAX];  /**< enable dual cropping unit for path */
    CamEngineWindow_t           dcWindow[CAM_ENGINE_PATH_MAX];  /**< cropping window for path */

    /* jpeg - encoder */
    bool_t                      enableJPE;
    osEvent                     JpeEventGenHeader;

    /* Mim-Ctrl */
    osEvent                     MimEventCmdStart;
    osEvent                     MimEventCmdStop;
    MimCtrlContextHandle_t      hMimCtrl;

    /* BufSync-Ctrl */
    osEvent                     BufSyncEventStart;
    osEvent                     BufSyncEventStop;
    BufSyncCtrlHandle_t         hBufSyncCtrl;

    MediaBufPoolConfig_t        BufferPoolConfigInput;      /**< configuration input bufferpool */
    MediaBufPoolMemory_t        BufferPoolMemInput;         /**< */
    MediaBufPool_t              BufferPoolInput;            /**< */

    PicBufMetaData_t            *pDmaBuffer;
    MediaBuffer_t               *pDmaMediaBuffer;

    CamEngineBufferCb_t         cbBuffer;                   /**< callback for delivering buffers to application layer */
    void*                       *pBufferCbCtx;              /**< context of application layer (transfered with callback) */

    CamResolutionName_t         ResName;                    /**< identifier to get resolution dependend calibration data from db */
    CamCalibDbHandle_t          hCamCalibDb;                /**< handle to calibration database */

    CamEngineLockType_t         LockMask;                   /**< current lock mask (status of currently locked auto algorithms) */
    CamEngineLockType_t         LockedMask;                 /**< required lock mask for taking snapshots */

    AecHandle_t                 hAec;                       /**< handle of auto exposure module */
    AwbHandle_t                 hAwb;                       /**< handle of auto white balance module */
    AfHandle_t                  hAf;                        /**< handle of auto focus module */
    AdpfHandle_t                hAdpf;                      /**< handle of adaptive denoising prefilter module */
    AdpccHandle_t               hAdpcc;                     /**< handle of adaptive defect pixel cluster correction module */
    AvsHandle_t                 hAvs;                       /**< handle of aito video stabilization module */
    bool_t                      availableAf;                /**< support for auto focus available */

    bool_t                      chainsSwapped;

    AvsConfig_t                 avsConfig;
    AvsDampFuncParams_t         avsDampParams;
    int                         numDampData;
    double                      *pDampXData;
    double                      *pDampYData;

    float                       vGain;
    float                       vItime;

    bool_t                      isSOCsensor;    
	uint32_t					bufNum;
	uint32_t					bufSize;

	osMutex 					camEngineCtx_lock;
	uint32_t					index;
	bool_t						color_mode;
} CamEngineContext_t;


/*****************************************************************************/
/**
 * @brief   Create an instance of CamEngine
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineCreate
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   Destroy an instance of CamEngine
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineDestroy
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   initialize
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineInitCamerIc
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   undo init
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseCamerIc
(
    CamEngineContext_t  *pCamEngineCtx
);



/*****************************************************************************/
/**
 * @brief   prepare access to calibration database
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEnginePrepareCalibDbAccess
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   initialize
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineInitPixelIf
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   undo init
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleasePixelIf
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   initialize
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineInitDrvForSensor
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   initialize
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineInitDrvForTestpattern
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   initialize
 *
 * @param   pCamEngineCtx   Pointer to the context of cam-engine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineInitDrvForDma
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   reinitialize
 *
 * @param   pCamEngineCtx   Pointer to the context of cam-engine instance
 * @param   numFramesToSkip Number of frames to skip
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReInitDrv
(
    CamEngineContext_t  *pCamEngineCtx,
    uint32_t            numFramesToSkip
);


/*****************************************************************************/
/**
 * @brief   undo initialize
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineReleaseDrv
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   setup
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupAcqForSensor
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig,
    CamEngineChainIdx_t idx
);


/*****************************************************************************/
/**
 * @brief   setup
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupAcqForDma
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig,
    CamEngineChainIdx_t idx
);


/*****************************************************************************/
/**
 * @brief   setup
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupAcqForImgStab
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig,
    CamEngineChainIdx_t idx
);


/*****************************************************************************/
/**
 * @brief   setup
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupAcqResolution
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineChainIdx_t idx
);


/*****************************************************************************/
/**
 * @brief   CamEngineConfigureDataPath
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupMiDataPath
(
    CamEngineContext_t            *pCamEngineCtx,
    const CamEnginePathConfig_t   *pConfigMain,
    const CamEnginePathConfig_t   *pConfigSelf,
    CamEngineChainIdx_t           idx
);


/*****************************************************************************/
/**
 * @brief   setup
 *
 * @param   pCamEngineCtx   Pointer to the context of CamEngine instance
 *
 * @return              Return the result of the function call.
 * @retval              RET_SUCCESS
 * @retval              RET_VAL2
 *
 *****************************************************************************/
RESULT CamEngineSetupMiResolution
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineChainIdx_t idx
);


/*****************************************************************************/
/**
 * @brief   CamEnginePreloadImage
 *
 * @param   pCamEngineCtx   context of the cam-engine instance
 * @param   pCommand        pointer to command to send
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeed
 * @retval  RET_WRONG_HANDLE    pCamEngine is NULL or invalid
 * @retval  RET_CANCELED        cam-instance is shutting down
 * @retval  RET_FAILURE         write to command-queue failed
 *
 *****************************************************************************/
RESULT CamEnginePreloadImage
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);


/*****************************************************************************/
/**
 * @brief   This functions sends a command to a CamEngine instance by putting
 *          a command id into the command-queue of the cam-egine instance.
 *
 * @param   pCamEngineCtx   context of the CamEngine instance
 * @param   pCommand        pointer to command to send
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeed
 * @retval  RET_WRONG_HANDLE    pCamEngine is NULL or invalid
 * @retval  RET_CANCELED        cam-instance is shutting down
 * @retval  RET_FAILURE         write to command-queue failed
 *
 *****************************************************************************/
RESULT CamEngineSendCommand
(
    CamEngineContext_t   *pCamEngineCtx,
    CamEngineCmd_t       *pCommand
);



/******************************************************************************
 * CamEngineStartPixelIf
 *****************************************************************************/
RESULT CamEngineStartPixelIf
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineConfig_t   *pConfig
);

#ifdef __cplusplus
}
#endif


/* @} cam_engine */

#endif /* __CAM_ENGINE_H__ */

