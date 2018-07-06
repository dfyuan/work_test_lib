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
 * @vom_ctrl_mvdu.c
 *
 * @brief
 *   Implementation of vom ctrl mvdu driver.
 *
 *****************************************************************************/
/**
 * @page vom_ctrl_page VOM Ctrl
 * The Video Output Module displays image buffers handed in via QuadMVDU_FX on
 * a connected HDMI display device.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref vom_ctrl_api
 * - @ref vom_ctrl_common
 * - @ref vom_ctrl
 * - @ref vom_ctrl_mvdu
 *
 */

#ifdef VDU_FPS
#include <sys/time.h>
#endif // VDU_FPS

#include <ebase/trace.h>

#include <common/return_codes.h>

#include <oslayer/oslayer.h>

#include <hal/hal_api.h>

#include "vom_ctrl_mvdu.h"
#include "vom_ctrl_hdmi.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(VOM_CTRL_MVDU_DEBUG, "VOM-CTRL-MVDU: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_MVDU_INFO , "VOM-CTRL-MVDU: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_MVDU_ERROR, "VOM-CTRL-MVDU: ", ERROR, 1);

CREATE_TRACER(VOM_CTRL_MVDU_SCT_DEBUG, "VOM-CTRL-MVDU: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_MVDU_SCT_INFO , "VOM-CTRL-MVDU: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_MVDU_SCT_ERROR, "VOM-CTRL-MVDU: ", ERROR, 1);

CREATE_TRACER(VOM_CTRL_MVDU_IRQ_DEBUG, "VOM-CTRL-MVDU-IRQ: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_MVDU_IRQ_INFO,  "VOM-CTRL-MVDU-IRQ: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_MVDU_IRQ_WARN,  "VOM-CTRL-MVDU-IRQ: ", WARNING, 1);
CREATE_TRACER(VOM_CTRL_MVDU_IRQ_ERROR, "VOM-CTRL-MVDU-IRQ: ", ERROR, 1);

// VDU block map
#define VDU_RESIZE_BASE       0x100
#define VDU_DIF_BASE          0x400
#define VDU_MI_BASE           0x900
#define VDU_MI_SEL            0x100

// VDU counter sizes (8192x4096)
#define VDU_H_CNT_MASK  0x1fff
#define VDU_V_CNT_MASK  0x0fff

// VDU register map
#define DIF_STATUS_OFFS             0x0000
#define DIF_DIF_BUSY_MASK           0x00020000 // for VDU_XL; for standard VDU:0x00001000
#define DIF_CONFIG_OFFS             0x0004
#define DIF_CONFIG_PEN_POL_MASK     0x00000001
#define DIF_CONFIG_FSYNC_POL_MASK   0x00000002
#define DIF_CONFIG_HSYNC_POL_MASK   0x00000004
#define DIF_CONFIG_VSYNC_POL_MASK   0x00000008
#define DIF_CONFIG_CTRL_POL_MASK    0x0000000f
#define DIF_LINE_CNT_THRES_OFFS     0x0008
#define DIF_DEFAULT_COL_OFFS  	    0x000C
#define DIF_FRAM_START_OFFS   	    0x0010
#define DIF_DISP_FINISH_POS_OFFS    0x0014
#define DIF_SAV_START_OFFS    	    0x0018
#define DIF_PIC_START_OFFS    	    0x001C
#define DIF_PIC_STOP_OFFS     	    0x0020
#define DIF_CLIPPING_OFFS     	    0x0024
#define DIF_PATH_ENABLE_OFFS  	    0x0028
#define DIF_601_PATH_EN_MASK        0x00000001
#define DIF_POS_MAX_OFFS      	    0x002C
#define DIF_LINE_MAX_OFFS     	    0x0030
#define DIF_EAV_F_START_OFFS  	    0x0034
#define DIF_EAV_F_STOP_OFFS   	    0x0038
#define DIF_HSYNC_START_OFFS  	    0x003C
#define DIF_HSYNC_STOP_OFFS   	    0x0040
#define DIF_VSYNC_START_OFFS  	    0x0044
#define DIF_VSYNC_STOP_OFFS   	    0x0048
#define DIF_FSYNC_START_OFFS  	    0x004C
#define DIF_FSYNC_STOP_OFFS   	    0x0050
#define DIF_ACTIVE_START_OFFS 	    0x0058
#define DIF_INIT_OFFS 	            0x005C

#define VDU_ID_OFFS                     0x0004
#define DATAPATH_CONFIG_OFFS            0x0010
#define VDU_CTRL_SET_ENABLE_VID1_MASK   0x00000001
#define DATAPATH_STATUS_OFFS            0x0014
#define VDU_CONTROL_OFFS                0x0020
#define VDU_CONTROL_START_VTG_MASK      0x00000001

#define VDU_MSK_OFFS  0x0080
#define VDU_RIS_OFFS  0x0084
#define VDU_MIS_OFFS  0x0088
#define VDU_ICR_OFFS  0x008C
#define VDU_ISR_OFFS  0x0090
#define VDU_IR_CTRL_DIF_VID1_UFLW_MASK   0x00000008
#define VDU_IR_CTRL_DIF_DISP_FINISH_MASK 0x00000001

#define RESIZE_CTRL_OFFS      0x0000
#define RESIZE_PHASE_VC_OFFS  0x0020
#define RESIZE_SIZE_VC_OFFS   0x0038

#define VDU_MI_Y_BA_OFFS             0x0000
#define VDU_MI_C_BA_OFFS             0x0004
#define VDU_MI_Y_NR_OF_ROWS_OFFS     0x0008
#define VDU_MI_C_NR_OF_ROWS_OFFS     0x000C
#define VDU_MI_Y_LINE_SIZE_OFFS      0x0010
#define VDU_MI_C_LINE_SIZE_OFFS      0x0014
#define VDU_MI_Y_FULL_LINE_SIZE_OFFS 0x0018
#define VDU_MI_C_FULL_LINE_SIZE_OFFS 0x001C
#define VDU_MI_Y_START_POS_OFFS      0x0020
#define VDU_MI_C_START_POS_OFFS      0x0024
#define VDU_MI_CFG_OFFS              0x0028
#define MVDU_FX_MI_V1_SMALL_PACKET_EN_MASK  0x00000010U
#define MVDU_FX_MI_V1_LITTLE_ENDIAN_MASK    0x00000004U
#define MVDU_FX_MI_V1_COLOR_MODE_MASK       0x00000002U

#define VDU_WRITE_REG(halhandle, addr, data) ((void) HalWriteReg(halhandle, HAL_BASEADDR_VDU + (addr), data))
#define VDU_READ_REG(halhandle, addr)        (       HalReadReg (halhandle, HAL_BASEADDR_VDU + (addr)      ))

/******************************************************************************
 * local type definitions
 *****************************************************************************/

typedef struct vomCtrlMvduDisplayConfig_s
{
    int32_t  DisplayWidth;       //!< Active width in pixel.
    int32_t  DisplayHeight;      //!< Active height in lines.
    int32_t  DisplayHfinish;
    int32_t  DisplayVfinish;
    int32_t  DisplayHmax;
    int32_t  DisplayVmax;
    int32_t  DisplayHstart;
    int32_t  DisplayVstart;
    int32_t  DisplayVstartDelay; //!< Start of real image data; used for 3D video support; set to 0 for 2D video.
    int32_t  DisplayVstop;
    int32_t  DisplayHsyncStart;
    int32_t  DisplayHsyncStop;
    int32_t  DisplayVsyncStart;
    int32_t  DisplayVsyncStop;
    uint32_t DisplayCtrlSigPol;
    uint32_t DisplayPixelClock;  //!< Given in Hz.
} vomCtrlMvduDisplayConfig_t;


typedef enum SCThreadCmd_e
{
    SCT_CMD_PAUSE = 0,
    SCT_CMD_START = 1,
    SCT_CMD_STOP  = 2
} SCThreadCmd_t;


typedef struct vomCtrlMvduContext_s
{
    vomCtrlMvduBufferReleaseCb          BufReleaseCB;
    void                                *pUserContext;
    const Cea861VideoFormatDetails_t    *pVideoFormat;
    bool_t                              Enable3D;
    Hdmi3DVideoFormat_t                 VideoFormat3D;          //!< Defined only if 3D is enabled.
    uint32_t                            MvduDisplayConfigNum;   //!< Defined only if 3D is enabled.
    vomCtrlMvduDisplayConfig_t          MvduDisplayConfig[HDMI_3D_MAX_NUM_SUBIMAGES];
    PicBufMetaData_t                    PicBufMetaData;

    osQueue                             DisplayBufferQueue;     //!< Used by IRQ DPC
    HalHandle_t                         HalHandle;              //!< Used by IRQ DPC.
    HalIrqCtx_t                         HalIrqCtx;              //!< Used by IRQ DPC.
    MediaBuffer_t                       *pBufCurr;              //!< Used by IRQ DPC. Buffer currently being displayed.
    MediaBuffer_t                       *pBufNext;              //!< Used by IRQ DPC. Buffer to be displayed next.
    uint32_t                            NextSubImageNum;        //!< Used by IRQ DPC. Number of sub image to be displayed next.
    MediaBuffer_t                       *pBufSubCurr;           //!< Used by IRQ DPC. Sub image buffer currently being displayed.

    vomCtrlHdmiHandle_t                 HdmiHandle;

    osQueue                             SCThreadCmdQueue;       //!< Holds items of type @ref SCThreadCmd_t.
    osThread                            SCThread;

} vomCtrlMvduContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/

