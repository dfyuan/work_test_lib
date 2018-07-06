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
 * @vom_ctrl_hdmi.c
 *
 * @brief
 *   Implementation of vom ctrl hdmi tx driver.
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

#include <ebase/trace.h>

#include <common/return_codes.h>
#include <common/picture_buffer.h>

#include <oslayer/oslayer.h>

#include <hal/hal_api.h>

#include "vom_ctrl_hdmi.h"

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(VOM_CTRL_HDMI_DEBUG, "VOM-CTRL-HDMI: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_HDMI_INFO , "VOM-CTRL-HDMI: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_HDMI_ERROR, "VOM-CTRL-HDMI: ", ERROR, 1);

CREATE_TRACER(VOM_CTRL_HDMI_HPT_DEBUG, "VOM-CTRL-HDMI: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_HDMI_HPT_INFO , "VOM-CTRL-HDMI: ", INFO, 0);
CREATE_TRACER(VOM_CTRL_HDMI_HPT_ERROR, "VOM-CTRL-HDMI: ", ERROR, 1);

//RESULT HalReadI2CReg(  HalHandle_t HalHandle, uint8_t bus_num, uint16_t slave_addr, uint32_t reg_address, uint8_t reg_addr_size, uint32_t *preg_value, uint8_t reg_size )
//RESULT HalWriteI2CReg( HalHandle_t HalHandle, uint8_t bus_num, uint16_t slave_addr, uint32_t reg_address, uint8_t reg_addr_size, uint32_t  reg_value,  uint8_t reg_size )
#define HDMI_READ_REG(  _pHdmiContext, _regAddr, _pData )  HalReadI2CReg(  (_pHdmiContext)->HalHandle, (_pHdmiContext)->I2cBusNum, (_pHdmiContext)->I2cSlaveAddr,  (_regAddr), 1, (_pData), 1 )
#define HDMI_WRITE_REG( _pHdmiContext, _regAddr, _Data )   HalWriteI2CReg( (_pHdmiContext)->HalHandle, (_pHdmiContext)->I2cBusNum, (_pHdmiContext)->I2cSlaveAddr,  (_regAddr), 1, (_Data),  1 )
#define HDMI_WRITE_REG2( _pHdmiContext, _regAddr, _Data )  HalWriteI2CReg( (_pHdmiContext)->HalHandle, (_pHdmiContext)->I2cBusNum, (_pHdmiContext)->I2cSlaveAddr2, (_regAddr), 1, (_Data),  1 )


/******************************************************************************
 * local type definitions
 *****************************************************************************/

typedef enum HPThreadCmd_e
{
    HPT_CMD_PAUSE = 0,
    HPT_CMD_START = 1,
    HPT_CMD_STOP  = 2
} HPThreadCmd_t;

typedef struct vomCtrlHdmiContext_s
{
    HalHandle_t                 HalHandle;
    uint8_t                     I2cBusNum;
    uint8_t                     I2cSlaveAddr;
    uint8_t                     I2cSlaveAddr2;

    osQueue                     HPThreadCmdQueue; //!< Holds items of type @ref HPThreadCmd_t.
    osThread                    HPThread;

    osMutex                     HwAccessLock;
    Cea861VideoFormatDetails_t  VideoFormat;
    bool_t                      Enable3D;
    Hdmi3DVideoFormat_t         VideoFormat3D;
} vomCtrlHdmiContext_t;


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 * @brief   Checks picture meta data (see @ref PicBufMetaData_t) and frame rate
 *          for forming a supported format, layout & framerate combination.
 *
 * @param   pVideoFormat        Reference to CEA style video format description.
 * @param   Enable3D            Enable 3D display mode.
 * @param   VideoFormat3D       The HDMI 3D display mode to use; undefined if 3D not enabled.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         Format, layout & frame rate combination is supported.
 * @retval  RET_NOTSUPP         Format, layout & frame rate combination is not supported.
 * @retval  RET_OUTOFRANGE      Format and/or layout and/or frame rate param is invalid/outofrange.
 *
 *****************************************************************************/
static RESULT vomCtrlHdmiIsConfigValid
(
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
);

/*****************************************************************************/
/**
 * @brief   Periodically polls HDMI HW to see if downstream device was plugged in.
 *
 * @param   p_arg               HDMI context handle as returned by @ref vomCtrlHdmiInit.
 *
 * @return  Always returns 0.
 *
 *****************************************************************************/
static int32_t vomCtrlHdmiHotPlugThread
(
    void *p_arg
);

/*****************************************************************************/
/**
 * @brief   Start HDMI transmitter.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         HDMI transmitter started.
 * @retval  RET_FAILURE         Error starting HDMI transmitter.
 *
 *****************************************************************************/
static RESULT vomCtrlHdmiStartHW
(
    vomCtrlHdmiHandle_t HdmiHandle
);

/*****************************************************************************/
/**
 * @brief   Stop HDMI transmitter.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         HDMI transmitter stopped.
 * @retval  RET_FAILURE         Error stopping HDMI transmitter.
 *
 *****************************************************************************/
static RESULT vomCtrlHdmiStopHW
(
    vomCtrlHdmiHandle_t HdmiHandle
);

/*****************************************************************************/
/**
 * @brief   Tell HDMI transmitter about new image settings.
 *
 * @note    This is just a shell doing the HW locking and then calling
 *          @ref vomCtrlHdmiSetHWcore to do the real work.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 * @param   pVideoFormat        Reference to CEA style video format description.
 * @param   Enable3D            Enable 3D display mode.
 * @param   VideoFormat3D       The HDMI 3D display mode to use; undefined if 3D not enabled.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         HDMI transmitter reconfigured.
 * @retval  RET_FAILURE         Error reconfiguring HDMI transmitter.
 *
 *****************************************************************************/
static RESULT vomCtrlHdmiSetHW
(
    vomCtrlHdmiHandle_t                 HdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
);

/*****************************************************************************/
/**
 * @brief   Tell HDMI transmitter about new image settings from within HW access
 *          functions that already hold the HW access lock.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 * @param   pVideoFormat        Reference to CEA style video format description.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         HDMI transmitter reconfigured.
 * @retval  RET_FAILURE         Error reconfiguring HDMI transmitter.
 *
 *****************************************************************************/
static RESULT vomCtrlHdmiSetHWcore
(
    vomCtrlHdmiHandle_t                 HdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
);

/*****************************************************************************/
/**
 * @brief   Check HDMI transmitter for hot plug and restore image settings.
 *
 * @param   HdmiHandle          HDMI context handle as returned by @ref vomCtrlHdmiInit.
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS         No hot plug detected or HDMI transmitter reconfigured.
 * @retval  RET_FAILURE         Error detecting hot plug or reconfiguring HDMI transmitter.
 *
 *****************************************************************************/
static RESULT vomCtrlHdmiCheckHotPlugAndSetHW
(
    vomCtrlHdmiHandle_t HdmiHandle
);


/******************************************************************************
 * API functions; see header file for detailed comment.
 *****************************************************************************/

/******************************************************************************
 * vomCtrlHdmiInit()
 *****************************************************************************/
RESULT vomCtrlHdmiInit
(
    vomCtrlHdmiHandle_t                 *pHdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D,
    HalHandle_t                         HalHandle
)
{
    RESULT result;
    OSLAYER_STATUS osStatus;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (pHdmiHandle == NULL) || (pVideoFormat == NULL) || (HalHandle == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // check format, layout & frame rate are supported by us
    result = vomCtrlHdmiIsConfigValid( pVideoFormat, Enable3D, VideoFormat3D );
    if (result != RET_SUCCESS)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s configuration invalid\n", __FUNCTION__);
        return result;
    }

    // allocate context structure
    *pHdmiHandle = malloc( sizeof(vomCtrlHdmiContext_t) );
    if (*pHdmiHandle == NULL)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s allocating context memory failed\n", __FUNCTION__);
        return RET_OUTOFMEM;
    }

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(*pHdmiHandle);

    // init context
    memset( pHdmiContext, 0, sizeof(vomCtrlHdmiContext_t) );
    pHdmiContext->HalHandle     = HalHandle;
    pHdmiContext->I2cBusNum     = HAL_I2C_BUS_HDMI_TX;
    pHdmiContext->I2cSlaveAddr  = 0x39;
    pHdmiContext->I2cSlaveAddr2 = 0x38;

    // reference HAL
    result = HalAddRef( pHdmiContext->HalHandle );
    if (result != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_HDMI_ERROR, "%s: HalAddRef() failed.\n", __FUNCTION__ );
        goto cleanup_0;
    }

    // init hw access lock
    osStatus = osMutexInit( &pHdmiContext->HwAccessLock );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s creating hw access lock failed\n", __FUNCTION__);
        result = RET_FAILURE;
        goto cleanup_1;
    }

    // create hot plug monitor thread cmd queue
    osStatus = osQueueInit( &pHdmiContext->HPThreadCmdQueue, 1, sizeof(HPThreadCmd_t) );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s creating command queue failed\n", __FUNCTION__);
        result = RET_FAILURE;
        goto cleanup_2;
    }

    // create hot plug monitor thread itself
    osStatus = osThreadCreate( &pHdmiContext->HPThread, vomCtrlHdmiHotPlugThread, pHdmiContext );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s creating hot plug monitor thread failed\n", __FUNCTION__);
        result = RET_FAILURE;
        goto cleanup_3;
    }

    // start HDMI
    result = vomCtrlHdmiStartHW( *pHdmiHandle );
    if (result != RET_SUCCESS)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s vomCtrlHdmiStartHW() failed (result = %d)\n", __FUNCTION__, result);
        goto cleanup_4;
    }

    // set HDMI TX
    result = vomCtrlHdmiChangeDisplay( *pHdmiHandle, pVideoFormat, Enable3D, VideoFormat3D );
    if (result != RET_SUCCESS)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s vomCtrlHdmiChangeDisplay() failed (result = %d)\n", __FUNCTION__, result);
        goto cleanup_4;
    }

    // enable hot plug monitor thread
    HPThreadCmd_t Command = HPT_CMD_START;
    osStatus = osQueueWrite( &pHdmiContext->HPThreadCmdQueue, &Command );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s sending enable cmd to hot plug monitor thread failed\n", __FUNCTION__);
        goto cleanup_4;
    }

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__ );

    // success
    return ( RET_SUCCESS );

    // failure cleanup
