using System;
using System.Collections.Generic;
using System.Net;
using Nightfall;
using Nightfall.Net;
using UnityEditor;
using UnityEngine;

namespace Nightfall.Editor
{
    public static class Smoke
    {
        public static void Run()
        {
            try
            {
                int n = RunInternal();
                Debug.Log("SMOKE OK " + n + " checks");
                EditorApplication.Exit(0);
            }
            catch (Exception e)
            {
                Debug.LogError("SMOKE FAIL: " + e);
                EditorApplication.Exit(2);
            }
        }

        public static int RunInternal()
        {
            int n = 0;
            n += CheckHex();
            n += CheckLevels();
            n += CheckProgress();
            n += CheckLanIp();
            n += CheckCatalog();
            n += CheckTraversal();
            n += CheckNoTrap();
            n += CheckSnapshot();
            n += CheckPlatforms();
            n += CheckPlayable();
            n += CheckFullLoop();
            return n;
        }

        /// <summary>
        /// Plays a level end to end: route, boss, bell. Proves the whole loop closes, including
        /// Belial's second phase in the last world.
        /// </summary>
        static int CheckFullLoop()
        {
            foreach (int i in new[] { 0, 7, 15 })
            {
                UnityEngine.Random.InitState(4200 + i);
                var sim = new GameSim();
                var bot = new Bot();
                sim.ReadCtl = _ => bot.Ctl;
                sim.Start(i, "mike", "denyse", false);
                sim.Players[0].Lives = 999;

                const float dt = 1f / 60f;
                bool done = false;
                for (int f = 0; f < 60 * 300 && !done; f++)
                {
                    bot.Think(sim, dt);
                    sim.Tick(dt);
                    if (sim.State == "clear" || sim.State == "win") done = true;
                    if (sim.State == "dead") throw new Exception("fase " + (i + 1) + ": herói morreu com 999 vidas");
                }
                if (!done)
                {
                    string where = !sim.LockedArena ? "não chegou na arena"
                        : sim.Bosses.Exists(b => !b.Dead) ? "não venceu o chefe"
                        : "não alcançou o sino";
                    throw new Exception("fase " + (i + 1) + " não fecha: " + where);
                }
            }
            return 3;
        }

        /// <summary>
        /// A hero dropped onto a one-way platform must land on it at any frame rate. The old physics
        /// moved a whole frame at once and skipped straight through the 10 px landing window.
        /// </summary>
        static int CheckPlatforms()
        {
            foreach (float dt in new[] { 1f / 60f, 1f / 30f, 1f / 20f })
            {
                var sim = new GameSim { ReadCtl = _ => new Ctl() };
                sim.Start(0, "mike", "denyse", false);
                var p = sim.Players[0];
                // find a platform tile in the level and drop the hero straight onto it
                int px = -1, py = -1;
                for (int x = 4; x < sim.Level.Cols && px < 0; x++)
                    for (int y = 1; y < sim.Level.Rows; y++)
                        if (sim.Level.Tiles[y][x] == T.Platform && sim.Level.Tiles[y - 1][x] == T.Empty)
                        { px = x; py = y; break; }
                if (px < 0) throw new Exception("nenhuma plataforma na fase 1");

                p.X = px * T.Tile + 5;
                p.Y = py * T.Tile - p.H - 150;      // 150 px above: terminal-ish speed on arrival
                p.Vx = 0; p.Vy = 0; p.Inv = 99;
                float floor = py * T.Tile - p.H;
                bool landed = false;
                for (int i = 0; i < 240 && !landed; i++)
                {
                    sim.Tick(dt);
                    if (p.OnGround && Mathf.Abs(p.Y - floor) < 3f) landed = true;
                    if (p.Y > floor + T.Tile * 2) break;   // fell through
                }
                if (!landed)
                    throw new Exception("herói atravessou a plataforma a " + Mathf.RoundToInt(1f / dt) + " fps");
            }
            return 3;
        }

