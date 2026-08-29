using System;
using System.Collections.Generic;
using Nightfall.Net;
using UnityEngine;

namespace Nightfall
{
    /// <summary>Visual events emitted by the sim and drained by the view (and replicated to the client).</summary>
    public enum FxKind : byte
    {
        Hit = 0, Slash = 1, Death = 2, Pickup = 3, Heal = 4,
        Land = 5, Hurt = 6, Boom = 7, Rescue = 8, Cast = 9, BossDown = 10,
        Charge = 11, Burst = 12
    }

    public struct FxEvent { public byte Kind; public float X, Y; public int Dir; }

    public class Actor
    {
        public float X, Y, W, H, Vx, Vy;
        public int Dir = 1, Facing = 1, Hp, MaxHp;
        public bool OnGround, Dead, Water;
        public float FlipCd, Stun, Flash, T;
        /// <summary>Last swing id from each hero, so one swing lands once per target.</summary>
        public int HitS0 = -1, HitS1 = -1;
    }

    public class PlayerA : Actor
    {
        public int Slot, Lives = 3;
        public HeroDef Hero;
        public float Coyote, JumpBuf, Atk, AtkCd, Inv, HurtT, AnimT;
        public string Anim = "idle";
        public float SpawnX, SpawnY;
        public int Jumps;
        // Last spot where the hero stood on real, non-lethal ground. Hazards send him back here
        // instead of leaving him bouncing inside a spike pit until he runs out of lives.
        public float SafeX, SafeY, SafeT;
        public bool WasAir;
        public int Swing;
        public bool Fired;
        public float Drop;
    }

    public class EnemyA : Actor
    {
        public EnemyDef Def; public string Id;
        public float HomeX, HomeY, ShotT = 1;
        public int Kind;
    }

    public class BossA : Actor
    {
        public BossDef Def; public string Id, State = "idle";
        public float StateT = 1.2f, Intro = 1.6f;
    }

    public class ProjA
    {
        public float X, Y, W, H, Vx, Vy, Life, Age; public int Dmg; public bool Friendly; public string Kind;
    }

    public class ItemA { public string Type; public float X, Y, Bob; public bool Taken; }

    public class Ctl { public bool L, R, D, Jp, Jn, Ap; }

    public class GameSim
    {
        public LevelData Level;
        public readonly List<PlayerA> Players = new();
        public readonly List<EnemyA> Enemies = new();
        public readonly List<BossA> Bosses = new();
        public readonly List<ProjA> Projectiles = new();
        public readonly List<ItemA> Items = new();
        public readonly List<FxEvent> Fx = new();
        readonly List<BossA> _spawnQueue = new();
        public float CamX, CamY, Shake, Time, Fade = 1, FadeDir = -1, MessageT, BossIntro, HitStop;
        public string Message = "", State = "play";
        public int Score, Souls, WorldIndex;
        public bool LockedArena, BelialPhase;
        public Action<string> Sfx;
        public Func<int, Ctl> ReadCtl;

        public void Emit(FxKind kind, float x, float y, int dir = 1)
        {
            if (Fx.Count > 64) return;
            Fx.Add(new FxEvent { Kind = (byte)kind, X = x, Y = y, Dir = dir });
        }

        /// <summary>Tile lookup in world pixels, for tests and tools.</summary>
        public int PeekTile(float px, float py) => Level == null ? T.Empty : TileAt(px, py);

        int TileAt(float px, float py)
        {
            int tx = Mathf.FloorToInt(px / T.Tile), ty = Mathf.FloorToInt(py / T.Tile);
            if (ty >= Level.Rows) return T.Empty;
            if (tx < 0 || ty < 0 || tx >= Level.Cols) return T.Solid;
            return Level.Tiles[ty][tx];
        }
        /// <summary>Set while a hero is deliberately dropping through a one-way platform.</summary>
        bool _noPlat;

        bool SolidAt(float px, float py, bool falling)
        {
            int t = TileAt(px, py);
            if (t == T.Solid || t == T.Ice || t == T.Break) return true;
            if (t == T.Platform && falling && !_noPlat && ModTile(py) < 12) return true;
            if (t == T.Bounce && falling && ModTile(py) < 14) return true;
            return false;
        }
        bool Block(int t) => t == T.Solid || t == T.Ice || t == T.Break;
        static bool Hazard(int t) => t == T.Spike || t == T.Lava || t == T.Thorn;
        static float ModTile(float py)
        {
            float m = py % T.Tile;
            return m < 0 ? m + T.Tile : m;
        }
        bool HazardFloor(int t) => t == T.Empty || t == T.Spike || t == T.Lava || t == T.Water || t == T.Thorn;

        public void Start(int world, string h1, string h2, bool two)
        {
            WorldIndex = world;
            Level = LevelBuilder.Compile(world);
            Enemies.Clear(); Bosses.Clear(); Projectiles.Clear(); Items.Clear(); Players.Clear();
            Fx.Clear(); _spawnQueue.Clear();
            LockedArena = BelialPhase = false; BossIntro = 0; Fade = 1; FadeDir = -1; State = "play";
            Score = 0; Souls = 0; Shake = 0; Time = 0;
            foreach (var e in Level.Ents)
            {
                var d = Catalog.Enemy(e.id); if (d == null) continue;
                var a = new EnemyA
                {
                    Id = e.id, Def = d, Kind = Catalog.EnemyIndex(e.id), X = e.x, Y = e.y, W = d.W, H = d.H,
                    Hp = d.Hp, MaxHp = d.Hp, Dir = UnityEngine.Random.value > 0.5f ? 1 : -1,
                    HomeX = e.x, HomeY = e.y, ShotT = 1 + UnityEngine.Random.value
                };
                a.Facing = a.Dir; Lift(a); a.HomeY = a.Y; Enemies.Add(a);
            }
            foreach (var it in Level.Items) Items.Add(new ItemA { Type = it.type, X = it.x, Y = it.y, Bob = UnityEngine.Random.value * 4 });
            var s = Level.Spawn;
            Players.Add(MakePlayer(0, h1, s.x, s.y));
            if (two) Players.Add(MakePlayer(1, h2, s.x + 40, s.y));
            foreach (var p in Players) { Lift(p); p.SafeX = p.X; p.SafeY = p.Y; }
            CamX = Mathf.Max(0, s.x - T.ViewW / 3f);
            CamY = Mathf.Max(0, s.y - T.ViewH * 0.62f);
            Message = Level.World.Name; MessageT = 2.4f;
        }