cleanup_4: // destroy HDMI
    vomCtrlHdmiDestroy( *pHdmiHandle );

cleanup_3: // delete cmd queue
    osQueueDestroy( &pHdmiContext->HPThreadCmdQueue );

cleanup_2: // delete HW mutex
    osMutexDestroy( &pHdmiContext->HwAccessLock );

cleanup_1: // release HAL
    HalDelRef( pHdmiContext->HalHandle );

cleanup_0: // free context memory
    free( pHdmiContext );

    return result;;
}


/******************************************************************************
 * vomCtrlHdmiDestroy()
 *****************************************************************************/
RESULT vomCtrlHdmiDestroy
(
    vomCtrlHdmiHandle_t HdmiHandle
)
{
    RESULT result = RET_SUCCESS;
    RESULT lres;
    OSLAYER_STATUS osStatus;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    if (HdmiHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // stop hot plug monitor thread first
    HPThreadCmd_t Command = HPT_CMD_STOP;
    osStatus = osQueueWrite( &pHdmiContext->HPThreadCmdQueue, &Command );
    if (OSLAYER_OK == osStatus)
    {
        // wait for hot plug monitor thread having stopped
        osStatus = osThreadWait( &pHdmiContext->HPThread );
        if (OSLAYER_OK != osStatus)
        {
            TRACE(VOM_CTRL_HDMI_ERROR, "%s waiting hot plug monitor thread failed\n", __FUNCTION__);
            UPDATE_RESULT( result, RET_FAILURE );
        }
    }
    else
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s sending stop cmd to hot plug monitor thread failed\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // destroy hot plug monitor thread anyway
    osStatus = osThreadClose( &pHdmiContext->HPThread );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s closing hot plug monitor thread failed\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // destroy hot plug monitor thread cmd queue
    osStatus = osQueueDestroy( &pHdmiContext->HPThreadCmdQueue );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s destryoing hot plug monitor thread cmd queue failed\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // then stop HDMI
    lres = vomCtrlHdmiStopHW( HdmiHandle );
    if (RET_SUCCESS != lres)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s vomCtrlHdmiStopHW() failed\n", __FUNCTION__);
        UPDATE_RESULT( result, lres);
    }

     // destroy hw access lock last
    osStatus = osMutexDestroy( &pHdmiContext->HwAccessLock );
    if (OSLAYER_OK != osStatus)
    {
        TRACE(VOM_CTRL_HDMI_ERROR, "%s destroying hw access lock failed\n", __FUNCTION__);
        UPDATE_RESULT( result, RET_FAILURE );
    }

    // release HAL
    lres = HalDelRef( pHdmiContext->HalHandle );
    if (lres != RET_SUCCESS)
    {
        TRACE( VOM_CTRL_HDMI_INFO, "%s: HalDelRef() failed (RESULT=%d).\n", __FUNCTION__, lres );
        UPDATE_RESULT( result, lres );
    }

    // free context structure
    free( pHdmiContext );

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__ );

    return ( result );
}


