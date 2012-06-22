/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ATI85/TI85.h"
#include "EMULib/EMULib.h"

jobject g_MainBitmap;
JNIEnv * g_JNIEnv;
AAssetManager* g_AAssetManager;
jobject g_EmuTiNativeObj;
jmethodID g_FlipMid, g_GetTouchMid, g_SaveSTAMid, g_LoadSTAMid;
jfieldID g_MotionEventXFid, g_MotionEventYFid, g_MotionEventPressedFid;
int g_ForceExit;


void jni_FlipImage() {
	(*g_JNIEnv)->CallVoidMethod(g_JNIEnv, g_EmuTiNativeObj, g_FlipMid);
}

int jni_GetTouch(int* x, int* y, int* pressed) {
	//LOGE("Entered jni_GetTouch");
	jobject jTouchEvent = (*g_JNIEnv)->CallObjectMethod(g_JNIEnv, g_EmuTiNativeObj, g_GetTouchMid);
	if (!jTouchEvent) {
		//LOGE("No pending events");
		return 0;
	}
	*x = (*g_JNIEnv)->GetIntField(g_JNIEnv, jTouchEvent, g_MotionEventXFid);
	*y = (*g_JNIEnv)->GetIntField(g_JNIEnv, jTouchEvent, g_MotionEventYFid);
	*pressed = (*g_JNIEnv)->GetBooleanField(g_JNIEnv, jTouchEvent, g_MotionEventPressedFid);
	//LOGE("Warning: touched point %d, %d, status (%d).", *x, *y, *pressed);
	return 1;
}

int jni_IsExiting() {
	return g_ForceExit;
}

void jni_SaveSTA(void* STA, size_t size) {
    jbyteArray a = (*g_JNIEnv)->NewByteArray(g_JNIEnv, size);
    (*g_JNIEnv)->SetByteArrayRegion(g_JNIEnv, a, 0, 
                size, (jbyte*)STA);
    (*g_JNIEnv)->CallVoidMethod(g_JNIEnv, g_EmuTiNativeObj, g_SaveSTAMid, a);
    (*g_JNIEnv)->DeleteLocalRef(g_JNIEnv, a);
}

size_t jni_LoadSTA(void* STA, size_t size) {
    size_t ret; 
    jbyteArray a = (*g_JNIEnv)->CallObjectMethod(g_JNIEnv, g_EmuTiNativeObj, g_LoadSTAMid);
		if (!a) return 0;
    ret = (*g_JNIEnv)->GetArrayLength(g_JNIEnv, a);
    if (ret != size) {
        LOGE("Warning: jni_LoadSta: expected to get %d bytes, got %d.", size, ret);
        if (ret > size)
            ret = size;
    }
    if (ret)
        (*g_JNIEnv)->GetByteArrayRegion(g_JNIEnv, a, 0, ret, (jbyte*)STA);
    return ret;
}

JNIEXPORT void JNICALL Java_com_scygan_emuTi_EmuTiNative_nativeEntry(JNIEnv * env, jobject  obj, jobject bitmap, jobject assetManager, jobject emuTiNative) {
    int ret;
    jclass emuTiNativeCls, touchEventCls;

    g_MainBitmap = bitmap;
    g_JNIEnv = env;
    g_AAssetManager =  AAssetManager_fromJava(g_JNIEnv, assetManager);
    g_EmuTiNativeObj = (*g_JNIEnv)->NewGlobalRef(g_JNIEnv, emuTiNative);
    emuTiNativeCls = (*g_JNIEnv)->GetObjectClass(g_JNIEnv, g_EmuTiNativeObj);
    g_FlipMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, emuTiNativeCls, "flip", "()V");
    g_GetTouchMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, emuTiNativeCls, "getTouch", "()Lcom/scygan/emuTi/TouchEvent;");
    g_SaveSTAMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, emuTiNativeCls, "saveSTA", "([B)V");
    g_LoadSTAMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, emuTiNativeCls, "loadSTA", "()[B");

	if (!g_FlipMid || !g_GetTouchMid || !g_SaveSTAMid || !g_LoadSTAMid) {
		LOGE("Cannot locate needed java callbacks from native code.");
		return;
	}
    touchEventCls = (*g_JNIEnv)->FindClass(g_JNIEnv, "com/scygan/emuTi/TouchEvent");
    g_MotionEventXFid = (*g_JNIEnv)->GetFieldID(g_JNIEnv, touchEventCls, "x", "I");
    g_MotionEventYFid = (*g_JNIEnv)->GetFieldID(g_JNIEnv, touchEventCls, "y", "I");
    g_MotionEventPressedFid = (*g_JNIEnv)->GetFieldID(g_JNIEnv, touchEventCls, "pressed", "Z");
	if (!g_MotionEventXFid || !g_MotionEventYFid || !g_MotionEventPressedFid) {
		LOGE("Cannot locate needed java class fields from native code.");
		return;
	}


#ifdef DEBUG
    CPU.Trap  = 0xFFFF;
    CPU.Trace = 0;
#endif

    g_ForceExit = 0;

    Mode=(Mode&~ATI_MODEL)|ATI_TI85;

    if(!InitMachine()) return;
        StartTI85(/*RAMName*/NULL);
        TrashTI85();
        TrashMachine();
}


JNIEXPORT void JNICALL Java_com_scygan_emuTi_EmuTiNative_nativeStop() {
	g_ForceExit = 1;
}