        PlayerA MakePlayer(int slot, string hero, float x, float y)
        {
            var h = Catalog.Heroes.ContainsKey(hero ?? "") ? Catalog.Heroes[hero] : Catalog.Heroes["mike"];
            return new PlayerA
            {
                Slot = slot, Hero = h, X = x, Y = y, W = 30, H = 48, Hp = h.Hp, MaxHp = h.Hp,
                SpawnX = x, SpawnY = y, SafeX = x, SafeY = y, Dir = 1, Facing = 1
            };
        }

        public void Tick(float dt)
        {
            Time += dt; Fade = Mathf.Clamp01(Fade + FadeDir * dt * 1.6f); MessageT = Mathf.Max(0, MessageT - dt);
            if (State != "play") return;
            if (Level == null || Players.Count == 0) return;
            // impact bite: a few frames of slow motion so a landed hit has weight
            if (HitStop > 0) { HitStop = Mathf.Max(0, HitStop - dt); dt *= 0.2f; }
            if (BossIntro > 0) BossIntro -= dt;
            foreach (var p in Players) if (!p.Dead) UpdatePlayer(p, dt);
            foreach (var e in Enemies) UpdateEnemy(e, dt);
            foreach (var b in Bosses) UpdateBoss(b, dt);
            FlushSpawns();
            UpdateProj(dt); UpdateItems(dt); MaybeLock(); CheckClear(); UpdateCam(dt);
            FlushSpawns();
            if (State == "play" && Players.TrueForAll(p => p.Dead)) { State = "dead"; Sfx?.Invoke("die"); }
        }

        void FlushSpawns()
        {
            if (_spawnQueue.Count == 0) return;
            Bosses.AddRange(_spawnQueue);
            _spawnQueue.Clear();
        }

        Ctl Inp(int slot) => ReadCtl != null ? (ReadCtl(slot) ?? new Ctl()) : new Ctl();

        void UpdatePlayer(PlayerA p, float dt)
        {
            var ctl = Inp(p.Slot);
            p.Atk = Mathf.Max(0, p.Atk - dt); p.AtkCd = Mathf.Max(0, p.AtkCd - dt);
            p.Inv = Mathf.Max(0, p.Inv - dt); p.HurtT = Mathf.Max(0, p.HurtT - dt); p.AnimT += dt;
            p.Coyote = p.OnGround ? 0.12f : p.Coyote - dt;
            if (ctl.Jp) p.JumpBuf = 0.14f; else p.JumpBuf -= dt;
            int ax = (ctl.L ? -1 : 0) + (ctl.R ? 1 : 0);
            if (ax != 0) p.Facing = ax;
            float spd = p.Hero.Speed * (p.Water ? 0.7f : 1f);
            bool ice = TileAt(p.X + p.W / 2, p.Y + p.H + 1) == T.Ice;
            float acc = p.OnGround ? (ice ? 6f : 18f) : 10f;
            p.Vx += (ax * spd - p.Vx) * Mathf.Min(1, acc * dt);
            if (Level.Zones != null)
            {
                foreach (var z in Level.Zones)
                    if (z.type == "wind" && p.X > z.x0 && p.X < z.x1) p.Vx += z.vx * dt * 3f;
            }
            bool onLadder = TileAt(p.X + p.W / 2, p.Y + p.H / 2) == T.Ladder;
            if (p.JumpBuf > 0 && (p.Coyote > 0 || (p.Jumps < 1 && !p.OnGround) || p.Water || onLadder))
            {
                p.Vy = p.Hero.Jump * (p.Water ? 0.62f : 1f);
                p.OnGround = false; p.Coyote = 0; p.JumpBuf = 0; p.Jumps++;
                Emit(FxKind.Land, p.X + p.W / 2, p.Y + p.H);
                Sfx?.Invoke("jump");
            }
            if (p.OnGround) p.Jumps = 0;
            if (!ctl.Jn && p.Vy < -3) p.Vy += 28 * dt;
            if (onLadder && (ctl.Jn || ctl.D)) { p.Vy = ctl.D ? 3 : -3.4f; p.Jumps = 0; }
            if (p.Hero.Id == "denyse" && !p.OnGround && ctl.Jn && p.Vy > 1) p.Vy = Mathf.Min(p.Vy, 3.2f);
            // hold ▼ on a one-way platform to step down through it
            if (ctl.D && !onLadder && p.OnGround && TileAt(p.X + p.W / 2, p.Y + p.H + 2) == T.Platform)
                p.Drop = 0.16f;
            p.Drop = Mathf.Max(0, p.Drop - dt);

            bool wasAir = !p.OnGround;
            _noPlat = p.Drop > 0;
            Phys(p, dt, p.Hero.Id == "denyse" ? 23 : 26, false);
            _noPlat = false;
            if (wasAir && p.OnGround && p.Vy >= 0) Emit(FxKind.Land, p.X + p.W / 2, p.Y + p.H);
            if (p.OnGround && TileAt(p.X + p.W / 2, p.Y + p.H + 2) == T.Bounce)
            {
                p.Vy = -11.2f;
                p.OnGround = false;
                p.Jumps = 0;
                Emit(FxKind.Land, p.X + p.W / 2, p.Y + p.H);
                Sfx?.Invoke("jump");
            }
            float maxX = Level.Cols * T.Tile - p.W - 4;
            p.X = Mathf.Clamp(p.X, 4, Mathf.Max(4, maxX));
            if (p.Y < -80) { p.Y = -80; p.Vy = Mathf.Max(0, p.Vy); }
            if (p.AtkCd <= 0 && ctl.Ap)
            {
                p.Atk = p.Hero.AtkTime; p.AtkCd = p.Hero.AtkCd;
                p.Swing++; p.Fired = false;
                if (p.Hero.AtkKind == "magic")
                {
                    Emit(FxKind.Charge, p.X + p.W / 2 + p.Facing * 18, p.Y + 18, p.Facing);
                }
                else
                {
                    // the blade carries the body: a short step into the cut, much weaker in the air
                    // so it never turns into a glide (it did, and it threw the hero past platforms)
                    p.Vx += p.Facing * (p.OnGround ? 1.1f : 0.35f);
                    Sfx?.Invoke("attack");
                }
            }
            if (p.Atk > 0)
            {
                float st = 1f - p.Atk / Mathf.Max(0.01f, p.Hero.AtkTime);
                if (p.Hero.AtkKind == "magic")
                {
                    if (!p.Fired && st >= 0.35f) { p.Fired = true; FireMagic(p); }
                }
                else if (st >= 0.12f && st <= 0.88f) MeleeSweep(p, st);
            }
            p.Anim = p.Atk > 0 ? "attack" : !p.OnGround ? (p.Vy < 0 ? "jump" : "fall") : (Mathf.Abs(p.Vx) > 0.4f && ax != 0 ? "walk" : "idle");
            RememberSafe(p, dt);
            int hz = TileAt(p.X + p.W / 2, p.Y + p.H - 4);
            int hzFeet = TileAt(p.X + p.W / 2, p.Y + p.H - 1);
            if (Hazard(hz) || Hazard(hzFeet)) HazardHit(p, (hz == T.Lava || hzFeet == T.Lava) ? 2 : 1);
            if (p.Y > Level.Rows * T.Tile + 20) { HurtDirect(p, 1); Rescue(p); }
            if (Level.Check.HasValue && Mathf.Abs(p.X - Level.Check.Value.x) < 40 && Mathf.Abs(p.Y - Level.Check.Value.y) < 50)
            { p.SpawnX = Level.Check.Value.x; p.SpawnY = Level.Check.Value.y - 10; }
            if (LockedArena) p.X = Mathf.Clamp(p.X, Level.ArenaX0 + 8, Level.ArenaX1 - 8 - p.W);
        }

