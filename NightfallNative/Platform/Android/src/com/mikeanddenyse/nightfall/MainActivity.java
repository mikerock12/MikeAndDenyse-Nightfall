package com.mikeanddenyse.nightfall;
import android.app.NativeActivity;
import android.os.Bundle;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.view.View;
import android.view.WindowManager;
/** The complete C++ frontend owns rendering, input and audio. */
public class MainActivity extends NativeActivity {
    private WifiManager.MulticastLock discoveryLock;
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        WifiManager wifi = (WifiManager)getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wifi != null) {
            discoveryLock = wifi.createMulticastLock("nightfall-discovery");
            discoveryLock.setReferenceCounted(false);
        }
    }
    @Override protected void onResume() {
        super.onResume();
        if (discoveryLock != null && !discoveryLock.isHeld()) discoveryLock.acquire();
    }
    @Override protected void onPause() {
        if (discoveryLock != null && discoveryLock.isHeld()) discoveryLock.release();
        super.onPause();
    }
}
