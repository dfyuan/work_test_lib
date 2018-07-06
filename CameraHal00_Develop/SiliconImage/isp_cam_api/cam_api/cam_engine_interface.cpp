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
 * @file cam_engine_interface.cpp
 *
 * @brief
 *   Implementation of Cam Engine C++ API.
 *
 *****************************************************************************/
#include "cam_api/cam_engine_interface.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <version/version.h>

#include <hal/hal_api.h>
#include <isi/isi_iss.h>

#include <cameric_reg_drv/cameric_reg_description.h>

#include <dlfcn.h>
#include "cam_calibdb.h"
#include "cam_calibdb_api.h"
#include "cam_api/mapcaps.h"
#include "cam_api/mapbuffer.h"

#include "calib_xml/calibdb.h"

#include <string>
#include <limits>

#include <fcntl.h>
#include <utils/threads.h>

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(CAM_API_CAMENGINE_INFO , "CAM_API_CAMENGINE: ", INFO,    1);
CREATE_TRACER(CAM_API_CAMENGINE_WARN , "CAM_API_CAMENGINE: ", WARNING, 1);
CREATE_TRACER(CAM_API_CAMENGINE_ERROR, "CAM_API_CAMENGINE: ", ERROR,   1);
CREATE_TRACER(CAM_API_CAMENGINE_NOTICE0, "", TRACE_NOTICE0,    1);
CREATE_TRACER(CAM_API_CAMENGINE_NOTICE1, "CAM_API_CAMENGINE: ", TRACE_NOTICE1,    1);
CREATE_TRACER(CAM_API_CAMENGINE_DEBUG, "CAM_API_CAMENGINE: ", TRACE_DEBUG,    1);

#define CAM_API_CAMENGINE_TIMEOUT 30000U // 30 s


using android::Thread;

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/

static const CamEngineWbGains_t defaultWbGains = { 1.0f, 1.0f, 1.0f, 1.0f };

static const CamEngineCcMatrix_t defaultCcMatrix = { {  1.0f, 0.0f, 0.0f,
                                                        0.0f, 1.0f, 0.0f,
                                                        0.0f, 0.0f, 1.0f } };

static const CamEngineCcOffset_t defaultCcOffset = { 0, 0, 0 };

static const CamEngineBlackLevel_t defaultBlvl = { 0U, 0U, 0U, 0U };

static const CamEnginePathConfig_t defaultPathDisabled =
{
    0U,                                     // width
    0U,                                     // height
    CAMERIC_MI_DATAMODE_DISABLED,           // mode
    CAMERIC_MI_DATASTORAGE_SEMIPLANAR,      // layout
    BOOL_FALSE,                             // dcEnable
    { 0U, 0U, 0U, 0U }                      // dcWin
};

//for test ,zyc
static const CamEnginePathConfig_t defaultPathPreview =
{
    1920U,//3264U,//1920U,  //1280U,                                // width
    1080U,//2448U,//1080U,  // 720U,                               // height
    CAMERIC_MI_DATAMODE_YUV422,//CAMERIC_MI_DATAMODE_YUV422,             // mode
    CAMERIC_MI_DATASTORAGE_INTERLEAVED,//CAMERIC_MI_DATASTORAGE_SEMIPLANAR,      // layout
    BOOL_FALSE,                             // dcEnable
    { 0U, 0U, /*1280U*/1920U/*3264*/, /*720U*/1080/*2448*/ }                // dcWin 
};

static const CamEnginePathConfig_t defaultPathPreviewImgStab =
{
    1920U,//3264U,//1920U, //1280U,                                 // width
    1080U,//2448U,//1080U, //720U,                                  // height
    CAMERIC_MI_DATAMODE_YUV422,             // mode
    CAMERIC_MI_DATASTORAGE_SEMIPLANAR,      // layout
    BOOL_FALSE,                             // dcEnable
    { 0U, 0U, 1920U/*1280U*//*3264*/, 1080U/*720U*//*2448*/ }                 // dcWin 
};


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * CamEngineItf::SensorHolder
 *****************************************************************************/
class CamEngineItf::SensorHolder
{
public:
    SensorHolder( HalHandle_t hal );
    ~SensorHolder();

private:
    SensorHolder (const SensorHolder& other);
    SensorHolder& operator = (const SensorHolder& other);

public:
    HalHandle_t                 hHal;
    void*                       hSensorLib;
    IsiCamDrvConfig_t           *pCamDrvConfig;
    IsiSensorHandle_t           hSensor;
    IsiSensorHandle_t           hSubSensor;
    IsiSensorConfig_t           sensorConfig;

    PicBufMetaData_t            image;
    CamEngineWbGains_t          wbGains;
    CamEngineCcMatrix_t         ccMatrix;
    CamEngineCcOffset_t         ccOffset;
    CamEngineBlackLevel_t       blvl;
    float                       vGain;
    float                       vItime;

    bool                        enableTestpattern;

    uint32_t                    minFocus;
    uint32_t                    maxFocus;

    int                         sensorItf;
    CamEngineFlickerPeriod_t    flickerPeriod;
    bool                        enableAfps;
};


/******************************************************************************
 * SensorHolder
 *****************************************************************************/
CamEngineItf::SensorHolder::SensorHolder( HalHandle_t hal )
    : hHal              ( hal  ),
      hSensorLib        ( NULL ),
      pCamDrvConfig     ( NULL ),
      hSensor           ( NULL ),
      hSubSensor        ( NULL ),
      enableTestpattern (false),
      minFocus          ( 0 ),
      maxFocus          ( 0 ),
      sensorItf         ( 0 ),
      flickerPeriod     ( CAM_ENGINE_FLICKER_100HZ ),
      enableAfps        (false)
{
    MEMSET( &sensorConfig, 0, sizeof( IsiSensorConfig_t ) );

    MEMSET( &image, 0, sizeof( PicBufMetaData_t ) );
    MEMSET( &wbGains, 0, sizeof( CamEngineWbGains_t ) );
    MEMSET( &ccMatrix, 0, sizeof( CamEngineCcMatrix_t ) );
    MEMSET( &ccOffset, 0, sizeof( CamEngineCcOffset_t ) );
    MEMSET( &blvl, 0, sizeof( CamEngineBlackLevel_t ) );
    vGain  = 0;
    vItime = 0;


    // reference marvin software HAL
    if ( RET_SUCCESS != HalAddRef( hHal ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't reference HAL)\n", __FUNCTION__ );
    }

}



/******************************************************************************
 * ~SensorHolder
 *****************************************************************************/
CamEngineItf::SensorHolder::~SensorHolder()
{
    // dereference marvin software HAL
    if ( RET_SUCCESS != HalDelRef( hHal ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't dereference HAL)\n", __FUNCTION__ );
    }

}


/******************************************************************************
 * CamEngineItf::CamEngineHolder
 *****************************************************************************/
class CamEngineItf::CamEngineHolder
{
public:
    CamEngineHolder( HalHandle_t hal, CamEngineItf *camEngineItf, bool isSystem3D = false,int mipiLaneNum=0 );
    ~CamEngineHolder();

private:
    CamEngineHolder (const CamEngineHolder& other);
    CamEngineHolder& operator = (const CamEngineHolder& other);

public:
    static void cbCompletion( CamEngineCmdId_t cmdId, RESULT result, const void* pUserCtx );
    static void cbAfpsResChange( uint32_t NewRes, const void* pUserCtx );
    class AfpsResChangeThread;
    android::sp<AfpsResChangeThread> pAfpsResChangeThread;

public:
    CamEngineItf        *pCamEngineItf;
    CamEngineHandle_t   hCamEngine;
    CamEngineConfig_t   camEngineConfig;
    CamEngineItf::State               state;

    std::string         sensorDrvFile;
    std::string         calibDbFile;
    CalibDb             calibDb;

    osEvent             m_eventStart;
    osEvent             m_eventStop;
    osEvent             m_eventStartStreaming;
    osEvent             m_eventStopStreaming;
    osEvent             m_eventAcquireLock;
    osEvent             m_eventReleaseLock;
    osMutex             m_syncLock;
    osQueue             m_queueResChange;
};


/******************************************************************************
 * CamEngineItf::CamEngineHolder::AfpsResChangeThread
 *****************************************************************************/
class CamEngineItf::CamEngineHolder::AfpsResChangeThread : public android::Thread
{
public:
    AfpsResChangeThread(CamEngineHolder *camEngineHolder)
    {
        DCT_ASSERT(camEngineHolder);
        pCamEngineHolder = camEngineHolder;
    };
    virtual ~AfpsResChangeThread(){};

private:
    virtual bool threadLoop();
    CamEngineHolder *pCamEngineHolder;
};

/******************************************************************************
 * AfpsResChangeThread
 *****************************************************************************/
bool CamEngineItf::CamEngineHolder::AfpsResChangeThread::threadLoop()
{
    TRACE( CAM_API_CAMENGINE_INFO, "%s (started)\n", "AfpsResChangeThread" );

    uint32_t NewRes = 0;
    while( OSLAYER_OK == osQueueRead( &pCamEngineHolder->m_queueResChange, &NewRes ) )
    {
        if( NewRes == 0 )
        {
            TRACE( CAM_API_CAMENGINE_DEBUG, "%s (stopping)\n", "AfpsResChangeThread" );
            break;
        }
        else
        {
            if (pCamEngineHolder->pCamEngineItf->m_pCamEngine->state == CamEngineItf::Running)
            {
                char *szNewResName = NULL;
                RESULT result = IsiGetResolutionName( NewRes, &szNewResName );

                TRACE( CAM_API_CAMENGINE_NOTICE1, "%s (AFPS resolution change -> %s)\n", "AfpsResChangeThread", szNewResName );
                if ( !pCamEngineHolder->pCamEngineItf->changeResolution( NewRes, true ) )
                {
                    TRACE( CAM_API_CAMENGINE_ERROR, "%s (AFPS resolution change failed!)\n", "AfpsResChangeThread" );
                }
            }
            else
            {
                TRACE( CAM_API_CAMENGINE_WARN, "%s (cam engine not running)\n", "AfpsResChangeThread" );
            }
        }
    }

    return false;
    TRACE( CAM_API_CAMENGINE_INFO, "%s (stopped)\n", "AfpsResChangeThread" );
}


/******************************************************************************
 * CamEngineHolder
 *****************************************************************************/
CamEngineItf::CamEngineHolder::CamEngineHolder( HalHandle_t hal, CamEngineItf *camEngineItf, bool isSystem3D, int mipiLaneNum )
    : hCamEngine    (NULL),
      state         (Invalid)
{
    bool ok = true;

    DCT_ASSERT(camEngineItf);
    pCamEngineItf = camEngineItf;

    // default isp configuration
    MEMSET( &camEngineConfig, 0, sizeof( CamEngineConfig_t ) );
    
    camEngineConfig.mode = CAM_ENGINE_MODE_SENSOR_2D;
	camEngineConfig.mipiLaneNum = mipiLaneNum;
	camEngineConfig.mipiLaneFreq= 0;

    //for test zyc,use main path
    camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathPreview;
    camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] =  defaultPathDisabled;

   // camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
   // camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathPreview;
    
    camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
    camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;

    // init register description
    CamerIcRegDescriptionDrvConfig_t regDescriptionConfig;
    MEMSET( &regDescriptionConfig, 0, sizeof( CamerIcRegDescriptionDrvConfig_t ) );

    regDescriptionConfig.HalHandle = hal;

    if ( RET_SUCCESS != CamerIcInitRegDescriptionDrv( &regDescriptionConfig ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't initialize the register description, main)\n", __FUNCTION__ );
        ok = false;
    }

    // create and initialize the cam-engine instance
    CamEngineInstanceConfig_t camInstanceConfig;
    MEMSET( &camInstanceConfig, 0, sizeof( CamEngineInstanceConfig_t ) );

    camInstanceConfig.maxPendingCommands = 4;
    camInstanceConfig.cbCompletion       = cbCompletion;
    camInstanceConfig.cbAfpsResChange    = cbAfpsResChange;
    camInstanceConfig.pUserCbCtx         = (void *)this;
    camInstanceConfig.hHal               = hal;
    camInstanceConfig.hCamEngine         = NULL;
    camInstanceConfig.isSystem3D         = ( true == isSystem3D ) ? BOOL_TRUE : BOOL_FALSE;

    if ( ok && ( RET_SUCCESS != CamEngineInit( &camInstanceConfig ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create the cam-engine instance)\n", __FUNCTION__ );
        ok = false;
    }

    // create events
    if ( ok && ( OSLAYER_OK != osEventInit( &m_eventStart, 1, 0 ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create start event)\n", __FUNCTION__ );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &m_eventStop, 1, 0 ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create stop event)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &m_eventStartStreaming, 1, 0 ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create start streaming event)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop  );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &m_eventStopStreaming, 1, 0 ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create stop streaming event)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop );
        (void)osEventDestroy( &m_eventStartStreaming );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &m_eventAcquireLock, 1, 0 ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create acquire lock event)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop );
        (void)osEventDestroy( &m_eventStartStreaming );
        (void)osEventDestroy( &m_eventStopStreaming );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &m_eventReleaseLock, 1, 0 ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create release lock event)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop );
        (void)osEventDestroy( &m_eventStartStreaming );
        (void)osEventDestroy( &m_eventStopStreaming );
        (void)osEventDestroy( &m_eventAcquireLock );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osMutexInit( &m_syncLock ) ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create sync lock)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop );
        (void)osEventDestroy( &m_eventStartStreaming );
        (void)osEventDestroy( &m_eventStopStreaming );
        (void)osEventDestroy( &m_eventAcquireLock );
        (void)osEventDestroy( &m_eventReleaseLock );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osQueueInit( &m_queueResChange, 1, sizeof(uint32_t) ) ) ) // must have 1 (one) item only!
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create resolution change queue)\n", __FUNCTION__ );
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop );
        (void)osEventDestroy( &m_eventStartStreaming );
        (void)osEventDestroy( &m_eventStopStreaming );
        (void)osEventDestroy( &m_eventAcquireLock );
        (void)osEventDestroy( &m_eventReleaseLock );
        (void)osMutexDestroy( &m_syncLock );
        (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
        ok = false;
    }

    if ( ok )
    {
        pAfpsResChangeThread = new AfpsResChangeThread( this );
        if (pAfpsResChangeThread == NULL)
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't create resolution change thread)\n", __FUNCTION__ );
            (void)osEventDestroy( &m_eventStart );
            (void)osEventDestroy( &m_eventStop );
            (void)osEventDestroy( &m_eventStartStreaming );
            (void)osEventDestroy( &m_eventStopStreaming );
            (void)osEventDestroy( &m_eventAcquireLock );
            (void)osEventDestroy( &m_eventReleaseLock );
            (void)osMutexDestroy( &m_syncLock );
            (void)osQueueDestroy( &m_queueResChange );
            (void)CamEngineShutDown( camInstanceConfig.hCamEngine );
            ok = false;
        }
        else
        {
            pAfpsResChangeThread->run("AfpsResChangeThread");
        }
    }

    if ( true == ok )
    {
        hCamEngine = camInstanceConfig.hCamEngine;
    }
}


/******************************************************************************
 * ~CamEngineHolder
 *****************************************************************************/
