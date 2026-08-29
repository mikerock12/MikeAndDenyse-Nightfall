using System.Collections.Generic;
using UnityEngine;

namespace Nightfall
{
    public class WorldView : MonoBehaviour
    {
        [System.NonSerialized] public GameSim Sim;
        [System.NonSerialized] public SpriteBank Bank;
        public bool Visible;

        Camera _cam;
        Texture2D _atlas, _sky, _ridgeFar, _ridgeNear, _vig;
        Color _tone, _fog, _lip;
        int _builtWorld = -1;
        int _openGroups;
        float _shx, _shy, _flash;
        readonly List<Part> _parts = new();
        readonly List<FxEvent> _fxQueue = new();
        float _weatherT;
        int _weather;

        struct Part
        {
            public float X, Y, Vx, Vy, Life, Max, Size, Grav, Spin, Rot;
            public Color C;
            public byte Tex;    // 0 glow · 1 spark · 2 smoke · 3 shard · 4 star · 5 drop
        }

        /// <summary>Expanding (or imploding) shockwave ring drawn at an impact.</summary>
        struct Wave { public float X, Y, Life, Max, R0, R1, Thick; public Color C; }

        readonly List<Wave> _waves = new();

        // ───────────────────────────── boot ─────────────────────────────

        public void Boot(GameSim sim, SpriteBank bank)
        {
            Sim = sim;
            Bank = bank;
            ArtGen.Ensure();
            _cam = Camera.main;
            if (_cam == null) _cam = FindAnyObjectByType<Camera>();
            if (_cam == null) _cam = gameObject.AddComponent<Camera>();
            _cam.orthographic = true;
            _cam.orthographicSize = T.ViewH / 2f;
            _cam.backgroundColor = Color.black;
            _cam.clearFlags = CameraClearFlags.SolidColor;
            _cam.nearClipPlane = -10;
            _cam.farClipPlane = 100;
            _cam.transform.position = new Vector3(T.ViewW / 2f, T.ViewH / 2f, -10);
            if (_vig == null) _vig = ArtGen.Vignette();
        }

        public void PrepareWorld()
        {
            ArtGen.Ensure();
            if (Sim?.Level?.World == null) return;
            var w = Sim.Level.World;
            // these are native objects: without an explicit Destroy every level start leaks a set
            Drop(ref _atlas); Drop(ref _sky); Drop(ref _ridgeFar); Drop(ref _ridgeNear);
            _tone = Hex(w.Tone, new Color(0.05f, 0.06f, 0.08f));
            _fog = Hex(w.Fog, new Color(0.10f, 0.12f, 0.16f));
            _lip = Hex(w.Lip, new Color(0.5f, 0.6f, 0.3f));
            _atlas = ArtGen.TileAtlas(w);
            _sky = ArtGen.Sky(_tone, _fog);
            float seed = Mathf.Abs((w.Id ?? "x").GetHashCode() % 500);
            int style = w.Id is "castle" or "cathedral" or "village" or "throne" or "cabin" ? 2
                      : w.Id is "peak" or "ice" or "abyss" or "volcano" ? 1 : 0;
            _ridgeFar = ArtGen.Ridge(512, 170, seed, style);
            _ridgeNear = ArtGen.Ridge(512, 150, seed + 31, style == 2 ? 2 : 0);
            _weather = w.Id switch
            {
                "ice" => 1,                                   // snow
                "volcano" or "throne" => 2,                   // embers
                "swamp" or "catacombs" => 3,                  // drips
                "forest" or "woods" or "coven" => 4,          // fireflies
                _ => 0                                        // dust
            };
            _parts.Clear();
            _waves.Clear();
            _fxQueue.Clear();
            _builtWorld = Sim.WorldIndex;
            _flash = 0;
        }

        static void Drop(ref Texture2D t)
        {
            if (t == null) return;
            Destroy(t);
            t = null;
        }

        void OnDestroy()
        {
            Drop(ref _atlas); Drop(ref _sky); Drop(ref _ridgeFar); Drop(ref _ridgeNear); Drop(ref _vig);
        }

        static Color Hex(string h, Color fallback)
        {
            if (string.IsNullOrEmpty(h)) return fallback;
            if (h[0] != '#') h = "#" + h;
            return ColorUtility.TryParseHtmlString(h, out var c) ? c : fallback;
        }

        // ───────────────────────────── frame update ─────────────────────────────

        void Update()
        {
            if (Sim == null || Sim.Level == null) return;
            if (_builtWorld != Sim.WorldIndex || _atlas == null) PrepareWorld();
            float dt = Mathf.Min(Time.deltaTime, 0.05f);
            DrainFx();
            StepParts(dt);
            for (int i = _waves.Count - 1; i >= 0; i--)
            {
                var w = _waves[i];
                w.Life -= dt;
                if (w.Life <= 0) _waves.RemoveAt(i); else _waves[i] = w;
            }
            Weather(dt);
            _flash = Mathf.Max(0, _flash - dt * 3.2f);
            _shx = (Random.value - 0.5f) * Sim.Shake;
            _shy = (Random.value - 0.5f) * Sim.Shake;
        }

        /// <summary>NightApp owns the sim's FX list and hands events over after the snapshot is built.</summary>
        public void PushFx(FxEvent e)
        {
            if (_fxQueue.Count < 96) _fxQueue.Add(e);
        }

