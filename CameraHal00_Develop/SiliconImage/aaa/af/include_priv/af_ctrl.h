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
#ifndef __AF_CTRL_H__
#define __AF_CTRL_H__

/**
 * @file aec_ctrl.h
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
 * @defgroup AF Auto Focus Control
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>

#include <isi/isi_iss.h>
#include <isi/isi.h>

#include "bssa.h"

#ifdef __cplusplus
extern "C"
{
#endif



typedef RESULT (AfSearchFunc_t) (AfHandle_t handle, const CamerIcAfmMeasuringResult_t *pMeasResult, int32_t *pulLensPos );



typedef enum AfSearchState_e
{
    AFM_FSSTATE_INVALID      = 0,           /**< invalid state of search engine */
    AFM_FSSTATE_INIT         = 1,           /**< restart search cycle */
    AFM_FSSTATE_SEARCHFOCUS  = 2,           /**< searching focus */
    AFM_FSSTATE_FOCUSFOUND   = 3,           /**< searching focus */
    AFM_FSSTATE_MAX
} AfSearchState_t;

typedef struct AfSeachPos_s
{
    List                    nlist;
    List                    plist;
    int32_t                 index;
    int32_t                 pos;  
    float                   sharpness;
    float                   dSharpness;
} AfSeachPos_t;

typedef struct AfSeachPath_s
{
    int32_t                 index;
    List                    plist;
    List                    nlist;
    osMutex                 lock;
    List                    *curPos;
    List                    *maxSharpnessPos;
} AfSeachPath_t;

typedef struct AfSeachContext_s
{
    AfSearchState_t         State;
    int32_t                 LensePosMin;
    int32_t                 LensePosMax;

    float                   MinSharpness;
    float                   MaxSharpness;
    int32_t                 MaxSharpnessPos;
    int32_t                 Step;

    AfSeachPath_t           Path;
} AfSearchContext_t;



typedef enum AfState_e
{
    AF_STATE_INVALID        = 0,
    AF_STATE_INITIALIZED    = 1,
    AF_STATE_STOPPED        = 2,                /**< stopped */
    AF_STATE_RUNNING        = 3,                /**< searching for best lense position */
    AF_STATE_TRACKING       = 4,                /**< tracking */
    AF_STATE_LOCKED         = 5,                /**< */
    AF_STATE_DNFIRMWARE     = 6,
    AF_STATE_MAX
} AfState_t;


typedef struct AfEvtQuePool_s 
{
    List                   QueLst;
    osMutex                lock;
} AfEvtQuePool_t;

typedef struct AfChkAckItem_s
{
    bool_t                shot;
} AfChkAckItem_t;

typedef enum AfmCmd_e
{
    AF_ONESHOT,
    AF_TRACKING,
    AF_STOP,
    AF_SHOTCHK,
} AfmCmd_t;

typedef struct AfmCmdItem_s
{
    AfmCmd_t                cmd;
    AfSearchStrategy_t      fss;
} AfmCmdItem_t;

typedef struct AfContext_s
{
    AfState_t               state;              /**< state of auto focus module */
    AfState_t               state_before_lock;  /**< state before lock succeded */

    osQueue                 afmCmdQue;
    osQueue                 afmChkAckQue;
    osEvent                 event_afm;
    uint32_t                oneShotWait;
    uint32_t                afm_cnt;
    uint32_t                afm_inval;
    uint32_t                afmFrmIntval; 
    uint32_t                vcm_move_idx;
    uint32_t                vcm_move_frames;
    uint32_t                vcm_movefull_t;
    
    IsiSensorHandle_t       hSensor;            /**< sensor handle on which the ae system is working */
    IsiSensorHandle_t       hSubSensor;
    CamerIcDrvHandle_t      hCamerIc;           /**< cameric handle */

    AfSearchStrategy_t      Afss;               /**< enumeration type for search strategy */
    AfSearchFunc_t          *pAfSearchFunc;     /**< function pointer to search strategy */
    AfSearchContext_t       AfSearchCtx;        /**< search context */

    uint32_t                MinFocus;           /**< min lense position of sensor */
    uint32_t                MaxFocus;           /**< max lense position od sensor */

    float                   oldSharpness;
    float                   dSharpnessRawLog[10];
    uint32_t                dSharpnessRawIdx;

    bool_t                  MachineMoved;
    uint32_t                StableFrames;

    CamerIcWindow_t         measureWdw;  
    bool_t                  WdwChanged;

    bool_t                  trackNotify;
    
    float                   dSharpness;     
    float                   Sharpness;          /**< sharpness value */
    int32_t                 LensePos;           /**< current lense position */

    uint32_t                SOC_af_maxCommands;                /**< Max pending commands in command queue. */
    osQueue                 SOC_af_commandQueue;               /**< Command queue. */
    osThread                SOC_af_thread;                     /**< CamEngine thread. */

	bool                    isSOCAf;
    AfEvtQuePool_t          EvtQuePool;
} AfContext_t;

typedef enum SOC_AfCmdId_e
{
    SOC_AF_CMD_FIRMWARE    = 1,
    SOC_AF_CMD_FOCUS       = 2,
    SOC_AF_CMD_STOP        = 3,
    SOC_AF_CMD_EXIT        = 4,
} SOC_AfCmdId_t;


typedef struct SOC_AfCmd_s
{
    SOC_AfCmdId_t    cmdId;
    void            *pCmdCtx;
} SOC_AfCmd_t;


#ifdef __cplusplus
}
#endif

/* @} AF */


#endif /* __AF_CTRL_H__*/
