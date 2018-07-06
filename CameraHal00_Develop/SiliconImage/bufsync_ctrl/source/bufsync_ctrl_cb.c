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
 * @bufsync_ctrl_cb.c
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include <ebase/types.h>
#include <ebase/trace.h>
#include <ebase/builtins.h>

#include <common/return_codes.h>
#include <common/picture_buffer.h>

#include <bufferpool/media_buffer.h>
#include <bufferpool/media_buffer_pool.h>
#include <bufferpool/media_buffer_queue_ex.h>

#include "bufsync_ctrl_cb.h"
#include "bufsync_ctrl.h"



/******************************************************************************
 * local macro definitions
 *****************************************************************************/
CREATE_TRACER( BUFSYNC_CTRL_CB_INFO , "BUFSYNC-CTRL-CB: ", INFO      , 1 );
CREATE_TRACER( BUFSYNC_CTRL_CB_WARN , "BUFSYNC-CTRL-CB: ", WARNING   , 1 );
CREATE_TRACER( BUFSYNC_CTRL_CB_ERROR, "BUFSYNC-CTRL-CB: ", ERROR     , 1 );

CREATE_TRACER( BUFSYNC_CTRL_CB_DEBUG, "", INFO, 0 );

