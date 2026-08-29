using System;
using System.Collections.Generic;
using UnityEngine;

namespace Nightfall
{
    public class LevelData
    {
        public int WorldIndex, Cols, Rows;
        public WorldDef World;
        public byte[][] Tiles;
        public Vector2 Spawn, Exit;
        public Vector2? Check;
        public Vector2 BossAt;
        public float ArenaX0, ArenaX1;
        public List<(string id, float x, float y)> Ents = new();
        public List<(string type, float x, float y)> Items = new();
        public List<(string type, float x0, float x1, float vx)> Zones = new();
        public List<(string kind, float x, float y, float scale)> Decor = new();
        /// <summary>Surfaces the player can never climb out of; the builder seals them with hazard.</summary>
        public int SealedPockets;
    }

    public static class LevelBuilder
    {
        // how far a hero can climb in one jump (tiles) and how far across
        const int JumpUp = 5, Reach = 4;

        class Ctx
        {
            public WorldDef World; public int Cols, Rows, Cursor, Base;
            public byte[][] Grid;
            public List<(string, float, float)> Ents = new();
            public List<(string, float, float)> Items = new();
            public List<(string type, float x0, float x1, float vx)> Zones = new();
            public List<(string kind, float x, float y, float scale)> Decor = new();
            public Vector2 Spawn, Exit, BossAt, Check;
            public bool HasCheck;
            public float ArenaX0, ArenaX1;
            public int Sealed;
        }

        static bool In(Ctx c, int x, int y) => x >= 0 && y >= 0 && x < c.Cols && y < c.Rows;
        static void Set(Ctx c, int x, int y, int t) { if (In(c, x, y)) c.Grid[y][x] = (byte)t; }
        static int Get(Ctx c, int x, int y) => In(c, x, y) ? c.Grid[y][x] : T.Solid;
        static void FillDown(Ctx c, int x, int top, int t) { for (int y = top; y < c.Rows; y++) Set(c, x, y, t); }
        static void Rect(Ctx c, int x, int y, int w, int h, int t) { for (int iy = 0; iy < h; iy++) for (int ix = 0; ix < w; ix++) Set(c, x + ix, y + iy, t); }
        static void Plat(Ctx c, int x, int y, int w, int t = T.Platform) { for (int i = 0; i < w; i++) Set(c, x + i, y, t); }
        static void Solid(Ctx c, int x, int y, int w, int h) => Rect(c, x, y, w, h, T.Solid);
        static void Ground(Ctx c, int x0, int x1, Func<int, int> h, int t)
        {
            for (int x = x0; x < x1 && x < c.Cols; x++) FillDown(c, x, h(x), t);
        }
        static void AddE(Ctx c, string id, int tx, int ty)
        {
            var d = Catalog.Enemy(id); if (d == null) return;
            c.Ents.Add((id, tx * T.Tile + (T.Tile - d.W) / 2f, ty * T.Tile - d.H));
        }
        static void AddI(Ctx c, string type, int tx, int ty) => c.Items.Add((type, tx * T.Tile + 10, ty * T.Tile + 8));
        static int Hz(WorldDef w) => w.Hazard == "lava" ? T.Lava : w.Hazard == "water" ? T.Water : w.Hazard == "ice" ? T.Ice : w.Hazard == "thorn" ? T.Thorn : T.Spike;
        /// <summary>Hazard that actually costs a hit — ice and water are terrain, not traps.</summary>
        static int Sting(WorldDef w) => w.Hazard == "lava" ? T.Lava : w.Hazard == "thorn" ? T.Thorn : T.Spike;

