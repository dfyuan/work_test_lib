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
 * @file som_ctrl_interface.cpp
 *
 * @brief
 *   Implementation of SOM (Snapshot Output Module) C++ API.
 *
 *****************************************************************************/
#include "cam_api/som_ctrl_interface.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <som_ctrl/som_ctrl_api.h>

#include <utils/threads.h>
using android::Thread;

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(SOM_CTRL_ITF_INFO , "SOM_CTRL_ITF: ", INFO,  0);
CREATE_TRACER(SOM_CTRL_ITF_ERROR, "SOM_CTRL_ITF: ", ERROR, 1);

#define SOM_CTRL_ITF_TIMEOUT 30000U // 30 s

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
 * SomCtrlItf::SomCtrlHolder
 *****************************************************************************/
class SomCtrlItf::SomCtrlHolder
{
public:
    SomCtrlHolder( HalHandle_t hHal );
    ~SomCtrlHolder();

private:
    SomCtrlHolder (const SomCtrlHolder& other);
    SomCtrlHolder& operator = (const SomCtrlHolder& other);

public:
    static void cbCompletion( somCtrlCmdID_t cmdId, RESULT result, somCtrlCmdParams_t *, somCtrlCompletionInfo_t *, void* pUserCtx );

public:
    somCtrlHandle_t         hCtrl;
    State                   state;

    osEvent                 eventStarted;
    osEvent                 eventFinished;
    osEvent                 eventStop;
};


/******************************************************************************
 * SomCtrlHolder
 *****************************************************************************/
SomCtrlItf::SomCtrlHolder::SomCtrlHolder( HalHandle_t hal )
    : hCtrl(NULL),
      state(Invalid)
{
    bool ok = true;

    // initialize som ctrl
    somCtrlConfig_t ctrlConfig;
    MEMSET( &ctrlConfig, 0, sizeof( somCtrlConfig_t ) );

    ctrlConfig.MaxPendingCommands   = 10;
    ctrlConfig.MaxBuffers           = 1;
    ctrlConfig.somCbCompletion      = cbCompletion;
    ctrlConfig.pUserContext         = (void *)this;
    ctrlConfig.HalHandle            = hal;
    ctrlConfig.somCtrlHandle        = NULL;

    if ( ok && ( RET_SUCCESS != somCtrlInit( &ctrlConfig ) ) )
    {
        TRACE( SOM_CTRL_ITF_ERROR, "%s (can't create the snapshot output module)\n", __FUNCTION__ );
        ok = false;
    }

    // create events
    if ( ok && ( OSLAYER_OK != osEventInit( &eventStarted, 1, 0 ) ) )
    {
        TRACE( SOM_CTRL_ITF_ERROR, "%s (can't create start event)\n", __FUNCTION__ );
        (void)somCtrlShutDown( ctrlConfig.somCtrlHandle );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &eventFinished, 1, 0 ) ) )
    {
        TRACE( SOM_CTRL_ITF_ERROR, "%s (can't create start event)\n", __FUNCTION__ );
        (void)osEventDestroy( &eventStarted );
        (void)somCtrlShutDown( ctrlConfig.somCtrlHandle );
        ok = false;
    }

    if ( ok && ( OSLAYER_OK != osEventInit( &eventStop, 1, 0 ) ) )
    {
        TRACE( SOM_CTRL_ITF_ERROR, "%s (can't create stop event)\n", __FUNCTION__ );
        (void)osEventDestroy( &eventStarted );
        (void)osEventDestroy( &eventFinished );
        (void)somCtrlShutDown( ctrlConfig.somCtrlHandle );
        ok = false;
    }

    if ( true == ok )
    {
        hCtrl = ctrlConfig.somCtrlHandle;
        state = Idle;
    }
}


/******************************************************************************
 * ~SomCtrlHolder
 *****************************************************************************/
SomCtrlItf::SomCtrlHolder::~SomCtrlHolder()
{
    if( NULL != hCtrl )
    {
        if ( RET_SUCCESS != somCtrlShutDown( hCtrl ) )
        {
            TRACE( SOM_CTRL_ITF_ERROR, "%s (can't shutdown snapshot output module)\n", __FUNCTION__ );
        }

        (void)osEventDestroy( &eventStarted );
        (void)osEventDestroy( &eventFinished );
        (void)osEventDestroy( &eventStop  );
    }
}


/******************************************************************************
 * SomCtrlHolder::cbCompletion
 *****************************************************************************/
