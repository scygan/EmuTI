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
jclass g_EmuTiNativeCls, g_TouchEventCls;
jobject g_EmuTiNativeObj;
jmethodID g_FlipMid, g_GetTouchMid;
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
	LOGE("Warning: touched point %d, %d, status (%d).", *x, *y, *pressed);
	return 1;
}

JNIEXPORT void JNICALL Java_com_scygan_emuTi_EmuTiNative_nativeEntry(JNIEnv * env, jobject  obj, jobject bitmap, jobject assetManager, jobject emuTiNative) {
    int ret;
    g_MainBitmap = bitmap;
    g_JNIEnv = env;
    g_AAssetManager =  AAssetManager_fromJava(g_JNIEnv, assetManager);
    g_EmuTiNativeObj = (*g_JNIEnv)->NewGlobalRef(g_JNIEnv, emuTiNative);
    g_EmuTiNativeCls = (*g_JNIEnv)->GetObjectClass(g_JNIEnv, g_EmuTiNativeObj);
    g_FlipMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, g_EmuTiNativeCls, "flip", "()V");
    g_GetTouchMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, g_EmuTiNativeCls, "getTouch", "()Lcom/scygan/emuTi/TouchEvent;");
	if (!g_FlipMid || !g_GetTouchMid) {
		LOGE("Cannot locate needed java callbacks from native code.");
		return;
	}
    g_TouchEventCls = (*g_JNIEnv)->FindClass(g_JNIEnv, "com/scygan/emuTi/TouchEvent");
    g_MotionEventXFid = (*g_JNIEnv)->GetFieldID(g_JNIEnv, g_TouchEventCls, "x", "I");
    g_MotionEventYFid = (*g_JNIEnv)->GetFieldID(g_JNIEnv, g_TouchEventCls, "y", "I");
    g_MotionEventPressedFid = (*g_JNIEnv)->GetFieldID(g_JNIEnv, g_TouchEventCls, "pressed", "Z");
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