        public static LevelData Compile(int worldIndex)
        {
            var world = Catalog.Worlds[worldIndex];
            var c = new Ctx { World = world, Rows = 20, Cols = world.Cols + 40, Cursor = 2, Base = 15 };
            c.Grid = new byte[c.Rows][];
            for (int y = 0; y < c.Rows; y++) c.Grid[y] = new byte[c.Cols];
            Ground(c, 0, 2, _ => c.Base, T.Solid);
            foreach (var beat in world.Beats) RunBeat(c, beat);
            StampArena(c);
            for (int x = 0; x < c.Cols; x++) if (c.Grid[c.Rows - 1][x] == 0) c.Grid[c.Rows - 1][x] = T.Solid;
            SealUnreachable(c);
            ScatterDecor(c, worldIndex);
            return new LevelData
            {
                WorldIndex = worldIndex, World = world, Cols = c.Cols, Rows = c.Rows, Tiles = c.Grid,
                Spawn = c.Spawn.sqrMagnitude > 0 ? c.Spawn : new Vector2(80, 200),
                Check = c.HasCheck ? c.Check : null,
                Exit = c.Exit, BossAt = c.BossAt, ArenaX0 = c.ArenaX0, ArenaX1 = c.ArenaX1,
                Ents = c.Ents, Items = c.Items, Zones = c.Zones, Decor = c.Decor,
                SealedPockets = c.Sealed
            };
        }

        static void RunBeat(Ctx c, string beat)
        {
            int x = c.Cursor, b = c.Base;
            switch (beat)
            {
                case "intro":
                    Ground(c, x, x + 16, _ => b, T.Solid);
                    if (c.Spawn.sqrMagnitude == 0) c.Spawn = new Vector2((x + 3) * T.Tile, (b - 2) * T.Tile);
                    Plat(c, x + 9, b - 3, 3);
                    AddI(c, "soul", x + 8, b - 1); AddI(c, "soul", x + 10, b - 4);
                    AddE(c, c.World.Enemies[0], x + 13, b); c.Cursor += 16; break;

                case "hills":
                    Ground(c, x, x + 24, xx => Mathf.RoundToInt(b - 1 - Mathf.Sin((xx - x) * 0.38f) * 2 - Mathf.Sin((xx - x) * 0.11f)), T.Solid);
                    AddE(c, c.World.Enemies[0], x + 8, b - 2); AddE(c, c.World.Enemies[1], x + 16, b - 2);
                    AddI(c, "soul", x + 11, b - 5); AddI(c, "soul", x + 20, b - 5); c.Cursor += 24; break;

                case "pit":
                    // A fall costs one hit, never the run: the basin floor is only three tiles down,
                    // its rim is safe on both ends and the hazard band sits in the middle.
                    Ground(c, x, x + 4, _ => b, T.Solid);
                    int hz = Hz(c.World);
                    int floor = b + 4;
                    for (int i = x + 4; i < x + 16; i++) FillDown(c, i, floor, T.Solid);
                    for (int i = x + 6; i < x + 14; i++)
                    {
                        Set(c, i, floor - 1, hz == T.Ice ? T.Spike : hz);
                        if (hz == T.Water || hz == T.Lava) Set(c, i, floor - 2, hz);
                    }
                    Plat(c, x + 5, b - 1, 3); Plat(c, x + 9, b - 3, 3); Plat(c, x + 13, b - 1, 3);
                    if (!string.IsNullOrEmpty(c.World.Flyer)) AddE(c, c.World.Flyer, x + 10, b - 6);
                    AddI(c, "soul", x + 10, b - 5);
                    Ground(c, x + 16, x + 20, _ => b, T.Solid); c.Cursor += 20; break;

                case "trees":
                    Ground(c, x, x + 22, _ => b, T.Solid);
                    foreach (int px in new[] { x + 4, x + 10, x + 16 })
                    {
                        // four tiles tall: a hero clears five, so the canopy is a hop, not a wall
                        Solid(c, px, b - 4, 1, 4); Plat(c, px - 2, b - 4, 5); AddI(c, "soul", px, b - 5);
                        c.Decor.Add(("canopy", px * T.Tile, (b - 4) * T.Tile, 1.2f));
                    }
                    AddE(c, c.World.Enemies[0], x + 7, b); AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 13, b - 8); c.Cursor += 22; break;

                case "canopy":
                    Ground(c, x, x + 3, _ => b, T.Solid);
                    for (int i = 0; i < 6; i++)
                    {
                        Plat(c, x + 3 + i * 3, b - 2 - (i % 3), 3);
                        if (i % 2 == 0) AddI(c, "soul", x + 4 + i * 3, b - 3 - (i % 3));
                    }
                    // a floor under the gaps so a missed hop is a scratch, not a void
                    for (int i = x + 3; i < x + 21; i++) { FillDown(c, i, b + 4, T.Solid); Set(c, i, b + 3, Sting(c.World)); }
                    AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 10, b - 6);
                    Ground(c, x + 21, x + 24, _ => b, T.Solid); c.Cursor += 24; break;