CamEngineItf::CamEngineHolder::~CamEngineHolder()
{
    if( NULL != hCamEngine )
    {
#if 0
        QEventLoop loop;
        if ( true == QObject::connect( pAfpsResChangeThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            uint32_t dummy;
            uint32_t NewRes = 0; // 0 resembles stop request
            (void)osQueueTryRead( &m_queueResChange, &dummy );    // remove previous change request if not serviced so far
            (void)osQueueWrite( &m_queueResChange, &NewRes );     // send stop request

            loop.exec();                                          // wait for thread to finish
        }
        else
        {
            TRACE( CAM_API_CAMENGINE_WARN, "%s (can't connect to AfpsResChangeThread.finished(), trying alternative shutdown)\n", __FUNCTION__ );
            uint32_t dummy;
            uint32_t NewRes = 0; // 0 resembles stop request
            (void)osQueueTryRead( &m_queueResChange, &dummy );    // remove previous change request if not serviced so far
            (void)osQueueWrite( &m_queueResChange, &NewRes );     // send stop request

            (void)osQueueWrite( &m_queueResChange, &NewRes );     // wait for thread to have finished, by waiting for thread having read the previous stop request
        }
#else
        if(pAfpsResChangeThread != NULL){
                uint32_t dummy;
                uint32_t NewRes = 0; // 0 resembles stop request
                (void)osQueueTryRead( &m_queueResChange, &dummy );    // remove previous change request if not serviced so far
                (void)osQueueWrite( &m_queueResChange, &NewRes );     // send stop request
                pAfpsResChangeThread->join();

        }
#endif
        (void)osEventDestroy( &m_eventStart );
        (void)osEventDestroy( &m_eventStop );
        (void)osEventDestroy( &m_eventStartStreaming );
        (void)osEventDestroy( &m_eventStopStreaming );
        (void)osEventDestroy( &m_eventAcquireLock );
        (void)osEventDestroy( &m_eventReleaseLock );
        (void)osMutexDestroy( &m_syncLock );
        (void)osQueueDestroy( &m_queueResChange );
        pAfpsResChangeThread.clear();

        if ( RET_SUCCESS != CamEngineShutDown( hCamEngine ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't shutdown the cam-engine instance)\n", __FUNCTION__ );
        }
        //zyc add
        CamerIcReleaseRegDescriptionDrv();
    }
}


/******************************************************************************
 * CamEngineHolder::cbCompletion
 *****************************************************************************/
void CamEngineItf::CamEngineHolder::cbCompletion( CamEngineCmdId_t cmdId,
                                               RESULT result,
                                               const void* pUserCtx )
{
    CamEngineItf::CamEngineHolder *pCamEngineHolder = (CamEngineItf::CamEngineHolder *)pUserCtx;

    if ( pCamEngineHolder != NULL )
    {
        switch ( cmdId )
        {
            case CAM_ENGINE_CMD_START:
            {
                (void)osEventSignal( &pCamEngineHolder->m_eventStart );
                TRACE( CAM_API_CAMENGINE_INFO, "%s (CAM_ENGINE_CMD_START)\n", __FUNCTION__ );
                break;
            }

            case CAM_ENGINE_CMD_STOP:
            {
                (void)osEventSignal( &pCamEngineHolder->m_eventStop );
                TRACE( CAM_API_CAMENGINE_INFO, "%s (CAM_ENGINE_CMD_STOP)\n", __FUNCTION__ );
                break;
            }

            case CAM_ENGINE_CMD_START_STREAMING:
            {
                (void)osEventSignal( &pCamEngineHolder->m_eventStartStreaming );
                TRACE( CAM_API_CAMENGINE_INFO, "%s (CAM_ENGINE_CMD_START_STREAMING)\n", __FUNCTION__ );
                break;
            }

            case CAM_ENGINE_CMD_STOP_STREAMING:
            {
                (void)osEventSignal( &pCamEngineHolder->m_eventStopStreaming );
                TRACE( CAM_API_CAMENGINE_INFO, "%s (CAM_ENGINE_CMD_STOP_STREAMING)\n", __FUNCTION__ );
                break;
            }

            case CAM_ENGINE_CMD_ACQUIRE_LOCK:
            {
                (void)osEventSignal( &pCamEngineHolder->m_eventAcquireLock );
                TRACE( CAM_API_CAMENGINE_INFO, "%s (CAM_ENGINE_CMD_ACQUIRE_LOCK)\n", __FUNCTION__ );
                break;
            }

            case CAM_ENGINE_CMD_RELEASE_LOCK:
            {
                (void)osEventSignal( &pCamEngineHolder->m_eventReleaseLock );
                TRACE( CAM_API_CAMENGINE_INFO, "%s (CAM_ENGINE_CMD_RELEASE_LOCK)\n", __FUNCTION__ );
                break;
            }

            default:
            {
                break;
            }
        }
    }
}


/******************************************************************************
 * CamEngineHolder::cbAfpsResChange
 *****************************************************************************/
void CamEngineItf::CamEngineHolder::cbAfpsResChange( uint32_t NewRes,
                                                     const void* pUserCtx )
{
    CamEngineItf::CamEngineHolder *pCamEngineHolder = (CamEngineItf::CamEngineHolder *)pUserCtx;

    if ( pCamEngineHolder != NULL )
    {
        uint32_t dummy;
        TRACE( CAM_API_CAMENGINE_INFO, "%s (queueing AFPS_RES_CHANGE)\n", __FUNCTION__ );
        (void)osQueueTryRead( &pCamEngineHolder->m_queueResChange, &dummy );  // remove previous change request if not serviced so far
        (void)osQueueWrite( &pCamEngineHolder->m_queueResChange, &NewRes );   // add new change request
        TRACE( CAM_API_CAMENGINE_INFO, "%s (AFPS_RES_CHANGE queued)\n", __FUNCTION__ );
    }
}


/******************************************************************************
 * CamEngineItf
 *****************************************************************************/
CamEngineItf::CamEngineItf( HalHandle_t hHal, AfpsResChangeCb_t *pcbResChange, void *ctxCbResChange,int mipiLaneNum )
  : m_hHal( hHal ),
    m_pcbItfAfpsResChange( pcbResChange ),
    m_ctxCbResChange( ctxCbResChange ),
    m_pSensor    ( new SensorHolder( hHal ) ),
    m_pCamEngine ( new CamEngineHolder( hHal, this, isBitstream3D(),mipiLaneNum) )
{
    DCT_ASSERT( NULL != m_pSensor );
    DCT_ASSERT( NULL != m_pCamEngine );
}


/******************************************************************************
 * ~CamEngineItf
 *****************************************************************************/
CamEngineItf::~CamEngineItf()
{
    // stop & release sensor and camerIC
    closeSensor();

    delete m_pCamEngine;
    delete m_pSensor;
}


/******************************************************************************
 * state
 *****************************************************************************/
CamEngineItf::State
CamEngineItf::state() const
{
    return m_pCamEngine->state;
}


/******************************************************************************
 * configType
 *****************************************************************************/
CamEngineItf::ConfigType
CamEngineItf::configType() const
{
    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
    {
        return Sensor;
    }
   
    if ( CAM_ENGINE_MODE_IMAGE_PROCESSING == m_pCamEngine->camEngineConfig.mode )
    {
        return Image;
    }

    return None;
}


/******************************************************************************
 * configMode
 *****************************************************************************/
CamEngineModeType_t CamEngineItf::configMode() const
{
    return ( m_pCamEngine->camEngineConfig.mode );
}


/******************************************************************************
 * softwareVersion
 *****************************************************************************/
const char *
CamEngineItf::softwareVersion() const
{
    return (const char *)GetVersion();
}


/******************************************************************************
 * bitstreamId
 *****************************************************************************/
uint32_t
CamEngineItf::bitstreamId() const
{
    return HalReadSysReg( m_hHal, 0x0000 );
}


/******************************************************************************
 * camerIcMasterId 
 *****************************************************************************/
uint32_t
CamEngineItf::camerIcMasterId() const
{
    RESULT      result = RET_SUCCESS;
    uint32_t    id     = 0;

    if ( NULL == m_pCamEngine->hCamEngine )
    {
        return ( 0xFFFFFFFFUL );
    }

    result = CamEngineCamerIcMasterId( m_pCamEngine->hCamEngine, &id );
    if ( RET_SUCCESS  != result )
    {
        return ( 0xFFFFFFFFUL );
    }

    return id;
}


/******************************************************************************
 * camerIcMasterId 
 *****************************************************************************/
uint32_t
CamEngineItf::camerIcSlaveId() const
{
    RESULT      result = RET_SUCCESS;
    uint32_t    id     = 0;

    if ( !isBitstream3D() )
    {
        return ( 0xFFFFFFFFUL );
    }

    if ( NULL == m_pCamEngine->hCamEngine )
    {
        return ( 0xFFFFFFFFUL );
    }

    result = CamEngineCamerIcSlaveId( m_pCamEngine->hCamEngine, &id );
    if ( RET_SUCCESS  != result )
    {
        return ( 0xFFFFFFFFUL );
    }

    return id;
}


/******************************************************************************
 * isBitstream3D
 *****************************************************************************/
bool
CamEngineItf::isBitstream3D() const
{
    return (bool)(bitstreamId() & 0x3D000000);
}


/******************************************************************************
 * openSensor
 *****************************************************************************/
bool
CamEngineItf::openSensor( const char *pFileName, int sensorItfIdx )
{
    IsiSensorInstanceConfig_t sensorInstanceConfig;
    if ( NULL != m_pSensor->hSensorLib )
    {
        closeSensor();
    }
    if ( NULL != m_pSensor->image.Data.raw.pBuffer )
    {
        closeImage();
    }

    DCT_ASSERT( NULL  == m_pSensor->hSensorLib );
    DCT_ASSERT( NULL  == m_pSensor->pCamDrvConfig );
    DCT_ASSERT( NULL  == m_pSensor->hSensor );
    DCT_ASSERT( NULL  == m_pSensor->hSubSensor );
    DCT_ASSERT( NULL  == m_pSensor->image.Data.raw.pBuffer );
    DCT_ASSERT( Invalid == m_pCamEngine->state );

    // sensor driver file
    m_pCamEngine->sensorDrvFile = std::string( pFileName );

#if 0
    // load calibration data
    QString str = QString::fromAscii( pFileName );
    str.replace( QString( ".drv" ), QString( ".xml" ) );
    if ( true != loadCalibrationData( str.toAscii().constData() ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load the calibration data: %s)\n", __FUNCTION__, str.toAscii().constData() );
        return false;
    }
#else    
    //serch ".drv"
    std::string str = std::string( pFileName );
    std::string strReplace = ".so";

    std::size_t found = str.find(strReplace);
    if (found!=std::string::npos){
        str.replace(found,strlen(".xml"),".xml");
    }
    if ( true != loadCalibrationData( str.c_str() ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load the calibration data: %s)\n", __FUNCTION__, str.c_str() );
        return false;
    }
#endif

    // read system configuration
    CamCalibSystemData_t systemData;
    MEMSET( &systemData, 0, sizeof(CamCalibSystemData_t) );

    if ( RET_SUCCESS != CamCalibDbGetSystemData( m_pCamEngine->calibDb.GetCalibDbHandle(), &systemData ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't read the system configuration: %s)\n", __FUNCTION__, str.c_str() );
        return false;
    }
    m_pSensor->enableAfps = ( (BOOL_TRUE == systemData.AfpsDefault) && (CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB != m_pCamEngine->camEngineConfig.mode) ) ? true : false;

    // configure sample edge on fpga
    HalWriteSysReg( m_pSensor->hHal, 0x0004, 0xC601 ); //TODO: this crude write does probably overwrite vital HW control signal settings

    // open sensor driver file
    m_pSensor->hSensorLib = dlopen( pFileName, RTLD_NOW/*RTLD_LAZY*/ );
    if ( NULL == m_pSensor->hSensorLib )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't open the specified driver)\n", __FUNCTION__ );
        return false;
    }

    // load sensor configuration struct from driver file
    m_pSensor->pCamDrvConfig = (IsiCamDrvConfig_t *)dlsym( m_pSensor->hSensorLib, "IsiCamDrvConfig" );
    if ( NULL == m_pSensor->pCamDrvConfig )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load sensor driver)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    // initialize function pointer
    if ( RET_SUCCESS != m_pSensor->pCamDrvConfig->pfIsiGetSensorIss( &(m_pSensor->pCamDrvConfig->IsiSensor) ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiGetSensorIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }
    
    // create sub/right sensor instance first; required due to current control signal sharing bewteen main & sub sensor
    if ( ( true == isBitstream3D() ) && ( is3DEnabled() ) )
    {
        // setup sub/right instance
        MEMSET( &sensorInstanceConfig, 0, sizeof(IsiSensorInstanceConfig_t) );
        sensorInstanceConfig.hSensor     = NULL;
        sensorInstanceConfig.HalHandle   = m_pSensor->hHal;
        sensorInstanceConfig.HalDevID    = HAL_DEVID_CAM_2;
        sensorInstanceConfig.I2cBusNum   = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAddr   = 0U;
        sensorInstanceConfig.I2cAfBusNum = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAfAddr = 0U;
        sensorInstanceConfig.pSensor     = &m_pSensor->pCamDrvConfig->IsiSensor;

        if ( RET_SUCCESS != IsiCreateSensorIss( &sensorInstanceConfig ) )
        {

            TRACE( CAM_API_CAMENGINE_WARN, "%s (IsiCreateSensorIss (SubSensor) failed, probably no sub-sensor connected -> 3D not supported)\n", __FUNCTION__ );
            closeSensor();
            return false;
        }
        else
        {

            // power on sub/right sensor
            if ( RET_SUCCESS != IsiSensorSetPowerIss( sensorInstanceConfig.hSensor, BOOL_TRUE ) )
            {

                TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiSensorSetPowerIss (SubSensor) failed, probably no sub-sensor connected -> 3D not supported)\n", __FUNCTION__ );
                (void)IsiReleaseSensorIss( sensorInstanceConfig.hSensor );
                closeSensor();
                return false;
            }
            else
            {
                // check sub/right sensor connection (internal driver revision - id check)
                if ( RET_SUCCESS != IsiCheckSensorConnectionIss( sensorInstanceConfig.hSensor ) )
                {

                    TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiCheckSensorConnectionIss (SubSensor) failed, probably no sub-sensor connected -> 3D not supported)\n", __FUNCTION__ );
                    (void)IsiSensorSetPowerIss( sensorInstanceConfig.hSensor, BOOL_FALSE );
                    (void)IsiReleaseSensorIss( sensorInstanceConfig.hSensor );
                    closeSensor();
                    return false;
                }
                else
                {
                    // sub/right sensor is connected, store handle
                    m_pSensor->hSubSensor = sensorInstanceConfig.hSensor;
                }
            }
        }
    }

    // setup main/left instance
    MEMSET( &sensorInstanceConfig, 0, sizeof(IsiSensorInstanceConfig_t) );
    sensorInstanceConfig.hSensor     = NULL;
    sensorInstanceConfig.HalHandle   = m_pSensor->hHal;
    sensorInstanceConfig.pSensor     = &m_pSensor->pCamDrvConfig->IsiSensor;

    // in image stabilization mode, we use the slave as main pipe
    if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode )
    {
        sensorInstanceConfig.HalDevID    = HAL_DEVID_CAM_2;
        sensorInstanceConfig.I2cBusNum   = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAddr   = 0U;
        sensorInstanceConfig.I2cAfBusNum = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAfAddr = 0U;
    }
    else
    {
        sensorInstanceConfig.HalDevID    = ( !sensorItfIdx ) ? HAL_DEVID_CAM_1A : HAL_DEVID_CAM_1B;
        sensorInstanceConfig.I2cBusNum   = ( !sensorItfIdx ) ? HAL_I2C_BUS_CAM_1A : HAL_I2C_BUS_CAM_1B;
        sensorInstanceConfig.SlaveAddr   = 0U;
        sensorInstanceConfig.I2cAfBusNum = ( !sensorItfIdx ) ? HAL_I2C_BUS_CAM_1A : HAL_I2C_BUS_CAM_1B;
        sensorInstanceConfig.SlaveAfAddr = 0U;
    }

    // create main/left sensor instance
    if ( RET_SUCCESS != IsiCreateSensorIss( &sensorInstanceConfig ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiCreateSensorIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }
    m_pSensor->hSensor = sensorInstanceConfig.hSensor;


    TRACE(CAM_API_CAMENGINE_ERROR,"POWER ON SENSOR");
    // power on main/left sensor
    if ( RET_SUCCESS != IsiSensorSetPowerIss( m_pSensor->hSensor, BOOL_TRUE ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiSensorSetPowerIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    // check main/left sensor connection (internal driver revision - id check)
    if ( RET_SUCCESS != IsiCheckSensorConnectionIss( m_pSensor->hSensor ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiCheckSensorConnectionIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    // get shared default config for main & sub sensor from main sensor
    MEMCPY( &m_pSensor->sensorConfig, m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps, sizeof( IsiSensorCaps_t ) );

    // get shared max focus for main & sub sensor from main sensor
    RESULT result = IsiMdiInitMotoDrive( m_pSensor->hSensor );
    if ( RET_SUCCESS == result )
    {
        if ( RET_SUCCESS != IsiMdiSetupMotoDrive( m_pSensor->hSensor, &m_pSensor->maxFocus ) )
        {

            TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiMdiSetupMotoDrive failed)\n", __FUNCTION__ );
            m_pSensor->maxFocus = 0;
        } else {
            m_pSensor->maxFocus = m_pSensor->maxFocus&0xffff;
        }
    }
    else if ( RET_NOTSUPP == result )
    {

        TRACE( CAM_API_CAMENGINE_INFO, "%s (Autofocus not available)\n", __FUNCTION__ );
        m_pSensor->maxFocus = 0;
    }
    else
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiMdiInitMotoDrive failed)\n", __FUNCTION__ );
        m_pSensor->maxFocus = 0;
    }

    return true;
}

/******************************************************************************
 * openSensor
 *****************************************************************************/
bool
CamEngineItf::openSensor( rk_cam_total_info *pCamInfo, int sensorItfIdx )
{
    TRACE( CAM_API_CAMENGINE_INFO, "%s(%d)\n", __FUNCTION__, __LINE__);

    IsiSensorInstanceConfig_t sensorInstanceConfig;
    if ( NULL != m_pSensor->hSensorLib )
    {
        closeSensor();
    }
    if ( NULL != m_pSensor->image.Data.raw.pBuffer )
    {
        closeImage();
    }

    DCT_ASSERT( NULL  == m_pSensor->hSensorLib );
    DCT_ASSERT( NULL  == m_pSensor->pCamDrvConfig );
    DCT_ASSERT( NULL  == m_pSensor->hSensor );
    DCT_ASSERT( NULL  == m_pSensor->hSubSensor );
    DCT_ASSERT( NULL  == m_pSensor->image.Data.raw.pBuffer );
    DCT_ASSERT( Invalid == m_pCamEngine->state );
    

    // sensor driver file
    m_pCamEngine->sensorDrvFile = std::string( pCamInfo->mLoadSensorInfo.mSensorLibName);


    mSensorBusWidthBoardSet = pCamInfo->mHardInfo.mSensorInfo.mPhy.info.cif.fmt;
    // configure sample edge on fpga
    HalWriteSysReg( m_pSensor->hHal, 0x0004, 0xC601 ); //TODO: this crude write does probably overwrite vital HW control signal settings

    // open sensor driver file
    m_pSensor->hSensorLib = pCamInfo->mLoadSensorInfo.mhSensorLib;
    if ( NULL == m_pSensor->hSensorLib )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't open the specified driver)\n", __FUNCTION__ );
        return false;
    }

    // load sensor configuration struct from driver file
    m_pSensor->pCamDrvConfig = pCamInfo->mLoadSensorInfo.pCamDrvConfig;
    if ( NULL == m_pSensor->pCamDrvConfig )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load sensor driver)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    // initialize function pointer
    if ( RET_SUCCESS != m_pSensor->pCamDrvConfig->pfIsiGetSensorIss( &(m_pSensor->pCamDrvConfig->IsiSensor) ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiGetSensorIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    //if(!isSOCSensor()){ //zyc
	if(m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps->SensorOutputMode == ISI_SENSOR_OUTPUT_MODE_RAW){ //oyyf
        //serch ".drv"
        if ( true != loadCalibrationData( &(pCamInfo->mLoadSensorInfo) ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load the calibration data: %s)\n", __FUNCTION__, pCamInfo->mLoadSensorInfo.mSensorXmlFile);
            return false;
        }
        
        // read system configuration
        CamCalibSystemData_t systemData;
        MEMSET( &systemData, 0, sizeof(CamCalibSystemData_t) );

        if ( RET_SUCCESS != CamCalibDbGetSystemData( m_pCamEngine->calibDb.GetCalibDbHandle(), &systemData ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't read the system configuration: %s)\n", __FUNCTION__,  pCamInfo->mLoadSensorInfo.mSensorXmlFile );
            return false;
        }
        m_pSensor->enableAfps = ( (BOOL_TRUE == systemData.AfpsDefault) && (CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB != m_pCamEngine->camEngineConfig.mode) ) ? true : false;
    }



    // create sub/right sensor instance first; required due to current control signal sharing bewteen main & sub sensor
    if ( ( true == isBitstream3D() ) && ( is3DEnabled() ) )
    {
        // setup sub/right instance
        MEMSET( &sensorInstanceConfig, 0, sizeof(IsiSensorInstanceConfig_t) );
        sensorInstanceConfig.hSensor     = NULL;
        sensorInstanceConfig.HalHandle   = m_pSensor->hHal;
        sensorInstanceConfig.HalDevID    = HAL_DEVID_CAM_2;
        sensorInstanceConfig.I2cBusNum   = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAddr   = 0U;
        sensorInstanceConfig.I2cAfBusNum = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAfAddr = 0U;
        sensorInstanceConfig.pSensor     = &m_pSensor->pCamDrvConfig->IsiSensor;

        if ( RET_SUCCESS != IsiCreateSensorIss( &sensorInstanceConfig ) )
        {

            TRACE( CAM_API_CAMENGINE_WARN, "%s (IsiCreateSensorIss (SubSensor) failed, probably no sub-sensor connected -> 3D not supported)\n", __FUNCTION__ );
            closeSensor();
            return false;
        }
        else
        {

            // power on sub/right sensor
            if ( RET_SUCCESS != IsiSensorSetPowerIss( sensorInstanceConfig.hSensor, BOOL_TRUE ) )
            {

                TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiSensorSetPowerIss (SubSensor) failed, probably no sub-sensor connected -> 3D not supported)\n", __FUNCTION__ );
                (void)IsiReleaseSensorIss( sensorInstanceConfig.hSensor );
                closeSensor();
                return false;
            }
            else
            {
                // check sub/right sensor connection (internal driver revision - id check)
                if ( RET_SUCCESS != IsiCheckSensorConnectionIss( sensorInstanceConfig.hSensor ) )
                {

                    TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiCheckSensorConnectionIss (SubSensor) failed, probably no sub-sensor connected -> 3D not supported)\n", __FUNCTION__ );
                    (void)IsiSensorSetPowerIss( sensorInstanceConfig.hSensor, BOOL_FALSE );
                    (void)IsiReleaseSensorIss( sensorInstanceConfig.hSensor );
                    closeSensor();
                    return false;
                }
                else
                {
                    // sub/right sensor is connected, store handle
                    m_pSensor->hSubSensor = sensorInstanceConfig.hSensor;
                }
            }
        }
    }

    // setup main/left instance
    MEMSET( &sensorInstanceConfig, 0, sizeof(IsiSensorInstanceConfig_t) );
    sensorInstanceConfig.hSensor     = NULL;
    sensorInstanceConfig.HalHandle   = m_pSensor->hHal;
    sensorInstanceConfig.pSensor     = &m_pSensor->pCamDrvConfig->IsiSensor;
    sensorInstanceConfig.mipiLaneNum   = m_pCamEngine->camEngineConfig.mipiLaneNum;

    // in image stabilization mode, we use the slave as main pipe
    if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode )
    {
        sensorInstanceConfig.HalDevID    = HAL_DEVID_CAM_2;
        sensorInstanceConfig.I2cBusNum   = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAddr   = 0U;
        sensorInstanceConfig.I2cAfBusNum = HAL_I2C_BUS_CAM_2;
        sensorInstanceConfig.SlaveAfAddr = 0U;
    }
    else
    {
        sensorInstanceConfig.HalDevID    = pCamInfo->mHardInfo.mSensorInfo.mCamDevid;
        sensorInstanceConfig.I2cBusNum   = pCamInfo->mHardInfo.mSensorInfo.mSensorI2cBusNum;
        sensorInstanceConfig.SlaveAddr   = pCamInfo->mLoadSensorInfo.mpI2cInfo->i2c_addr;
        sensorInstanceConfig.I2cAfBusNum = pCamInfo->mHardInfo.mVcmInfo.mVcmI2cBusNum;
        sensorInstanceConfig.SlaveAfAddr = 0U;

        sensorInstanceConfig.VcmStartCurrent = pCamInfo->mHardInfo.mVcmInfo.mStartCurrent;
        sensorInstanceConfig.VcmRatedCurrent = pCamInfo->mHardInfo.mVcmInfo.mRatedCurrent;
        sensorInstanceConfig.VcmMaxCurrent = pCamInfo->mHardInfo.mVcmInfo.mVcmMaxCurrent;
        sensorInstanceConfig.VcmDrvMaxCurrent = pCamInfo->mHardInfo.mVcmInfo.mVcmDrvMaxCurrent;
        sensorInstanceConfig.VcmStepMode = pCamInfo->mHardInfo.mVcmInfo.mStepMode;
    }

    // create main/left sensor instance
    if ( RET_SUCCESS != IsiCreateSensorIss( &sensorInstanceConfig ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiCreateSensorIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }
    m_pSensor->hSensor = sensorInstanceConfig.hSensor;
    
    // set preview frame limit
    if( pCamInfo->mSoftInfo.mFrameRate > 0 )
    	IsiSensorFrameRateLimitSet( m_pSensor->hSensor, pCamInfo->mSoftInfo.mFrameRate);

    // power on main/left sensor
    if ( RET_SUCCESS != IsiSensorSetPowerIss( m_pSensor->hSensor, BOOL_TRUE ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiSensorSetPowerIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    // check main/left sensor connection (internal driver revision - id check)
    if ( RET_SUCCESS != IsiCheckSensorConnectionIss( m_pSensor->hSensor ) )
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiCheckSensorConnectionIss failed)\n", __FUNCTION__ );
        closeSensor();
        return false;
    }

    // get shared default config for main & sub sensor from main sensor
    MEMCPY( &m_pSensor->sensorConfig, m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps, sizeof( IsiSensorCaps_t ) );

    // get shared max focus for main & sub sensor from main sensor
    RESULT result = IsiMdiInitMotoDrive( m_pSensor->hSensor );
    if ( RET_SUCCESS == result )
    {
        if ( RET_SUCCESS != IsiMdiSetupMotoDrive( m_pSensor->hSensor, &m_pSensor->maxFocus ) )
        {

            TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiMdiSetupMotoDrive failed)\n", __FUNCTION__ );
            m_pSensor->maxFocus = 0;
        } else {
            m_pSensor->maxFocus = m_pSensor->maxFocus&0xffff;
        }
    }
    else if ( RET_NOTSUPP == result )
    {

        TRACE( CAM_API_CAMENGINE_INFO, "%s (Autofocus not available)\n", __FUNCTION__ );
        m_pSensor->maxFocus = 0;
    }
    else
    {

        TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiMdiInitMotoDrive failed)\n", __FUNCTION__ );
        m_pSensor->maxFocus = 0;
    }

    return true;
}

/******************************************************************************
 * previewSensor_ex
 *zyc add
 width_in,height_in:user preferred resolution
 width_out,height_out:preferred resolution
 method:
     1 :  get pixels diff is min and image aspect ratio is same;
     2 :  get pixels diff is min;
 *****************************************************************************/
