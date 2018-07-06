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
 * @file camdevice.cpp
 *
 * @brief
 *   Implementation of Cam Device C++ API.
 *
 *****************************************************************************/
#include "cam_api/camdevice.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include "cam_api/halholder.h"
#include "cam_api/mapcaps.h"
#include "cam_api/dom_ctrl_interface.h"
#include "cam_api/vom_ctrl_interface.h"
#include "cam_api/som_ctrl_interface.h"
#include "cam_api/exa_ctrl_interface.h"

#include <list>
#include <functional>
#include <algorithm>
#include <utils/Log.h>


/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(CAM_API_CAMDEVICE_INFO , "CAM_API_CAMDEVICE: ", INFO,  0);
CREATE_TRACER(CAM_API_CAMDEVICE_ERROR, "CAM_API_CAMDEVICE: ", ERROR, 1);

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/

struct BufferCbContext
{
    std::list<BufferCb *> mainPath;
    std::list<BufferCb *> selfPath;
};


static void bufferCb( CamEnginePathType_t path, MediaBuffer_t* pMediaBuffer, void* pBufferCbCtx )
{
    BufferCbContext *pCtx = (BufferCbContext *)pBufferCbCtx;

    TRACE( CAM_API_CAMDEVICE_INFO, "%s (enter)\n", __FUNCTION__ );

    if ( NULL == pMediaBuffer->pOwner )
    {
        return;
    }

    if ( NULL != pCtx )
    {
        if ( CAM_ENGINE_PATH_SELF == path )
        {
            std::for_each( pCtx->selfPath.begin(), pCtx->selfPath.end(),
                    std::bind2nd( std::mem_fun( &BufferCb::bufferCb ), pMediaBuffer ) );
        }
        else if ( CAM_ENGINE_PATH_MAIN == path )
        {
            std::for_each( pCtx->mainPath.begin(), pCtx->mainPath.end(),
                    std::bind2nd( std::mem_fun( &BufferCb::bufferCb ), pMediaBuffer ) );
        }
    }

    TRACE( CAM_API_CAMDEVICE_INFO, "%s (exit)\n", __FUNCTION__ );
}

/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * CamDevice
 *****************************************************************************/
CamDevice::CamDevice( HalHandle_t hHal, AfpsResChangeCb_t *pcbResChange, void *ctxCbResChange ,void* hParent, int mipiLaneNum)
  : CamEngineItf  ( hHal, pcbResChange, ctxCbResChange, mipiLaneNum ),
    m_enableDom   ( false ),
    m_enableVom   ( false ),
    m_enableExa   ( false ),
    m_domCtrl     ( NULL ),
    m_vomCtrl     ( NULL ),
    m_somCtrl     ( NULL ),
    m_exaCtrl     ( NULL ),
    m_bufferCbCtx ( NULL ),
    m_sampleCb    ( NULL ),
    m_sampleCbCtx ( NULL ),
    m_sampleSkip  ( 0 )
{
    // create objects
//    m_domCtrl   = new DomCtrlItf( HalHolder::handle(), hParent );
//    DCT_ASSERT( NULL != m_domCtrl );
 //   m_vomCtrl   = new VomCtrlItf( HalHolder::handle() );
 //   DCT_ASSERT( NULL != m_vomCtrl );
//    m_somCtrl   = new SomCtrlItf( HalHolder::handle() );
//    DCT_ASSERT( NULL != m_somCtrl );
//    m_exaCtrl   = new ExaCtrlItf( HalHolder::handle() );
//    DCT_ASSERT( NULL != m_exaCtrl );
    m_bufferCbCtx = new BufferCbContext();
    DCT_ASSERT( NULL != m_bufferCbCtx );
    // register buffer callback
    bool ok = CamEngineItf::registerBufferCb( bufferCb, (void *)m_bufferCbCtx );
    DCT_ASSERT( true == ok );
    
}


/******************************************************************************
 * ~CamEngineItf
 *****************************************************************************/
CamDevice::~CamDevice()
{
    disconnectCamera();

    if ( NULL != m_domCtrl )
    {
        delete m_domCtrl;
    }
    if ( NULL != m_vomCtrl )
    {
        delete m_vomCtrl;
    }
    if ( NULL != m_somCtrl )
    {
        delete m_somCtrl;
    }
    if ( NULL != m_exaCtrl )
    {
        delete m_exaCtrl;
    }
    if ( NULL != m_bufferCbCtx )
    {
        delete m_bufferCbCtx;
    }
}


/******************************************************************************
 * enableDisplayOutput
 *****************************************************************************/
