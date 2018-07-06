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
 * @file vom_ctrl_interface.cpp
 *
 * @brief
 *   Implementation of VOM (Video Output Module) C++ API.
 *
 *****************************************************************************/
#include "cam_api/vom_ctrl_interface.h"

#include <ebase/trace.h>
#include <ebase/builtins.h>
#include <ebase/dct_assert.h>

#include <vom_ctrl/vom_ctrl_api.h>

#include <utils/threads.h>
using android::Thread;

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(VOM_CTRL_ITF_INFO , "VOM_CTRL_ITF: ", INFO,  0);
CREATE_TRACER(VOM_CTRL_ITF_ERROR, "VOM_CTRL_ITF: ", ERROR, 1);

#define VOM_CTRL_ITF_TIMEOUT 30000U // 30 s

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
 * VomCtrlItf::VomCtrlHolder
 *****************************************************************************/
class VomCtrlItf::VomCtrlHolder
{
public:
    VomCtrlHolder( HalHandle_t hal );
    ~VomCtrlHolder();

private:
    VomCtrlHolder (const VomCtrlHolder& other);
    VomCtrlHolder& operator = (const VomCtrlHolder& other);

public:
    bool init( bool enable3D, Cea861VideoFormat_t format );
    void shutDown();

    static void cbCompletion( vomCtrlCmdId_t cmdId, RESULT result, const void* pUserCtx );

public:
    vomCtrlHandle_t         hCtrl;
    HalHandle_t 			hHal;
    State                   state;

    osEvent                 eventStart;
    osEvent                 eventStop;
};


/******************************************************************************
 * VomCtrlHolder
 *****************************************************************************/
VomCtrlItf::VomCtrlHolder::VomCtrlHolder( HalHandle_t hal )
    : hCtrl(NULL),
      hHal(hal),
      state(Idle)
{
	DCT_ASSERT( NULL != hHal );
}


/******************************************************************************
 * ~VomCtrlHolder
 *****************************************************************************/
VomCtrlItf::VomCtrlHolder::~VomCtrlHolder()
{
    if( NULL != hCtrl )
    {
        if ( RET_SUCCESS != vomCtrlShutDown( hCtrl ) )
        {
            TRACE( VOM_CTRL_ITF_ERROR, "%s (can't shutdown video output module)\n", __FUNCTION__ );
        }

        (void)osEventDestroy( &eventStart );
        (void)osEventDestroy( &eventStop  );
    }
}


/******************************************************************************
 * VomCtrlHolder::cbCompletion
 *****************************************************************************/