bool
CamEngineItf::getPreferedSensorRes(CamEngineBestSensorResReq_t *req)
{
    // get sensor capabilities
    bool result, match;
    IsiSensorCaps_t sensorCaps;
    IsiAfpsInfo_t sensorAfpsInfo;
    unsigned int max_w,max_h,best_res,max_res,max_exp_res,max_fps,max_fps_res,i;
    float max_exp;

    max_w = 0;
    max_h = 0;
    max_fps = 0;
    max_fps_res = 0;
    max_exp_res = 0;
    max_exp = 0.0;
    best_res = 0xFFFFFFFF;
    MEMSET( &sensorCaps, 0, sizeof( IsiSensorCaps_t ) );       
    MEMSET( &sensorAfpsInfo,0,sizeof(IsiAfpsInfo_t)); 

    TRACE(CAM_API_CAMENGINE_NOTICE1, "Request %dx%d, Exp(>= %f), Fps(>= %dfps) ",
            req->request_w,req->request_h,req->request_exp_t,req->request_fps);
    
    while (getSensorCaps( sensorCaps ) == true) {
        match = false;
        if (max_w*max_h < (ISI_RES_W_GET(sensorCaps.Resolution)*ISI_RES_H_GET(sensorCaps.Resolution))) {
            max_w = ISI_RES_W_GET(sensorCaps.Resolution);
            max_h = ISI_RES_H_GET(sensorCaps.Resolution);
            max_res = sensorCaps.Resolution;
        }
        
        if ((ISI_RES_W_GET(sensorCaps.Resolution)*ISI_RES_H_GET(sensorCaps.Resolution)*10)>=(req->request_w*req->request_h*9)) {
            if (ISI_FPS_GET(sensorCaps.Resolution) >= req->request_fps) {
                if (req->requset_aspect == BOOL_TRUE) {
                    if (ISI_RES_W_GET(sensorCaps.Resolution)*10/ISI_RES_H_GET(sensorCaps.Resolution) == 
                        req->request_w*10/req->request_h) {
                        match = true;
                    }
                } else {
                    match = true;
                }

                if ((req->request_exp_t -  0.0) > 0.001) {
                    match = false;
                    if ( (ISI_RES_W_GET(sensorCaps.Resolution) != ISI_RES_W_GET(sensorAfpsInfo.Stage[0].Resolution))
                        || (ISI_RES_H_GET(sensorCaps.Resolution) != ISI_RES_H_GET(sensorAfpsInfo.Stage[0].Resolution)) ) {

                        IsiGetAfpsInfoIss(m_pSensor->hSensor, sensorCaps.Resolution, &sensorAfpsInfo);
                    }

                    for (i=0; i<ISI_NUM_AFPS_STAGES; i++) {
                        if (sensorAfpsInfo.Stage[i].Resolution == sensorCaps.Resolution) {
                            TRACE( CAM_API_CAMENGINE_DEBUG, "%d: %dx%d@%dfps MaxIntTime(%f), curMaxIntTime(%f)",
                                i,
                                ISI_RES_W_GET(sensorAfpsInfo.Stage[i].Resolution),
                                ISI_RES_H_GET(sensorAfpsInfo.Stage[i].Resolution),
                                ISI_FPS_GET(sensorAfpsInfo.Stage[i].Resolution),
                                sensorAfpsInfo.Stage[i].MaxIntTime,
                                req->request_exp_t);
                            if (sensorAfpsInfo.Stage[i].MaxIntTime >= req->request_exp_t) {
                                match = true;
                            } 

                            if (sensorAfpsInfo.Stage[i].MaxIntTime > max_exp) {
                                max_exp = sensorAfpsInfo.Stage[i].MaxIntTime;
                                max_exp_res = sensorAfpsInfo.Stage[i].Resolution;
                            }
                            break;
                        }
                    }                    
                }
            }

            if (ISI_FPS_GET(sensorCaps.Resolution) > max_fps) {
                max_fps = ISI_FPS_GET(sensorCaps.Resolution);
                max_fps_res = sensorCaps.Resolution;
            }
            
        }

        if (match == true) {
            if (ISI_RES_W_GET(sensorCaps.Resolution)*ISI_RES_H_GET(sensorCaps.Resolution) < 
                ISI_RES_W_GET(best_res)*ISI_RES_H_GET(best_res)) {
                best_res = sensorCaps.Resolution;
            }
        }
        
        sensorCaps.Index++;
    };
    if (best_res == 0xFFFFFFFF) {

        if ((req->request_exp_t -  0.0) > 0.001) {
            if ( max_exp_res != 0 ) {
                best_res = max_exp_res;
                TRACE( CAM_API_CAMENGINE_WARN,"Request %dx%d, Exp(>= %f) failed, select max_exp_res: %dx%d@%dfps, MaxIntTime: %f",
                    req->request_w,req->request_h,req->request_exp_t,
                    ISI_RES_W_GET(max_exp_res),ISI_RES_H_GET(max_exp_res),ISI_FPS_GET(max_exp_res),
                    max_exp);
            }
        } else if ( req->request_fps > 0 ) {
            if ( max_fps_res != 0 ) {
                best_res = max_fps_res;
                TRACE( CAM_API_CAMENGINE_WARN,"Request %dx%d, Fps(>= %dfps) failed, select max_fps_res: %dx%d@%dfps",
                    req->request_w,req->request_h,req->request_fps,
                    ISI_RES_W_GET(max_fps_res),ISI_RES_H_GET(max_fps_res),ISI_FPS_GET(max_fps_res));
            }
        } else { 
            if ( max_res!=0 ) {
                best_res = max_res;
                TRACE( CAM_API_CAMENGINE_WARN,"Request %dx%d, Exp(>= %f), Fps(>= %dfps) failed, select max_res: %dx%d@%dfps",
                     req->request_w,req->request_h,req->request_exp_t,req->request_fps,
                     ISI_RES_W_GET(max_res),ISI_RES_H_GET(max_res),ISI_FPS_GET(max_res));
            }
        }
    }

    if (req->request_fullfov) {
        best_res = max_res;
    }

    if (best_res!=0xFFFFFFFF) {
        req->resolution = best_res;
        TRACE( CAM_API_CAMENGINE_NOTICE1, "getPreferedSensorRes is %dx%d@%dfps",
            ISI_RES_W_GET(best_res),
            ISI_RES_H_GET(best_res),
            ISI_FPS_GET(best_res));
        return true;
    } else {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s failed! best_res: 0x%x max_res: 0x%x\n", __FUNCTION__,
            best_res,max_res);
        return false;
    }
    
}


uint32_t CamEngineItf::getYCSequence(){
    return m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps->YCSequence;

}

uint32_t CamEngineItf::getBusWidth(){
   // return m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps->BusWidth;
   if(mSensorBusWidthBoardSet == CamSys_Fmt_Raw_10b)
        return ISI_BUSWIDTH_12BIT;
   else if(mSensorBusWidthBoardSet == CamSys_Fmt_Raw_12b)
   	    return ISI_BUSWIDTH_12BIT;
   else
        return ISI_BUSWIDTH_8BIT_ZZ;

}


bool  CamEngineItf::isSOCSensor()
{
    if(m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps->SensorOutputMode == ISI_SENSOR_OUTPUT_MODE_YUV){
        TRACE( CAM_API_CAMENGINE_INFO, "%s this is a soc  sensor\n", __FUNCTION__ );
        CamEngineSetIsSocSensor(m_pCamEngine->hCamEngine,true);
        return true;
    }else{
        CamEngineSetIsSocSensor(m_pCamEngine->hCamEngine,false);
        return false;
    }
}

bool  CamEngineItf::setIspBufferInfo(unsigned int bufNum, unsigned int bufSize)
{
	CamEngineSetBufferSize(m_pCamEngine->hCamEngine, bufNum, bufSize); 
	return true;
}

/*******************************************************
******oyyf add getIspVersion
************************************************/
void CamEngineItf::getIspVersion(unsigned int* version)
{
	*version = CONFIG_SILICONIMAGE_LIBISP_VERSION;
}

/******************************************************************************
 * previewSetup_ex
 *zyc add
 *****************************************************************************/
bool
CamEngineItf::previewSetup_ex(CamEngineWindow_t dcWin,int usr_w,int usr_h,CamerIcMiDataMode_t mode,CamerIcMiDataLayout_t layout,bool_t dcEnable)
{
    switch ( m_pCamEngine->camEngineConfig.mode )
    {
        case CAM_ENGINE_MODE_SENSOR_2D:
        case CAM_ENGINE_MODE_IMAGE_PROCESSING:
        {
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN].width = usr_w;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN].height = usr_h;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN].mode = mode;//CAMERIC_MI_DATAMODE_RAW12/*CAMERIC_MI_DATAMODE_YUV422*/;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN].layout = layout;//CAMERIC_MI_DATASTORAGE_INTERLEAVED;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN].dcEnable = dcEnable;//BOOL_TRUE;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN].dcWin = dcWin;
            
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathDisabled ;
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;
            break;
        }

        default:
        {
            return ( false );
        }
    }
    return true;
}

/******************************************************************************
 * pre2capparameter
 *****************************************************************************/
bool
CamEngineItf::pre2capparameter(bool is_capture,rk_cam_total_info *pCamInfo)
{
	CamerIcColorConversionRange_t YConvRange = CAMERIC_ISP_CCONV_RANGE_FULL_RANGE;
	CamerIcColorConversionRange_t CrConvRange = CAMERIC_ISP_CCONV_RANGE_FULL_RANGE;
	CamEngineCprocConfig_t cproc_config = {
		CAM_ENGINE_CPROC_CHROM_RANGE_OUT_BT601,//CAM_ENGINE_CPROC_CHROM_RANGE_OUT_FULL_RANGE,
		CAM_ENGINE_CPROC_LUM_RANGE_OUT_FULL_RANGE,//CAM_ENGINE_CPROC_LUM_RANGE_OUT_BT601,//,
		CAM_ENGINE_CPROC_LUM_RANGE_IN_FULL_RANGE,//CAM_ENGINE_CPROC_LUM_RANGE_IN_BT601,//,
		1.1,  //contrast 0-1.992
		0,		//brightness -128 - 127
		1.0,	  //saturation 0-1.992
		0,		//hue	-90 - 87.188
	};

	if(!pCamInfo)
		return false;

    //must to get illum befor resolution changed
    if (is_capture) {
		if(pCamInfo->mSoftInfo.mCapCprocConfig.mSupported == true){
			cproc_config.ChromaOut = CAM_ENGINE_CPROC_CHROM_RANGE_OUT_FULL_RANGE;
			cproc_config.LumaOut = CAM_ENGINE_CPROC_LUM_RANGE_OUT_FULL_RANGE;
			cproc_config.LumaIn = CAM_ENGINE_CPROC_LUM_RANGE_IN_FULL_RANGE;
            cproc_config.contrast = pCamInfo->mSoftInfo.mCapCprocConfig.mContrast;
            cproc_config.saturation= pCamInfo->mSoftInfo.mCapCprocConfig.mSaturation;
            cproc_config.hue= pCamInfo->mSoftInfo.mCapCprocConfig.mHue;
            cproc_config.brightness= pCamInfo->mSoftInfo.mCapCprocConfig.mBrightness;
			cProcEnable( &cproc_config);
        }
		YConvRange = CAMERIC_ISP_CCONV_RANGE_FULL_RANGE;
		CrConvRange = CAMERIC_ISP_CCONV_RANGE_FULL_RANGE;
    }
	else
	{
		if(pCamInfo->mSoftInfo.mPreCprocConfig.mSupported == true){
			cproc_config.ChromaOut = CAM_ENGINE_CPROC_CHROM_RANGE_OUT_BT601;
			cproc_config.LumaOut = CAM_ENGINE_CPROC_LUM_RANGE_OUT_BT601;
			cproc_config.LumaIn = CAM_ENGINE_CPROC_LUM_RANGE_IN_BT601;
            cproc_config.contrast = pCamInfo->mSoftInfo.mPreCprocConfig.mContrast;
            cproc_config.saturation= pCamInfo->mSoftInfo.mPreCprocConfig.mSaturation;
            cproc_config.hue= pCamInfo->mSoftInfo.mPreCprocConfig.mHue;
            cproc_config.brightness= pCamInfo->mSoftInfo.mPreCprocConfig.mBrightness;
			cProcEnable( &cproc_config);
        }
		YConvRange = CAMERIC_ISP_CCONV_RANGE_LIMITED_RANGE;
		CrConvRange = CAMERIC_ISP_CCONV_RANGE_LIMITED_RANGE;
	}
	ColorConversionSetRange(YConvRange, CrConvRange);

    return true;
}

bool
CamEngineItf::restartMipiDrv()
{
    IsiSensorMipiInfo curIsiSensorMipiInfo;
    //check engine state ?

    m_pSensor->pCamDrvConfig->IsiSensor.pIsiGetSensorMipiInfoIss(m_pSensor->hSensor,&curIsiSensorMipiInfo);

    //need to restart mipi drv?
    if((m_pCamEngine->camEngineConfig.mipiLaneNum != curIsiSensorMipiInfo.ucMipiLanes) || 
        (m_pCamEngine->camEngineConfig.mipiLaneFreq != curIsiSensorMipiInfo.ulMipiFreq)){
        m_pCamEngine->camEngineConfig.mipiLaneNum = curIsiSensorMipiInfo.ucMipiLanes;
        m_pCamEngine->camEngineConfig.mipiLaneFreq = curIsiSensorMipiInfo.ulMipiFreq;
        m_pCamEngine->camEngineConfig.phyAttachedDevId = curIsiSensorMipiInfo.sensorHalDevID;
        CamEngineStartPixelIfApi(m_pCamEngine->hCamEngine,&m_pCamEngine->camEngineConfig);
        TRACE( CAM_API_CAMENGINE_INFO, "%s reset mipi drv,mipi lanenum %d,mipi freq 0x%x,devid 0x%x\n", __FUNCTION__ ,m_pCamEngine->camEngineConfig.mipiLaneNum,m_pCamEngine->camEngineConfig.mipiLaneFreq,m_pCamEngine->camEngineConfig.phyAttachedDevId );

    }

    return true;
}

uint32_t 
CamEngineItf::reSetMainPathWhileStreaming
(
    CamEngineWindow_t* pWin,
    uint32_t outWidth, 
    uint32_t outHeight
    
)
{
    CamerEngineSetDataPathWhileStreaming(
                m_pCamEngine->hCamEngine,
                (CamerIcWindow_t*)pWin,
                outWidth, 
                outHeight);
    return 0;
}



/******************************************************************************
 * previewSensor
 *****************************************************************************/
bool
CamEngineItf::previewSensor()
{
    // configure sensor for preview resolution

    // get sensor capabilities
    IsiSensorCaps_t sensorCaps;
    MEMSET( &sensorCaps, 0, sizeof( IsiSensorCaps_t ) );
    getSensorCaps( sensorCaps );

    // get resolution index
    uint32_t resolution = 0;

    // use one of the preferred preview resolutions
    uint32_t resolutionCaps[] =
    { // in descending priority order
        //ISI_RES_TV1080P60,
        //ISI_RES_TV1080P50,
        #if 0
        ISI_RES_1296_972,
        ISI_RES_TV1080P30,
        //ISI_RES_TV1080P25,
        ISI_RES_TV1080P24,
        ISI_RES_TV1080P15,
        ISI_RES_TV1080P12,
        ISI_RES_TV1080P10,
        ISI_RES_TV1080P6 ,
        ISI_RES_TV1080P5 ,
        ISI_RES_TV720P60 ,
        ISI_RES_TV720P30 ,
        ISI_RES_TV720P15 ,
        ISI_RES_TV720P5 ,
        ISI_RES_VGA      ,
        ISI_RES_4224_3136,
        ISI_RES_2112_1568,
        ISI_RES_1632_1224,
        #endif
        0 // must be last, marks end of array/list
    };

    if ( BOOL_TRUE == IsiTryToSetConfigFromPreferredCaps(
                        &resolution, resolutionCaps, sensorCaps.Resolution ) )
    {
        IsiSensorConfig_t sensorConfig;
        MEMSET( &sensorConfig, 0, sizeof( IsiSensorConfig_t ) );
        CamEngineItf::getSensorConfig( sensorConfig );

        if ( sensorConfig.Resolution != resolution )
        {
            sensorConfig.Resolution = resolution;
            CamEngineItf::setSensorConfig( sensorConfig );
        }
    }
    else
    {
        return false;
    }

    return true;
}


/******************************************************************************
 * openImage
 *****************************************************************************/
