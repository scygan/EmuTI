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

import java.util.*;
import java.util.concurrent.Semaphore;
import java.lang.Thread;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.content.res.AssetManager;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.View.OnTouchListener;
import android.graphics.*;

public class EmuTi extends Activity
{
	EmuTiView emutiView;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
    	requestWindowFeature(Window.FEATURE_NO_TITLE);
        super.onCreate(savedInstanceState);
        emutiView = new EmuTiView(this);
        setContentView(emutiView);
        emutiView.requestFocus();
    }

    /* load our native library */
    static {
        System.loadLibrary("emuti");
    }
}

class EmuTiView extends View implements OnTouchListener {
    private Thread mNativeThread;
    private EmuTiNative mNative;
    final int W = 480;
    final int H = 800;
    
    public EmuTiView(Context context) {
    	super(context);
    	this.setClickable(true);
    	this.setOnTouchListener(this);
    }

    @Override protected void onDraw(Canvas canvas) {
    	if (mNativeThread == null || !mNativeThread.isAlive()) {
    		mNative = new EmuTiNative(W, H, this, getContext().getAssets());
    		Runnable handle = mNative;
    		mNativeThread = new Thread(handle);
    		mNativeThread.start();
    	}
    	mNative.onDraw(canvas);
    }
    public boolean onTouch(View v, MotionEvent event) {
    	if (mNativeThread != null  && mNativeThread.isAlive()) {
    		return mNative.onTouch(event);
    	}
    	return false;    	
    }
}

class TouchEvent {
	public TouchEvent(int x, int y, boolean pressed){
		this.x = x;
		this.y = y;
		this.pressed = pressed;
	}
	int x, y;
	boolean pressed;
}

class EmuTiNative implements Runnable {
	private View mParrent;
	private Bitmap mBitmap;
	private AssetManager mAssetManager;
	private Semaphore mLockDraw = new Semaphore(0);
	private Semaphore mLockNative = new Semaphore(1);
	private Semaphore mLockEvent = new Semaphore(1);
	private Queue<MotionEvent> mMotionEventQueue = new LinkedList<MotionEvent>();
	
    /* Implemented by libemuti.so */
	private static native void nativeEntry(Bitmap  bitmap, AssetManager assetManager, EmuTiNative emuTiNative);
	
	public boolean onTouch(MotionEvent event) {
		if (event.getAction() == MotionEvent.ACTION_UP || event.getAction() == MotionEvent.ACTION_DOWN) {
			try {
				mLockEvent.acquire();
				mMotionEventQueue.add(event);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			mLockEvent.release();
			return true;
		}
		return false;
	}

	/* Called by libemuti.so */
	public TouchEvent getTouch() { //called from native thread
		TouchEvent ret = null;
		try {
			mLockEvent.acquire();
			if (!mMotionEventQueue.isEmpty()) {
				MotionEvent event = mMotionEventQueue.remove();
				ret = new TouchEvent((int)event.getX(0), (int)event.getY(0), event.getAction() == MotionEvent.ACTION_DOWN);
			}
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}				
		mLockEvent.release();
		return ret; 
	}
	
	/* Called by libemuti.so */
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

	public EmuTiNative(int width, int height, View parrent, AssetManager assetManager) {
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
		mParrent = parrent;
		mAssetManager = assetManager;
	}
}


