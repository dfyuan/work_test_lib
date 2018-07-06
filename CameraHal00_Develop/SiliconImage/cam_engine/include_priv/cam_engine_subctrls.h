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
 * @file cam_engine_subctrls.h
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

#ifndef __CAM_ENGINE_SUBCTRLS_H__
#define __CAM_ENGINE_SUBCTRLS_H__

#include <ebase/types.h>
#include <ebase/dct_assert.h>
#include <common/misc.h>
#include <oslayer/oslayer.h>

#include <common/return_codes.h>

#include "cam_engine.h"

#define PIC_BUFFER_ALIGN            ( 0x400 )                   // 1K aligment for buffer
#define PIC_BUFFER_MEM_SIZE_MAX     ( 256 * 1024 * 1024 )       // total memory size (256 MB )

// mode image processing
#define PIC_BUFFER_NUM_INPUT        ( 1 )
#define PIC_BUFFER_SIZE_INPUT       ( 42 * 1024 * 1024 )    // 21MPixel@12bit => 42MByte
#define PIC_BUFFER_NUM_MAIN_IMAGE   1//( 4 )//zyc modify
#define PIC_BUFFER_SIZE_MAIN_IMAGE  ( 1 * 1024 * 1024 )    // 21MPixel@12bit => 42MByte
#define PIC_BUFFER_NUM_SELF_IMAGE   4//( 6 ) //zyc modify
#define PIC_BUFFER_SIZE_SELF_IMAGE  ( 6 * 1024 * 1024 )     // 2MPixel@4:4:4  => 6MByte
DCT_ASSERT_STATIC( ( (PIC_BUFFER_NUM_INPUT*PIC_BUFFER_SIZE_INPUT) \
                        + (PIC_BUFFER_NUM_MAIN_IMAGE*PIC_BUFFER_SIZE_MAIN_IMAGE) \
                        + (PIC_BUFFER_NUM_SELF_IMAGE*PIC_BUFFER_SIZE_SELF_IMAGE)) < PIC_BUFFER_MEM_SIZE_MAX );

// mode 2D single pipe
#define PIC_BUFFER_NUM_SIMP_INPUT           ( 1 )
#define PIC_BUFFER_SIZE_SIMP_INPUT          ( 6 * 1024 * 1024 )    //  2MPixel@12bit => 42MByte
#define PIC_BUFFER_NUM_MAIN_SENSOR          (4)//( 4 ) //zyc modify
#define PIC_BUFFER_SIZE_MAIN_SENSOR         ( 30 * 1024 * 1024 )    // ISI_RES_3264_2448 => 8MPixel@12bit => 16MByte
#define PIC_BUFFER_NUM_SELF_SENSOR          ( 6 ) // zyc modify,not use self patch now
#define PIC_BUFFER_SIZE_SELF_SENSOR         ( 9 * 1024 * 1024 )     // 2MPixel@4:4:4  => 6MByte
DCT_ASSERT_STATIC( ( (PIC_BUFFER_NUM_SIMP_INPUT*PIC_BUFFER_SIZE_SIMP_INPUT)
                        + (PIC_BUFFER_NUM_MAIN_SENSOR*PIC_BUFFER_SIZE_MAIN_SENSOR) \
                        + (PIC_BUFFER_NUM_SELF_SENSOR*PIC_BUFFER_SIZE_SELF_SENSOR)) < PIC_BUFFER_MEM_SIZE_MAX );

// mode 2D with image stabilization
#define PIC_BUFFER_NUM_MAIN_SENSOR_IMGSTAB  ( 10 )
#define PIC_BUFFER_SIZE_MAIN_SENSOR_IMGSTAB ( 6 * 1024 * 1024 )    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte
#define PIC_BUFFER_NUM_SELF_SENSOR_IMGSTAB  ( 10 )
#define PIC_BUFFER_SIZE_SELF_SENSOR_IMGSTAB ( 6 * 1024 * 1024 )    // ISI_RES_4416_3312 => 14MPixel@12bit => 28MByte
DCT_ASSERT_STATIC( ( (PIC_BUFFER_NUM_SIMP_INPUT*PIC_BUFFER_SIZE_SIMP_INPUT)
                        + (PIC_BUFFER_NUM_MAIN_SENSOR_IMGSTAB*PIC_BUFFER_SIZE_MAIN_SENSOR_IMGSTAB) \
                        + (PIC_BUFFER_NUM_SELF_SENSOR_IMGSTAB*PIC_BUFFER_SIZE_SELF_SENSOR_IMGSTAB)) < PIC_BUFFER_MEM_SIZE_MAX );