bool
CamEngineItf::openImage
(
    const char              *pFileName,
    PicBufMetaData_t        image,
    CamEngineWbGains_t      *wbGains,
    CamEngineCcMatrix_t     *ccMatrix,
    CamEngineCcOffset_t     *ccOffset,
    CamEngineBlackLevel_t   *blvl,
    float                   gain,
    float                   itime
)
{
    if ( NULL != m_pSensor->hSensorLib )
    {
        closeSensor();
    }

    if ( NULL != m_pSensor->image.Data.raw.pBuffer )
    {
        closeImage();
    }

    DCT_ASSERT( NULL  == m_pSensor->hSensorLib );
    DCT_ASSERT( NULL  == m_pSensor->pCamDrvConfig );
    DCT_ASSERT( NULL  == m_pSensor->hSensor );
    DCT_ASSERT( NULL  == m_pSensor->hSubSensor );
    DCT_ASSERT( NULL  == m_pSensor->image.Data.raw.pBuffer );
    DCT_ASSERT( Invalid == m_pCamEngine->state );

    if ( ( PIC_BUF_TYPE_RAW8  != image.Type ) &&
         ( PIC_BUF_TYPE_RAW16 != image.Type ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (invalid image type)\n", __FUNCTION__ );
        return false;
    }

    if ( ( PIC_BUF_LAYOUT_BAYER_RGRGGBGB != image.Layout ) &&
         ( PIC_BUF_LAYOUT_BAYER_GRGRBGBG != image.Layout ) &&
         ( PIC_BUF_LAYOUT_BAYER_GBGBRGRG != image.Layout ) &&
         ( PIC_BUF_LAYOUT_BAYER_BGBGGRGR != image.Layout ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (invalid image layout)\n", __FUNCTION__ );
        return false;
    }

    DCT_ASSERT( NULL  != image.Data.raw.pBuffer );

    // load calibration data file, if exists
#if 0
    QString str = QString::fromAscii( pFileName );
    QFileInfo fi( str );
    str.replace( fi.suffix(), QString( "xml" ) );
    if ( QFile::exists( str ) )
    {
        if ( true != loadCalibrationData( str.toAscii().constData() ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load the calibration data: %s)\n", __FUNCTION__, str.toAscii().constData() );
            return false;
        }
    }
#else
    std::string str = std::string( pFileName );
    std::string strReplace = ".";

    std::size_t found = str.find(strReplace);
    if (found!=std::string::npos){
        //erase filename suffix
        str.erase(found,(str.length()-found));
        str +=".xml";
    }

    if(access(str.c_str(),O_RDWR) >=0){
        if ( true != loadCalibrationData( str.c_str() ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't load the calibration data: %s)\n", __FUNCTION__, str.c_str() );
            return false;
        }

    }
#endif
    m_pSensor->image.Data.raw.pBuffer = (uint8_t *)malloc( image.Data.raw.PicWidthBytes * image.Data.raw.PicHeightPixel );
    if ( NULL == m_pSensor->image.Data.raw.pBuffer )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't allocate memory)\n", __FUNCTION__ );
        return false;
    }
    MEMCPY( m_pSensor->image.Data.raw.pBuffer, image.Data.raw.pBuffer, image.Data.raw.PicWidthBytes * image.Data.raw.PicHeightPixel );

    m_pSensor->image.Data.raw.PicWidthPixel  = image.Data.raw.PicWidthPixel;
    m_pSensor->image.Data.raw.PicWidthBytes  = image.Data.raw.PicWidthBytes;
    m_pSensor->image.Data.raw.PicHeightPixel = image.Data.raw.PicHeightPixel;

    m_pSensor->image.Type   = image.Type;
    m_pSensor->image.Layout = image.Layout;

    m_pSensor->wbGains  = ( NULL != wbGains ) ? *wbGains : defaultWbGains;
    m_pSensor->ccMatrix = ( NULL != ccMatrix ) ? *ccMatrix : defaultCcMatrix;
    m_pSensor->ccOffset = ( NULL != ccOffset ) ? *ccOffset : defaultCcOffset;
    m_pSensor->blvl     = ( NULL != blvl ) ? *blvl : defaultBlvl;

    m_pSensor->vGain  = gain;
    m_pSensor->vItime = itime;

    return true;
}

/******************************************************************************
 * previewSetup
 *****************************************************************************/
bool
CamEngineItf::previewSetup()
{
    int width = 0, height = 0;

    // get resolution
    CamEngineItf::getResolution( width, height );
    width  = std::min( width, 1920 );
    height = std::min( height, 1080 );

    switch ( m_pCamEngine->camEngineConfig.mode )
    {
        case CAM_ENGINE_MODE_SENSOR_2D:
        case CAM_ENGINE_MODE_IMAGE_PROCESSING:
        {
            // preview path = self path on camerIc master
            //for test zyc,use main path 
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathPreview;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathDisabled ;
            
            //m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
            //m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathPreview;
            
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;
            break;
        }

        case CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB:
        {
            // preview path = main path on camerIc master -> main path on camerIc slave
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathPreview;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;
            
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = defaultPathPreviewImgStab;
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;
            break;
        }

        case CAM_ENGINE_MODE_SENSOR_3D: 
        {
            // preview path = main path on camerIc master and main path on camerIc slave
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathPreview;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;
            
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = defaultPathPreview;
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;
            break;
        }

        default:
        {
            return ( false );
        }
    }

    return ( true );
}
  
/******************************************************************************
 * hasImage
 *****************************************************************************/
bool
CamEngineItf::hasImage()
{
    return ( NULL != m_pSensor->image.Data.raw.pBuffer );
}



 /******************************************************************************
 * getSensorIsiVer
 *****************************************************************************/
bool
CamEngineItf::getSensorIsiVersion(unsigned int* pVersion) 
{
    // sensor connected and driver loaded ?
    if ( NULL != m_pSensor->hSensor && (pVersion) )
    {
        // sensor isi version
        if ( RET_SUCCESS != IsiGetSensorIsiVer( m_pSensor->hSensor, pVersion ) )
        {
            return false;
        }
    }
    else
    {
		return false;
    }

    return true;
}

 bool
 CamEngineItf::getLibispIsiVersion(unsigned int* pVersion) 
 {
	 // sensor connected and driver loaded ?
	 if (  (pVersion) )
	 {
		 // libisp isi version
		 *pVersion = CONFIG_ISI_VERSION;
	 }
	 else
	 {
		 return false;
	 }
 
	 return true;
 }


 /******************************************************************************
 * getSensorIsiVer
 *****************************************************************************/
bool
CamEngineItf::getSensorTuningXmlVersion(char** pTuningXmlVersion) 
{
    // sensor connected and driver loaded ?
    if ( NULL != m_pSensor->hSensor && (pTuningXmlVersion) )
    {
        // sensor isi version
        if ( RET_SUCCESS != IsiGetSensorTuningXmlVersion( m_pSensor->hSensor, pTuningXmlVersion ) )
        {
            return false;
        }
    }
    else
    {
		return false;
    }

    return true;
}

//oyyf add
bool CamEngineItf::checkVersion(rk_cam_total_info *pCamInfo)
{
    bool ret= true;
    
	if(!pCamInfo)
		return false;
	
	//check camsys_head version   -> board_xml_parse.cpp
	unsigned int CamsysHeadVersion = pCamInfo->mCamsysVersion.head_ver;
	unsigned int CamerahalCamsysHeadVersion = CAMSYS_HEAD_VERSION;

    TRACE( CAM_API_CAMENGINE_NOTICE0,"\n\n\nCameraHal Version Check:");
    
	TRACE( CAM_API_CAMENGINE_NOTICE0,"    CamSys_Head.h version: \n"
                                     "       kernel: (0x) v%x.%x.%x\n"
                                     "       libisp_siliconimage_isp.so: (0x) v%x.%x.%x\n", 
		(CamsysHeadVersion>>16)&0xff, (CamsysHeadVersion>>8)&0xff,(CamsysHeadVersion)&0xff,
		(CamerahalCamsysHeadVersion>>16)&0xff, (CamerahalCamsysHeadVersion>>8)&0xff, (CamerahalCamsysHeadVersion)&0xff);
	
	//check camera.rk30board.so and  libisp_siliconimage_isp.so version
	unsigned int LibispVersion = CONFIG_SILICONIMAGE_LIBISP_VERSION;
	unsigned int CamerahalLibispVersion = pCamInfo->mLibIspVersion;
    
	TRACE( CAM_API_CAMENGINE_NOTICE0,"    LibIsp version: \n"
                                     "       local: (0x) v%x.%x.%x\n"
                                     "       camera.rk30board.so: (0x) v%x.%x.%x\n",
		(LibispVersion>>16)&0xff, (LibispVersion>>8)&0xff,(LibispVersion)&0xff,
		(CamerahalLibispVersion>>16)&0xff, (CamerahalLibispVersion>>8)&0xff, (CamerahalLibispVersion)&0xff);
	
    
	//check sensor.driver.so and libisp_siliconimage_isp.so version
	unsigned int sensorIsiVersion = 0;
	unsigned int LibispIsiVersion = 0;
	getSensorIsiVersion(&sensorIsiVersion);
	getLibispIsiVersion(&LibispIsiVersion);
    
	TRACE( CAM_API_CAMENGINE_NOTICE0,"    Sensor ISI version: \n"
                                     "        libisp_isi_drv_XXX: (0x) v%x.%x.%x\n"
                                     "        libisp_siliconimage_isp.so: (0x) v%x.%x.%x\n", 
		(sensorIsiVersion>>16)&0xff, (sensorIsiVersion>>8)&0xff,(sensorIsiVersion)&0xff,
		(LibispIsiVersion>>16)&0xff, (LibispIsiVersion>>8)&0xff, (LibispIsiVersion)&0xff);
	
	//check if sensor need sensor.tuning.xml
	if(m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps->SensorOutputMode == ISI_SENSOR_OUTPUT_MODE_RAW){
		//check sensor.driver.so and sensor.tuning.xml version
		char* pSensorTuningXmlVersion;
		char calibdbTuningXmlVersion[100];
		getSensorTuningXmlVersion(&pSensorTuningXmlVersion);
		memset(calibdbTuningXmlVersion, 0x00, 100);
		CamCalibDbMetaData_t MetaData;
		RESULT res = CamCalibDbGetMetaData(m_pCamEngine->calibDb.GetCalibDbHandle(), &MetaData);
		if(!res){
			snprintf(calibdbTuningXmlVersion,sizeof(calibdbTuningXmlVersion)-1, "%s_%s_%s_%s_%s", MetaData.cdate, MetaData.cname, MetaData.sname, MetaData.sid, MetaData.cversion);
			TRACE( CAM_API_CAMENGINE_NOTICE0,"    Tunning XML version:\n"
                                             "        libisp_isi_drv_XXX: (%s)\n"
                                             "        Calibdb: (%s)\n",pSensorTuningXmlVersion, calibdbTuningXmlVersion);	
		}else{
			TRACE( CAM_API_CAMENGINE_ERROR,"%s(%d) get calibdb metadata error\n", __FUNCTION__,__LINE__);
		}
	}

	//print sensor drv version
	unsigned int sensor_drv_version = pCamInfo->mLoadSensorInfo.mpI2cInfo->sensor_drv_version;
	TRACE( CAM_API_CAMENGINE_NOTICE0,"    sensor name %s: \n"
                                     "       sensor drv version: (0x) v%x.%x.%x\n",
		pCamInfo->mHardInfo.mSensorInfo.mSensorName,
		(sensor_drv_version>>16)&0xff, (sensor_drv_version>>8)&0xff, (sensor_drv_version)&0xff);

	
	if(CamsysHeadVersion != CamerahalCamsysHeadVersion )
	{
		TRACE( CAM_API_CAMENGINE_ERROR,"\n VERSION-WARNING: camera.rk30board.so and camsys head version don't match \n");
        DCT_ASSERT(0);
		ret = false;
	}
    
	if(LibispVersion != CamerahalLibispVersion)
	{
		TRACE( CAM_API_CAMENGINE_ERROR,"\n VERSION-WARNING: camera.rk30board.so and libisp_siliconimage_isp.so don't match \n");
		ret = false;
        DCT_ASSERT(0);
	}
    
	if(sensorIsiVersion != LibispIsiVersion)
	{
		TRACE( CAM_API_CAMENGINE_ERROR,"\n VERSION-WARNING: libisp_%s.so and libisp_siliconimage_isp.so don't match \n", pCamInfo->mHardInfo.mSensorInfo.mSensorName);
		ret = false;
        DCT_ASSERT(0);
	}

    TRACE( CAM_API_CAMENGINE_NOTICE0,"\n\n\n");

end:    
	return ret;
}

/******************************************************************************
 * hasSensor
 **************************/
bool
CamEngineItf::hasSensor()
{
    return ( NULL != sensorDriverFile() );
}


/******************************************************************************
 * sensorDriverFile
 *****************************************************************************/
const char *
CamEngineItf::sensorDriverFile() const
{
    if ( true != m_pCamEngine->sensorDrvFile.empty() )
    {
        return m_pCamEngine->sensorDrvFile.c_str();
    }

    return NULL;
}


/******************************************************************************
 * clearSensorDriverFile
 *****************************************************************************/
void
CamEngineItf::clearSensorDriverFile() const
{
    m_pCamEngine->sensorDrvFile = std::string();
}


/******************************************************************************
 * loadCalibrationData
 *****************************************************************************/
bool
CamEngineItf::loadCalibrationData( const char *pFileName )
{
    if ( NULL != pFileName )
    {
        // reset calibration database
        //m_pCamEngine->sensorDrvFile = std::string();
        m_pCamEngine->calibDbFile = std::string();
        m_pCamEngine->calibDb = CalibDb();

        m_pCamEngine->calibDbFile = std::string( pFileName );
        return m_pCamEngine->calibDb.CreateCalibDb( m_pCamEngine->calibDbFile.c_str() );
    }

    return false;
}

bool
CamEngineItf::loadCalibrationData( camsys_load_sensor_info* pLoadInfo)
{
    if ( NULL != pLoadInfo )
    {
        if(!(pLoadInfo->calidb.GetCalibDbHandle()))
            return false;
        
        // reset calibration database
        //m_pCamEngine->sensorDrvFile = std::string();
        m_pCamEngine->calibDbFile = std::string(pLoadInfo->mSensorXmlFile);
        m_pCamEngine->calibDb = CalibDb();
        bool ret = m_pCamEngine->calibDb.SetCalibDbHandle(pLoadInfo->calidb.GetCalibDbHandle());   
        struct sensor_calib_info *pCalibInfo = pLoadInfo->calidb.GetCalibDbInfo();
        ret &= m_pCamEngine->calibDb.SetCalibDbInfo( pCalibInfo );

        return ret;
    }

    return false;
}

/******************************************************************************
 * calibrationDataFile
 *****************************************************************************/
const char *
CamEngineItf::calibrationDataFile() const
{
    if ( true != m_pCamEngine->calibDbFile.empty() )
    {
        return m_pCamEngine->calibDbFile.c_str();
    }

    return NULL;
}


/******************************************************************************
 * calibrationDb
 *****************************************************************************/
CamCalibDbHandle_t
CamEngineItf::calibrationDbHandle() const
{
    return ( m_pCamEngine->calibDb.GetCalibDbHandle() );
}


/******************************************************************************
 * sensorName
 *****************************************************************************/
const char *
CamEngineItf::sensorName() const
{
    if ( NULL != m_pSensor->pCamDrvConfig )
    {
        return m_pSensor->pCamDrvConfig->IsiSensor.pszName;
    }

    return NULL;
}


/******************************************************************************
 * sensorRevision
 *****************************************************************************/
uint32_t
CamEngineItf::sensorRevision() const
{
    if ( NULL != m_pSensor->hSensor )
    {
        uint32_t revId;
        if ( RET_SUCCESS == IsiGetSensorRevisionIss( m_pSensor->hSensor, &revId ) )
        {
            return revId;
        }
    }

    return 0;
}


/******************************************************************************
 * isSensorConnected
 *****************************************************************************/
bool
CamEngineItf::isSensorConnected() const
{
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS == IsiCheckSensorConnectionIss( m_pSensor->hSensor ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * subSensorRevision
 *****************************************************************************/
uint32_t
CamEngineItf::subSensorRevision() const
{
    if ( NULL != m_pSensor->hSubSensor )
    {
        uint32_t revId;
        if ( RET_SUCCESS == IsiGetSensorRevisionIss( m_pSensor->hSubSensor, &revId ) )
        {
            return revId;
        }
    }

    return 0;
}


/******************************************************************************
 * isSubSensorConnected
 *****************************************************************************/
bool
CamEngineItf::isSubSensorConnected() const
{
    if ( NULL != m_pSensor->hSubSensor )
    {
        if ( RET_SUCCESS == IsiCheckSensorConnectionIss( m_pSensor->hSubSensor ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * sensorRegisterTable
 *****************************************************************************/
const IsiRegDescription_t*
CamEngineItf::sensorRegisterTable() const
{
    if ( NULL != m_pSensor->pCamDrvConfig )
    {
        return m_pSensor->pCamDrvConfig->IsiSensor.pRegisterTable;
    }

    return NULL;
}


/******************************************************************************
 * sensorCaps
 *****************************************************************************/
bool
CamEngineItf::getSensorCaps( IsiSensorCaps_t &sensorCaps ) const
{
    RESULT result;
    
    if ( NULL != m_pSensor->hSensor )
    {
        result = IsiGetCapsIss( m_pSensor->hSensor, &sensorCaps );
        if ( RET_SUCCESS != result )
        {
            if (RET_OUTOFRANGE != result) 
                TRACE( CAM_API_CAMENGINE_ERROR, "%s (IsiGetCapsIss failed)\n", __FUNCTION__ );
            return false;
        }

        return true;
    } 

    return false;
}


/******************************************************************************
 * sensorConfig
 *****************************************************************************/
void
CamEngineItf::getSensorConfig( IsiSensorConfig_t &sensorConfig ) const
{
    MEMSET( &sensorConfig, 0, sizeof( IsiSensorConfig_t ) );

    if ( NULL != m_pSensor->hSensor )
    {
        sensorConfig = m_pSensor->sensorConfig;
    }
}


/******************************************************************************
 * setSensorConfig
 *****************************************************************************/
void
CamEngineItf::setSensorConfig( const IsiSensorConfig_t &sensorConfig )
{
    m_pSensor->sensorConfig = sensorConfig;
}


/******************************************************************************
 * getIlluProfiles
 *****************************************************************************/
bool
CamEngineItf::getIlluminationProfiles( std::vector<CamIlluProfile_t *> &profiles ) const
{
    int32_t no = 0;

    // get no of available profiles
    RESULT result = CamCalibDbGetNoOfIlluminations( m_pCamEngine->calibDb.GetCalibDbHandle(), &no );
    if ( RET_SUCCESS != result )
    {
        return ( false );
    }

    // check no
    if ( !no )
    {
        return ( true );
    }

    // add preofiles to vector
    for ( int i = 0; i < no; ++i )
    {
        CamIlluProfile_t *pIlluProfile = NULL;
        result = CamCalibDbGetIlluminationByIdx( m_pCamEngine->calibDb.GetCalibDbHandle(), i, &pIlluProfile );
        if ( RET_SUCCESS == result )
        {
            profiles.push_back( pIlluProfile );
        }
    }

    return ( true );
}


/******************************************************************************
 * dumpSensorRegister
 *****************************************************************************/
bool
CamEngineItf::dumpSensorRegister( bool main_notsub, const char* szFileName ) const
{
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS == IsiDumpAllRegisters( main_notsub ? m_pSensor->hSensor : m_pSensor->hSubSensor, (const uint8_t *)szFileName ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * getSensorRegisterDescription
 *****************************************************************************/
bool
CamEngineItf::getSensorRegisterDescription( uint32_t addr, IsiRegDescription_t &description ) const
{
    MEMSET( &description, 0, sizeof( IsiRegDescription_t ) );

    if ( NULL != m_pSensor->hSensor )
    {
        if ( NULL != m_pSensor->pCamDrvConfig->IsiSensor.pRegisterTable )
        {
            const IsiRegDescription_t* ptReg = m_pSensor->pCamDrvConfig->IsiSensor.pRegisterTable;

            while (ptReg->Flags != eTableEnd)
            {
                if ( ptReg->Addr == addr)
                {
                    description = *ptReg;

                    return true;
                }
                ++ptReg;
            }
        }
    }

    return false;
}


/******************************************************************************
 * readSensorRegister
 *****************************************************************************/
bool
CamEngineItf::readSensorRegister( bool main_notsub, uint32_t addr, uint32_t &value ) const
{
    value = 0U;

    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS == IsiReadRegister( main_notsub ? m_pSensor->hSensor : m_pSensor->hSubSensor, addr, &value ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * writeSensorRegister
 *****************************************************************************/
bool
CamEngineItf::writeSensorRegister( bool main_notsub, uint32_t addr, uint32_t value ) const
{
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS == IsiWriteRegister( main_notsub ? m_pSensor->hSensor : m_pSensor->hSubSensor, addr, value ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * getScenarioMode
 *****************************************************************************/
void
CamEngineItf::getScenarioMode( CamEngineModeType_t &mode ) const
{
    mode = m_pCamEngine->camEngineConfig.mode;
}


/******************************************************************************
 * setScenarioMode
 *****************************************************************************/
void
CamEngineItf::setScenarioMode( CamEngineModeType_t &mode ) const
{
    m_pCamEngine->camEngineConfig.mode = mode;
}


/******************************************************************************
 * getSensorItf
 *****************************************************************************/
int 
CamEngineItf::getSensorItf() const
{
    return ( m_pSensor->sensorItf );
}


/******************************************************************************
 * setSensorItf
 *****************************************************************************/
void
CamEngineItf::setSensorItf( int &idx ) const
{
    m_pSensor->sensorItf = idx;
}


/******************************************************************************
 * is3DEnabled
 *****************************************************************************/
bool
CamEngineItf::is3DEnabled() const
{
    return ( CAM_ENGINE_MODE_SENSOR_3D == m_pCamEngine->camEngineConfig.mode );
}


/******************************************************************************
 * enableTestpattern
 *****************************************************************************/
void
CamEngineItf::enableTestpattern( bool enable )
{
    m_pSensor->enableTestpattern = enable;
}


/******************************************************************************
 * isTestpatternEnabled
 *****************************************************************************/
bool
CamEngineItf::isTestpatternEnabled() const
{
    return ( m_pSensor->enableTestpattern );
}


/******************************************************************************
 * enableAfps
 *****************************************************************************/
void
CamEngineItf::enableAfps( bool enable )
{
    m_pSensor->enableAfps = enable;
}

/******************************************************************************
 * isAfpsEnabled
 *****************************************************************************/
bool
CamEngineItf::isAfpsEnabled() const
{
    return m_pSensor->enableAfps;
}


/******************************************************************************
 * getFlickerPeriod
 *****************************************************************************/
bool
CamEngineItf::getFlickerPeriod( float &flickerPeriod ) const
{
    switch( m_pSensor->flickerPeriod )
    {
        case CAM_ENGINE_FLICKER_OFF  : flickerPeriod = ( 0.0000001 ); break; // this small value virtually turns of flicker avoidance
        case CAM_ENGINE_FLICKER_100HZ: flickerPeriod = ( 1.0/100.0 ); break;
        case CAM_ENGINE_FLICKER_120HZ: flickerPeriod = ( 1.0/120.0 ); break;

        default: flickerPeriod = ( 0.0000001 ); return false;
    }

    return true;
}


/******************************************************************************
 * getFlickerPeriod
 *****************************************************************************/
void
CamEngineItf::getFlickerPeriod( CamEngineFlickerPeriod_t &flickerPeriod ) const
{
    flickerPeriod = m_pSensor->flickerPeriod;
}


/******************************************************************************
 * setFlickerPeriod
 *****************************************************************************/
void
CamEngineItf::setFlickerPeriod( CamEngineFlickerPeriod_t &flickerPeriod ) const
{
    m_pSensor->flickerPeriod = flickerPeriod;
}


/******************************************************************************
 * registerBufferCb
 *****************************************************************************/
bool
CamEngineItf::registerBufferCb( CamEngineBufferCb_t fpCallback, void* BufferCbCtx )
{
    if ( NULL == m_pCamEngine->hCamEngine )
    {
        return false;
    }

    return RET_SUCCESS == CamEngineRegisterBufferCb(
            m_pCamEngine->hCamEngine, fpCallback, BufferCbCtx );
}


/******************************************************************************
 * deRegisterBufferCb
 *****************************************************************************/
bool
CamEngineItf::deRegisterBufferCb()
{
    if ( NULL == m_pCamEngine->hCamEngine )
    {
        return false;
    }

    return RET_SUCCESS == CamEngineDeRegisterBufferCb( m_pCamEngine->hCamEngine );
}


/******************************************************************************
 * getBufferCb
 *****************************************************************************/
bool
CamEngineItf::getBufferCb( CamEngineBufferCb_t* fpCallback, void** BufferCbCtx )
{
    if ( NULL == m_pCamEngine->hCamEngine )
    {
        return false;
    }

    return RET_SUCCESS == CamEngineGetBufferCb(
            m_pCamEngine->hCamEngine, fpCallback, BufferCbCtx );
}


/******************************************************************************
 * mapBuffer
 *****************************************************************************/
bool
CamEngineItf::mapBuffer( const MediaBuffer_t* pSrcBuffer, PicBufMetaData_t* pDstBuffer )
{
    if ( ( NULL == pDstBuffer ) || ( NULL == pSrcBuffer ) )
    {
        return false;
    }

    // get & check buffer meta data
    PicBufMetaData_t *pPicBufMetaData = (PicBufMetaData_t *)(pSrcBuffer->pMetaData);
    if ( NULL == pPicBufMetaData )
    {
        return false;
    }
    if ( RET_SUCCESS != PicBufIsConfigValid( pPicBufMetaData ) )
    {
        return false;
    }

    // check type, select copy functions
    switch (pPicBufMetaData->Type)
    {
        case PIC_BUF_TYPE_DATA:
        {
            return false;
        }
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        {
            return mapRawBuffer( m_pSensor->hHal, pPicBufMetaData, pDstBuffer );
        }
        case PIC_BUF_TYPE_JPEG:
        {
            return false;
        }
        case PIC_BUF_TYPE_YCbCr444:
        case PIC_BUF_TYPE_YCbCr422:
        case PIC_BUF_TYPE_YCbCr420:
        case PIC_BUF_TYPE_YCbCr32:
        {
            return mapYCbCrBuffer( m_pSensor->hHal, pPicBufMetaData, pDstBuffer );
        }
        case PIC_BUF_TYPE_RGB888:
        case PIC_BUF_TYPE_RGB32:
        {
            return false;
        }
        default:
        {
            return false;
        }
    }
}


/******************************************************************************
 * unmapBuffer
 *****************************************************************************/
bool
CamEngineItf::unmapBuffer( PicBufMetaData_t* pBuffer )
{
    if ( NULL == pBuffer )
    {
        return false;
    }

    // check buffer meta data
    if ( RET_SUCCESS != PicBufIsConfigValid( pBuffer ) )
    {
        return false;
    }

    // check type, select copy functions
    switch (pBuffer->Type)
    {
        case PIC_BUF_TYPE_DATA:
        {
            return false;
        }
        case PIC_BUF_TYPE_RAW8:
        case PIC_BUF_TYPE_RAW16:
        {
            return unmapRawBuffer( m_pSensor->hHal, pBuffer );
        }
        case PIC_BUF_TYPE_JPEG:
        {
            return false;
        }
        case PIC_BUF_TYPE_YCbCr444:
        case PIC_BUF_TYPE_YCbCr422:
        case PIC_BUF_TYPE_YCbCr420:
        case PIC_BUF_TYPE_YCbCr32:
        {
            return unmapYCbCrBuffer( m_pSensor->hHal, pBuffer );
        }
        case PIC_BUF_TYPE_RGB888:
        case PIC_BUF_TYPE_RGB32:
        {
            return false;
        }
        default:
        {
            return false;
        }
    }
}


/******************************************************************************
 * reset
 *****************************************************************************/
bool
CamEngineItf::reset()
{
    if ( Invalid != m_pCamEngine->state )
    {
        disconnect();
    }

    // reset data path
    //for test zyc ,use main path
    m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathPreview;
    m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathDisabled ;

    //m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
    //m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = defaultPathPreview;
    
    m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = defaultPathDisabled;
    m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = defaultPathDisabled;

    // reset sensor
    if ( ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
           ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
           ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
           &&
           ( NULL != m_pSensor->hSensor ) )
    {
        // default sensor config
        MEMCPY( &m_pSensor->sensorConfig, m_pSensor->pCamDrvConfig->IsiSensor.pIsiSensorCaps, sizeof( IsiSensorCaps_t ) );
    }
    else
    {
        // clear sensor config
        MEMSET( &m_pSensor->sensorConfig, 0, sizeof( IsiSensorCaps_t ) );
    }

    return true;
}


/******************************************************************************
 * connect
 *****************************************************************************/
bool
CamEngineItf::connect()
{
    if ( Invalid != m_pCamEngine->state )
    {
        disconnect();
    }

    if ( ( CAM_ENGINE_MODE_INVALID >= m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_MAX     <= m_pCamEngine->camEngineConfig.mode ) )
    {
        return false;
    }

    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
    {
        if( true != setupSensor() )
        {
            return false;
        }
		setupOTPInfo(); //add by zyl
    }

    if( true != setupCamerIC() )
    {
        return false;
    }

    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
    {
        //have to start mipi phy before sensor streaming
        if (m_pCamEngine->camEngineConfig.data.sensor.ifSelect == CAMERIC_ITF_SELECT_MIPI)
            restartMipiDrv();

        startSensor();
    }

    m_pCamEngine->state = Idle;

    // try to start AEC module
    (void)startAec();

    // start AWB module
   (void)startAwb( CAM_ENGINE_AWB_MODE_AUTO, 1, BOOL_TRUE );
    //(void)startAwb( CAM_ENGINE_AWB_MODE_MANUAL_CT, 5666, BOOL_TRUE );//test

    // start ADPF module
    (void)startAdpf();

    // try to start ADPCC module
    (void)startAdpcc();

    // try to start AVS module
    if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode )
    {
        // start AVS module
        (void)startAvs();
    }

#if 1
	//set color convation to full range 
	CamerIcColorConversionRange_t YConvRange = CAMERIC_ISP_CCONV_RANGE_FULL_RANGE;
	CamerIcColorConversionRange_t CrConvRange = CAMERIC_ISP_CCONV_RANGE_FULL_RANGE;
	ColorConversionSetRange(YConvRange, CrConvRange);

	CamerIc3x3Matrix_t CConvCoefficients;
	CConvCoefficients.Coeff[0] = 0x0026;
	CConvCoefficients.Coeff[1] = 0x004B;
	CConvCoefficients.Coeff[2] = 0x000F; 
	CConvCoefficients.Coeff[3] = 0x01EA;
	CConvCoefficients.Coeff[4] = 0x01D6;
	CConvCoefficients.Coeff[5] = 0x0040;
	CConvCoefficients.Coeff[6] = 0x0040;
	CConvCoefficients.Coeff[7] = 0x01CA;
	CConvCoefficients.Coeff[8] = 0x01F6;

	ColorConversionSetCoefficients(&CConvCoefficients);
#endif
//	CamCalibGammaOut_t *pgamm;
//	CamCalibDbGetGammaOut( m_pCamEngine->calibDb.GetCalibDbHandle(), &pgamm);


//	filterSetLevel(2, 6);
    return true;
}


/******************************************************************************
 * disconnect
 *****************************************************************************/
void
CamEngineItf::disconnect()
{
    // stop AEC module
    (void)stopAec();

    // stop AWB module
    (void)stopAwb();

    // stop AF module
    (void)stopAf();

    // stop ADPF module
    (void)stopAdpf();

    // stop ADPCC module
    (void)stopAdpcc();

    // stop AVS module
    (void)stopAvs();

    // check cam-engine state
    if ( Invalid == m_pCamEngine->state )
    {
        return;
    }

    // stop cam-engine state
    if ( Idle != m_pCamEngine->state )
    {
        stop();
    }

    DCT_ASSERT( Idle == m_pCamEngine->state );

    // shutdown cameric
    doneCamerIC();

    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
    {
        // shutdown sensor
        doneSensor();
    }

    m_pCamEngine->state = Invalid;
}


/******************************************************************************
 * changeResolution
 *****************************************************************************/
bool CamEngineItf::changeResolution
(
    uint32_t    resolution,
    bool        restoreState
)
{
    
    uint8_t numFramesToSkip = 0;


    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }
    // check is change framerate only or change resolution
    if (m_pSensor->enableAfps == true) {
        if ( (ISI_RES_W_GET(m_pSensor->sensorConfig.Resolution)== ISI_RES_W_GET(resolution)) &&
            (ISI_RES_H_GET(m_pSensor->sensorConfig.Resolution)== ISI_RES_H_GET(resolution)) ) { 
            // change sensor framerate
            if ( NULL != m_pSensor->hSensor )
            {
                if ( RET_SUCCESS != IsiChangeSensorResolutionIss( m_pSensor->hSensor, resolution, &numFramesToSkip ) )
                {
                    TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't change resolution of sensor)\n", __FUNCTION__ );
                }
                else
                {
                    m_pSensor->sensorConfig.Resolution = resolution;
    				if (m_pcbItfAfpsResChange){
    					CtxCbResChange_t *pctx = (CtxCbResChange_t *)m_ctxCbResChange;
    					pctx->res = resolution;
    			        m_pcbItfAfpsResChange( m_ctxCbResChange );
                	}
                }            
            } else {
                TRACE(CAM_API_CAMENGINE_ERROR, "m_pSensor->hSensor is NULL\n");
            }
            return true;

        }
    }
  
    // remember old state and stop if required
    State oldState = m_pCamEngine->state;
    if ( Idle != m_pCamEngine->state )
    {
        stop();
    }

    DCT_ASSERT( Idle == m_pCamEngine->state );

    // turn off sensor streaming
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sensor)\n", __FUNCTION__ );
        }
    }

    // turn off sub sensor streaming
    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sub sensor)\n", __FUNCTION__ );
        }
    }

    // change sensor resolution
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiChangeSensorResolutionIss( m_pSensor->hSensor, resolution, &numFramesToSkip ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't change resolution of sensor)\n", __FUNCTION__ );
        }
        else
        {
            m_pSensor->sensorConfig.Resolution = resolution;
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        uint8_t dummy; // frames to skip are taken from leading sensor
        if ( RET_SUCCESS != IsiChangeSensorResolutionIss( m_pSensor->hSubSensor, resolution, &dummy ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't change resolution of sub sensor)\n", __FUNCTION__ );
        }
    }

    // change resolution of input acquisition
    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
    {
        CamEngineWindow_t acqWindow         = { 0, 0, 0, 0};
        CamEngineWindow_t outWindow         = { 0, 0, 0, 0};
        CamEngineWindow_t isWindow          = { 0, 0, 0, 0};
        CamEngineWindow_t isCroppingWindow  = { 0, 0, 0, 0};

        isiCapValue( acqWindow       , m_pSensor->sensorConfig.Resolution );
        isiCapValue( outWindow       , m_pSensor->sensorConfig.Resolution );
        isiCapValue( isWindow        , m_pSensor->sensorConfig.Resolution );

        if(isSOCSensor()){
            acqWindow.width *=2;
            if(getBusWidth() == ISI_BUSWIDTH_12BIT){
                outWindow.width *=2;
                isWindow.width  *=2;
            }

        }
        // image stab window for slave
        if ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode )
        {
            isCroppingWindow.hOffset = ((isWindow.width  - 1280u) >> 1);
            isCroppingWindow.vOffset = ((isWindow.height -  720u) >> 1);
            isCroppingWindow.width   = 1280u;
            isCroppingWindow.height  = 720u; 
        }
        else
        {
            isCroppingWindow = isWindow;
        }

        if ( RET_SUCCESS != CamEngineSetAcqResolution( m_pCamEngine->hCamEngine,
                                acqWindow, outWindow, isWindow, isCroppingWindow, numFramesToSkip ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't set the resolution of the input acquisition)\n", __FUNCTION__ );
        }
    }


    //have to start mipi phy before sensor streaming
    if (m_pCamEngine->camEngineConfig.data.sensor.ifSelect == CAMERIC_ITF_SELECT_MIPI)
        restartMipiDrv();


    // turn on sensor streaming
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sub sensor)\n", __FUNCTION__ );
        }
    }

    // restore old state
    if (restoreState)
    {
        switch( oldState )
        {
            case Running:
                start();
                break;

            case Paused:
                start();
                pause();
                break;

            default:
                break;
        }
    }

    // interface callback
    if (m_pcbItfAfpsResChange)
    {
        CtxCbResChange_t *pctx = (CtxCbResChange_t *)m_ctxCbResChange;

        pctx->res = resolution;
        m_pcbItfAfpsResChange( m_ctxCbResChange );
    }

    return true;
}


/******************************************************************************
 * changeSensorItf
 *****************************************************************************/
bool
CamEngineItf::changeSensorItf( bool restoreState )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    // remember old state and stop if required
    State oldState = m_pCamEngine->state;
    if ( Idle != m_pCamEngine->state )
    {
        stop();
    }

    DCT_ASSERT( Idle == m_pCamEngine->state );

    // turn off sensor streaming
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sub sensor)\n", __FUNCTION__ );
        }
    }

    // turn on sensor streaming
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sub sensor)\n", __FUNCTION__ );
        }
    }

    // restore old state
    if (restoreState)
    {
        switch( oldState )
        {
            case Running:
                start();
                break;

            case Paused:
                start();
                pause();
                break;

            default:
                break;
        }
    }

    return true;
}


