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
 * @file dom_ctrl_interface.cpp
 *
 * @brief
 *   Implementation of DOM (Display Output Module) C++ API.
 *
 *****************************************************************************/
#include "cam_api/dom_ctrl_interface.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <dom_ctrl/dom_ctrl_api.h>

#include <utils/threads.h>
using android::Thread;

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(DOM_CTRL_ITF_INFO , "DOM_CTRL_ITF: ", INFO,  0);
CREATE_TRACER(DOM_CTRL_ITF_ERROR, "DOM_CTRL_ITF: ", ERROR, 1);

#define DOM_CTRL_ITF_TIMEOUT 30000U // 30 s

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
 * DomCtrlItf::DomCtrlHolder
 *****************************************************************************/
class DomCtrlItf::DomCtrlHolder
{
public:
    DomCtrlHolder( HalHandle_t hal, void *hParent );
    ~DomCtrlHolder();

private:
    DomCtrlHolder (const DomCtrlHolder& other);
    DomCtrlHolder& operator = (const DomCtrlHolder& other);

public:
    static void cbCompletion( domCtrlCmdId_t cmdId, RESULT result, const void* pUserCtx );

public:
    domCtrlHandle_t         hCtrl;
    State                   state;

    osEvent                 eventStart;
    osEvent                 eventStop;
};


/******************************************************************************
 * DomCtrlHolder
 *****************************************************************************/
DomCtrlItf::DomCtrlHolder::DomCtrlHolder( HalHandle_t hal, void *hParent )
    : hCtrl(NULL),
      state(Invalid)
{
    bool ok = true;

    // initialize dom ctrl
    domCtrlConfig_t ctrlConfig;
    MEMSET( &ctrlConfig, 0, sizeof( domCtrlConfig_t ) );

    ctrlConfig.MaxPendingCommands   = 10;
    ctrlConfig.MaxBuffers           = 1;
    ctrlConfig.domCbCompletion      = cbCompletion;
    ctrlConfig.pUserContext         = (void *)this;
    ctrlConfig.HalHandle            = hal;
    ctrlConfig.hParent              = hParent;
    ctrlConfig.width                = 0;
    ctrlConfig.height               = 0;
    ctrlConfig.ImgPresent           = DOMCTRL_IMAGE_PRESENTATION_3D_VERTICAL;
    ctrlConfig.domCtrlHandle        = NULL;

    if ( ok && ( RET_SUCCESS != domCtrlInit( &ctrlConfig ) ) )
    {
        TRACE( DOM_CTRL_ITF_ERROR, "%s (can't create the display output module)\n", __FUNCTION__ );
        ok = false;
    }

    // create events
    if ( ok && ( OSLAYER_OK != osEventInit( &eventStart, 1, 0 ) ) )
    {
        TRACE( DOM_CTRL_ITF_ERROR, "%s (can't create start event)\n", __FUNCTION__ );
        (void)domCtrlShutDown( ctrlConfig.domCtrlHandle );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &eventStop, 1, 0 ) ) )
    {
        TRACE( DOM_CTRL_ITF_ERROR, "%s (can't create stop event)\n", __FUNCTION__ );
        (void)osEventDestroy( &eventStart );
        (void)domCtrlShutDown( ctrlConfig.domCtrlHandle );
        ok = false;
    }

    if ( true == ok )
    {
        hCtrl = ctrlConfig.domCtrlHandle;
        state = Idle;
    }
}


/******************************************************************************
 * ~DomCtrlHolder
 *****************************************************************************/
DomCtrlItf::DomCtrlHolder::~DomCtrlHolder()
{
    if( NULL != hCtrl )
    {
        if ( RET_SUCCESS != domCtrlShutDown( hCtrl ) )
        {
            TRACE( DOM_CTRL_ITF_ERROR, "%s (can't shutdown display output module)\n", __FUNCTION__ );
        }

        (void)osEventDestroy( &eventStart );
        (void)osEventDestroy( &eventStop  );
    }
}


/******************************************************************************
 * DomCtrlHolder::cbCompletion
 *****************************************************************************/