// mode 3D dual pipe
//#define PIC_BUFFER_NUM_MAIN_SENSOR_3D       ( 4 )
//#define PIC_BUFFER_SIZE_MAIN_SENSOR_3D      ( 28 * 1024 * 1024 )     // allocated twice!, master & slave chain
#define PIC_BUFFER_NUM_MAIN_SENSOR_3D       ( 10 )
#define PIC_BUFFER_SIZE_MAIN_SENSOR_3D      ( 6 * 1024 * 1024 )     // allocated twice!, master & slave chain
DCT_ASSERT_STATIC( ((PIC_BUFFER_NUM_MAIN_SENSOR_3D*2*PIC_BUFFER_SIZE_MAIN_SENSOR_3D)) < PIC_BUFFER_MEM_SIZE_MAX );


/*****************************************************************************/
/**
 *          CamEngineSubCtrlsSetup
 *
 * @brief   Create and Initialize all Cam-Engines Sub-Controls
 *
 * @param   pCamEngineCtx   context of the cam-engine instance
 *          mode            viewfinder mode
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeed
 * @retval  RET_WRONG_HANDLE    pCamEngine is NULL or invalid
 * @retval  RET_CANCELED        cam-instance is shutting down
 * @retval  RET_FAILURE         write to command-queue failed
 *
 *****************************************************************************/
RESULT CamEngineSubCtrlsSetup
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 *          CamEngineSubCtrlsStart
 *
 * @brief   Start all Cam-Engines Sub-Controls
 *
 * @param   pCamEngineCtx   context of the cam-engine instance
 *          mode            viewfinder mode
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeed
 * @retval  RET_WRONG_HANDLE    pCamEngine is NULL or invalid
 * @retval  RET_CANCELED        cam-instance is shutting down
 * @retval  RET_FAILURE         write to command-queue failed
 *
 *****************************************************************************/
RESULT CamEngineSubCtrlsStart
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 *          CamEngineSubCtrlsStop
 *
 * @brief   Stop all Cam-Engines Sub-Controls
 *
 * @param   pCamEngineCtx   context of the cam-engine instance
 *          mode            viewfinder mode
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeed
 * @retval  RET_WRONG_HANDLE    pCamEngine is NULL or invalid
 * @retval  RET_CANCELED        cam-instance is shutting down
 * @retval  RET_FAILURE         write to command-queue failed
 *
 *****************************************************************************/
RESULT CamEngineSubCtrlsStop
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 *          CamEngineSubCtrlsRelease
 *
 * @brief   Release all Cam-Engines Sub-Controls
 *
 * @param   pCamEngineCtx   context of the cam-engine instance
 *          mode            viewfinder mode
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         operation succeed
 * @retval  RET_WRONG_HANDLE    pCamEngine is NULL or invalid
 * @retval  RET_CANCELED        cam-instance is shutting down
 * @retval  RET_FAILURE         write to command-queue failed
 *
 *****************************************************************************/
RESULT CamEngineSubCtrlsRelease
(
    CamEngineContext_t  *pCamEngineCtx
);


/*****************************************************************************/
/**
 * @brief   TODO
 *
 *****************************************************************************/
RESULT CamEngineSubCtrlsRegisterBufferCb
(
    CamEngineContext_t  *pCamEngineCtx,
    CamEngineBufferCb_t fpCallback,
    void                *pBufferCbCtx
);


/*****************************************************************************/
/**
 * @brief   TODO
 *
 *****************************************************************************/
RESULT CamEngineSubCtrlsDeRegisterBufferCb
(
    CamEngineContext_t  *pCamEngineCtx
);


/* @} module_name*/

#endif /* __CAM_ENGINE_SUBCTRLS_H__ */

