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
extern int jni_GetTouch(int* x, int* y, int* pressed);
extern int jni_IsExiting();
void   jni_SaveSTA(void* STA, size_t size);
size_t jni_LoadSTA(void* STA, size_t size);

