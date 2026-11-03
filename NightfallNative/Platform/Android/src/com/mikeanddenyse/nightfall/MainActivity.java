package com.mikeanddenyse.nightfall;
import android.app.NativeActivity;
import android.os.Bundle;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.net.wifi.WifiManager;
import android.view.View;
import android.view.WindowManager;
import android.util.Log;
import java.io.IOException;
/** The complete C++ frontend owns rendering, input and audio. */
public class MainActivity extends NativeActivity {
    private WifiManager.MulticastLock discoveryLock;
    @Override public void onCreate(Bundle state) {
        // Configure the window before NativeActivity starts the C++ thread.
        // A late fullscreen/layout transition can leave the first EGL surface
        // in portrait dimensions while the Activity is already landscape.
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        super.onCreate(state);
        try {
            String[] files = getAssets().list("art");
            Log.i("NightfallAssets", "Java art entries: " + java.util.Arrays.toString(files));
            try (java.io.InputStream index = getAssets().open("art/index.txt")) {
                Log.i("NightfallAssets", "Java index first byte: " + index.read());
            }
        } catch (IOException error) { Log.e("NightfallAssets", "APK assets inaccessible", error); }
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
