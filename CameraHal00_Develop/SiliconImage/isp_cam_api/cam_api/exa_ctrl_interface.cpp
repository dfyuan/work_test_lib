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
 * @file exa_ctrl_interface.cpp
 *
 * @brief
 *   Implementation of EXA (External Algorithm Module) C++ API.
 *
 *****************************************************************************/
#include "cam_api/exa_ctrl_interface.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <exa_ctrl/exa_ctrl_api.h>

#include <utils/threads.h>
using android::Thread;

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(EXA_CTRL_ITF_INFO , "EXA_CTRL_ITF: ", INFO,  0);
CREATE_TRACER(EXA_CTRL_ITF_ERROR, "EXA_CTRL_ITF: ", ERROR, 1);

#define EXA_CTRL_ITF_TIMEOUT 30000U // 30 s

/******************************************************************************
 * local type definitions
 *****************************************************************************/


/******************************************************************************
 * local variable declarations
 *****************************************************************************/


/******************************************************************************
 * local function prototypes
 *****************************************************************************/


/******************************************************************************
 * See header file for detailed comment.
 *****************************************************************************/


/******************************************************************************
 * ExaCtrlItf::ExaCtrlHolder
 *****************************************************************************/
class ExaCtrlItf::ExaCtrlHolder
{
public:
    ExaCtrlHolder( HalHandle_t hal );
    ~ExaCtrlHolder();

private:
    ExaCtrlHolder (const ExaCtrlHolder& other);
    ExaCtrlHolder& operator = (const ExaCtrlHolder& other);

public:
    static void cbCompletion( exaCtrlCmdID_t cmdId, RESULT result, void* pUserCtx );

public:
    exaCtrlHandle_t         hCtrl;
    State                   state;

    osEvent                 eventStart;
    osEvent                 eventStop;
};


/******************************************************************************
 * ExaCtrlHolder
 *****************************************************************************/
ExaCtrlItf::ExaCtrlHolder::ExaCtrlHolder( HalHandle_t hal )
    : hCtrl(NULL),
      state(Invalid)
{
    bool ok = true;

    // initialize exa ctrl
    exaCtrlConfig_t ctrlConfig;
    MEMSET( &ctrlConfig, 0, sizeof( exaCtrlConfig_t ) );

    ctrlConfig.MaxPendingCommands   = 10;
    ctrlConfig.MaxBuffers           = 1;
    ctrlConfig.exaCbCompletion      = cbCompletion;
    ctrlConfig.pUserContext         = (void *)this;
    ctrlConfig.HalHandle            = hal;
    ctrlConfig.exaCtrlHandle        = NULL;

    if ( ok && ( RET_SUCCESS != exaCtrlInit( &ctrlConfig ) ) )
    {
        TRACE( EXA_CTRL_ITF_ERROR, "%s (can't create the external algorithm module)\n", __FUNCTION__ );
        ok = false;
    }

    // create events
    if ( ok && ( OSLAYER_OK != osEventInit( &eventStart, 1, 0 ) ) )
    {
        TRACE( EXA_CTRL_ITF_ERROR, "%s (can't create start event)\n", __FUNCTION__ );
        (void)exaCtrlShutDown( ctrlConfig.exaCtrlHandle );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &eventStop, 1, 0 ) ) )
    {
        TRACE( EXA_CTRL_ITF_ERROR, "%s (can't create stop event)\n", __FUNCTION__ );
        (void)osEventDestroy( &eventStart );
        (void)exaCtrlShutDown( ctrlConfig.exaCtrlHandle );
        ok = false;
    }

    if ( true == ok )
    {
        hCtrl = ctrlConfig.exaCtrlHandle;
        state = Idle;
    }
}


/******************************************************************************
 * ~ExaCtrlHolder
 *****************************************************************************/
ExaCtrlItf::ExaCtrlHolder::~ExaCtrlHolder()
{
    if( NULL != hCtrl )
    {
        if ( RET_SUCCESS != exaCtrlShutDown( hCtrl ) )
        {
            TRACE( EXA_CTRL_ITF_ERROR, "%s (can't shutdown external algorithm module)\n", __FUNCTION__ );
        }

        (void)osEventDestroy( &eventStart );
        (void)osEventDestroy( &eventStop  );
    }
}


/******************************************************************************
 * ExaCtrlHolder::cbCompletion
 *****************************************************************************/
