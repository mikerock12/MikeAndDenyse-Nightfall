using UnityEngine;

namespace Nightfall
{
    public static class Bootstrap
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        static void Go()
        {
            if (Object.FindAnyObjectByType<NightApp>() != null) return;
            var go = new GameObject("NightApp");
            go.AddComponent<NightApp>();
        }
    }
}