/******************************************************************************
 * changeEcm
 *****************************************************************************/
bool
CamEngineItf::changeEcm( bool restoreState )
{
    uint8_t numFramesToSkip = 0;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    // remember old state and stop if required
    State oldState = m_pCamEngine->state;
    if ( Idle != m_pCamEngine->state )
    {
        stop();
    }

    DCT_ASSERT( Idle == m_pCamEngine->state );

    // turn off sensor streaming
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sub sensor)\n", __FUNCTION__ );
        }
    }

    // change ECM parameters
    if ( ( CAM_ENGINE_MODE_SENSOR_2D         == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB == m_pCamEngine->camEngineConfig.mode ) ||
         ( CAM_ENGINE_MODE_SENSOR_3D         == m_pCamEngine->camEngineConfig.mode ) )
    {
        if ( RET_SUCCESS != CamEngineSetEcm( m_pCamEngine->hCamEngine,
                                             m_pSensor->flickerPeriod,
                                             m_pSensor->enableAfps ? BOOL_TRUE : BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't set ECM parameters)\n", __FUNCTION__ );
        }
    }

    // turn on sensor streaming
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sub sensor)\n", __FUNCTION__ );
        }
    }

    // restore old state
    if (restoreState)
    {
        switch( oldState )
        {
            case Running:
                start();
                break;

            case Paused:
                start();
                pause();
                break;

            default:
                break;
        }
    }

    return true;
}

/******************************************************************************
 * start
 *****************************************************************************/
bool
CamEngineItf::start( uint32_t frames )
{
    osMutexLock( &m_pCamEngine->m_syncLock );

    //restart mipi ?
   // if (m_pCamEngine->camEngineConfig.data.sensor.ifSelect == CAMERIC_ITF_SELECT_MIPI)
   //     restartMipiDrv();

    switch ( m_pCamEngine->state )
    {
        case CamEngineItf::Idle:
        case CamEngineItf::Paused:
        {
            struct StartThread : public android::Thread
            {
                StartThread (CamEngineHolder *camEngine, int32_t frames) : Thread(false),m_camEngine (camEngine), m_frames (frames) {};

                virtual bool threadLoop()
                {
                    RESULT result = CamEngineStartStreaming( m_camEngine->hCamEngine, m_frames );
                    if ( RET_PENDING == result )
                    {
                        if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventStartStreaming ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                        {
                            TRACE( CAM_API_CAMENGINE_ERROR,
                                    "%s (start streaming timed out)\n", __FUNCTION__ );
                        }
                    }
                    else if ( RET_SUCCESS != result )
                    {
                        TRACE( CAM_API_CAMENGINE_ERROR,
                                "%s (can't start streaming)\n", __FUNCTION__ );
                    }
                    return false;
                };
                CamEngineHolder *m_camEngine;
                int32_t         m_frames;
            };
            android::sp<StartThread> pstartThread(new StartThread( m_pCamEngine, frames ));
            pstartThread->run("startThread");
            pstartThread->join();
            pstartThread.clear();
            m_pCamEngine->state = Running;
            break;
        }
        default:
            break;
    }

    osMutexUnlock( &m_pCamEngine->m_syncLock );

    return Running == m_pCamEngine->state ? true : false;
}


/******************************************************************************
 * pause
 *****************************************************************************/
bool
CamEngineItf::pause()
{
    osMutexLock( &m_pCamEngine->m_syncLock );

    switch ( m_pCamEngine->state )
    {
        case CamEngineItf::Running:
        {
            struct StopThread : public Thread
            {
                StopThread (CamEngineHolder *camEngine) : m_camEngine (camEngine) {};

                virtual bool threadLoop()
                {
                    RESULT result = CamEngineStopStreaming( m_camEngine->hCamEngine );
                    if ( RET_PENDING == result )
                    {
                        if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventStopStreaming ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                        {
                            TRACE( CAM_API_CAMENGINE_ERROR,
                                    "%s (stop streaming timed out)\n", __FUNCTION__ );
                        }
                    }
                    else if ( RET_SUCCESS != result )
                    {
                        TRACE( CAM_API_CAMENGINE_ERROR,
                                "%s (can't stop streaming)\n", __FUNCTION__ );
                    }
                    return false;
                };

                CamEngineHolder *m_camEngine;
            } ;
            android::sp<StopThread> pstopThread(new StopThread( m_pCamEngine ));
            pstopThread->run("stopThread");
            pstopThread->join();
            pstopThread.clear();
            m_pCamEngine->state = Paused;
            break;
        }
        default:
            break;
    }

    osMutexUnlock( &m_pCamEngine->m_syncLock );

    return Paused == m_pCamEngine->state ? true : false;
}


/******************************************************************************
 * stop
 *****************************************************************************/
bool
CamEngineItf::stop()
{
    osMutexLock( &m_pCamEngine->m_syncLock );

    switch ( m_pCamEngine->state )
    {
        case CamEngineItf::Running:
        {
            struct StopThread : public Thread
            {
                StopThread (CamEngineHolder *camEngine) : m_camEngine (camEngine) {};

                virtual bool threadLoop()

                {
                    RESULT result = CamEngineStopStreaming( m_camEngine->hCamEngine );
                    if ( RET_PENDING == result )
                    {
                        if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventStopStreaming ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                        {
                            TRACE( CAM_API_CAMENGINE_ERROR,
                                    "%s (stop streaming timed out)\n", __FUNCTION__ );
                        }
                        
                    }
                    else if ( RET_SUCCESS != result )
                    {
                        TRACE( CAM_API_CAMENGINE_ERROR,
                                "%s (can't stop streaming)\n", __FUNCTION__ );
                    }
                    TRACE(CAM_API_CAMENGINE_INFO,"%s(%d):stop success",__FUNCTION__,__LINE__);
                    return false;
                };

                CamEngineHolder *m_camEngine;
            } ;
            android::sp<StopThread> pstopThread(new StopThread( m_pCamEngine ));
			TRACE( CAM_API_CAMENGINE_INFO ,
                                "%s (pstopThread->run())\n", __FUNCTION__ );
            pstopThread->run("stopThread");
            pstopThread->join();
            pstopThread.clear();
            m_pCamEngine->state = Idle;
            
            break;
        }
        case CamEngineItf::Paused:
        {
			TRACE( CAM_API_CAMENGINE_ERROR,
                                "%s (CamEngineItf::Paused:)\n", __FUNCTION__ );
            m_pCamEngine->state = Idle;
            break;
        }
        default:
            break;
    }

    osMutexUnlock( &m_pCamEngine->m_syncLock );

    return Idle == m_pCamEngine->state ? true : false;
}


/******************************************************************************
 * getResolution
 *****************************************************************************/
bool
CamEngineItf::getResolution( int &width, int &height ) const
{
    if ( NULL != m_pSensor->hSensor )
    {
        CamEngineWindow_t resolution = { 0, 0, 0, 0};
        isiCapValue( resolution, m_pSensor->sensorConfig.Resolution );

        width  = resolution.width;
        height = resolution.height;
        return true;
    }
    else if ( NULL != m_pSensor->image.Data.raw.pBuffer )
    {
        width  = m_pSensor->image.Data.raw.PicWidthPixel;
        height = m_pSensor->image.Data.raw.PicHeightPixel;
        return true;
    }

    return false;
}


/******************************************************************************
 * getResolution
 *****************************************************************************/
bool
CamEngineItf::getResolution( uint32_t &resolution ) const
{
    if ( NULL != m_pSensor->hSensor )
    {
        resolution = m_pSensor->sensorConfig.Resolution;
    }

    return false;
}


/******************************************************************************
 * getGain
 *****************************************************************************/
bool
CamEngineItf::getGain( float &gain ) const
{
    // sensor connected and driver loaded ?
    if ( NULL != m_pSensor->hSensor )
    {
        // sensor mode 
        if ( RET_SUCCESS != IsiGetGainIss( m_pSensor->hSensor, &gain ) ) 
        {
            return false;
        }
    }
    else
    {
        // assumption: image mode
        gain = m_pSensor->vGain;
    }

    return true;
}


/******************************************************************************
 * getGainLimits
 *****************************************************************************/
bool
CamEngineItf::getGainLimits( float &minGain, float &maxGain, float &step ) const
{
    // sensor connected and driver loaded ?
    if ( NULL != m_pSensor->hSensor )
    {
        // sensor mode 
        if ( RET_SUCCESS != IsiGetGainLimitsIss( m_pSensor->hSensor, &minGain, &maxGain ) )
        {
            return false;
        }
        if ( RET_SUCCESS != IsiGetGainIncrementIss( m_pSensor->hSensor, &step ) )
        {
            return false;
        }
    }
    else
    {
        // assumption: image mode
        minGain = m_pSensor->vGain;
        maxGain = m_pSensor->vGain;
        step = 0;
    }

    return true;
}


/******************************************************************************
 * setGain
 *****************************************************************************/
bool
CamEngineItf::setGain( float newGain, float &setGain )
{
    bool ok = false;

    if ( NULL != m_pSensor->hSensor )
    {
        ok = ( RET_SUCCESS == IsiSetGainIss( m_pSensor->hSensor, newGain, &setGain ) );
    }

    if ( ok && ( NULL != m_pSensor->hSubSensor ) )
    {
        ok = ( RET_SUCCESS == IsiSetGainIss( m_pSensor->hSubSensor, newGain, &setGain ) );
    }

    return ok;
}


/******************************************************************************
 * getIntegrationTime
 *****************************************************************************/
bool
CamEngineItf::getIntegrationTime( float &time ) const
{
    // sensor connected and driver loaded ?
    if ( NULL != m_pSensor->hSensor )
    {
        // sensor mode 
        if ( RET_SUCCESS != IsiGetIntegrationTimeIss( m_pSensor->hSensor, &time ) )
        {
            return false;
        }
    }
    else
    {
        // assumption: image mode
        time = m_pSensor->vItime;
    }

    return true;
}


/******************************************************************************
 * getIntegrationTimeLimits
 *****************************************************************************/
bool
CamEngineItf::getIntegrationTimeLimits( float &minTime, float &maxTime, float &step ) const
{
    // sensor connected and driver loaded ?
    if ( NULL != m_pSensor->hSensor )
    {
        // sensor mode 
        if ( RET_SUCCESS != IsiGetIntegrationTimeLimitsIss( m_pSensor->hSensor, &minTime, &maxTime ) )
        {
            return false;
        }
        if ( RET_SUCCESS != IsiGetIntegrationTimeIncrementIss( m_pSensor->hSensor, &step ) )
        {
            return false;
        }
    }
    else
    {
        // assumption: image mode
        minTime = m_pSensor->vItime;
        maxTime = m_pSensor->vItime;
        step = 0;
    }

    return true;
}


/******************************************************************************
 * setIntegrationTime
 *****************************************************************************/
bool
CamEngineItf::setIntegrationTime( float newTime, float &setTime )
{
    bool ok = false;

    if ( NULL != m_pSensor->hSensor )
    {
        ok = ( RET_SUCCESS == IsiSetIntegrationTimeIss( m_pSensor->hSensor, newTime, &setTime ) );
    }

    if ( ok && ( NULL != m_pSensor->hSubSensor ) )
    {
        ok = ( RET_SUCCESS == IsiSetIntegrationTimeIss( m_pSensor->hSubSensor, newTime, &setTime ) );
    }

    return ok;
}


/******************************************************************************
 * setExposure
 *****************************************************************************/
bool
CamEngineItf::setExposure( float newExp, float &setExp )
{
    bool ok = false;

    //TODO: implement

    return ok;
}


/******************************************************************************
 * getFocus
 *****************************************************************************/
bool
CamEngineItf::getFocus( uint32_t &focus ) const
{
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS == IsiMdiFocusGet( m_pSensor->hSensor, &focus ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * getFocusLimits
 *****************************************************************************/
bool
CamEngineItf::getFocusLimits( uint32_t &minFocus, uint32_t &maxFocus ) const
{
    if ( NULL != m_pSensor->hSensor )
    {
        minFocus = m_pSensor->minFocus;
        maxFocus = m_pSensor->maxFocus;

        return true;
    }

    return false;
}


/******************************************************************************
 * setFocus
 *****************************************************************************/
bool
CamEngineItf::setFocus( uint32_t focus )
{
    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS == IsiMdiFocusSet( m_pSensor->hSensor, focus ) )
        {
            return true;
        }
    }

    return false;
}


/******************************************************************************
 * getPathConfig
 *****************************************************************************/
