using UnityEngine;

namespace Nightfall.Net
{
    public static class WifiDirectBridge
    {
#if UNITY_ANDROID && !UNITY_EDITOR
        static void CallStatic(string method)
        {
            try
            {
                using var unity = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
                var act = unity.GetStatic<AndroidJavaObject>("currentActivity");
                if (act != null)
                {
                    using var plugin = new AndroidJavaClass("com.mikeanddenyse.nightfall.WifiDirectPlugin");
                    plugin.CallStatic(method, act);
                }
            }
            catch (System.Exception e)
            {
                Debug.LogWarning("WifiDirectBridge." + method + ": " + e.Message);
            }
        }
#endif

        public static void AcquireMulticastLock()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            CallStatic("acquireMulticastLock");
#endif
        }

        public static void ReleaseMulticastLock()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            try
            {
                using var plugin = new AndroidJavaClass("com.mikeanddenyse.nightfall.WifiDirectPlugin");
                plugin.CallStatic("releaseMulticastLock");
            }
            catch (System.Exception e)
            {
                Debug.LogWarning("WifiDirectBridge.ReleaseMulticastLock: " + e.Message);
            }
#endif
        }

        public static void CreateGroup()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            CallStatic("createGroup");
#endif
        }

        public static void Discover()
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            CallStatic("discoverPeers");
#endif
        }
    }
}
