LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := emuti
LOCAL_SRC_FILES := \
	JNILayer.c \
	EMULib/Console.c \
	EMULib/EMULib.c \
	EMULib/Image.c \
	EMULib/Unix/LibUnix.c \
	Z80/Z80.c \
	Z80/ConDebug.c \
	ATI85/TI85.c \
	ATI85/Unix/Unix.c \
	ATI85/Unix/lodepng.c \
	ATI85/ATI85.c
LOCAL_LDLIBS    := -lm -llog -ljnigraphics -landroid
LOCAL_CFLAGS    := -DATI85 -DLSB_FIRST -DNO_CONDEBUG -DANDROID -include EmuTi.h -O0
include $(BUILD_SHARED_LIBRARY)

