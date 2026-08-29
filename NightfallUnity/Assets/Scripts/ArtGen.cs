using UnityEngine;

namespace Nightfall
{
    /// <summary>
    /// Every pixel the engine draws that is not one of the painted PNGs is generated here:
    /// tile atlas with real silhouettes (spikes, brambles, lava crust, ladders), FX sprites,
    /// HUD icons, parallax ridges and the gold menu frames.
    /// </summary>
    public static class ArtGen
    {
        public static Texture2D Px, Glow, Spark, Star, Heart, HeartEmpty, Soul, Slash, Bell, Moon, Smoke, Shard, Drop, Flag, Disc, Tri, Ring;
        public static Sprite PanelSpr, PanelSoftSpr, ButtonSpr, ButtonGhostSpr, BarSpr, GlowSpr;
        static bool _ready;

        public static void Ensure()
        {
            if (_ready) return;
            _ready = true;

            Px = Fill(2, 2, Color.white);
            Glow = MakeGlow(64);
            Spark = MakeSpark(32);
            Star = MakeStar(24);
            Heart = MakeHeart(24, true);
            HeartEmpty = MakeHeart(24, false);
            Soul = MakeSoul(28);
            Slash = MakeSlash(72, 72);
            Bell = MakeBell(40);
            Moon = MakeMoon(96);
            Smoke = MakeSmoke(32);
            Shard = MakeShard(16);
            Drop = MakeDrop(12);
            Flag = MakeFlag(28, 44);
            Disc = MakeDisc(64);
            Tri = MakeTri(48);
            Ring = MakeRing(64, 0.16f);

            PanelSpr = Slice(MakePanel(64, true), 20);
            PanelSoftSpr = Slice(MakePanel(64, false), 20);
            ButtonSpr = Slice(MakeButton(48, false), 14);
            ButtonGhostSpr = Slice(MakeButton(48, true), 14);
            BarSpr = Slice(MakeBar(24), 6);
            GlowSpr = Sprite.Create(Glow, new Rect(0, 0, Glow.width, Glow.height), new Vector2(0.5f, 0.5f), 64f);
        }

        // ───────────────────────────── helpers ─────────────────────────────

        static Texture2D New(int w, int h)
        {
            var t = new Texture2D(w, h, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Bilinear,
                wrapMode = TextureWrapMode.Clamp
            };
            return t;
        }

        static Texture2D Fill(int w, int h, Color c)
        {
            var t = New(w, h);
            var p = new Color[w * h];
            for (int i = 0; i < p.Length; i++) p[i] = c;
            t.SetPixels(p); t.Apply();
            return t;
        }

        static Sprite Slice(Texture2D t, int b)
        {
            t.filterMode = FilterMode.Bilinear;
            return Sprite.Create(t, new Rect(0, 0, t.width, t.height), new Vector2(0.5f, 0.5f), 100f, 0,
                SpriteMeshType.FullRect, new Vector4(b, b, b, b));
        }

        public static float Hash(float x, float y, float s)
        {
            float v = Mathf.Sin(x * 127.1f + y * 311.7f + s * 74.7f) * 43758.5453f;
            return v - Mathf.Floor(v);
        }

        public static float Noise(float x, float y, float s)
        {
            int xi = Mathf.FloorToInt(x), yi = Mathf.FloorToInt(y);
            float xf = x - xi, yf = y - yi;
            float u = xf * xf * (3 - 2 * xf), v = yf * yf * (3 - 2 * yf);
            float a = Hash(xi, yi, s), b = Hash(xi + 1, yi, s), c = Hash(xi, yi + 1, s), d = Hash(xi + 1, yi + 1, s);
            return Mathf.Lerp(Mathf.Lerp(a, b, u), Mathf.Lerp(c, d, u), v);
        }

        static Color Mul(Color c, float f) => new(c.r * f, c.g * f, c.b * f, c.a);
        static Color Mix(Color a, Color b, float t) => Color.Lerp(a, b, Mathf.Clamp01(t));

