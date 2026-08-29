package com.mikeanddenyse.nightfall;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Looper;
import android.util.Log;

public class WifiDirectPlugin {
    private static final String TAG = "NightfallWifi";
    private static WifiManager.MulticastLock sMulticastLock;
    private static WifiP2pManager sManager;
    private static WifiP2pManager.Channel sChannel;

    public static void acquireMulticastLock(Activity activity) {
        if (activity == null) return;
        try {
            if (sMulticastLock == null || !sMulticastLock.isHeld()) {
                WifiManager wifi = (WifiManager) activity.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                if (wifi != null) {
                    sMulticastLock = wifi.createMulticastLock("nightfall_multicast");
                    sMulticastLock.setReferenceCounted(true);
                    sMulticastLock.acquire();
                    Log.i(TAG, "Multicast lock acquired successfully.");
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to acquire multicast lock: " + e.getMessage());
        }
    }

    public static void releaseMulticastLock() {
        try {
            if (sMulticastLock != null && sMulticastLock.isHeld()) {
                sMulticastLock.release();
                Log.i(TAG, "Multicast lock released.");
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to release multicast lock: " + e.getMessage());
        }
    }

    public static void initP2p(Activity activity) {
        if (activity == null) return;
        acquireMulticastLock(activity);
        try {
            if (sManager == null) {
                sManager = (WifiP2pManager) activity.getSystemService(Context.WIFI_P2P_SERVICE);
                if (sManager != null) {
                    sChannel = sManager.initialize(activity, Looper.getMainLooper(), null);
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Failed to init P2P: " + e.getMessage());
        }
    }

    public static void createGroup(Activity activity) {
        if (activity == null) return;
        initP2p(activity);
        if (sManager == null || sChannel == null) return;
        activity.runOnUiThread(() -> {
            try {
                sManager.createGroup(sChannel, new WifiP2pManager.ActionListener() {
                    @Override public void onSuccess() { Log.i(TAG, "Wi-Fi Direct createGroup success"); }
                    @Override public void onFailure(int reason) { Log.w(TAG, "Wi-Fi Direct createGroup failure: " + reason); }
                });
            } catch (Exception e) {
                Log.w(TAG, "createGroup error: " + e.getMessage());
            }
        });
    }

    public static void discoverPeers(Activity activity) {
        if (activity == null) return;
        initP2p(activity);
        if (sManager == null || sChannel == null) return;
        activity.runOnUiThread(() -> {
            try {
                sManager.discoverPeers(sChannel, new WifiP2pManager.ActionListener() {
                    @Override public void onSuccess() { Log.i(TAG, "Wi-Fi Direct discoverPeers success"); }
                    @Override public void onFailure(int reason) { Log.w(TAG, "Wi-Fi Direct discoverPeers failure: " + reason); }
                });
            } catch (Exception e) {
                Log.w(TAG, "discoverPeers error: " + e.getMessage());
            }
        });
    }
}