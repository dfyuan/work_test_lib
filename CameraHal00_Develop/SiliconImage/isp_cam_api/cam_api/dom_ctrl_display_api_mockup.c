#include <ebase/types.h>
#include <ebase/trace.h>

#include <oslayer/oslayer.h>
#include "cam_api/dom_ctrl_vidplay_api.h"
#if defined(ANDROID_5_X)
#include "../../CameraHal5.x/common_type.h"
#else
#include "../../CameraHal4.4/common_type.h"
#endif
CREATE_TRACER(CAM_API_DOMDISPLAYMOCKUP_INFO , "DOMDISPLAYMOCKUP: ", INFO,    0);
CREATE_TRACER(CAM_API_DOMDISPLAYMOCKUP_WARN , "DOMDISPLAYMOCKUP: ", WARNING, 1);
CREATE_TRACER(CAM_API_DOMDISPLAYMOCKUP_ERROR, "DOMDISPLAYMOCKUP: ", ERROR,   1);

CREATE_TRACER(CAM_API_DOMDISPLAYMOCKUP_DEBUG, "DOMDISPLAYMOCKUP: ", INFO,    0);

typedef int DisplayAdapter;

typedef struct domCtrlVidplayHandle_s
{
    func_displayCBForIsp pDisplayAdapter;
}domCtrlVidplayContext_t;

RESULT domCtrlVidplayInit
(
    domCtrlVidplayConfig_t *pConfig                 //!< Reference to configuration structure.
){
    domCtrlVidplayContext_t *pDomCtrlVidplayContext;
    if (pConfig == NULL)
    {
        return RET_NULL_POINTER;
    }

    // allocate control context
    pDomCtrlVidplayContext = (domCtrlVidplayContext_t*)malloc( sizeof(domCtrlVidplayContext_t) );
    if (pDomCtrlVidplayContext == NULL)
    {
        //TRACE(""  ,"%s: allocating control context failed.\n", __FUNCTION__ );
        return RET_OUTOFMEM;
    }
    memset( pDomCtrlVidplayContext, 0, sizeof(domCtrlVidplayContext_t) );
    pDomCtrlVidplayContext->pDisplayAdapter = (func_displayCBForIsp)(pConfig->hParent);
    // success, so let's return the control context handle
    pConfig->domCtrlVidplayHandle = (domCtrlVidplayHandle_t) pDomCtrlVidplayContext;

    return 0;
}

RESULT domCtrlVidplayShutDown
(
    domCtrlVidplayHandle_t  domCtrlVidplayHandle    //!< Handle to dom control videoplayer context as returned by @ref domCtrlVidplayInit.
){
    RESULT result = RET_SUCCESS;

    TRACE(CAM_API_DOMDISPLAYMOCKUP_INFO, "%s (enter)\n", __FUNCTION__ );

    if (domCtrlVidplayHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context
    domCtrlVidplayContext_t *pDomCtrlVidplayContext = (domCtrlVidplayContext_t*)domCtrlVidplayHandle;
    // free context
    free( pDomCtrlVidplayContext );

    TRACE( CAM_API_DOMDISPLAYMOCKUP_INFO, "%s (exit)\n", __FUNCTION__ );

    return 0;
}

//send frame to HAL  display thread
 RESULT domCtrlVidplayDisplay
(
    domCtrlVidplayHandle_t  domCtrlVidplayHandle,   //!< Handle to dom control videoplayer context as returned by @ref domCtrlVidplayInit.
    PicBufMetaData_t        *pPicBufMetaData        //!< The meta data describing the image buffer to display (includes the data pointers as well...).
){

    TRACE(CAM_API_DOMDISPLAYMOCKUP_INFO,"%s (enter)\n", __FUNCTION__ );

    if (domCtrlVidplayHandle == NULL)
    {
        return RET_NULL_POINTER;
    }

    // get context
    domCtrlVidplayContext_t *pDomCtrlVidplayContext = (domCtrlVidplayContext_t*)domCtrlVidplayHandle;

    //get DisplayAdapter
    if(pDomCtrlVidplayContext){
        func_displayCBForIsp pCurDisplayAdapter = pDomCtrlVidplayContext->pDisplayAdapter;
    

    }

    return 0;
}
 RESULT domCtrlVidplayClear
(
    domCtrlVidplayHandle_t  domCtrlVidplayHandle    //!< Handle to dom control videoplayer context as returned by @ref domCtrlVidplayInit.
){
    return 0;
}

 RESULT domCtrlVidplayShow
(
    domCtrlVidplayHandle_t  domCtrlVidplayHandle,   //!< Handle to dom control videoplayer context as returned by @ref domCtrlVidplayInit.
    bool_t                  show
){
    return 0;
}

 RESULT domCtrlVidplaySetOverlayText
(
    domCtrlVidplayHandle_t  domCtrlVidplayHandle,   //!< Handle to dom control videoplayer context as returned by @ref domCtrlVidplayInit.
    char                    *szOverlayText          //!< Pointer to zero terminated string holding performance data to display; may be NULL.
){
    return 0;
}