void
CamDevice::enableDisplayOutput( bool enable )
{
    if ( true == enable )
    {
        if ( CamEngineItf::Running == CamEngineItf::state() )
        {
            m_domCtrl->start();
        }
    }
    else
    {
        m_domCtrl->stop();
    }

    m_enableDom = enable;
}


/******************************************************************************
 * enableVideoOutput
 *****************************************************************************/
void
CamDevice::enableVideoOutput( bool enable )
{
    if ( true == enable )
    {
        if ( CamEngineItf::Running == CamEngineItf::state() )
        {
            m_vomCtrl->start( is3DEnabled() );
        }
    }
    else
    {
        m_vomCtrl->stop();
    }

    m_enableVom = enable;
}


/******************************************************************************
 * enableExternalAlgorithm
 *****************************************************************************/
void
CamDevice::enableExternalAlgorithm( bool enable )
{
    if ( true == enable )
    {
        if ( ( CamEngineItf::Running == CamEngineItf::state() )
                && ( NULL != m_sampleCb ) )
        {
            m_exaCtrl->start( m_sampleCb, m_sampleCbCtx, m_sampleSkip );
        }
    }
    else
    {
        m_exaCtrl->stop();
    }

    m_enableExa = enable;
}


/******************************************************************************
 * connectCamera
 *****************************************************************************/
bool
CamDevice::connectCamera( bool preview, BufferCb *bufferCb )
{
    if ( CamEngineItf::Invalid != CamEngineItf::state() )
    {
        CamEngineItf::disconnect();
    }

#if 0 //modified by zyc ,move to camerahal
    // configure for preview
    if ( true == preview )
    {
        if ( CamEngineItf::Sensor == CamEngineItf::configType() )
        {
            if ( true != CamEngineItf::previewSensor() )
            {
                TRACE( CAM_API_CAMDEVICE_ERROR, "%s:%d (ERRO)\n", __FUNCTION__ ,__LINE__);
                return ( false );
            }
        }

        if ( true != CamEngineItf::previewSetup() )
        {
            TRACE( CAM_API_CAMDEVICE_ERROR, "%s:%d (ERRO)\n", __FUNCTION__ ,__LINE__);
            return ( false );
        }
    }
#endif
    CamEngineModeType_t mode; 
    CamEngineItf::getScenarioMode( mode );
    switch ( mode )
    {
        case CAM_ENGINE_MODE_SENSOR_2D:
        case CAM_ENGINE_MODE_IMAGE_PROCESSING:
        {
            if(m_domCtrl != NULL)
                m_bufferCbCtx->selfPath.push_back( m_domCtrl );
           // m_bufferCbCtx->selfPath.push_back( m_vomCtrl );
            if(m_exaCtrl != NULL)
                m_bufferCbCtx->selfPath.push_back( m_exaCtrl );

            if ( NULL != bufferCb )
            {
                m_bufferCbCtx->selfPath.push_back( bufferCb );
                //for test zyc
                m_bufferCbCtx->mainPath.push_back( bufferCb );
            }

            break;
        }

        case CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB:
        case CAM_ENGINE_MODE_SENSOR_3D: 
        {
            m_bufferCbCtx->mainPath.push_back( m_domCtrl );
         //   m_bufferCbCtx->mainPath.push_back( m_vomCtrl );

            if ( NULL != bufferCb )
            {
                m_bufferCbCtx->mainPath.push_back( bufferCb );
            }

            break;
        }

        default:
        {
            return ( false );
        }
    }

    return CamEngineItf::connect();
}


/******************************************************************************
 * disconnectCamera
 *****************************************************************************/
void
CamDevice::disconnectCamera()
{
    if(m_domCtrl)
        m_domCtrl->stop();
    //m_vomCtrl->stop();
    if(m_exaCtrl)
        m_exaCtrl->stop();
    CamEngineItf::disconnect();

    m_bufferCbCtx->mainPath.clear();
    m_bufferCbCtx->selfPath.clear();
}


/******************************************************************************
 * resetCamera
 *****************************************************************************/
void
CamDevice::resetCamera()
{
    if(m_domCtrl)
        m_domCtrl->stop();
    //m_vomCtrl->stop();
    if(m_exaCtrl)
        m_exaCtrl->stop();
    CamEngineItf::reset();

    m_bufferCbCtx->mainPath.clear();
    m_bufferCbCtx->selfPath.clear();
}


/******************************************************************************
 * startPreview
 *****************************************************************************/
