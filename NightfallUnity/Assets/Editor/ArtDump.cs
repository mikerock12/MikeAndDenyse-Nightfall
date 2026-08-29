using System.IO;
using Nightfall;
using UnityEditor;
using UnityEngine;

namespace Nightfall.Editor
{
    /// <summary>Writes the procedural art to disk so it can be eyeballed without a device.</summary>
    public static class ArtDump
    {
        public static void Run()
        {
            try
            {
                string dir = @"D:\MikeAndDenyse\tools\artdump";
                Directory.CreateDirectory(dir);
                ArtGen.Ensure();

                Save(dir, "fx_glow", ArtGen.Glow);
                Save(dir, "fx_spark", ArtGen.Spark);
                Save(dir, "fx_star", ArtGen.Star);
                Save(dir, "icon_heart", ArtGen.Heart);
                Save(dir, "icon_heart_empty", ArtGen.HeartEmpty);
                Save(dir, "icon_soul", ArtGen.Soul);
                Save(dir, "fx_slash", ArtGen.Slash);
                Save(dir, "icon_bell", ArtGen.Bell);
                Save(dir, "sky_moon", ArtGen.Moon);
                Save(dir, "ui_tri", ArtGen.Tri);
                Save(dir, "ui_ring", ArtGen.Ring);
                Save(dir, "ui_disc", ArtGen.Disc);
                Save(dir, "ui_flag", ArtGen.Flag);
                Save(dir, "ui_panel", ArtGen.PanelSpr.texture);
                Save(dir, "ui_button", ArtGen.ButtonSpr.texture);
                Save(dir, "ui_button_ghost", ArtGen.ButtonGhostSpr.texture);
                Save(dir, "screen_vignette", ArtGen.Vignette());

                foreach (int wi in new[] { 0, 6, 7, 12 })
                {
                    var w = Catalog.Worlds[wi];
                    Save(dir, "atlas_" + w.Id, ArtGen.TileAtlas(w));
                    Save(dir, "sky_" + w.Id, ArtGen.Sky(Parse(w.Tone), Parse(w.Fog)));
                    Save(dir, "ridge_" + w.Id, ArtGen.Ridge(512, 170, wi * 13, wi % 3));
                    Save(dir, "level_" + w.Id, LevelMap(wi));
                    Save(dir, "frame_" + w.Id, MockFrame(wi));
                }

                Debug.Log("ARTDUMP OK " + dir);
                EditorApplication.Exit(0);
            }
            catch (System.Exception e)
            {
                Debug.LogError("ARTDUMP FAIL: " + e);
                EditorApplication.Exit(2);
            }
        }

        static Color Parse(string h)
        {
            if (string.IsNullOrEmpty(h)) return Color.gray;
            if (h[0] != '#') h = "#" + h;
            return ColorUtility.TryParseHtmlString(h, out var c) ? c : Color.gray;
        }

        /// <summary>Whole level as a one-pixel-per-tile map, to sanity-check the layout and the seals.</summary>
        static Texture2D LevelMap(int world)
        {
            var L = LevelBuilder.Compile(world);
            int scale = 3;
            var t = new Texture2D(L.Cols * scale, L.Rows * scale, TextureFormat.RGBA32, false);
            var px = new Color[t.width * t.height];
            for (int y = 0; y < L.Rows; y++)
                for (int x = 0; x < L.Cols; x++)
                {
                    Color c = L.Tiles[y][x] switch
                    {
                        T.Solid => new Color(0.35f, 0.30f, 0.22f),
                        T.Platform => new Color(0.85f, 0.72f, 0.35f),
                        T.Spike => new Color(0.95f, 0.20f, 0.25f),
                        T.Lava => new Color(1f, 0.45f, 0.10f),
                        T.Ice => new Color(0.65f, 0.90f, 1f),
                        T.Water => new Color(0.20f, 0.50f, 0.85f),
                        T.Break => new Color(0.55f, 0.40f, 0.25f),
                        T.Bounce => new Color(0.35f, 0.95f, 0.40f),
                        T.Ladder => new Color(0.70f, 0.50f, 0.30f),
                        T.Thorn => new Color(0.55f, 0.75f, 0.20f),
                        _ => new Color(0.05f, 0.04f, 0.07f)
                    };
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++)
                            px[(t.height - 1 - (y * scale + sy)) * t.width + x * scale + sx] = c;
                }
            Mark(px, t, L.Spawn, scale, Color.white);
            Mark(px, t, L.Exit, scale, new Color(1f, 0.9f, 0.2f));
            Mark(px, t, L.BossAt, scale, new Color(1f, 0.2f, 0.8f));
            if (L.Check.HasValue) Mark(px, t, L.Check.Value, scale, new Color(0.3f, 1f, 0.9f));
            t.SetPixels(px); t.Apply();
            Debug.Log("level " + world + " cols=" + L.Cols + " sealed=" + L.SealedPockets +
                      " ents=" + L.Ents.Count + " items=" + L.Items.Count + " decor=" + L.Decor.Count);
            return t;
        }