void SomCtrlItf::SomCtrlHolder::cbCompletion( somCtrlCmdID_t cmdId,
                                                RESULT result,
                                                somCtrlCmdParams_t *,
                                                somCtrlCompletionInfo_t *,
                                                void* pUserCtx )
{
    SomCtrlItf::SomCtrlHolder *pCtrlHolder = (SomCtrlItf::SomCtrlHolder *)pUserCtx;

    if ( pCtrlHolder != NULL )
    {
        switch ( cmdId )
        {
            case SOM_CTRL_CMD_START:
            {
                if ( RET_PENDING == result )
                {
                    (void)osEventSignal( &pCtrlHolder->eventStarted );
                }
                else
                {
                    (void)osEventSignal( &pCtrlHolder->eventFinished );
                }
                break;
            }

            case SOM_CTRL_CMD_STOP:
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
 * SomCtrlItf
 *****************************************************************************/
SomCtrlItf::SomCtrlItf( HalHandle_t hHal )
    : m_pSomCtrl ( new SomCtrlHolder( hHal ) )
{
    DCT_ASSERT( NULL != m_pSomCtrl );
}


/******************************************************************************
 * ~SomCtrlItf
 *****************************************************************************/
SomCtrlItf::~SomCtrlItf()
{
    stop();
    delete m_pSomCtrl;
}


/******************************************************************************
 * state
 *****************************************************************************/
SomCtrlItf::State
SomCtrlItf::state() const
{
    return m_pSomCtrl->state;
}


/******************************************************************************
 * bufferCb
 *****************************************************************************/
void
SomCtrlItf::bufferCb( MediaBuffer_t *pBuffer )
{
    somCtrlStoreBuffer( m_pSomCtrl->hCtrl, pBuffer );
}


/******************************************************************************
 * start
 *****************************************************************************/
bool
SomCtrlItf::start( const char *fileNameBase, uint32_t frames, uint32_t skip, bool exif, bool average )
{
    if ( Idle == m_pSomCtrl->state)
    {
        #if 0
        struct StartThread : public QThread
        {
            StartThread (SomCtrlHolder *somCtrl, const char *fileNameBase, uint32_t frames, uint32_t skip, bool exif, bool average)
                : m_somCtrl(somCtrl), m_fileNameBase(fileNameBase), m_frames(frames), m_skip(skip), m_exif(exif), m_average(average) {};

            virtual void run()
            {
                somCtrlCmdParams_t params;
                params.Start.szBaseFileName = m_fileNameBase;
                params.Start.NumOfFrames = m_frames;
                params.Start.NumSkipFrames = m_skip;
                params.Start.AverageFrames = m_average ? 1 : 0;
                params.Start.ForceRGBOut = BOOL_TRUE;
                params.Start.ExtendName  = BOOL_FALSE;
                params.Start.Exif        = (m_exif == true) ? BOOL_TRUE : BOOL_FALSE;

                RESULT result = somCtrlStart( m_somCtrl->hCtrl, &params );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventWait( &m_somCtrl->eventStarted ) )
                    {
                        TRACE( SOM_CTRL_ITF_ERROR,
                                "%s (start snapshot output timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( SOM_CTRL_ITF_ERROR,
                            "%s (can't start snapshot output)\n", __FUNCTION__ );
                }
            };

            SomCtrlHolder *m_somCtrl;
            const char    *m_fileNameBase;
            uint32_t      m_frames;
            uint32_t      m_skip;
            bool          m_exif;
            bool          m_average;
        } startThread( m_pSomCtrl, fileNameBase, frames, skip, exif, average );

        QEventLoop loop;
        if ( true == QObject::connect( &startThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            osEventReset( &m_pSomCtrl->eventFinished );

            startThread.start();
            loop.exec();

            m_pSomCtrl->state = Running;
        }
        #else
        struct StartThread : public Thread
        {
            StartThread (SomCtrlHolder *somCtrl, const char *fileNameBase, uint32_t frames, uint32_t skip, bool exif, bool average)
                : m_somCtrl(somCtrl), m_fileNameBase(fileNameBase), m_frames(frames), m_skip(skip), m_exif(exif), m_average(average) {};

            virtual bool threadLoop()
            {
                somCtrlCmdParams_t params;
                params.Start.szBaseFileName = m_fileNameBase;
                params.Start.NumOfFrames = m_frames;
                params.Start.NumSkipFrames = m_skip;
                params.Start.AverageFrames = m_average ? 1 : 0;
                params.Start.ForceRGBOut = BOOL_TRUE;
                params.Start.ExtendName  = BOOL_FALSE;
                params.Start.Exif        = (m_exif == true) ? BOOL_TRUE : BOOL_FALSE;

                RESULT result = somCtrlStart( m_somCtrl->hCtrl, &params );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventWait( &m_somCtrl->eventStarted ) )
                    {
                        TRACE( SOM_CTRL_ITF_ERROR,
                                "%s (start snapshot output timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( SOM_CTRL_ITF_ERROR,
                            "%s (can't start snapshot output)\n", __FUNCTION__ );
                }
                return false;
            };

            SomCtrlHolder *m_somCtrl;
            const char    *m_fileNameBase;
            uint32_t      m_frames;
            uint32_t      m_skip;
            bool          m_exif;
            bool          m_average;
        } ;
        android::sp<StartThread>  pstartThread(new StartThread( m_pSomCtrl, fileNameBase, frames, skip, exif, average ));

        osEventReset( &m_pSomCtrl->eventFinished );
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        pstartThread->run("pstartThread");
        pstartThread->join();
        pstartThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        m_pSomCtrl->state = Running;
        #endif
    }

    return Running == m_pSomCtrl->state ? true : false;
}


/******************************************************************************
 * stop
 *****************************************************************************/
bool
SomCtrlItf::stop()
{
	if ( Running == m_pSomCtrl->state)
	{
      #if 0
		struct StopThread : public QThread
		{
			StopThread (SomCtrlHolder *somCtrl) : m_somCtrl (somCtrl) {};

			virtual void run()
			{
		        RESULT result = somCtrlStop( m_somCtrl->hCtrl );
		        if ( RET_PENDING == result )
		        {
		            if ( OSLAYER_OK != osEventTimedWait( &m_somCtrl->eventStop, SOM_CTRL_ITF_TIMEOUT ) )
		            {
		                TRACE( SOM_CTRL_ITF_ERROR,
		                        "%s (stop snapshot output timed out)\n", __FUNCTION__ );
		            }
		        }
		        else if ( RET_SUCCESS != result )
		        {
		            TRACE( SOM_CTRL_ITF_ERROR,
		                    "%s (can't stop snapshot output)\n", __FUNCTION__ );
		        }
			};

			SomCtrlHolder *m_somCtrl;
		} stopThread( m_pSomCtrl );

		QEventLoop loop;
		if ( true == QObject::connect( &stopThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
		{
			stopThread.start();
			loop.exec();

			m_pSomCtrl->state = Idle;
		}
        #else
		struct StopThread : public Thread
		{
			StopThread (SomCtrlHolder *somCtrl) : m_somCtrl (somCtrl) {};

            virtual bool threadLoop()
			{
		        RESULT result = somCtrlStop( m_somCtrl->hCtrl );
		        if ( RET_PENDING == result )
		        {
		            if ( OSLAYER_OK != osEventTimedWait( &m_somCtrl->eventStop, SOM_CTRL_ITF_TIMEOUT ) )
		            {
		                TRACE( SOM_CTRL_ITF_ERROR,
		                        "%s (stop snapshot output timed out)\n", __FUNCTION__ );
		            }
		        }
		        else if ( RET_SUCCESS != result )
		        {
		            TRACE( SOM_CTRL_ITF_ERROR,
		                    "%s (can't stop snapshot output)\n", __FUNCTION__ );
		        }
                return false;
			};

			SomCtrlHolder *m_somCtrl;
		} ;
        android::sp<StopThread> pstopThread(new StopThread( m_pSomCtrl ));
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        pstopThread->run("pstopThread");
        pstopThread->join();
        pstopThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
		m_pSomCtrl->state = Idle;
        #endif
	}

    return Idle == m_pSomCtrl->state ? true : false;
}


/******************************************************************************
 * waitForFinished
 *****************************************************************************/
bool
SomCtrlItf::waitForFinished() const
{
    if ( Running == m_pSomCtrl->state)
    {
        #if 0
        struct WaitThread : public QThread
        {
            WaitThread (SomCtrlHolder *somCtrl)
                : m_somCtrl(somCtrl) {};

            virtual void run()
            {
                if ( OSLAYER_OK != osEventWait( &m_somCtrl->eventFinished ) )
                {
                    TRACE( SOM_CTRL_ITF_ERROR,
                            "%s (wait for snapshot output timed out)\n", __FUNCTION__ );
                }
            };

            SomCtrlHolder *m_somCtrl;
        } waitThread( m_pSomCtrl );

        QEventLoop loop;
        if ( true == QObject::connect( &waitThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
            waitThread.start();
            loop.exec();
        }
        #else
        struct WaitThread : public Thread
        {
            WaitThread (SomCtrlHolder *somCtrl)
                : m_somCtrl(somCtrl) {};

            virtual bool threadLoop()
            {
                if ( OSLAYER_OK != osEventWait( &m_somCtrl->eventFinished ) )
                {
                    TRACE( SOM_CTRL_ITF_ERROR,
                            "%s (wait for snapshot output timed out)\n", __FUNCTION__ );
                }
                return false;
            };

            SomCtrlHolder *m_somCtrl;
        } ;
        android::sp<WaitThread> pwaitThread(new WaitThread( m_pSomCtrl ));
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        pwaitThread->run("pstopThread");
        pwaitThread->join();
        pwaitThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        #endif

        return true;
    }

    return false;
}
