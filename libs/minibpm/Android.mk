
LOCAL_MODULE := minibpm
LOCAL_MODULE_FILENAME := libminibpm-jni

LOCAL_C_INCLUDES := $(LOCAL_PATH)/minibpm/src 

MINIBPM_PATH := minibpm
MINIBPM_SRC_PATH := $(MINIBPM_PATH)/src

MINIBPM_JNI_FILES := \
	$(MINIBPM_SRC_PATH)/jni/MiniBpmJNI.cpp

MINIBPM_SRC_FILES := \
        $(MINIBPM_SRC_PATH)/MiniBpm.cpp

LOCAL_SRC_FILES += \
	$(MINIBPM_JNI_FILES) \
        $(MINIBPM_SRC_FILES)

LOCAL_CFLAGS_DEBUG := \
	-g \
	-mfloat-abi=softfp

LOCAL_CFLAGS_RELEASE := \
	-O3 \
	-mfpu=neon \
	-mfloat-abi=softfp \
	-ffast-math \
	-ftree-vectorize \
	-ftree-vect-loop-version \
	-freciprocal-math \
	-fsingle-precision-constant \
	-D__ARM_ARCH_7__

LOCAL_CFLAGS := \
	-Wall \
	-I$(MINIBPM_SRC_PATH) \
	-DNO_EXCEPTIONS \
	$(LOCAL_CFLAGS_RELEASE)

LOCAL_LDLIBS += -llog

TARGET_ARCH_ABI	:= armeabi-v7a
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

include $(BUILD_SHARED_LIBRARY)