        void DrainFx()
        {
            if (_fxQueue.Count == 0) return;
            foreach (var f in _fxQueue)
            {
                switch ((FxKind)f.Kind)
                {
                    case FxKind.Hit: Burst(f.X, f.Y, 9, new Color(1f, 0.86f, 0.55f), 150, 1, 0.30f); break;

                    // the blade arc itself is drawn from the hero's attack timer; this is the contact
                    case FxKind.Slash:
                        Burst(f.X, f.Y, 14, new Color(1f, 0.95f, 0.74f), 230, 1, 0.24f);
                        Burst(f.X, f.Y, 7, new Color(1f, 0.70f, 0.34f), 160, 3, 0.34f);
                        Ring(f.X, f.Y, 10, 66, 0.20f, 6, new Color(1f, 0.92f, 0.72f));
                        break;

                    case FxKind.Cast:
                        Burst(f.X, f.Y, 10, new Color(0.80f, 0.62f, 1f), 130, 1, 0.30f, -20);
                        Ring(f.X, f.Y, 6, 44, 0.22f, 5, new Color(0.72f, 0.52f, 1f));
                        break;

                    case FxKind.Charge:
                        Ring(f.X, f.Y, 52, 12, 0.20f, 4, new Color(0.70f, 0.55f, 1f));
                        break;

                    case FxKind.Burst:
                        Burst(f.X, f.Y, 18, new Color(0.86f, 0.70f, 1f), 220, 3, 0.40f);
                        Burst(f.X, f.Y, 10, new Color(1f, 0.98f, 1f), 150, 1, 0.28f);
                        Ring(f.X, f.Y, 8, 74, 0.26f, 7, new Color(0.78f, 0.58f, 1f));
                        break;
                    case FxKind.Death:
                        Burst(f.X, f.Y, 16, new Color(0.62f, 0.25f, 0.72f), 130, 2, 0.55f);
                        Burst(f.X, f.Y, 8, new Color(0.95f, 0.92f, 1f), 90, 1, 0.40f);
                        break;
                    case FxKind.Pickup: Burst(f.X, f.Y, 12, new Color(0.72f, 0.52f, 1f), 90, 4, 0.55f, -30); break;
                    case FxKind.Heal: Burst(f.X, f.Y, 12, new Color(1f, 0.42f, 0.5f), 80, 4, 0.60f, -40); break;
                    case FxKind.Hurt: Burst(f.X, f.Y, 12, new Color(0.94f, 0.24f, 0.28f), 150, 3, 0.45f); _flash = 0.55f; break;
                    case FxKind.Land: Burst(f.X, f.Y, 6, new Color(_lip.r, _lip.g, _lip.b, 1f), 70, 2, 0.30f, -10); break;
                    case FxKind.Boom:
                        Burst(f.X, f.Y, 22, new Color(1f, 0.72f, 0.35f), 210, 2, 0.65f);
                        _flash = 0.5f; break;
                    case FxKind.Rescue: Burst(f.X, f.Y, 18, new Color(0.68f, 0.86f, 1f), 120, 1, 0.55f, -60); break;
                    case FxKind.BossDown:
                        Burst(f.X, f.Y, 34, new Color(1f, 0.55f, 0.25f), 240, 2, 1.0f);
                        Burst(f.X, f.Y, 20, new Color(1f, 0.95f, 0.8f), 180, 1, 0.8f);
                        Ring(f.X, f.Y, 20, 260, 0.55f, 12, new Color(1f, 0.78f, 0.42f));
                        _flash = 0.9f; break;
                }
            }
            _fxQueue.Clear();
        }

        void Ring(float x, float y, float r0, float r1, float life, float thick, Color c)
        {
            if (_waves.Count > 24) return;
            _waves.Add(new Wave { X = x, Y = y, R0 = r0, R1 = r1, Life = life, Max = life, Thick = thick, C = c });
        }

        void Burst(float x, float y, int n, Color c, float spd, byte tex, float life, float grav = 220)
        {
            if (_parts.Count > 260) return;
            for (int i = 0; i < n; i++)
            {
                float a = Random.value * Mathf.PI * 2;
                float s = spd * (0.35f + Random.value * 0.75f);
                _parts.Add(new Part
                {
                    X = x, Y = y,
                    Vx = Mathf.Cos(a) * s, Vy = Mathf.Sin(a) * s - spd * 0.25f,
                    Life = life * (0.6f + Random.value * 0.7f), Max = life,
                    Size = 5 + Random.value * 8, Grav = grav, C = c, Tex = tex,
                    Spin = (Random.value - 0.5f) * 720, Rot = Random.value * 360
                });
            }
        }

        void StepParts(float dt)
        {
            for (int i = _parts.Count - 1; i >= 0; i--)
            {
                var p = _parts[i];
                p.Life -= dt;
                if (p.Life <= 0) { _parts.RemoveAt(i); continue; }
                p.X += p.Vx * dt; p.Y += p.Vy * dt;
                p.Vy += p.Grav * dt;
                p.Vx *= 1 - Mathf.Min(0.9f, 1.6f * dt);
                p.Rot += p.Spin * dt;
                _parts[i] = p;
            }
        }

        void Weather(float dt)
        {
            _weatherT -= dt;
            if (_weatherT > 0 || _parts.Count > 200) return;
            _weatherT = _weather == 1 ? 0.05f : _weather == 2 ? 0.09f : _weather == 3 ? 0.20f : _weather == 4 ? 0.30f : 0.24f;
            float x = Sim.CamX + Random.value * (T.ViewW + 120) - 60;
            float y = Sim.CamY + Random.value * T.ViewH;
            switch (_weather)
            {
                case 1: // snow
                    _parts.Add(new Part { X = x, Y = Sim.CamY - 20, Vx = -14 + Random.value * 28, Vy = 34 + Random.value * 26, Life = 5.5f, Max = 5.5f, Size = 3 + Random.value * 4, Grav = 2, C = new Color(0.92f, 0.97f, 1f, 0.9f), Tex = 0 });
                    break;
                case 2: // embers
                    _parts.Add(new Part { X = x, Y = Sim.CamY + T.ViewH + 20, Vx = -10 + Random.value * 20, Vy = -30 - Random.value * 45, Life = 4.2f, Max = 4.2f, Size = 3 + Random.value * 5, Grav = -8, C = new Color(1f, 0.48f + Random.value * 0.3f, 0.18f, 0.95f), Tex = 0 });
                    break;
                case 3: // cave drips
                    _parts.Add(new Part { X = x, Y = Sim.CamY - 10, Vx = 0, Vy = 60, Life = 3.5f, Max = 3.5f, Size = 4 + Random.value * 3, Grav = 90, C = new Color(0.55f, 0.85f, 0.95f, 0.7f), Tex = 5 });
                    break;
                case 4: // fireflies
                    _parts.Add(new Part { X = x, Y = y, Vx = -18 + Random.value * 36, Vy = -12 + Random.value * 24, Life = 3.4f, Max = 3.4f, Size = 5 + Random.value * 5, Grav = 0, C = new Color(0.75f, 1f, 0.45f, 0.85f), Tex = 0 });
                    break;
                default: // drifting dust
                    _parts.Add(new Part { X = x, Y = y, Vx = -22 + Random.value * 12, Vy = -6 + Random.value * 12, Life = 4.5f, Max = 4.5f, Size = 3 + Random.value * 5, Grav = 0, C = new Color(_lip.r, _lip.g, _lip.b, 0.35f), Tex = 0 });
                    break;
            }
        }