        /// <summary>Stores the last honest piece of floor the hero stood on, used to bail him out of traps.</summary>
        void RememberSafe(PlayerA p, float dt)
        {
            p.SafeT -= dt;
            if (!p.OnGround || p.SafeT > 0) return;
            int under = TileAt(p.X + p.W / 2, p.Y + p.H + 2);
            if (!(under == T.Solid || under == T.Ice || under == T.Platform || under == T.Break)) return;
            int body = TileAt(p.X + p.W / 2, p.Y + p.H - 4);
            int chest = TileAt(p.X + p.W / 2, p.Y + p.H / 2);
            if (Hazard(body) || Hazard(chest) || body == T.Water) return;
            p.SafeX = p.X; p.SafeY = p.Y - 2; p.SafeT = 0.2f;
        }

        /// <summary>
        /// Spikes / lava / thorns used to only push the hero up 8 units every frame, which pinned him
        /// inside the pit until every life was gone. Now it costs one hit and puts him back on safe ground.
        /// </summary>
        void HazardHit(PlayerA p, int dmg)
        {
            if (p.Inv > 0) return;
            HurtDirect(p, dmg);
            if (p.Dead) return;
            Rescue(p);
        }

        void Rescue(PlayerA p)
        {
            float sx = p.SafeX, sy = p.SafeY;
            if (LockedArena && (sx < Level.ArenaX0 + 24 || sx + p.W > Level.ArenaX1 - 24))
            {
                // the safe spot is outside the sealed arena: drop him at the entrance instead
                sx = Level.ArenaX0 + 60;
                sy = Level.BossAt.y - p.H - 4;
            }
            p.X = sx; p.Y = sy; p.Vx = 0; p.Vy = 0;
            p.OnGround = false; p.Jumps = 0; p.Coyote = 0;
            Lift(p);
            p.Inv = Mathf.Max(p.Inv, 1.35f);
            Emit(FxKind.Rescue, p.X + p.W / 2, p.Y + p.H / 2);
        }

        /// <summary>
        /// Moves an actor in sub-steps of at most <see cref="MaxStep"/> pixels.
        ///
        /// The old version applied the whole frame's displacement at once. A one-way platform only
        /// counts as ground when the feet land inside the top 10 px of its tile, but a normal fall
        /// covers 14 px per frame at 60 fps and 28 px at 30 fps — so the feet jumped clean over the
        /// window and the hero fell straight through platforms he had clearly landed on. That also
        /// made every structure whose only route was a platform cap impossible to climb.
        /// </summary>
        const float MaxStep = 6f;

        void Phys(Actor a, float dt, float grav, bool fly)
        {
            if (!fly) a.Vy += grav * dt;
            a.Vy = Mathf.Clamp(a.Vy, -22f, 16f);

            float dx = a.Vx * 92 * dt;
            float dy = a.Vy * 72 * dt;
            int steps = Mathf.Clamp(Mathf.CeilToInt(Mathf.Max(Mathf.Abs(dx), Mathf.Abs(dy)) / MaxStep), 1, 24);
            float sx = dx / steps, sy = dy / steps;

            a.OnGround = false;
            for (int i = 0; i < steps; i++)
            {
                MoveX(a, sx);
                if (MoveY(a, sy)) sy = 0;
            }
            // resting contact: with sy == 0 the sweep never probes the floor
            if (!a.OnGround && Mathf.Abs(dy) < 0.001f) MoveY(a, 0);

            a.Water = TileAt(a.X + a.W / 2, a.Y + a.H * 0.6f) == T.Water;
            if (a.Water) { a.Vy *= 0.86f; a.Vx *= 0.9f; }
            if (a.OnGround && TileAt(a.X + a.W / 2, a.Y + a.H + 1) == T.Ice && a.Vx != 0) a.Vx *= 0.992f;
        }

