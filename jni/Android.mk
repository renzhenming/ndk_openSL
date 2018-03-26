LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := OpenSLAudioPlayer
LOCAL_SRC_FILES := OpenSLAudioPlayer.cpp

# Use WAVLib static library
LOCAL_STATIC_LIBRARIES += wavlib_static

# Link with OpenSL ES
LOCAL_LDLIBS += -lOpenSLES -llog

include $(BUILD_SHARED_LIBRARY)

# Import WAVLib library module,如果不引入的话，wavlib.h是无法使用的
$(call import-module,transcode-1.1.7/avilib )