/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   Checks picture meta data (see @ref PicBufMetaData_t) for forming
 *          a supported format & layout combination taking the video format
 *          into account.
 *
 * @param   pVideoFormat        CEA style video format description
 * @param   pPicBufMetaData     Reference to picture buffer meta data structure.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Format & layout combination is supported.
 * @retval  RET_NOTSUPP         Format & layout combination is not supported.
 * @retval  RET_OUTOFRANGE      Format and/or layout param is invalid/outofrange.
 *
 *****************************************************************************/
static RESULT vomCtrlMvduIsConfigValid
(
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    PicBufMetaData_t                    *pPicBufMetaData
);

/******************************************************************************
 * vomCtrlMvduGetDisplayConfig()
 *****************************************************************************/
static RESULT vomCtrlMvduGetDisplayConfig
(
    vomCtrlMvduHandle_t                 MvduHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
);

/******************************************************************************
 * vomCtrlMvduIrqDpc()
 *****************************************************************************/
static int32_t vomCtrlMvduIrqDpc
(
    void *pArg
);

/******************************************************************************
 * vomCtrlMvduChangeDisplay()
 *****************************************************************************/
static RESULT vomCtrlMvduChangeDisplay
(
    vomCtrlMvduHandle_t MvduHandle,
    PicBufMetaData_t    *pPicBufMetaData
);

/******************************************************************************
 * vomCtrlMvduSyncCheckThread()
 *****************************************************************************/
static int32_t vomCtrlMvduSyncCheckThread
(
    void *p_arg
);

/******************************************************************************
 * vomCtrlMvduSyncCheckAndRestart()
 *****************************************************************************/
static RESULT vomCtrlMvduSyncCheckAndRestart
(
    vomCtrlMvduHandle_t MvduHandle
);

/******************************************************************************
 * vomCtrlMvduStartHW()
 *****************************************************************************/
static RESULT vomCtrlMvduStartHW
(
    vomCtrlMvduHandle_t MvduHandle
);

/******************************************************************************
 * vomCtrlMvduStopHW()
 *****************************************************************************/
static RESULT vomCtrlMvduStopHW
(
    vomCtrlMvduHandle_t MvduHandle
);

/******************************************************************************
 * vomCtrlMvduSetIrqHW()
 *****************************************************************************/
static RESULT  vomCtrlMvduSetIrqHW
(
    vomCtrlMvduHandle_t MvduHandle,
    bool_t              EnableIRQs
);

/******************************************************************************
 * vomCtrlMvduSetPclkHW()
 *****************************************************************************/
static RESULT  vomCtrlMvduSetPclkHW
(
    vomCtrlMvduHandle_t         MvduHandle,
    vomCtrlMvduDisplayConfig_t  *pMvduDisplayConfig
);

/******************************************************************************
 * vomCtrlMvduSetDifHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetDifHW
(
    vomCtrlMvduHandle_t         MvduHandle,
    vomCtrlMvduDisplayConfig_t  *pMvduDisplayConfig
);

/******************************************************************************
 * vomCtrlMvduSetMiHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetMiHW
(
    vomCtrlMvduHandle_t         MvduHandle,
    vomCtrlMvduDisplayConfig_t  *pMvduDisplayConfig,
    PicBufMetaData_t            *pPicBufMetaData,
    bool_t                      BaseAddrOnly
);

/******************************************************************************
 * vomCtrlMvduSetHW()
 *****************************************************************************/
static RESULT  vomCtrlMvduSetHW
(
    vomCtrlMvduHandle_t                 MvduHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    vomCtrlMvduDisplayConfig_t          *pMvduDisplayConfig,
    PicBufMetaData_t                    *pPicBufMetaData
);


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * vomCtrlMvduInit()
 *****************************************************************************/
RESULT vomCtrlMvduInit
(
    vomCtrlMvduHandle_t                 *pMvduHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D,
    vomCtrlMvduBufferReleaseCb          BufReleaseCB,
    void                                *pUserContext,
    HalHandle_t                         HalHandle
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus = OSLAYER_OK;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pMvduHandle == NULL) || (pVideoFormat == NULL) || (BufReleaseCB == NULL) || (HalHandle == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // allocate context structure
    *pMvduHandle = malloc( sizeof(vomCtrlMvduContext_t) );
    if (*pMvduHandle == NULL)
    {
        return RET_OUTOFMEM;
    }

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(*pMvduHandle);

    // init context
    memset( pMvduContext, 0, sizeof(vomCtrlMvduContext_t) );
    pMvduContext->BufReleaseCB    = BufReleaseCB;
    pMvduContext->pUserContext    = pUserContext;
    pMvduContext->pVideoFormat    = pVideoFormat;
    pMvduContext->Enable3D        = Enable3D;
    pMvduContext->VideoFormat3D   = VideoFormat3D;
    pMvduContext->HalHandle       = HalHandle;

    result = vomCtrlMvduGetDisplayConfig( *pMvduHandle, pMvduContext->pVideoFormat, pMvduContext->Enable3D, pMvduContext->VideoFormat3D );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlMvduGetDisplayConfig() failed (RESULT=%d).\n", __FUNCTION__, result );
        goto cleanup_0;
    }

    // create queue of buffers ready for display
    osStatus = osQueueInit( &pMvduContext->DisplayBufferQueue, 1, sizeof(void*) );
    if (osStatus != OSLAYER_OK)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: osQueueInit() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        result = RET_FAILURE;
        goto cleanup_0;
    }

    // reference HAL
    result = HalAddRef( pMvduContext->HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: HalAddRef() failed.\n", __FUNCTION__ );
        goto cleanup_1;
    }

    // init HDMI Tx
    result = vomCtrlHdmiInit( &pMvduContext->HdmiHandle, pMvduContext->pVideoFormat, pMvduContext->Enable3D, pMvduContext->VideoFormat3D, pMvduContext->HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlHdmiInit() failed (RESULT=%d).\n", __FUNCTION__, result );
        goto cleanup_2;
    }

    // init MVDU
    vomCtrlMvduStopHW( *pMvduHandle ); // it may have been left running, e.g. by a killed process
    result = vomCtrlMvduStartHW( *pMvduHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlMvduStartHW() failed (RESULT=%d).\n", __FUNCTION__, result );
        goto cleanup_3;
    }

    // register IRQ handler
    pMvduContext->HalIrqCtx.misRegAddress = (uint32_t)(HAL_BASEADDR_VDU + VDU_MIS_OFFS);
    pMvduContext->HalIrqCtx.icrRegAddress = (uint32_t)(HAL_BASEADDR_VDU + VDU_ICR_OFFS);
    result = HalConnectIrq( pMvduContext->HalHandle, &pMvduContext->HalIrqCtx, 0, NULL, &vomCtrlMvduIrqDpc, *pMvduHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: HalConnectIrq() failed (RESULT=%d).\n", __FUNCTION__, result );
        goto cleanup_4;
    }

    // create sync check thread command queue
    osStatus = osQueueInit( &pMvduContext->SCThreadCmdQueue, 1, sizeof(SCThreadCmd_t) );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: osQueueInit() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        result = RET_FAILURE;
        goto cleanup_5;
    }

    // create sync check thread itself
    osStatus = osThreadCreate( &pMvduContext->SCThread, vomCtrlMvduSyncCheckThread, pMvduContext );
    if (OSLAYER_OK != osStatus)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: osThreadCreate() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        result = RET_FAILURE;
        goto cleanup_6;
    }

    // enable sync check thread
    SCThreadCmd_t Command = SCT_CMD_PAUSE; ////###DAV:pMvduContext->Enable3D ? SCT_CMD_PAUSE : SCT_CMD_START;
    osStatus = osQueueWrite( &pMvduContext->SCThreadCmdQueue, &Command );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s osQueueWrite() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        vomCtrlMvduDestroy( *pMvduHandle );
        return RET_FAILURE;
    }

    // enable IRQs
    result = vomCtrlMvduSetIrqHW( *pMvduHandle, BOOL_TRUE);
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlMvduSetIrqHW() failed (RESULT=%d).\n", __FUNCTION__, result );
        vomCtrlMvduDestroy( *pMvduHandle );
        return result;
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__ );

    // success
    return ( RET_SUCCESS );

    // failure cleanup
cleanup_6: // delete cmd queue
    osQueueDestroy( &pMvduContext->SCThreadCmdQueue );

cleanup_5: // disable IRQs
    HalDisconnectIrq( &pMvduContext->HalIrqCtx );

cleanup_4: // stop MVDU
    vomCtrlMvduStopHW( *pMvduHandle );

cleanup_3: // shutdown HDMI
    vomCtrlHdmiDestroy( pMvduContext->HdmiHandle );

cleanup_2: // release HAL
    HalDelRef( pMvduContext->HalHandle );

cleanup_1: // delete buffer queue
    osQueueDestroy( &pMvduContext->DisplayBufferQueue );

cleanup_0: // free context memory
    free( pMvduContext );

    return result;
}