        void MoveX(Actor a, float d)
        {
            if (d == 0) return;
            a.X += d;
            float feet = a.Y + a.H - 2, mid = a.Y + a.H * 0.5f, head = a.Y + 6;
            if (d > 0 && (SolidAt(a.X + a.W, feet, false) || SolidAt(a.X + a.W, mid, false) || SolidAt(a.X + a.W, head, false)))
            { a.X = Mathf.Floor((a.X + a.W) / T.Tile) * T.Tile - a.W - 0.01f; a.Vx = 0; }
            else if (d < 0 && (SolidAt(a.X, feet, false) || SolidAt(a.X, mid, false) || SolidAt(a.X, head, false)))
            { a.X = Mathf.Floor(a.X / T.Tile) * T.Tile + T.Tile + 0.01f; a.Vx = 0; }
        }

        /// <summary>One vertical sub-step. Returns true when the actor was stopped.</summary>
        bool MoveY(Actor a, float d)
        {
            a.Y += d;
            if (d >= 0)
            {
                // probe half a pixel under the feet: after a landing snap the feet rest just above
                // the surface, so a flush probe would report mid-air every other sub-step
                float fy = a.Y + a.H + 0.5f;
                bool hit = SolidAt(a.X + 4, fy, true) || SolidAt(a.X + a.W - 4, fy, true) || SolidAt(a.X + a.W / 2, fy, true);
                a.OnGround = hit;
                if (!hit) return false;
                a.Y = Mathf.Floor(fy / T.Tile) * T.Tile - a.H - 0.01f;
                a.Vy = 0;
                return true;
            }
            if (SolidAt(a.X + 4, a.Y + 2, false) || SolidAt(a.X + a.W - 4, a.Y + 2, false))
            {
                a.Y = Mathf.Floor(a.Y / T.Tile) * T.Tile + T.Tile + 0.01f;
                a.Vy = 0;
                return true;
            }
            return false;
        }

        bool WallAhead(Actor e) { float x = e.Dir > 0 ? e.X + e.W + 4 : e.X - 4; return Block(TileAt(x, e.Y + e.H * 0.35f)) || Block(TileAt(x, e.Y + e.H * 0.7f)); }
        bool Ledge(Actor e) => HazardFloor(TileAt(e.Dir > 0 ? e.X + e.W + 6 : e.X - 6, e.Y + e.H + 5));
        bool Bounds(Actor e) => e.X < 20 || e.X + e.W > Level.Cols * T.Tile - 20;
        void Flip(Actor e, float cd) { e.Dir = e.Dir < 0 ? 1 : -1; e.Facing = e.Dir; e.FlipCd = cd; }
        void FaceWalk(Actor e, float dt, float speed)
        {
            e.FlipCd = Mathf.Max(0, e.FlipCd - dt);
            if (e.FlipCd <= 0 && (WallAhead(e) || (e.OnGround && Ledge(e)) || Bounds(e))) Flip(e, 0.36f);
            e.Vx = e.Dir * speed; e.Facing = e.Dir;
        }
        void Lift(Actor e) { for (int i = 0; i < 28; i++) { int t = TileAt(e.X + e.W * 0.5f, e.Y + e.H - 3); if (Block(t) || t == T.Platform || t == T.Bounce) e.Y -= 5; else break; } }

        PlayerA Nearest(float x, float y)
        {
            PlayerA best = null; float bd = 1e9f;
            foreach (var p in Players) if (!p.Dead) { float d = Mathf.Abs(p.X - x) + Mathf.Abs(p.Y - y); if (d < bd) { bd = d; best = p; } }
            return best;
        }

        void CollidePlayers(Actor e, int dmg)
        {
            if (e.Dead) return;
            foreach (var pl in Players)
            {
                if (pl.Dead || !Aabb(e, pl)) continue;
                bool stomp = pl.Vy > 1.2f && pl.Y + pl.H < e.Y + e.H * 0.58f;
                if (stomp) { DmgEnemy(e, 2); pl.Vy = -8; Sfx?.Invoke("stomp"); }
                else HurtFrom(pl, dmg, e.X + e.W / 2);
            }
        }

        static bool Aabb(Actor a, Actor b) => a.X < b.X + b.W && a.X + a.W > b.X && a.Y < b.Y + b.H && a.Y + a.H > b.Y;