void DomCtrlItf::DomCtrlHolder::cbCompletion( domCtrlCmdId_t cmdId,
                                               RESULT result,
                                               const void* pUserCtx )
{
    DomCtrlItf::DomCtrlHolder *pCtrlHolder = (DomCtrlItf::DomCtrlHolder *)pUserCtx;

    if ( pCtrlHolder != NULL )
    {
        switch ( cmdId )
        {
            case DOM_CTRL_CMD_START:
            {
                (void)osEventSignal( &pCtrlHolder->eventStart );
                break;
            }

            case DOM_CTRL_CMD_STOP:
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
 * DomCtrlItf
 *****************************************************************************/
DomCtrlItf::DomCtrlItf( HalHandle_t hHal, void *hParent )
    : m_pDomCtrl ( new DomCtrlHolder( hHal, hParent ) )
{
    DCT_ASSERT( NULL != m_pDomCtrl );
}


/******************************************************************************
 * ~DomCtrlItf
 *****************************************************************************/
DomCtrlItf::~DomCtrlItf()
{
    stop();
    delete m_pDomCtrl;
}


/******************************************************************************
 * state
 *****************************************************************************/
DomCtrlItf::State
DomCtrlItf::state() const
{
    return m_pDomCtrl->state;
}


/******************************************************************************
 * handle
 *****************************************************************************/
void*
DomCtrlItf::handle() const
{
    return (void *)m_pDomCtrl->hCtrl;
}


/******************************************************************************
 * bufferCb
 *****************************************************************************/
void
DomCtrlItf::bufferCb( MediaBuffer_t *pBuffer )
{
    domCtrlShowBuffer( m_pDomCtrl->hCtrl, pBuffer );
}


/******************************************************************************
 * start
 *****************************************************************************/
bool
DomCtrlItf::start()
{
    TRACE( DOM_CTRL_ITF_ERROR, "%s (enter)\n", __FUNCTION__ );
    if ( Idle == m_pDomCtrl->state)
    {
        #if 0
        struct StartThread : public QThread
        {
            StartThread (DomCtrlHolder *domCtrl) : m_domCtrl (domCtrl) {};

            virtual void run()
            {
                RESULT result = domCtrlStart( m_domCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_domCtrl->eventStart, DOM_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( DOM_CTRL_ITF_ERROR,
                                "%s (start display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( DOM_CTRL_ITF_ERROR,
                            "%s (can't start display output)\n", __FUNCTION__ );
                }
            };

            DomCtrlHolder *m_domCtrl;
        } startThread( m_pDomCtrl );

        QEventLoop loop;
        if ( true == QObject::connect( &startThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            startThread.start();
            loop.exec();

            m_pDomCtrl->state = Running;
        }
        #else
        struct StartThread : public Thread
        {
            StartThread (DomCtrlHolder *domCtrl) : Thread(false),m_domCtrl (domCtrl) {};
            virtual ~StartThread(){TRACE(NULL,"quit startthread+++++++++++++++++++++++++++++++");};

            virtual bool threadLoop()
            {
                RESULT result = domCtrlStart( m_domCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_domCtrl->eventStart, DOM_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( DOM_CTRL_ITF_ERROR,
                                "%s (start display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( DOM_CTRL_ITF_ERROR,
                            "%s (can't start display output)\n", __FUNCTION__ );
                }

                return false;
            };
            private:
            DomCtrlHolder *m_domCtrl;
        };


        //for test
        #if 0

        {
            class testThread : public Thread{
                public:
                testThread():Thread(false){TRACE(NULL,"enter test thread++++++++++++");};
                virtual bool threadLoop()
                {
                    TRACE(NULL,"run in thread ++++++++++++++");
                    return false;
                }
                virtual ~testThread(){TRACE(NULL,"quit test thread++++++++++++");}
            };
             android::sp<testThread> sptest(new testThread());
             sptest->run();
             sptest->join();
             sptest.clear();
        }
        
        while(1){sleep(10);}
        #endif
        
        android::sp<StartThread> spstartThread(new StartThread( m_pDomCtrl ));
        
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        spstartThread->run("spstartThread");
        spstartThread->join();
        spstartThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        m_pDomCtrl->state = Running;
        #endif

    }
    TRACE( DOM_CTRL_ITF_ERROR, "%s (exit)\n", __FUNCTION__ );
    return Running == m_pDomCtrl->state ? true : false;
}


/******************************************************************************
 * stop
 *****************************************************************************/
bool
DomCtrlItf::stop()
{
    if ( Running == m_pDomCtrl->state)
    {
        #if 0
        struct StopThread : public QThread
        {
            StopThread (DomCtrlHolder *domCtrl) : m_domCtrl (domCtrl) {};

            virtual void run()
            {
                RESULT result = domCtrlStop( m_domCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_domCtrl->eventStop, DOM_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( DOM_CTRL_ITF_ERROR,
                                "%s (stop display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( DOM_CTRL_ITF_ERROR,
                            "%s (can't stop display output)\n", __FUNCTION__ );
                }
            };

            DomCtrlHolder *m_domCtrl;
        } stopThread( m_pDomCtrl );

        QEventLoop loop;
        if ( true == QObject::connect( &stopThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            stopThread.start();
            loop.exec();

            m_pDomCtrl->state = Idle;
        }
        #else
        struct StopThread : public Thread
        {
            StopThread (DomCtrlHolder *domCtrl) : m_domCtrl (domCtrl) {};

            virtual bool threadLoop()
            {
                RESULT result = domCtrlStop( m_domCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_domCtrl->eventStop, DOM_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( DOM_CTRL_ITF_ERROR,
                                "%s (stop display timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( DOM_CTRL_ITF_ERROR,
                            "%s (can't stop display output)\n", __FUNCTION__ );
                }
                return false;
            };

            DomCtrlHolder *m_domCtrl;
        } ;
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        android::sp<StopThread> pstopthread(new StopThread( m_pDomCtrl ));
        pstopthread->run("pstopthread");
        pstopthread->join();
        pstopthread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        m_pDomCtrl->state = Idle;
        #endif
    }

    return Idle == m_pDomCtrl->state ? true : false;
}
