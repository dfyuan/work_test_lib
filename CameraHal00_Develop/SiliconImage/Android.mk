LOCAL_PATH:= $(call my-dir)

#include $(CLEAR_VARS)

#get all "include" directories
all_include_dir := $(shell find $(LOCAL_PATH) -name "include")


#need to create "include" dir in local path?
ifeq ($(strip $(filter $(LOCAL_PATH)/include,$(all_include_dir))),)
$(shell mkdir $(LOCAL_PATH)/include)
endif

#get various path
full_path := $(shell pwd)/$(LOCAL_PATH)
include_dir_except_curinclud := $(filter-out $(LOCAL_PATH)/include%,$(all_include_dir))
include_dir_except_curinclud_dir := $(patsubst %/,%,$(dir $(include_dir_except_curinclud)))
include_dir_except_curinclud_dir_except_localpatch := $(patsubst $(LOCAL_PATH)%,%,$(include_dir_except_curinclud_dir))


include_dir_name := $(notdir $(include_dir_except_curinclud_dir))

cur_include_dri := $(filter-out ./,$(dir $(shell ls -F $(LOCAL_PATH)/include)))

#create neccesary dir in "$(LOCAL_PATH)/include"
define func_mkdir
$(if $(findstring  $(strip $(1)),$(cur_include_dri)),,$(shell mkdir $(LOCAL_PATH)/include/$(1)))
endef

$(foreach n,$(include_dir_name),$(call func_mkdir,$(n)))

#copy ".h" files 
define func_copy
$(shell rm $(LOCAL_PATH)/include/$(notdir $(1))/* -rf )\
$(shell cp $(full_path)$(1)/include/* $(LOCAL_PATH)/include/$(notdir $(1))/ -rf)
endef

$(foreach m,$(include_dir_except_curinclud_dir_except_localpatch),$(call func_copy,$(m)))

include $(call all-subdir-makefiles)