bool
CamEngineItf::getPathConfig( const CamEngineChainIdx_t idx, CamEnginePathType_t path, CamEnginePathConfig_t &pathConfig ) const
{
    switch (idx)
    {
        case CHAIN_MASTER:
        {
            switch (path)
            {
                case CAM_ENGINE_PATH_MAIN:
                {
                    pathConfig = m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN];
                    return true;
                }
                case CAM_ENGINE_PATH_SELF:
                {
                    pathConfig = m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF];
                    return true;
                }
                default:
                {
                    MEMSET( &pathConfig, 0, sizeof( CamEnginePathConfig_t ) );
                }
            }
            break;
        }
        case CHAIN_SLAVE:
        {
            switch (path)
            {
                case CAM_ENGINE_PATH_MAIN:
                {
                    pathConfig = m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN];
                    return true;
                }
                case CAM_ENGINE_PATH_SELF:
                {
                    pathConfig = m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF];
                    return true;
                }
                default:
                {
                    MEMSET( &pathConfig, 0, sizeof( CamEnginePathConfig_t ) );
                }
            }
            break;
        }
        default:
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (invalid chain index)\n", __FUNCTION__ );
            return false;
        }
    }

    return false;
}


/******************************************************************************
 * setPathConfig
 *****************************************************************************/
bool CamEngineItf::setPathConfig
(
    const CamEngineChainIdx_t       idx,
    const CamEnginePathConfig_t&    mpConfig,
    const CamEnginePathConfig_t&    spConfig
)
{
    if ( Invalid != m_pCamEngine->state )
    {

        if ( RET_SUCCESS != CamEngineSetPathConfig( m_pCamEngine->hCamEngine, idx, &mpConfig, &spConfig ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR,
                    "%s (can't configure the output path of the cam-engine)\n", __FUNCTION__ );

            return false;
        }
    }

    switch ( idx )
    {
        case CHAIN_MASTER:
        {
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_MAIN] = mpConfig;
            m_pCamEngine->camEngineConfig.pathConfigMaster[CAMERIC_MI_PATH_SELF] = spConfig;
            break;
        }
        case CHAIN_SLAVE:
        {
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_MAIN] = mpConfig;
            m_pCamEngine->camEngineConfig.pathConfigSlave[CAMERIC_MI_PATH_SELF] = spConfig;
            break;
        }
        default:
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (invalid chain index)\n", __FUNCTION__ );

            return false;
        }
    }



    return true;
}


/******************************************************************************
 * searchAndLock
 *****************************************************************************/
bool
CamEngineItf::searchAndLock( CamEngineLockType_t locks )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

#if 0
    struct LockThread : public QThread
    {
        LockThread (CamEngineHolder *camEngine, CamEngineLockType_t locks) : m_camEngine (camEngine), m_locks (locks) {};

        virtual void run()
        {
            RESULT result = CamEngineSearchAndLock( m_camEngine->hCamEngine, m_locks );
            if ( RET_PENDING == result )
            {
                if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventAcquireLock ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                {
                    TRACE( CAM_API_CAMENGINE_ERROR,
                            "%s (acquire lock timed out)\n", __FUNCTION__ );
                    exit( RET_CANCELED );
                }
            }
            else if ( ( RET_SUCCESS != result ) && ( RET_NOTSUPP != result ) )
            {
                TRACE( CAM_API_CAMENGINE_ERROR,
                        "%s (can't acquire lock)\n", __FUNCTION__ );
                exit( result );
            }
        };

        CamEngineHolder     *m_camEngine;
        CamEngineLockType_t  m_locks;
    } lockThread( m_pCamEngine, locks );

    QEventLoop loop;
    int result = 1;
    if ( true == QObject::connect( &lockThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
    {
        lockThread.start();
        result = loop.exec();
    }

#else

    struct LockThread : public Thread
    {
        LockThread (CamEngineHolder *camEngine, CamEngineLockType_t locks) : m_camEngine (camEngine), m_locks (locks) {};

        virtual bool threadLoop()
        {
            RESULT result = CamEngineSearchAndLock( m_camEngine->hCamEngine, m_locks );
            if ( RET_PENDING == result )
            {
                if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventAcquireLock ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                {
                    TRACE( CAM_API_CAMENGINE_ERROR,
                            "%s (acquire lock timed out)\n", __FUNCTION__ );
                   // exit( RET_CANCELED );
                }
            }
            else if ( ( RET_SUCCESS != result ) && ( RET_NOTSUPP != result ) )
            {
                TRACE( CAM_API_CAMENGINE_ERROR,
                        "%s (can't acquire lock)\n", __FUNCTION__ );
              //  exit( result );
            }
            return false;
        };

        CamEngineHolder     *m_camEngine;
        CamEngineLockType_t  m_locks;
    } lockThread( m_pCamEngine, locks );

    //QEventLoop loop;
    int result = 0;
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
    lockThread.run("lockThread");
    lockThread.join();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);

#endif
    return (0 == result) ? RET_SUCCESS : RET_FAILURE;
}


/******************************************************************************
 * unlock
 *****************************************************************************/
bool
CamEngineItf::unlock( CamEngineLockType_t locks )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

#if 0
    struct UnlockThread : public QThread
    {
        UnlockThread (CamEngineHolder *camEngine, CamEngineLockType_t locks) : m_camEngine (camEngine), m_locks (locks) {};

        virtual void run()
        {
            RESULT result = CamEngineUnlock( m_camEngine->hCamEngine, m_locks );
            if ( RET_PENDING == result )
            {
                if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventReleaseLock ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                {
                    TRACE( CAM_API_CAMENGINE_ERROR,
                            "%s (release lock timed out)\n", __FUNCTION__ );
                    exit( RET_CANCELED );
                }
            }
            else if ( ( RET_SUCCESS != result ) && ( RET_NOTSUPP != result ) )
            {
                TRACE( CAM_API_CAMENGINE_ERROR,
                        "%s (can't release lock)\n", __FUNCTION__ );
                exit( result );
            }
        };

        CamEngineHolder     *m_camEngine;
        CamEngineLockType_t  m_locks;
    } unlockThread( m_pCamEngine, locks );

    QEventLoop loop;
    int result = 1;
    if ( true == QObject::connect( &unlockThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
    {
        unlockThread.start();
        result = loop.exec();
    }
#else

    struct UnlockThread : public Thread
    {
        UnlockThread (CamEngineHolder *camEngine, CamEngineLockType_t locks) : m_camEngine (camEngine), m_locks (locks) {};

        virtual bool threadLoop()
        {
            RESULT result = CamEngineUnlock( m_camEngine->hCamEngine, m_locks );
            if ( RET_PENDING == result )
            {
                if ( OSLAYER_OK != osEventWait( &m_camEngine->m_eventReleaseLock ) )//, CAM_API_CAMENGINE_TIMEOUT ) )
                {
                    TRACE( CAM_API_CAMENGINE_ERROR,
                            "%s (release lock timed out)\n", __FUNCTION__ );
                  //  exit( RET_CANCELED );
                }
            }
            else if ( ( RET_SUCCESS != result ) && ( RET_NOTSUPP != result ) )
            {
                TRACE( CAM_API_CAMENGINE_ERROR,
                        "%s (can't release lock)\n", __FUNCTION__ );
                //exit( result );
            }
            return false;
        };

        CamEngineHolder     *m_camEngine;
        CamEngineLockType_t  m_locks;
    } unlockThread( m_pCamEngine, locks );

    int result = 0;
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
    unlockThread.run("unlockThread");
    unlockThread.join();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
#endif
    return (0 == result) ? RET_SUCCESS : RET_FAILURE;
}


/******************************************************************************
 * startAec
 *****************************************************************************/
bool
CamEngineItf::startAec()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecStart( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * stopAec
 *****************************************************************************/
bool
CamEngineItf::stopAec()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecStop( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * resetAec
 *****************************************************************************/
bool
CamEngineItf::resetAec()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecReset( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}

/******************************************************************************
 * lock3a
 *****************************************************************************/
bool
CamEngineItf::lock3a ( CamEngine3aLock_t lock )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngine3aLock( m_pCamEngine->hCamEngine, lock ) )
    {
        return true;
    }
    return false;
}
/******************************************************************************
 * unlock3a
 *****************************************************************************/
bool
CamEngineItf::unlock3a ( CamEngine3aLock_t unlock )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngine3aUnLock( m_pCamEngine->hCamEngine, unlock ) )
    {
        return true;
    }
    return false;
}
/******************************************************************************
 * configureAec
 *****************************************************************************/
bool
CamEngineItf::configureAec
(
    const CamEngineAecSemMode_t &mode,
    const float                 setPoint,
    const float                 clmTolerance,
    const float                 dampOver,
    const float                 dampUnder
)
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecConfigure( m_pCamEngine->hCamEngine,
                                                mode, 
                                                setPoint,
                                                clmTolerance,
                                                CAM_ENGINE_AEC_DAMPING_MODE_STILL_IMAGE,
                                                dampOver,
                                                dampUnder,
                                                3*dampOver,
                                                3*dampUnder) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * isAecEnabled
 *****************************************************************************/
bool
CamEngineItf::isAecEnabled()
{
    CamEngineAecSemMode_t   mode;
    float                   setPoint;
    float                   clmTolerance;
    float                   dampOver;
    float                   dampUnder;
    
    CamEngineAecDampingMode_t DampingMode;
    float                     DampOverVideo;
    float                     DampUnderVideo;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t running = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAecStatus( m_pCamEngine->hCamEngine, 
                                                &running,
                                                &mode,
                                                &setPoint,
                                                &clmTolerance,
                                                &DampingMode,
                                                &dampOver,
                                                &dampUnder,
                                                &DampOverVideo,
                                                &DampUnderVideo) )
    {
        return ( (BOOL_TRUE == running) ? true : false );
    }
    
    return false;
}


/******************************************************************************
 * getAecStatus
 *****************************************************************************/
bool CamEngineItf::getAecStatus
(
    bool                    &enabled,
    CamEngineAecSemMode_t   &mode,
    float                   &setPoint,
    float                   &clmTolerance,
    float                   &dampOver,
    float                   &dampUnder
)
{
    CamEngineAecDampingMode_t DampingMode;
    float                     DampOverVideo;
    float                     DampUnderVideo;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t running = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAecStatus( m_pCamEngine->hCamEngine, 
                                                &running,
                                                &mode,
                                                &setPoint,
                                                &clmTolerance,
                                                &DampingMode,
                                                &dampOver,
                                                &dampUnder,
                                                &DampOverVideo,
                                                &DampUnderVideo) )
    {
        enabled = (BOOL_TRUE == running) ? true : false;
        return true;
    }

    return false;
}


/******************************************************************************
 * getAecHistogram
 *****************************************************************************/
bool
CamEngineItf::getAecHistogram( CamEngineAecHistBins_t &histogram ) const
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecGetHistogram( m_pCamEngine->hCamEngine, &histogram ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * getAecLuminance
 *****************************************************************************/
bool
CamEngineItf::getAecLuminance( CamEngineAecMeanLuma_t &luma ) const
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecGetLuminance( m_pCamEngine->hCamEngine, &luma ) )
    {
        return true;
    }

    return false;
}

float
CamEngineItf::getAecMeanLuminance() const
{
    CamEngineAecMeanLuma_t luma;
    float meanLum = 0;
    if ( Invalid == m_pCamEngine->state )
    {
        return -1;
    }

    if ( RET_SUCCESS == CamEngineAecGetLuminance( m_pCamEngine->hCamEngine, &luma ) )
    {
        for(int i = 0;i<CAM_ENGINE_AEC_EXP_GRID_ITEMS;i++){
            meanLum += luma[i]; 
        }

        meanLum = meanLum/CAM_ENGINE_AEC_EXP_GRID_ITEMS;
        return meanLum;
    }

    return -1;

}

/******************************************************************************
 * getAecObjectRegion
 *****************************************************************************/
bool
CamEngineItf::getAecObjectRegion( CamEngineAecMeanLuma_t &objectRegion ) const
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAecGetObjectRegion( m_pCamEngine->hCamEngine, &objectRegion ) )
    {
        return true;
    }

    return false;
}
/******************************************************************************
 * getAecMeasureWindow
 *****************************************************************************/
bool
CamEngineItf::getAecMeasureWindow(CamEngineWindow_t *measureWin, CamEngineWindow_t *grid )
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAecGetMeasuringWindow( m_pCamEngine->hCamEngine, measureWin, grid);

    return (result == RET_SUCCESS)?true:false;

}
/******************************************************************************
 * setAecMeasureWindow
 *****************************************************************************/
bool
CamEngineItf::setAecHistMeasureWinAndMode( 
    int16_t x,
    int16_t y,
    uint16_t width,
    uint16_t height,
    CamEngineAecHistMeasureMode_t mode)
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAecHistSetMeasureWinAndMode( m_pCamEngine->hCamEngine,
        x, y,width,height,mode);

    return (result == RET_SUCCESS)?true:false;

}


/******************************************************************************
 * isAfAvailable
 *****************************************************************************/
bool
CamEngineItf::isAfAvailable( bool &available )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t avail = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAfAvailable( m_pCamEngine->hCamEngine, &avail ) )
    {
        available = (avail == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}


/******************************************************************************
 * startAfContinous
 *****************************************************************************/
bool
CamEngineItf::startAfContinous( const CamEngineAfSearchAlgorithm_t &searchAlgorithm )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAfStart( m_pCamEngine->hCamEngine, searchAlgorithm ) )
    {
        return true;
    }

    return false;

}


/******************************************************************************
 * startAfOneshot
 *****************************************************************************/