void VomCtrlItf::VomCtrlHolder::cbCompletion( vomCtrlCmdId_t cmdId, RESULT result, const void* pUserCtx )
{
    VomCtrlItf::VomCtrlHolder *pCtrlHolder = (VomCtrlItf::VomCtrlHolder *)pUserCtx;

    if ( pCtrlHolder != NULL )
    {
        switch ( cmdId )
        {
            case VOM_CTRL_CMD_START:
            {
                (void)osEventSignal( &pCtrlHolder->eventStart );
                break;
            }

            case VOM_CTRL_CMD_STOP:
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
 * VomCtrlHolder::init
 *****************************************************************************/
bool VomCtrlItf::VomCtrlHolder::init( bool enable3D, Cea861VideoFormat_t format )
{
	if ( NULL != hCtrl )
	{
		shutDown();
	}
	DCT_ASSERT( NULL == hCtrl );

    // initialize vom ctrl
    vomCtrlConfig_t ctrlConfig;
    MEMSET( &ctrlConfig, 0, sizeof( vomCtrlConfig_t ) );

    ctrlConfig.MaxPendingCommands   = 10;
    ctrlConfig.MaxBuffers           = 1;
    ctrlConfig.vomCbCompletion      = cbCompletion;
    ctrlConfig.pUserContext         = (void *)this;
    ctrlConfig.HalHandle            = hHal;
    ctrlConfig.Enable3D             = enable3D ? BOOL_TRUE : BOOL_FALSE;
    ctrlConfig.VideoFormat          = format;
    ctrlConfig.VideoFormat3D        = HDMI_3D_VIDEOFORMAT_FRAME_PACKING;
    ctrlConfig.VomCtrlHandle        = NULL;

    if ( RET_SUCCESS != vomCtrlInit( &ctrlConfig ) )
    {
        TRACE( VOM_CTRL_ITF_ERROR, "%s (can't create the video output module)\n", __FUNCTION__ );
        return false;
    }

    // create events
    if ( OSLAYER_OK != osEventInit( &eventStart, 1, 0 ) )
    {
        TRACE( VOM_CTRL_ITF_ERROR, "%s (can't create start event)\n", __FUNCTION__ );
        (void)vomCtrlShutDown( ctrlConfig.VomCtrlHandle );
        return false;
    }

    if ( OSLAYER_OK != osEventInit( &eventStop, 1, 0 ) )
    {
        TRACE( VOM_CTRL_ITF_ERROR, "%s (can't create stop event)\n", __FUNCTION__ );
        (void)osEventDestroy( &eventStart );
        (void)vomCtrlShutDown( ctrlConfig.VomCtrlHandle );
        return false;
    }

    hCtrl = ctrlConfig.VomCtrlHandle;
    return true;
}


/******************************************************************************
 * VomCtrlHolder::shutDown
 *****************************************************************************/
void VomCtrlItf::VomCtrlHolder::shutDown()
{
    if( NULL != hCtrl )
    {
        if ( RET_SUCCESS != vomCtrlShutDown( hCtrl ) )
        {
            TRACE( VOM_CTRL_ITF_ERROR, "%s (can't shutdown video output module)\n", __FUNCTION__ );
        }

        (void)osEventDestroy( &eventStart );
        (void)osEventDestroy( &eventStop  );
        hCtrl = NULL;
    }
}


/******************************************************************************
 * VomCtrlItf
 *****************************************************************************/
VomCtrlItf::VomCtrlItf( HalHandle_t hHal )
	: m_pVomCtrl ( new VomCtrlHolder( hHal ) )
{
	DCT_ASSERT( NULL != m_pVomCtrl );
}


/******************************************************************************
 * ~VomCtrlItf
 *****************************************************************************/
VomCtrlItf::~VomCtrlItf()
{
    if ( NULL != m_pVomCtrl )
    {
        stop();
    	delete m_pVomCtrl;
    }
}


/******************************************************************************
 * state
 *****************************************************************************/
VomCtrlItf::State
VomCtrlItf::state() const
{
    return m_pVomCtrl->state;
}


/******************************************************************************
 * showBuffer
 *****************************************************************************/
void
VomCtrlItf::bufferCb( MediaBuffer_t *pBuffer )
{
    vomCtrlShowBuffer( m_pVomCtrl->hCtrl, pBuffer );
}


/******************************************************************************
 * start
 *****************************************************************************/
bool
VomCtrlItf::start( bool enable3D, Cea861VideoFormat_t format )
{
    if ( Idle == m_pVomCtrl->state)
    {
        #if 0
        struct StartThread : public QThread
        {
            StartThread (VomCtrlHolder *vomCtrl)
                : m_vomCtrl(vomCtrl) {};

            virtual void run()
            {
                RESULT result = vomCtrlStart( m_vomCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_vomCtrl->eventStart, VOM_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( VOM_CTRL_ITF_ERROR,
                                "%s (start video output timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( VOM_CTRL_ITF_ERROR,
                            "%s (can't start video output)\n", __FUNCTION__ );
                }
            };

            VomCtrlHolder *m_vomCtrl;
        } startThread( m_pVomCtrl );

        QEventLoop loop;
        if ( true == QObject::connect( &startThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
        {
        	if ( true == m_pVomCtrl->init( enable3D, format ) )
        	{
				startThread.start();
				loop.exec();

				m_pVomCtrl->state = Running;
        	}
        }
        #else
        struct StartThread : public Thread
        {
            StartThread (VomCtrlHolder *vomCtrl)
                : m_vomCtrl(vomCtrl) {};

            virtual bool threadLoop()
            {
                RESULT result = vomCtrlStart( m_vomCtrl->hCtrl );
                if ( RET_PENDING == result )
                {
                    if ( OSLAYER_OK != osEventTimedWait( &m_vomCtrl->eventStart, VOM_CTRL_ITF_TIMEOUT ) )
                    {
                        TRACE( VOM_CTRL_ITF_ERROR,
                                "%s (start video output timed out)\n", __FUNCTION__ );
                    }
                }
                else if ( RET_SUCCESS != result )
                {
                    TRACE( VOM_CTRL_ITF_ERROR,
                            "%s (can't start video output)\n", __FUNCTION__ );
                }
                return false;
            };

            VomCtrlHolder *m_vomCtrl;
        } ;

        android::sp<StartThread> pstartThread(new StartThread(m_pVomCtrl));
    	if ( true == m_pVomCtrl->init( enable3D, format ) )
    	{
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
            pstartThread->run("pstartThread");
            pstartThread->join();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
			m_pVomCtrl->state = Running;
    	}
        pstartThread.clear();
        #endif
    }

    return Running == m_pVomCtrl->state ? true : false;
}


/******************************************************************************
 * stop
 *****************************************************************************/
bool
VomCtrlItf::stop()
{
	if ( Running == m_pVomCtrl->state)
	{
      #if 0
		struct StopThread : public QThread
		{
			StopThread (VomCtrlHolder *vomCtrl) : m_vomCtrl (vomCtrl) {};

			virtual void run()
			{
				RESULT result = vomCtrlStop( m_vomCtrl->hCtrl );
				if ( RET_PENDING == result )
				{
					if ( OSLAYER_OK != osEventTimedWait( &m_vomCtrl->eventStop, VOM_CTRL_ITF_TIMEOUT ) )
					{
						TRACE( VOM_CTRL_ITF_ERROR,
								"%s (stop video output timed out)\n", __FUNCTION__ );
					}
				}
				else if ( RET_SUCCESS != result )
				{
					TRACE( VOM_CTRL_ITF_ERROR,
							"%s (can't stop video output)\n", __FUNCTION__ );
				}
			};

			VomCtrlHolder *m_vomCtrl;
		} stopThread( m_pVomCtrl );

		QEventLoop loop;
		if ( true == QObject::connect( &stopThread, SIGNAL( finished() ), &loop, SLOT( quit() ) ) )
		{
			stopThread.start();
			loop.exec();
			m_pVomCtrl->shutDown();

			m_pVomCtrl->state = Idle;
		}
        #else
		struct StopThread : public Thread
		{
			StopThread (VomCtrlHolder *vomCtrl) : m_vomCtrl (vomCtrl) {};

            virtual bool threadLoop()
			{
				RESULT result = vomCtrlStop( m_vomCtrl->hCtrl );
				if ( RET_PENDING == result )
				{
					if ( OSLAYER_OK != osEventTimedWait( &m_vomCtrl->eventStop, VOM_CTRL_ITF_TIMEOUT ) )
					{
						TRACE( VOM_CTRL_ITF_ERROR,
								"%s (stop video output timed out)\n", __FUNCTION__ );
					}
				}
				else if ( RET_SUCCESS != result )
				{
					TRACE( VOM_CTRL_ITF_ERROR,
							"%s (can't stop video output)\n", __FUNCTION__ );
				}
                return false;
			};

			VomCtrlHolder *m_vomCtrl;
		} ;
        android::sp<StopThread> pstopThread(new StopThread( m_pVomCtrl ));
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
        pstopThread->run("pstartThread");
        pstopThread->join();
        pstopThread.clear();
        TRACE(NULL,"%s(%d)",__FUNCTION__,__LINE__);
		m_pVomCtrl->shutDown();

		m_pVomCtrl->state = Idle;
        #endif
	}

    return Idle == m_pVomCtrl->state ? true : false;
}
