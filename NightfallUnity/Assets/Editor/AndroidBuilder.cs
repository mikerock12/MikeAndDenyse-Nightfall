using System;
using System.IO;
using Nightfall.Net;
using Unity.Netcode;
using Unity.Netcode.Transports.UTP;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.SceneManagement;

namespace Nightfall.Editor
{
    public static class AndroidBuilder
    {
        public static void Build()
        {
            try
            {
                int checks = Smoke.RunInternal();
                Debug.Log("Pre-build smoke: " + checks + " checks");
                PrepareProject();
                ApplyPlayerSettings();
                PointAndroidTools();

                string outPath = @"D:\MikeAndDenyse\MikeAndDenyse-Nightfall-NGO.apk";
                if (File.Exists(outPath)) File.Delete(outPath);

                var opts = new BuildPlayerOptions
                {
                    scenes = new[] { "Assets/Scenes/Boot.unity" },
                    locationPathName = outPath,
                    target = BuildTarget.Android,
                    options = BuildOptions.None
                };
                var report = BuildPipeline.BuildPlayer(opts);
                if (report.summary.result != BuildResult.Succeeded)
                {
                    Debug.LogError("Build failed: " + report.summary.result);
                    EditorApplication.Exit(1);
                }
                else
                {
                    Debug.Log("APK: " + outPath);
                    EditorApplication.Exit(0);
                }
            }
            catch (Exception e)
            {
                Debug.LogError(e);
                EditorApplication.Exit(2);
            }
        }

        public static void PrepareProject()
        {
            Directory.CreateDirectory(Path.Combine(Application.dataPath, "Scenes"));
            Directory.CreateDirectory(Path.Combine(Application.dataPath, "Resources"));
            Directory.CreateDirectory(Path.Combine(Application.dataPath, "Resources/Art"));
            CopyArtIntoResources();
            PunchMagentaOnDisk();
            ApplyArtImporters();
            AssetDatabase.Refresh();
            var prefab = CreateGameNetPrefab();
            var list = CreatePrefabList(prefab);
            CreateBootScene(prefab, list);
            AssetDatabase.SaveAssets();
        }

        static void CopyArtIntoResources()
        {
            string src = Path.Combine(Application.dataPath, "StreamingAssets/art");
            string dst = Path.Combine(Application.dataPath, "Resources/Art");
            if (!Directory.Exists(src)) return;
            Directory.CreateDirectory(dst);
            foreach (var file in Directory.GetFiles(src, "*.png", SearchOption.AllDirectories))
            {
                string name = Path.GetFileName(file);
                File.Copy(file, Path.Combine(dst, name), true);
            }
        }

        static void PunchMagentaOnDisk()
        {
            string dst = Path.Combine(Application.dataPath, "Resources/Art");
            if (!Directory.Exists(dst)) return;
            foreach (var file in Directory.GetFiles(dst, "*.png"))
            {
                string n = Path.GetFileNameWithoutExtension(file).ToLowerInvariant();
                if (n.EndsWith("_bg")) continue;
                var data = File.ReadAllBytes(file);
                var tex = new Texture2D(2, 2, TextureFormat.RGBA32, false);
                if (!ImageConversion.LoadImage(tex, data)) continue;
                var pix = tex.GetPixels32();
                bool any = false;
                for (int i = 0; i < pix.Length; i++)
                {
                    var c = pix[i];
                    if (c.r > 180 && c.b > 180 && c.g < 160)
                    {
                        pix[i] = new Color32(0, 0, 0, 0);
                        any = true;
                    }
                }
                if (!any) continue;
                tex.SetPixels32(pix);
                tex.Apply();
                File.WriteAllBytes(file, tex.EncodeToPNG());
            }
        }

