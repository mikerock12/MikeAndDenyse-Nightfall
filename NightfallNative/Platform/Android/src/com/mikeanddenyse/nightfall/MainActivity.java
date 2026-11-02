package com.mikeanddenyse.nightfall;

import android.app.Activity;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;

public class MainActivity extends Activity implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("nightfall");
    }

    private static native void nativeInit();
    private static native void nativeStep(float dt);
    private static native void nativeTouch(int action, float normX, float normY);

    private SurfaceView surfaceView;
    private volatile boolean running = false;
    private Thread gameThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_FULLSCREEN | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        );
        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);
        nativeInit();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int w = Math.max(1, surfaceView.getWidth());
        int h = Math.max(1, surfaceView.getHeight());
        nativeTouch(event.getActionMasked(), event.getX() / w, event.getY() / h);
        return true;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        running = true;
        gameThread = new Thread(() -> {
            while (running) {
                nativeStep(1.0f / 60.0f);
                try {
                    Thread.sleep(16);
                } catch (InterruptedException ignored) {
                    break;
                }
            }
        }, "NightfallLoop");
        gameThread.start();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        running = false;
    }
}