/******************************************************************************
 * vomCtrlHdmiChangeDisplay()
 *****************************************************************************/
RESULT vomCtrlHdmiChangeDisplay
(
    vomCtrlHdmiHandle_t                 HdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
)
{
    RESULT result;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    if ( (HdmiHandle == NULL) || (pVideoFormat == NULL) )
    {
        return RET_NULL_POINTER;
    }

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // check format is supported by us
    result = vomCtrlHdmiIsConfigValid( pVideoFormat, Enable3D, VideoFormat3D );
    if (result != RET_SUCCESS)
    {
        return result;
    }

    // change HDMI setup
    result = vomCtrlHdmiSetHW( HdmiHandle, pVideoFormat, Enable3D, VideoFormat3D );

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlHdmiCheckValidSync()
 *****************************************************************************/
RESULT vomCtrlHdmiCheckValidSync
(
    vomCtrlHdmiHandle_t HdmiHandle
)
{
    OSLAYER_STATUS osStatus;
    RESULT result = RET_SUCCESS;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    if (HdmiHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // this block in fact is a HW access function, but being lazy and the function being
    // really short, the API & internal hw access parts are not split up into separate functions
    {
        RESULT lRes = RET_SUCCESS;
        uint8_t data = 0;

        // get access lock
        osStatus = osMutexLock( &pHdmiContext->HwAccessLock );
        DCT_ASSERT(OSLAYER_OK == osStatus);

        // check if HDMI has detected valid sync signals
        lRes = HDMI_READ_REG( pHdmiContext, 0x3e, &data ); //TODO: check why this line is doubled
        lRes = HDMI_READ_REG( pHdmiContext, 0x3e, &data );
        if (RET_SUCCESS == lRes)
        {
            uint32_t format = (data >> 2);
            if (format != 0)
            {
                // valid syncs detected
                TRACE(VOM_CTRL_HDMI_DEBUG, "%s valid syncs detected; format=%d\n", __FUNCTION__, format);
            }
            else
            {
                // no valid syncs detected, let's see why
                lRes = HDMI_READ_REG( pHdmiContext, 0x41, &data );
                if (RET_SUCCESS == lRes)
                {
                    if (data & 0x40)
                    {
                        // we're currently in power down state (e.g. because no display is connected)
                        TRACE(VOM_CTRL_HDMI_INFO, "%s can't check for valid syncs in power down state\n", __FUNCTION__);
                        UPDATE_RESULT( result, RET_WRONG_STATE );
                    }
                    else
                    {
                        // looks like there are no valid syncs present
                        TRACE(VOM_CTRL_HDMI_DEBUG, "%s no valid syncs detected\n", __FUNCTION__);
                        UPDATE_RESULT( result, RET_OUTOFRANGE );
                    }
                }
                else
                {
                    TRACE(VOM_CTRL_HDMI_ERROR, "%s error accessing i2c; result=%d\n", __FUNCTION__, lRes);
                    UPDATE_RESULT( result, lRes );
                }
            }
        }
        else
        {
            TRACE(VOM_CTRL_HDMI_ERROR, "%s error accessing i2c; result=%d\n", __FUNCTION__, lRes);
            UPDATE_RESULT( result, lRes );
        }

        // release access lock
        osStatus = osMutexUnlock( &pHdmiContext->HwAccessLock );
        DCT_ASSERT(OSLAYER_OK == osStatus);

    }
    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * Local functions
 *****************************************************************************/

/******************************************************************************
 * vomCtrlHdmiHotPlugThread()
 *****************************************************************************/
static int32_t vomCtrlHdmiHotPlugThread
(
    void *p_arg
)
{
    TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s (enter)\n", __FUNCTION__);

    if ( p_arg == NULL )
    {
        TRACE(VOM_CTRL_HDMI_HPT_ERROR, "%s arg pointer is NULL\n", __FUNCTION__);
    }
    else
    {
        // get handle from arg
        vomCtrlHdmiHandle_t HdmiHandle = (vomCtrlHdmiHandle_t)p_arg;

        // get context
        vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

        bool_t bExit = BOOL_FALSE;
        bool_t bRunning = BOOL_FALSE;

        // processing loop
        do
        {
            // wait for next command
            OSLAYER_STATUS osStatus;
            HPThreadCmd_t Command = 0;

            if ( bRunning == BOOL_FALSE )
            {
                osStatus = osQueueRead(&pHdmiContext->HPThreadCmdQueue, &Command); // wait indefinitely for next command
            }
            else
            {
                osStatus = osQueueTimedRead(&pHdmiContext->HPThreadCmdQueue, &Command, 250);  // wait poll timeout for next command
            }

            switch (osStatus)
            {
                case OSLAYER_OK: // we've received a new command
                {
                    // process command
                    switch ( Command )
                    {
                        case HPT_CMD_PAUSE:
                        {
                            TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s -> disabling\n", __FUNCTION__);
                            bRunning = BOOL_FALSE;
                            break;
                        }

                        case HPT_CMD_START:
                        {
                            TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s -> enabling\n", __FUNCTION__);
                            bRunning = BOOL_TRUE;
                            break;
                        }

                        case HPT_CMD_STOP:
                        {
                            TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s -> exiting\n", __FUNCTION__);
                            bExit = BOOL_TRUE;
                            break;
                        }

                        default:
                        {
                            TRACE(VOM_CTRL_HDMI_HPT_ERROR, "%s -> illegal command %d\n", __FUNCTION__, Command);
                            break; // for now we simply go on receiving commands
                        }
                    }
                    break;
                }

                case OSLAYER_TIMEOUT : // it's time to poll...
                {
                    if (bRunning == BOOL_TRUE) // ...but only if we're not paused
                    {
                        TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s (begin: hot plug processing)\n", __FUNCTION__);

                        RESULT result = vomCtrlHdmiCheckHotPlugAndSetHW( HdmiHandle );
                        if (RET_SUCCESS != result)
                        {
                            TRACE(VOM_CTRL_HDMI_HPT_ERROR, "%s hot plug check failed, RESULT=%d\n", __FUNCTION__, result);
                        }

                        TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s (end: hot plug processing)\n", __FUNCTION__);
                    }
                    break;
                }

                default: // bad things have happened
                    TRACE(VOM_CTRL_HDMI_HPT_ERROR, "%s receiving command failed -> OSLAYER_RESULT=%d\n", __FUNCTION__, osStatus);
                    break; // for now we simply go on receiving commands
            }
        } while ( bExit == BOOL_FALSE );  /* !bExit */
    }

    TRACE(VOM_CTRL_HDMI_HPT_INFO, "%s (exit)\n", __FUNCTION__);

    return ( 0 );
}


/******************************************************************************
 * vomCtrlHdmiIsConfigValid()
 *****************************************************************************/
static RESULT vomCtrlHdmiIsConfigValid
(
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
)
{
    DCT_ASSERT(pVideoFormat != NULL);

    if (pVideoFormat->FormatID == CEA_861_VIDEOFORMAT_INVALID)
    {
        return RET_INVALID_PARM;
    }

    if (pVideoFormat->FormatID >= CEA_861_VIDEOFORMAT_CUSTOM_BASE)
    {
        return RET_OUTOFRANGE;
    }

    // valid configuration
    return RET_SUCCESS;
}


/******************************************************************************
 * Internal functions for HW accesses
 *****************************************************************************/


/******************************************************************************
 * vomCtrlHdmiStartHW()
 *****************************************************************************/
static RESULT vomCtrlHdmiStartHW
(
    vomCtrlHdmiHandle_t HdmiHandle
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    RESULT lRes;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(HdmiHandle != NULL);

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // get access lock
    osStatus = osMutexLock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    // power down HDMI
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x41, 0x50 );
    UPDATE_RESULT( result, lRes );

    // power up HDMI
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x41, 0x10 );
    UPDATE_RESULT( result, lRes );

    // release access lock
    osStatus = osMutexUnlock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlHdmiStopHW()
 *****************************************************************************/
static RESULT vomCtrlHdmiStopHW
(
    vomCtrlHdmiHandle_t HdmiHandle
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    RESULT lRes;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(HdmiHandle != NULL);

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // get access lock
    osStatus = osMutexLock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    // power down HDMI
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x41, 0x50 );
    UPDATE_RESULT( result, lRes );

    // release access lock
    osStatus = osMutexUnlock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlHdmiSetHW()
 *****************************************************************************/
static RESULT vomCtrlHdmiSetHW
(
    vomCtrlHdmiHandle_t                 HdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
)
{
    RESULT result;
    OSLAYER_STATUS osStatus;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(HdmiHandle != NULL);
    DCT_ASSERT(pVideoFormat != NULL);

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // get access lock
    osStatus = osMutexLock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    // do the real work
    result = vomCtrlHdmiSetHWcore( HdmiHandle, pVideoFormat, Enable3D, VideoFormat3D );

    // release access lock
    osStatus = osMutexUnlock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlHdmiSetHWcore()
 *****************************************************************************/
static RESULT vomCtrlHdmiSetHWcore
(
    vomCtrlHdmiHandle_t                 HdmiHandle,
    const Cea861VideoFormatDetails_t    *pVideoFormat,
    bool_t                              Enable3D,
    Hdmi3DVideoFormat_t                 VideoFormat3D
)
{
    RESULT result = RET_SUCCESS;
    RESULT lRes;
    uint8_t idx;
    char *pcText;
    bool_t eoText;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(HdmiHandle != NULL);
    DCT_ASSERT(pVideoFormat != NULL);

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // power dowm
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x41, 0x50 );
    UPDATE_RESULT( result, lRes );

    // power up
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x41, 0x10 );
    UPDATE_RESULT( result, lRes );

    // initial Settings for "Registers which must be set"
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x0A, 0x01 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0x98, 0x07 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0x9C, 0x38 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0x9D, 0x61 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0x9F, 0x70 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0xA2, 0x84 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0xA3, 0x84 );
    UPDATE_RESULT( result, lRes );

    lRes = HDMI_WRITE_REG( pHdmiContext, 0xBB, 0xFF );
    UPDATE_RESULT( result, lRes );

    // Video In: 16/20/24 bit YCC 4:2:2 (Input-ID1), separate sync.
    uint32_t FrameRate = (!pVideoFormat || !pVideoFormat->PixRep || !pVideoFormat->Htotal ||!pVideoFormat->Vtotal)
                       ? 60 : (pVideoFormat->PixClk / pVideoFormat->PixRep / pVideoFormat->Htotal / pVideoFormat->Vtotal);
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x15, 0x02 | ( ((FrameRate<=30) && (!Enable3D)) ? 1 : 0 ) );
    UPDATE_RESULT( result, lRes );

    // Video In: 8 bit per component, style 1, DDR rising edge, Video out: RGB 4:4:4
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x16, 0x30 );
    UPDATE_RESULT( result, lRes );

    // InfoFrame: out RGB, no format info, no bar info
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x45, 0x00 );
    UPDATE_RESULT( result, lRes );

    // InfoFrame: no scan, no colorimetry, no aspect, no non-uniform scaling
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x46, 0x00 );
    UPDATE_RESULT( result, lRes );

    // InfoFrames: (no SPDIF input), no N_CTS packet, no audio sample packet, but AVI frame, no audio info frame, no GMP packet
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x44, 0x10 );
    UPDATE_RESULT( result, lRes );

    // no HDCP, no frame encrypt, DVI (HDMI=+0x2)
    lRes = HDMI_WRITE_REG( pHdmiContext, 0xaf, 0x04 | (Enable3D?0x02:0x00) ); // need HDMI to be able to xmit packets (required e.g. for GMP 3D stuff)
    UPDATE_RESULT( result, lRes );

    // video clk edge rising(=0x60; falling=0xe0)
    lRes = HDMI_WRITE_REG( pHdmiContext, 0xba, 0x60);
    UPDATE_RESULT( result, lRes );

    // enable color space conversion
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x3B, 0x81 );
    UPDATE_RESULT( result, lRes );

    // define coefficient range to -4..+4; 4:2:2->4:4:4 upconversion using
    // linear interpolation; 16:9 aspect ratio (=+0x2; 4:3=+0x0)
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x17, 0x10 );////0x14 | 0x2 );
    UPDATE_RESULT( result, lRes );

    // set Secondary Device Address (required for GMP register access)
    lRes = HDMI_WRITE_REG( pHdmiContext, 0xCF, pHdmiContext->I2cSlaveAddr2<<1 );
    UPDATE_RESULT( result, lRes );

    // GMP packet abused for HDMI 3D descriptor
    uint8_t abGMP_3D[] = {
        0x81,               // packet type
        0x01,               // version
        0x05, ////###DAV:+(Hdmi3DHasExtData(VideoFormat3D)?1:0), // length
        0x03, 0x0c, 0x00,   // HDMI org IEEE identifier
        0x40,               // HDMI video format: 3D info present
        0x00, ////###DAV:VideoFormat3D,      // 3D video format type
        0x00                // the 3D extended data; fixed for now
    };
    DCT_ASSERT(sizeof(abGMP_3D) < 31);
    for (idx=0; idx<sizeof(abGMP_3D); idx++)
    {
        lRes = HDMI_WRITE_REG2( pHdmiContext, idx, abGMP_3D[idx] );
        UPDATE_RESULT( result, lRes );
    }

    // set outgoing video ID code
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x3C, pVideoFormat->FormatIDInfoFrame );
    UPDATE_RESULT( result, lRes );

    // InfoFrames: (no SPDIF input), no N_CTS packet, no audio sample packet, but AVI frame, no audio info frame, GMP packet only if 3D
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x44, 0x10 | (Enable3D?0x02:0x00) );
    UPDATE_RESULT( result, lRes );

    // Source Product Descriptor
    pcText = "DCT/SIMG"; // Vendor Name (max 8 chars)
    for (eoText = false, idx=0; idx<8; idx++)
    {
        char c;
        if (!eoText)
        {
            c = pcText[idx];
            eoText = (c==0);
        }
        lRes = HDMI_WRITE_REG( pHdmiContext, 0x52+idx, eoText ? ' ' : c );
        UPDATE_RESULT( result, lRes );
    }
    pcText = "CamerIC 3D Demo"; // Product Name (max 16 chars)
    for (idx=0; idx<16; idx++)
    {
        char c;
        if (!eoText)
        {
            c = pcText[idx];
            eoText = (c==0);
        }
        lRes = HDMI_WRITE_REG( pHdmiContext, 0x5a+idx, eoText ? ' ' : c );
        UPDATE_RESULT( result, lRes );
    }
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x6a+idx, 0x00 ); // Source Device Information Code (0x00 = 'unknown')
    UPDATE_RESULT( result, lRes );

    // Info Pakets: no General Control, but Source Product Descriptor, no MPEG, no ACP, no ISRC packets
    lRes = HDMI_WRITE_REG( pHdmiContext, 0x40, 0x40 );
    UPDATE_RESULT( result, lRes );

    // remember new settings
    pHdmiContext->VideoFormat   = *pVideoFormat;
    pHdmiContext->Enable3D      = Enable3D;
    pHdmiContext->VideoFormat3D = VideoFormat3D;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