/******************************************************************************
 * vomCtrlMvduDestroy()
 *****************************************************************************/
RESULT vomCtrlMvduDestroy
(
    vomCtrlMvduHandle_t MvduHandle
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    if (MvduHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // disable frame end IRQ
    lres = vomCtrlMvduSetIrqHW( MvduHandle, BOOL_FALSE);
    if (lres != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlMvduSetIrqHW() failed (RESULT=%d).\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // stop sync check thread first
    SCThreadCmd_t Command = SCT_CMD_STOP;
    osStatus = osQueueWrite( &pMvduContext->SCThreadCmdQueue, &Command );
    if (OSLAYER_OK == osStatus)
    {
        // wait for sync check thread having stopped
        osStatus = osThreadWait( &pMvduContext->SCThread );
        if (OSLAYER_OK != osStatus)
        {
            TRACE(VOM_CTRL_MVDU_ERROR, "%s osThreadWait() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }
    else
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s osQueueWrite() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // destroy sync check thread anyway
    osStatus = osThreadClose( &pMvduContext->SCThread );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s osThreadClose() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // destroy sync check thread cmd queue
    osStatus = osQueueDestroy( &pMvduContext->SCThreadCmdQueue );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s osQueueDestroy() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // disconnect IRQ
    HalDisconnectIrq( &pMvduContext->HalIrqCtx );

    // cleanup buffers that were already taken control off by IRQ
    if (pMvduContext->pBufCurr != NULL)
    {
        TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: BufReleaseCB(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
        pMvduContext->BufReleaseCB( pMvduContext->pUserContext, pMvduContext->pBufCurr, RET_CANCELED );
    }
    if ( (pMvduContext->pBufNext != NULL) && (pMvduContext->pBufNext != pMvduContext->pBufCurr) )
    {
        TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: BufReleaseCB(%p)\n", __FUNCTION__, pMvduContext->pBufNext );
        pMvduContext->BufReleaseCB( pMvduContext->pUserContext, pMvduContext->pBufNext, RET_CANCELED );
    }

    // shutdown MVDU
    lres = vomCtrlMvduStopHW( MvduHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlMvduStopHW() failed (RESULT=%d).\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    // shutdown HDMI
    lres = vomCtrlHdmiDestroy( pMvduContext->HdmiHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: vomCtrlHdmiDestroy() failed (RESULT=%d).\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    // release HAL
    lres = HalDelRef( pMvduContext->HalHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: HalDelRef() failed (RESULT=%d).\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    // flush & destroy queue of buffers ready for display
    MediaBuffer_t *pBuffer = NULL;
    while ( OSLAYER_OK == osQueueTryRead( &pMvduContext->DisplayBufferQueue, &pBuffer ) )
    {
        DCT_ASSERT(pBuffer != NULL);
        TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: BufReleaseCB(%p)\n", __FUNCTION__, pBuffer );
        pMvduContext->BufReleaseCB( pMvduContext->pUserContext, pBuffer, RET_CANCELED );
    }
    osStatus = osQueueDestroy( &pMvduContext->DisplayBufferQueue );
    if (osStatus != OSLAYER_OK)
    {
        TRACE( VOM_CTRL_MVDU_ERROR, "%s: osQueueDestroy() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // free context structure
    free( pMvduContext );

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * vomCtrlMvduDisplay()
 *****************************************************************************/
RESULT vomCtrlMvduDisplay
(
    vomCtrlMvduHandle_t MvduHandle,
    MediaBuffer_t       *pBuffer
)
{
    RESULT result;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (MvduHandle == NULL) || (pBuffer == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // get meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pBuffer->pMetaData);
    if (pPicBufMetaData == NULL)
    {
        return RET_NULL_POINTER;
    }

    // check format & layout are supported by us
    result = vomCtrlMvduIsConfigValid( pMvduContext->pVideoFormat, pPicBufMetaData );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // forward image to MVDU -> enqueue for processing by worker thread/IRQ on Vsync
    OSLAYER_STATUS osStatus = osQueueWrite( &pMvduContext->DisplayBufferQueue, &pBuffer );
    result = (osStatus != OSLAYER_OK) ? RET_FAILURE : RET_PENDING;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * vomCtrlMvduIsConfigValid()
 *****************************************************************************/
static RESULT vomCtrlMvduIsConfigValid
(
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    PicBufMetaData_t                    *pPicBufMetaData
)
{
    if ( (pVideoFormat == NULL) || (pPicBufMetaData == NULL) )
    {
        return RET_NULL_POINTER;
    }

    switch ( pPicBufMetaData->Type )
    {
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        case PIC_BUF_TYPE_JPEG:
        case PIC_BUF_TYPE_YCbCr444:
        case PIC_BUF_TYPE_YCbCr420:
        case PIC_BUF_TYPE_RGB888:
            return RET_NOTSUPP;
        case PIC_BUF_TYPE_YCbCr422:
            switch ( pPicBufMetaData->Layout )
            {
                case PIC_BUF_LAYOUT_SEMIPLANAR:
                    // for now we just support displaying images in exact video resolution
                    if ( (pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel  == pVideoFormat->Hactive)
                      && (pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel == pVideoFormat->Vactive)
                      && ((pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel & 1) == 0) // some more sanity checks
                      && ((pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel & 1) == 0) )
                    {
                        return RET_SUCCESS;
                    }
                    return RET_NOTSUPP;
                case PIC_BUF_LAYOUT_PLANAR:
                case PIC_BUF_LAYOUT_COMBINED:
                    return RET_NOTSUPP;
                default:
                    break;
            };
            break;
        default:
            break;
    }

    // invalid configuration
    return RET_OUTOFRANGE;
}


/******************************************************************************
 * vomCtrlMvduGetDisplayConfig()
 *****************************************************************************/
static RESULT vomCtrlMvduGetDisplayConfig
(
    vomCtrlMvduHandle_t                 MvduHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
)
{
    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (MvduHandle == NULL) || (pVideoFormat == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // get MVDU video settings
    if (!Enable3D)
    {

////        // get a copy of the video timing details and align it so that the active video area is aligned to the right & bottom of the frame/field timing
////        // this gives the VDU more time to prefill all its pipelines under heavy load conditions on the memory subsystem
////        Cea861VideoFormatDetails_t VideoFormatDetails = *pVideoFormat;
////        if (!Cea861AlignVideoFormatDetails( &VideoFormatDetails ))
////        {
////            TRACE(VOM_CTRL_MVDU_ERROR, "%s Cea861AlignVideoFormatDetails() failed\n", __FUNCTION__);
////            return RET_WRONG_CONFIG;
////        }
////        pVideoFormat = &VideoFormatDetails;
        // number of settings
        pMvduContext->MvduDisplayConfigNum = 1;

        // determine setting params
        vomCtrlMvduDisplayConfig_t *pMvduDispCfg = &pMvduContext->MvduDisplayConfig[0]; // just a shortcut
        pMvduDispCfg->DisplayWidth       = pVideoFormat->Hactive;
        pMvduDispCfg->DisplayHeight      = pVideoFormat->Vactive;
        pMvduDispCfg->DisplayHmax        = pVideoFormat->Htotal - 1;
        pMvduDispCfg->DisplayVmax        = pVideoFormat->Vtotal - 1;
        pMvduDispCfg->DisplayHfinish     = 0;
        pMvduDispCfg->DisplayVfinish     = 0;
        pMvduDispCfg->DisplayHstart      = pVideoFormat->HactStart - 1;
        pMvduDispCfg->DisplayVstart      = pVideoFormat->VactStart - 1;
        pMvduDispCfg->DisplayVstartDelay = pVideoFormat->VactStartDelay;
        pMvduDispCfg->DisplayVstop       = pMvduDispCfg->DisplayVstart + pMvduDispCfg->DisplayVstartDelay + pMvduDispCfg->DisplayHeight - 1;
        pMvduDispCfg->DisplayHsyncStart  = pVideoFormat->HsyncStart - 2;
        pMvduDispCfg->DisplayHsyncStop   = pVideoFormat->HsyncStop - 2;
        pMvduDispCfg->DisplayVsyncStart  = pVideoFormat->VsyncStart - 1;
        pMvduDispCfg->DisplayVsyncStop   = pVideoFormat->VsyncStop - 1;
        pMvduDispCfg->DisplayCtrlSigPol  =   ( pVideoFormat->EnPolarity    ? 0 : DIF_CONFIG_PEN_POL_MASK)
                                           | ( pVideoFormat->FsyncPolarity ? 0 : DIF_CONFIG_FSYNC_POL_MASK)
                                           | ( pVideoFormat->VsyncPolarity ? 0 : DIF_CONFIG_HSYNC_POL_MASK)
                                           | ( pVideoFormat->HsyncPolarity ? 0 : DIF_CONFIG_VSYNC_POL_MASK);
        pMvduDispCfg->DisplayPixelClock  = pVideoFormat->PixClk;

////        ////TODO: remove
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s Setting:\n", __FUNCTION__);
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayWidth       = %d\n", __FUNCTION__, pMvduDispCfg->DisplayWidth      );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHeight      = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHeight     );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHmax        = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHmax       );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVmax        = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVmax       );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHfinish     = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHfinish    );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVfinish     = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVfinish    );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHstart      = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHstart     );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVstart      = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVstart     );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVstartDelay = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVstartDelay);
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVstop       = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVstop      );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHsyncStart  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHsyncStart );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHsyncStop   = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHsyncStop  );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVsyncStart  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVsyncStart );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVsyncStop   = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVsyncStop  );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayCtrlSigPol  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayCtrlSigPol );
////        TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayPixelClock  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayPixelClock );
    }
    else
    {
        uint16_t idx = 0;

        // number of settings
        pMvduContext->MvduDisplayConfigNum = Hdmi3DGetNumSubImages( VideoFormat3D, pVideoFormat );
        if (pMvduContext->MvduDisplayConfigNum == 0)
        {
            TRACE(VOM_CTRL_MVDU_ERROR, "%s Hdmi3DGetNumSubImages() failed\n", __FUNCTION__);
        }
        else
        {
            // get settings for all subimages
            for (idx = 0; idx < pMvduContext->MvduDisplayConfigNum; idx++)
            {
                RESULT result;
                Cea861VideoFormatDetails_t VideoFormatDetails3D;
                Cea861VideoFormatDetails_t *pVideoFormat3D = &VideoFormatDetails3D;

                result = Hdmi3DGetVideoFormatDetails( VideoFormat3D, idx, pVideoFormat, pVideoFormat3D );
                if (RET_SUCCESS != result)
                {
                    TRACE(VOM_CTRL_MVDU_ERROR, "%s Hdmi3DGetVideoFormatDetails() failed, RESULT=%d\n", __FUNCTION__, result);
                }

                // determine setting params
                vomCtrlMvduDisplayConfig_t *pMvduDispCfg = &pMvduContext->MvduDisplayConfig[idx]; // just a shortcut
                pMvduDispCfg->DisplayWidth       = pVideoFormat3D->Hactive;
                pMvduDispCfg->DisplayHeight      = pVideoFormat3D->Vactive;
                pMvduDispCfg->DisplayHmax        = pVideoFormat3D->Htotal - 1;
                pMvduDispCfg->DisplayVmax        = pVideoFormat3D->Vtotal - 1;
                pMvduDispCfg->DisplayHfinish     = 0;
                pMvduDispCfg->DisplayVfinish     = 0;
                pMvduDispCfg->DisplayHstart      = pVideoFormat3D->HactStart - 1;
                pMvduDispCfg->DisplayVstart      = pVideoFormat3D->VactStart - 1;
                pMvduDispCfg->DisplayVstartDelay = pVideoFormat3D->VactStartDelay;
                pMvduDispCfg->DisplayVstop       = pMvduDispCfg->DisplayVstart + pMvduDispCfg->DisplayVstartDelay + pMvduDispCfg->DisplayHeight - 1;
                pMvduDispCfg->DisplayHsyncStart  = pVideoFormat3D->HsyncStart - 2;
                pMvduDispCfg->DisplayHsyncStop   = pVideoFormat3D->HsyncStop  - 2;
                pMvduDispCfg->DisplayVsyncStart  = pVideoFormat3D->VsyncStart ? pVideoFormat3D->VsyncStart - 1 : pVideoFormat3D->Vtotal + 5;
                pMvduDispCfg->DisplayVsyncStop   = pVideoFormat3D->VsyncStop  ? pVideoFormat3D->VsyncStop  - 1 : pVideoFormat3D->Vtotal + 6;
                pMvduDispCfg->DisplayCtrlSigPol  =   ( pVideoFormat3D->EnPolarity    ? 0 : DIF_CONFIG_PEN_POL_MASK)
                                                   | ( pVideoFormat3D->FsyncPolarity ? 0 : DIF_CONFIG_FSYNC_POL_MASK)
                                                   | ( pVideoFormat3D->VsyncPolarity ? 0 : DIF_CONFIG_HSYNC_POL_MASK)
                                                   | ( pVideoFormat3D->HsyncPolarity ? 0 : DIF_CONFIG_VSYNC_POL_MASK);
                pMvduDispCfg->DisplayPixelClock  = pVideoFormat3D->PixClk;

////                ////TODO: remove
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s Setting #%d:\n", __FUNCTION__, idx);
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayWidth       = %d\n", __FUNCTION__, pMvduDispCfg->DisplayWidth      );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHeight      = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHeight     );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHmax        = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHmax       );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVmax        = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVmax       );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHfinish     = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHfinish    );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVfinish     = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVfinish    );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHstart      = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHstart     );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVstart      = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVstart     );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVstartDelay = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVstartDelay);
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVstop       = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVstop      );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHsyncStart  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHsyncStart );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayHsyncStop   = %d\n", __FUNCTION__, pMvduDispCfg->DisplayHsyncStop  );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVsyncStart  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVsyncStart );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayVsyncStop   = %d\n", __FUNCTION__, pMvduDispCfg->DisplayVsyncStop  );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayCtrlSigPol  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayCtrlSigPol );
////                TRACE(VOM_CTRL_MVDU_ERROR, "%s   DisplayPixelClock  = %d\n", __FUNCTION__, pMvduDispCfg->DisplayPixelClock );
            }
        }
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( RET_SUCCESS );
}


/******************************************************************************
 * vomCtrlMvduSyncCheckThread()
 *****************************************************************************/
static int32_t vomCtrlMvduSyncCheckThread
(
    void *p_arg
)
{
    TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg == NULL )
    {
        TRACE(VOM_CTRL_MVDU_SCT_ERROR, "%s arg pointer is NULL\n", __FUNCTION__);
    }
    else
    {
        bool_t bExit = BOOL_FALSE;
        bool_t bRunning = BOOL_FALSE;
        uint32_t NumRestart = 0;

        // get handle from arg
        vomCtrlMvduHandle_t MvduHandle = (vomCtrlMvduHandle_t)p_arg;

        // get context
        vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

        // processing loop
        do
        {
            // wait for next command
            OSLAYER_STATUS osStatus;
            SCThreadCmd_t Command = 0;

            if ( bRunning == BOOL_FALSE )
            {
                osStatus = osQueueRead(&pMvduContext->SCThreadCmdQueue, &Command); // wait indefinitely for next command
            }
            else
            {
                #define FRAMES_PER_SYNC_CHECK 4
                uint32_t FrameRate = (!pMvduContext->pVideoFormat || !pMvduContext->pVideoFormat->PixRep || !pMvduContext->pVideoFormat->Htotal ||!pMvduContext->pVideoFormat->Vtotal)
                                   ? 0 : (pMvduContext->pVideoFormat->PixClk / pMvduContext->pVideoFormat->PixRep / pMvduContext->pVideoFormat->Htotal / pMvduContext->pVideoFormat->Vtotal);
                uint32_t waitTimeMs = (FrameRate != 0) ? ((1000*FRAMES_PER_SYNC_CHECK)/FrameRate) : 250; // check about every n-th frame
                TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s cmd timeout=%d ms\n", __FUNCTION__, waitTimeMs);
                osStatus = osQueueTimedRead(&pMvduContext->SCThreadCmdQueue, &Command, waitTimeMs);  // wait poll timeout for next command
            }

            switch (osStatus)
            {
                case OSLAYER_OK: // we've received a new command
                {
                    // process command
                    switch ( Command )
                    {
                        case SCT_CMD_PAUSE:
                        {
                            TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s -> disabling\n", __FUNCTION__);
                            bRunning = BOOL_FALSE;
                            break;
                        }

                        case SCT_CMD_START:
                        {
                            TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s -> enabling\n", __FUNCTION__);
                            bRunning = BOOL_TRUE;
                            NumRestart = 0;
                            break;
                        }

                        case SCT_CMD_STOP:
                        {
                            TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s -> exiting\n", __FUNCTION__);
                            bExit = BOOL_TRUE;
                            break;
                        }

                        default:
                        {
                            TRACE(VOM_CTRL_MVDU_SCT_ERROR, "%s illegal command %d\n", __FUNCTION__, Command);
                            break; // for now we simply go on receiving commands
                        }
                    }
                    break;
                }

                case OSLAYER_TIMEOUT : // it's time to poll...
                {
                    if (bRunning == BOOL_TRUE) // ...but only if we're not paused
                    {
                        TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s (begin: sync check processing)\n", __FUNCTION__);

                        RESULT result = vomCtrlMvduSyncCheckAndRestart( MvduHandle );
                        if (RET_SUCCESS == result)
                        {
                            TRACE(VOM_CTRL_MVDU_SCT_DEBUG, "%s sync check successfull; NumRestart=%d\n", __FUNCTION__, NumRestart );
                            // we can disable ourselves until display changes will wake us up again
                            bRunning = BOOL_FALSE;
                        }
                        else if (RET_WRONG_STATE == result)
                        {
                            TRACE(VOM_CTRL_MVDU_SCT_DEBUG, "%s sync check currently not possible (e.g. no display attached)\n", __FUNCTION__ );
                        }
                        else
                        {
                            NumRestart++;

                            if (RET_OUTOFRANGE == result)
                            {
                                TRACE(VOM_CTRL_MVDU_SCT_DEBUG, "%s restart successfull; NumRestart=%d\n", __FUNCTION__, NumRestart );
                            }
                            else
                            {
                                TRACE(VOM_CTRL_MVDU_SCT_ERROR, "%s sync check or restart failed, RESULT=%d; NumRestart=%d\n", __FUNCTION__, result, NumRestart );
                            }
                        }

                        TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s (end: sync check processing)\n", __FUNCTION__);
                    }
                    break;
                }

                default: // bad things have happened
                    TRACE(VOM_CTRL_MVDU_SCT_ERROR, "%s receiving command failed -> OSLAYER_RESULT=%d\n", __FUNCTION__, osStatus);
                    break; // for now we simply go on receiving commands
            }
        } while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s (exit)\n", __FUNCTION__);

    return 0;
}


/******************************************************************************
 * vomCtrlMvduSyncCheckAndRestart()
 *****************************************************************************/
static RESULT vomCtrlMvduSyncCheckAndRestart
(
    vomCtrlMvduHandle_t MvduHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // check if restart is required
    result = vomCtrlHdmiCheckValidSync( pMvduContext->HdmiHandle );
    if (result == RET_SUCCESS)
    {
        TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s HDMI detects valid syncs\n", __FUNCTION__);
    }
    else if (result == RET_WRONG_STATE)
    {
        TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s HDMI currently can't detect syncs (e.g. no display is connected)\n", __FUNCTION__);
    }
    else if (result == RET_OUTOFRANGE)
    {
        RESULT lres;
        result = RET_SUCCESS;

        TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s restarting MVDU+PCLK\n", __FUNCTION__);

        // disable IRQs
        lres = vomCtrlMvduSetIrqHW( MvduHandle, BOOL_FALSE);
        if (lres != RET_SUCCESS)
        {
            TRACE( VOM_CTRL_MVDU_SCT_ERROR, "%s: vomCtrlMvduSetIrqHW() failed (RESULT=%d).\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // disconnect IRQ
        HalDisconnectIrq( &pMvduContext->HalIrqCtx );

        // shutdown MVDU
        lres = vomCtrlMvduStopHW( MvduHandle );
        if (lres != RET_SUCCESS)
        {
            TRACE( VOM_CTRL_MVDU_SCT_ERROR, "%s: vomCtrlMvduStopHW() failed (RESULT=%d).\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // cleanup of buffers that were already taken control off by IRQ...
        // ...release last buffer displayed
        if ( pMvduContext->pBufCurr && (pMvduContext->pBufCurr != pMvduContext->pBufNext) )
        {
            TRACE( VOM_CTRL_MVDU_SCT_DEBUG, "%s: BufReleaseCB(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
            pMvduContext->BufReleaseCB( pMvduContext->pUserContext, pMvduContext->pBufCurr, RET_SUCCESS );
        }

        // ...advance display mini-pipeline
        pMvduContext->pBufCurr = pMvduContext->pBufNext;

        // ...get next buffer from the output queue
        OSLAYER_STATUS osStatus = osQueueTryRead( &pMvduContext->DisplayBufferQueue, &pMvduContext->pBufNext );
        if (osStatus != OSLAYER_OK)
        {
            // no next buffer
            // was the current buffer also the last one?
            if ( (pMvduContext->pBufCurr) && (pMvduContext->pBufCurr->last) )
            {
                // well, that it's then
                TRACE( VOM_CTRL_MVDU_SCT_DEBUG, "%s: was last buffer(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
                pMvduContext->pBufNext = NULL;
            }
            else
            {
                // display the last one again
                TRACE( VOM_CTRL_MVDU_SCT_DEBUG, "%s: repeat buffer(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
                pMvduContext->pBufNext = pMvduContext->pBufCurr;
            }
        }
        else
        {
            TRACE( VOM_CTRL_MVDU_SCT_DEBUG, "%s: next buffer(%p)\n", __FUNCTION__, pMvduContext->pBufNext );
        }

        // ...reset 3D sub image stuff
        pMvduContext->NextSubImageNum = 0;
        pMvduContext->pBufSubCurr = NULL;

        // re-init MVDU
        lres = vomCtrlMvduStartHW( MvduHandle );
        if (lres != RET_SUCCESS)
        {
            TRACE( VOM_CTRL_MVDU_SCT_ERROR, "%s: vomCtrlMvduStartHW() failed (RESULT=%d).\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // re-register IRQ handler
        pMvduContext->HalIrqCtx.misRegAddress = (uint32_t)(HAL_BASEADDR_VDU + VDU_MIS_OFFS);
        pMvduContext->HalIrqCtx.icrRegAddress = (uint32_t)(HAL_BASEADDR_VDU + VDU_ICR_OFFS);
        lres = HalConnectIrq( pMvduContext->HalHandle, &pMvduContext->HalIrqCtx, 0, NULL, &vomCtrlMvduIrqDpc, MvduHandle );
        if (lres != RET_SUCCESS)
        {
            TRACE( VOM_CTRL_MVDU_SCT_ERROR, "%s: HalConnectIrq() failed (RESULT=%d).\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // re-enable IRQs
        lres = vomCtrlMvduSetIrqHW( MvduHandle, BOOL_TRUE);
        if (lres != RET_SUCCESS)
        {
            TRACE( VOM_CTRL_MVDU_SCT_ERROR, "%s: vomCtrlMvduSetIrqHW() failed (RESULT=%d).\n", __FUNCTION__, lres );
            UPDATE_RESULT( result, lres );
        }

        // tell caller that MVDU wasn't producing valid sync signals and was therefor restarted
        // but do this only if restart was successfull -> UPDATE_RESULT already takes care of that!
        UPDATE_RESULT( result, RET_OUTOFRANGE );
    }
    else
    {
        TRACE(VOM_CTRL_MVDU_SCT_ERROR, "%s checking HDMI sync state failed (RESULT=%d)\n", __FUNCTION__, result);
    }

    TRACE(VOM_CTRL_MVDU_SCT_INFO, "%s (exit)\n", __FUNCTION__ );

    return result;
}


/******************************************************************************
 * vomCtrlMvduIrqDpc()
 *****************************************************************************/
static int32_t vomCtrlMvduIrqDpc
(
    void *p_arg
)
{
    // get IRQ context from args
    HalIrqCtx_t *pHalIrqCtx = (HalIrqCtx_t*)(p_arg);

    DCT_ASSERT(pHalIrqCtx != NULL);

    TRACE( VOM_CTRL_MVDU_IRQ_INFO, "%s: (enter mis=%08x) \n", __FUNCTION__, pHalIrqCtx->misValue);

    // get MVDU handle & context
    vomCtrlMvduHandle_t  MvduHandle    = (vomCtrlMvduHandle_t)(pHalIrqCtx->OsIrq.p_context);
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);
    DCT_ASSERT(pMvduContext != NULL);

    // acknowledge ALL pending irqs we've been called for at once
    VDU_WRITE_REG( pHalIrqCtx->HalHandle, pHalIrqCtx->icrRegAddress - HAL_BASEADDR_VDU, pHalIrqCtx->misValue); // need to use VDU reg offset for macro call, but icrRegAddress includes VDU base address as well

    // handle IRQs...
    // ...VID1 data underflow irq
    if ( pHalIrqCtx->misValue & VDU_IR_CTRL_DIF_VID1_UFLW_MASK )
    {
        // try to recover from data underflow on MI VID1 path (already done by clearing the irq)
        TRACE( VOM_CTRL_MVDU_IRQ_WARN, "%s: VID1 data underflow -> recovering\n", __FUNCTION__ );
    }

    // ...frame end irq
    if ( pHalIrqCtx->misValue & VDU_IR_CTRL_DIF_DISP_FINISH_MASK )
    {
        RESULT result;
        MediaBuffer_t *pBufSubNext = NULL;

#ifdef VDU_FPS
        {
            // get current time
            static int32_t lastUs;
            struct timeval now;
            gettimeofday( &now, NULL );
            int32_t nowUs = (now.tv_sec*1000000) + now.tv_usec;
            int32_t deltaUs = nowUs - lastUs;
            lastUs = nowUs;
            if (((uint32_t)deltaUs) < 1000000)
            {
                TRACE( VOM_CTRL_MVDU_IRQ_ERROR, "%s: Delta time %d us\n", __FUNCTION__, deltaUs );
            }
        }
#endif // VDU_FPS

        // do we have another sub image to display?
        if (++pMvduContext->NextSubImageNum < pMvduContext->MvduDisplayConfigNum)
        {
            // yes, so try to get it
            pBufSubNext = (pMvduContext->pBufSubCurr != NULL) ? pMvduContext->pBufSubCurr->pNext : NULL; // pBufSubNext = NULL is perfectly OK, we'll deal with that later on
            TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: %s sub buffer #%d (%p)\n", __FUNCTION__, pBufSubNext?"Next":"No next", pMvduContext->NextSubImageNum, pBufSubNext );
        }
        else
        {
            // no, so release last buffer displayed
            if ( pMvduContext->pBufCurr && (pMvduContext->pBufCurr != pMvduContext->pBufNext) )
            {
                TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: BufReleaseCB(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
                pMvduContext->BufReleaseCB( pMvduContext->pUserContext, pMvduContext->pBufCurr, RET_SUCCESS );
            }

            // advance display mini-pipeline
            pMvduContext->pBufCurr = pMvduContext->pBufNext;

            // reset to first sub image
            pMvduContext->NextSubImageNum = 0;

            // get next buffer from the output queue
            OSLAYER_STATUS osStatus = osQueueTryRead( &pMvduContext->DisplayBufferQueue, &pMvduContext->pBufNext );
            if (osStatus != OSLAYER_OK)
            {
                // no next buffer, was it the last buffer?
                if ( (pMvduContext->pBufCurr) && (pMvduContext->pBufCurr->last) )
                {
                    // well, that it's then
                    TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: was last buffer(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
                    pMvduContext->pBufNext = NULL;
                }
                else
                {
                    // display the last one again
                    TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: repeat buffer(%p)\n", __FUNCTION__, pMvduContext->pBufCurr );
                    pMvduContext->pBufNext = pMvduContext->pBufCurr;
                }
            }
            else
            {
                TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: next buffer(%p)\n", __FUNCTION__, pMvduContext->pBufNext );
            }

            // 'get' sub image
            pBufSubNext = pMvduContext->pBufNext; // pBufSubNext = NULL is perfectly OK, we'll deal with that later on
            TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: %s sub buffer #%d (%p)\n", __FUNCTION__, pBufSubNext?"First":"No first", pBufSubNext );

        }
        TRACE( VOM_CTRL_MVDU_IRQ_DEBUG, "%s: sub buffer #%d (%p)\n", __FUNCTION__, pMvduContext->NextSubImageNum, pBufSubNext );

        // try to get next (sub) image meta data
        PicBufMetaData_t *pNextPicBufMetaData = (pBufSubNext != NULL) ? (PicBufMetaData_t*)(pBufSubNext->pMetaData) : NULL; // pNextPicBufMetaData = NULL is perfectly OK, we'll deal with that later on

        // check format or layout have changed and update settings accordingly
        if (pMvduContext->MvduDisplayConfigNum == 0)
        {
            if (pNextPicBufMetaData != NULL)
            {
                result = vomCtrlMvduChangeDisplay( MvduHandle, pNextPicBufMetaData );
                if (result != RET_SUCCESS)
                {
                    TRACE( VOM_CTRL_MVDU_IRQ_ERROR, "%s: vomCtrlMvduChangeDisplay() failed (result = %d)\n", __FUNCTION__ , result);
                }
            }
        }

        // update DIF settings
        result = vomCtrlMvduSetDifHW( MvduHandle, &pMvduContext->MvduDisplayConfig[pMvduContext->NextSubImageNum] );
        if (result != RET_SUCCESS)
        {
            TRACE( VOM_CTRL_MVDU_IRQ_ERROR, "%s: vomCtrlMvduSetDifHW() failed (result = %d)\n", __FUNCTION__ , result);
        }

        // set up next (sub) image
        if (pNextPicBufMetaData != NULL)
        {
            // update display buffer pointers in MI
            result = vomCtrlMvduSetMiHW( MvduHandle, &pMvduContext->MvduDisplayConfig[pMvduContext->NextSubImageNum], pNextPicBufMetaData, true );
            if (result != RET_SUCCESS)
            {
                TRACE( VOM_CTRL_MVDU_IRQ_ERROR, "%s: vomCtrlMvduSetMiHW() failed (result = %d)\n", __FUNCTION__ , result);
            }

            // enable data path
            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00808000 ); // black background
            VDU_WRITE_REG( pMvduContext->HalHandle, DATAPATH_CONFIG_OFFS, VDU_CTRL_SET_ENABLE_VID1_MASK);
        }
        else
        {
            // disable data path
            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00808040 ); // gray background
            VDU_WRITE_REG( pMvduContext->HalHandle, DATAPATH_CONFIG_OFFS, 0);
        }

        // store sub buffer for sub image chain walking
        pMvduContext->pBufSubCurr = pBufSubNext;
    }

    TRACE( VOM_CTRL_MVDU_IRQ_INFO, "%s: (exit)\n", __FUNCTION__ );

    return ( 0 );
}


/******************************************************************************
 * vomCtrlMvduChangeDisplay()
 *****************************************************************************/
static RESULT vomCtrlMvduChangeDisplay
(
    vomCtrlMvduHandle_t MvduHandle,
    PicBufMetaData_t    *pPicBufMetaData
)
{
    RESULT result;
    OSLAYER_STATUS osStatus;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);
    DCT_ASSERT(pPicBufMetaData != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // check format & layout are supported by us
    result = vomCtrlMvduIsConfigValid( pMvduContext->pVideoFormat, pPicBufMetaData );
    if (result != RET_SUCCESS)
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s vomCtrlMvduIsConfigValid -> %d\n", __FUNCTION__, result);
        return result;
    }

    // check for any changes in settings
    if ( (pMvduContext->PicBufMetaData.Type   == pPicBufMetaData->Type)
      && (pMvduContext->PicBufMetaData.Layout == pPicBufMetaData->Layout) )
    {
        switch ( pPicBufMetaData->Type )
        {
            case PIC_BUF_TYPE_YCbCr422:
                switch ( pPicBufMetaData->Layout )
                {
                    case PIC_BUF_LAYOUT_SEMIPLANAR:
                        if ( (pMvduContext->PicBufMetaData.Data.YCbCr.semiplanar.Y.PicWidthPixel     == pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel    )
                          && (pMvduContext->PicBufMetaData.Data.YCbCr.semiplanar.Y.PicHeightPixel    == pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel   )
                          && (pMvduContext->PicBufMetaData.Data.YCbCr.semiplanar.CbCr.PicWidthPixel  == pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel )
                          && (pMvduContext->PicBufMetaData.Data.YCbCr.semiplanar.CbCr.PicHeightPixel == pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicHeightPixel) )
                        {
                            TRACE(VOM_CTRL_MVDU_INFO, "%s (exit) no change\n", __FUNCTION__);
                            return RET_SUCCESS; // nothing has changed
                        }
                        break;
                    default:
                        TRACE(VOM_CTRL_MVDU_ERROR, "%s invalid pPicBufMetaData->Layout\n", __FUNCTION__);
                        return RET_NOTSUPP;
                };
                break;
            default:
                TRACE(VOM_CTRL_MVDU_ERROR, "%s invalid pPicBufMetaData->Type\n", __FUNCTION__);
                return RET_NOTSUPP;
        }
    }

    // change MVDU setup
    result = vomCtrlMvduSetHW( MvduHandle, pMvduContext->pVideoFormat, &pMvduContext->MvduDisplayConfig[0], pPicBufMetaData );
    if (result == RET_SUCCESS)
    {
        // remember new settings
        pMvduContext->PicBufMetaData = *pPicBufMetaData;
    }
    else
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s vomCtrlMvduSetHW -> %d\n", __FUNCTION__, result);
        // force a change retry at next image to be displayed
        pMvduContext->PicBufMetaData.Type   = PIC_BUF_TYPE_INVALID;
        pMvduContext->PicBufMetaData.Layout = PIC_BUF_LAYOUT_INVALID;
    }

    // re-start sync check thread
    SCThreadCmd_t Command = SCT_CMD_PAUSE; ////###DAV:pMvduContext->Enable3D ? SCT_CMD_PAUSE : SCT_CMD_START;
    osStatus = osQueueWrite( &pMvduContext->SCThreadCmdQueue, &Command );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_MVDU_ERROR, "%s osQueueWrite() failed (OSLAYER_STATUS=%d).\n", __FUNCTION__, osStatus );
        UPDATE_RESULT( result, RET_FAILURE );
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * Internal functions for HW accesses
 *****************************************************************************/
#define USE_NEW_RESET_STRATEGY 0 // 0: old, 1: new reset strategy

/******************************************************************************
 * vomCtrlMvduStartHW()
 *****************************************************************************/
static RESULT vomCtrlMvduStartHW
(
    vomCtrlMvduHandle_t MvduHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // make sure, MVDU reset is released (here: MVDU+PClk)
    HalSetReset( pMvduContext->HalHandle, HAL_DEVID_VDU | HAL_DEVID_PCLK, false );
    osSleep(10);

#if !(USE_NEW_RESET_STRATEGY)
    // stop DIF module
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_CONTROL_OFFS, 0 ); // just set vtg_start bit to 0

    // wait for DIF ain't busy anymore
    uint32_t Timeout;
    for (Timeout = 100; (Timeout != 0) && ((VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_STATUS_OFFS ) & DIF_DIF_BUSY_MASK) != 0); Timeout--)
    {
        osSleep(10);
    }
    UPDATE_RESULT( result, (Timeout == 0) ? RET_BUSY : RET_SUCCESS);

    // reset MVDU (here: MVDU+PClk)
    HalSetReset( pMvduContext->HalHandle, HAL_DEVID_VDU | HAL_DEVID_PCLK, true );
    osSleep(1);
    HalSetReset( pMvduContext->HalHandle, HAL_DEVID_VDU | HAL_DEVID_PCLK, false );
    osSleep(10);
#endif //(USE_NEW_RESET_STRATEGY)

    // create a special initial idle setting with no syncs & no active region; display finish at end of frame
    // this bank will be active for a certain time, depending on DisplayPixelClock
    vomCtrlMvduDisplayConfig_t MvduStartupDispCfg =
    {
           0, // DisplayWidth       = pVideoFormat->Hactive;
           0, // DisplayHeight      = pVideoFormat->Vactive;
        2000, // DisplayHmax        = pVideoFormat->Htotal - 1;
        1000, // DisplayVmax        = pVideoFormat->Vtotal - 1;
        2002, // DisplayHfinish     = 0;
        1000, // DisplayVfinish     = 0;
        2001, // DisplayHstart      = pVideoFormat->HactStart - 1;
        1002, // DisplayVstart      = pVideoFormat->VactStart - 1;
           0, // DisplayVstartDelay = pVideoFormat->VactStartDelay;
        1001, // DisplayVstop       = pMvduDispCfg->DisplayVstart + pMvduDispCfg->DisplayVstartDelay + pMvduDispCfg->DisplayHeight - 1;
        2003, // DisplayHsyncStart  = pVideoFormat->HsyncStart - 2;
        2002, // DisplayHsyncStop   = pVideoFormat->HsyncStop - 2;
        1004, // DisplayVsyncStart  = pVideoFormat->VsyncStart - 1;
        1003, // DisplayVsyncStop   = pVideoFormat->VsyncStop - 1;
           0, // DisplayCtrlSigPol
           0  // DisplayPixelClock
    };

    // take these from real settings to avoid glitches on start of real timing
    MvduStartupDispCfg.DisplayCtrlSigPol = pMvduContext->MvduDisplayConfig[0].DisplayCtrlSigPol;
    MvduStartupDispCfg.DisplayPixelClock = pMvduContext->MvduDisplayConfig[0].DisplayPixelClock;

    // set Pclk generator
    result = vomCtrlMvduSetPclkHW( MvduHandle, &MvduStartupDispCfg );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // set DIF module ('current' register bank)
    result = vomCtrlMvduSetDifHW( MvduHandle, &MvduStartupDispCfg );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // set MI module ('current' register bank)
    result = vomCtrlMvduSetMiHW( MvduHandle, &MvduStartupDispCfg, NULL, false );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // start DIF module (using 'current' register bank's settings)
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_CONTROL_OFFS, VDU_CONTROL_START_VTG_MASK );

    // change display finish position to beginning of frame; this bank switches out again immediately
    MvduStartupDispCfg.DisplayHfinish = 0;
    MvduStartupDispCfg.DisplayVfinish = 0;

    // set DIF module ('next' register bank)
    result = vomCtrlMvduSetDifHW( MvduHandle, &MvduStartupDispCfg );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // set MI module ('next' register bank)
    result = vomCtrlMvduSetMiHW( MvduHandle, &MvduStartupDispCfg, NULL, false );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlMvduStopHW()
 *****************************************************************************/
static RESULT vomCtrlMvduStopHW
(
    vomCtrlMvduHandle_t MvduHandle
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // make sure, MVDU reset is released (here: MVDU+PClk)
    HalSetReset( pMvduContext->HalHandle, HAL_DEVID_VDU | HAL_DEVID_PCLK, false );
    osSleep(1);

#if (USE_NEW_RESET_STRATEGY)
    // flush MI VID1 data path
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_ISR_OFFS, VDU_IR_CTRL_DIF_VID1_UFLW_MASK );
    osSleep(100);
#else
    // disable datapath
    VDU_WRITE_REG( pMvduContext->HalHandle, DATAPATH_CONFIG_OFFS, 0 );  // set_enable_vid1 = 0

    // disable output
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PATH_ENABLE_OFFS , 0 );  // dif_601_path_en = 0

    // stop DIF module
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_CONTROL_OFFS, 0 ); // just set vtg_start bit to 0

    // wait for DIF is not busy anymore
    uint32_t Timeout;
    for (Timeout = 100; (Timeout != 0) && ((VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_STATUS_OFFS ) & DIF_DIF_BUSY_MASK) != 0); Timeout--)
    {
        osSleep(10);
    }
    UPDATE_RESULT( result, (Timeout == 0) ? RET_BUSY : RET_SUCCESS);
#endif // (USE_NEW_RESET_STRATEGY)
    // shutdown Pclk generator
    vomCtrlMvduSetPclkHW( MvduHandle, NULL );

    // reset MVDU (here: MVDU+PClk)
    HalSetReset( pMvduContext->HalHandle, HAL_DEVID_VDU | HAL_DEVID_PCLK, true );
    osSleep(1);
    HalSetReset( pMvduContext->HalHandle, HAL_DEVID_VDU | HAL_DEVID_PCLK, false );

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlMvduSetIrqHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetIrqHW
(
    vomCtrlMvduHandle_t MvduHandle,
    bool_t              EnableIRQs
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // enable/disable IRQs?
    if (EnableIRQs)
    {
        // enable handled irqs
        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_ICR_OFFS, VDU_IR_CTRL_DIF_DISP_FINISH_MASK | VDU_IR_CTRL_DIF_VID1_UFLW_MASK);
        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MSK_OFFS, VDU_IR_CTRL_DIF_DISP_FINISH_MASK | VDU_IR_CTRL_DIF_VID1_UFLW_MASK);
    }
    else
    {
        // disable all irqs
        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MSK_OFFS, 0);
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlMvduSetPclkHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetPclkHW
(
    vomCtrlMvduHandle_t         MvduHandle,
    vomCtrlMvduDisplayConfig_t  *pMvduDisplayConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // set or stop?
    if (pMvduDisplayConfig == NULL)
    {
        //TODO: stop Pclk generator
        //...
    }
    else
    {
        TRACE(VOM_CTRL_MVDU_INFO, "%s HalSetClock(%d)\n", __FUNCTION__, pMvduDisplayConfig->DisplayPixelClock);

        // set Pclk generator
        result  = HalSetClock( pMvduContext->HalHandle, HAL_DEVID_PCLK, pMvduDisplayConfig->DisplayPixelClock );
        if (RET_SUCCESS != result)
        {
            TRACE(VOM_CTRL_MVDU_ERROR, "%s HalSetClock(%d) failed\n", __FUNCTION__, pMvduDisplayConfig->DisplayPixelClock);
        }
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlMvduSetDifHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetDifHW
(
    vomCtrlMvduHandle_t         MvduHandle,
    vomCtrlMvduDisplayConfig_t  *pMvduDisplayConfig
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);
    DCT_ASSERT(pMvduDisplayConfig != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // set MVDU
    #define VDU_H_TIMING(_horizontal) ((_horizontal) & VDU_H_CNT_MASK)
    #define VDU_V_TIMING(_vertical)   ((_vertical)   & VDU_V_CNT_MASK)
    #define VDU_HV_TIMING(_vertical, _horizontal) ( (VDU_V_TIMING(_vertical) << 16) | VDU_H_TIMING(_horizontal) )

    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_CONFIG_OFFS         , pMvduDisplayConfig->DisplayCtrlSigPol & DIF_CONFIG_CTRL_POL_MASK);
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_LINE_CNT_THRES_OFFS , 0x4f0 );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS    , 0x0066F03F ); //00CbCrYY //709:red
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_FRAM_START_OFFS     , 0 );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DISP_FINISH_POS_OFFS, VDU_HV_TIMING( pMvduDisplayConfig->DisplayVfinish, pMvduDisplayConfig->DisplayHfinish ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_SAV_START_OFFS      , VDU_H_TIMING( pMvduDisplayConfig->DisplayHstart - 2 ) ); // 656 not used but configured correctly to avoid potential hiccups
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PIC_START_OFFS      , VDU_HV_TIMING( pMvduDisplayConfig->DisplayVstart, pMvduDisplayConfig->DisplayHstart ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PIC_STOP_OFFS       , VDU_V_TIMING( pMvduDisplayConfig->DisplayVstop ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_CLIPPING_OFFS       , 0xFF00FF00 );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PATH_ENABLE_OFFS    , DIF_601_PATH_EN_MASK );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_POS_MAX_OFFS        , VDU_H_TIMING( pMvduDisplayConfig->DisplayHmax ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_LINE_MAX_OFFS       , VDU_V_TIMING( pMvduDisplayConfig->DisplayVmax ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_EAV_F_START_OFFS    , VDU_V_TIMING( 0 ) ); // 656 not used
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_EAV_F_STOP_OFFS     , VDU_V_TIMING( 0 ) ); // 656 not used
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_HSYNC_START_OFFS    , VDU_H_TIMING( pMvduDisplayConfig->DisplayHsyncStart ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_HSYNC_STOP_OFFS     , VDU_H_TIMING( pMvduDisplayConfig->DisplayHsyncStop ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_VSYNC_START_OFFS    , VDU_HV_TIMING( pMvduDisplayConfig->DisplayVsyncStart, pMvduDisplayConfig->DisplayHsyncStart ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_VSYNC_STOP_OFFS     , VDU_HV_TIMING( pMvduDisplayConfig->DisplayVsyncStop, pMvduDisplayConfig->DisplayHsyncStart ) );
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_FSYNC_START_OFFS    , VDU_HV_TIMING( 0, 0 ) ); // interlaced not used
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_FSYNC_STOP_OFFS     , VDU_HV_TIMING( 0, 0 ) ); // interlaced not used
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_ACTIVE_START_OFFS   , VDU_HV_TIMING( pMvduDisplayConfig->DisplayVstartDelay, 0 ) ); // numbers are relative to DIF_PIC_START position!
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_INIT_OFFS           , 0x00000000 );

    // select special default color                                                    CbCrYY
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00D4781C ); //709:blue
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00483A70 ); //601:green
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x001092D2 ); //709:yellow
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x0066F03F ); //709:red
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00F07620 ); //709:blue
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00E0770E ); //709:blue, 235->255
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x009A10BC ); //709:cyan
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00D6E64E ); //709:magenta
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x008080C0 ); //gray
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x00808000 ); //709:black
    //VDU_WRITE_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS, 0x009A10BC ); //709:cyan

    // setup chroma scaler (part I)
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_RESIZE_BASE + RESIZE_CTRL_OFFS     , 0 ); // scale_vc_enable = 0
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_RESIZE_BASE + RESIZE_PHASE_VC_OFFS , 0 );