        void UpdateEnemy(EnemyA e, float dt)
        {
            if (e.Dead) return;
            e.T += dt; e.Flash = Mathf.Max(0, e.Flash - dt); e.Stun = Mathf.Max(0, e.Stun - dt);
            var def = e.Def; var p = Nearest(e.X, e.Y);
            bool flying = def.Fly || def.Ai == "fly" || def.Ai == "swoop";
            if (e.Stun > 0) { if (!flying) Phys(e, dt, 26, false); CollidePlayers(e, def.Dmg); return; }
            if (flying)
            {
                e.FlipCd = Mathf.Max(0, e.FlipCd - dt);
                if (e.FlipCd <= 0)
                {
                    if (e.X > e.HomeX + 86 || e.X + e.W > Level.Cols * T.Tile - 24) Flip(e, 0.25f);
                    else if (e.X < e.HomeX - 86 || e.X < 24) Flip(e, 0.25f);
                    else if (WallAhead(e)) Flip(e, 0.3f);
                }
                e.X += e.Dir * def.Speed * 68 * dt;
                e.Y = e.HomeY + Mathf.Sin(e.T * 2.5f) * (def.Ai == "swoop" ? 20 : 13);
                if (def.Ai == "swoop" && p != null && Mathf.Abs(p.X - e.X) < 190) e.Y += Mathf.Sign((p.Y + 10) - e.Y) * 32 * dt;
                e.Facing = e.Dir; e.OnGround = false; CollidePlayers(e, def.Dmg); return;
            }
            Lift(e);
            if (def.Ai == "jump") { if (e.OnGround && e.T % 1.55f < dt + 0.02f) e.Vy = -8.6f; FaceWalk(e, dt, def.Speed); }
            else if (def.Ai == "charge")
            {
                if (p != null && Mathf.Abs(p.Y - e.Y) < 58 && Mathf.Abs(p.X - e.X) < 230)
                {
                    int want = p.X + p.W / 2 >= e.X + e.W / 2 ? 1 : -1;
                    e.FlipCd = Mathf.Max(0, e.FlipCd - dt);
                    if (e.FlipCd <= 0 && want != e.Dir) Flip(e, 0.4f);
                    if (WallAhead(e) || (e.OnGround && Ledge(e))) { if (e.FlipCd <= 0) Flip(e, 0.45f); }
                    e.Vx = e.Dir * def.Speed * 1.5f; e.Facing = e.Dir;
                }
                else FaceWalk(e, dt, def.Speed);
            }
            else if (def.Ai == "shoot" || def.Ai == "mage")
            {
                FaceWalk(e, dt, def.Speed * 0.7f);
                e.ShotT -= dt;
                if (e.ShotT <= 0 && p != null && Mathf.Abs(p.X - e.X) < 360)
                {
                    FireShot(e, def.Shot ?? "dark");
                    Emit(FxKind.Cast, e.X + e.W / 2, e.Y + 16);
                    e.ShotT = def.Ai == "mage" ? 1.4f : 1.8f;
                }
                if (p != null) e.Facing = p.X > e.X ? 1 : -1;
            }
            else FaceWalk(e, dt, def.Speed);
            Phys(e, dt, 26, false); CollidePlayers(e, def.Dmg);
        }

        void UpdateBoss(BossA b, float dt)
        {
            if (b.Dead) return;
            b.T += dt; b.Flash = Mathf.Max(0, b.Flash - dt);
            if (b.Intro > 0) { b.Intro -= dt; return; }
            b.StateT -= dt;
            var p = Nearest(b.X, b.Y);
            float enrage = b.Hp < b.MaxHp * 0.4f ? 1.35f : 1f;
            string pat = b.Def.Pattern ?? "";
            if (b.State == "idle")
            {
                if (p != null) { b.Facing = p.X > b.X ? 1 : -1; b.Vx = b.Facing * b.Def.Speed * 0.6f * enrage; }
                if (b.StateT <= 0)
                {
                    var opts = new List<string> { "slam", "shot", "dash" };
                    if (pat == "hex" || pat == "final") opts.Add("cast");
                    if (pat == "summon" || pat == "final" || pat == "multi") opts.Add("summon");
                    if (pat == "flyfire") opts.Add("air");
                    if (pat == "wave" || pat == "lava" || pat == "ice") opts.Add("wave");
                    b.State = opts[UnityEngine.Random.Range(0, opts.Count)];
                    b.StateT = 0.9f;
                }
            }
            else if (b.State == "slam")
            {
                if (b.StateT > 0.55f) b.Vy = -10;
                if (b.StateT < 0.25f && b.OnGround)
                {
                    Shake = 14;
                    Emit(FxKind.Boom, b.X + b.W / 2, b.Y + b.H);
                    foreach (var pl in Players) if (!pl.Dead && Mathf.Abs(pl.X - (b.X + b.W / 2)) < 90) HurtFrom(pl, 1, b.X);
                }
                if (b.StateT <= 0) { b.State = "idle"; b.StateT = 0.8f; }
            }
            else if (b.State == "shot" || b.State == "cast")
            {
                b.Vx = 0;
                string kind = pat == "ice" ? "ice" : (pat == "lava" || pat == "flyfire") ? "hell" : "hex";
                if (b.StateT < 0.7f && Mathf.FloorToInt(b.StateT * 8) != Mathf.FloorToInt((b.StateT + dt) * 8))
                {
                    FireShot(b, kind);
                    Emit(FxKind.Cast, b.X + b.W / 2, b.Y + 20);
                }
                if (b.StateT <= 0) { b.State = "idle"; b.StateT = 0.7f; }
            }
            else if (b.State == "dash")
            {
                b.Vx = b.Facing * b.Def.Speed * 3.4f * enrage;
                if (b.StateT <= 0) { b.State = "idle"; b.StateT = 1; b.Vx = 0; }
            }
            else if (b.State == "summon")
            {
                b.Vx = 0;
                int live = 0;
                foreach (var e in Enemies) if (!e.Dead) live++;
                if (b.StateT < 0.85f && live < 6 && Level.World.Enemies != null && Level.World.Enemies.Length > 0)
                {
                    string eid = Level.World.Enemies[UnityEngine.Random.Range(0, Level.World.Enemies.Length)];
                    var d = Catalog.Enemy(eid);
                    if (d != null && Enemies.Count < 40)
                    {
                        var a = new EnemyA
                        {
                            Id = eid, Def = d, Kind = Catalog.EnemyIndex(eid),
                            X = b.X + UnityEngine.Random.Range(-20, 60), Y = b.Y, W = d.W, H = d.H,
                            Hp = d.Hp, MaxHp = d.Hp, Dir = 1, HomeX = b.X, HomeY = b.Y, ShotT = 1
                        };
                        a.Facing = a.Dir; Lift(a); Enemies.Add(a);
                        Emit(FxKind.Cast, a.X + a.W / 2, a.Y + a.H / 2);
                    }
                    b.State = "idle"; b.StateT = 1.4f;
                }
                if (b.StateT <= 0) { b.State = "idle"; b.StateT = 1; }
            }
            else if (b.State == "air")
            {
                b.Vy = -4;
                b.Y -= 20 * dt;
                if (Mathf.FloorToInt(b.T * 4) != Mathf.FloorToInt((b.T - dt) * 4)) FireShot(b, "hell");
                if (b.StateT <= 0) { b.State = "idle"; b.StateT = 0.8f; }
            }
            else if (b.State == "wave")
            {
                b.Vx = 0;
                if (Mathf.FloorToInt(b.StateT * 3) != Mathf.FloorToInt((b.StateT + dt) * 3))
                {
                    int dir = UnityEngine.Random.value > 0.5f ? 1 : -1;
                    string kind = pat == "lava" ? "hell" : pat == "ice" ? "ice" : "dark";
                    Projectiles.Add(new ProjA { Kind = kind, X = b.X + b.W / 2, Y = b.Y + b.H - 18, W = 20, H = 16, Vx = dir * 5.5f, Life = 2.2f, Dmg = 1 });
                }
                if (b.StateT <= 0) { b.State = "idle"; b.StateT = 0.9f; }
            }
            Phys(b, dt, 22, false);
            if (b.X < Level.ArenaX0 + 10) { b.X = Level.ArenaX0 + 10; b.Facing = 1; }
            if (b.X + b.W > Level.ArenaX1 - 10) { b.X = Level.ArenaX1 - 10 - b.W; b.Facing = -1; }
            CollidePlayers(b, 1);
        }