        static void Mark(Color[] px, Texture2D t, Vector2 p, int scale, Color c)
        {
            int tx = Mathf.RoundToInt(p.x / T.Tile) * scale, ty = Mathf.RoundToInt(p.y / T.Tile) * scale;
            for (int dy = -3; dy <= 3; dy++)
                for (int dx = -3; dx <= 3; dx++)
                {
                    int x = tx + dx, y = ty + dy;
                    if (x < 0 || y < 0 || x >= t.width || y >= t.height) continue;
                    px[(t.height - 1 - y) * t.width + x] = c;
                }
        }

        // ── software compositor: approximates one in-game frame so the look can be judged offline ──

        static Color[] _buf;
        static int _bw, _bh;

        static Texture2D MockFrame(int world)
        {
            var L = LevelBuilder.Compile(world);
            var w = L.World;
            _bw = T.ViewW; _bh = T.ViewH;
            _buf = new Color[_bw * _bh];

            float camX = L.Spawn.x + 340, camY = Mathf.Max(0, L.Spawn.y - T.ViewH * 0.55f);
            camX = Mathf.Clamp(camX, 0, Mathf.Max(0, L.Cols * T.Tile - T.ViewW));
            camY = Mathf.Clamp(camY, 0, Mathf.Max(0, L.Rows * T.Tile - T.ViewH));

            var sky = ArtGen.Sky(Parse(w.Tone), Parse(w.Fog));
            for (int y = 0; y < _bh; y++)
            {
                var c = sky.GetPixelBilinear(0.5f, 1f - y / (float)_bh);
                for (int x = 0; x < _bw; x++) _buf[y * _bw + x] = c;
            }

            float seed = Mathf.Abs((w.Id ?? "x").GetHashCode() % 500);
            int style = w.Id is "castle" or "cathedral" or "village" or "throne" or "cabin" ? 2
                      : w.Id is "peak" or "ice" or "abyss" or "volcano" ? 1 : 0;
            var far = ArtGen.Ridge(512, 170, seed, style);
            var near = ArtGen.Ridge(512, 150, seed + 31, style == 2 ? 2 : 0);
            var fog = Parse(w.Fog); var tone = Parse(w.Tone);
            var farTint = ArtGen.Lift(fog, 1.35f);
            Layer(far, 0, T.ViewH - 268, T.ViewW, 170, new Color(farTint.r, farTint.g, farTint.b, 0.9f));
            Layer(near, 0, T.ViewH - 196, T.ViewW, 152, new Color(tone.r * 0.75f, tone.g * 0.75f, tone.b * 0.85f, 0.98f));
            for (int i = 0; i < 5; i++)
                Fog(0, T.ViewH - 160 + i * 32, T.ViewW, 32, new Color(fog.r, fog.g, fog.b, 0.05f + i * 0.025f));

            var atlas = ArtGen.TileAtlas(w);
            int x0 = Mathf.Max(0, Mathf.FloorToInt(camX / T.Tile));
            int x1 = Mathf.Min(L.Cols - 1, Mathf.CeilToInt((camX + T.ViewW) / T.Tile));
            int y0 = Mathf.Max(0, Mathf.FloorToInt(camY / T.Tile));
            int y1 = Mathf.Min(L.Rows - 1, Mathf.CeilToInt((camY + T.ViewH) / T.Tile));
            for (int ty = y0; ty <= y1; ty++)
                for (int tx = x0; tx <= x1; tx++)
                {
                    int t = L.Tiles[ty][tx];
                    if (t == 0 || t > 10) continue;
                    int above = ty > 0 ? L.Tiles[ty - 1][tx] : 0;
                    bool exposed = !(above == T.Solid || above == T.Ice || above == T.Break || above == t);
                    bool liquid = t == T.Lava || t == T.Water;
                    bool mirror = !liquid && t != T.Ladder && ArtGen.Hash(tx, ty, 9.3f) > 0.5f;
                    float deep = Mathf.Clamp01((ty - (L.Rows - 6)) / 6f);
                    float shade = liquid ? 1f : (0.88f + ArtGen.Hash(tx, ty, 5.1f) * 0.22f) * (1f - deep * 0.28f);
                    Cell(atlas, t, exposed, Mathf.RoundToInt(tx * T.Tile - camX), Mathf.RoundToInt(ty * T.Tile - camY), mirror, shade);
                }

            // vignette on top, like the live frame
            var vig = ArtGen.Vignette();
            for (int y = 0; y < _bh; y++)
                for (int x = 0; x < _bw; x++)
                {
                    var v = vig.GetPixelBilinear(x / (float)_bw, 1f - y / (float)_bh);
                    Blend(x, y, new Color(0, 0, 0, v.a));
                }

            // buffer row 0 is the top of the screen; a PNG's row 0 is the bottom, so flip on the way out
            var flip = new Color[_buf.Length];
            for (int y = 0; y < _bh; y++)
                System.Array.Copy(_buf, y * _bw, flip, (_bh - 1 - y) * _bw, _bw);
            var tex = new Texture2D(_bw, _bh, TextureFormat.RGBA32, false);
            tex.SetPixels(flip); tex.Apply();
            return tex;
        }

