using System.Collections.Generic;
using UnityEngine;

namespace Nightfall
{
    public class SpriteBank
    {
        readonly Dictionary<string, Texture2D> _tex = new();
        readonly Dictionary<string, Sprite> _spr = new();

        public int Count => _tex.Count;

        public void LoadAll()
        {
            var all = Resources.LoadAll<Texture2D>("Art");
            foreach (var src in all)
            {
                if (src == null) continue;
                string key = src.name.ToLowerInvariant();
                src.filterMode = key.EndsWith("_bg") ? FilterMode.Bilinear : FilterMode.Point;
                src.wrapMode = TextureWrapMode.Clamp;
                PunchMagenta(src, key);
                _tex[key] = src;
                try
                {
                    if (src.width > 0 && src.height > 0)
                        _spr[key] = Sprite.Create(src, new Rect(0, 0, src.width, src.height), new Vector2(0.5f, 0f), 64f);
                }
                catch (System.Exception e)
                {
                    Debug.LogWarning("SpriteBank " + key + ": " + e.Message);
                }
            }
        }

        public Texture2D Tex(params string[] keys)
        {
            foreach (var k in keys)
            {
                if (string.IsNullOrEmpty(k)) continue;
                string n = k.ToLowerInvariant();
                if (_tex.TryGetValue(n, out var t)) return t;
                int slash = n.LastIndexOf('/');
                if (slash >= 0 && _tex.TryGetValue(n.Substring(slash + 1), out t)) return t;
            }
            return null;
        }

        public Sprite Spr(params string[] keys)
        {
            foreach (var k in keys)
            {
                if (string.IsNullOrEmpty(k)) continue;
                string n = k.ToLowerInvariant();
                if (_spr.TryGetValue(n, out var s)) return s;
                int slash = n.LastIndexOf('/');
                if (slash >= 0 && _spr.TryGetValue(n.Substring(slash + 1), out s)) return s;
            }
            return null;
        }

        static void PunchMagenta(Texture2D src, string key)
        {
            if (src == null || key.EndsWith("_bg")) return;
            try
            {
                var pix = src.GetPixels32();
                bool any = false;
                for (int i = 0; i < pix.Length; i++)
                {
                    var c = pix[i];
                    if (c.a == 0) continue;
                    if (c.r > 180 && c.b > 180 && c.g < 160)
                    {
                        pix[i] = new Color32(0, 0, 0, 0);
                        any = true;
                    }
                }
                if (!any) return;
                src.SetPixels32(pix);
                src.Apply(false, false);
            }
            catch { }
        }
    }
}