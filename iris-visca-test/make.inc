LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= iris_test
LOCAL_SRCS	:= 
		   $(LOCAL_PATH)/motor.c \
		   $(LOCAL_PATH)/visca.c \
		   $(LOCAL_PATH)/main.c
	   
LOCAL_LDFLAGS   := -lpthread -lm 

LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer \
		   -I$(LOCAL_PATH)/include \
		   -I$(AMB_TOPDIR)/include \
		   -I$(AMB_TOPDIR)/packages/textinsert/

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