        void FireMagic(PlayerA p)
        {
            Projectiles.Add(new ProjA
            {
                Kind = "magic", X = p.X + (p.Facing > 0 ? p.W : -16), Y = p.Y + 14,
                W = 18, H = 12, Vx = p.Facing * 14, Life = 0.9f, Dmg = p.Hero.Damage, Friendly = true
            });
            p.Vx -= p.Facing * 0.9f;                       // recoil
            Emit(FxKind.Cast, p.X + p.W / 2 + p.Facing * 22, p.Y + 18, p.Facing);
            Shake = Mathf.Max(Shake, 3);
            Sfx?.Invoke("magic");
        }
        void FireShot(Actor e, string kind)
        {
            var p = Nearest(e.X, e.Y); if (p == null) return;
            float dx = p.X + p.W / 2 - (e.X + e.W / 2), dy = p.Y + p.H / 2 - (e.Y + 16);
            float len = Mathf.Max(0.01f, Mathf.Sqrt(dx * dx + dy * dy));
            Projectiles.Add(new ProjA { Kind = kind, X = e.X + e.W / 2, Y = e.Y + 16, W = 12, H = 12, Vx = dx / len * 4.2f, Vy = dy / len * 4.2f, Life = 2.4f, Dmg = 1 });
        }
        /// <summary>
        /// The blade is a real arc that sweeps from overhead to low across the swing, not a single
        /// instant box on the frame the button went down. Each swing lands once per target, and a
        /// clean hit pushes the target back and bites into time for a few frames.
        /// </summary>
        void MeleeSweep(PlayerA p, float st)
        {
            float cx = p.X + p.W / 2, cy = p.Y + p.H * 0.46f;
            float reach = p.Hero.Reach;
            float sweep = Mathf.Lerp(-52f, 44f, Mathf.Clamp01((st - 0.12f) / 0.76f));
            foreach (var e in Enemies) if (!e.Dead) TrySwing(p, e, cx, cy, reach, sweep);
            foreach (var b in Bosses) if (!b.Dead) TrySwing(p, b, cx, cy, reach, sweep);
        }

        void TrySwing(PlayerA p, Actor e, float cx, float cy, float reach, float sweep)
        {
            if ((p.Slot == 0 ? e.HitS0 : e.HitS1) == p.Swing) return;
            // nearest point of the target box, so big enemies are not judged by their centre
            float nx = Mathf.Clamp(cx, e.X, e.X + e.W);
            float ny = Mathf.Clamp(cy, e.Y, e.Y + e.H);
            float dx = (nx - cx) * p.Facing, dy = ny - cy;
            float d = Mathf.Sqrt(dx * dx + dy * dy);
            if (d > reach || dx < -10) return;
            float ang = d < 1f ? sweep : Mathf.Atan2(dy, Mathf.Max(0.001f, dx)) * Mathf.Rad2Deg;
            if (Mathf.Abs(Mathf.DeltaAngle(ang, sweep)) > 52f) return;

            if (p.Slot == 0) e.HitS0 = p.Swing; else e.HitS1 = p.Swing;
            DmgEnemy(e, p.Hero.Damage, p.Facing, e is BossA ? 1.1f : 4.4f);
            Shake = Mathf.Max(Shake, 6);
            HitStop = Mathf.Max(HitStop, 0.055f);
            Emit(FxKind.Slash, nx, ny, p.Facing);
        }

        void UpdateProj(float dt)
        {
            for (int i = Projectiles.Count - 1; i >= 0; i--)
            {
                var pr = Projectiles[i];
                pr.Life -= dt; pr.Age += dt;
                pr.X += pr.Vx * 110 * dt; pr.Y += pr.Vy * 110 * dt;
                int t = TileAt(pr.X + pr.W / 2, pr.Y + pr.H / 2);
                if (t == T.Solid || t == T.Ice || t == T.Break) { pr.Life = 0; Emit(FxKind.Hit, pr.X, pr.Y); }
                var box = new Actor { X = pr.X, Y = pr.Y, W = pr.W, H = pr.H };
                if (pr.Friendly)
                {
                    float dir = pr.Vx >= 0 ? 1 : -1;
                    foreach (var e in Enemies)
                        if (!e.Dead && Aabb(box, e))
                        {
                            DmgEnemy(e, pr.Dmg, dir, 3.0f);
                            Emit(FxKind.Burst, pr.X + pr.W / 2, pr.Y + pr.H / 2, (int)dir);
                            HitStop = Mathf.Max(HitStop, 0.035f);
                            pr.Life = 0;
                        }
                    foreach (var b in Bosses)
                        if (!b.Dead && Aabb(box, b))
                        {
                            DmgEnemy(b, pr.Dmg, dir, 0.8f);
                            Emit(FxKind.Burst, pr.X + pr.W / 2, pr.Y + pr.H / 2, (int)dir);
                            pr.Life = 0;
                        }
                }
                else foreach (var p in Players) if (!p.Dead && Aabb(box, p)) { HurtFrom(p, pr.Dmg, pr.X); pr.Life = 0; }
                if (pr.Life <= 0) Projectiles.RemoveAt(i);
            }
        }