bool
CamDevice::startPreview()
{
    TRACE( CAM_API_CAMDEVICE_INFO, "%s (ENTER)\n", __FUNCTION__ );
    if ( ( true == m_enableExa ) && 
         ( NULL != m_sampleCb )  )
    {
        m_exaCtrl->start( m_sampleCb, m_sampleCbCtx, m_sampleSkip );
    }

    // show preview widget
    if ( true == m_enableDom )
    {
        m_domCtrl->start();
    }

    if ( true == m_enableVom )
    {
         Cea861VideoFormat_t format =
            ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == configMode() )
                ? CEA_861_VIDEOFORMAT_1280x720p60 : CEA_861_VIDEOFORMAT_1920x1080p24;
        
        m_vomCtrl->start( is3DEnabled(), format );
    }
    TRACE( CAM_API_CAMDEVICE_INFO, "%s (exit)\n", __FUNCTION__ );

    return CamEngineItf::start();
}


/******************************************************************************
 * pausePreview
 *****************************************************************************/
bool
CamDevice::pausePreview()
{
    return CamEngineItf::pause();
}


/******************************************************************************
 * stopPreview
 *****************************************************************************/
bool
CamDevice::stopPreview()
{
    if(m_domCtrl)
        m_domCtrl->stop();
    //m_vomCtrl->stop();
    if(m_exaCtrl)
        m_exaCtrl->stop();
    return CamEngineItf::stop();
}


bool CamDevice::setSensorResConfig(uint32_t mask)
{
    IsiSensorConfig_t sensorConfig;

    MEMSET( &sensorConfig, 0, sizeof( IsiSensorConfig_t ) );
    CamEngineItf::getSensorConfig( sensorConfig );
    if ( sensorConfig.Resolution != mask )
    {
        sensorConfig.Resolution = mask;
        CamEngineItf::setSensorConfig( sensorConfig );
    }
    return true;
        
}
/******************************************************************************
 * captureSnapshot
 *****************************************************************************/