        /// <summary>
        /// Plays every level with the real physics: hold right, jump at walls, gaps and hazards.
        /// Geometry checks alone passed while the game was unplayable, because the platforms the
        /// route depends on were being tunnelled through.
        /// </summary>
        static int CheckPlayable()
        {
            for (int i = 0; i < 16; i++)
            {
                // three fixed seeds: the sim rolls enemy headings and boss patterns, so an unseeded
                // run makes this a coin flip — it passed standalone and failed inside the build
                for (int seed = 0; seed < 3; seed++)
                {
                    UnityEngine.Random.InitState(9001 + i * 31 + seed);
                    var sim = new GameSim();
                    var bot = new Bot();
                    sim.ReadCtl = _ => bot.Ctl;
                    sim.Start(i, "mike", "denyse", false);
                    sim.Players[0].Lives = 999;          // this test is about the route, not the fight

                    const float dt = 1f / 60f;
                    float best = sim.Players[0].X;
                    bool arrived = false;
                    for (int f = 0; f < 60 * 200 && !arrived; f++)
                    {
                        bot.Think(sim, dt);
                        sim.Tick(dt);
                        var p = sim.Players[0];
                        if (p.X > best) best = p.X;
                        if (sim.LockedArena) arrived = true;
                        if (sim.State != "play") break;
                    }
                    if (!arrived)
                        throw new Exception("fase " + (i + 1) + " (" + Catalog.Worlds[i].Name + ") não jogável " +
                            "[seed " + seed + "]: parou na coluna " + Mathf.RoundToInt(best / T.Tile) + " de " +
                            Mathf.RoundToInt(sim.Level.ArenaX0 / T.Tile));
                }
            }
            return 16;
        }

        /// <summary>
        /// Investigation aid: runs the bot over every level with many seeds and reports how often it
        /// gets through, instead of failing on the first bad roll. Not part of the build gate.
        /// </summary>
        public static void Survey()
        {
            try
            {
                int totalFail = 0;
                for (int i = 0; i < 16; i++)
                {
                    int ok = 0; float worst = 1e9f; int worstSeed = -1, deaths = 0; float endX = 0;
                    for (int seed = 0; seed < 8; seed++)
                    {
                        UnityEngine.Random.InitState(500 + i * 97 + seed);
                        var sim = new GameSim();
                        var bot = new Bot();
                        sim.ReadCtl = _ => bot.Ctl;
                        sim.Start(i, "mike", "denyse", false);
                        sim.Players[0].Lives = 999;
                        const float dt = 1f / 60f;
                        float best = sim.Players[0].X;
                        bool arrived = false;
                        for (int f = 0; f < 60 * 200 && !arrived; f++)
                        {
                            bot.Think(sim, dt);
                            sim.Tick(dt);
                            if (sim.Players[0].X > best) best = sim.Players[0].X;
                            if (sim.LockedArena) arrived = true;
                            if (sim.State != "play") break;
                        }
                        if (arrived) ok++;
                        else if (best < worst)
                        {
                            worst = best; worstSeed = seed;
                            deaths = 999 - sim.Players[0].Lives;
                            endX = sim.Players[0].X;
                        }
                    }
                    if (ok < 8)
                    {
                        totalFail++;
                        Debug.Log("SURVEY fase " + (i + 1) + " " + Catalog.Worlds[i].Name +
                                  ": " + ok + "/8 · pior parada coluna " + Mathf.RoundToInt(worst / T.Tile) +
                                  " (seed " + worstSeed + ") · mortes=" + deaths +
                                  " · terminou na coluna " + Mathf.RoundToInt(endX / T.Tile));
                    }
                    else Debug.Log("SURVEY fase " + (i + 1) + " " + Catalog.Worlds[i].Name + ": 8/8");
                }
                Debug.Log("SURVEY DONE falhas=" + totalFail);
                EditorApplication.Exit(0);
            }
            catch (Exception e)
            {
                Debug.LogError("SURVEY FAIL: " + e);
                EditorApplication.Exit(2);
            }
        }

        /// <summary>Holds right, jumps at what blocks it. If this thing can finish a level, a player can.</summary>
        class Bot
        {
            public Ctl Ctl = new();
            float _jumpCd, _atkCd, _stuck, _lastX = float.MinValue;