        // ───────────────────────────── draw ─────────────────────────────

        void OnGUI()
        {
            if (!Visible || Sim == null || Sim.Level == null || _atlas == null) return;
            _openGroups = 0;
            var m = GUI.matrix;
            try { DrawFrame(); }
            catch (System.Exception e) { Debug.LogException(e); }
            finally
            {
                // never leave a clip on the stack: a half-open group kills IMGUI for the rest of the run
                while (_openGroups > 0) { GUI.EndGroup(); _openGroups--; }
                GUI.color = Color.white;
                GUI.matrix = m;
            }
        }

        void Begin(Rect r) { GUI.BeginGroup(r); _openGroups++; }
        void End() { if (_openGroups > 0) { GUI.EndGroup(); _openGroups--; } }

        void DrawFrame()
        {
            float sx = Screen.width / (float)T.ViewW;
            float sy = Screen.height / (float)T.ViewH;
            GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(sx, sy, 1));

            DrawSky();
            Begin(new Rect(_shx, _shy, T.ViewW, T.ViewH));
            DrawTiles();
            DrawDecor();
            DrawCheckpoint();
            DrawBell();
            DrawItems();
            DrawActors();
            DrawWaves();
            DrawParts();
            End();

            DrawAtmosphere();
            DrawHud();

            if (Sim.Fade > 0)
            {
                GUI.color = new Color(0, 0, 0, Sim.Fade);
                Quad(0, 0, T.ViewW, T.ViewH);
                GUI.color = Color.white;
            }
            if (Sim.State == "play") DrawControls();
        }

        void Quad(float x, float y, float w, float h)
        {
            GUI.DrawTexture(new Rect(x, y, w, h), ArtGen.Px);
        }

        void Blit(Texture2D t, float x, float y, float w, float h, Color c)
        {
            if (t == null) return;
            GUI.color = c;
            GUI.DrawTexture(new Rect(x, y, w, h), t);
            GUI.color = Color.white;
        }

        void Spin(Texture2D t, float cx, float cy, float w, float h, float deg, Color c)
        {
            var m = GUI.matrix;
            GUIUtility.RotateAroundPivot(deg, new Vector2(cx, cy));
            Blit(t, cx - w / 2, cy - h / 2, w, h, c);
            GUI.matrix = m;
        }

        // ── background ──

        void DrawSky()
        {
            GUI.color = Color.white;
            GUI.DrawTexture(new Rect(0, 0, T.ViewW, T.ViewH), _sky, ScaleMode.StretchToFill);

            float span = Mathf.Max(1, Sim.Level.Cols * T.Tile - T.ViewW);
            float t = Mathf.Clamp01(Sim.CamX / span);

            // moon, barely moving
            Blit(ArtGen.Moon, T.ViewW - 190 - t * 26, 34, 96, 96, new Color(1f, 0.92f, 0.86f, 0.55f));
            Blit(ArtGen.Glow, T.ViewW - 250 - t * 26, -26, 220, 220, new Color(1f, 0.82f, 0.72f, 0.14f));

            // far ridge — lighter than the sky above it so the depth actually reads
            var farTint = ArtGen.Lift(_fog, 1.35f);
            Blit(_ridgeFar, -t * 90, T.ViewH - 268, T.ViewW + 120, 170, new Color(farTint.r, farTint.g, farTint.b, 0.9f));

            // painted world art in between
            var src = Bank != null ? Bank.Tex(Sim.Level.World.Id + "_bg", "worlds/" + Sim.Level.World.Id + "_bg") : null;
            if (src != null)
            {
                float scale = T.ViewH / (float)src.height;
                float iw = src.width * scale;
                float extra = Mathf.Max(0, iw - T.ViewW);
                GUI.color = new Color(1, 1, 1, 0.62f);
                GUI.DrawTexture(new Rect(-t * extra, 0, iw, T.ViewH), src, ScaleMode.StretchToFill);
                GUI.color = Color.white;
            }

            // near ridge, the darkest band, moves most
            Blit(_ridgeNear, -t * 230, T.ViewH - 196, T.ViewW + 260, 152,
                new Color(_tone.r * 0.75f, _tone.g * 0.75f, _tone.b * 0.85f, 0.98f));

            // haze settling into the near ridge
            for (int i = 0; i < 5; i++)
                Blit(ArtGen.Px, 0, T.ViewH - 160 + i * 32, T.ViewW, 32,
                    new Color(_fog.r, _fog.g, _fog.b, 0.05f + i * 0.025f));
        }

        void DrawAtmosphere()
        {
            if (_flash > 0)
            {
                GUI.color = new Color(1f, 0.85f, 0.8f, _flash * 0.30f);
                Quad(0, 0, T.ViewW, T.ViewH);
                GUI.color = Color.white;
            }
            GUI.DrawTexture(new Rect(0, 0, T.ViewW, T.ViewH), _vig, ScaleMode.StretchToFill);
            if (Sim.BossIntro > 0)
            {
                float a = Mathf.Clamp01(Sim.BossIntro / 1.8f);
                GUI.color = new Color(0.5f, 0.03f, 0.06f, a * 0.30f);
                Quad(0, 0, T.ViewW, T.ViewH);
                GUI.color = Color.white;
            }
        }

        // ── world ──

