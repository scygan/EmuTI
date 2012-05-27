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
package com.scygan.emuTi;

import java.util.concurrent.Semaphore;
import java.lang.Thread;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.content.res.AssetManager;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.Canvas;

public class EmuTi extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(new EmuTiView(this));  
    }

    /* load our native library */
    static {
        System.loadLibrary("emuti");
    }
}

class EmuTiView extends View {
    private Thread mNativeThread;
    private EmuTiNative mNative;
    final int W = 480;
    final int H = 800;
    
    public EmuTiView(Context context) { super(context); }

    @Override protected void onDraw(Canvas canvas) {
    	if (mNativeThread == null || !mNativeThread.isAlive()) {
    		mNative = new EmuTiNative(W, H, this, getContext().getAssets());
    		Runnable handle = mNative;
    		mNativeThread = new Thread(handle);
    		mNativeThread.start();
    	}
    	mNative.onDraw(canvas);
    }
}


class EmuTiNative implements Runnable {
	private View mParrent;
	private Bitmap mBitmap;
	private AssetManager mAssetManager;
	private final Semaphore mLockDraw = new Semaphore(0);
	private final Semaphore mLockNative = new Semaphore(1);
	
	/* Implemented by libemuti.so */
	private static native void nativeEntry(Bitmap  bitmap, AssetManager assetManager, EmuTiNative emuTiNative);
	
	public void run() {
		try {
			mLockNative.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		nativeEntry(mBitmap, mAssetManager, this);
		//this code is executed only on native code failure
		mLockDraw.release();
	}
		
	public void onDraw(Canvas canvas) { //called from UI thread
		try {
			mLockDraw.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		canvas.drawBitmap(mBitmap, 0, 0, null);		
		mLockNative.release();
	}
	
	public void flip() { //called from native thread
		mParrent.postInvalidate();
		mLockDraw.release();
		try {
			mLockNative.acquire();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public EmuTiNative(int width, int height, View parrent, AssetManager assetManager) {
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.RGB_565);
		mParrent = parrent;
		mAssetManager = assetManager;
	}
}