        void UpdateItems(float dt)
        {
            foreach (var it in Items)
            {
                if (it.Taken) continue;
                it.Bob += dt;
                var box = new Actor { X = it.X, Y = it.Y + Mathf.Sin(it.Bob * 3) * 3, W = 20, H = 20 };
                foreach (var p in Players)
                {
                    if (p.Dead || !Aabb(box, p)) continue;
                    if (it.Type == "soul") { Souls++; Score += 25; Emit(FxKind.Pickup, it.X + 10, it.Y + 10); Sfx?.Invoke("coin"); }
                    else if (it.Type == "heart") { p.Hp = Mathf.Min(p.MaxHp, p.Hp + 2); Emit(FxKind.Heal, it.X + 10, it.Y + 10); Sfx?.Invoke("heart"); }
                    else { Emit(FxKind.Pickup, it.X + 10, it.Y + 10); }
                    it.Taken = true;
                    break;
                }
            }
        }

        void HurtFrom(PlayerA p, int dmg, float srcX)
        {
            if (p.Dead || p.Inv > 0) return;
            p.Vx = (p.X + p.W / 2 < srcX ? -1 : 1) * 3.2f; p.Vy = -6;
            HurtDirect(p, dmg);
        }

        void HurtDirect(PlayerA p, int dmg)
        {
            if (p.Dead || p.Inv > 0) return;
            p.Hp -= dmg; p.Inv = 1.15f; p.HurtT = 0.25f; Shake = 8;
            Emit(FxKind.Hurt, p.X + p.W / 2, p.Y + p.H / 2);
            Sfx?.Invoke("hurt");
            if (p.Hp > 0) return;
            p.Hp = 0; p.Lives--;
            if (p.Lives < 0) { p.Dead = true; Emit(FxKind.Death, p.X + p.W / 2, p.Y + p.H / 2); Sfx?.Invoke("die"); }
            else
            {
                p.Hp = p.MaxHp;
                p.X = p.SpawnX; p.Y = p.SpawnY; p.SafeX = p.SpawnX; p.SafeY = p.SpawnY;
                p.Vx = p.Vy = 0; p.Inv = 2;
                Lift(p);
                Emit(FxKind.Rescue, p.X + p.W / 2, p.Y + p.H / 2);
            }
        }

        void DmgEnemy(Actor e, int dmg, float pushDir = 0, float push = 0)
        {
            if (e.Dead) return;
            e.Hp -= dmg; e.Flash = 0.14f; e.Stun = Mathf.Max(e.Stun, push > 2 ? 0.2f : 0.12f);
            if (push != 0)
            {
                e.Vx = pushDir * push;
                if (e.OnGround && push > 2) e.Vy = -3.2f;
            }
            Emit(FxKind.Hit, e.X + e.W / 2, e.Y + e.H / 2);
            Sfx?.Invoke("hit");
            if (e.Hp > 0) return;
            if (e is BossA b)
            {
                b.Dead = true; Score += b.Def.Score; Souls += 15;
                Message = b.Def.Name + " caiu! Sigam o sino dourado."; MessageT = 3f;
                Shake = 18;
                Emit(FxKind.BossDown, b.X + b.W / 2, b.Y + b.H / 2);
                Sfx?.Invoke("clear");
                if (WorldIndex == 15 && b.Id != "belial" && !BelialPhase)
                {
                    BelialPhase = true; BossIntro = 2.2f; Message = "Belial desperta do trono!"; MessageT = 2.6f;
                    var def = Catalog.Belial;
                    // queued: Bosses may be mid-iteration in Melee / UpdateProj / UpdateBoss
                    _spawnQueue.Add(new BossA
                    {
                        Id = def.Id, Def = def,
                        X = (Level.ArenaX0 + Level.ArenaX1) / 2 - def.W / 2f, Y = Level.BossAt.y - def.H,
                        W = def.W, H = def.H, Hp = def.Hp, MaxHp = def.Hp, Facing = -1
                    });
                }
                else if (b.Id == "belial") State = "win";
            }
            else if (e is EnemyA en)
            {
                en.Dead = true; Score += en.Def.Score; Souls++;
                Emit(FxKind.Death, en.X + en.W / 2, en.Y + en.H / 2);
                Sfx?.Invoke("stomp");
            }
        }

        void MaybeLock()
        {
            if (LockedArena) return;
            if (Level == null) return;
            if (!Players.Exists(p => !p.Dead && p.X > Level.ArenaX0 + 40)) return;
            var def = Catalog.Boss(Level.World.Boss);
            if (def == null) return;
            LockedArena = true; BossIntro = 1.8f;
            _spawnQueue.Add(new BossA
            {
                Id = def.Id, Def = def, X = Level.BossAt.x - def.W / 2f, Y = Level.BossAt.y - def.H,
                W = def.W, H = def.H, Hp = def.Hp, MaxHp = def.Hp, Facing = -1
            });
            Message = def.Name.ToUpperInvariant(); MessageT = 2.2f; Sfx?.Invoke("boss");
        }

        public void EnsureSecond(string hero)
        {
            if (Players.Count >= 2 || Level == null) return;
            if (string.IsNullOrEmpty(hero) || !Catalog.Heroes.ContainsKey(hero)) hero = "denyse";
            var s = Players.Count > 0 ? Players[0] : null;
            float x = s != null ? s.X + 40 : Level.Spawn.x;
            float y = s != null ? s.Y : Level.Spawn.y;
            var p = MakePlayer(1, hero, x, y);
            Lift(p);
            Players.Add(p);
        }

