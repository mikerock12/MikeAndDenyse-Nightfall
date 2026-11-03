#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Nightfall
{
    namespace T
    {
        constexpr int Empty = 0;
        constexpr int Solid = 1;
        constexpr int Platform = 2;
        constexpr int Spike = 3;
        constexpr int Lava = 4;
        constexpr int Ice = 5;
        constexpr int Water = 6;
        constexpr int Break = 7;
        constexpr int Bounce = 8;
        constexpr int Ladder = 9;
        constexpr int Thorn = 10;
        constexpr int Tile = 40;
        constexpr int ViewW = 960;
        constexpr int ViewH = 540;
    }

    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
        float SqrMagnitude() const { return x * x + y * y; }
    };

    struct HeroDef
    {
        std::string Id;
        std::string Name;
        std::string Title;
        int Hp = 6;
        float Speed = 3.55f;
        float Jump = -12.2f;
        std::string AtkKind = "melee";
        float AtkTime = 0.30f;
        float AtkCd = 0.34f;
        int Damage = 2;
        int Reach = 58;
    };

    struct EnemyDef
    {
        std::string Id;
        std::string Name;
        std::string Ai;
        std::string Shot;
        int Hp = 1;
        int W = 32;
        int H = 32;
        int Dmg = 1;
        int Score = 100;
        float Speed = 1.0f;
        bool Fly = false;
    };

    struct BossDef
    {
        std::string Id;
        std::string Name;
        std::string Pattern;
        int Hp = 28;
        int W = 90;
        int H = 110;
        int Score = 2000;
        float Speed = 0.55f;
    };

    struct WorldDef
    {
        std::string Id;
        std::string Name;
        std::string Tone;
        std::string Fog;
        std::string Ground;
        std::string Lip;
        std::string Hazard;
        std::vector<std::string> Enemies;
        std::string Flyer;
        std::string Boss;
        int Cols = 196;
        std::vector<std::string> Beats;
    };

    class Catalog
    {
    public:
        static const std::unordered_map<std::string, HeroDef>& Heroes();
        static const std::vector<EnemyDef>& Enemies();
        static const std::vector<BossDef>& Bosses();
        static const BossDef& Belial();
        static const std::vector<WorldDef>& Worlds();

        static const HeroDef* Hero(const std::string& id);
        static const EnemyDef* Enemy(const std::string& id);
        static const BossDef* Boss(const std::string& id);
        static int EnemyIndex(const std::string& id);
        static const EnemyDef* EnemyAt(int i);
        static int BossIndex(const std::string& id);
        static const BossDef* BossAt(int i);
    };

    struct LevelEntSpawn { std::string id; float x = 0.0f; float y = 0.0f; };
    struct LevelItemSpawn { std::string type; float x = 0.0f; float y = 0.0f; };
    struct LevelZone { std::string type; float x0 = 0.0f; float x1 = 0.0f; float vx = 0.0f; };
    struct LevelDecor { std::string kind; float x = 0.0f; float y = 0.0f; float scale = 1.0f; };

    struct LevelData
    {
        int WorldIndex = 0;
        int Cols = 0;
        int Rows = 20;
        WorldDef World;
        std::vector<std::vector<uint8_t>> Tiles;
        Vec2 Spawn;
        Vec2 Exit;
        bool HasCheck = false;
        Vec2 Check;
        Vec2 BossAt;
        float ArenaX0 = 0.0f;
        float ArenaX1 = 0.0f;
        std::vector<LevelEntSpawn> Ents;
        std::vector<LevelItemSpawn> Items;
        std::vector<LevelZone> Zones;
        std::vector<LevelDecor> Decor;
        int SealedPockets = 0;
    };

    class LevelBuilder
    {
    public:
        static float ArtHash(float x, float y, float s);
        static LevelData Compile(int worldIndex);
        static bool Walk(int t);
        static std::unordered_set<int> Reach(const LevelData& L);
        static bool Walkable(const LevelData& L, std::string& why);
    };

    class Progress
    {
    public:
        static constexpr int WorldCount = 16;
        static int Unlocked;
        static std::string Hero;
        static int Souls;
        static int Score;
        static int LastWorld;

        static void SetStoragePath(const std::string& path);
        static void Load();
        static void Save();
        static bool CanPlay(int worldIndex);
        static void OnClear(int worldIndex);
        static void RememberRun(int souls, int score, const std::string& hero);
        static void ResetAll();
    };

    class LanIp
    {
    public:
        static int ScoreAddressBytes(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);
        static int ScoreAddress(const std::string& ip);
        static bool IsUsable(const std::string& host);
        static int ScoreIface(const std::string& nameAndDesc);
        static std::string Pick();
    };

    enum class FxKind : uint8_t
    {
        Hit = 0, Slash = 1, Death = 2, Pickup = 3, Heal = 4,
        Land = 5, Hurt = 6, Boom = 7, Rescue = 8, Cast = 9, BossDown = 10,
        Charge = 11, Burst = 12
    };

    struct FxEvent
    {
        uint8_t Kind = 0;
        float X = 0.0f;
        float Y = 0.0f;
        int Dir = 1;
    };

    struct Actor
    {
        float X = 0.0f, Y = 0.0f, W = 30.0f, H = 48.0f, Vx = 0.0f, Vy = 0.0f;
        int Dir = 1, Facing = 1, Hp = 1, MaxHp = 1;
        bool OnGround = false, Dead = false, Water = false;
        float FlipCd = 0.0f, Stun = 0.0f, Flash = 0.0f, T = 0.0f;
        int HitS0 = -1, HitS1 = -1;
        virtual ~Actor() = default;
    };

    struct PlayerA : Actor
    {
        int Slot = 0, Lives = 3;
        HeroDef Hero;
        float Coyote = 0.0f, JumpBuf = 0.0f, Atk = 0.0f, AtkCd = 0.0f, Inv = 0.0f, HurtT = 0.0f, AnimT = 0.0f;
        std::string Anim = "idle";
        float SpawnX = 0.0f, SpawnY = 0.0f;
        int Jumps = 0;
        float SafeX = 0.0f, SafeY = 0.0f, SafeT = 0.0f;
        bool WasAir = false;
        int Swing = 0;
        bool Fired = false;
        float Drop = 0.0f;
    };

    struct EnemyA : Actor
    {
        EnemyDef Def;
        std::string Id;
        float HomeX = 0.0f, HomeY = 0.0f, ShotT = 1.0f;
        int Kind = 0;
    };

    struct BossA : Actor
    {
        BossDef Def;
        std::string Id;
        std::string State = "idle";
        float StateT = 1.2f, Intro = 1.6f;
    };

    struct ProjA
    {
        float X = 0.0f, Y = 0.0f, W = 12.0f, H = 12.0f, Vx = 0.0f, Vy = 0.0f, Life = 1.0f, Age = 0.0f;
        int Dmg = 1;
        bool Friendly = false;
        std::string Kind = "dark";
    };

    struct ItemA
    {
        std::string Type;
        float X = 0.0f, Y = 0.0f, Bob = 0.0f;
        bool Taken = false;
    };

    struct Ctl
    {
        bool L = false, R = false, D = false, Jp = false, Jn = false, Ap = false;
    };

    // Network Snapshot v2 types (mirrors Nightfall.Net.GameNet)
    struct InputMsg
    {
        bool L = false, R = false, D = false, Jp = false, Jn = false, Ap = false;
    };

    struct PlrSnap
    {
        float X = 0.0f, Y = 0.0f;
        uint8_t Hp = 0, MaxHp = 0, Lives = 0;
        uint8_t Anim = 0;
        uint8_t Atk = 0;
        uint8_t Flags = 0;
        uint8_t Hero = 0;
    };

    struct EntSnap
    {
        float X = 0.0f, Y = 0.0f;
        uint8_t Hp = 0;
        uint8_t Kind = 0;
        uint8_t Flags = 0;
    };

    struct ProjSnap
    {
        float X = 0.0f, Y = 0.0f, Vx = 0.0f, Vy = 0.0f;
        uint8_t Kind = 0;
        uint8_t Flags = 0;
    };

    struct FxSnap
    {
        float X = 0.0f, Y = 0.0f;
        uint8_t Kind = 0;
        uint8_t Dir = 1;
    };

    struct FrameSnap
    {
        static constexpr int MaxEnts = 20;
        static constexpr int MaxProjs = 12;
        static constexpr int MaxFx = 8;

        PlrSnap P0{};
        PlrSnap P1{};
        int Score = 0, Souls = 0;
        bool LockArena = false;
        std::string SimState = "play";
        int BossHp = 0, BossMax = 0;
        uint8_t BossKind = 0, BossFlags = 0;
        Vec2 BossPos{};
        uint64_t ItemMask = 0;
        float Shake = 0.0f;
        std::string Msg;
        float MsgT = 0.0f;
        std::vector<EntSnap> Ents;
        std::vector<ProjSnap> Projs;
        std::vector<FxSnap> Fxs;

        std::vector<uint8_t> Serialize() const;
        static bool Deserialize(const uint8_t* data, size_t len, FrameSnap& out);
    };

    class GameSim
    {
    public:
        LevelData Level;
        bool HasLevel = false;
        std::vector<PlayerA> Players;
        std::vector<EnemyA> Enemies;
        std::vector<BossA> Bosses;
        std::vector<ProjA> Projectiles;
        std::vector<ItemA> Items;
        std::vector<FxEvent> Fx;
        float CamX = 0.0f, CamY = 0.0f, Shake = 0.0f, Time = 0.0f, Fade = 1.0f, FadeDir = -1.0f;
        float MessageT = 0.0f, BossIntro = 0.0f, HitStop = 0.0f;
        std::string Message;
        std::string State = "play";
        int Score = 0, Souls = 0, WorldIndex = 0;
        bool LockedArena = false, BelialPhase = false;
        std::function<void(const std::string&)> Sfx;
        std::function<Ctl(int)> ReadCtl;

        static void InitRandomState(uint32_t seed);
        float NextRandom01();
        int NextRandomRange(int minInclusive, int maxExclusive);

        void Emit(FxKind kind, float x, float y, int dir = 1);
        int PeekTile(float px, float py) const;
        void Start(int world, const std::string& h1, const std::string& h2, bool two);
        void Tick(float dt);
        void TickRemote(float dt);
        void EnsureSecond(const std::string& hero);

        void ApplyNetEnemies(const std::vector<EntSnap>& ents);
        void ApplyNetProjectiles(const std::vector<ProjSnap>& ps);
        void ApplyNetItems(uint64_t mask);
        uint64_t ItemMask() const;
        FrameSnap CaptureSnapshot();
        void ApplySnapshot(const FrameSnap& snap);

        static uint8_t ProjKindId(const std::string& kind);
        static std::string ProjKindName(uint8_t id);
        static bool TryParseHexColor(const std::string& hex, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

    private:
        std::vector<BossA> _spawnQueue;
        bool _noPlat = false;

        int TileAt(float px, float py) const;
        bool SolidAt(float px, float py, bool falling) const;
        static bool Block(int t);
        static bool Hazard(int t);
        static float ModTile(float py);
        static bool HazardFloor(int t);

        PlayerA MakePlayer(int slot, const std::string& hero, float x, float y);
        void FlushSpawns();
        Ctl Inp(int slot) const;
        void UpdatePlayer(PlayerA& p, float dt);
        void RememberSafe(PlayerA& p, float dt);
        void HazardHit(PlayerA& p, int dmg);
        void Rescue(PlayerA& p);
        void Phys(Actor& a, float dt, float grav, bool fly);
        void MoveX(Actor& a, float d);
        bool MoveY(Actor& a, float d);
        bool WallAhead(const Actor& e) const;
        bool Ledge(const Actor& e) const;
        bool Bounds(const Actor& e) const;
        void Flip(Actor& e, float cd);
        void FaceWalk(Actor& e, float dt, float speed);
        void Lift(Actor& e);
        PlayerA* Nearest(float x, float y);
        void CollidePlayers(Actor& e, int dmg, bool isBoss);
        static bool Aabb(const Actor& a, const Actor& b);
        void UpdateEnemy(EnemyA& e, float dt);
        void UpdateBoss(BossA& b, float dt);
        void FireMagic(PlayerA& p);
        void FireShot(const Actor& e, const std::string& kind);
        void MeleeSweep(PlayerA& p, float st);
        void TrySwing(PlayerA& p, Actor& e, bool isBoss, float cx, float cy, float reach, float sweep);
        void UpdateProj(float dt);
        void UpdateItems(float dt);
        void HurtFrom(PlayerA& p, int dmg, float srcX);
        void HurtDirect(PlayerA& p, int dmg);
        void DmgEnemy(Actor& e, bool isBoss, int dmg, float pushDir = 0.0f, float push = 0.0f);
        void MaybeLock();
        void CheckClear();
        void UpdateCam(float dt);
    };
}