                case "rooms":
                    // the side walls used to reach the floor and seal the room shut; they now hang
                    Ground(c, x, x + 26, _ => b, T.Solid); Solid(c, x, b - 8, 1, 6); Solid(c, x + 25, b - 8, 1, 6);
                    Plat(c, x + 1, b - 8, 24); Plat(c, x + 4, b - 4, 6); Plat(c, x + 14, b - 4, 6);
                    for (int y = b - 7; y < b; y++) Set(c, x + 12, y, T.Ladder);
                    AddE(c, c.World.Enemies[1], x + 6, b - 4); AddE(c, c.World.Enemies[0], x + 18, b);
                    AddI(c, "heart", x + 20, b - 5); AddI(c, "soul", x + 6, b - 9); c.Cursor += 26; break;

                case "stairs":
                    Ground(c, x, x + 18, _ => b, T.Solid);
                    for (int i = 0; i < 6; i++) Solid(c, x + 3 + i * 2, b - 1 - i, 2, 1 + i);
                    AddE(c, c.World.Enemies[0], x + 14, b - 7); AddI(c, "soul", x + 15, b - 8); c.Cursor += 18; break;

                case "attic":
                    Ground(c, x, x + 20, _ => b, T.Solid); Plat(c, x + 2, b - 4, 16); Plat(c, x + 1, b - 8, 18);
                    for (int y = b - 8; y < b; y++) Set(c, x + 3, y, T.Ladder);
                    AddE(c, c.World.Enemies[1], x + 10, b - 5); AddE(c, c.World.Enemies[0], x + 14, b);
                    AddI(c, "soul", x + 15, b - 10); c.Cursor += 20; break;

                case "water":
                    int fl = c.World.Hazard == "lava" ? T.Lava : T.Water;
                    Ground(c, x, x + 4, _ => b, T.Solid);
                    for (int i = x + 4; i < x + 18; i++) { FillDown(c, i, b + 3, T.Solid); for (int y = b; y < b + 3; y++) Set(c, i, y, fl); }
                    Plat(c, x + 6, b - 1, 3); Plat(c, x + 11, b - 2, 3);
                    AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 10, b - 5);
                    AddI(c, "soul", x + 12, b - 4);
                    Ground(c, x + 18, x + 22, _ => b, T.Solid); c.Cursor += 22; break;