        // ───────────────────────────── FX sprites ─────────────────────────────

        static Texture2D MakeGlow(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float d = Mathf.Sqrt((x - r + 0.5f) * (x - r + 0.5f) + (y - r + 0.5f) * (y - r + 0.5f)) / r;
                    float a = Mathf.Clamp01(1 - d);
                    p[y * n + x] = new Color(1, 1, 1, a * a * a);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeDisc(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float d = Mathf.Sqrt((x - r + 0.5f) * (x - r + 0.5f) + (y - r + 0.5f) * (y - r + 0.5f)) / r;
                    p[y * n + x] = new Color(1, 1, 1, Mathf.Clamp01((1 - d) * r * 0.6f));
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeRing(int n, float thick)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float d = Mathf.Sqrt((x - r + 0.5f) * (x - r + 0.5f) + (y - r + 0.5f) * (y - r + 0.5f)) / r;
                    float a = Mathf.Clamp01((1 - Mathf.Abs(d - (1 - thick)) / thick)) * Mathf.Clamp01((1 - d) * 8f);
                    p[y * n + x] = new Color(1, 1, 1, a);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        /// <summary>Right-pointing triangle; rotate it for the other directions.</summary>
        static Texture2D MakeTri(int n)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float fx = (x + 0.5f) / n, fy = Mathf.Abs((y + 0.5f) / n * 2 - 1);
                    float a = fx <= 0.92f && fy <= (1 - fx) * 1.05f ? 1f : 0f;
                    p[y * n + x] = new Color(1, 1, 1, a);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeSpark(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float dx = (x - r + 0.5f) / r, dy = (y - r + 0.5f) / r;
                    float d = Mathf.Sqrt(dx * dx + dy * dy);
                    float ang = Mathf.Atan2(dy, dx);
                    float rays = Mathf.Abs(Mathf.Cos(ang * 3f));
                    float a = Mathf.Clamp01(1 - d) * (0.35f + rays * 0.85f);
                    p[y * n + x] = new Color(1, 1, 1, a * a);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeStar(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float dx = Mathf.Abs(x - r + 0.5f) / r, dy = Mathf.Abs(y - r + 0.5f) / r;
                    float cross = Mathf.Max(0, 1 - (dx * 3.2f + dy * 0.6f)) + Mathf.Max(0, 1 - (dy * 3.2f + dx * 0.6f));
                    float a = Mathf.Clamp01(cross);
                    p[y * n + x] = new Color(1, 1, 1, a * a);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeHeart(int n, bool full)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float fx = (x + 0.5f) / n * 2 - 1;
                    float fy = 1 - (y + 0.5f) / n * 2;
                    fy += 0.18f;
                    float v = fx * fx + fy * fy - 0.42f;
                    bool inside = v * v * v - fx * fx * fy * fy * fy < 0;
                    if (!inside) { p[(n - 1 - y) * n + x] = new Color(0, 0, 0, 0); continue; }
                    float edge = Mathf.Clamp01(1.6f + fy);
                    Color c = full
                        ? Mix(new Color(0.98f, 0.34f, 0.38f), new Color(0.55f, 0.06f, 0.13f), 1 - edge * 0.8f)
                        : new Color(0.20f, 0.08f, 0.11f);
                    if (full && fx < -0.15f && fy > 0.25f) c = Mix(c, Color.white, 0.55f);
                    p[(n - 1 - y) * n + x] = c;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeSoul(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float dx = (x - r + 0.5f) / r;
                    float fy = (y + 0.5f) / n;                  // 0 bottom · 1 top
                    float width = 0.42f + 0.5f * Mathf.Sin(fy * 3.14159f) - Mathf.Pow(fy, 3f) * 0.35f;
                    float d = Mathf.Abs(dx) / Mathf.Max(0.05f, width);
                    float a = Mathf.Clamp01(1.15f - d);
                    Color c = Mix(new Color(0.55f, 0.28f, 0.95f), new Color(0.85f, 0.75f, 1f), Mathf.Clamp01(fy * 1.1f));
                    p[y * n + x] = new Color(c.r, c.g, c.b, a * a * (0.55f + fy * 0.5f));
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeSlash(int w, int h)
        {
            var t = New(w, h); var p = new Color[w * h];
            float cx = 0, cy = h * 0.5f;
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    float dx = x - cx, dy = y - cy;
                    float d = Mathf.Sqrt(dx * dx + dy * dy) / (w * 0.92f);
                    float ang = Mathf.Atan2(dy, Mathf.Max(0.001f, dx)) / 1.2f;      // −1..1 across the arc
                    float band = Mathf.Exp(-Mathf.Pow((d - 0.78f) * 9f, 2f));
                    float sweep = Mathf.Clamp01(1 - Mathf.Abs(ang));
                    float a = band * Mathf.Pow(sweep, 0.7f);
                    Color c = Mix(new Color(1f, 0.86f, 0.62f), Color.white, a);
                    p[y * w + x] = new Color(c.r, c.g, c.b, Mathf.Clamp01(a * 1.35f));
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeBell(int n)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float fx = (x + 0.5f) / n * 2 - 1;
                    float fy = (y + 0.5f) / n;                       // 0 bottom
                    float body = 0.22f + (1 - fy) * 0.62f;
                    bool inBell = fy > 0.12f && fy < 0.86f && Mathf.Abs(fx) < body;
                    bool inSkirt = fy >= 0.06f && fy <= 0.16f && Mathf.Abs(fx) < 0.86f;
                    bool inKnob = fy >= 0.84f && fy < 0.97f && Mathf.Abs(fx) < 0.14f;
                    bool clapper = fy > 0.0f && fy < 0.08f && Mathf.Abs(fx) < 0.12f;
                    if (!(inBell || inSkirt || inKnob || clapper)) { p[y * n + x] = default; continue; }
                    float shade = Mathf.Clamp01(0.55f + (0.5f - fx) * 0.5f);
                    Color c = Mix(new Color(0.55f, 0.38f, 0.12f), new Color(1f, 0.88f, 0.52f), shade);
                    if (fx < -0.35f) c = Mul(c, 0.7f);
                    p[y * n + x] = c;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeMoon(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float dx = (x - r + 0.5f) / r, dy = (y - r + 0.5f) / r;
                    float d = Mathf.Sqrt(dx * dx + dy * dy);
                    if (d > 1) { p[y * n + x] = default; continue; }
                    float crat = Noise(x * 0.22f, y * 0.22f, 3.1f);
                    Color c = Mix(new Color(0.94f, 0.72f, 0.62f), new Color(0.72f, 0.42f, 0.38f), crat * 0.55f);
                    float edge = Mathf.Clamp01((1 - d) * 6f);
                    p[y * n + x] = new Color(c.r, c.g, c.b, edge);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeSmoke(int n)
        {
            var t = New(n, n); var p = new Color[n * n]; float r = n * 0.5f;
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float dx = (x - r + 0.5f) / r, dy = (y - r + 0.5f) / r;
                    float d = Mathf.Sqrt(dx * dx + dy * dy);
                    float lump = Noise(x * 0.35f, y * 0.35f, 7.7f);
                    float a = Mathf.Clamp01(1 - d - 0.35f + lump * 0.55f);
                    p[y * n + x] = new Color(1, 1, 1, a * a * 0.9f);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeShard(int n)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    float fx = (x + 0.5f) / n * 2 - 1, fy = (y + 0.5f) / n * 2 - 1;
                    bool inside = Mathf.Abs(fx) + Mathf.Abs(fy) < 0.95f;
                    p[y * n + x] = inside ? new Color(1, 1, 1, 1) : default;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeDrop(int n)
        {
            var t = New(n, n * 2); var p = new Color[n * n * 2];
            for (int y = 0; y < n * 2; y++)
                for (int x = 0; x < n; x++)
                {
                    float fx = (x + 0.5f) / n * 2 - 1;
                    float fy = (y + 0.5f) / (n * 2f);
                    float w = Mathf.Sin(fy * 2.2f) * 0.9f;
                    p[y * n + x] = Mathf.Abs(fx) < w ? new Color(1, 1, 1, 0.85f) : default;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeFlag(int w, int h)
        {
            var t = New(w, h); var p = new Color[w * h];
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    int iy = h - 1 - y;
                    bool pole = x >= 4 && x < 7;
                    bool cloth = iy >= 3 && iy < 18 && x >= 7 && x < 7 + (15 - Mathf.Abs(iy - 10));
                    if (pole) p[y * w + x] = new Color(0.35f, 0.26f, 0.18f);
                    else if (cloth) p[y * w + x] = Mix(new Color(0.78f, 0.16f, 0.22f), new Color(0.42f, 0.06f, 0.12f), (iy - 3) / 15f);
                    else p[y * w + x] = default;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        // ───────────────────────────── UI frames ─────────────────────────────

        static Texture2D MakePanel(int n, bool strong)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    int e = Mathf.Min(Mathf.Min(x, y), Mathf.Min(n - 1 - x, n - 1 - y));
                    Color c;
                    if (e == 0) c = new Color(0.02f, 0.01f, 0.02f, 0.98f);
                    else if (e <= 2) c = new Color(0.83f, 0.71f, 0.42f, strong ? 0.95f : 0.55f);
                    else if (e <= 4) c = new Color(0.16f, 0.09f, 0.10f, 0.98f);
                    else
                    {
                        float g = Mathf.Clamp01(e / (float)(n * 0.5f));
                        c = Mix(new Color(0.09f, 0.05f, 0.07f, 0.97f), new Color(0.04f, 0.02f, 0.04f, 0.97f), g);
                        float grain = Noise(x * 0.7f, y * 0.7f, 2.3f);
                        c = Mul(c, 0.92f + grain * 0.16f);
                        c.a = strong ? 0.97f : 0.90f;
                    }
                    p[y * n + x] = c;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeButton(int n, bool ghost)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    int e = Mathf.Min(Mathf.Min(x, y), Mathf.Min(n - 1 - x, n - 1 - y));
                    float v = 1 - y / (float)n;
                    Color c;
                    if (e == 0) c = new Color(0, 0, 0, 0.85f);
                    else if (e <= 1) c = new Color(0.83f, 0.71f, 0.42f, ghost ? 0.42f : 0.85f);
                    else
                        c = ghost
                            ? Mix(new Color(0.10f, 0.06f, 0.08f, 0.80f), new Color(0.05f, 0.03f, 0.05f, 0.80f), v)
                            : Mix(new Color(0.46f, 0.13f, 0.18f, 0.97f), new Color(0.22f, 0.06f, 0.10f, 0.97f), v);
                    p[y * n + x] = c;
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        static Texture2D MakeBar(int n)
        {
            var t = New(n, n); var p = new Color[n * n];
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    int e = Mathf.Min(Mathf.Min(x, y), Mathf.Min(n - 1 - x, n - 1 - y));
                    p[y * n + x] = e == 0 ? new Color(0, 0, 0, 0.9f) : new Color(1, 1, 1, 1);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        // ───────────────────────────── world layers ─────────────────────────────

        /// <summary>
        /// Vertical sky gradient. The horizon (bottom) is the lit end and the zenith the dark one —
        /// the old ramp ran the other way, which flattened every backdrop into one dark smear.
        /// </summary>
        public static Texture2D Sky(Color tone, Color fog)
        {
            var horizon = Lift(fog, 2.15f);
            var zenith = Mul(tone, 0.45f);
            var t = New(4, 160); var p = new Color[4 * 160];
            for (int y = 0; y < 160; y++)
            {
                float v = y / 159f;                              // 0 = bottom of the texture = horizon
                Color c = Mix(horizon, zenith, Mathf.Pow(v, 0.62f));
                if (v > 0.55f)                                   // a few stars up high
                {
                    float n = Hash(Mathf.Floor(v * 900f), 3, 11f);
                    if (n > 0.985f) c = Mix(c, new Color(0.9f, 0.92f, 1f), 0.45f);
                }
                for (int x = 0; x < 4; x++) p[y * 4 + x] = c;
            }
            t.SetPixels(p); t.Apply();
            t.wrapMode = TextureWrapMode.Clamp;
            return t;
        }

        /// <summary>Brightens a colour without blowing out its hue — dark world palettes need the lift.</summary>
        public static Color Lift(Color c, float f)
        {
            float m = Mathf.Max(0.0001f, Mathf.Max(c.r, Mathf.Max(c.g, c.b)));
            float target = Mathf.Clamp01(m * f);
            float k = target / m;
            return new Color(Mathf.Clamp01(c.r * k + 0.03f), Mathf.Clamp01(c.g * k + 0.03f), Mathf.Clamp01(c.b * k + 0.045f), 1f);
        }

        /// <summary>Silhouette band used for parallax: hills, ruins, spires, depending on the seed.</summary>
        public static Texture2D Ridge(int w, int h, float seed, int style)
        {
            var t = New(w, h); var p = new Color[w * h];
            for (int x = 0; x < w; x++)
            {
                float fx = x / (float)w;
                float prof;
                if (style == 0)      // rolling hills
                    prof = 0.42f + Mathf.Sin(fx * 9.2f + seed) * 0.13f + Mathf.Sin(fx * 23f + seed * 3) * 0.06f + Noise(x * 0.05f, seed, seed) * 0.10f;
                else if (style == 1) // jagged peaks
                    prof = 0.30f + Mathf.Abs(Mathf.Sin(fx * 12f + seed)) * 0.42f + Noise(x * 0.09f, seed, seed) * 0.08f;
                else                 // city / ruins skyline
                {
                    int block = Mathf.FloorToInt(fx * 22f);
                    prof = 0.30f + Hash(block, 3, seed) * 0.45f;
                }
                int top = Mathf.Clamp(Mathf.RoundToInt((1 - prof) * h), 0, h - 1);
                for (int y = 0; y < h; y++)
                {
                    int iy = h - 1 - y;
                    p[y * w + x] = iy >= top ? new Color(1, 1, 1, 1) : default;
                }
                if (style == 2)
                {
                    // lit windows in the ruins
                    for (int y = top + 4; y < h - 2; y += 7)
                        if (Hash(x, y, seed) > 0.94f)
                            p[(h - 1 - y) * w + x] = new Color(1f, 0.75f, 0.35f, 1f);
                }
            }
            t.SetPixels(p); t.Apply(); return t;
        }

        /// <summary>Screen vignette: transparent middle, dark corners.</summary>
        public static Texture2D Vignette()
        {
            int w = 96, h = 64;
            var t = New(w, h); var p = new Color[w * h];
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                {
                    float dx = (x + 0.5f) / w * 2 - 1, dy = (y + 0.5f) / h * 2 - 1;
                    float d = Mathf.Sqrt(dx * dx * 0.85f + dy * dy);
                    float a = Mathf.Clamp01((d - 0.55f) / 0.75f);
                    p[y * w + x] = new Color(0, 0, 0, a * a * 0.85f);
                }
            t.SetPixels(p); t.Apply(); return t;
        }

        // ───────────────────────────── tile atlas ─────────────────────────────

        public const int AtlasCells = 11;
        const int TS = T.Tile;

        /// <summary>
        /// Two rows: row 1 (upper half of the texture) is the exposed variant with a lit cap,
        /// row 0 is the buried variant. UVs come from <see cref="TileUv"/>.
        /// </summary>
        public static Texture2D TileAtlas(WorldDef w)
        {
            int stride = TS * AtlasCells, hgt = TS * 2;
            var t = new Texture2D(stride, hgt, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp
            };
            var pix = new Color[stride * hgt];
            Color ground = Hex(w.Ground, new Color(0.2f, 0.3f, 0.16f));
            Color lip = Hex(w.Lip, new Color(0.45f, 0.6f, 0.25f));
            float seed = Mathf.Abs(w.Id != null ? w.Id.GetHashCode() % 997 : 7);

            for (int cell = 1; cell < AtlasCells; cell++)
                for (int variant = 0; variant < 2; variant++)
                    PaintTile(pix, stride, cell, variant == 1, ground, lip, seed);

            t.SetPixels(pix); t.Apply();
            return t;
        }

        public static Rect TileUv(int cell, bool exposed) =>
            new(cell / (float)AtlasCells, exposed ? 0.5f : 0f, 1f / AtlasCells, 0.5f);

        static Color Hex(string h, Color fallback)
        {
            if (string.IsNullOrEmpty(h)) return fallback;
            if (h[0] != '#') h = "#" + h;
            return ColorUtility.TryParseHtmlString(h, out var c) ? c : fallback;
        }

        static void Put(Color[] pix, int stride, int cell, bool exposed, int x, int y, Color c)
        {
            if (x < 0 || y < 0 || x >= TS || y >= TS) return;
            int baseY = exposed ? TS : 0;
            int ty = baseY + (TS - 1 - y);
            pix[ty * stride + cell * TS + x] = c;
        }

        static void PaintTile(Color[] pix, int stride, int cell, bool exposed, Color ground, Color lip, float seed)
        {
            switch (cell)
            {
                case T.Solid: PaintGround(pix, stride, cell, exposed, ground, lip, seed); break;
                case T.Ice: PaintGround(pix, stride, cell, exposed, new Color(0.42f, 0.62f, 0.74f), new Color(0.88f, 0.96f, 1f), seed, true); break;
                case T.Break: PaintBrick(pix, stride, cell, exposed, seed); break;
                case T.Platform: PaintPlatform(pix, stride, cell, exposed, lip, seed); break;
                case T.Spike: PaintSpike(pix, stride, cell, exposed); break;
                case T.Thorn: PaintThorn(pix, stride, cell, exposed, seed); break;
                case T.Lava: PaintLiquid(pix, stride, cell, exposed, new Color(0.62f, 0.09f, 0.03f), new Color(1f, 0.62f, 0.16f), seed, true); break;
                case T.Water: PaintLiquid(pix, stride, cell, exposed, new Color(0.06f, 0.18f, 0.28f), new Color(0.28f, 0.62f, 0.78f), seed, false); break;
                case T.Bounce: PaintBounce(pix, stride, cell, exposed); break;
                case T.Ladder: PaintLadder(pix, stride, cell, exposed); break;
            }
        }

        static void PaintGround(Color[] pix, int stride, int cell, bool exposed, Color body, Color cap, float seed, bool crystal = false)
        {
            for (int x = 0; x < TS; x++)
            {
                int capH = exposed ? 7 + Mathf.RoundToInt(Noise(x * 0.28f, 0, seed) * 4f) : 0;
                for (int y = 0; y < TS; y++)
                {
                    float n = Noise(x * 0.35f, y * 0.35f, seed);
                    float broad = Noise(x * 0.06f, y * 0.06f, seed + 13);   // low frequency: breaks up the grid
                    float n2 = Noise(x * 0.11f, y * 0.11f, seed + 5);
                    Color c;
                    if (y < capH)
                    {
                        c = Mul(cap, 0.80f + n * 0.36f);
                        if (y >= capH - 2) c = Mul(c, 0.62f);              // shaded underside of the cap
                        if (y < 2) c = Mix(c, Color.white, 0.14f);
                    }
                    else
                    {
                        float depth = Mathf.Clamp01((y - capH) / (float)TS);
                        c = Mul(body, 0.68f + n * 0.22f + broad * 0.26f - depth * 0.20f);
                        if (n2 > 0.80f) c = Mix(c, cap, 0.16f);            // embedded pebbles
                        if (crystal && ((x + y) % 13 == 0)) c = Mix(c, Color.white, 0.26f);
                    }
                    // barely-there bevel: the old hard edges printed a visible grid over the whole level
                    if (x == 0) c = Mul(c, 0.95f);
                    if (x == TS - 1) c = Mul(c, 0.97f);
                    c.a = 1;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
            }
        }

        static void PaintBrick(Color[] pix, int stride, int cell, bool exposed, float seed)
        {
            Color body = new(0.30f, 0.21f, 0.16f), mortar = new(0.16f, 0.11f, 0.09f), crack = new(0.55f, 0.42f, 0.26f);
            for (int x = 0; x < TS; x++)
                for (int y = 0; y < TS; y++)
                {
                    int row = y / 10;
                    int off = (row % 2) * 10;
                    bool line = y % 10 == 0 || (x + off) % 20 == 0;
                    float n = Noise(x * 0.4f, y * 0.4f, seed);
                    Color c = line ? mortar : Mul(body, 0.85f + n * 0.35f);
                    if (Mathf.Abs(x - y) < 2 || Mathf.Abs(TS - x - y) < 2) c = Mix(c, crack, 0.35f);
                    c.a = 1;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
        }

        static void PaintPlatform(Color[] pix, int stride, int cell, bool exposed, Color cap, float seed)
        {
            int h = 15;
            for (int x = 0; x < TS; x++)
                for (int y = 0; y < h; y++)
                {
                    float n = Noise(x * 0.5f, y * 1.4f, seed);
                    Color c = Mix(new Color(0.55f, 0.40f, 0.22f), Mul(cap, 0.9f), 0.35f);
                    c = Mul(c, 0.78f + n * 0.30f);
                    if (y < 2) c = Mix(c, Color.white, 0.30f);                  // top light
                    if (y >= h - 3) c = Mul(c, 0.55f);                          // underside shadow
                    if (y % 6 == 3) c = Mul(c, 0.86f);                          // plank grain
                    c.a = 1;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
            for (int x = 0; x < TS; x++)
                Put(pix, stride, cell, exposed, x, h, new Color(0, 0, 0, 0.35f));
        }

        static void PaintSpike(Color[] pix, int stride, int cell, bool exposed)
        {
            Color rock = new(0.19f, 0.14f, 0.17f);
            Color steel = new(0.78f, 0.80f, 0.86f);
            for (int x = 0; x < TS; x++)
                for (int y = 30; y < TS; y++)
                {
                    float n = Noise(x * 0.4f, y * 0.4f, 11);
                    Put(pix, stride, cell, exposed, x, y, Mul(rock, 0.8f + n * 0.4f));
                }
            // four cones, tips near the top of the cell
            for (int k = 0; k < 4; k++)
            {
                int cx = 5 + k * 10;
                for (int y = 4; y < 32; y++)
                {
                    float t = (y - 4) / 28f;
                    int half = Mathf.RoundToInt(t * 4.6f);
                    for (int x = cx - half; x <= cx + half; x++)
                    {
                        float side = (x - (cx - half)) / Mathf.Max(1f, half * 2f);
                        Color c = Mix(Mul(steel, 1.0f), Mul(steel, 0.42f), side);
                        if (side < 0.28f) c = Mix(c, Color.white, 0.35f);
                        c.a = 1;
                        Put(pix, stride, cell, exposed, x, y, c);
                    }
                }
            }
        }

        static void PaintThorn(Color[] pix, int stride, int cell, bool exposed, float seed)
        {
            Color vine = new(0.15f, 0.18f, 0.08f);
            Color thorn = new(0.62f, 0.58f, 0.36f);
            for (int x = 0; x < TS; x++)
                for (int y = 26; y < TS; y++)
                {
                    float n = Noise(x * 0.3f, y * 0.3f, seed + 2);
                    Put(pix, stride, cell, exposed, x, y, Mul(new Color(0.12f, 0.13f, 0.07f), 0.8f + n * 0.5f));
                }
            for (int s = 0; s < 3; s++)
            {
                float phase = s * 2.1f + seed;
                for (int y = 4; y < TS; y++)
                {
                    int x = Mathf.RoundToInt(TS * 0.5f + Mathf.Sin(y * 0.30f + phase) * (11 + s * 3) + (s - 1) * 3);
                    for (int t = -1; t <= 1; t++) Put(pix, stride, cell, exposed, x + t, y, Mul(vine, t == -1 ? 1.5f : 1f));
                    if (y % 7 == 3)
                    {
                        int dir = (y / 7 + s) % 2 == 0 ? 1 : -1;
                        for (int k = 1; k <= 4; k++)
                            Put(pix, stride, cell, exposed, x + dir * k, y - k, Mul(thorn, 1f - k * 0.12f));
                    }
                }
            }
        }

        static void PaintLiquid(Color[] pix, int stride, int cell, bool exposed, Color deep, Color bright, float seed, bool lava)
        {
            for (int x = 0; x < TS; x++)
                for (int y = 0; y < TS; y++)
                {
                    float n = Noise(x * 0.18f, y * 0.18f, seed);
                    float depth = y / (float)TS;
                    Color c = Mix(bright, deep, Mathf.Clamp01(depth * 1.15f + n * 0.25f));
                    float a = lava ? 1f : 0.72f;
                    if (exposed && y < 5)
                    {
                        c = Mix(c, Color.white, lava ? 0.30f : 0.42f);
                        a = lava ? 1f : 0.88f;
                    }
                    if (lava)
                    {
                        float vein = Noise(x * 0.5f, y * 0.16f, seed + 9);
                        if (vein > 0.72f) c = Mix(c, new Color(1f, 0.94f, 0.62f), (vein - 0.72f) * 3f);
                        if (exposed && y < 3 && Noise(x * 0.7f, 0, seed + 4) > 0.55f) c = Mul(new Color(0.20f, 0.09f, 0.07f), 1f); // crust
                    }
                    c.a = a;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
        }

        static void PaintBounce(Color[] pix, int stride, int cell, bool exposed)
        {
            Color capA = new(0.45f, 0.82f, 0.36f), capB = new(0.14f, 0.35f, 0.16f), stem = new(0.86f, 0.84f, 0.72f);
            for (int x = 0; x < TS; x++)
                for (int y = 0; y < TS; y++)
                {
                    float fx = (x + 0.5f) / TS * 2 - 1;
                    bool dome = y < 17 && Mathf.Abs(fx) <= Mathf.Sqrt(Mathf.Max(0, 1 - Mathf.Pow(y / 17f, 2f))) * 1.0f;
                    bool trunk = y >= 15 && y < TS && Mathf.Abs(fx) < 0.34f;
                    if (dome)
                    {
                        Color c = Mix(capA, capB, y / 17f);
                        if (y < 4) c = Mix(c, Color.white, 0.35f);
                        if (Noise(x * 0.6f, y * 0.6f, 6) > 0.80f) c = Mix(c, Color.white, 0.5f);
                        c.a = 1;
                        Put(pix, stride, cell, exposed, x, y, c);
                    }
                    else if (trunk)
                    {
                        Color c = Mul(stem, 0.7f + (1 - Mathf.Abs(fx)) * 0.4f);
                        c.a = 1;
                        Put(pix, stride, cell, exposed, x, y, c);
                    }
                }
        }

        static void PaintLadder(Color[] pix, int stride, int cell, bool exposed)
        {
            Color wood = new(0.52f, 0.36f, 0.20f);
            for (int y = 0; y < TS; y++)
            {
                for (int x = 7; x < 12; x++) Put(pix, stride, cell, exposed, x, y, Mul(wood, x == 7 ? 1.3f : 0.9f));
                for (int x = 28; x < 33; x++) Put(pix, stride, cell, exposed, x, y, Mul(wood, x == 28 ? 1.3f : 0.9f));
            }
            for (int y = 6; y < TS; y += 13)
                for (int x = 7; x < 33; x++)
                    for (int t = 0; t < 4; t++)
                        Put(pix, stride, cell, exposed, x, y + t, Mul(wood, t == 0 ? 1.35f : 0.8f));
        }
    }
}
