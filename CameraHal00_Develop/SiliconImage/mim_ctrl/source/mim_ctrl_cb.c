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
 * @mim_ctrl_cb.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/

#include <ebase/trace.h>
#include <common/return_codes.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include "mim_ctrl_cb.h"
#include "mim_ctrl.h"


/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( MIM_CTRL_CB_INFO , "MIM-CTRL-CB: ", INFO      , 0 );
CREATE_TRACER( MIM_CTRL_CB_WARN , "MIM-CTRL-CB: ", WARNING   , 1 );
CREATE_TRACER( MIM_CTRL_CB_ERROR, "MIM-CTRL-CB: ", ERROR     , 1 );



/******************************************************************************
 * MimCtrlDmaCompletionCb()
 *****************************************************************************/
void MimCtrlDmaCompletionCb
(
	const CamerIcCommandId_t    cmdId,
	const RESULT                result,
	void                        *pParam,
	void                        *pUserContext
)
{
    MimCtrlContext_t *pMimCtrlCtx = (MimCtrlContext_t *)pUserContext;

    TRACE( MIM_CTRL_CB_INFO, "%s: (enter)\n", __FUNCTION__ );

    if ( (pMimCtrlCtx != NULL) && (pParam != NULL) )
    {
        switch ( cmdId )
        {
            case CAMERIC_MI_COMMAND_DMA_TRANSFER:
                {
                    RESULT result = MimCtrlSendCommand( pMimCtrlCtx, MIM_CTRL_CMD_DMA_TRANSFER );
                    DCT_ASSERT( (result == RET_SUCCESS) );

                    pMimCtrlCtx->dmaResult = result;
                    break;
                }

            default:
                {
                     TRACE( MIM_CTRL_CB_WARN, "%s: (unsupported command)\n", __FUNCTION__ );
                      break;
                }
        }
    }

    TRACE( MIM_CTRL_CB_INFO, "%s: (exit)\n", __FUNCTION__ );
}

