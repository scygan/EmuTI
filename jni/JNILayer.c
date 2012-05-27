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
jclass g_EmuTiNativeCls;
jobject g_EmuTiNativeObj;
jmethodID g_FlipMid;


void jni_FlipImage() {
	(*g_JNIEnv)->CallVoidMethod(g_JNIEnv, g_EmuTiNativeObj, g_FlipMid);
}

JNIEXPORT void JNICALL Java_com_scygan_emuTi_EmuTiNative_nativeEntry(JNIEnv * env, jobject  obj, jobject bitmap, jobject assetManager, jobject emuTiNative) {
    int ret;

    g_MainBitmap = bitmap;
    g_JNIEnv = env;
    g_AAssetManager =  AAssetManager_fromJava(g_JNIEnv, assetManager);
    g_EmuTiNativeObj = (*g_JNIEnv)->NewGlobalRef(g_JNIEnv, emuTiNative);
    g_EmuTiNativeCls = (*g_JNIEnv)->GetObjectClass(g_JNIEnv, g_EmuTiNativeObj);
    g_FlipMid = (*g_JNIEnv)->GetMethodID(g_JNIEnv, g_EmuTiNativeCls, "flip", "()V");
    if (!g_FlipMid) {
    	LOGE("Cannot locate needed java callbacks from native code.");
    	return;
    }

#ifdef DEBUG
    CPU.Trap  = 0xFFFF;
    CPU.Trace = 0;
#endif

    Mode=(Mode&~ATI_MODEL)|ATI_TI85;

    if(!InitMachine()) return;
        StartTI85(/*RAMName*/NULL);
        TrashTI85();
        TrashMachine();
}