////    //TODO: remove
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s DIF Register dump #%d\n", __FUNCTION__, pMvduDisplayConfig->DisplayVstartDelay?1:0 );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_CONFIG_OFFS         ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_CONFIG_OFFS          ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_LINE_CNT_THRES_OFFS ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_LINE_CNT_THRES_OFFS  ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_DEFAULT_COL_OFFS    ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DEFAULT_COL_OFFS     ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_FRAM_START_OFFS     ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_FRAM_START_OFFS      ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_DISP_FINISH_POS_OFFS", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_DISP_FINISH_POS_OFFS ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_SAV_START_OFFS      ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_SAV_START_OFFS       ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_PIC_START_OFFS      ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PIC_START_OFFS       ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_PIC_STOP_OFFS       ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PIC_STOP_OFFS        ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_CLIPPING_OFFS       ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_CLIPPING_OFFS        ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_PATH_ENABLE_OFFS    ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_PATH_ENABLE_OFFS     ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_POS_MAX_OFFS        ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_POS_MAX_OFFS         ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_LINE_MAX_OFFS       ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_LINE_MAX_OFFS        ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_EAV_F_START_OFFS    ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_EAV_F_START_OFFS     ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_EAV_F_STOP_OFFS     ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_EAV_F_STOP_OFFS      ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_HSYNC_START_OFFS    ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_HSYNC_START_OFFS     ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_HSYNC_STOP_OFFS     ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_HSYNC_STOP_OFFS      ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_VSYNC_START_OFFS    ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_VSYNC_START_OFFS     ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_VSYNC_STOP_OFFS     ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_VSYNC_STOP_OFFS      ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_FSYNC_START_OFFS    ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_FSYNC_START_OFFS     ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_FSYNC_STOP_OFFS     ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_FSYNC_STOP_OFFS      ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_ACTIVE_START_OFFS   ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_ACTIVE_START_OFFS    ) );
////    TRACE(VOM_CTRL_MVDU_DEBUG, "%s %s = 0x%08x\n", __FUNCTION__, "DIF_INIT_OFFS           ", VDU_READ_REG( pMvduContext->HalHandle, VDU_DIF_BASE + DIF_INIT_OFFS            ) );

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlMvduSetMiHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetMiHW
(
    vomCtrlMvduHandle_t         MvduHandle,
    vomCtrlMvduDisplayConfig_t  *pMvduDisplayConfig,
    PicBufMetaData_t            *pPicBufMetaData,
    bool_t                      BaseAddrOnly
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter) %s\n", __FUNCTION__, BaseAddrOnly ? "BaseAddr" : "Full");

    DCT_ASSERT(MvduHandle != NULL);
    DCT_ASSERT(pMvduDisplayConfig != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    ulong_t  YDataBase;
    uint32_t YNumRows;
    uint32_t YLineSize;
    uint32_t YFullLineSize;
    ulong_t  CDataBase;
    uint32_t CNumRows;
    uint32_t CLineSize;
    uint32_t CFullLineSize;

    // real settings or default values?
    if (pPicBufMetaData != NULL)
    {
        // pre-extract some config data
        if (pPicBufMetaData->Type != PIC_BUF_TYPE_YCbCr422) // we support this type only
        {
            return RET_NOTSUPP;
        }

        // get raw image data
        YDataBase      = (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.pBuffer);
        YNumRows       = (uint32_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicHeightPixel);
        YLineSize      = (uint32_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthPixel);
        YFullLineSize  = (uint32_t)(pPicBufMetaData->Data.YCbCr.semiplanar.Y.PicWidthBytes);
        CDataBase      = (ulong_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.pBuffer);
        CNumRows       = (uint32_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicHeightPixel);
        CLineSize      = (uint32_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthPixel);
        CFullLineSize  = (uint32_t)(pPicBufMetaData->Data.YCbCr.semiplanar.CbCr.PicWidthBytes);

        //TODO: adopt to video format; e.g. scale, pan, etc.
        //....
    }
    else
    {
        YDataBase      = 0;
        YNumRows       = 0;
        YLineSize      = 0;
        YFullLineSize  = 0;
        CDataBase      = 0;
        CNumRows       = 0;
        CLineSize      = 0;
        CFullLineSize  = 0;
    }

    // setup chroma scaler (part II)
    VDU_WRITE_REG( pMvduContext->HalHandle, VDU_RESIZE_BASE + RESIZE_SIZE_VC_OFFS, YNumRows );

    // set MVDU
    // setup 4 MI masters, master 0 is connected, master 1...3 execute accesses and receive dummy data
    int i;
    for (i = 0;  i < 4;  ++i) {
        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_Y_BA_OFFS            , YDataBase );
        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_C_BA_OFFS            , CDataBase );

        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_Y_FULL_LINE_SIZE_OFFS, YFullLineSize );
        VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_C_FULL_LINE_SIZE_OFFS, CFullLineSize );

       ////TODO: was if (!BaseAddrOnly)
        {
            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_Y_NR_OF_ROWS_OFFS    , YNumRows );
            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_C_NR_OF_ROWS_OFFS    , CNumRows );

            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_Y_LINE_SIZE_OFFS     , YLineSize );
            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_C_LINE_SIZE_OFFS     , CLineSize );

            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_Y_START_POS_OFFS     , 0x00000000 );
            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_C_START_POS_OFFS     , 0x00000000 );

            VDU_WRITE_REG( pMvduContext->HalHandle, VDU_MI_BASE + VDU_MI_SEL * i + VDU_MI_CFG_OFFS             , MVDU_FX_MI_V1_SMALL_PACKET_EN_MASK
                                                                                                               | MVDU_FX_MI_V1_LITTLE_ENDIAN_MASK
                                                                                                               | MVDU_FX_MI_V1_COLOR_MODE_MASK ); // == 0x16
        }
    }

    //TODO: remove
    TRACE(VOM_CTRL_MVDU_DEBUG, "%s MI Register dump\n", __FUNCTION__ );
    TRACE(VOM_CTRL_MVDU_DEBUG, "RESIZE_SIZE_VC_OFFS          = 0x%08x\n", YNumRows );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_Y_BA_OFFS             = 0x%08x\n", YDataBase );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_C_BA_OFFS             = 0x%08x\n", CDataBase );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_Y_NR_OF_ROWS_OFFS     = 0x%08x\n", YNumRows );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_C_NR_OF_ROWS_OFFS     = 0x%08x\n", CNumRows );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_Y_LINE_SIZE_OFFS      = 0x%08x\n", YLineSize );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_C_LINE_SIZE_OFFS      = 0x%08x\n", CLineSize );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_Y_FULL_LINE_SIZE_OFFS = 0x%08x\n", YFullLineSize );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_C_FULL_LINE_SIZE_OFFS = 0x%08x\n", CFullLineSize );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_Y_START_POS_OFFS      = 0x%08x\n", 0x00000000 );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_C_START_POS_OFFS      = 0x%08x\n", 0x00000000 );
    TRACE(VOM_CTRL_MVDU_DEBUG, "VDU_MI_CFG_OFFS              = 0x%08x\n", MVDU_FX_MI_V1_SMALL_PACKET_EN_MASK
                                                                        | MVDU_FX_MI_V1_LITTLE_ENDIAN_MASK
                                                                        | MVDU_FX_MI_V1_COLOR_MODE_MASK ); // == 0x16

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlMvduSetHW()
 *****************************************************************************/
static RESULT vomCtrlMvduSetHW
(
    vomCtrlMvduHandle_t                 MvduHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    vomCtrlMvduDisplayConfig_t          *pMvduDisplayConfig,
    PicBufMetaData_t                    *pPicBufMetaData
)
{
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_MVDU_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(MvduHandle != NULL);
    DCT_ASSERT(pMvduDisplayConfig != NULL);
    DCT_ASSERT(pPicBufMetaData != NULL);

    // get context
    vomCtrlMvduContext_t *pMvduContext = (vomCtrlMvduContext_t*)(MvduHandle);

    // set Pclk generator
    result = vomCtrlMvduSetPclkHW( MvduHandle, pMvduDisplayConfig );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // set DIF module
    result = vomCtrlMvduSetDifHW( MvduHandle, pMvduDisplayConfig );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // set MI module
    result = vomCtrlMvduSetMiHW( MvduHandle, pMvduDisplayConfig, pPicBufMetaData, false );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    TRACE(VOM_CTRL_MVDU_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}