/******************************************************************************
 * vomCtrlHdmiCheckHotPlugAndSetHW()
 *****************************************************************************/
static RESULT vomCtrlHdmiCheckHotPlugAndSetHW
(
    vomCtrlHdmiHandle_t HdmiHandle
)
{
    RESULT result = RET_SUCCESS;
    OSLAYER_STATUS osStatus;
    RESULT lRes = RET_SUCCESS;
    bool_t DoSetHW = BOOL_FALSE;
    uint8_t data = 0;

    TRACE(VOM_CTRL_HDMI_INFO, "%s (enter)\n", __FUNCTION__);

    DCT_ASSERT(HdmiHandle != NULL);

    // get context
    vomCtrlHdmiContext_t *pHdmiContext = (vomCtrlHdmiContext_t*)(HdmiHandle);

    // get access lock
    osStatus = osMutexLock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    // check if HDMI is in power down state
    lRes = HDMI_READ_REG( pHdmiContext, 0x41, &data );
    if (RET_SUCCESS == lRes)
    {
        if ((data & 0x40) == 0x40)
        {
            TRACE(VOM_CTRL_HDMI_HPT_DEBUG, "%s HDMI Tx is off\n", __FUNCTION__);

            // check if HPD is asserted and line termination is detected
            lRes = HDMI_READ_REG( pHdmiContext, 0x42, &data );
            if (RET_SUCCESS == lRes)
            {
                DoSetHW = ((data & 0x60) == 0x60) ? BOOL_TRUE : BOOL_FALSE;
                TRACE(VOM_CTRL_HDMI_HPT_DEBUG, "%s HDMI Tx start=%d (HPD=%d, RxDet=%d)\n", __FUNCTION__, DoSetHW?1:0, (data&0x40)?1:0, (data&0x20)?1:0);
            }
            else
            {
                TRACE(VOM_CTRL_HDMI_HPT_ERROR, "%s error accessing i2c; result=%d\n", __FUNCTION__, lRes);
                UPDATE_RESULT( result, lRes );
            }
        }
    }
    else
    {
        TRACE(VOM_CTRL_HDMI_HPT_ERROR, "%s error accessing i2c; result=%d\n", __FUNCTION__, lRes);
        UPDATE_RESULT( result, lRes );
    }

    // do we need to setup HW?
    if (DoSetHW)
    {
        TRACE(VOM_CTRL_HDMI_HPT_DEBUG, "%s HDMI Tx restart\n", __FUNCTION__);
        lRes = vomCtrlHdmiSetHWcore( HdmiHandle, &pHdmiContext->VideoFormat, pHdmiContext->Enable3D, pHdmiContext->VideoFormat3D );
        UPDATE_RESULT( result, lRes );
    }

    // release access lock
    osStatus = osMutexUnlock( &pHdmiContext->HwAccessLock );
    DCT_ASSERT(OSLAYER_OK == osStatus);

    TRACE(VOM_CTRL_HDMI_INFO, "%s (exit)\n", __FUNCTION__);

    return result;
}