void ExaCtrlItf::ExaCtrlHolder::cbCompletion( exaCtrlCmdID_t cmdId,
                                               RESULT result,
                                               void* pUserCtx )
{
    ExaCtrlItf::ExaCtrlHolder *pCtrlHolder = (ExaCtrlItf::ExaCtrlHolder *)pUserCtx;

    if ( pCtrlHolder != NULL )
    {
        switch ( cmdId )
        {
            case EXA_CTRL_CMD_START:
            {
                (void)osEventSignal( &pCtrlHolder->eventStart );
                break;
            }

            case EXA_CTRL_CMD_STOP:
            {
                (void)osEventSignal( &pCtrlHolder->eventStop );
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
 * ExaCtrlItf
 *****************************************************************************/
ExaCtrlItf::ExaCtrlItf( HalHandle_t hHal )
    : m_pExaCtrl ( new ExaCtrlHolder( hHal ) )
{
    DCT_ASSERT( NULL != m_pExaCtrl );
}


/******************************************************************************
 * ~ExaCtrlItf
 *****************************************************************************/
ExaCtrlItf::~ExaCtrlItf()
{
    stop();
    delete m_pExaCtrl;
}


/******************************************************************************
 * state
 *****************************************************************************/
ExaCtrlItf::State
ExaCtrlItf::state() const
{
    return m_pExaCtrl->state;
}


/******************************************************************************
 * bufferCb
 *****************************************************************************/
void
ExaCtrlItf::bufferCb( MediaBuffer_t *pBuffer )
{
    exaCtrlShowBuffer( m_pExaCtrl->hCtrl, pBuffer );
}


/******************************************************************************
 * start
 *****************************************************************************/
bool
ExaCtrlItf::start( exaCtrlSampleCb_t sampleCb, void *pSampleContext, uint8_t sampleSkip )
{
    if ( NULL == sampleCb )
    {
        return false;
    }

    if ( Idle == m_pExaCtrl->state )
    {
        #if 0
        struct StartThread : public QThread
        {
            StartThread ( ExaCtrlHolder *exaCtrl, exaCtrlSampleCb_t cbSample, void *sampleContext, uint8_t skip )
                : m_exaCtrl (exaCtrl), m_exaCbSample (cbSample), m_pSampleContext (sampleContext), m_sampleSkip (skip) {};

            virtual void run()
            {
                RESULT result = exaCtrlStart( m_exaCtrl->hCtrl, m_exaCbSample, m_pSampleContext, m_sampleSkip );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_exaCtrl->eventStart, EXA_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( EXA_CTRL_ITF_ERROR,
                                "%s (start display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( EXA_CTRL_ITF_ERROR,
                            "%s (can't start external algorithm)\n", __FUNCTION__ );
                }
            };

            ExaCtrlHolder         *m_exaCtrl;
            exaCtrlSampleCb_t     m_exaCbSample;
            void                  *m_pSampleContext;
            uint8_t               m_sampleSkip;
        } startThread( m_pExaCtrl, sampleCb, pSampleContext, sampleSkip );

        QEventLoop loop;
        if ( true == QObject::connect( &startThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            startThread.start();
            loop.exec();

            m_pExaCtrl->state = Running;
        }
        #else
        struct StartThread : public Thread
        {
            StartThread ( ExaCtrlHolder *exaCtrl, exaCtrlSampleCb_t cbSample, void *sampleContext, uint8_t skip )
                : m_exaCtrl (exaCtrl), m_exaCbSample (cbSample), m_pSampleContext (sampleContext), m_sampleSkip (skip) {};

            virtual bool threadLoop()
            {
                RESULT result = exaCtrlStart( m_exaCtrl->hCtrl, m_exaCbSample, m_pSampleContext, m_sampleSkip );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_exaCtrl->eventStart, EXA_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( EXA_CTRL_ITF_ERROR,
                                "%s (start display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( EXA_CTRL_ITF_ERROR,
                            "%s (can't start external algorithm)\n", __FUNCTION__ );
                }
                return false;
            };

            ExaCtrlHolder         *m_exaCtrl;
            exaCtrlSampleCb_t     m_exaCbSample;
            void                  *m_pSampleContext;
            uint8_t               m_sampleSkip;
        } ;
        android::sp<StartThread> pstartThread(new StartThread( m_pExaCtrl, sampleCb, pSampleContext, sampleSkip ));
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        pstartThread->run("pstartThread");
        pstartThread->join();
        pstartThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        m_pExaCtrl->state = Running;
        #endif
    }

    return Running == m_pExaCtrl->state ? true : false;
}


/******************************************************************************
 * stop
 *****************************************************************************/
bool
ExaCtrlItf::stop()
{
    if ( Running == m_pExaCtrl->state)
    {
        #if 0
        struct StopThread : public QThread
        {
            StopThread (ExaCtrlHolder *exaCtrl) : m_exaCtrl (exaCtrl) {};

            virtual void run()
            {
                RESULT result = exaCtrlStop( m_exaCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_exaCtrl->eventStop, EXA_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( EXA_CTRL_ITF_ERROR,
                                "%s (stop display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( EXA_CTRL_ITF_ERROR,
                            "%s (can't stop external algorithm)\n", __FUNCTION__ );
                }
            };

            ExaCtrlHolder *m_exaCtrl;
        } stopThread( m_pExaCtrl );

        QEventLoop loop;
        if ( true == QObject::connect( &stopThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            stopThread.start();
            loop.exec();

            m_pExaCtrl->state = Idle;
        }
        #else
        struct StopThread : public Thread
        {
            StopThread (ExaCtrlHolder *exaCtrl) : m_exaCtrl (exaCtrl) {};

            virtual bool threadLoop()
            {
                RESULT result = exaCtrlStop( m_exaCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_exaCtrl->eventStop, EXA_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( EXA_CTRL_ITF_ERROR,
                                "%s (stop display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( EXA_CTRL_ITF_ERROR,
                            "%s (can't stop external algorithm)\n", __FUNCTION__ );
                }
                return false;
            };

            ExaCtrlHolder *m_exaCtrl;
        } ;
        android::sp<StopThread> pstopThread(new StopThread( m_pExaCtrl ));
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        pstopThread->run("pstopThread");
        pstopThread->join();
        pstopThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        m_pExaCtrl->state = Idle;
        #endif
    }

    return Idle == m_pExaCtrl->state ? true : false;
}
