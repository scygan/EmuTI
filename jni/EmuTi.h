#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <android/asset_manager_jni.h>

#define  LOG_TAG    "EmuTI [native]"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

extern JNIEnv * g_JNIEnv;
extern jobject g_MainBitmap;
extern AAssetManager* g_AAssetManager;
extern void jni_FlipImage();