bool
CamDevice::captureSnapshot( const char* fileName, int type, uint32_t resolution, CamEngineLockType_t locks )
{
    if ( CamEngineItf::Running != CamEngineItf::state() )
    {
        return false;
    }

    int width  = 0;
    int height = 0;

    int curWidth  = 0;
    int curHeight = 0;
    bool resolutionChange = false;
    bool exif = false;

    CamEngineItf::getResolution( curWidth, curHeight );

    if ( 0 != resolution )
    {
        CamEngineWindow_t window = { 0, 0, 0, 0 };
        if ( true == isiCapValue( window, resolution ) )
        {
            width = window.width;
            height = window.height;

            if ( ( width != curWidth ) || ( height != curHeight ) )
            {
                resolutionChange = true;
            }
        }
        else
        {
            return false;
        }
    }
    else//DMA mode
    {
        width = curWidth;
        height = curHeight;
    }

    // current settings
    CamEnginePathConfig_t curMpConfigMaster;
    CamEnginePathConfig_t curSpConfigMaster;
    CamEnginePathConfig_t curMpConfigSlave;
    CamEnginePathConfig_t curSpConfigSlave;
    IsiSensorConfig_t     curSensorConfig;
    CamEngineItf::getSensorConfig( curSensorConfig );// curSensorConfig.Resolution == 0 if DMA mode

    if ( ( true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_MAIN, curMpConfigMaster ) ) ||
         ( true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_SELF, curSpConfigMaster ) ) ||
         ( true != CamEngineItf::getPathConfig( CHAIN_SLAVE, CAM_ENGINE_PATH_MAIN, curMpConfigSlave ) ) ||
         ( true != CamEngineItf::getPathConfig( CHAIN_SLAVE, CAM_ENGINE_PATH_SELF, curSpConfigSlave ) ) )
    {
        return false;
    }

    CamerIcMiDataLayout_t mpLayout  = CAMERIC_MI_DATASTORAGE_INVALID;
    CamerIcMiDataMode_t   mpMode    = CAMERIC_MI_DATAMODE_INVALID;

    CamEngineItf::SnapshotType snapshotType = (CamEngineItf::SnapshotType) type;
    switch( snapshotType )
    {
        case CamEngineItf::RGB:
        {
            mpMode   = CAMERIC_MI_DATAMODE_YUV422;
            mpLayout = CAMERIC_MI_DATASTORAGE_SEMIPLANAR;
            break;
        }
        case CamEngineItf::RAW8:
        {
            mpMode   = CAMERIC_MI_DATAMODE_RAW8;
            mpLayout = CAMERIC_MI_DATASTORAGE_INTERLEAVED;
            break;
        }
        case CamEngineItf::RAW12:
        {
            mpMode   = CAMERIC_MI_DATAMODE_RAW12;
            mpLayout = CAMERIC_MI_DATASTORAGE_INTERLEAVED;
            break;
        }
        case CamEngineItf::JPEG:
        {
            mpMode   = CAMERIC_MI_DATAMODE_JPEG;
            mpLayout = CAMERIC_MI_DATASTORAGE_INTERLEAVED;
            exif = true;

            CamEngineItf::enableJpe( width, height );
            break;
        }
        case CamEngineItf::JPEG_JFIF:
        {
            mpMode   = CAMERIC_MI_DATAMODE_JPEG;
            mpLayout = CAMERIC_MI_DATASTORAGE_INTERLEAVED;

            CamEngineItf::enableJpe( width, height );
            break;
        }
        case CamEngineItf::DPCC:
        {
            mpMode   = CAMERIC_MI_DATAMODE_DPCC;
            mpLayout = CAMERIC_MI_DATASTORAGE_INTERLEAVED;
            break;
        }
        default:
        {
            return false;
        }
    }

    // configure for capture
    CamEnginePathConfig_t mpConfig;
    CamEnginePathConfig_t spConfig;
    MEMSET( &mpConfig, 0, sizeof( CamEnginePathConfig_t ) );
    MEMSET( &spConfig, 0, sizeof( CamEnginePathConfig_t ) );

    mpConfig.mode   = mpMode;
    mpConfig.layout = mpLayout;
    mpConfig.width  = 0;//width;
    mpConfig.height = 0;//height;
    spConfig.mode   = CAMERIC_MI_DATAMODE_DISABLED;
    spConfig.layout = CAMERIC_MI_DATASTORAGE_PLANAR;

    if ( true != CamEngineItf::isTestpatternEnabled() )
    {
        CamEngineItf::searchAndLock( locks );
    }

    if ( true == resolutionChange )
    {
        CamEngineItf::changeResolution( resolution );
    }
    else
    {
        CamEngineItf::stop();
    }

    // configure
    if ( true != CamEngineItf::setPathConfig( CHAIN_MASTER, mpConfig, spConfig ) )
    {
        CamEngineItf::unlock( locks );
        if ( true == resolutionChange )
        {
            CamEngineItf::changeResolution( curSensorConfig.Resolution );
        }
        CamEngineItf::start();
        return false;
    }

    if ( true == CamEngineItf::is3DEnabled() )
    {
        if ( true != CamEngineItf::setPathConfig( CHAIN_SLAVE, mpConfig, spConfig ) )
        {
            CamEngineItf::unlock( locks );
            if ( true == resolutionChange )
            {
                CamEngineItf::changeResolution( curSensorConfig.Resolution );
            }
            CamEngineItf::start();
            return false;
        }
    }

    // remove callbacks, add som ctrl callback
    std::list<BufferCb *> curMainPath = m_bufferCbCtx->mainPath;
    m_bufferCbCtx->mainPath.clear();
    std::list<BufferCb *> curSelfPath = m_bufferCbCtx->selfPath;
    m_bufferCbCtx->selfPath.clear();

    m_bufferCbCtx->mainPath.push_back( m_somCtrl );

    // capture one image, skip none (first broken images are skipped internally by cam-engine now)
    m_somCtrl->start( fileName, 1, 2, exif, false );
    CamEngineItf::start();

    // wait
    m_somCtrl->waitForFinished();

    m_somCtrl->stop();
    CamEngineItf::unlock( locks );
    if ( true == resolutionChange )
    {
        CamEngineItf::changeResolution( curSensorConfig.Resolution );
    }
    else
    {
        CamEngineItf::stop();
    }

    // restore callbacks
    m_bufferCbCtx->mainPath = curMainPath;
    m_bufferCbCtx->selfPath = curSelfPath;

    // restart preview
    if ( ( CamEngineItf::JPEG      == snapshotType ) ||
         ( CamEngineItf::JPEG_JFIF == snapshotType ) )
    {
        CamEngineItf::disableJpe();
    }

    if ( true == CamEngineItf::setPathConfig( CHAIN_MASTER, curMpConfigMaster, curSpConfigMaster ) )
    {
    	if ( true == CamEngineItf::is3DEnabled() )
        {
        	CamEngineItf::setPathConfig( CHAIN_SLAVE, curMpConfigSlave, curSpConfigSlave );
        }
        CamEngineItf::start();
    }
    else
    {
        CamEngineItf::stop();
    }

    return true;
}


/******************************************************************************
 * registerExternalAlgorithmCallback
 *****************************************************************************/
void
CamDevice::registerExternalAlgorithmCallback( exaCtrlSampleCb_t sampleCb, void *pSampleContext, uint8_t sampleSkip )
{
    m_sampleCb    = sampleCb;
    m_sampleCbCtx = pSampleContext;
    m_sampleSkip  = sampleSkip;
}