        void DrawTiles()
        {
            var L = Sim.Level;
            int x0 = Mathf.Max(0, Mathf.FloorToInt(Sim.CamX / T.Tile) - 1);
            int x1 = Mathf.Min(L.Cols - 1, Mathf.CeilToInt((Sim.CamX + T.ViewW) / T.Tile) + 1);
            int y0 = Mathf.Max(0, Mathf.FloorToInt(Sim.CamY / T.Tile) - 1);
            int y1 = Mathf.Min(L.Rows - 1, Mathf.CeilToInt((Sim.CamY + T.ViewH) / T.Tile) + 1);
            float pulse = 0.5f + Mathf.Sin(Sim.Time * 3.4f) * 0.5f;

            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++)
                {
                    int t = L.Tiles[y][x];
                    if (t == 0 || t > 10) continue;
                    float dx = x * T.Tile - Sim.CamX, dy = y * T.Tile - Sim.CamY;
                    int above = y > 0 ? L.Tiles[y - 1][x] : 0;
                    bool exposed = !(above == T.Solid || above == T.Ice || above == T.Break || above == t);

                    // one cell per tile type would print a visible grid across the whole level,
                    // so every tile gets its own shade and half of them are mirrored
                    var uv = ArtGen.TileUv(t, exposed);
                    bool liquid = t == T.Lava || t == T.Water;
                    if (!liquid && t != T.Ladder && ArtGen.Hash(x, y, 9.3f) > 0.5f)
                    {
                        uv.x += uv.width;
                        uv.width = -uv.width;
                    }
                    if (!liquid)
                    {
                        float deep = Mathf.Clamp01((y - (L.Rows - 6)) / 6f);
                        float shade = (0.88f + ArtGen.Hash(x, y, 5.1f) * 0.22f) * (1f - deep * 0.28f);
                        GUI.color = new Color(shade, shade, shade * 1.02f, 1f);
                    }
                    GUI.DrawTextureWithTexCoords(new Rect(dx, dy, T.Tile, T.Tile), _atlas, uv);
                    GUI.color = Color.white;

                    if (t == T.Lava)
                    {
                        Blit(ArtGen.Px, dx, dy, T.Tile, T.Tile, new Color(1f, 0.35f + pulse * 0.35f, 0.08f, 0.22f));
                        if (exposed)
                            Blit(ArtGen.Glow, dx - 14, dy - 26, T.Tile + 28, 40, new Color(1f, 0.45f, 0.12f, 0.30f + pulse * 0.16f));
                    }
                    else if (t == T.Water)
                    {
                        Blit(ArtGen.Px, dx, dy, T.Tile, T.Tile, new Color(0.16f, 0.47f, 0.63f, 0.18f + pulse * 0.08f));
                        if (exposed)
                            Blit(ArtGen.Px, dx, dy + Mathf.Sin(Sim.Time * 2.4f + x) * 1.6f, T.Tile, 3, new Color(0.75f, 0.95f, 1f, 0.35f));
                    }
                    else if (t == T.Spike && exposed)
                        Blit(ArtGen.Glow, dx - 6, dy - 6, T.Tile + 12, T.Tile + 12, new Color(0.9f, 0.3f, 0.35f, 0.10f));
                    else if (t == T.Bounce && exposed)
                        Blit(ArtGen.Glow, dx - 10, dy - 14, T.Tile + 20, 34, new Color(0.55f, 1f, 0.5f, 0.16f + pulse * 0.10f));
                }
        }

        void DrawDecor()
        {
            var decor = Sim.Level.Decor;
            if (decor == null) return;
            foreach (var d in decor)
            {
                float x = d.x - Sim.CamX, y = d.y - Sim.CamY;
                if (x < -220 || x > T.ViewW + 220) continue;
                float s = d.scale;
                switch (d.kind)
                {
                    case "tree":
                    case "canopy":
                        Blit(ArtGen.Px, x - 5 * s, y - 74 * s, 10 * s, 74 * s, new Color(0.16f, 0.11f, 0.07f, 1f));
                        Blit(ArtGen.Disc, x - 46 * s, y - 128 * s, 92 * s, 74 * s, new Color(_lip.r * 0.35f, _lip.g * 0.42f, _lip.b * 0.3f, 0.95f));
                        Blit(ArtGen.Disc, x - 30 * s, y - 150 * s, 62 * s, 56 * s, new Color(_lip.r * 0.26f, _lip.g * 0.34f, _lip.b * 0.24f, 0.95f));
                        break;
                    case "pillar":
                        Blit(ArtGen.Px, x - 13 * s, y - 118 * s, 26 * s, 118 * s, new Color(0.20f, 0.17f, 0.20f, 1f));
                        Blit(ArtGen.Px, x - 18 * s, y - 126 * s, 36 * s, 12 * s, new Color(0.28f, 0.24f, 0.27f, 1f));
                        Blit(ArtGen.Px, x - 18 * s, y - 10 * s, 36 * s, 10 * s, new Color(0.24f, 0.20f, 0.23f, 1f));
                        break;
                    case "tomb":
                        Blit(ArtGen.Px, x - 13 * s, y - 40 * s, 26 * s, 40 * s, new Color(0.30f, 0.30f, 0.32f, 1f));
                        Blit(ArtGen.Disc, x - 13 * s, y - 50 * s, 26 * s, 24 * s, new Color(0.30f, 0.30f, 0.32f, 1f));
                        Blit(ArtGen.Px, x - 3 * s, y - 34 * s, 6 * s, 18 * s, new Color(0.16f, 0.16f, 0.18f, 1f));
                        Blit(ArtGen.Px, x - 9 * s, y - 28 * s, 18 * s, 5 * s, new Color(0.16f, 0.16f, 0.18f, 1f));
                        break;
                    case "obelisk":
                        Blit(ArtGen.Px, x - 11 * s, y - 96 * s, 22 * s, 96 * s, new Color(0.42f, 0.32f, 0.18f, 1f));
                        Blit(ArtGen.Tri, x - 11 * s, y - 118 * s, 22 * s, 24 * s, new Color(0.52f, 0.40f, 0.22f, 1f));
                        break;
                    case "crystal":
                        Blit(ArtGen.Shard, x - 15 * s, y - 62 * s, 30 * s, 62 * s, new Color(0.55f, 0.80f, 0.98f, 0.85f));
                        Blit(ArtGen.Glow, x - 34 * s, y - 82 * s, 68 * s, 68 * s, new Color(0.5f, 0.8f, 1f, 0.18f));
                        break;
                    case "rock":
                        Blit(ArtGen.Disc, x - 26 * s, y - 30 * s, 52 * s, 34 * s, new Color(0.22f, 0.19f, 0.18f, 1f));
                        Blit(ArtGen.Disc, x - 12 * s, y - 42 * s, 30 * s, 26 * s, new Color(0.26f, 0.22f, 0.21f, 1f));
                        break;
                    case "reed":
                        for (int i = -2; i <= 2; i++)
                            Blit(ArtGen.Px, x + i * 8 * s, y - (34 + Mathf.Abs(i) * -6) * s, 3 * s, (34 - Mathf.Abs(i) * 6) * s,
                                new Color(0.22f, 0.30f, 0.20f, 0.95f));
                        break;
                    case "torch":
                    case "candle":
                    case "lamp":
                        {
                            float fl = 0.72f + Mathf.Sin(Sim.Time * 9f + x * 0.05f) * 0.28f;
                            Blit(ArtGen.Px, x - 3 * s, y - 26 * s, 6 * s, 26 * s, new Color(0.24f, 0.17f, 0.11f, 1f));
                            Blit(ArtGen.Glow, x - 52 * s * fl, y - 66 * s * fl, 104 * s * fl, 104 * s * fl, new Color(1f, 0.66f, 0.28f, 0.24f));
                            Blit(ArtGen.Soul, x - 8 * s, y - 46 * s, 16 * s, 26 * s, new Color(1f, 0.72f, 0.28f, 0.95f));
                            break;
                        }
                    case "shrine":
                        Blit(ArtGen.Px, x - 22 * s, y - 16 * s, 44 * s, 16 * s, new Color(0.26f, 0.22f, 0.24f, 1f));
                        Blit(ArtGen.Ring, x - 26 * s, y - 76 * s, 52 * s, 52 * s, new Color(0.85f, 0.72f, 0.42f, 0.85f));
                        Blit(ArtGen.Glow, x - 46 * s, y - 96 * s, 92 * s, 92 * s, new Color(1f, 0.85f, 0.5f, 0.14f));
                        break;
                    case "arch":
                        Blit(ArtGen.Px, x - 62 * s, y - 128 * s, 20 * s, 128 * s, new Color(0.19f, 0.16f, 0.19f, 1f));
                        Blit(ArtGen.Px, x + 42 * s, y - 128 * s, 20 * s, 128 * s, new Color(0.19f, 0.16f, 0.19f, 1f));
                        Blit(ArtGen.Px, x - 66 * s, y - 142 * s, 132 * s, 20 * s, new Color(0.23f, 0.19f, 0.22f, 1f));
                        break;
                    case "arena":
                        Blit(ArtGen.Glow, x - 300 * s, y - 300 * s, 600 * s, 400 * s, new Color(0.55f, 0.06f, 0.10f, 0.14f));
                        break;
                }
            }
        }

        void DrawCheckpoint()
        {
            if (!Sim.Level.Check.HasValue) return;
            var c = Sim.Level.Check.Value;
            float x = c.x - Sim.CamX, y = c.y - Sim.CamY;
            float glow = 0.5f + Mathf.Sin(Sim.Time * 2.6f) * 0.5f;
            Blit(ArtGen.Glow, x - 44, y - 76, 108, 108, new Color(1f, 0.85f, 0.45f, 0.10f + glow * 0.08f));
            Blit(ArtGen.Flag, x, y - 46, 28, 46, Color.white);
        }

        void DrawBell()
        {
            if (!Sim.LockedArena) return;
            bool bossLive = false;
            foreach (var b in Sim.Bosses) if (!b.Dead) bossLive = true;
            if (bossLive) return;
            if (Sim.WorldIndex == 15 && !Sim.BelialPhase) return;
            float x = Sim.Level.Exit.x - Sim.CamX;
            float y = Sim.Level.Exit.y - Sim.CamY;
            float sway = Mathf.Sin(Sim.Time * 2.2f) * 5f;
            float glow = 0.55f + Mathf.Sin(Sim.Time * 4f) * 0.45f;
            Blit(ArtGen.Glow, x - 52, y - 76, 128, 128, new Color(1f, 0.88f, 0.5f, 0.16f + glow * 0.20f));
            Blit(ArtGen.Px, x + 8, y - 42, 4, 22, new Color(0.28f, 0.22f, 0.14f));
            Spin(ArtGen.Bell, x + 10, y - 6, 40, 40, sway, Color.white);
            GuiText(new Rect(x - 90, y + 34, 200, 22), "TOQUE O SINO", 13, new Color(1f, 0.88f, 0.55f, 0.85f), TextAnchor.MiddleCenter);
        }

        void DrawItems()
        {
            foreach (var it in Sim.Items)
            {
                if (it.Taken) continue;
                float bob = Mathf.Sin(it.Bob * 3) * 3;
                float x = it.X - Sim.CamX, y = it.Y + bob - Sim.CamY;
                var tex = Bank != null ? Bank.Tex(it.Type, "items/" + it.Type) : null;
                Color halo = it.Type == "heart" ? new Color(1f, 0.35f, 0.45f, 0.22f) : new Color(0.72f, 0.5f, 1f, 0.22f);
                Blit(ArtGen.Glow, x - 22, y - 22, 64, 64, halo);
                if (tex) GUI.DrawTexture(new Rect(x - 4, y - 4, 26, 26), tex, ScaleMode.ScaleToFit);
                else if (it.Type == "heart") Blit(ArtGen.Heart, x - 2, y - 2, 22, 22, Color.white);
                else Blit(ArtGen.Soul, x, y - 4, 18, 26, Color.white);
            }
        }

        void Shadow(float x, float y, float w)
        {
            Blit(ArtGen.Disc, x - Sim.CamX - w * 0.15f, y - Sim.CamY - 5, w * 1.3f, 11, new Color(0, 0, 0, 0.34f));
        }

        void DrawSpr(Texture2D tex, float x, float y, float w, float h, bool flip, bool flash, float tint = 1f)
        {
            var box = new Rect(x - Sim.CamX, y - Sim.CamY, Mathf.Abs(w), Mathf.Abs(h));
            if (flash) GUI.color = new Color(1.9f, 1.5f, 1.5f, 1f);
            else if (tint != 1f) GUI.color = new Color(tint, tint, tint, 1f);
            if (tex)
            {
                var dest = FitFeet(box, tex.width, tex.height);
                var uv = flip ? new Rect(1f, 0f, -1f, 1f) : new Rect(0f, 0f, 1f, 1f);
                GUI.DrawTextureWithTexCoords(dest, tex, uv);
            }
            else
            {
                GUI.color = new Color(0.32f, 0.10f, 0.16f, 0.92f);
                GUI.DrawTexture(box, ArtGen.Px);
            }
            GUI.color = Color.white;
        }

        static Rect FitFeet(Rect box, int tw, int th)
        {
            if (tw < 1 || th < 1) return box;
            float aspect = tw / (float)th;
            float bw = box.width, bh = box.height;
            float dw = bw, dh = bh;
            if (bw / bh > aspect) dw = bh * aspect;
            else dh = bw / aspect;
            return new Rect(box.x + (bw - dw) * 0.5f, box.y + (bh - dh), dw, dh);
        }

        void DrawActors()
        {
            foreach (var e in Sim.Enemies)
            {
                if (e.Dead) continue;
                Shadow(e.X + e.W / 2, e.Y + e.H, e.W);
                DrawSpr(Bank.Tex(e.Id, "enemies/" + e.Id, "en_" + e.Id), e.X - 8, e.Y - 10, e.W + 16, e.H + 12, e.Facing < 0, e.Flash > 0);
                if (e.MaxHp > 2 && e.Hp < e.MaxHp)
                {
                    float bx = e.X - Sim.CamX, by = e.Y - Sim.CamY - 9;
                    Blit(ArtGen.Px, bx, by, e.W, 4, new Color(0.08f, 0.02f, 0.04f, 0.85f));
                    Blit(ArtGen.Px, bx + 1, by + 1, (e.W - 2) * Mathf.Clamp01(e.Hp / (float)e.MaxHp), 2, new Color(0.85f, 0.35f, 0.30f, 0.95f));
                }
            }

            foreach (var b in Sim.Bosses)
            {
                if (b.Dead) continue;
                float pulse = 0.5f + Mathf.Sin(Sim.Time * 3f) * 0.5f;
                Blit(ArtGen.Glow, b.X - Sim.CamX - b.W * 0.4f, b.Y - Sim.CamY - b.H * 0.25f, b.W * 1.8f, b.H * 1.5f,
                    new Color(0.75f, 0.12f, 0.18f, 0.12f + pulse * 0.08f));
                Shadow(b.X + b.W / 2, b.Y + b.H, b.W);
                DrawSpr(Bank.Tex(b.Id, "bosses/" + b.Id, "boss_" + b.Id), b.X - 8, b.Y - 10, b.W + 16, b.H + 12, b.Facing < 0, b.Flash > 0);
            }

            foreach (var p in Sim.Players)
            {
                if (p.Dead) continue;
                bool blink = p.Inv > 0 && Mathf.FloorToInt(Sim.Time * 18) % 2 == 0;
                Shadow(p.X + p.W / 2, p.Y + p.H, p.W);
                if (blink) continue;
                string id = p.Hero.Id;
                Texture2D tex = p.Anim == "attack" ? Bank.Tex(id + "_attack") :
                    (p.Anim == "jump" || p.Anim == "fall") ? Bank.Tex(id + "_jump") :
                    p.Anim == "walk" ? Bank.Tex(id + "_walk" + ((Mathf.FloorToInt(p.AnimT * 7) % 2) + 1)) :
                    Bank.Tex(id + "_idle");
                tex ??= Bank.Tex(id + "_idle");
                float bob = p.Anim == "idle" ? Mathf.Sin(Sim.Time * 3f) * 1.2f : 0;
                if (id == "denyse")
                    Blit(ArtGen.Glow, p.X - Sim.CamX - 18, p.Y - Sim.CamY - 12, p.W + 36, p.H + 30, new Color(0.62f, 0.42f, 1f, 0.10f));
                if (p.Atk > 0 && p.Hero.AtkKind != "magic") Swing(p);
                DrawSpr(tex, p.X - 14, p.Y - 16 + bob, p.W + 28, p.H + 18, p.Facing < 0, p.HurtT > 0);
                if (p.Atk > 0 && p.Hero.AtkKind == "magic") Channel(p);
                // slot pip so two players never lose track of each other
                Blit(ArtGen.Disc, p.X - Sim.CamX + p.W / 2 - 4, p.Y - Sim.CamY - 24, 8, 8,
                    p.Slot == 0 ? new Color(1f, 0.85f, 0.45f, 0.85f) : new Color(0.55f, 0.85f, 1f, 0.85f));
            }

            foreach (var pr in Sim.Projectiles)
            {
                float x = pr.X - Sim.CamX, y = pr.Y - Sim.CamY;
                Color core, halo;
                switch (pr.Kind)
                {
                    case "magic": core = new Color(0.88f, 0.76f, 1f); halo = new Color(0.62f, 0.34f, 1f, 0.45f); break;
                    case "ice": core = new Color(0.88f, 0.98f, 1f); halo = new Color(0.45f, 0.80f, 1f, 0.45f); break;
                    case "hell": core = new Color(1f, 0.88f, 0.55f); halo = new Color(1f, 0.36f, 0.10f, 0.5f); break;
                    case "holy": core = new Color(1f, 0.98f, 0.82f); halo = new Color(1f, 0.86f, 0.42f, 0.45f); break;
                    case "bone": core = new Color(0.95f, 0.92f, 0.84f); halo = new Color(0.7f, 0.66f, 0.55f, 0.35f); break;
                    default: core = new Color(0.92f, 0.72f, 1f); halo = new Color(0.48f, 0.10f, 0.72f, 0.5f); break;
                }
                float ang = Mathf.Atan2(pr.Vy, pr.Vx) * Mathf.Rad2Deg;

                // comet trail behind the bolt, oldest and faintest first
                for (int k = 5; k >= 1; k--)
                {
                    float gx = x - pr.Vx * 110f * 0.011f * k;
                    float gy = y - pr.Vy * 110f * 0.011f * k;
                    float ga = (0.34f - k * 0.05f) * (pr.Friendly ? 1.1f : 0.85f);
                    float gs = 34 - k * 3;
                    Blit(ArtGen.Glow, gx - gs / 2 + 6, gy - gs / 2 + 6, gs, gs,
                        new Color(halo.r, halo.g, halo.b, Mathf.Max(0, ga)));
                }

                Blit(ArtGen.Glow, x - 22, y - 22, 60, 60, halo);
                if (pr.Friendly)
                    Spin(ArtGen.Ring, x + 6, y + 6, 34, 34, Sim.Time * 300f, new Color(core.r, core.g, core.b, 0.55f));
                Spin(ArtGen.Soul, x + 6, y + 6, 14, 26, ang + 90, core);
                Blit(ArtGen.Glow, x - 4, y - 2, 24, 20, new Color(core.r, core.g, core.b, 0.9f));
                Blit(ArtGen.Star, x - 4, y - 4, 24, 24, new Color(1f, 1f, 1f, 0.55f));
            }
        }

        /// <summary>
        /// Mike's blade. The arc is anchored to the hero and sweeps overhead-to-low across the
        /// swing, trailing after-images, with a bright tip riding the leading edge — it matches
        /// the hitbox the sim actually tests, so what you see is what cuts.
        /// </summary>
        void Swing(PlayerA p)
        {
            float t = Mathf.Clamp01(1f - p.Atk / Mathf.Max(0.01f, p.Hero.AtkTime));
            float cx = p.X + p.W / 2 - Sim.CamX, cy = p.Y + p.H * 0.46f - Sim.CamY;
            float reach = p.Hero.Reach;
            float fade = Mathf.Sin(Mathf.Clamp01(t * 1.05f) * Mathf.PI);
            int dir = p.Facing >= 0 ? 1 : -1;

            for (int k = 3; k >= 0; k--)
            {
                float tk = t - k * 0.075f;
                if (tk < 0.04f) continue;
                float ang = Mathf.Lerp(-52f, 44f, Mathf.Clamp01((tk - 0.12f) / 0.76f));
                float a = fade * (1f - k * 0.26f);
                if (a <= 0.02f) continue;
                // the texture's bright band sits at 0.718 of its width, so this puts the arc
                // exactly on the reach the hitbox uses
                float size = reach * 1.39f * (0.94f + k * 0.03f);
                var m = GUI.matrix;
                GUIUtility.RotateAroundPivot(ang * dir, new Vector2(cx, cy));
                var r = new Rect(dir > 0 ? cx : cx - size, cy - size / 2f, size, size);
                GUI.color = new Color(1f, 0.94f + k * 0.02f, 0.80f, a * (k == 0 ? 0.95f : 0.55f));
                GUI.DrawTextureWithTexCoords(r, ArtGen.Slash, dir > 0 ? new Rect(0, 0, 1, 1) : new Rect(1, 0, -1, 1));
                GUI.color = Color.white;
                GUI.matrix = m;
            }

            // blade tip riding the arc
            float lead = Mathf.Lerp(-52f, 44f, Mathf.Clamp01((t - 0.12f) / 0.76f)) * Mathf.Deg2Rad;
            float tx = cx + Mathf.Cos(lead) * reach * 0.92f * dir;
            float ty = cy + Mathf.Sin(lead) * reach * 0.92f;
            Blit(ArtGen.Glow, tx - 22, ty - 22, 44, 44, new Color(1f, 0.96f, 0.82f, fade * 0.75f));
            Blit(ArtGen.Star, tx - 14, ty - 14, 28, 28, new Color(1f, 1f, 0.92f, fade * 0.9f));
        }

        /// <summary>Denyse gathers the crystal, then lets it go — the wind-up the sim waits for.</summary>
        void Channel(PlayerA p)
        {
            float t = Mathf.Clamp01(1f - p.Atk / Mathf.Max(0.01f, p.Hero.AtkTime));
            int dir = p.Facing >= 0 ? 1 : -1;
            float hx = p.X + p.W / 2 + dir * 20 - Sim.CamX;
            float hy = p.Y + 18 - Sim.CamY;
            if (t < 0.35f)
            {
                float g = t / 0.35f;
                float size = Mathf.Lerp(46f, 16f, g);
                Blit(ArtGen.Ring, hx - size / 2, hy - size / 2, size, size, new Color(0.72f, 0.52f, 1f, 0.35f + g * 0.55f));
                Blit(ArtGen.Glow, hx - 24, hy - 24, 48, 48, new Color(0.65f, 0.45f, 1f, g * 0.55f));
                Spin(ArtGen.Shard, hx, hy, 10 + g * 8, 10 + g * 8, Sim.Time * 420f, new Color(0.95f, 0.88f, 1f, g));
            }
            else
            {
                float g = 1f - (t - 0.35f) / 0.65f;
                Blit(ArtGen.Glow, hx - 34, hy - 34, 68, 68, new Color(0.72f, 0.55f, 1f, g * 0.5f));
                Blit(ArtGen.Star, hx - 20, hy - 20, 40, 40, new Color(0.92f, 0.85f, 1f, g * 0.7f));
            }
        }

        void DrawWaves()
        {
            foreach (var w in _waves)
            {
                float t = 1f - w.Life / Mathf.Max(0.01f, w.Max);
                float r = Mathf.Lerp(w.R0, w.R1, t * t * (3 - 2 * t));
                float a = (1f - t) * (1f - t);
                float x = w.X - Sim.CamX, y = w.Y - Sim.CamY;
                Blit(ArtGen.Ring, x - r, y - r, r * 2, r * 2, new Color(w.C.r, w.C.g, w.C.b, a * 0.85f));
            }
        }

        void DrawParts()
        {
            foreach (var p in _parts)
            {
                float a = Mathf.Clamp01(p.Life / Mathf.Max(0.01f, p.Max));
                var c = new Color(p.C.r, p.C.g, p.C.b, p.C.a * a);
                float x = p.X - Sim.CamX, y = p.Y - Sim.CamY;
                if (x < -60 || x > T.ViewW + 60 || y < -60 || y > T.ViewH + 60) continue;
                switch (p.Tex)
                {
                    case 1: Blit(ArtGen.Spark, x - p.Size, y - p.Size, p.Size * 2, p.Size * 2, c); break;
                    case 2: Blit(ArtGen.Smoke, x - p.Size, y - p.Size, p.Size * 2, p.Size * 2, c); break;
                    case 3: Spin(ArtGen.Shard, x, y, p.Size, p.Size, p.Rot, c); break;
                    case 4: Blit(ArtGen.Star, x - p.Size, y - p.Size, p.Size * 2, p.Size * 2, c); break;
                    case 5: Blit(ArtGen.Drop, x - p.Size * 0.4f, y - p.Size, p.Size * 0.8f, p.Size * 2, c); break;
                    default: Blit(ArtGen.Glow, x - p.Size, y - p.Size, p.Size * 2, p.Size * 2, c); break;
                }
            }
        }

        // ── hud ──

        static void GuiText(Rect r, string s, int size, Color c, TextAnchor anchor)
        {
            var st = GUI.skin.label;
            var pa = st.alignment; int ps = st.fontSize; var pc = st.normal.textColor;
            st.alignment = anchor; st.fontSize = size;
            st.normal.textColor = new Color(0, 0, 0, c.a * 0.85f);
            GUI.Label(new Rect(r.x + 1, r.y + 1, r.width, r.height), s);
            st.normal.textColor = c;
            GUI.Label(r, s);
            st.alignment = pa; st.fontSize = ps; st.normal.textColor = pc;
        }

        void DrawHud()
        {
            for (int i = 0; i < Sim.Players.Count && i < 2; i++)
            {
                var p = Sim.Players[i];
                float x = i == 0 ? 14 : T.ViewW - 246, y = 12;
                Blit(ArtGen.Px, x, y, 232, 52, new Color(0.03f, 0.01f, 0.03f, 0.62f));
                Blit(ArtGen.Px, x, y, 232, 2, new Color(0.83f, 0.71f, 0.42f, 0.55f));
                Blit(ArtGen.Px, x, y + 50, 232, 2, new Color(0.83f, 0.71f, 0.42f, 0.28f));
                Blit(ArtGen.Disc, x + 8, y + 8, 10, 10, i == 0 ? new Color(1f, 0.85f, 0.45f) : new Color(0.55f, 0.85f, 1f));
                GuiText(new Rect(x + 24, y + 2, 200, 20), (i == 0 ? "P1  " : "P2  ") + p.Hero.Name,
                    13, new Color(0.90f, 0.82f, 0.62f), TextAnchor.MiddleLeft);
                for (int h = 0; h < p.MaxHp && h < 10; h++)
                    Blit(h < p.Hp ? ArtGen.Heart : ArtGen.HeartEmpty, x + 24 + h * 19, y + 24, 17, 17, Color.white);
                for (int l = 0; l < Mathf.Clamp(p.Lives, 0, 5); l++)
                    Blit(ArtGen.Disc, x + 200 - l * 10, y + 30, 7, 7, new Color(0.85f, 0.72f, 0.45f, 0.9f));
                if (p.Dead)
                    GuiText(new Rect(x + 24, y + 24, 200, 20), "CAÍDO", 13, new Color(0.85f, 0.3f, 0.3f), TextAnchor.MiddleLeft);
            }

            // souls / score band
            float cx = T.ViewW / 2f;
            Blit(ArtGen.Px, cx - 150, 12, 300, 26, new Color(0.03f, 0.01f, 0.03f, 0.55f));
            Blit(ArtGen.Soul, cx - 142, 15, 14, 20, Color.white);
            GuiText(new Rect(cx - 122, 12, 120, 26), Sim.Souls.ToString(), 14, new Color(0.83f, 0.74f, 1f), TextAnchor.MiddleLeft);
            GuiText(new Rect(cx - 10, 12, 150, 26), Sim.Score + " pts", 13, new Color(0.86f, 0.78f, 0.58f), TextAnchor.MiddleLeft);
            GuiText(new Rect(cx - 220, 40, 440, 20), (Sim.WorldIndex + 1) + " / 16 · " + Sim.Level.World.Name,
                12, new Color(0.72f, 0.64f, 0.55f), TextAnchor.MiddleCenter);

            // boss bar
            BossA boss = null;
            foreach (var b in Sim.Bosses) if (!b.Dead) boss = b;
            if (boss != null)
            {
                float bw = 520, bx = cx - bw / 2, by = T.ViewH - 62;
                Blit(ArtGen.Px, bx - 3, by - 3, bw + 6, 26, new Color(0.02f, 0.01f, 0.02f, 0.8f));
                Blit(ArtGen.Px, bx, by, bw, 20, new Color(0.14f, 0.03f, 0.05f, 0.95f));
                float f = boss.MaxHp > 0 ? Mathf.Clamp01(boss.Hp / (float)boss.MaxHp) : 0;
                Blit(ArtGen.Px, bx, by, bw * f, 20, new Color(0.80f, 0.16f, 0.20f));
                Blit(ArtGen.Px, bx, by, bw * f, 6, new Color(1f, 0.42f, 0.38f, 0.65f));
                Blit(ArtGen.Px, bx - 3, by - 3, bw + 6, 2, new Color(0.83f, 0.71f, 0.42f, 0.75f));
                GuiText(new Rect(bx, by - 22, bw, 20), (boss.Def?.Name ?? "").ToUpperInvariant(),
                    14, new Color(0.95f, 0.86f, 0.66f), TextAnchor.MiddleCenter);
            }

            if (Sim.MessageT > 0)
            {
                float a = Mathf.Min(1, Sim.MessageT);
                Blit(ArtGen.Px, 0, 96, T.ViewW, 46, new Color(0, 0, 0, 0.42f * a));
                GuiText(new Rect(0, 100, T.ViewW, 40), Sim.Message ?? "", 22, new Color(0.96f, 0.90f, 0.76f, a), TextAnchor.MiddleCenter);
            }
        }

        // ── touch pads ──

        void DrawControls()
        {
            PadCircle(PlayTouch.Left, PlayTouch.Current.L, ArtGen.Tri, 180);
            PadCircle(PlayTouch.Right, PlayTouch.Current.R, ArtGen.Tri, 0);
            PadCircle(PlayTouch.Down, PlayTouch.Current.D, ArtGen.Tri, 90);
            PadCircle(PlayTouch.Jump, PlayTouch.Current.Jn, ArtGen.Tri, 270);
            PadCircle(PlayTouch.Attack, PlayTouch.Current.Ap, ArtGen.Slash, 0);

            var pr = PlayTouch.Pause;
            Blit(ArtGen.Px, pr.x, pr.y, pr.width, pr.height, new Color(0.05f, 0.02f, 0.04f, 0.45f));
            Blit(ArtGen.Px, pr.x + pr.width * 0.36f, pr.y + 12, 5, pr.height - 24, new Color(0.92f, 0.86f, 0.7f, 0.85f));
            Blit(ArtGen.Px, pr.x + pr.width * 0.56f, pr.y + 12, 5, pr.height - 24, new Color(0.92f, 0.86f, 0.7f, 0.85f));
        }

        void PadCircle(Rect r, bool down, Texture2D glyph, float deg)
        {
            float cx = r.x + r.width / 2, cy = r.y + r.height / 2;
            Blit(ArtGen.Disc, r.x, r.y, r.width, r.height, down
                ? new Color(0.66f, 0.24f, 0.28f, 0.55f)
                : new Color(0.06f, 0.03f, 0.05f, 0.34f));
            Blit(ArtGen.Ring, r.x, r.y, r.width, r.height, down
                ? new Color(1f, 0.82f, 0.55f, 0.85f)
                : new Color(0.88f, 0.78f, 0.58f, 0.42f));
            float g = r.width * (glyph == ArtGen.Slash ? 0.62f : 0.36f);
            Spin(glyph, cx, cy, g, g, deg, new Color(0.97f, 0.92f, 0.80f, down ? 1f : 0.72f));
        }
    }
}
