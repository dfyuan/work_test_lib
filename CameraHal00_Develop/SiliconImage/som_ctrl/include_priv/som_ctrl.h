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
 * @som_ctrl.h
 *
 * @brief
 *   Internal stuff used by som ctrl implementation.
 *
 *****************************************************************************/
/**
 * @page som_ctrl_page SOM Ctrl
 * The Snapshot Output Module captures image buffers handed in on disk.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref som_ctrl_api
 * - @ref som_ctrl_common
 * - @ref som_ctrl
 *
 * @defgroup som_ctrl SOM Ctrl
 * @{
 *
 */


#ifndef __SOM_CTRL_H__
#define __SOM_CTRL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cea_861.h>
#include <common/picture_buffer.h>

#include <oslayer/oslayer.h>

#include <hal/hal_api.h>

#include "som_ctrl_common.h"

/**
 * @brief   Internal states of the som control.
 *
 */
typedef enum
{
    eSomCtrlStateInvalid      = 0,  //!< FSM state is invalid since SOM instance does not exist, is currently being created or is currently being shutdown.
    eSomCtrlStateIdle,              //!< FSM is in state Idle.
    eSomCtrlStateRunning            //!< FSM is in state Running.
} somCtrlState_t;


/**
 * @brief   Context of som control instance. Holds all information required for operation.
 *
 * @note
 *
 */
typedef struct somCtrlContext_s
{
    somCtrlState_t                      State;              //!< Holds internal state.

    uint32_t                            MaxCommands;        //!< Max number of commands that can be queued.
    somCtrlCompletionCb_t               somCbCompletion;    //!< Buffer completion callback.
    void                                *pUserContext;      //!< User context to pass in to buffer completion callback.
    HalHandle_t                         HalHandle;          //!< HAL session to use for HW access.

    osQueue                             CommandQueue;       //!< Command queue; holds elements of type @ref somCtrlCmd_t.
    osThread                            Thread;             //!< Command processing thread.

    osQueue                             FullBufQueue;
    uint32_t                            MaxBuffers;

    somCtrlCmdParams_t                  *pStartParams;      //!< Reference to the params structure of the current start command. @note: The referenced data is expected to be valid until the command is completed!
    char                                acBaseFileName[FILENAME_MAX]; //!< Base filename; buffer type and size information will be added to name as well as mumber of image in sequence if applicable.
    uint32_t                            NumOfFrames;        //!< Number of frames/buffers to capture; 0 (zero) for continuous capture until @ref SOM_CTRL_CMD_STOP is issued.
    uint32_t                            NumSkipFrames;      //!< Number of frames/buffers to skip before capture starts.
    uint32_t                            AverageFrames;      //!< Calculate and store per pixel average data; Note: only for RAW type images; 1 < @ref NumOfFrames < 65535 must hold true.
    bool_t                              ForceRGBOut;        //!< Converts YCbCr & RGB type image buffers to standard RGB; YCbCr is upscaled to 4:4:4 by simple pixel/line doubling as necessary
    bool_t                              ExtendName;         //!< Automatically extend the base filename with format, size, date/time, sequence and other information
    uint32_t                            FramesCaptured;     //!< Number of next buffer to capture; incremented with every captured buffer.
    uint32_t                            FramesLost;         //!< Number of frames/buffers lost during capture.
    uint32_t                            *pAveragedData;     //!< Pointer to averaged image data; valid only if @ref AverageFrames == true; NULL otherwise.
    FILE                                *pFile;             //!< File handle used during continuous capture.
    FILE                                *pFileRight;        //!< File handle used during continuous capture.

    bool_t                              ExifHeader;         //!< write jpeg with exif header or hardware generated jfif header

    PicBufType_t                        CurrentPicType;     //!< Type of picture data currently captured.
    PicBufLayout_t                      CurrentPicLayout;   //!< Kind of data layout currently captured.
    uint32_t                            CurrentPicWidth;    //!< Width of image currently captured.
    uint32_t                            CurrentPicHeight;   //!< Height of image currently captured.
    struct tm                           FileCreationTime;   //!< Creation time of first file in a sequence of files. Used for creation of file names.
} somCtrlContext_t;



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
extern RESULT somCtrlCreate
(
    somCtrlContext_t    *pSomContext
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
extern RESULT somCtrlDestroy
(
    somCtrlContext_t    *pSomContext
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
extern RESULT somCtrlSendCommand
(
    somCtrlContext_t    *pSomContext,
    somCtrlCmd_t        *pCommand
);


/* @} som_ctrl */

#ifdef __cplusplus
}
#endif

#endif /* __SOM_CTRL_H__ */