        /// <summary>Client-side tick: no physics, just presentation so the mirrored world stays alive.</summary>
        public void TickRemote(float dt)
        {
            Time += dt;
            Fade = Mathf.Clamp01(Fade + FadeDir * dt * 1.6f);
            MessageT = Mathf.Max(0, MessageT - dt);
            Shake *= 0.88f;
            foreach (var it in Items) if (!it.Taken) it.Bob += dt;
            foreach (var p in Players)
            {
                p.AnimT += dt;
                p.Atk = Mathf.Max(0, p.Atk - dt);
                p.Inv = Mathf.Max(0, p.Inv - dt);
                p.HurtT = Mathf.Max(0, p.HurtT - dt);
            }
            foreach (var e in Enemies) { e.T += dt; e.Flash = Mathf.Max(0, e.Flash - dt); }
            foreach (var b in Bosses) { b.T += dt; b.Flash = Mathf.Max(0, b.Flash - dt); }
            foreach (var pr in Projectiles) { pr.Age += dt; pr.X += pr.Vx * 110 * dt; pr.Y += pr.Vy * 110 * dt; }
            UpdateCam(dt);
        }

        /// <summary>Client: replace the enemy roster with the host's, spawning late-summoned mobs.</summary>
        public void ApplyNetEnemies(EntSnap[] ents)
        {
            if (ents == null) return;
            for (int i = 0; i < ents.Length; i++)
            {
                var s = ents[i];
                while (Enemies.Count <= i)
                {
                    var kd = Catalog.EnemyAt(s.Kind);
                    if (kd == null) return;
                    Enemies.Add(new EnemyA { Id = kd.Id, Def = kd, Kind = s.Kind, X = s.X, Y = s.Y, W = kd.W, H = kd.H, Hp = kd.Hp, MaxHp = kd.Hp });
                }
                var e = Enemies[i];
                if (e.Kind != s.Kind)
                {
                    var kd = Catalog.EnemyAt(s.Kind);
                    if (kd != null) { e.Id = kd.Id; e.Def = kd; e.Kind = s.Kind; e.W = kd.W; e.H = kd.H; e.MaxHp = kd.Hp; }
                }
                float d = Mathf.Abs(e.X - s.X) + Mathf.Abs(e.Y - s.Y);
                if (d > 220) { e.X = s.X; e.Y = s.Y; }
                else { e.X = Mathf.Lerp(e.X, s.X, 0.5f); e.Y = Mathf.Lerp(e.Y, s.Y, 0.5f); }
                e.Facing = (s.Flags & 4) != 0 ? -1 : 1;
                e.Dir = e.Facing;
                e.Hp = s.Hp;
                bool wasDead = e.Dead;
                e.Dead = (s.Flags & 1) != 0;
                if (!wasDead && e.Dead) Emit(FxKind.Death, e.X + e.W / 2, e.Y + e.H / 2);
                if ((s.Flags & 2) != 0) e.Flash = 0.14f;
            }
        }

        /// <summary>Client: rebuild the live projectile list from the host snapshot.</summary>
        public void ApplyNetProjectiles(ProjSnap[] ps)
        {
            Projectiles.Clear();
            if (ps == null) return;
            foreach (var s in ps)
                Projectiles.Add(new ProjA
                {
                    Kind = ProjKindName(s.Kind), X = s.X, Y = s.Y,
                    W = s.Kind == 0 ? 18 : 12, H = s.Kind == 0 ? 12 : 12,
                    Vx = s.Vx, Vy = s.Vy, Friendly = (s.Flags & 1) != 0, Life = 1
                });
        }

        public static byte ProjKindId(string kind) => kind switch
        {
            "magic" => 0, "ice" => 1, "hell" => 2, "hex" => 3, "bone" => 4, "holy" => 5, _ => 6
        };
        public static string ProjKindName(byte id) => id switch
        {
            0 => "magic", 1 => "ice", 2 => "hell", 3 => "hex", 4 => "bone", 5 => "holy", _ => "dark"
        };

        /// <summary>Client: apply which items are still on the ground (bit set = still there).</summary>
        public void ApplyNetItems(ulong mask)
        {
            int n = Mathf.Min(64, Items.Count);
            for (int i = 0; i < n; i++)
            {
                bool alive = (mask & (1UL << i)) != 0;
                if (!Items[i].Taken && !alive) Emit(FxKind.Pickup, Items[i].X + 10, Items[i].Y + 10);
                Items[i].Taken = !alive;
            }
        }

        public ulong ItemMask()
        {
            ulong m = 0;
            int n = Mathf.Min(64, Items.Count);
            for (int i = 0; i < n; i++) if (!Items[i].Taken) m |= 1UL << i;
            return m;
        }

        void CheckClear()
        {
            if (State == "win") return;
            if (!LockedArena || Bosses.Count == 0 || Bosses.Exists(b => !b.Dead)) return;
            if (WorldIndex == 15 && (!BelialPhase || Bosses.Exists(b => b.Id == "belial" && !b.Dead))) return;
            foreach (var p in Players) if (!p.Dead && Mathf.Abs(p.X - Level.Exit.x) < 46 && Mathf.Abs(p.Y - Level.Exit.y) < 60) { State = "clear"; return; }
        }

        void UpdateCam(float dt)
        {
            if (Level == null) return;
            var live = Players.FindAll(p => !p.Dead); if (live.Count == 0) return;
            float cx = 0, cy = 0; foreach (var p in live) { cx += p.X + p.W / 2; cy += p.Y + p.H / 2; }
            cx /= live.Count; cy /= live.Count;
            CamX = Mathf.Lerp(CamX, cx - T.ViewW * 0.4f, Mathf.Min(1, 4 * dt));
            CamY = Mathf.Lerp(CamY, cy - T.ViewH * 0.62f, Mathf.Min(1, 3.2f * dt));
            CamX = Mathf.Clamp(CamX, 0, Mathf.Max(0, Level.Cols * T.Tile - T.ViewW));
            CamY = Mathf.Clamp(CamY, 0, Mathf.Max(0, Level.Rows * T.Tile - T.ViewH));
            Shake *= 0.88f;
        }
    }
}