        static void Fog(int x0, int y0, int w, int h, Color c)
        {
            for (int y = 0; y < h; y++)
                for (int x = 0; x < w; x++)
                    Blend(x0 + x, y0 + y, c);
        }

        static void Blend(int x, int y, Color c)
        {
            if (x < 0 || y < 0 || x >= _bw || y >= _bh || c.a <= 0) return;
            int i = y * _bw + x;
            var d = _buf[i];
            _buf[i] = new Color(
                d.r + (c.r - d.r) * c.a,
                d.g + (c.g - d.g) * c.a,
                d.b + (c.b - d.b) * c.a, 1f);
        }

        static void Layer(Texture2D t, int dx, int dy, int dw, int dh, Color tint)
        {
            for (int y = 0; y < dh; y++)
                for (int x = 0; x < dw; x++)
                {
                    var s = t.GetPixelBilinear(x / (float)dw, 1f - y / (float)dh);
                    if (s.a <= 0.01f) continue;
                    Blend(dx + x, dy + y, new Color(s.r * tint.r, s.g * tint.g, s.b * tint.b, s.a * tint.a));
                }
        }

        static void Cell(Texture2D atlas, int cell, bool exposed, int dx, int dy, bool mirror, float shade)
        {
            int baseY = exposed ? T.Tile : 0;
            for (int y = 0; y < T.Tile; y++)
                for (int x = 0; x < T.Tile; x++)
                {
                    int sx = mirror ? T.Tile - 1 - x : x;
                    var s = atlas.GetPixel(cell * T.Tile + sx, baseY + (T.Tile - 1 - y));
                    Blend(dx + x, dy + y, new Color(s.r * shade, s.g * shade, s.b * shade, s.a));
                }
        }

        static void Save(string dir, string name, Texture2D tex)
        {
            if (tex == null) return;
            File.WriteAllBytes(Path.Combine(dir, name + ".png"), tex.EncodeToPNG());
        }
    }
}