bool
CamEngineItf::startAfOneShot( const CamEngineAfSearchAlgorithm_t &searchAlgorithm )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAfOneShot( m_pCamEngine->hCamEngine, searchAlgorithm ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * stopAf
 *****************************************************************************/
bool
CamEngineItf::stopAf()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAfStop( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * getAfStatus
 *****************************************************************************/
bool
CamEngineItf::getAfStatus( bool &enabled, CamEngineAfSearchAlgorithm_t &searchAlgorithm, float *sharpness )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t running = BOOL_FALSE;
    searchAlgorithm = CAM_ENGINE_AUTOFOCUS_SEARCH_ALGORITHM_INVALID;
    if ( RET_SUCCESS == CamEngineAfStatus( m_pCamEngine->hCamEngine, &running, &searchAlgorithm, sharpness ) )
    {
        enabled = (running == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}
/******************************************************************************
 * checkAfShot
 *****************************************************************************/
bool
CamEngineItf::checkAfShot( bool *shot )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t shot_ret;
    
    if ( RET_SUCCESS == CamEngineAfShotCheck( m_pCamEngine->hCamEngine, &shot_ret) )
    {   
        *shot = (shot_ret == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}


/******************************************************************************
 * registerAfEvtQue
 *****************************************************************************/
bool
CamEngineItf::registerAfEvtQue( CamEngineAfEvtQue_t *evtQue )
{
    bool ret;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    ret = CamEngineAfRegisterEvtQue( m_pCamEngine->hCamEngine, evtQue);    
    

    return ret;
}

/******************************************************************************
 * resetAf
 *****************************************************************************/
bool
CamEngineItf::resetAf( const CamEngineAfSearchAlgorithm_t &searchAlgorithm )
{
    bool ret;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    ret = CamEngineAfReset( m_pCamEngine->hCamEngine, searchAlgorithm);    
    

    return ret;
}
/******************************************************************************
 * getAfMeasureWindow
 *****************************************************************************/
bool
CamEngineItf::getAfMeasureWindow( CamEngineWindow_t *measureWin )
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAfGetMeasuringWindow( m_pCamEngine->hCamEngine,measureWin);

    return (result == RET_SUCCESS)?true:false;

}


/******************************************************************************
 * setAfMeasureWindow
 *****************************************************************************/
bool
CamEngineItf::setAfMeasureWindow( 
    int16_t                  x,
    int16_t                  y,
    uint16_t                  width,
    uint16_t                  height)
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAfSetMeasuringWindow( m_pCamEngine->hCamEngine,
        x, y,width,height);

    return (result == RET_SUCCESS)?true:false;

}

/******************************************************************************
 * configureFlash
 *****************************************************************************/
bool 
CamEngineItf::configureFlash( CamEngineFlashCfg_t *cfgFsh )
{
    bool ret;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    ret = CamEngineConfigureFlash(m_pCamEngine->hCamEngine,(CamerIcIspFlashCfg_t*)cfgFsh);

    return true;
}

/******************************************************************************
 * startFlash
 *****************************************************************************/
bool 
CamEngineItf::startFlash( bool operate_now )
{
    bool ret;
    bool_t operate = (operate_now==true)?BOOL_TRUE:BOOL_FALSE;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    ret = CamEngineStartFlash(m_pCamEngine->hCamEngine,operate);

    return true;
}

/******************************************************************************
 * stopFlash
 *****************************************************************************/
bool 
CamEngineItf::stopFlash( bool operate_now )
{
    bool ret;
    bool_t operate = (operate_now==true)?BOOL_TRUE:BOOL_FALSE;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    ret = CamEngineStopFlash(m_pCamEngine->hCamEngine,operate);

    return true;
}
/******************************************************************************
 * isAwbEnabled
 *****************************************************************************/
bool
CamEngineItf::isAwbEnabled()
{
    CamEngineAwbMode_t      mode;
    uint32_t                idx;
    CamEngineAwbRgProj_t    RgProj;
    bool_t                  damp;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t running = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAwbStatus( m_pCamEngine->hCamEngine, &running, &mode, &idx, &RgProj, &damp ) )
    {
        return ( (BOOL_TRUE == running) ? true : false );
    }

    return false;
}


/******************************************************************************
 * startAwb
 *****************************************************************************/
bool
CamEngineItf::startAwb( const CamEngineAwbMode_t &mode, uint32_t idx, const bool_t damp )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAwbStart( m_pCamEngine->hCamEngine, mode, idx, damp ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * stopAwb
 *****************************************************************************/
bool
CamEngineItf::stopAwb()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAwbStop( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * resetAwb
 *****************************************************************************/
bool
CamEngineItf::resetAwb()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAwbReset( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * resetAwb
 *****************************************************************************/
bool CamEngineItf::isAwbStable()
{
	bool_t stable_state=BOOL_FALSE;
	uint32_t dNoWhitePixel=0;
	RESULT result = RET_SUCCESS;
	 
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

	result = CamEngineIsAwbStable( m_pCamEngine->hCamEngine, &stable_state, &dNoWhitePixel);
    if ( RET_SUCCESS == result && stable_state==BOOL_TRUE && dNoWhitePixel>0 )
    {
        return true;
    }

    return false;
}



/******************************************************************************
 * getAwbStatus
 *****************************************************************************/
bool
CamEngineItf::getAwbStatus( bool &enabled, CamEngineAwbMode_t &mode, uint32_t &idx, CamEngineAwbRgProj_t &RgProj, bool &damping )
{
    bool_t running = BOOL_FALSE;
    bool_t damp = BOOL_FALSE;
    mode = CAM_ENGINE_AWB_MODE_INVALID;
    idx = 0;

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAwbStatus( m_pCamEngine->hCamEngine, &running, &mode, &idx, &RgProj, &damp ) )
    {
        damping = (damp == BOOL_TRUE) ? true : false;
        enabled = (running == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}

/******************************************************************************
 * chkAwbIllumination
 *****************************************************************************/
bool
CamEngineItf::chkAwbIllumination( CamIlluminationName_t   name )
{
    int32_t result;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if(!isSOCSensor()) {
        CamCalibDbHandle_t calibHandle;
        CamIlluProfile_t    *pIllumination;
        
        calibHandle = m_pCamEngine->calibDb.GetCalibDbHandle();
        if (calibHandle == NULL) {
            TRACE(CAM_API_CAMENGINE_ERROR, "%s: calibHandle is NULL!",__FUNCTION__);
            return false;
        } else {
            int32_t no,i;
            
            result = CamCalibDbGetNoOfIlluminations( calibHandle, &no );
            if ( (RET_SUCCESS != result) || ( !no ) )
            {
                TRACE( CAM_API_CAMENGINE_ERROR, "%s: database does not conatin illumination data\n",__FUNCTION__ );
                return false;
            }

            for (i=0; i<no; i++) {
                // get an illumination profile from database
                result = CamCalibDbGetIlluminationByIdx( calibHandle, i, &pIllumination );
                RETURN_RESULT_IF_DIFFERENT( result, RET_SUCCESS );
                if (strstr( pIllumination->name,name) != NULL) {
                    return true;
                }
            }

            return false;
            
        }
    } else {
        if (IsiWhiteBalanceIlluminationChk( m_pSensor->hSensor, name) == RET_SUCCESS)
            return true;
        else
            return false;
    }

}

/******************************************************************************
 * getMfdGain
 *****************************************************************************/
bool CamEngineItf::getMfdGain(	
	char						*mfd_enable,
	float						mfd_gain[],
	float						mfd_frames[]
)
{
    // sensor isi mfd_gain0
    if ( RET_SUCCESS != CamEnginegetMfdgain( m_pCamEngine->hCamEngine, mfd_enable, mfd_gain, mfd_frames ) )
    {
        return false;
    }
    return true;
}

/******************************************************************************
 * getUvnrPara
 *****************************************************************************/
bool CamEngineItf::getUvnrPara(
	char						*uvnr_enable,
	float						uvnr_gain[],
	float						uvnr_ratio[],
	float						uvnr_distances[]
)
{
    // sensor isi mfd_gain0
    if ( RET_SUCCESS != CamEnginegetUvnrpara( m_pCamEngine->hCamEngine, uvnr_enable, uvnr_gain, uvnr_ratio, uvnr_distances ) )
    {
        return false;
    }
    return true;
}

/******************************************************************************
 * setAwbMeasureWindow
 *****************************************************************************/
bool
CamEngineItf::SetAwbMeasuringWindow( 
    int16_t x,
    int16_t y,
    uint16_t width,
    uint16_t height)
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAwbSetMeasuringWindow( m_pCamEngine->hCamEngine,
        x, y,width,height);

    return (result == RET_SUCCESS)?true:false;

}

/******************************************************************************
 * getAwbTemperature
 *****************************************************************************/
bool
CamEngineItf::getAwbTemperature(float *ct)
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAwbGetTemperature( m_pCamEngine->hCamEngine, ct);

    return (result == RET_SUCCESS)?true:false;

}

/******************************************************************************
 * getAwbMeasureWindow
 *****************************************************************************/
bool
CamEngineItf::getAwbMeasureWindow(CamEngineWindow_t *measureWin)
{
    RESULT result = RET_SUCCESS;
    
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    result = CamEngineAwbGetMeasuringWindow( m_pCamEngine->hCamEngine, measureWin);

    return (result == RET_SUCCESS)?true:false;

}


/******************************************************************************
 * startAdpf
 *****************************************************************************/
bool
CamEngineItf::startAdpf()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAdpfStart( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * stopAdpf
 *****************************************************************************/
bool
CamEngineItf::stopAdpf()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAdpfStop( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * configureAdpf
 *****************************************************************************/
bool
CamEngineItf::configureAdpf( const float gradient, const float offset, const float min, const float div, const uint8_t sigmaGreen, const uint8_t sigmaRedBlue )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAdpfConfigure( m_pCamEngine->hCamEngine, gradient, offset, min, div, sigmaGreen, sigmaRedBlue ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * getAdpfStatus
 *****************************************************************************/
bool
CamEngineItf::getAdpfStatus( bool &enabled, float &gradient, float &offset, float &min, float &div, uint8_t &sigmaGreen, uint8_t &sigmaRedBlue )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t running = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAdpfStatus( m_pCamEngine->hCamEngine, &running, &gradient, &offset, &min, &div, &sigmaGreen, &sigmaRedBlue ) )
    {
        enabled = (running == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}


/******************************************************************************
 * startAdpcc
 *****************************************************************************/
bool
CamEngineItf::startAdpcc()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAdpccStart( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * stopAdpcc
 *****************************************************************************/
bool
CamEngineItf::stopAdpcc()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAdpccStop( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * getAdpccStatus
 *****************************************************************************/
bool
CamEngineItf::getAdpccStatus( bool &enabled )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t running = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAdpccStatus( m_pCamEngine->hCamEngine, &running ) )
    {
        enabled = (running == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}



/******************************************************************************
 * startAvs
 *****************************************************************************/
bool
CamEngineItf::startAvs()
{


    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAvsStart( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * stopAvs
 *****************************************************************************/
bool
CamEngineItf::stopAvs()
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAvsStop( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}



/******************************************************************************
 * configureAvs
 *****************************************************************************/
bool
CamEngineItf::configureAvs( const bool useParams,
    const unsigned short numItpPoints, const float theta, const float baseGain,
    const float fallOff, const float acceleration )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineAvsConfigure( m_pCamEngine->hCamEngine,
        useParams, numItpPoints, theta, baseGain, fallOff, acceleration) )
    {
        return true;
    }

    return false;
}



/******************************************************************************
 * getAvsConfig
 *****************************************************************************/
bool
CamEngineItf::getAvsConfig( bool &usingParams, unsigned short &numItpPoints,
    float &theta, float &baseGain, float &fallOff, float &acceleration,
    int &numDampData, double **ppDampXData, double **ppDampYData )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t isUsingParams = BOOL_FALSE;

    if ( RET_SUCCESS == CamEngineAvsGetConfig( m_pCamEngine->hCamEngine,
        &isUsingParams, &numItpPoints, &theta, &baseGain, &fallOff,
        &acceleration, &numDampData, ppDampXData, ppDampYData ) )
    {
        usingParams = (isUsingParams == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}



/******************************************************************************
 * getAvsStatus
 *****************************************************************************/
bool
CamEngineItf::getAvsStatus( bool &running, CamEngineVector_t &displVec,
    CamEngineVector_t &offsetVec )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    bool_t isRunning = BOOL_FALSE;
    if ( RET_SUCCESS == CamEngineAvsGetStatus( m_pCamEngine->hCamEngine,
        &isRunning, &displVec, &offsetVec ) )
    {
        running = (isRunning == BOOL_TRUE) ? true : false;
        return true;
    }

    return false;
}




/******************************************************************************
 * enableJpe
 *****************************************************************************/
bool
CamEngineItf::enableJpe( int width, int height )
{
    CamerIcJpeConfig_t JpeConfig =
    {
        CAMERIC_JPE_MODE_LARGE_CONTINUOUS,
        CAMERIC_JPE_COMPRESSION_LEVEL_HIGH,
        CAMERIC_JPE_LUMINANCE_SCALE_DISABLE,
        CAMERIC_JPE_CHROMINANCE_SCALE_DISABLE,
        (uint16_t)width,
        (uint16_t)height
    };

    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineEnableJpe( m_pCamEngine->hCamEngine, &JpeConfig ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * disableJpe
 *****************************************************************************/
bool
CamEngineItf::disableJpe( )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineDisableJpe( m_pCamEngine->hCamEngine ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 * isPictureOrientationAllowed
 *****************************************************************************/
bool
CamEngineItf::isPictureOrientationAllowed( CamEngineMiOrientation_t orientation )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineIsPictureOrientationAllowed( 
                            m_pCamEngine->hCamEngine, orientation ) )
    {
        return true;
    }

    return false;
}

/******************************************************************************
 * setPictureOrientation
 *****************************************************************************/
bool
CamEngineItf::setPictureOrientation( CamEngineMiOrientation_t orientation )
{
    if ( Invalid == m_pCamEngine->state )
    {
        return false;
    }

    if ( RET_SUCCESS == CamEngineSetPictureOrientation(
                            m_pCamEngine->hCamEngine, orientation ) )
    {
        return true;
    }

    return false;
}


/******************************************************************************
 ** CamEngineItf::imgEffectsEnable()
 ******************************************************************************/
void CamEngineItf::imgEffectsEnable( CamerIcIeConfig_t *pCamerIcIeConfig )
{
    RESULT result = CamEngineEnableImageEffect( m_pCamEngine->hCamEngine, pCamerIcIeConfig );
    DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_BUSY) );
}

/******************************************************************************
 ** CamEngineItf::imgEffectsDisable()
 ******************************************************************************/
void CamEngineItf::imgEffectsDisable()
{
    RESULT result = CamEngineDisableImageEffect( m_pCamEngine->hCamEngine );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 ** CamEngineItf::imgEffectsSetTintCb()
 ******************************************************************************/
void CamEngineItf::imgEffectsSetTintCb( uint8_t tint )
{
    RESULT result = CamEngineImageEffectSetTintCb( m_pCamEngine->hCamEngine, tint );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 ** CamEngineItf::imgEffectsSetTintCr()
 ******************************************************************************/
void CamEngineItf::imgEffectsSetTintCr( uint8_t tint )
{
    RESULT result = CamEngineImageEffectSetTintCr( m_pCamEngine->hCamEngine, tint );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 ** CamEngineItf::imgEffectsSetColorSelection()
 ******************************************************************************/
void CamEngineItf::imgEffectsSetColorSelection( CamerIcIeColorSelection_t color, uint8_t threshold )
{
    RESULT result = CamEngineImageEffectSetColorSelection( m_pCamEngine->hCamEngine, color, threshold );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 ** CamEngineItf::imgEffectsSetSharpen()
 ******************************************************************************/
void CamEngineItf::imgEffectsSetSharpen( uint8_t factor, uint8_t threshold )
{
    RESULT result = CamEngineImageEffectSetSharpen( m_pCamEngine->hCamEngine, factor, threshold );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 * CamEngineItf::cProcIsAvailable
 *****************************************************************************/
bool CamEngineItf::cProcIsAvailable()
{
    RESULT result = CamEngineCprocIsAvailable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
    return ( (result == RET_SUCCESS) ? true : false );
}

/******************************************************************************
 ** CamEngineItf::cProcEnable()
 ******************************************************************************/
void CamEngineItf::cProcEnable( CamEngineCprocConfig_t *config )
{
    RESULT result = CamEngineCprocEnable( m_pCamEngine->hCamEngine, config );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
}

/******************************************************************************
 ** CamEngineItf::cProcDisable()
 ******************************************************************************/
void CamEngineItf::cProcDisable()
{
    RESULT result = CamEngineCprocDisable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
}

/******************************************************************************
 ** CamEngineItf::cProcStatus()
 ******************************************************************************/
void CamEngineItf::cProcStatus( bool& running, CamEngineCprocConfig_t& config )
{
    bool_t enabled = BOOL_FALSE;
    RESULT result = CamEngineCprocStatus( m_pCamEngine->hCamEngine, &enabled, &config );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
    running = ( BOOL_TRUE == enabled ) ? true : false;
}

/******************************************************************************
 ** CamEngineItf::cProcSetContrast()
 ******************************************************************************/
void CamEngineItf::cProcSetContrast( float contrast )
{
    RESULT result = CamEngineCprocSetContrast( m_pCamEngine->hCamEngine, contrast );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
}

/******************************************************************************
 ** CamEngineItf::cProcSetBrightness()
 ******************************************************************************/
void CamEngineItf::cProcSetBrightness( float brightness )
{
    RESULT result = CamEngineCprocSetBrightness( m_pCamEngine->hCamEngine, brightness );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
}

/******************************************************************************
 ** CamEngineItf::cProcSetSaturation()
 ******************************************************************************/
void CamEngineItf::cProcSetSaturation( float saturation )
{
    RESULT result = CamEngineCprocSetSaturation( m_pCamEngine->hCamEngine, saturation );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
}

/******************************************************************************
 ** CamEngineItf::cProcSetHue()
 ******************************************************************************/
void CamEngineItf::cProcSetHue( float hue )
{
    RESULT result = CamEngineCprocSetHue( m_pCamEngine->hCamEngine, hue );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
}

/******************************************************************************
 ** CamEngineItf::sImpEnable()
 ******************************************************************************/
void CamEngineItf::sImpEnable( CamEngineSimpConfig_t *config )
{
    RESULT result = CamEngineEnableSimp( m_pCamEngine->hCamEngine, config );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 ** CamEngineItf::sImpDisable()
 ******************************************************************************/
void CamEngineItf::sImpDisable()
{
    RESULT result = CamEngineDisableSimp( m_pCamEngine->hCamEngine );
    DCT_ASSERT( result == RET_SUCCESS );
}

/******************************************************************************
 ** CamEngineItf::blsSet()
 *****************************************************************************/
void CamEngineItf::blsSet
(
    const uint16_t Red,
    const uint16_t GreenR,
    const uint16_t GreenB,
    const uint16_t Blue
)
{
    RESULT result = CamEngineBlsSet( m_pCamEngine->hCamEngine, Red, GreenR, GreenB, Blue );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::blsGet()
 *****************************************************************************/
void CamEngineItf::blsGet
(
    uint16_t *Red,
    uint16_t *GreenR,
    uint16_t *GreenB,
    uint16_t *Blue
)
{
    RESULT result = CamEngineBlsGet( m_pCamEngine->hCamEngine, Red, GreenR, GreenB, Blue );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::wbGainSet()
 *****************************************************************************/
void CamEngineItf::wbGainSet
(
    const float Red,
    const float GreenR,
    const float GreenB,
    const float Blue
)
{
    CamEngineWbGains_t Gains;

    Gains.Red       = Red;
    Gains.GreenR    = GreenR;
    Gains.GreenB    = GreenB;
    Gains.Blue      = Blue;

    RESULT result = CamEngineWbSetGains( m_pCamEngine->hCamEngine, &Gains );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::wbGainGet()
 *****************************************************************************/
void CamEngineItf::wbGainGet
(
    float *Red,
    float *GreenR,
    float *GreenB,
    float *Blue
)
{
    if ( (Red != NULL) && (GreenR!=NULL) && (GreenB!=NULL) && (Blue!=NULL) )
    {
        CamEngineWbGains_t Gains;

        RESULT result = CamEngineWbGetGains( m_pCamEngine->hCamEngine, &Gains );
        DCT_ASSERT( result == RET_SUCCESS );

        *Red     = Gains.Red;
        *GreenR  = Gains.GreenR;
        *GreenB  = Gains.GreenB;
        *Blue    = Gains.Blue;
    }
}


/******************************************************************************
 ** CamEngineItf::wbCcMatrixGet()
 *****************************************************************************/
void CamEngineItf::wbCcMatrixGet
(
    float *Red1,
    float *Red2,
    float *Red3,

    float *Green1,
    float *Green2,
    float *Green3,

    float *Blue1,
    float *Blue2,
    float *Blue3
)
{
    if ( (Red1 != NULL) && (Red2 != NULL) && (Red3 != NULL)
            && (Green1 != NULL) && (Green2 != NULL) && (Green3 != NULL)
            && (Blue1 != NULL) && (Blue2 != NULL) && (Blue2 != NULL) )
    {
        CamEngineCcMatrix_t CcMatrix;

        RESULT result = CamEngineWbGetCcMatrix( m_pCamEngine->hCamEngine, &CcMatrix );
        DCT_ASSERT( result == RET_SUCCESS );

        *Red1 = CcMatrix.Coeff[0];
        *Red2 = CcMatrix.Coeff[1];
        *Red3 = CcMatrix.Coeff[2];

        *Green1 = CcMatrix.Coeff[3];
        *Green2 = CcMatrix.Coeff[4];
        *Green3 = CcMatrix.Coeff[5];

        *Blue1 = CcMatrix.Coeff[6];
        *Blue2 = CcMatrix.Coeff[7];
        *Blue3 = CcMatrix.Coeff[8];
    }
}


/******************************************************************************
 ** CamEngineItf::wbCcMatrixSet()
 *****************************************************************************/
void CamEngineItf::wbCcMatrixSet
(
    const float Red1,
    const float Red2,
    const float Red3,

    const float Green1,
    const float Green2,
    const float Green3,

    const float Blue1,
    const float Blue2,
    const float Blue3
)
{
    CamEngineCcMatrix_t CcMatrix;

    CcMatrix.Coeff[0] = Red1;
    CcMatrix.Coeff[1] = Red2;
    CcMatrix.Coeff[2] = Red3;

    CcMatrix.Coeff[3] = Green1;
    CcMatrix.Coeff[4] = Green2;
    CcMatrix.Coeff[5] = Green3;

    CcMatrix.Coeff[6] = Blue1;
    CcMatrix.Coeff[7] = Blue2;
    CcMatrix.Coeff[8] = Blue3;

    RESULT result = CamEngineWbSetCcMatrix( m_pCamEngine->hCamEngine, &CcMatrix );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::wbCcOffsetGet()
 *****************************************************************************/
void CamEngineItf::wbCcOffsetGet
(
    int16_t *Red,
    int16_t *Green,
    int16_t *Blue
)
{
    CamEngineCcOffset_t CcOffset;

    RESULT result = CamEngineWbGetCcOffset( m_pCamEngine->hCamEngine, &CcOffset );
    DCT_ASSERT( result == RET_SUCCESS );

    *Red    = CcOffset.Red;
    *Green  = CcOffset.Green;
    *Blue   = CcOffset.Blue;
}


/******************************************************************************
 ** CamEngineItf::wbCcOffsetSet()
 *****************************************************************************/
void CamEngineItf::wbCcOffsetSet
(
    const int16_t Red,
    const int16_t Green,
    const int16_t Blue
)
{
    CamEngineCcOffset_t CcOffset;

    CcOffset.Red    = Red;
    CcOffset.Green  = Green;
    CcOffset.Blue   = Blue;

    RESULT result = CamEngineWbSetCcOffset( m_pCamEngine->hCamEngine, &CcOffset );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * CamEngineItf::lscStatus()
 *****************************************************************************/
void CamEngineItf::lscStatus( bool& running, CamEngineLscConfig_t& config )
{
    bool_t enabled = BOOL_FALSE;
    
    RESULT result = CamEngineLscStatus( m_pCamEngine->hCamEngine, &enabled, &config );
    DCT_ASSERT( result == RET_SUCCESS );

    running = ( enabled == BOOL_TRUE ) ? true : false;
}


/******************************************************************************
 ** CamEngineItf::lscEnable()
 *****************************************************************************/
void CamEngineItf::lscEnable()
{
    RESULT result = CamEngineLscEnable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * CamEngineItf::lscDisable()
 *****************************************************************************/
void CamEngineItf::lscDisable()
{
    RESULT result = CamEngineLscDisable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * CamEngineItf::cacStatus()
 *****************************************************************************/
void CamEngineItf::cacStatus( bool& running, CamEngineCacConfig_t& config )
{
    bool_t enabled = BOOL_FALSE;
    
    RESULT result = CamEngineCacStatus( m_pCamEngine->hCamEngine, &enabled, &config );
    DCT_ASSERT( result == RET_SUCCESS );

    running = ( enabled == BOOL_TRUE ) ? true : false;
}


/******************************************************************************
 ** CamEngineItf::cacEnable()
 *****************************************************************************/
void CamEngineItf::cacEnable()
{
    RESULT result = CamEngineCacEnable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * CamEngineItf::cacDisable()
 *****************************************************************************/
void CamEngineItf::cacDisable()
{
    RESULT result = CamEngineCacDisable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::wdrEnable()
 *****************************************************************************/
void CamEngineItf::wdrEnable()
{
    RESULT result = CamEngineWdrEnable(m_pCamEngine->hCamEngine);
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::wdrDisable()
 *****************************************************************************/
void CamEngineItf::wdrDisable()
{
    RESULT result = CamEngineWdrDisable(m_pCamEngine->hCamEngine);
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::wdrSetCurve()
 *****************************************************************************/
void CamEngineItf::wdrSetCurve( CamEngineWdrCurve_t WdrCurve )
{
    RESULT result = CamEngineWdrSetCurve( m_pCamEngine->hCamEngine , WdrCurve );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::gamCorrectStatus()
 *****************************************************************************/
void CamEngineItf::gamCorrectStatus( bool& running )
{
    bool_t enabled = BOOL_FALSE;

    RESULT result = CamEngineGammaStatus( m_pCamEngine->hCamEngine, &enabled );
    DCT_ASSERT( result == RET_SUCCESS );
    
    running = ( enabled == BOOL_TRUE ) ? true : false;
}


/******************************************************************************
 ** CamEngineItf::gamCorrectEnable()
 *****************************************************************************/
void CamEngineItf::gamCorrectEnable()
{
    RESULT result = CamEngineGammaEnable(m_pCamEngine->hCamEngine);
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::gamCorrectDisable()
 *****************************************************************************/
void CamEngineItf::gamCorrectDisable()
{
    RESULT result = CamEngineGammaDisable(m_pCamEngine->hCamEngine);
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 ** CamEngineItf::gamCorrectSetCurve()
 *****************************************************************************/
void CamEngineItf::gamCorrectSetCurve()
{
	CamCalibDbHandle_t handle = m_pCamEngine->calibDb.GetCalibDbHandle();
    RESULT result = CamEngineGammaSetCurve( m_pCamEngine->hCamEngine, handle->pGammaOut->Curve );
    DCT_ASSERT( result == RET_SUCCESS );
}

void CamEngineItf::ColorConversionSetRange
(
	CamerIcColorConversionRange_t YConvRange,
	CamerIcColorConversionRange_t CrConvRange
)
{
    RESULT result = CamEngineSetColorConversionRange( m_pCamEngine->hCamEngine, YConvRange, CrConvRange);
    DCT_ASSERT( result == RET_SUCCESS );
}

void CamEngineItf::ColorConversionSetCoefficients
(
	CamerIc3x3Matrix_t	*pCConvCoefficients
)
{
    RESULT result = CamEngineSetColorConversionCoefficients( m_pCamEngine->hCamEngine, pCConvCoefficients );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * demosaicGet
 *****************************************************************************/
void CamEngineItf::demosaicGet( bool& bypass, uint8_t& threshold ) const
{
    bool_t bypassMode = BOOL_FALSE;

    RESULT result = CamEngineDemosaicGet( m_pCamEngine->hCamEngine, &bypassMode, &threshold );
    DCT_ASSERT( result == RET_SUCCESS );

    bypass = (BOOL_TRUE != bypassMode) ? false : true;
}


/******************************************************************************
 * demosaicSet
 *****************************************************************************/
void CamEngineItf::demosaicSet( bool bypass, uint8_t threshold )
{
    bool_t bypassMode = (true != bypass) ? BOOL_FALSE : BOOL_TRUE;

    RESULT result = CamEngineDemosaicSet( m_pCamEngine->hCamEngine, bypassMode, threshold );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * filterStatus
 *****************************************************************************/
void CamEngineItf::filterStatus( bool& running, uint8_t& denoiseLevel, uint8_t& sharpenLevel )
{
    bool_t enabled = BOOL_FALSE;

    RESULT result = CamEngineFilterStatus( m_pCamEngine->hCamEngine, &enabled, &denoiseLevel, &sharpenLevel );
    DCT_ASSERT( result == RET_SUCCESS );

    running = ( enabled == BOOL_TRUE ) ? true : false;
}


/******************************************************************************
 * filterEnable
 *****************************************************************************/
void CamEngineItf::filterEnable()
{
    RESULT result = CamEngineFilterEnable(m_pCamEngine->hCamEngine);
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * filterDisable
 *****************************************************************************/
void CamEngineItf::filterDisable()
{
    RESULT result = CamEngineFilterDisable(m_pCamEngine->hCamEngine);
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * filterSetLevel
 *****************************************************************************/
void CamEngineItf::filterSetLevel( uint8_t denoiseLevel, uint8_t sharpenLevel )
{
    RESULT result = CamEngineFilterSetLevel( m_pCamEngine->hCamEngine, denoiseLevel, sharpenLevel );
    DCT_ASSERT( result == RET_SUCCESS );
}


/******************************************************************************
 * cnrIsAvailable
 *****************************************************************************/
bool CamEngineItf::cnrIsAvailable()
{
    RESULT result = CamEngineCnrIsAvailable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
    return ( (result == RET_SUCCESS) ? true : false );
}


/******************************************************************************
 * cnrStatus 
 *****************************************************************************/
void CamEngineItf::cnrStatus( bool& running, uint32_t& TC1, uint32_t& TC2  )
{
    bool_t enabled = BOOL_FALSE;

    RESULT result = CamEngineCnrStatus( m_pCamEngine->hCamEngine, &enabled, &TC1, &TC2 );
    DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_NOTSUPP) );
    
    running = ( enabled == BOOL_TRUE ) ? true : false;
}


/******************************************************************************
 * cnrEnable
 *****************************************************************************/
void CamEngineItf::cnrEnable()
{
    RESULT result = CamEngineCnrEnable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_NOTSUPP) );
}


/******************************************************************************
 * cnrDisable
 *****************************************************************************/
void CamEngineItf::cnrDisable()
{
    RESULT result = CamEngineCnrDisable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_NOTSUPP) );
}


/******************************************************************************
 * cnrSetThresholds
 *****************************************************************************/
void CamEngineItf::cnrSetThresholds( uint32_t TC1, uint32_t TC2 )
{
    RESULT result = CamEngineCnrSetThresholds( m_pCamEngine->hCamEngine, TC1, TC2 );
    DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_NOTSUPP) );
}


/******************************************************************************
 * miSwapUV 
 *****************************************************************************/
void CamEngineItf::miSwapUV( bool swap )
{
    RESULT result = CamEngineMiSwapUV( m_pCamEngine->hCamEngine, (swap) ? BOOL_TRUE : BOOL_FALSE );
    DCT_ASSERT( (result == RET_SUCCESS) || (result == RET_NOTSUPP) );
}

/******************************************************************************
 * dcropIsAvailable
 *****************************************************************************/
bool CamEngineItf::dcropIsAvailable()
{
    RESULT result = CamEngineDualCroppingIsAvailable( m_pCamEngine->hCamEngine );
    DCT_ASSERT( (RET_SUCCESS == result) || (RET_NOTSUPP == result) );
    return ( (result == RET_SUCCESS) ? true : false );
}

/******************************************************************************
 * dcropMpWindowGet
 *****************************************************************************/
void CamEngineItf::dcropMpWindowGet( bool& enabled, uint16_t& x,  uint16_t& y, uint16_t& width, uint16_t& height )
{
    // get current settings
    CamEnginePathConfig_t curConfig;
    if ( true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_MAIN, curConfig ) )
    {
        return;
    }

    enabled = (BOOL_TRUE == curConfig.dcEnable) ? true : false;
    x       = curConfig.dcWin.hOffset;
    y       = curConfig.dcWin.vOffset;
    width   = curConfig.dcWin.width;
    height  = curConfig.dcWin.height;
}

/******************************************************************************
 * dcropMpWindowSet
 *****************************************************************************/
void CamEngineItf::dcropMpWindowSet( bool enable, uint16_t x,  uint16_t y, uint16_t width, uint16_t height )
{
    // get current settings
    CamEnginePathConfig_t curMpConfig;
    CamEnginePathConfig_t curSpConfig;

    if ( (true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_MAIN, curMpConfig )) ||
         (true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_SELF, curSpConfig )) )
    {
        return;
    }

    curMpConfig.dcEnable        = ( true == enable ) ? BOOL_TRUE : BOOL_FALSE;
    curMpConfig.dcWin.hOffset   = x;
    curMpConfig.dcWin.vOffset   = y;
    curMpConfig.dcWin.width     = width;
    curMpConfig.dcWin.height    = height;

    // set changed settings
    if ( true != CamEngineItf::setPathConfig( CHAIN_MASTER, curMpConfig, curSpConfig ) )
    {
        return;
    }
}

/******************************************************************************
 * dcropSpWindowGet
 *****************************************************************************/
void CamEngineItf::dcropSpWindowGet( bool& enabled, uint16_t& x,  uint16_t& y, uint16_t& width, uint16_t& height )
{
    // get current settings
    CamEnginePathConfig_t curConfig;
    if ( true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_SELF, curConfig ) )
    {
        return;
    }

    enabled = (BOOL_TRUE == curConfig.dcEnable) ? true : false;
    x       = curConfig.dcWin.hOffset;
    y       = curConfig.dcWin.vOffset;
    width   = curConfig.dcWin.width;
    height  = curConfig.dcWin.height;
}

/******************************************************************************
 * dcropMpWindowSet
 *****************************************************************************/
void CamEngineItf::dcropSpWindowSet( bool enable, uint16_t x,  uint16_t y, uint16_t width, uint16_t height )
{
    // get current settings
    CamEnginePathConfig_t curMpConfig;
    CamEnginePathConfig_t curSpConfig;

    if ( (true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_MAIN, curMpConfig )) ||
         (true != CamEngineItf::getPathConfig( CHAIN_MASTER, CAM_ENGINE_PATH_SELF, curSpConfig )) )
    {
        return;
    }

    curSpConfig.dcEnable        = ( true == enable ) ? BOOL_TRUE : BOOL_FALSE;
    curSpConfig.dcWin.hOffset   = x;
    curSpConfig.dcWin.vOffset   = y;
    curSpConfig.dcWin.width     = width;
    curSpConfig.dcWin.height    = height;

    // set changed settings
    if ( true != CamEngineItf::setPathConfig( CHAIN_MASTER, curMpConfig, curSpConfig ) )
    {
        return;
    }
}

/******************************************************************************
 * closeSensor
 *****************************************************************************/
void CamEngineItf::closeSensor()
{
    reset();

    DCT_ASSERT( Invalid == m_pCamEngine->state );

    if ( NULL != m_pSensor->hSensor )
    {
        (void)IsiReleaseSensorIss( m_pSensor->hSensor );
        m_pSensor->hSensor = NULL;
    }

    if ( NULL != m_pSensor->hSubSensor )
    {
        (void)IsiReleaseSensorIss( m_pSensor->hSubSensor );
        m_pSensor->hSubSensor = NULL;
    }

    if ( NULL != m_pSensor->hSensorLib )
    {
        //not close sensor lib here,zyc
       // dlclose( m_pSensor->hSensorLib );
        m_pSensor->hSensorLib    = NULL;
        m_pSensor->pCamDrvConfig = NULL;
    }


    // reset calibration database
    m_pCamEngine->sensorDrvFile = std::string();
    m_pCamEngine->calibDbFile = std::string();
   m_pCamEngine->calibDb = CalibDb();

    m_pSensor->enableTestpattern = false;
    m_pSensor->enableAfps= false;

    MEMSET( &m_pSensor->sensorConfig, 0, sizeof( IsiSensorConfig_t ) );
}


/******************************************************************************
 * closeImage
 *****************************************************************************/
void
CamEngineItf::closeImage()
{
    reset();

    DCT_ASSERT( Invalid == m_pCamEngine->state );

    if ( NULL != m_pSensor->image.Data.raw.pBuffer )
    {
        free( m_pSensor->image.Data.raw.pBuffer );
        m_pSensor->image.Data.raw.pBuffer = NULL;
    }

    // reset calibration database
    m_pCamEngine->sensorDrvFile = std::string();
    m_pCamEngine->calibDbFile = std::string();
    m_pCamEngine->calibDb = CalibDb();

    MEMSET( &m_pSensor->image, 0, sizeof( PicBufMetaData_t ) );
}


/******************************************************************************
 * setupSensor
 *****************************************************************************/
bool
CamEngineItf::setupSensor()
{
    DCT_ASSERT( Invalid == m_pCamEngine->state );

    if ( true != isSensorConnected() )
    {
        return false;
    }
    if ( ( is3DEnabled() ) && ( true != isSubSensorConnected() ) )
    {
        return false;
    }

    if ( RET_SUCCESS != IsiSetupSensorIss( m_pSensor->hSensor, &m_pSensor->sensorConfig ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't configure sensor)\n", __FUNCTION__ );
        return false;
    }

    if ( RET_SUCCESS != IsiActivateTestPattern( m_pSensor->hSensor, m_pSensor->enableTestpattern ? BOOL_TRUE : BOOL_FALSE ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't activate test pattern of sensor)\n", __FUNCTION__ );
        return false;
    }

    if ( is3DEnabled() )
    {
        if ( RET_SUCCESS != IsiSetupSensorIss( m_pSensor->hSubSensor, &m_pSensor->sensorConfig ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't configure sub sensor)\n", __FUNCTION__ );
            return false;
        }

        if ( RET_SUCCESS != IsiActivateTestPattern( m_pSensor->hSubSensor, m_pSensor->enableTestpattern ? BOOL_TRUE : BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't activate test pattern of sub sensor)\n", __FUNCTION__ );
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * doneSensor
 *****************************************************************************/
void
CamEngineItf::doneSensor()
{
    DCT_ASSERT( Idle == m_pCamEngine->state );

    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sub sensor)\n", __FUNCTION__ );
        }
    }
}


/******************************************************************************
 * startSensor
 *****************************************************************************/
void
CamEngineItf::startSensor()
{
    DCT_ASSERT( Invalid == m_pCamEngine->state );

    if ( true != isSensorConnected() )
    {
        return;
    }
    if ( ( is3DEnabled() ) && ( true != isSubSensorConnected() ) )
    {
        return;
    }

    if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_TRUE ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sensor)\n", __FUNCTION__ );
        return;
    }

    if ( is3DEnabled() )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_TRUE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't start streaming from sub sensor)\n", __FUNCTION__ );
            return;
        }
    }
}


/******************************************************************************
 * stopSensor
 *****************************************************************************/
void
CamEngineItf::stopSensor()
{
    DCT_ASSERT( Idle == m_pCamEngine->state );

    if ( NULL != m_pSensor->hSensor )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sensor)\n", __FUNCTION__ );
        }
    }

    if ( ( NULL != m_pSensor->hSubSensor ) && ( is3DEnabled() ) )
    {
        if ( RET_SUCCESS != IsiSensorSetStreamingIss( m_pSensor->hSubSensor, BOOL_FALSE ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't stop streaming from sub sensor)\n", __FUNCTION__ );
        }
    }
}



/******************************************************************************
 * setupCamerIC
 *****************************************************************************/
bool
CamEngineItf::setupCamerIC()
{
    DCT_ASSERT( Invalid == m_pCamEngine->state );

    if ( NULL == m_pCamEngine->hCamEngine )
    {
        return false;
    }

    switch ( m_pCamEngine->camEngineConfig.mode )
    {
        
        case CAM_ENGINE_MODE_SENSOR_2D_IMGSTAB:
        {
            // sensor interface
            m_pCamEngine->camEngineConfig.data.sensor.hSensor = m_pSensor->hSensor;
            m_pCamEngine->camEngineConfig.data.sensor.hSubSensor = m_pSensor->hSubSensor;

            // input interface
            if ( m_pSensor->sensorConfig.SmiaMode != ISI_SMIA_OFF )
            {
                m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_SMIA;
            }
            else if ( m_pSensor->sensorConfig.MipiMode != ISI_MIPI_OFF )
            {
                if ( m_pSensor->sensorItf == 0 )
                {
                    m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_MIPI;
                }
                else
                {
                    m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_MIPI_2;
                }
            }
            else
            {
                m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_PARALLEL;
            }

            // input acquisition
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.sampleEdge, m_pSensor->sensorConfig.Edge );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.hSyncPol, m_pSensor->sensorConfig.HPol );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.vSyncPol, m_pSensor->sensorConfig.VPol );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.bayerPattern, m_pSensor->sensorConfig.BPat );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.subSampling, m_pSensor->sensorConfig.Conv422 );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.seqCCIR, m_pSensor->sensorConfig.YCSequence );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.fieldSelection, m_pSensor->sensorConfig.FieldSelection );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.inputSelection, m_pSensor->sensorConfig.BusWidth );

            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.mode, m_pSensor->sensorConfig.Mode );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.acqWindow, m_pSensor->sensorConfig.Resolution );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.outWindow, m_pSensor->sensorConfig.Resolution );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.isWindow, m_pSensor->sensorConfig.Resolution );
            
            m_pCamEngine->camEngineConfig.data.sensor.isCroppingWindow.hOffset = ((m_pCamEngine->camEngineConfig.data.sensor.isWindow.width  - 1280u) >> 1);
            m_pCamEngine->camEngineConfig.data.sensor.isCroppingWindow.vOffset = ((m_pCamEngine->camEngineConfig.data.sensor.isWindow.height -  720u) >> 1);
            m_pCamEngine->camEngineConfig.data.sensor.isCroppingWindow.width   = 1280u;
            m_pCamEngine->camEngineConfig.data.sensor.isCroppingWindow.height  = 720u; 

            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.mipiMode, m_pSensor->sensorConfig.MipiMode );

            m_pCamEngine->camEngineConfig.data.sensor.enableTestpattern = m_pSensor->enableTestpattern ? BOOL_TRUE : BOOL_FALSE;
            m_pCamEngine->camEngineConfig.data.sensor.flickerPeriod = m_pSensor->flickerPeriod;
            m_pCamEngine->camEngineConfig.data.sensor.enableAfps = m_pSensor->enableAfps ? BOOL_TRUE : BOOL_FALSE;

            break;
        }

        case CAM_ENGINE_MODE_SENSOR_2D:
        case CAM_ENGINE_MODE_SENSOR_3D:
        {
            // sensor interface
            m_pCamEngine->camEngineConfig.data.sensor.hSensor = m_pSensor->hSensor;
            m_pCamEngine->camEngineConfig.data.sensor.hSubSensor = m_pSensor->hSubSensor;

            // input interface
            if ( m_pSensor->sensorConfig.SmiaMode != ISI_SMIA_OFF )
            {
                m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_SMIA;
            }
            else if ( m_pSensor->sensorConfig.MipiMode != ISI_MIPI_OFF )
            {
                if ( m_pSensor->sensorItf == 0 )
                {
                    m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_MIPI;
                }
                else
                {
                    m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_MIPI_2;
                }
            }
            else
            {
                m_pCamEngine->camEngineConfig.data.sensor.ifSelect = CAMERIC_ITF_SELECT_PARALLEL;
            }

            // input acquisition
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.sampleEdge, m_pSensor->sensorConfig.Edge );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.hSyncPol, m_pSensor->sensorConfig.HPol );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.vSyncPol, m_pSensor->sensorConfig.VPol );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.bayerPattern, m_pSensor->sensorConfig.BPat );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.subSampling, m_pSensor->sensorConfig.Conv422 );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.seqCCIR, m_pSensor->sensorConfig.YCSequence );
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.fieldSelection, m_pSensor->sensorConfig.FieldSelection );
            if(isSOCSensor())
                isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.inputSelection, getBusWidth() );
            else
                isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.inputSelection, m_pSensor->sensorConfig.BusWidth );

            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.mode, m_pSensor->sensorConfig.Mode );

            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.acqWindow, m_pSensor->sensorConfig.Resolution );


            //for test soc camera
            if(isSOCSensor())
                m_pCamEngine->camEngineConfig.data.sensor.acqWindow.width *=2;
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.outWindow, m_pSensor->sensorConfig.Resolution );
            if(isSOCSensor() && (getBusWidth() == ISI_BUSWIDTH_12BIT))
                m_pCamEngine->camEngineConfig.data.sensor.outWindow.width *=2; // soc camera raw
            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.isWindow, m_pSensor->sensorConfig.Resolution );

            isiCapValue( m_pCamEngine->camEngineConfig.data.sensor.mipiMode, m_pSensor->sensorConfig.MipiMode );

            m_pCamEngine->camEngineConfig.data.sensor.enableTestpattern = m_pSensor->enableTestpattern ? BOOL_TRUE : BOOL_FALSE;
            m_pCamEngine->camEngineConfig.data.sensor.flickerPeriod = m_pSensor->flickerPeriod;
            m_pCamEngine->camEngineConfig.data.sensor.enableAfps = m_pSensor->enableAfps ? BOOL_TRUE : BOOL_FALSE;

            break;
        }

        case CAM_ENGINE_MODE_IMAGE_PROCESSING:
        {
            // image configuration
            m_pCamEngine->camEngineConfig.mode = CAM_ENGINE_MODE_IMAGE_PROCESSING;

            m_pCamEngine->camEngineConfig.data.image.type    = m_pSensor->image.Type;
            m_pCamEngine->camEngineConfig.data.image.layout  = m_pSensor->image.Layout;

            m_pCamEngine->camEngineConfig.data.image.pBuffer = m_pSensor->image.Data.raw.pBuffer;
            m_pCamEngine->camEngineConfig.data.image.width   = m_pSensor->image.Data.raw.PicWidthPixel;
            m_pCamEngine->camEngineConfig.data.image.height  = m_pSensor->image.Data.raw.PicHeightPixel;

            m_pCamEngine->camEngineConfig.data.image.pWbGains  = &m_pSensor->wbGains;
            m_pCamEngine->camEngineConfig.data.image.pCcMatrix = &m_pSensor->ccMatrix;
            m_pCamEngine->camEngineConfig.data.image.pCcOffset = &m_pSensor->ccOffset;
            m_pCamEngine->camEngineConfig.data.image.pBlvl     = &m_pSensor->blvl;

            m_pCamEngine->camEngineConfig.data.image.vGain     = m_pSensor->vGain;
            m_pCamEngine->camEngineConfig.data.image.vItime    = m_pSensor->vItime;

            break;
        }
        default:
        {
            return false;
        }
    }
    
    //zyc ,for test, not use calib ,soc camera
    if(!isSOCSensor())
        m_pCamEngine->camEngineConfig.hCamCalibDb = m_pCamEngine->calibDb.GetCalibDbHandle();
    else
        m_pCamEngine->camEngineConfig.hCamCalibDb = NULL;

    RESULT result = CamEngineStart( m_pCamEngine->hCamEngine, &m_pCamEngine->camEngineConfig );
    if ( RET_PENDING == result )
    {
        if ( OSLAYER_OK != osEventTimedWait( &m_pCamEngine->m_eventStart, CAM_API_CAMENGINE_TIMEOUT ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR,
                    "%s (starting the cam-engine instance timed out)\n", __FUNCTION__ );
        }
    }
    else if ( RET_SUCCESS != result )
    {
        TRACE( CAM_API_CAMENGINE_ERROR,
                "%s (can't start the cam-engine instance)\n", __FUNCTION__ );
        return false;
    }

    return true;
}


