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
 * @file halholder.cpp
 *
 * @brief
 *   Implementation of Hal Holder C++ API.
 *
 *****************************************************************************/
#include "cam_api/halholder.h"

#include <ebase/trace.h>

/******************************************************************************
 * local macro definitions
 *****************************************************************************/

CREATE_TRACER(CAM_API_HALHOLDER_INFO , "CAM_API_HALHOLDER: ", INFO,  0);
CREATE_TRACER(CAM_API_HALHOLDER_ERROR, "CAM_API_HALHOLDER: ", ERROR, 1);

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

HalHandle_t HalHolder::m_hHal = NULL;
HalHolder* HalHolder::m_halHolder  = NULL;

/******************************************************************************
 * HalHolder
 *****************************************************************************/
#if 0
HalHolder::HalHolder()
{
    // open HAL, issues general board reset as well
    HalHandle_t hHal = HalOpen();
    if ( NULL != hHal )
    {
        m_hHal = hHal;

        // reference marvin software HAL
        if ( RET_SUCCESS != HalAddRef( m_hHal ) )
        {
            TRACE( CAM_API_HALHOLDER_ERROR, "%s (can't reference HAL)\n", __FUNCTION__ );
        }
    }

    if ( NULL == m_hHal )
    {
        TRACE( CAM_API_HALHOLDER_ERROR, "%s (can't open HAL)\n", __FUNCTION__ );
    }
}
#endif

HalHolder::HalHolder(char* dev_filename, HalPara_t *para)
{
    // open HAL, issues general board reset as well
    HalHandle_t hHal = HalOpen(dev_filename,para);
    if ( NULL != hHal )
    {
        m_hHal = hHal;

        // reference marvin software HAL
        if ( RET_SUCCESS != HalAddRef( m_hHal ) )
        {
            TRACE( CAM_API_HALHOLDER_ERROR, "%s (can't reference HAL)\n", __FUNCTION__ );
        }
    }

    if ( NULL == m_hHal )
    {
        TRACE( CAM_API_HALHOLDER_ERROR, "%s (can't open HAL)\n", __FUNCTION__ );
    }
}



/******************************************************************************
 * ~HalHolder
 *****************************************************************************/
HalHolder::~HalHolder()
{
    if ( NULL != m_hHal )
    {

        // dereference marvin software HAL
        if ( RET_SUCCESS != HalDelRef( m_hHal ) )
        {
            TRACE( CAM_API_HALHOLDER_ERROR, "%s (can't dereference HAL)\n", __FUNCTION__ );
        }

        // close HAL, closes low level driver if ref count reached zero
        if ( RET_SUCCESS != HalClose( m_hHal ) )
        {
            TRACE( CAM_API_HALHOLDER_ERROR, "%s (can't close HAL)\n", __FUNCTION__ );
        }

        m_hHal = NULL;
    }
}

/******************************************************************************
 * handle
 *****************************************************************************/
#if 0
HalHandle_t HalHolder::handle()
{
    // not thread safe
   // static HalHolder instance;
   if(m_halHolder == NULL){
       m_halHolder = new  HalHolder();
   }
    return m_halHolder->m_hHal;
}
#endif

HalHandle_t HalHolder::handle(char* dev_filename, HalPara_t *para)
{
    // not thread safe
   // static HalHolder instance;

       m_halHolder = new  HalHolder(dev_filename,para);

    return m_halHolder->m_hHal;
}


/******************************************************************************
 * handle
 *****************************************************************************/
int HalHolder::NumOfCams()
{
    uint32_t no = 0;

	#if 0 
    RESULT result = HalNumOfCams( handle(), &no );
    if ( RET_SUCCESS != result )
    {
        no = 1UL;
    }
	#endif

    return ( (int)no );
}