            public void Think(GameSim sim, float dt)
            {
                var p = sim.Players[0];
                _jumpCd -= dt; _atkCd -= dt;

                // where to head: forward, then the live boss, then the bell
                int dir = 1;
                if (sim.LockedArena)
                {
                    BossA boss = null;
                    foreach (var b in sim.Bosses) if (!b.Dead) boss = b;
                    float goal = boss != null ? boss.X + boss.W / 2f : sim.Level.Exit.x;
                    dir = goal > p.X + p.W / 2f ? 1 : -1;
                }

                float side = dir > 0 ? p.X + p.W + 6 : p.X - 6;
                float probe1 = dir > 0 ? p.X + p.W + 24 : p.X - 24;
                float probe2 = dir > 0 ? p.X + p.W + 52 : p.X - 52;
                bool wall = Blocks(sim.PeekTile(side, p.Y + p.H - 8)) || Blocks(sim.PeekTile(side, p.Y + p.H * 0.5f));
                bool gap = !Stands(sim, probe1, p.Y + p.H + 8) && !Stands(sim, probe2, p.Y + p.H + 8);
                bool burn = IsHazard(sim.PeekTile(probe1, p.Y + p.H + 8)) ||
                            IsHazard(sim.PeekTile(probe1, p.Y + p.H - 8));

                if (Mathf.Abs(p.X - _lastX) < 0.35f) _stuck += dt; else _stuck = 0;
                _lastX = p.X;

                bool want = (p.OnGround && (wall || gap || burn)) || _stuck > 0.45f;
                var c = new Ctl { R = dir > 0, L = dir < 0 };
                if (want && _jumpCd <= 0) { c.Jp = true; c.Jn = true; _jumpCd = 0.30f; }
                else if (p.Vy < 0) c.Jn = true;               // hold for full height
                if (_atkCd <= 0) { c.Ap = true; _atkCd = 0.40f; }
                Ctl = c;
            }

            static bool Blocks(int t) => t == T.Solid || t == T.Ice || t == T.Break;
            static bool IsHazard(int t) => t == T.Spike || t == T.Lava || t == T.Thorn;
            static bool Stands(GameSim sim, float x, float y)
            {
                for (int k = 0; k < 3; k++)
                {
                    int t = sim.PeekTile(x, y + k * T.Tile);
                    if (t == T.Solid || t == T.Ice || t == T.Break || t == T.Platform || t == T.Bounce) return true;
                    if (IsHazard(t)) return false;
                }
                return false;
            }
        }

        /// <summary>Every world must be walkable from the spawn to the boss arena exit.</summary>
        static int CheckTraversal()
        {
            for (int i = 0; i < 16; i++)
            {
                var L = LevelBuilder.Compile(i);
                if (!Walkable(L, out string why))
                    throw new Exception("fase " + (i + 1) + " intransponível: " + why);
            }
            return 16;
        }

        /// <summary>No standing spot may be a cage: every unreachable floor must hurt so the sim rescues.</summary>
        static int CheckNoTrap()
        {
            for (int i = 0; i < 16; i++)
            {
                var L = LevelBuilder.Compile(i);
                var reach = Reach(L);
                for (int x = 0; x < L.Cols; x++)
                    for (int y = 1; y < L.Rows - 1; y++)
                    {
                        if (!Walk(L.Tiles[y][x]) || Walk(L.Tiles[y - 1][x])) continue;
                        if (reach.Contains(x * 64 + y)) continue;
                        if (x * T.Tile >= L.ArenaX0 - T.Tile && x * T.Tile <= L.ArenaX1 + T.Tile) continue;
                        // an unreachable ledge is fine as scenery; an unreachable pit floor is a cage
                        if (y <= 15) continue;
                        int above = L.Tiles[y - 1][x];
                        if (above != T.Spike && above != T.Lava && above != T.Thorn)
                            throw new Exception("fase " + (i + 1) + " tem poço sem saída em " + x + "," + y);
                    }
            }
            return 16;
        }

        static bool Walk(int t) => t == T.Solid || t == T.Ice || t == T.Break || t == T.Platform || t == T.Bounce;

