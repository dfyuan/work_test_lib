#
# RockChip Camera HAL 
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
	embUnit/AssertImpl.c\
	embUnit/RepeatedTest.c\
	embUnit/stdImpl.c\
	embUnit/TestCaller.c\
	embUnit/TestCase.c\
	embUnit/TestResult.c\
	embUnit/TestRunner.c\
	embUnit/TestSuite.c\
	textui/CompilerOutputter.c\
	textui/ExtXMLOutputter.c\
	textui/TextOutputter.c\
	textui/TextUIRunner.c\
	textui/XMLOutputter.c
  

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/embUnit\
	$(LOCAL_PATH)/textui\


LOCAL_CFLAGS := -Wall -Wextra -std=c99   -Wformat-nonliteral -g -O0 -DDEBUG -pedantic -Wno-error=unused-function
LOCAL_CFLAGS += -DLINUX  -DMIPI_USE_CAMERIC -DHAL_MOCKUP -DCAM_ENGINE_DRAW_DOM_ONLY -D_FILE_OFFSET_BITS=64 -DHAS_STDINT_H
#LOCAL_SHARED_LIBRARIES := libc

#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= libisp_embUnit

LOCAL_MODULE_TAGS:= optional
include $(BUILD_STATIC_LIBRARY)