/******************************************************************************
 * doneCamerIC
 *****************************************************************************/
void
CamEngineItf::doneCamerIC()
{
    DCT_ASSERT( Idle == m_pCamEngine->state );

    RESULT result = CamEngineStop( m_pCamEngine->hCamEngine );
    if ( RET_PENDING == result )
    {
        if ( OSLAYER_OK != osEventTimedWait( &m_pCamEngine->m_eventStop, CAM_API_CAMENGINE_TIMEOUT ) )
        {
            TRACE( CAM_API_CAMENGINE_ERROR,
                    "%s (stopping the cam-engine instance timed out)\n", __FUNCTION__ );
        }
    }
    else if ( RET_SUCCESS != result )
    {
        TRACE( CAM_API_CAMENGINE_ERROR,
                "%s (can't stop the cam-engine instance)\n", __FUNCTION__ );
    }
}

/******************************************************************************
 * getSensorXmlVersion
 *****************************************************************************/
bool
CamEngineItf::getSensorXmlVersion(char (*pVersion)[64])
{
    if ( RET_SUCCESS != CamCalibDbGetSensorXmlVersion( m_pCamEngine->calibDb.GetCalibDbHandle(), pVersion ) )
    {
        TRACE( CAM_API_CAMENGINE_ERROR, "%s (can't read the sensor XML version)\n", __FUNCTION__);
        return false;
    }

	return true;
}

bool CamEngineItf::setAePoint(float point)
{
	if(RET_SUCCESS != CamEngineSetAecPoint(m_pCamEngine->hCamEngine, point))
		return false;
	return true;
}

bool CamEngineItf::getInitAePoint(float *point)
{
	if(RET_SUCCESS != CamCalibDbGetAecPoint(m_pCamEngine->calibDb.GetCalibDbHandle(), point))
		return false;
	return true;
}

float CamEngineItf::getAecClmTolerance() const
{
	float clmtolerance;
	CamEngineAecGetClmTolerance(m_pCamEngine->hCamEngine, &clmtolerance);

	return clmtolerance;
}

bool CamEngineItf::setAeClmTolerance(float clmtolerance)
{
	if(RET_SUCCESS != CamEngineSetAecClmTolerance(m_pCamEngine->hCamEngine, clmtolerance))
		return false;
	return true;
}

/*
CamEngineVersionItf::CamEngineVersionItf()
{


}

CamEngineVersionItf::~CamEngineVersionItf()
{


}
*/
bool CamEngineVersionItf::getVersion(CamEngVer_t *ver)
{
    
    ver->libisp_ver = CONFIG_SILICONIMAGE_LIBISP_VERSION;
    ver->isi_ver = CONFIG_ISI_VERSION;

    return true;
}

/******************************************************************************
 * getAwbGainInfo
 *****************************************************************************/
void CamEngineItf::getAwbGainInfo
(
	float *f_RgProj, 
	float *f_s, 
	float *f_s_Max1, 
	float *f_s_Max2, 
	float *f_Bg1, 
	float *f_Rg1, 
	float *f_Bg2, 
	float *f_Rg2
)
{
	CamEngineAwbGetinfo(m_pCamEngine->hCamEngine, f_RgProj,f_s, f_s_Max1, f_s_Max2, f_Bg1, f_Rg1, f_Bg2, f_Rg2);
}

/******************************************************************************
 * getIlluEstInfo
 *****************************************************************************/
void CamEngineItf::getIlluEstInfo(float *ExpPriorIn, float *ExpPriorOut, char (*name)[20], float likehood[], float wight[], int *curIdx, int *region, int *count)
{
	CamEngineAwbGetIlluEstInfo(m_pCamEngine->hCamEngine, ExpPriorIn, ExpPriorOut, name, likehood, wight, curIdx, region, count);
}

/******************************************************************************
 * setupOTPInfo
 *****************************************************************************/
void CamEngineItf::setupOTPInfo()
{
	uint32_t OTPInfo = 0;
	CamCalibDbGetOTPInfo( m_pCamEngine->calibDb.GetCalibDbHandle(), &OTPInfo);
	IsiSetupSensorOTPInfoIss(m_pSensor->hSensor, OTPInfo);
}

/******************************************************************************
 * enableSensorOTP
 *****************************************************************************/
void CamEngineItf::enableSensorOTP(bool_t enable)
{
	IsiEnableSensorOTPIss(m_pSensor->hSensor, enable);
}