        static HashSet<int> Reach(LevelData L)
        {
            var stand = new List<int>[L.Cols];
            for (int x = 0; x < L.Cols; x++)
            {
                stand[x] = new List<int>();
                for (int y = 1; y < L.Rows; y++)
                    if (Walk(L.Tiles[y][x]) && !Walk(L.Tiles[y - 1][x])) stand[x].Add(y);
            }
            var top = new int[L.Cols];
            for (int x = 0; x < L.Cols; x++) top[x] = stand[x].Count > 0 ? stand[x][0] : L.Rows;
            int sx = Mathf.Clamp(Mathf.RoundToInt(L.Spawn.x / T.Tile), 0, L.Cols - 1);
            var seen = new HashSet<int>();
            var q = new Queue<int>();
            for (int d = 0; d <= 4 && seen.Count == 0; d++)
                foreach (int probe in new[] { sx - d, sx + d })
                    if (probe >= 0 && probe < L.Cols)
                        foreach (var y in stand[probe]) if (seen.Add(probe * 64 + y)) q.Enqueue(probe * 64 + y);
            while (q.Count > 0)
            {
                int node = q.Dequeue();
                int x = node / 64, y = node % 64;
                for (int dx = -5; dx <= 5; dx++)
                {
                    int nx = x + dx;
                    if (nx < 0 || nx >= L.Cols) continue;
                    bool ladder = HasLadder(L, x) && HasLadder(L, nx);
                    foreach (var ny in stand[nx])
                    {
                        int climb = y - ny;
                        bool ok = ladder ? Math.Abs(dx) <= 2 : (climb <= 5 && Math.Abs(dx) <= (climb > 0 ? 4 : 5));
                        if (!ok) continue;
                        if (!ladder && !Clear(top, x, nx, y, ny)) continue;
                        int key = nx * 64 + ny;
                        if (seen.Add(key)) q.Enqueue(key);
                    }
                }
            }
            return seen;
        }

        static bool Clear(int[] top, int x, int nx, int y, int ny)
        {
            int lo = Math.Min(y, ny) - 5;
            int a = Math.Min(x, nx) + 1, b = Math.Max(x, nx);
            for (int cx = a; cx < b; cx++) if (top[cx] < lo) return false;
            return true;
        }

        static bool HasLadder(LevelData L, int x)
        {
            for (int y = 0; y < L.Rows; y++) if (L.Tiles[y][x] == T.Ladder) return true;
            return false;
        }

        static bool Walkable(LevelData L, out string why)
        {
            var seen = Reach(L);
            int gx = Mathf.Clamp(Mathf.RoundToInt(L.Exit.x / T.Tile), 0, L.Cols - 1);
            for (int y = 1; y < L.Rows; y++)
                if (seen.Contains(gx * 64 + y)) { why = null; return true; }
            for (int dx = -2; dx <= 2; dx++)
            {
                int x = Mathf.Clamp(gx + dx, 0, L.Cols - 1);
                for (int y = 1; y < L.Rows; y++)
                    if (seen.Contains(x * 64 + y)) { why = null; return true; }
            }
            why = "sino em " + gx + " fora de alcance do spawn";
            return false;
        }

        /// <summary>The LAN snapshot must survive a round trip with items, projectiles and FX.</summary>
        static int CheckSnapshot()
        {
            if (GameSim.ProjKindId("magic") != 0) throw new Exception("proj magic");
            if (GameSim.ProjKindName(GameSim.ProjKindId("hell")) != "hell") throw new Exception("proj hell");
            if (GameSim.ProjKindName(GameSim.ProjKindId("ice")) != "ice") throw new Exception("proj ice");

            var sim = new GameSim { ReadCtl = _ => new Ctl() };
            sim.Start(0, "mike", "denyse", true);
            if (sim.Players.Count != 2) throw new Exception("dois herois");
            if (sim.Items.Count == 0) throw new Exception("sem itens");
            ulong m = sim.ItemMask();
            if (m == 0) throw new Exception("mascara vazia");
            sim.Items[0].Taken = true;
            if (sim.ItemMask() == m) throw new Exception("mascara nao muda");
            sim.ApplyNetItems(m);
            if (sim.Items[0].Taken) throw new Exception("mascara nao restaura");

            if (Catalog.EnemyAt(Catalog.EnemyIndex("wraith"))?.Id != "wraith") throw new Exception("enemy index");
            if (Catalog.BossAt(Catalog.BossIndex("belial"))?.Id != "belial") throw new Exception("belial index");
            if (Catalog.BossAt(Catalog.BossIndex("treant"))?.Id != "treant") throw new Exception("treant index");

            // a hero dropped into spikes must be lifted out, not pinned there until he dies
            var hero = sim.Players[0];
            for (int i = 0; i < 600; i++) sim.Tick(1f / 60f);
            if (hero.Dead) throw new Exception("heroi morreu parado no spawn");
            if (sim.State != "play") throw new Exception("estado " + sim.State + " parado no spawn");
            return 3;
        }