                case "isles":
                    Ground(c, x, x + 3, _ => b, T.Solid);
                    int[] ys = { b - 1, b - 3, b - 2, b - 5, b - 3 };
                    for (int i = 0; i < 5; i++) { Plat(c, x + 4 + i * 4, ys[i], 3); if (i % 2 == 1) AddI(c, "soul", x + 5 + i * 4, ys[i] - 1); }
                    for (int i = x + 3; i < x + 24; i++) { FillDown(c, i, b + 4, T.Solid); Set(c, i, b + 3, Sting(c.World)); }
                    AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 16, b - 6);
                    Ground(c, x + 24, x + 28, _ => b, T.Solid); c.Cursor += 28; break;

                case "dunes":
                    Ground(c, x, x + 28, xx => b - Mathf.RoundToInt(Mathf.Sin((xx - x) * 0.22f) * 2.4f), T.Solid);
                    AddE(c, c.World.Enemies[1], x + 8, b); AddE(c, c.World.Enemies[0], x + 16, b); AddE(c, c.World.Enemies[1], x + 22, b);
                    AddI(c, "soul", x + 13, b - 5); c.Cursor += 28; break;

                case "ruins":
                    Ground(c, x, x + 22, _ => b, T.Solid); Solid(c, x + 4, b - 3, 2, 3); Solid(c, x + 10, b - 4, 2, 4); Solid(c, x + 16, b - 2, 3, 2); Plat(c, x + 6, b - 6, 5);
                    AddE(c, c.World.Enemies[0], x + 12, b); AddI(c, "soul", x + 8, b - 7);
                    c.Decor.Add(("arch", (x + 13) * T.Tile, b * T.Tile, 1.4f)); c.Cursor += 22; break;

                case "tombs":
                    Ground(c, x, x + 24, _ => b, T.Solid);
                    for (int i = 0; i < 5; i++)
                    {
                        Solid(c, x + 3 + i * 4, b - 2, 1, 2); Plat(c, x + 2 + i * 4, b - 3, 3);
                        c.Decor.Add(("tomb", (x + 5 + i * 4) * T.Tile, b * T.Tile, 1f));
                    }
                    AddE(c, c.World.Enemies[0], x + 8, b); AddE(c, c.World.Enemies[1], x + 16, b); AddI(c, "heart", x + 11, b - 4); c.Cursor += 24; break;

                case "crypt":
                    Ground(c, x, x + 6, _ => b, T.Solid); int dip = b + 3;
                    for (int i = x + 6; i < x + 20; i++) FillDown(c, i, dip, T.Solid);
                    Plat(c, x + 5, b, 2); Plat(c, x + 19, b, 2);
                    Plat(c, x + 9, b + 1, 3); Plat(c, x + 15, b + 1, 3);
                    AddE(c, c.World.Enemies[1], x + 10, dip); AddE(c, c.World.Enemies[0], x + 15, dip);
                    AddI(c, "soul", x + 12, dip - 1);
                    Ground(c, x + 20, x + 24, _ => b, T.Solid); c.Cursor += 24; break;

                case "hall":
                    Ground(c, x, x + 28, _ => b, T.Solid);
                    for (int i = 0; i < 4; i++)
                    {
                        Solid(c, x + 5 + i * 6, b - 4, 1, 4); Plat(c, x + 4 + i * 6, b - 4, 3);
                        Plat(c, x + 4 + i * 6, b - 8, 3);
                        c.Decor.Add(("torch", (x + 5 + i * 6) * T.Tile, (b - 5) * T.Tile, 1f));
                    }
                    AddE(c, c.World.Enemies[0], x + 8, b); AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 16, b - 9); AddE(c, c.World.Enemies[1], x + 22, b);
                    AddI(c, "soul", x + 17, b - 5); c.Cursor += 28; break;

                case "towers":
                    // both towers are now climbable: a four-tile stump, then platforms and a ladder
                    Ground(c, x, x + 20, _ => b, T.Solid); Solid(c, x + 4, b - 4, 3, 4); Solid(c, x + 13, b - 6, 3, 6);
                    Plat(c, x + 7, b - 4, 6); Plat(c, x + 7, b - 8, 6); Plat(c, x + 3, b - 8, 3);
                    for (int y = b - 9; y < b; y++) Set(c, x + 8, y, T.Ladder);
                    AddE(c, c.World.Enemies[0], x + 16, b); AddI(c, "soul", x + 9, b - 9); c.Cursor += 20; break;

                case "icefields":
                    Ground(c, x, x + 26, xx => b - Mathf.RoundToInt(Mathf.Sin((xx - x) * 0.3f)), T.Ice);
                    Plat(c, x + 8, b - 4, 3); Plat(c, x + 14, b - 6, 3, T.Bounce);
                    AddE(c, c.World.Enemies[0], x + 10, b); AddE(c, c.World.Enemies[1], x + 18, b);
                    AddI(c, "soul", x + 15, b - 9); c.Cursor += 26; break;

                case "lava":
                    // shallow crust pool: two tiles of lava on a solid bed, stepping stones above.
                    // Stones are four and five wide with single-tile gaps — the old three-wide
                    // spacing put the hardest leap under the flyer and the crossing failed on luck.
                    Ground(c, x, x + 3, _ => b, T.Solid);
                    for (int i = x + 3; i < x + 20; i++) { FillDown(c, i, b + 4, T.Solid); Set(c, i, b + 3, T.Lava); Set(c, i, b + 2, T.Lava); }
                    Plat(c, x + 4, b - 1, 4); Plat(c, x + 9, b - 3, 4); Plat(c, x + 14, b - 2, 5);
                    AddE(c, c.World.Flyer ?? c.World.Enemies[0], x + 7, b - 7);
                    AddI(c, "soul", x + 10, b - 5);
                    Ground(c, x + 20, x + 24, _ => b, T.Solid); c.Cursor += 24; break;

                case "fade":
                    Ground(c, x, x + 3, _ => b, T.Solid);
                    for (int i = 0; i < 5; i++) Plat(c, x + 4 + i * 4, b - 1 - (i % 2), 3);
                    for (int i = x + 3; i < x + 24; i++) { FillDown(c, i, b + 4, T.Solid); Set(c, i, b + 3, Sting(c.World)); }
                    AddI(c, "soul", x + 12, b - 3);
                    Ground(c, x + 24, x + 28, _ => b, T.Solid); c.Cursor += 28; break;

                case "cliffs":
                    Ground(c, x, x + 4, _ => b, T.Solid); Ground(c, x + 4, x + 8, _ => b - 3, T.Solid); Ground(c, x + 8, x + 11, _ => b - 6, T.Solid);
                    // the chasm used to bottom out nine tiles below the rim with no way back up
                    for (int i = x + 11; i < x + 16; i++) { FillDown(c, i, b + 1, T.Solid); Set(c, i, b, Sting(c.World)); }
                    Plat(c, x + 12, b - 5, 3); Plat(c, x + 12, b - 1, 3);
                    Ground(c, x + 16, x + 22, _ => b - 2, T.Solid); AddE(c, c.World.Enemies[1], x + 18, b - 2);
                    c.Zones.Add(("wind", (x + 11) * T.Tile, (x + 16) * T.Tile, 1.1f));
                    AddI(c, "soul", x + 13, b - 6); c.Cursor += 22; break;

                case "wind":
                    Ground(c, x, x + 6, _ => b, T.Solid);
                    for (int i = 0; i < 4; i++) Plat(c, x + 7 + i * 4, b - 2 - (i % 3), 3);
                    for (int i = x + 6; i < x + 22; i++) { FillDown(c, i, b + 4, T.Solid); Set(c, i, b + 3, Sting(c.World)); }
                    Ground(c, x + 22, x + 26, _ => b, T.Solid); AddE(c, c.World.Flyer ?? c.World.Enemies[0], x + 14, b - 7);
                    c.Zones.Add(("wind", (x + 6) * T.Tile, (x + 22) * T.Tile, 1.4f));
                    AddI(c, "soul", x + 18, b - 6); c.Cursor += 26; break;

                case "cave":
                    Ground(c, x, x + 26, _ => b, T.Solid);
                    for (int i = x; i < x + 26; i++) { int ceil = 2 + Mathf.RoundToInt(Mathf.Sin((i - x) * 0.4f) + 1); for (int y = 0; y < ceil; y++) Set(c, i, y, T.Solid); }
                    Plat(c, x + 6, b - 3, 4); Plat(c, x + 14, b - 4, 4);
                    AddE(c, c.World.Enemies[0], x + 8, b); AddE(c, c.World.Enemies[1], x + 18, b);
                    AddI(c, "soul", x + 15, b - 5); c.Cursor += 26; break;

                case "roofs":
                    Ground(c, x, x + 26, _ => b, T.Solid);
                    for (int i = 0; i < 4; i++)
                    {
                        int bx = x + 2 + i * 6; Solid(c, bx, b - 4, 4, 4); Plat(c, bx - 1, b - 5, 6);
                        c.Decor.Add(("lamp", (bx + 4) * T.Tile, b * T.Tile, 1f));
                    }
                    AddE(c, c.World.Enemies[0], x + 10, b - 5); AddE(c, c.World.Enemies[1], x + 20, b);
                    AddI(c, "soul", x + 14, b - 7); c.Cursor += 26; break;

                case "street":
                    Ground(c, x, x + 24, _ => b, T.Solid); AddE(c, c.World.Enemies[0], x + 6, b); AddE(c, c.World.Enemies[1], x + 12, b); AddE(c, c.World.Enemies[0], x + 18, b);
                    Plat(c, x + 8, b - 3, 4); AddI(c, "soul", x + 9, b - 4); c.Cursor += 24; break;

                case "nave":
                    // the columns used to run floor to ceiling and wall the cathedral off; they hang now
                    Ground(c, x, x + 24, _ => b, T.Solid);
                    for (int i = 0; i < 3; i++)
                    {
                        int colX = x + 4 + i * 7;
                        Solid(c, colX, 2, 1, 8); Plat(c, colX - 1, b - 4, 3);
                        c.Decor.Add(("torch", colX * T.Tile, (b - 5) * T.Tile, 1.1f));
                    }
                    Plat(c, x + 8, b - 8, 5);
                    AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 8, b - 9); AddE(c, c.World.Enemies[0], x + 16, b);
                    AddI(c, "soul", x + 10, b - 9); c.Cursor += 24; break;

                case "gauntlet":
                    Ground(c, x, x + 26, _ => b, T.Solid); Plat(c, x + 6, b - 3, 4); Plat(c, x + 14, b - 4, 4);
                    AddE(c, c.World.Enemies[0], x + 5, b); AddE(c, c.World.Enemies[1], x + 9, b - 3); AddE(c, c.World.Enemies[0], x + 13, b);
                    AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 17, b - 6); AddE(c, c.World.Enemies[1], x + 21, b);
                    AddI(c, "heart", x + 16, b - 5); c.Cursor += 26; break;

                case "check":
                    Ground(c, x, x + 12, _ => b, T.Solid);
                    c.Check = new Vector2((x + 6) * T.Tile, (b - 2) * T.Tile); c.HasCheck = true;
                    AddI(c, "heart", x + 8, b - 1);
                    c.Decor.Add(("shrine", (x + 6) * T.Tile, b * T.Tile, 1.2f));
                    c.Cursor += 12; break;

                case "finalgate":
                    Ground(c, x, x + 20, _ => b, T.Solid); AddE(c, c.World.Enemies[0], x + 6, b); AddE(c, c.World.Enemies[1], x + 10, b);
                    AddE(c, c.World.Flyer ?? c.World.Enemies[1], x + 14, b - 5); AddI(c, "heart", x + 16, b - 1);
                    c.Decor.Add(("arch", (x + 18) * T.Tile, b * T.Tile, 1.6f)); c.Cursor += 20; break;

                default:
                    Ground(c, x, x + 20, _ => b, T.Solid); c.Cursor += 20; break;
            }
        }

        static void StampArena(Ctx c)
        {
            int x = c.Cursor, w = 32, b = c.Base;
            Ground(c, x, x + w, _ => b, T.Solid);
            // hanging jambs, not walls: these used to reach the floor and sealed every boss arena
            // shut — no level could ever be finished, in any world
            Solid(c, x, b - 8, 1, 5); Solid(c, x + w - 1, b - 8, 1, 5);
            Plat(c, x + 4, b - 4, 4); Plat(c, x + w - 8, b - 4, 4);
            Plat(c, x + 13, b - 6, 6);
            c.BossAt = new Vector2((x + w - 10) * T.Tile, b * T.Tile);
            c.Exit = new Vector2((x + w - 4) * T.Tile, (b - 2) * T.Tile);
            c.ArenaX0 = x * T.Tile; c.ArenaX1 = (x + w) * T.Tile;
            c.Decor.Add(("arena", (x + w / 2) * T.Tile, b * T.Tile, 2f));
            c.Decor.Add(("torch", (x + 2) * T.Tile, (b - 6) * T.Tile, 1.2f));
            c.Decor.Add(("torch", (x + w - 3) * T.Tile, (b - 6) * T.Tile, 1.2f));
            c.Cursor += w;
        }

        // ───────────────────────── reachability safety net ─────────────────────────

        static bool Walk(int t) => t == T.Solid || t == T.Ice || t == T.Break || t == T.Platform || t == T.Bounce;

        static List<int>[] Stands(Ctx c)
        {
            var stand = new List<int>[c.Cols];
            for (int x = 0; x < c.Cols; x++)
            {
                stand[x] = new List<int>();
                for (int y = 1; y < c.Rows; y++)
                    if (Walk(Get(c, x, y)) && !Walk(Get(c, x, y - 1)))
                        stand[x].Add(y);
            }
            return stand;
        }

        /// <summary>
        /// Highest place you can stand in each column, or Rows when there is none.
        /// A column whose top standing spot is far above you is a wall you cannot cross.
        /// </summary>
        static int[] Skyline(Ctx c, List<int>[] stand)
        {
            var top = new int[c.Cols];
            for (int x = 0; x < c.Cols; x++) top[x] = stand[x].Count > 0 ? stand[x][0] : c.Rows;
            return top;
        }

        /// <summary>Nothing in between may stand taller than one jump above the lower end of the hop.</summary>
        static bool Clear(int[] top, int x, int nx, int y, int ny)
        {
            int lo = Mathf.Min(y, ny) - JumpUp;
            int a = Mathf.Min(x, nx) + 1, b = Mathf.Max(x, nx);
            for (int cx = a; cx < b; cx++) if (top[cx] < lo) return false;
            return true;
        }

        /// <summary>Flood the level as a graph of standing spots the hero can hop between.</summary>
        static HashSet<int> Flood(Ctx c, List<int>[] stand)
        {
            var top = Skyline(c, stand);
            int sx = Mathf.Clamp(Mathf.RoundToInt(c.Spawn.x / T.Tile), 0, c.Cols - 1);
            var seen = new HashSet<int>();
            var queue = new Queue<int>();
            for (int d = 0; d <= 4 && seen.Count == 0; d++)
                foreach (int probe in new[] { sx - d, sx + d })
                    if (probe >= 0 && probe < c.Cols)
                        foreach (var y in stand[probe])
                            if (seen.Add(probe * 64 + y)) queue.Enqueue(probe * 64 + y);

            while (queue.Count > 0)
            {
                int node = queue.Dequeue();
                int x = node / 64, y = node % 64;
                for (int dx = -Reach - 1; dx <= Reach + 1; dx++)
                {
                    int nx = x + dx;
                    if (nx < 0 || nx >= c.Cols) continue;
                    bool ladder = HasLadder(c, x) && HasLadder(c, nx);
                    foreach (var ny in stand[nx])
                    {
                        int climb = y - ny;                        // positive = going up
                        bool ok = ladder
                            ? Mathf.Abs(dx) <= 2
                            : (climb <= JumpUp && Mathf.Abs(dx) <= (climb > 0 ? Reach : Reach + 1));
                        if (!ok) continue;
                        if (!ladder && !Clear(top, x, nx, y, ny)) continue;
                        int key = nx * 64 + ny;
                        if (seen.Add(key)) queue.Enqueue(key);
                    }
                }
            }
            return seen;
        }

        static bool HasLadder(Ctx c, int x)
        {
            for (int y = 0; y < c.Rows; y++) if (Get(c, x, y) == T.Ladder) return true;
            return false;
        }

        static bool Reached(HashSet<int> seen, List<int>[] stand, int x)
        {
            if (x < 0 || x >= stand.Length) return false;
            foreach (var y in stand[x]) if (seen.Contains(x * 64 + y)) return true;
            return false;
        }

        /// <summary>
        /// Makes the level provably walkable and provably escapable:
        /// 1. any wall too tall to climb gets a staircase of platforms in front of it,
        /// 2. any hole the hero can drop into but not climb out of gets a hazard floor, so the
        ///    sim's rescue lifts him back to solid ground instead of caging him until he dies.
        /// </summary>
        static void SealUnreachable(Ctx c)
        {
            int goal = Mathf.Clamp(Mathf.RoundToInt(c.Exit.x / T.Tile), 0, c.Cols - 1);
            List<int>[] stand = null;
            HashSet<int> seen = null;

            for (int pass = 0; pass < 40; pass++)
            {
                stand = Stands(c);
                seen = Flood(c, stand);
                if (Reached(seen, stand, goal)) break;
                if (!Ramp(c, stand, seen, goal)) break;
                c.Sealed += 1000;                                  // bookkeeping: a ramp was needed
            }

            int sting = Sting(c.World);
            int arena0 = Mathf.FloorToInt(c.ArenaX0 / T.Tile), arena1 = Mathf.CeilToInt(c.ArenaX1 / T.Tile);
            for (int x = 0; x < c.Cols; x++)
            {
                if (x >= arena0 && x <= arena1) continue;
                foreach (var y in stand[x])
                {
                    if (seen.Contains(x * 64 + y)) continue;
                    if (y <= c.Base) continue;                     // ledges above the line are scenery
                    if (y >= c.Rows - 1) continue;
                    if (Get(c, x, y) == T.Platform) continue;
                    Set(c, x, y, sting);
                    c.Sealed++;
                }
            }
        }

        /// <summary>Stacks platforms in front of the first wall that blocks the run.</summary>
        static bool Ramp(Ctx c, List<int>[] stand, HashSet<int> seen, int goal)
        {
            int blocked = -1;
            for (int x = 0; x <= goal; x++)
                if (stand[x].Count > 0 && !Reached(seen, stand, x)) { blocked = x; break; }
            if (blocked < 0) return false;

            int from = -1;
            for (int x = blocked - 1; x >= 0; x--) if (Reached(seen, stand, x)) { from = x; break; }
            if (from < 0) return false;

            int yFrom = -1;
            foreach (var y in stand[from]) if (seen.Contains(from * 64 + y) && y > yFrom) yFrom = y;
            int yTo = int.MaxValue;
            foreach (var y in stand[blocked]) if (y < yTo) yTo = y;
            if (yFrom < 0 || yTo == int.MaxValue || yTo >= yFrom) return false;

            bool placed = false;
            int step = yFrom - 3;
            while (step - yTo > JumpUp - 1 && step > 1)
            {
                if (Get(c, from, step) == T.Empty) { Set(c, from, step, T.Platform); placed = true; }
                step -= 3;
            }
            if (step > yTo && step > 1 && Get(c, from, step) == T.Empty) { Set(c, from, step, T.Platform); placed = true; }
            return placed;
        }

        // ───────────────────────── scenery ─────────────────────────

        static void ScatterDecor(Ctx c, int worldIndex)
        {
            string kind = c.World.Id switch
            {
                "forest" or "woods" => "tree",
                "cabin" or "village" => "lamp",
                "swamp" or "catacombs" => "reed",
                "desert" => "obelisk",
                "grave" => "tomb",
                "castle" or "cathedral" or "throne" => "pillar",
                "ice" or "abyss" => "crystal",
                "volcano" or "peak" => "rock",
                "coven" => "candle",
                _ => "rock"
            };
            for (int x = 3; x < c.Cols - 3; x += 5)
            {
                float h = ArtGen.Hash(x, worldIndex * 7, 3.7f);
                if (h < 0.45f) continue;
                int surf = -1;
                for (int y = 1; y < c.Rows; y++)
                    if (Walk(Get(c, x, y)) && !Walk(Get(c, x, y - 1))) { surf = y; break; }
                if (surf < 2 || surf > c.Base + 1) continue;
                if (x * T.Tile > c.ArenaX0 - 80) continue;
                c.Decor.Add((kind, x * T.Tile + 20, surf * T.Tile, 0.75f + h * 0.85f));
            }
        }
    }
}