        static void ApplyArtImporters()
        {
            foreach (var guid in AssetDatabase.FindAssets("t:Texture2D", new[] { "Assets/Resources/Art" }))
            {
                string path = AssetDatabase.GUIDToAssetPath(guid);
                var imp = AssetImporter.GetAtPath(path) as TextureImporter;
                if (imp == null) continue;
                bool bg = Path.GetFileNameWithoutExtension(path).ToLowerInvariant().EndsWith("_bg");
                imp.textureType = TextureImporterType.Default;
                imp.alphaIsTransparency = !bg;
                imp.mipmapEnabled = false;
                imp.isReadable = false;
                imp.npotScale = TextureImporterNPOTScale.None;
                imp.filterMode = bg ? FilterMode.Bilinear : FilterMode.Point;
                imp.wrapMode = TextureWrapMode.Clamp;
                imp.maxTextureSize = bg ? 1024 : 512;
                imp.textureCompression = TextureImporterCompression.Compressed;
                imp.SaveAndReimport();
            }
        }

        static GameObject CreateGameNetPrefab()
        {
            string path = "Assets/Resources/GameNet.prefab";
            var existing = AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (existing != null && existing.GetComponent<NetworkObject>() != null && existing.GetComponent<GameNet>() != null)
                return existing;

            var go = new GameObject("GameNet");
            go.AddComponent<NetworkObject>();
            go.AddComponent<GameNet>();
            var prefab = PrefabUtility.SaveAsPrefabAsset(go, path);
            UnityEngine.Object.DestroyImmediate(go);
            return prefab;
        }

        static NetworkPrefabsList CreatePrefabList(GameObject prefab)
        {
            string path = "Assets/Resources/NightfallPrefabs.asset";
            var list = AssetDatabase.LoadAssetAtPath<NetworkPrefabsList>(path);
            if (list == null)
            {
                list = ScriptableObject.CreateInstance<NetworkPrefabsList>();
                AssetDatabase.CreateAsset(list, path);
            }
            bool has = false;
            foreach (var p in list.PrefabList)
                if (p != null && p.Prefab == prefab) has = true;
            if (!has) list.Add(new NetworkPrefab { Prefab = prefab });
            EditorUtility.SetDirty(list);
            return list;
        }

        static void CreateBootScene(GameObject prefab, NetworkPrefabsList list)
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            var camGo = new GameObject("Main Camera");
            camGo.tag = "MainCamera";
            var cam = camGo.AddComponent<Camera>();
            cam.orthographic = true;
            cam.orthographicSize = 270f;
            cam.backgroundColor = Color.black;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.nearClipPlane = -10f;
            cam.farClipPlane = 100f;
            camGo.transform.position = new Vector3(480f, 270f, -10f);
            camGo.AddComponent<AudioListener>();

            var appGo = new GameObject("NightApp");
            appGo.AddComponent<NightApp>();

            var netGo = new GameObject("NetworkManager");
            var nm = netGo.AddComponent<NetworkManager>();
            var utp = netGo.AddComponent<UnityTransport>();
            nm.NetworkConfig = new NetworkConfig
            {
                NetworkTransport = utp,
                TickRate = 30,
                EnableSceneManagement = false,
                PlayerPrefab = null
            };
            if (list != null)
                nm.NetworkConfig.Prefabs.NetworkPrefabsLists.Add(list);

            Directory.CreateDirectory(Path.Combine(Application.dataPath, "Scenes"));
            EditorSceneManager.SaveScene(scene, "Assets/Scenes/Boot.unity");
            var scenes = new EditorBuildSettingsScene[1];
            scenes[0] = new EditorBuildSettingsScene("Assets/Scenes/Boot.unity", true);
            EditorBuildSettings.scenes = scenes;
        }