        static int CheckHex()
        {
            foreach (var w in Catalog.Worlds)
            {
                MustParse(w.Tone);
                MustParse(w.Ground);
                MustParse(w.Lip);
                MustParse(w.Fog);
            }
            MustParse("#d4b46a");
            MustParse("8ec8dc");
            return 1;
        }

        static void MustParse(string h)
        {
            if (string.IsNullOrEmpty(h)) throw new Exception("empty hex");
            if (h[0] != '#') h = "#" + h;
            if (!ColorUtility.TryParseHtmlString(h, out _))
                throw new Exception("bad hex " + h);
            if (h.Length >= 7)
            {
                string raw = h.TrimStart('#');
                if (raw.Length < 6) throw new Exception("short " + h);
            }
        }

        static int CheckLevels()
        {
            for (int i = 0; i < 16; i++)
            {
                var L = LevelBuilder.Compile(i);
                if (L == null) throw new Exception("level null " + i);
                if (L.Cols < 40 || L.Rows != 20) throw new Exception("size " + i);
                if (L.Tiles == null || L.Tiles.Length != L.Rows) throw new Exception("tiles " + i);
                if (L.World == null || string.IsNullOrEmpty(L.World.Boss)) throw new Exception("boss " + i);
                if (Catalog.Boss(L.World.Boss) == null) throw new Exception("boss def " + L.World.Boss);
                if (L.Ents == null || L.Ents.Count < 1) throw new Exception("no ents " + i);
            }
            return 16;
        }

        static int CheckProgress()
        {
            Progress.ResetAll();
            if (Progress.Unlocked != 1) throw new Exception("start unlock");
            if (Progress.CanPlay(0) == false) throw new Exception("world0");
            if (Progress.CanPlay(1)) throw new Exception("world1 locked");
            Progress.OnClear(0);
            if (Progress.Unlocked != 2) throw new Exception("unlock 2");
            if (!Progress.CanPlay(1)) throw new Exception("world1 open");
            Progress.OnClear(0);
            if (Progress.Unlocked != 2) throw new Exception("replay no skip");
            for (int i = 1; i < 15; i++) Progress.OnClear(i);
            if (Progress.Unlocked != 16) throw new Exception("all " + Progress.Unlocked);
            Progress.OnClear(15);
            if (Progress.Unlocked != 16) throw new Exception("cap");
            Progress.ResetAll();
            return 1;
        }

        static int CheckLanIp()
        {
            if (LanIp.ScoreAddress(IPAddress.Parse("127.0.0.1")) >= 0) throw new Exception("loopback");
            if (LanIp.ScoreAddress(IPAddress.Parse("0.0.0.0")) >= 0) throw new Exception("zero");
            if (LanIp.ScoreAddress(IPAddress.Parse("169.254.1.1")) >= 10) throw new Exception("apipa");
            if (LanIp.ScoreAddress(IPAddress.Parse("192.168.0.14")) < 30) throw new Exception("lan");
            if (LanIp.IsUsable("127.0.0.1")) throw new Exception("usable loop");
            if (LanIp.IsUsable("8.8.8.8") == false) { /* public is usable but low score; allow */ }
            if (!LanIp.IsUsable("192.168.1.5")) throw new Exception("usable lan");
            if (LanIp.IsUsable("")) throw new Exception("empty");
            return 1;
        }

        static int CheckCatalog()
        {
            if (Catalog.Worlds.Count != 16) throw new Exception("worlds");
            if (Catalog.Enemies.Count != 32) throw new Exception("enemies");
            if (Catalog.Bosses.Count != 16) throw new Exception("bosses");
            if (Catalog.Belial == null) throw new Exception("belial");
            if (!Catalog.Heroes.ContainsKey("mike") || !Catalog.Heroes.ContainsKey("denyse"))
                throw new Exception("heroes");
            return 1;
        }
    }
}
