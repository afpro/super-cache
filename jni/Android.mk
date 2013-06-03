ROOT_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := sc
LOCAL_SRC_FILES := \
	main.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/jpeg \
	$(LOCAL_PATH)/png
LOCAL_LDLIBS := -llog -ljnigraphics -lz
LOCAL_C_INCLUDES := $(LOCAL_PATH)/libpng $(LOCAL_PATH)/jpeg
LOCAL_STATIC_LIBRARIES := jpeg png
include $(BUILD_SHARED_LIBRARY)

include $(ROOT_PATH)/libpng/Android.mk
include $(ROOT_PATH)/jpeg/Android.mk