        static void ApplyPlayerSettings()
        {
            PlayerSettings.companyName = "MikeAndDenyse";
            PlayerSettings.productName = "Mike & Denyse: Nightfall";
            PlayerSettings.applicationIdentifier = "com.mikeanddenyse.nightfall";
            PlayerSettings.bundleVersion = "3.1";
            PlayerSettings.Android.bundleVersionCode = 11;
            var iconPath = "Assets/Icons/AppIcon.png";
            if (File.Exists(Path.Combine(Application.dataPath, "Icons/AppIcon.png")))
            {
                var iconImp = AssetImporter.GetAtPath(iconPath) as TextureImporter;
                if (iconImp != null)
                {
                    iconImp.textureType = TextureImporterType.Default;
                    iconImp.alphaIsTransparency = true;
                    iconImp.mipmapEnabled = false;
                    iconImp.maxTextureSize = 1024;
                    iconImp.npotScale = TextureImporterNPOTScale.None;
                    iconImp.SaveAndReimport();
                }
                var icon = AssetDatabase.LoadAssetAtPath<Texture2D>(iconPath);
                if (icon != null)
                {
                    PlayerSettings.SetIcons(NamedBuildTarget.Android, new[] { icon }, IconKind.Application);
                    PlayerSettings.SetIcons(NamedBuildTarget.Unknown, new[] { icon }, IconKind.Application);
                }
            }
            PlayerSettings.Android.minSdkVersion = AndroidSdkVersions.AndroidApiLevel26;
            PlayerSettings.Android.targetSdkVersion = AndroidSdkVersions.AndroidApiLevel35;
            PlayerSettings.Android.applicationEntry = AndroidApplicationEntry.Activity;
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Android, ScriptingImplementation.IL2CPP);
            PlayerSettings.Android.targetArchitectures = AndroidArchitecture.ARM64 | AndroidArchitecture.ARMv7;
            PlayerSettings.SetIl2CppCompilerConfiguration(NamedBuildTarget.Android, Il2CppCompilerConfiguration.Release);
            PlayerSettings.SetManagedStrippingLevel(NamedBuildTarget.Android, ManagedStrippingLevel.Minimal);
            PlayerSettings.stripEngineCode = false;
            PlayerSettings.defaultInterfaceOrientation = UIOrientation.LandscapeLeft;
            PlayerSettings.allowedAutorotateToLandscapeLeft = true;
            PlayerSettings.allowedAutorotateToLandscapeRight = true;
            PlayerSettings.allowedAutorotateToPortrait = false;
            PlayerSettings.Android.forceInternetPermission = true;
            PlayerSettings.Android.startInFullscreen = true;
            PlayerSettings.Android.renderOutsideSafeArea = true;
            PlayerSettings.Android.optimizedFramePacing = false;
            PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.Android, false);
            PlayerSettings.SetGraphicsAPIs(BuildTarget.Android, new[]
            {
                GraphicsDeviceType.OpenGLES3,
                GraphicsDeviceType.Vulkan
            });
            PlayerSettings.Android.androidTVCompatibility = false;
            EditorUserBuildSettings.androidBuildSystem = AndroidBuildSystem.Gradle;
            EditorUserBuildSettings.buildAppBundle = false;
            EditorUserBuildSettings.exportAsGoogleAndroidProject = false;
            EditorUserBuildSettings.SwitchActiveBuildTarget(NamedBuildTarget.Android, BuildTarget.Android);
        }

        static void PointAndroidTools()
        {
            string[] sdkCands =
            {
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Android", "Sdk")
            };
            string[] ndkCands = { @"D:\Unity\Downloads\android-ndk-r27c" };
            string[] jdkCands = { @"D:\Unity\Downloads\jdk-extract" };
            string sdk = FirstDir(sdkCands);
            string ndk = FirstDir(ndkCands);
            string jdk = FirstDir(jdkCands);
            if (sdk != null)
            {
                EditorPrefs.SetString("AndroidSdkRoot", sdk);
                TrySetExt("sdkRootPath", sdk);
            }
            if (ndk != null)
            {
                EditorPrefs.SetString("AndroidNdkRoot", ndk);
                TrySetExt("ndkRootPath", ndk);
            }
            if (jdk != null)
            {
                EditorPrefs.SetString("JdkPath", jdk);
                TrySetExt("jdkRootPath", jdk);
            }
        }

        static string FirstDir(string[] cands)
        {
            foreach (var p in cands) if (Directory.Exists(p)) return p;
            return null;
        }

        static void TrySetExt(string prop, string value)
        {
            try
            {
                var t = Type.GetType("UnityEditor.Android.AndroidExternalToolsSettings, UnityEditor.Android.Extensions");
                var p = t?.GetProperty(prop);
                if (p != null && p.CanWrite) p.SetValue(null, value);
            }
            catch (Exception e)
            {
                Debug.LogWarning("Android tool " + prop + ": " + e.Message);
            }
        }
    }
}