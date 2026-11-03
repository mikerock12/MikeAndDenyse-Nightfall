#include "NightfallCore.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <queue>
#include <sstream>
#include <array>
#include <bit>
#include <type_traits>

namespace Nightfall
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kRad2Deg = 57.29577951308232f;

        inline float ClampF(float v, float lo, float hi)
        {
            return v < lo ? lo : (v > hi ? hi : v);
        }

        inline int ClampI(int v, int lo, int hi)
        {
            return v < lo ? lo : (v > hi ? hi : v);
        }

        inline float Clamp01(float v)
        {
            return ClampF(v, 0.0f, 1.0f);
        }

        inline float LerpF(float a, float b, float t)
        {
            return a + (b - a) * Clamp01(t);
        }

        inline int RoundToInt(float v)
        {
            return static_cast<int>(std::lrint(v));
        }

        inline int FloorToInt(float v)
        {
            return static_cast<int>(std::floor(v));
        }

        inline int CeilToInt(float v)
        {
            return static_cast<int>(std::ceil(v));
        }

        inline float SignF(float v)
        {
            return v >= 0.0f ? 1.0f : -1.0f;
        }

        inline float RepeatF(float t, float length)
        {
            return ClampF(t - std::floor(t / length) * length, 0.0f, length);
        }

        inline float DeltaAngle(float current, float target)
        {
            float delta = RepeatF(target - current, 360.0f);
            if (delta > 180.0f) delta -= 360.0f;
            return delta;
        }

        std::vector<std::string> SplitComma(const std::string& s)
        {
            std::vector<std::string> out;
            std::stringstream ss(s);
            std::string item;
            while (std::getline(ss, item, ','))
            {
                out.push_back(item);
            }
            return out;
        }

        EnemyDef MakeE(const char* id, const char* n, int hp, int w, int h, float sp, const char* ai, int dmg, int sc, bool fly, const char* shot = "")
        {
            EnemyDef d;
            d.Id = id; d.Name = n; d.Hp = hp; d.W = w; d.H = h;
            d.Speed = sp; d.Ai = ai; d.Dmg = dmg; d.Score = sc; d.Fly = fly;
            d.Shot = shot ? shot : "";
            return d;
        }

        BossDef MakeB(const char* id, const char* n, int hp, int w, int h, float sp, int sc, const char* pat)
        {
            BossDef d;
            d.Id = id; d.Name = n; d.Hp = hp; d.W = w; d.H = h;
            d.Speed = sp; d.Score = sc; d.Pattern = pat;
            return d;
        }

        WorldDef MakeW(const char* id, const char* n, const char* tone, const char* fog, const char* g, const char* lip,
                       const char* hz, std::initializer_list<const char*> en, const char* flyer, const char* boss,
                       int cols, const char* beats)
        {
            WorldDef w;
            w.Id = id; w.Name = n; w.Tone = tone; w.Fog = fog; w.Ground = g; w.Lip = lip;
            w.Hazard = hz;
            for (const char* e : en) w.Enemies.emplace_back(e);
            w.Flyer = flyer ? flyer : "";
            w.Boss = boss;
            w.Cols = cols;
            w.Beats = SplitComma(beats);
            return w;
        }

        uint32_t g_RngState = 0x12345678u;
    }

    // ───────────────────────────── Catalog ─────────────────────────────

    const std::unordered_map<std::string, HeroDef>& Catalog::Heroes()
    {
        static const std::unordered_map<std::string, HeroDef> kHeroes = {
            {"mike", HeroDef{"mike", "Mike", "Caçador da Lâmina", 6, 3.55f, -12.2f, "melee", 0.30f, 0.34f, 2, 58}},
            {"denyse", HeroDef{"denyse", "Denyse", "Bruxa do Cristal", 5, 3.85f, -11.8f, "magic", 0.22f, 0.38f, 2, 220}}
        };
        return kHeroes;
    }

    const std::vector<EnemyDef>& Catalog::Enemies()
    {
        static const std::vector<EnemyDef> kEnemies = {
            MakeE("wraith","Espectro da Floresta",2,38,50,0.75f,"patrol",1,120,false),
            MakeE("imp","Diabrete de Espinhos",1,32,34,1.35f,"jump",1,80,false),
            MakeE("crow","Corvo Amaldiçoado",1,36,28,1.6f,"swoop",1,90,true),
            MakeE("doll","Boneca da Cabana",2,30,40,0.9f,"charge",1,140,false),
            MakeE("slime","Limo do Pântano",3,40,28,0.45f,"patrol",1,100,false),
            MakeE("leech","Sanguessuga do Brejo",2,36,22,1.2f,"fly",1,110,true),
            MakeE("mummy","Múmia Andarilha",4,36,50,0.55f,"patrol",1,160,false),
            MakeE("scorpion","Escorpião de Cinzas",2,42,28,1.4f,"charge",1,130,false),
            MakeE("skelly","Arqueiro de Ossos",2,34,48,0.5f,"shoot",1,170,false,"bone"),
            MakeE("ghoul","Carniçal da Cova",3,38,46,1.05f,"charge",1,150,false),
            MakeE("bat","Morcego Vampiro",1,34,26,1.7f,"swoop",1,90,true),
            MakeE("thrall","Servo de Sangue",3,36,48,0.85f,"patrol",1,150,false),
            MakeE("icewraith","Espectro de Gelo",2,36,48,0.8f,"fly",1,140,true),
            MakeE("frostknight","Cavaleiro Congelado",5,40,52,0.6f,"patrol",2,220,false),
            MakeE("fireimp","Diabrete de Fogo",2,30,34,1.3f,"jump",1,120,false),
            MakeE("magmagolem","Golem de Magma",6,46,52,0.4f,"tank",2,260,false),
            MakeE("familiar","Familiar da Coven",1,30,26,1.8f,"swoop",1,100,true),
            MakeE("hexwitch","Bruxa do Hex",3,36,48,0.7f,"mage",1,200,false,"hex"),
            MakeE("drowned","Afogado",3,36,46,0.65f,"patrol",1,140,false),
            MakeE("bonefish","Peixe de Ossos",2,40,24,1.5f,"fly",1,120,true),
            MakeE("wolfpup","Lobisomem Jovem",3,42,36,1.55f,"charge",1,180,false),
            MakeE("cultist","Cultista da Lua",2,34,48,0.75f,"shoot",1,160,false,"dark"),
            MakeE("hatchling","Filhote de Dragão",3,40,32,1.1f,"jump",1,190,false),
            MakeE("wyvern","Serpe Batedora",3,48,32,1.45f,"swoop",1,210,true),
            MakeE("priest","Sacerdote Possuído",4,36,50,0.7f,"mage",1,220,false,"holy"),
            MakeE("hellhound","Cão do Inferno",3,44,32,1.7f,"charge",2,200,false),
            MakeE("plague","Aldeão da Peste",2,34,46,0.8f,"patrol",1,130,false),
            MakeE("bloodgolem","Golem de Sangue",6,46,52,0.45f,"tank",2,280,false),
            MakeE("crawler","Rastejante do Abismo",2,40,24,1.25f,"patrol",1,150,false),
            MakeE("serpent","Serpente das Sombras",4,52,28,1.15f,"fly",1,200,true),
            MakeE("acolyte","Acólito Demoníaco",3,36,48,0.8f,"mage",1,230,false,"hell"),
            MakeE("shade","Sombra do Pesadelo",4,38,50,1.1f,"swoop",2,250,true)
        };
        return kEnemies;
    }

    const std::vector<BossDef>& Catalog::Bosses()
    {
        static const std::vector<BossDef> kBosses = {
            MakeB("treant","O Ent Oco",28,90,110,0.55f,2000,"slam"),
            MakeB("babawitch","Baba da Cabana",26,70,90,0.7f,2200,"hex"),
            MakeB("hydra","Hidra do Pântano",34,100,80,0.6f,2400,"multi"),
            MakeB("pharaoh","Faraó de Cinzas",32,78,96,0.5f,2500,"summon"),
            MakeB("gravetitan","Titã da Cova",36,96,108,0.4f,2600,"slam"),
            MakeB("nosferatu","Conde Nosferatu",34,70,96,1.1f,2800,"bat"),
            MakeB("icequeen","Rainha de Gelo",32,72,96,0.75f,2800,"ice"),
            MakeB("magmatitan","Titã de Magma",40,100,110,0.45f,3000,"lava"),
            MakeB("morgana","Alta Bruxa Morgana",36,74,98,0.8f,3200,"hex"),
            MakeB("drownedking","Rei Afogado",38,86,100,0.55f,3200,"wave"),
            MakeB("alpha","Lobisomem Alfa",36,92,86,1.35f,3400,"charge"),
            MakeB("dragon","Dragão Carmesim",44,120,90,0.9f,3800,"flyfire"),
            MakeB("cathedral","Demônio da Catedral",42,96,112,0.7f,3800,"slam"),
            MakeB("priestess","Sacerdotisa de Sangue",38,74,98,0.85f,3600,"hex"),
            MakeB("leviathan","Leviatã do Abismo",46,130,80,0.7f,4000,"wave"),
            MakeB("warden","Guardião Infernal",42,100,110,0.65f,4200,"multi")
        };
        return kBosses;
    }

    const BossDef& Catalog::Belial()
    {
        static const BossDef kBelial = MakeB("belial","Belial, Senhor dos Pesadelos",72,140,130,0.85f,12000,"final");
        return kBelial;
    }

    const std::vector<WorldDef>& Catalog::Worlds()
    {
        static const std::vector<WorldDef> kWorlds = {
            MakeW("forest","Floresta Amaldiçoada","#0b1a10","#143018","#2a4a22","#6a8a38","thorn",{"wraith","imp"},"crow","treant",196,"intro,hills,pit,trees,check,canopy,pit,gauntlet"),
            MakeW("cabin","Cabana da Bruxa","#1a1010","#2a1814","#4a3020","#7a5030","spike",{"doll","imp"},"crow","babawitch",188,"intro,rooms,stairs,attic,check,rooms,pit,gauntlet"),
            MakeW("swamp","Pântano de Ossos","#0c1814","#1a3028","#2a3a28","#4a6a40","water",{"slime","drowned"},"leech","hydra",200,"intro,water,isles,pit,check,water,hills,gauntlet"),
            MakeW("desert","Deserto de Cinzas","#24180c","#3a2814","#6a4a24","#c4a060","spike",{"mummy","scorpion"},"crow","pharaoh",208,"intro,dunes,pit,ruins,check,dunes,stairs,gauntlet"),
            MakeW("grave","Cemitério Sombrio","#101018","#1c1c28","#2a2a30","#5a5a48","spike",{"skelly","ghoul"},"bat","gravetitan",200,"intro,tombs,pit,crypt,check,tombs,stairs,gauntlet"),
            MakeW("castle","Castelo Vampírico","#140810","#2a1020","#3a2030","#6a3048","spike",{"thrall","skelly"},"bat","nosferatu",210,"intro,hall,stairs,towers,check,hall,pit,gauntlet"),
            MakeW("ice","Ermos Congelados","#0c1824","#203848","#3a5a6a","#b0d4e4","ice",{"icewraith","frostknight"},"icewraith","icequeen",204,"intro,icefields,pit,stairs,check,icefields,canopy,gauntlet"),
            MakeW("volcano","Inferno Vulcânico","#1a0804","#3a1408","#4a2010","#c05020","lava",{"fireimp","magmagolem"},"crow","magmatitan",206,"intro,lava,isles,stairs,check,lava,pit,gauntlet"),
            MakeW("coven","Covil das Bruxas","#14081a","#281440","#302044","#7040a0","thorn",{"hexwitch","familiar"},"familiar","morgana",198,"intro,rooms,fade,stairs,check,fade,pit,gauntlet"),
            MakeW("catacombs","Catacumbas Afogadas","#081018","#102030","#243040","#486070","water",{"drowned","skelly"},"bonefish","drownedking",208,"intro,crypt,water,stairs,check,crypt,isles,gauntlet"),
            MakeW("woods","Bosque do Lobisomem","#10080c","#281018","#2a2018","#5a4030","thorn",{"wolfpup","cultist"},"crow","alpha",210,"intro,hills,trees,pit,check,canopy,gauntlet,hills"),
            MakeW("peak","Pico do Dragão","#180c10","#301820","#4a3030","#8a5050","spike",{"hatchling","wyvern"},"wyvern","dragon",214,"intro,cliffs,wind,stairs,check,cliffs,pit,gauntlet"),
            MakeW("cathedral","Catedral Demoníaca","#120810","#241028","#2a2030","#6a4058","spike",{"priest","hellhound"},"bat","cathedral",212,"intro,hall,towers,stairs,check,nave,pit,gauntlet"),
            MakeW("village","Vila da Lua de Sangue","#18080c","#301018","#3a2820","#8a5040","spike",{"plague","bloodgolem"},"bat","priestess",206,"intro,roofs,street,pit,check,roofs,stairs,gauntlet"),
            MakeW("abyss","Cavernas do Abismo","#080610","#140c24","#1a1830","#403868","spike",{"crawler","serpent"},"serpent","leviathan",216,"intro,cave,isles,stairs,check,cave,pit,gauntlet"),
            MakeW("throne","Trono Infernal","#140404","#2a0808","#3a1010","#8a2020","lava",{"acolyte","shade"},"shade","warden",220,"intro,gauntlet,hall,check,lava,gauntlet,stairs,finalgate")
        };
        return kWorlds;
    }

    const HeroDef* Catalog::Hero(const std::string& id)
    {
        const auto& h = Heroes();
        auto it = h.find(id);
        return it != h.end() ? &it->second : nullptr;
    }

    const EnemyDef* Catalog::Enemy(const std::string& id)
    {
        for (const auto& e : Enemies()) if (e.Id == id) return &e;
        return nullptr;
    }

    const BossDef* Catalog::Boss(const std::string& id)
    {
        if (id.empty()) return nullptr;
        if (id == "belial") return &Belial();
        for (const auto& b : Bosses()) if (b.Id == id) return &b;
        return nullptr;
    }

    int Catalog::EnemyIndex(const std::string& id)
    {
        const auto& list = Enemies();
        for (size_t i = 0; i < list.size(); ++i) if (list[i].Id == id) return static_cast<int>(i);
        return 0;
    }

    const EnemyDef* Catalog::EnemyAt(int i)
    {
        const auto& list = Enemies();
        return (i >= 0 && static_cast<size_t>(i) < list.size()) ? &list[static_cast<size_t>(i)] : nullptr;
    }

    int Catalog::BossIndex(const std::string& id)
    {
        if (id == "belial") return 100;
        const auto& list = Bosses();
        for (size_t i = 0; i < list.size(); ++i) if (list[i].Id == id) return static_cast<int>(i);
        return 0;
    }

    const BossDef* Catalog::BossAt(int i)
    {
        if (i == 100) return &Belial();
        const auto& list = Bosses();
        return (i >= 0 && static_cast<size_t>(i) < list.size()) ? &list[static_cast<size_t>(i)] : nullptr;
    }

    // ───────────────────────────── LevelBuilder ─────────────────────────────

    namespace
    {
        constexpr int kJumpUp = 5;
        constexpr int kReach = 4;

        struct BuilderCtx
        {
            WorldDef World;
            int Cols = 0, Rows = 20, Cursor = 2, Base = 15;
            std::vector<std::vector<uint8_t>> Grid;
            std::vector<LevelEntSpawn> Ents;
            std::vector<LevelItemSpawn> Items;
            std::vector<LevelZone> Zones;
            std::vector<LevelDecor> Decor;
            Vec2 Spawn{}, Exit{}, BossAt{}, Check{};
            bool HasCheck = false;
            float ArenaX0 = 0.0f, ArenaX1 = 0.0f;
            int Sealed = 0;
        };

        bool In(const BuilderCtx& c, int x, int y) { return x >= 0 && y >= 0 && x < c.Cols && y < c.Rows; }
        void Set(BuilderCtx& c, int x, int y, int t) { if (In(c, x, y)) c.Grid[y][x] = static_cast<uint8_t>(t); }
        int Get(const BuilderCtx& c, int x, int y) { return In(c, x, y) ? c.Grid[y][x] : T::Solid; }
        void FillDown(BuilderCtx& c, int x, int top, int t) { for (int y = top; y < c.Rows; ++y) Set(c, x, y, t); }
        void Rect(BuilderCtx& c, int x, int y, int w, int h, int t) { for (int iy = 0; iy < h; ++iy) for (int ix = 0; ix < w; ++ix) Set(c, x + ix, y + iy, t); }
        void Plat(BuilderCtx& c, int x, int y, int w, int t = T::Platform) { for (int i = 0; i < w; ++i) Set(c, x + i, y, t); }
        void Solid(BuilderCtx& c, int x, int y, int w, int h) { Rect(c, x, y, w, h, T::Solid); }
        template <typename Fn>
        void Ground(BuilderCtx& c, int x0, int x1, Fn h, int t)
        {
            for (int x = x0; x < x1 && x < c.Cols; ++x) FillDown(c, x, h(x), t);
        }
        void AddE(BuilderCtx& c, const std::string& id, int tx, int ty)
        {
            const auto* d = Catalog::Enemy(id);
            if (!d) return;
            c.Ents.push_back({id, tx * T::Tile + (T::Tile - d->W) / 2.0f, static_cast<float>(ty * T::Tile - d->H)});
        }
        void AddI(BuilderCtx& c, const std::string& type, int tx, int ty)
        {
            c.Items.push_back({type, static_cast<float>(tx * T::Tile + 10), static_cast<float>(ty * T::Tile + 8)});
        }
        int Hz(const WorldDef& w)
        {
            return w.Hazard == "lava" ? T::Lava : w.Hazard == "water" ? T::Water : w.Hazard == "ice" ? T::Ice : w.Hazard == "thorn" ? T::Thorn : T::Spike;
        }
        int Sting(const WorldDef& w)
        {
            return w.Hazard == "lava" ? T::Lava : w.Hazard == "thorn" ? T::Thorn : T::Spike;
        }

        void RunBeat(BuilderCtx& c, const std::string& beat)
        {
            int x = c.Cursor, b = c.Base;
            const std::string flyerOrE1 = !c.World.Flyer.empty() ? c.World.Flyer : c.World.Enemies[1];
            const std::string flyerOrE0 = !c.World.Flyer.empty() ? c.World.Flyer : c.World.Enemies[0];

            if (beat == "intro")
            {
                Ground(c, x, x + 16, [b](int) { return b; }, T::Solid);
                if (c.Spawn.SqrMagnitude() == 0.0f) c.Spawn = {static_cast<float>((x + 3) * T::Tile), static_cast<float>((b - 2) * T::Tile)};
                Plat(c, x + 9, b - 3, 3);
                AddI(c, "soul", x + 8, b - 1); AddI(c, "soul", x + 10, b - 4);
                AddE(c, c.World.Enemies[0], x + 13, b); c.Cursor += 16;
            }
            else if (beat == "hills")
            {
                Ground(c, x, x + 24, [x, b](int xx) {
                    return RoundToInt(b - 1.0f - std::sin((xx - x) * 0.38f) * 2.0f - std::sin((xx - x) * 0.11f));
                }, T::Solid);
                AddE(c, c.World.Enemies[0], x + 8, b - 2); AddE(c, c.World.Enemies[1], x + 16, b - 2);
                AddI(c, "soul", x + 11, b - 5); AddI(c, "soul", x + 20, b - 5); c.Cursor += 24;
            }
            else if (beat == "pit")
            {
                Ground(c, x, x + 4, [b](int) { return b; }, T::Solid);
                int hz = Hz(c.World);
                int floor = b + 4;
                for (int i = x + 4; i < x + 16; ++i) FillDown(c, i, floor, T::Solid);
                for (int i = x + 6; i < x + 14; ++i)
                {
                    Set(c, i, floor - 1, hz == T::Ice ? T::Spike : hz);
                    if (hz == T::Water || hz == T::Lava) Set(c, i, floor - 2, hz);
                }
                Plat(c, x + 5, b - 1, 3); Plat(c, x + 9, b - 3, 3); Plat(c, x + 13, b - 1, 3);
                if (!c.World.Flyer.empty()) AddE(c, c.World.Flyer, x + 10, b - 6);
                AddI(c, "soul", x + 10, b - 5);
                Ground(c, x + 16, x + 20, [b](int) { return b; }, T::Solid); c.Cursor += 20;
            }
            else if (beat == "trees")
            {
                Ground(c, x, x + 22, [b](int) { return b; }, T::Solid);
                for (int px : {x + 4, x + 10, x + 16})
                {
                    Solid(c, px, b - 4, 1, 4); Plat(c, px - 2, b - 4, 5); AddI(c, "soul", px, b - 5);
                    c.Decor.push_back({"canopy", static_cast<float>(px * T::Tile), static_cast<float>((b - 4) * T::Tile), 1.2f});
                }
                AddE(c, c.World.Enemies[0], x + 7, b); AddE(c, flyerOrE1, x + 13, b - 8); c.Cursor += 22;
            }
            else if (beat == "canopy")
            {
                Ground(c, x, x + 3, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 6; ++i)
                {
                    Plat(c, x + 3 + i * 3, b - 2 - (i % 3), 3);
                    if (i % 2 == 0) AddI(c, "soul", x + 4 + i * 3, b - 3 - (i % 3));
                }
                for (int i = x + 3; i < x + 21; ++i) { FillDown(c, i, b + 4, T::Solid); Set(c, i, b + 3, Sting(c.World)); }
                AddE(c, flyerOrE1, x + 10, b - 6);
                Ground(c, x + 21, x + 24, [b](int) { return b; }, T::Solid); c.Cursor += 24;
            }
            else if (beat == "rooms")
            {
                Ground(c, x, x + 26, [b](int) { return b; }, T::Solid); Solid(c, x, b - 8, 1, 6); Solid(c, x + 25, b - 8, 1, 6);
                Plat(c, x + 1, b - 8, 24); Plat(c, x + 4, b - 4, 6); Plat(c, x + 14, b - 4, 6);
                for (int y = b - 7; y < b; ++y) Set(c, x + 12, y, T::Ladder);
                AddE(c, c.World.Enemies[1], x + 6, b - 4); AddE(c, c.World.Enemies[0], x + 18, b);
                AddI(c, "heart", x + 20, b - 5); AddI(c, "soul", x + 6, b - 9); c.Cursor += 26;
            }
            else if (beat == "stairs")
            {
                Ground(c, x, x + 18, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 6; ++i) Solid(c, x + 3 + i * 2, b - 1 - i, 2, 1 + i);
                AddE(c, c.World.Enemies[0], x + 14, b - 7); AddI(c, "soul", x + 15, b - 8); c.Cursor += 18;
            }
            else if (beat == "attic")
            {
                Ground(c, x, x + 20, [b](int) { return b; }, T::Solid); Plat(c, x + 2, b - 4, 16); Plat(c, x + 1, b - 8, 18);
                for (int y = b - 8; y < b; ++y) Set(c, x + 3, y, T::Ladder);
                AddE(c, c.World.Enemies[1], x + 10, b - 5); AddE(c, c.World.Enemies[0], x + 14, b);
                AddI(c, "soul", x + 15, b - 10); c.Cursor += 20;
            }
            else if (beat == "water")
            {
                int fl = c.World.Hazard == "lava" ? T::Lava : T::Water;
                Ground(c, x, x + 4, [b](int) { return b; }, T::Solid);
                for (int i = x + 4; i < x + 18; ++i) { FillDown(c, i, b + 3, T::Solid); for (int y = b; y < b + 3; ++y) Set(c, i, y, fl); }
                Plat(c, x + 6, b - 1, 3); Plat(c, x + 11, b - 2, 3);
                AddE(c, flyerOrE1, x + 10, b - 5);
                AddI(c, "soul", x + 12, b - 4);
                Ground(c, x + 18, x + 22, [b](int) { return b; }, T::Solid); c.Cursor += 22;
            }
            else if (beat == "isles")
            {
                Ground(c, x, x + 3, [b](int) { return b; }, T::Solid);
                int ys[5] = {b - 1, b - 3, b - 2, b - 5, b - 3};
                for (int i = 0; i < 5; ++i) { Plat(c, x + 4 + i * 4, ys[i], 3); if (i % 2 == 1) AddI(c, "soul", x + 5 + i * 4, ys[i] - 1); }
                for (int i = x + 3; i < x + 24; ++i) { FillDown(c, i, b + 4, T::Solid); Set(c, i, b + 3, Sting(c.World)); }
                AddE(c, flyerOrE1, x + 16, b - 6);
                Ground(c, x + 24, x + 28, [b](int) { return b; }, T::Solid); c.Cursor += 28;
            }
            else if (beat == "dunes")
            {
                Ground(c, x, x + 28, [x, b](int xx) {
                    return b - RoundToInt(std::sin((xx - x) * 0.22f) * 2.4f);
                }, T::Solid);
                AddE(c, c.World.Enemies[1], x + 8, b); AddE(c, c.World.Enemies[0], x + 16, b); AddE(c, c.World.Enemies[1], x + 22, b);
                AddI(c, "soul", x + 13, b - 5); c.Cursor += 28;
            }
            else if (beat == "ruins")
            {
                Ground(c, x, x + 22, [b](int) { return b; }, T::Solid); Solid(c, x + 4, b - 3, 2, 3); Solid(c, x + 10, b - 4, 2, 4); Solid(c, x + 16, b - 2, 3, 2); Plat(c, x + 6, b - 6, 5);
                AddE(c, c.World.Enemies[0], x + 12, b); AddI(c, "soul", x + 8, b - 7);
                c.Decor.push_back({"arch", static_cast<float>((x + 13) * T::Tile), static_cast<float>(b * T::Tile), 1.4f}); c.Cursor += 22;
            }
            else if (beat == "tombs")
            {
                Ground(c, x, x + 24, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 5; ++i)
                {
                    Solid(c, x + 3 + i * 4, b - 2, 1, 2); Plat(c, x + 2 + i * 4, b - 3, 3);
                    c.Decor.push_back({"tomb", static_cast<float>((x + 5 + i * 4) * T::Tile), static_cast<float>(b * T::Tile), 1.0f});
                }
                AddE(c, c.World.Enemies[0], x + 8, b); AddE(c, c.World.Enemies[1], x + 16, b); AddI(c, "heart", x + 11, b - 4); c.Cursor += 24;
            }
            else if (beat == "crypt")
            {
                Ground(c, x, x + 6, [b](int) { return b; }, T::Solid); int dip = b + 3;
                for (int i = x + 6; i < x + 20; ++i) FillDown(c, i, dip, T::Solid);
                Plat(c, x + 5, b, 2); Plat(c, x + 19, b, 2);
                Plat(c, x + 9, b + 1, 3); Plat(c, x + 15, b + 1, 3);
                AddE(c, c.World.Enemies[1], x + 10, dip); AddE(c, c.World.Enemies[0], x + 15, dip);
                AddI(c, "soul", x + 12, dip - 1);
                Ground(c, x + 20, x + 24, [b](int) { return b; }, T::Solid); c.Cursor += 24;
            }
            else if (beat == "hall")
            {
                Ground(c, x, x + 28, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 4; ++i)
                {
                    Solid(c, x + 5 + i * 6, b - 4, 1, 4); Plat(c, x + 4 + i * 6, b - 4, 3);
                    Plat(c, x + 4 + i * 6, b - 8, 3);
                    c.Decor.push_back({"torch", static_cast<float>((x + 5 + i * 6) * T::Tile), static_cast<float>((b - 5) * T::Tile), 1.0f});
                }
                AddE(c, c.World.Enemies[0], x + 8, b); AddE(c, flyerOrE1, x + 16, b - 9); AddE(c, c.World.Enemies[1], x + 22, b);
                AddI(c, "soul", x + 17, b - 5); c.Cursor += 28;
            }
            else if (beat == "towers")
            {
                Ground(c, x, x + 20, [b](int) { return b; }, T::Solid); Solid(c, x + 4, b - 4, 3, 4); Solid(c, x + 13, b - 6, 3, 6);
                Plat(c, x + 7, b - 4, 6); Plat(c, x + 7, b - 8, 6); Plat(c, x + 3, b - 8, 3);
                for (int y = b - 9; y < b; ++y) Set(c, x + 8, y, T::Ladder);
                AddE(c, c.World.Enemies[0], x + 16, b); AddI(c, "soul", x + 9, b - 9); c.Cursor += 20;
            }
            else if (beat == "icefields")
            {
                Ground(c, x, x + 26, [x, b](int xx) {
                    return b - RoundToInt(std::sin((xx - x) * 0.3f));
                }, T::Ice);
                Plat(c, x + 8, b - 4, 3); Plat(c, x + 14, b - 6, 3, T::Bounce);
                AddE(c, c.World.Enemies[0], x + 10, b); AddE(c, c.World.Enemies[1], x + 18, b);
                AddI(c, "soul", x + 15, b - 9); c.Cursor += 26;
            }
            else if (beat == "lava")
            {
                Ground(c, x, x + 3, [b](int) { return b; }, T::Solid);
                for (int i = x + 3; i < x + 20; ++i) { FillDown(c, i, b + 4, T::Solid); Set(c, i, b + 3, T::Lava); Set(c, i, b + 2, T::Lava); }
                Plat(c, x + 4, b - 1, 4); Plat(c, x + 9, b - 3, 4); Plat(c, x + 14, b - 2, 5);
                AddE(c, flyerOrE0, x + 7, b - 7);
                AddI(c, "soul", x + 10, b - 5);
                Ground(c, x + 20, x + 24, [b](int) { return b; }, T::Solid); c.Cursor += 24;
            }
            else if (beat == "fade")
            {
                Ground(c, x, x + 3, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 5; ++i) Plat(c, x + 4 + i * 4, b - 1 - (i % 2), 3);
                for (int i = x + 3; i < x + 24; ++i) { FillDown(c, i, b + 4, T::Solid); Set(c, i, b + 3, Sting(c.World)); }
                AddI(c, "soul", x + 12, b - 3);
                Ground(c, x + 24, x + 28, [b](int) { return b; }, T::Solid); c.Cursor += 28;
            }
            else if (beat == "cliffs")
            {
                Ground(c, x, x + 4, [b](int) { return b; }, T::Solid); Ground(c, x + 4, x + 8, [b](int) { return b - 3; }, T::Solid); Ground(c, x + 8, x + 11, [b](int) { return b - 6; }, T::Solid);
                for (int i = x + 11; i < x + 16; ++i) { FillDown(c, i, b + 1, T::Solid); Set(c, i, b, Sting(c.World)); }
                Plat(c, x + 12, b - 5, 3); Plat(c, x + 12, b - 1, 3);
                Ground(c, x + 16, x + 22, [b](int) { return b - 2; }, T::Solid); AddE(c, c.World.Enemies[1], x + 18, b - 2);
                c.Zones.push_back({"wind", static_cast<float>((x + 11) * T::Tile), static_cast<float>((x + 16) * T::Tile), 1.1f});
                AddI(c, "soul", x + 13, b - 6); c.Cursor += 22;
            }
            else if (beat == "wind")
            {
                Ground(c, x, x + 6, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 4; ++i) Plat(c, x + 7 + i * 4, b - 2 - (i % 3), 3);
                for (int i = x + 6; i < x + 22; ++i) { FillDown(c, i, b + 4, T::Solid); Set(c, i, b + 3, Sting(c.World)); }
                Ground(c, x + 22, x + 26, [b](int) { return b; }, T::Solid); AddE(c, flyerOrE0, x + 14, b - 7);
                c.Zones.push_back({"wind", static_cast<float>((x + 6) * T::Tile), static_cast<float>((x + 22) * T::Tile), 1.4f});
                AddI(c, "soul", x + 18, b - 6); c.Cursor += 26;
            }
            else if (beat == "cave")
            {
                Ground(c, x, x + 26, [b](int) { return b; }, T::Solid);
                for (int i = x; i < x + 26; ++i) { int ceil = 2 + RoundToInt(std::sin((i - x) * 0.4f) + 1.0f); for (int y = 0; y < ceil; ++y) Set(c, i, y, T::Solid); }
                Plat(c, x + 6, b - 3, 4); Plat(c, x + 14, b - 4, 4);
                AddE(c, c.World.Enemies[0], x + 8, b); AddE(c, c.World.Enemies[1], x + 18, b);
                AddI(c, "soul", x + 15, b - 5); c.Cursor += 26;
            }
            else if (beat == "roofs")
            {
                Ground(c, x, x + 26, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 4; ++i)
                {
                    int bx = x + 2 + i * 6; Solid(c, bx, b - 4, 4, 4); Plat(c, bx - 1, b - 5, 6);
                    c.Decor.push_back({"lamp", static_cast<float>((bx + 4) * T::Tile), static_cast<float>(b * T::Tile), 1.0f});
                }
                AddE(c, c.World.Enemies[0], x + 10, b - 5); AddE(c, c.World.Enemies[1], x + 20, b);
                AddI(c, "soul", x + 14, b - 7); c.Cursor += 26;
            }
            else if (beat == "street")
            {
                Ground(c, x, x + 24, [b](int) { return b; }, T::Solid); AddE(c, c.World.Enemies[0], x + 6, b); AddE(c, c.World.Enemies[1], x + 12, b); AddE(c, c.World.Enemies[0], x + 18, b);
                Plat(c, x + 8, b - 3, 4); AddI(c, "soul", x + 9, b - 4); c.Cursor += 24;
            }
            else if (beat == "nave")
            {
                Ground(c, x, x + 24, [b](int) { return b; }, T::Solid);
                for (int i = 0; i < 3; ++i)
                {
                    int colX = x + 4 + i * 7;
                    Solid(c, colX, 2, 1, 8); Plat(c, colX - 1, b - 4, 3);
                    c.Decor.push_back({"torch", static_cast<float>(colX * T::Tile), static_cast<float>((b - 5) * T::Tile), 1.1f});
                }
                Plat(c, x + 8, b - 8, 5);
                AddE(c, flyerOrE1, x + 8, b - 9); AddE(c, c.World.Enemies[0], x + 16, b);
                AddI(c, "soul", x + 10, b - 9); c.Cursor += 24;
            }
            else if (beat == "gauntlet")
            {
                Ground(c, x, x + 26, [b](int) { return b; }, T::Solid); Plat(c, x + 6, b - 3, 4); Plat(c, x + 14, b - 4, 4);
                AddE(c, c.World.Enemies[0], x + 5, b); AddE(c, c.World.Enemies[1], x + 9, b - 3); AddE(c, c.World.Enemies[0], x + 13, b);
                AddE(c, flyerOrE1, x + 17, b - 6); AddE(c, c.World.Enemies[1], x + 21, b);
                AddI(c, "heart", x + 16, b - 5); c.Cursor += 26;
            }
            else if (beat == "check")
            {
                Ground(c, x, x + 12, [b](int) { return b; }, T::Solid);
                c.Check = {static_cast<float>((x + 6) * T::Tile), static_cast<float>((b - 2) * T::Tile)}; c.HasCheck = true;
                AddI(c, "heart", x + 8, b - 1);
                c.Decor.push_back({"shrine", static_cast<float>((x + 6) * T::Tile), static_cast<float>(b * T::Tile), 1.2f});
                c.Cursor += 12;
            }
            else if (beat == "finalgate")
            {
                Ground(c, x, x + 20, [b](int) { return b; }, T::Solid); AddE(c, c.World.Enemies[0], x + 6, b); AddE(c, c.World.Enemies[1], x + 10, b);
                AddE(c, flyerOrE1, x + 14, b - 5); AddI(c, "heart", x + 16, b - 1);
                c.Decor.push_back({"arch", static_cast<float>((x + 18) * T::Tile), static_cast<float>(b * T::Tile), 1.6f}); c.Cursor += 20;
            }
            else
            {
                Ground(c, x, x + 20, [b](int) { return b; }, T::Solid); c.Cursor += 20;
            }
        }

        void StampArena(BuilderCtx& c)
        {
            int x = c.Cursor, w = 32, b = c.Base;
            Ground(c, x, x + w, [b](int) { return b; }, T::Solid);
            Solid(c, x, b - 8, 1, 5); Solid(c, x + w - 1, b - 8, 1, 5);
            Plat(c, x + 4, b - 4, 4); Plat(c, x + w - 8, b - 4, 4);
            Plat(c, x + 13, b - 6, 6);
            c.BossAt = {static_cast<float>((x + w - 10) * T::Tile), static_cast<float>(b * T::Tile)};
            c.Exit = {static_cast<float>((x + w - 4) * T::Tile), static_cast<float>((b - 2) * T::Tile)};
            c.ArenaX0 = static_cast<float>(x * T::Tile);
            c.ArenaX1 = static_cast<float>((x + w) * T::Tile);
            c.Decor.push_back({"arena", static_cast<float>((x + w / 2) * T::Tile), static_cast<float>(b * T::Tile), 2.0f});
            c.Decor.push_back({"torch", static_cast<float>((x + 2) * T::Tile), static_cast<float>((b - 6) * T::Tile), 1.2f});
            c.Decor.push_back({"torch", static_cast<float>((x + w - 3) * T::Tile), static_cast<float>((b - 6) * T::Tile), 1.2f});
            c.Cursor += w;
        }

        std::vector<std::vector<int>> Stands(const BuilderCtx& c)
        {
            std::vector<std::vector<int>> stand(c.Cols);
            for (int x = 0; x < c.Cols; ++x)
            {
                for (int y = 1; y < c.Rows; ++y)
                    if (LevelBuilder::Walk(Get(c, x, y)) && !LevelBuilder::Walk(Get(c, x, y - 1)))
                        stand[x].push_back(y);
            }
            return stand;
        }

        std::vector<int> Skyline(const BuilderCtx& c, const std::vector<std::vector<int>>& stand)
        {
            std::vector<int> top(c.Cols, c.Rows);
            for (int x = 0; x < c.Cols; ++x)
                top[x] = !stand[x].empty() ? stand[x][0] : c.Rows;
            return top;
        }

        bool ClearSky(const std::vector<int>& top, int x, int nx, int y, int ny)
        {
            int lo = std::min(y, ny) - kJumpUp;
            int a = std::min(x, nx) + 1, b = std::max(x, nx);
            for (int cx = a; cx < b; ++cx) if (top[cx] < lo) return false;
            return true;
        }

        bool HasLadderCtx(const BuilderCtx& c, int x)
        {
            for (int y = 0; y < c.Rows; ++y) if (Get(c, x, y) == T::Ladder) return true;
            return false;
        }

        std::unordered_set<int> Flood(const BuilderCtx& c, const std::vector<std::vector<int>>& stand)
        {
            auto top = Skyline(c, stand);
            int sx = ClampI(RoundToInt(c.Spawn.x / T::Tile), 0, c.Cols - 1);
            std::unordered_set<int> seen;
            std::queue<int> queue;
            for (int d = 0; d <= 4 && seen.empty(); ++d)
            {
                for (int probe : {sx - d, sx + d})
                {
                    if (probe >= 0 && probe < c.Cols)
                    {
                        for (int y : stand[probe])
                            if (seen.insert(probe * 64 + y).second) queue.push(probe * 64 + y);
                    }
                }
            }

            while (!queue.empty())
            {
                int node = queue.front(); queue.pop();
                int x = node / 64, y = node % 64;
                for (int dx = -kReach - 1; dx <= kReach + 1; ++dx)
                {
                    int nx = x + dx;
                    if (nx < 0 || nx >= c.Cols) continue;
                    bool ladder = HasLadderCtx(c, x) && HasLadderCtx(c, nx);
                    for (int ny : stand[nx])
                    {
                        int climb = y - ny;
                        bool ok = ladder
                            ? std::abs(dx) <= 2
                            : (climb <= kJumpUp && std::abs(dx) <= (climb > 0 ? kReach : kReach + 1));
                        if (!ok) continue;
                        if (!ladder && !ClearSky(top, x, nx, y, ny)) continue;
                        int key = nx * 64 + ny;
                        if (seen.insert(key).second) queue.push(key);
                    }
                }
            }
            return seen;
        }

        bool Reached(const std::unordered_set<int>& seen, const std::vector<std::vector<int>>& stand, int x)
        {
            if (x < 0 || x >= static_cast<int>(stand.size())) return false;
            for (int y : stand[x]) if (seen.count(x * 64 + y) > 0) return true;
            return false;
        }

        bool Ramp(BuilderCtx& c, const std::vector<std::vector<int>>& stand, const std::unordered_set<int>& seen, int goal)
        {
            int blocked = -1;
            for (int x = 0; x <= goal; ++x)
                if (!stand[x].empty() && !Reached(seen, stand, x)) { blocked = x; break; }
            if (blocked < 0) return false;

            int from = -1;
            for (int x = blocked - 1; x >= 0; --x) if (Reached(seen, stand, x)) { from = x; break; }
            if (from < 0) return false;

            int yFrom = -1;
            for (int y : stand[from]) if (seen.count(from * 64 + y) > 0 && y > yFrom) yFrom = y;
            int yTo = std::numeric_limits<int>::max();
            for (int y : stand[blocked]) if (y < yTo) yTo = y;
            if (yFrom < 0 || yTo == std::numeric_limits<int>::max() || yTo >= yFrom) return false;

            bool placed = false;
            int step = yFrom - 3;
            while (step - yTo > kJumpUp - 1 && step > 1)
            {
                if (Get(c, from, step) == T::Empty) { Set(c, from, step, T::Platform); placed = true; }
                step -= 3;
            }
            if (step > yTo && step > 1 && Get(c, from, step) == T::Empty) { Set(c, from, step, T::Platform); placed = true; }
            return placed;
        }

        void SealUnreachable(BuilderCtx& c)
        {
            int goal = ClampI(RoundToInt(c.Exit.x / T::Tile), 0, c.Cols - 1);
            std::vector<std::vector<int>> stand;
            std::unordered_set<int> seen;

            for (int pass = 0; pass < 40; ++pass)
            {
                stand = Stands(c);
                seen = Flood(c, stand);
                if (Reached(seen, stand, goal)) break;
                if (!Ramp(c, stand, seen, goal)) break;
                c.Sealed += 1000;
            }

            int sting = Sting(c.World);
            int arena0 = FloorToInt(c.ArenaX0 / T::Tile);
            int arena1 = CeilToInt(c.ArenaX1 / T::Tile);
            for (int x = 0; x < c.Cols; ++x)
            {
                if (x >= arena0 && x <= arena1) continue;
                for (int y : stand[x])
                {
                    if (seen.count(x * 64 + y) > 0) continue;
                    if (y <= c.Base) continue;
                    if (y >= c.Rows - 1) continue;
                    if (Get(c, x, y) == T::Platform) continue;
                    Set(c, x, y, sting);
                    c.Sealed++;
                }
            }
        }

        void ScatterDecor(BuilderCtx& c, int worldIndex)
        {
            std::string kind = "rock";
            const std::string& id = c.World.Id;
            if (id == "forest" || id == "woods") kind = "tree";
            else if (id == "cabin" || id == "village") kind = "lamp";
            else if (id == "swamp" || id == "catacombs") kind = "reed";
            else if (id == "desert") kind = "obelisk";
            else if (id == "grave") kind = "tomb";
            else if (id == "castle" || id == "cathedral" || id == "throne") kind = "pillar";
            else if (id == "ice" || id == "abyss") kind = "crystal";
            else if (id == "volcano" || id == "peak") kind = "rock";
            else if (id == "coven") kind = "candle";

            for (int x = 3; x < c.Cols - 3; x += 5)
            {
                float h = LevelBuilder::ArtHash(static_cast<float>(x), static_cast<float>(worldIndex * 7), 3.7f);
                if (h < 0.45f) continue;
                int surf = -1;
                for (int y = 1; y < c.Rows; ++y)
                    if (LevelBuilder::Walk(Get(c, x, y)) && !LevelBuilder::Walk(Get(c, x, y - 1))) { surf = y; break; }
                if (surf < 2 || surf > c.Base + 1) continue;
                if (x * T::Tile > c.ArenaX0 - 80.0f) continue;
                c.Decor.push_back({kind, static_cast<float>(x * T::Tile + 20), static_cast<float>(surf * T::Tile), 0.75f + h * 0.85f});
            }
        }
    }

    float LevelBuilder::ArtHash(float x, float y, float s)
    {
        float v = std::sin(x * 127.1f + y * 311.7f + s * 74.7f) * 43758.5453f;
        return v - std::floor(v);
    }

    bool LevelBuilder::Walk(int t)
    {
        return t == T::Solid || t == T::Ice || t == T::Break || t == T::Platform || t == T::Bounce;
    }

    LevelData LevelBuilder::Compile(int worldIndex)
    {
        const auto& worlds = Catalog::Worlds();
        int idx = ClampI(worldIndex, 0, static_cast<int>(worlds.size()) - 1);
        const auto& world = worlds[idx];
        BuilderCtx c;
        c.World = world;
        c.Rows = 20;
        c.Cols = world.Cols + 40;
        c.Cursor = 2;
        c.Base = 15;
        c.Grid.assign(c.Rows, std::vector<uint8_t>(c.Cols, 0));
        Ground(c, 0, 2, [&c](int) { return c.Base; }, T::Solid);
        for (const auto& beat : world.Beats) RunBeat(c, beat);
        StampArena(c);
        for (int x = 0; x < c.Cols; ++x) if (c.Grid[c.Rows - 1][x] == 0) c.Grid[c.Rows - 1][x] = T::Solid;
        SealUnreachable(c);
        ScatterDecor(c, idx);

        LevelData out;
        out.WorldIndex = idx;
        out.World = world;
        out.Cols = c.Cols;
        out.Rows = c.Rows;
        out.Tiles = std::move(c.Grid);
        out.Spawn = c.Spawn.SqrMagnitude() > 0.0f ? c.Spawn : Vec2{80.0f, 200.0f};
        out.HasCheck = c.HasCheck;
        out.Check = c.Check;
        out.Exit = c.Exit;
        out.BossAt = c.BossAt;
        out.ArenaX0 = c.ArenaX0;
        out.ArenaX1 = c.ArenaX1;
        out.Ents = std::move(c.Ents);
        out.Items = std::move(c.Items);
        out.Zones = std::move(c.Zones);
        out.Decor = std::move(c.Decor);
        out.SealedPockets = c.Sealed;
        return out;
    }

    std::unordered_set<int> LevelBuilder::Reach(const LevelData& L)
    {
        std::vector<std::vector<int>> stand(L.Cols);
        for (int x = 0; x < L.Cols; ++x)
        {
            for (int y = 1; y < L.Rows; ++y)
                if (Walk(L.Tiles[y][x]) && !Walk(L.Tiles[y - 1][x])) stand[x].push_back(y);
        }
        std::vector<int> top(L.Cols, L.Rows);
        for (int x = 0; x < L.Cols; ++x) top[x] = !stand[x].empty() ? stand[x][0] : L.Rows;
        int sx = ClampI(RoundToInt(L.Spawn.x / T::Tile), 0, L.Cols - 1);
        std::unordered_set<int> seen;
        std::queue<int> q;
        for (int d = 0; d <= 4 && seen.empty(); ++d)
        {
            for (int probe : {sx - d, sx + d})
            {
                if (probe >= 0 && probe < L.Cols)
                    for (int y : stand[probe]) if (seen.insert(probe * 64 + y).second) q.push(probe * 64 + y);
            }
        }
        auto hasLadder = [&L](int x) {
            for (int y = 0; y < L.Rows; ++y) if (L.Tiles[y][x] == T::Ladder) return true;
            return false;
        };
        while (!q.empty())
        {
            int node = q.front(); q.pop();
            int x = node / 64, y = node % 64;
            for (int dx = -5; dx <= 5; ++dx)
            {
                int nx = x + dx;
                if (nx < 0 || nx >= L.Cols) continue;
                bool ladder = hasLadder(x) && hasLadder(nx);
                for (int ny : stand[nx])
                {
                    int climb = y - ny;
                    bool ok = ladder ? std::abs(dx) <= 2 : (climb <= 5 && std::abs(dx) <= (climb > 0 ? 4 : 5));
                    if (!ok) continue;
                    if (!ladder && !ClearSky(top, x, nx, y, ny)) continue;
                    int key = nx * 64 + ny;
                    if (seen.insert(key).second) q.push(key);
                }
            }
        }
        return seen;
    }

    bool LevelBuilder::Walkable(const LevelData& L, std::string& why)
    {
        auto seen = Reach(L);
        int gx = ClampI(RoundToInt(L.Exit.x / T::Tile), 0, L.Cols - 1);
        for (int y = 1; y < L.Rows; ++y)
            if (seen.count(gx * 64 + y) > 0) { why.clear(); return true; }
        for (int dx = -2; dx <= 2; ++dx)
        {
            int x = ClampI(gx + dx, 0, L.Cols - 1);
            for (int y = 1; y < L.Rows; ++y)
                if (seen.count(x * 64 + y) > 0) { why.clear(); return true; }
        }
        why = "sino em " + std::to_string(gx) + " fora de alcance do spawn";
        return false;
    }

    // ───────────────────────────── Progress ─────────────────────────────

    int Progress::Unlocked = 1;
    std::string Progress::Hero = "mike";
    int Progress::Souls = 0;
    int Progress::Score = 0;
    int Progress::LastWorld = 0;

    namespace { std::string g_ProgressPath = "nightfall_progress.cfg"; }

    void Progress::SetStoragePath(const std::string& path)
    {
        if (!path.empty()) g_ProgressPath = path;
    }

    void Progress::Load()
    {
        std::ifstream f(g_ProgressPath);
        if (f.is_open())
        {
            f >> Unlocked >> Hero >> Souls >> Score >> LastWorld;
        }
        Unlocked = ClampI(Unlocked, 1, WorldCount);
        if (Hero != "mike" && Hero != "denyse") Hero = "mike";
        Souls = std::max(0, Souls);
        Score = std::max(0, Score);
        LastWorld = ClampI(LastWorld, 0, Unlocked - 1);
    }

    void Progress::Save()
    {
        std::ofstream f(g_ProgressPath, std::ios::trunc);
        if (f.is_open())
        {
            f << Unlocked << " " << (Hero.empty() ? "mike" : Hero) << " " << Souls << " " << Score << " " << LastWorld << "\n";
        }
    }

    bool Progress::CanPlay(int worldIndex)
    {
        return worldIndex >= 0 && worldIndex < Unlocked && worldIndex < WorldCount;
    }

    void Progress::OnClear(int worldIndex)
    {
        if (worldIndex < 0) return;
        Unlocked = std::max(Unlocked, std::min(WorldCount, worldIndex + 2));
        LastWorld = ClampI(worldIndex + 1, 0, WorldCount - 1);
        Save();
    }

    void Progress::RememberRun(int souls, int score, const std::string& hero)
    {
        Souls = souls;
        Score = score;
        if (hero == "mike" || hero == "denyse") Hero = hero;
        Save();
    }

    void Progress::ResetAll()
    {
        Unlocked = 1;
        Hero = "mike";
        Souls = 0;
        Score = 0;
        LastWorld = 0;
        Save();
    }

    // ───────────────────────────── LanIp ─────────────────────────────

    int LanIp::ScoreAddressBytes(uint8_t b0, uint8_t b1, uint8_t /*b2*/, uint8_t /*b3*/)
    {
        if (b0 == 0 || b0 == 127 || b0 == 255) return -1;
        if (b0 == 169 && b1 == 254) return 1;
        if (b0 == 192 && b1 == 168) return 40;
        if (b0 == 10) return 30;
        if (b0 == 172 && b1 >= 16 && b1 <= 31) return 28;
        return 10;
    }

    int LanIp::ScoreAddress(const std::string& ip)
    {
        if (ip.empty()) return -1;
        int a = -1, b = -1, c = -1, d = -1;
        char dot1 = 0, dot2 = 0, dot3 = 0;
        std::stringstream ss(ip);
        if (!(ss >> a >> dot1 >> b >> dot2 >> c >> dot3 >> d)) return -1;
        if (dot1 != '.' || dot2 != '.' || dot3 != '.') return -1;
        if (!ss.eof()) return -1;
        if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) return -1;
        return ScoreAddressBytes(static_cast<uint8_t>(a), static_cast<uint8_t>(b), static_cast<uint8_t>(c), static_cast<uint8_t>(d));
    }

    bool LanIp::IsUsable(const std::string& host)
    {
        return ScoreAddress(host) >= 10;
    }

    int LanIp::ScoreIface(const std::string& nameAndDesc)
    {
        std::string n = nameAndDesc;
        std::transform(n.begin(), n.end(), n.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (n.find("loopback") != std::string::npos || n.find("vmware") != std::string::npos ||
            n.find("virtualbox") != std::string::npos || n.find("hyper-v") != std::string::npos ||
            n.find("vethernet") != std::string::npos) return -1;
        if (n.find("rmnet") != std::string::npos || n.find("wwan") != std::string::npos ||
            n.find("cellular") != std::string::npos || n.find("mobile") != std::string::npos) return 0;
        if (n.find("wlan") != std::string::npos || n.find("wifi") != std::string::npos ||
            n.find("wi-fi") != std::string::npos || n.find("p2p") != std::string::npos) return 20;
        return 5;
    }


    // ───────────────────────────── FrameSnap ─────────────────────────────

    namespace
    {
        template <typename TVal>
        void WriteVal(std::vector<uint8_t>& buf, const TVal& v)
        {
            static_assert(std::is_arithmetic_v<TVal>, "Wire values must be explicit scalar fields");
            std::array<uint8_t,sizeof(TVal)> bytes{};
            std::memcpy(bytes.data(),&v,sizeof(v));
            if constexpr(std::endian::native==std::endian::big) std::reverse(bytes.begin(),bytes.end());
            buf.insert(buf.end(),bytes.begin(),bytes.end());
        }

        void WriteStr(std::vector<uint8_t>& buf, const std::string& s, size_t maxLen)
        {
            uint8_t len = static_cast<uint8_t>(std::min(s.size(), maxLen));
            buf.push_back(len);
            buf.insert(buf.end(), s.begin(), s.begin() + len);
        }

        template <typename TVal>
        bool ReadVal(const uint8_t*& cur, const uint8_t* end, TVal& v)
        {
            static_assert(std::is_arithmetic_v<TVal>, "Wire values must be explicit scalar fields");
            if (static_cast<size_t>(end - cur) < sizeof(TVal)) return false;
            if constexpr(std::is_same_v<TVal,bool>) {
                if(*cur>1)return false;
                v=*cur!=0;
            } else {
                std::array<uint8_t,sizeof(TVal)> bytes{};
                std::copy(cur,cur+sizeof(TVal),bytes.begin());
                if constexpr(std::endian::native==std::endian::big) std::reverse(bytes.begin(),bytes.end());
                std::memcpy(&v,bytes.data(),sizeof(TVal));
                if constexpr(std::is_floating_point_v<TVal>) if(!std::isfinite(v)) return false;
            }
            cur += sizeof(TVal);
            return true;
        }

        void WriteVal(std::vector<uint8_t>& b,const Vec2& v) { WriteVal(b,v.x);WriteVal(b,v.y); }
        bool ReadVal(const uint8_t*& c,const uint8_t* e,Vec2& v) { return ReadVal(c,e,v.x)&&ReadVal(c,e,v.y); }
        void WriteVal(std::vector<uint8_t>& b,const PlrSnap& v) {
            WriteVal(b,v.X);WriteVal(b,v.Y);WriteVal(b,v.Hp);WriteVal(b,v.MaxHp);WriteVal(b,v.Lives);WriteVal(b,v.Anim);WriteVal(b,v.Atk);WriteVal(b,v.Flags);WriteVal(b,v.Hero);
        }
        bool ReadVal(const uint8_t*& c,const uint8_t* e,PlrSnap& v) {
            return ReadVal(c,e,v.X)&&ReadVal(c,e,v.Y)&&ReadVal(c,e,v.Hp)&&ReadVal(c,e,v.MaxHp)&&ReadVal(c,e,v.Lives)&&ReadVal(c,e,v.Anim)&&ReadVal(c,e,v.Atk)&&ReadVal(c,e,v.Flags)&&ReadVal(c,e,v.Hero);
        }
        void WriteVal(std::vector<uint8_t>& b,const EntSnap& v) { WriteVal(b,v.X);WriteVal(b,v.Y);WriteVal(b,v.Hp);WriteVal(b,v.Kind);WriteVal(b,v.Flags); }
        bool ReadVal(const uint8_t*& c,const uint8_t* e,EntSnap& v) { return ReadVal(c,e,v.X)&&ReadVal(c,e,v.Y)&&ReadVal(c,e,v.Hp)&&ReadVal(c,e,v.Kind)&&ReadVal(c,e,v.Flags); }
        void WriteVal(std::vector<uint8_t>& b,const ProjSnap& v) { WriteVal(b,v.X);WriteVal(b,v.Y);WriteVal(b,v.Vx);WriteVal(b,v.Vy);WriteVal(b,v.Kind);WriteVal(b,v.Flags); }
        bool ReadVal(const uint8_t*& c,const uint8_t* e,ProjSnap& v) { return ReadVal(c,e,v.X)&&ReadVal(c,e,v.Y)&&ReadVal(c,e,v.Vx)&&ReadVal(c,e,v.Vy)&&ReadVal(c,e,v.Kind)&&ReadVal(c,e,v.Flags); }
        void WriteVal(std::vector<uint8_t>& b,const FxSnap& v) { WriteVal(b,v.X);WriteVal(b,v.Y);WriteVal(b,v.Kind);WriteVal(b,v.Dir); }
        bool ReadVal(const uint8_t*& c,const uint8_t* e,FxSnap& v) { return ReadVal(c,e,v.X)&&ReadVal(c,e,v.Y)&&ReadVal(c,e,v.Kind)&&ReadVal(c,e,v.Dir); }

        bool ReadStr(const uint8_t*& cur, const uint8_t* end, std::string& s)
        {
            if (cur >= end) return false;
            uint8_t len = *cur++;
            if (static_cast<size_t>(end - cur) < len) return false;
            s.assign(reinterpret_cast<const char*>(cur), len);
            cur += len;
            return true;
        }
    }

    std::vector<uint8_t> FrameSnap::Serialize() const
    {
        std::vector<uint8_t> buf;
        buf.reserve(512);
        buf.push_back(3); // Native wire v3: little-endian scalars, never C++ struct padding.
        WriteVal(buf, P0);
        WriteVal(buf, P1);
        WriteVal(buf, Score);
        WriteVal(buf, Souls);
        WriteVal(buf, LockArena);
        WriteStr(buf, SimState, 32);
        WriteVal(buf, BossHp);
        WriteVal(buf, BossMax);
        WriteVal(buf, BossKind);
        WriteVal(buf, BossFlags);
        WriteVal(buf, BossPos);
        WriteVal(buf, ItemMask);
        WriteVal(buf, Shake);
        WriteStr(buf, Msg, 64);
        WriteVal(buf, MsgT);

        int ne = std::min(static_cast<int>(Ents.size()), MaxEnts);
        WriteVal(buf, ne);
        for (int i = 0; i < ne; ++i) WriteVal(buf, Ents[i]);

        int np = std::min(static_cast<int>(Projs.size()), MaxProjs);
        WriteVal(buf, np);
        for (int i = 0; i < np; ++i) WriteVal(buf, Projs[i]);

        int nf = std::min(static_cast<int>(Fxs.size()), MaxFx);
        WriteVal(buf, nf);
        for (int i = 0; i < nf; ++i) WriteVal(buf, Fxs[i]);
        return buf;
    }

    bool FrameSnap::Deserialize(const uint8_t* data, size_t len, FrameSnap& out)
    {
        if(!data || len<1 || data[0]!=3) return false;
        const uint8_t* cur = data+1;
        const uint8_t* end = data + len;
        if (!ReadVal(cur, end, out.P0)) return false;
        if (!ReadVal(cur, end, out.P1)) return false;
        if (!ReadVal(cur, end, out.Score)) return false;
        if (!ReadVal(cur, end, out.Souls)) return false;
        if (!ReadVal(cur, end, out.LockArena)) return false;
        if (!ReadStr(cur, end, out.SimState)) return false;
        if (!ReadVal(cur, end, out.BossHp)) return false;
        if (!ReadVal(cur, end, out.BossMax)) return false;
        if (!ReadVal(cur, end, out.BossKind)) return false;
        if (!ReadVal(cur, end, out.BossFlags)) return false;
        if (!ReadVal(cur, end, out.BossPos)) return false;
        if (!ReadVal(cur, end, out.ItemMask)) return false;
        if (!ReadVal(cur, end, out.Shake)) return false;
        if (!ReadStr(cur, end, out.Msg)) return false;
        if (!ReadVal(cur, end, out.MsgT)) return false;

        int ne = 0;
        if (!ReadVal(cur, end, ne) || ne < 0 || ne > MaxEnts) return false;
        out.Ents.resize(ne);
        for (int i = 0; i < ne; ++i) if (!ReadVal(cur, end, out.Ents[i])) return false;

        int np = 0;
        if (!ReadVal(cur, end, np) || np < 0 || np > MaxProjs) return false;
        out.Projs.resize(np);
        for (int i = 0; i < np; ++i) if (!ReadVal(cur, end, out.Projs[i])) return false;

        int nf = 0;
        if (!ReadVal(cur, end, nf) || nf < 0 || nf > MaxFx) return false;
        out.Fxs.resize(nf);
        for (int i = 0; i < nf; ++i) if (!ReadVal(cur, end, out.Fxs[i])) return false;
        return cur==end;
    }

    // ───────────────────────────── GameSim ─────────────────────────────

    void GameSim::InitRandomState(uint32_t seed)
    {
        g_RngState = seed == 0 ? 1u : seed;
    }

    float GameSim::NextRandom01()
    {
        uint32_t x = g_RngState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        g_RngState = x;
        return static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    int GameSim::NextRandomRange(int minInclusive, int maxExclusive)
    {
        if (maxExclusive <= minInclusive) return minInclusive;
        uint32_t span = static_cast<uint32_t>(maxExclusive - minInclusive);
        float r = NextRandom01();
        int idx = static_cast<int>(r * span);
        if (idx >= static_cast<int>(span)) idx = static_cast<int>(span) - 1;
        return minInclusive + idx;
    }

    void GameSim::Emit(FxKind kind, float x, float y, int dir)
    {
        if (Fx.size() > 64) return;
        Fx.push_back({static_cast<uint8_t>(kind), x, y, dir});
    }

    int GameSim::PeekTile(float px, float py) const
    {
        return !HasLevel ? T::Empty : TileAt(px, py);
    }

    int GameSim::TileAt(float px, float py) const
    {
        int tx = FloorToInt(px / T::Tile), ty = FloorToInt(py / T::Tile);
        if (ty >= Level.Rows) return T::Empty;
        if (tx < 0 || ty < 0 || tx >= Level.Cols) return T::Solid;
        return Level.Tiles[ty][tx];
    }

    bool GameSim::SolidAt(float px, float py, bool falling) const
    {
        int t = TileAt(px, py);
        if (t == T::Solid || t == T::Ice || t == T::Break) return true;
        if (t == T::Platform && falling && !_noPlat && ModTile(py) < 12.0f) return true;
        if (t == T::Bounce && falling && ModTile(py) < 14.0f) return true;
        return false;
    }

    bool GameSim::Block(int t) { return t == T::Solid || t == T::Ice || t == T::Break; }
    bool GameSim::Hazard(int t) { return t == T::Spike || t == T::Lava || t == T::Thorn; }
    float GameSim::ModTile(float py)
    {
        float m = std::fmod(py, static_cast<float>(T::Tile));
        return m < 0.0f ? m + T::Tile : m;
    }
    bool GameSim::HazardFloor(int t) { return t == T::Empty || t == T::Spike || t == T::Lava || t == T::Water || t == T::Thorn; }

    void GameSim::Start(int world, const std::string& h1, const std::string& h2, bool two)
    {
        WorldIndex = world;
        Level = LevelBuilder::Compile(world);
        HasLevel = true;
        Enemies.clear(); Bosses.clear(); Projectiles.clear(); Items.clear(); Players.clear();
        Fx.clear(); _spawnQueue.clear();
        LockedArena = BelialPhase = false; BossIntro = 0.0f; Fade = 1.0f; FadeDir = -1.0f; State = "play";
        Score = 0; Souls = 0; Shake = 0.0f; Time = 0.0f; HitStop = 0.0f;

        for (const auto& e : Level.Ents)
        {
            const auto* d = Catalog::Enemy(e.id);
            if (!d) continue;
            EnemyA a;
            a.Id = e.id; a.Def = *d; a.Kind = Catalog::EnemyIndex(e.id);
            a.X = e.x; a.Y = e.y; a.W = static_cast<float>(d->W); a.H = static_cast<float>(d->H);
            a.Hp = d->Hp; a.MaxHp = d->Hp;
            a.Dir = NextRandom01() > 0.5f ? 1 : -1;
            a.HomeX = e.x; a.HomeY = e.y; a.ShotT = 1.0f + NextRandom01();
            a.Facing = a.Dir;
            Lift(a);
            a.HomeY = a.Y;
            Enemies.push_back(a);
        }

        for (const auto& it : Level.Items)
        {
            Items.push_back({it.type, it.x, it.y, NextRandom01() * 4.0f, false});
        }

        Vec2 s = Level.Spawn;
        Players.push_back(MakePlayer(0, h1, s.x, s.y));
        if (two) Players.push_back(MakePlayer(1, h2, s.x + 40.0f, s.y));
        for (auto& p : Players)
        {
            Lift(p);
            p.SafeX = p.X;
            p.SafeY = p.Y;
        }
        CamX = std::max(0.0f, s.x - T::ViewW / 3.0f);
        CamY = std::max(0.0f, s.y - T::ViewH * 0.62f);
        Message = Level.World.Name;
        MessageT = 2.4f;
    }

    PlayerA GameSim::MakePlayer(int slot, const std::string& hero, float x, float y)
    {
        const auto* h = Catalog::Hero(hero);
        if (!h) h = Catalog::Hero("mike");
        PlayerA p;
        p.Slot = slot; p.Hero = *h; p.X = x; p.Y = y; p.W = 30.0f; p.H = 48.0f;
        p.Hp = h->Hp; p.MaxHp = h->Hp;
        p.SpawnX = x; p.SpawnY = y; p.SafeX = x; p.SafeY = y; p.Dir = 1; p.Facing = 1;
        return p;
    }

    void GameSim::Tick(float dt)
    {
        Time += dt;
        Fade = Clamp01(Fade + FadeDir * dt * 1.6f);
        MessageT = std::max(0.0f, MessageT - dt);
        if (State != "play") return;
        if (!HasLevel || Players.empty()) return;
        if (HitStop > 0.0f) { HitStop = std::max(0.0f, HitStop - dt); dt *= 0.2f; }
        if (BossIntro > 0.0f) BossIntro -= dt;
        for (auto& p : Players) if (!p.Dead) UpdatePlayer(p, dt);
        for (auto& e : Enemies) UpdateEnemy(e, dt);
        for (auto& b : Bosses) UpdateBoss(b, dt);
        FlushSpawns();
        UpdateProj(dt);
        UpdateItems(dt);
        MaybeLock();
        CheckClear();
        UpdateCam(dt);
        FlushSpawns();
        bool allDead = true;
        for (const auto& p : Players) if (!p.Dead) { allDead = false; break; }
        if (State == "play" && allDead) { State = "dead"; if (Sfx) Sfx("die"); }
    }

    void GameSim::FlushSpawns()
    {
        if (_spawnQueue.empty()) return;
        for (const auto& b : _spawnQueue) Bosses.push_back(b);
        _spawnQueue.clear();
    }

    Ctl GameSim::Inp(int slot) const
    {
        return ReadCtl ? ReadCtl(slot) : Ctl{};
    }

    void GameSim::UpdatePlayer(PlayerA& p, float dt)
    {
        Ctl ctl = Inp(p.Slot);
        p.Atk = std::max(0.0f, p.Atk - dt);
        p.AtkCd = std::max(0.0f, p.AtkCd - dt);
        p.Inv = std::max(0.0f, p.Inv - dt);
        p.HurtT = std::max(0.0f, p.HurtT - dt);
        p.AnimT += dt;
        p.Coyote = p.OnGround ? 0.12f : p.Coyote - dt;
        if (ctl.Jp) p.JumpBuf = 0.14f; else p.JumpBuf -= dt;
        int ax = (ctl.L ? -1 : 0) + (ctl.R ? 1 : 0);
        if (ax != 0) p.Facing = ax;
        float spd = p.Hero.Speed * (p.Water ? 0.7f : 1.0f);
        bool ice = TileAt(p.X + p.W / 2.0f, p.Y + p.H + 1.0f) == T::Ice;
        float acc = p.OnGround ? (ice ? 6.0f : 18.0f) : 10.0f;
        p.Vx += (ax * spd - p.Vx) * std::min(1.0f, acc * dt);
        for (const auto& z : Level.Zones)
        {
            if (z.type == "wind" && p.X > z.x0 && p.X < z.x1) p.Vx += z.vx * dt * 3.0f;
        }
        bool onLadder = TileAt(p.X + p.W / 2.0f, p.Y + p.H / 2.0f) == T::Ladder;
        if (p.JumpBuf > 0.0f && (p.Coyote > 0.0f || (p.Jumps < 1 && !p.OnGround) || p.Water || onLadder))
        {
            p.Vy = p.Hero.Jump * (p.Water ? 0.62f : 1.0f);
            p.OnGround = false; p.Coyote = 0.0f; p.JumpBuf = 0.0f; p.Jumps++;
            Emit(FxKind::Land, p.X + p.W / 2.0f, p.Y + p.H);
            if (Sfx) Sfx("jump");
        }
        if (p.OnGround) p.Jumps = 0;
        if (!ctl.Jn && p.Vy < -3.0f) p.Vy += 28.0f * dt;
        if (onLadder && (ctl.Jn || ctl.D)) { p.Vy = ctl.D ? 3.0f : -3.4f; p.Jumps = 0; }
        if (p.Hero.Id == "denyse" && !p.OnGround && ctl.Jn && p.Vy > 1.0f) p.Vy = std::min(p.Vy, 3.2f);
        if (ctl.D && !onLadder && p.OnGround && TileAt(p.X + p.W / 2.0f, p.Y + p.H + 2.0f) == T::Platform)
            p.Drop = 0.16f;
        p.Drop = std::max(0.0f, p.Drop - dt);

        bool wasAir = !p.OnGround;
        _noPlat = p.Drop > 0.0f;
        Phys(p, dt, p.Hero.Id == "denyse" ? 23.0f : 26.0f, false);
        _noPlat = false;
        if (wasAir && p.OnGround && p.Vy >= 0.0f) Emit(FxKind::Land, p.X + p.W / 2.0f, p.Y + p.H);
        if (p.OnGround && TileAt(p.X + p.W / 2.0f, p.Y + p.H + 2.0f) == T::Bounce)
        {
            p.Vy = -11.2f;
            p.OnGround = false;
            p.Jumps = 0;
            Emit(FxKind::Land, p.X + p.W / 2.0f, p.Y + p.H);
            if (Sfx) Sfx("jump");
        }
        float maxX = Level.Cols * T::Tile - p.W - 4.0f;
        p.X = ClampF(p.X, 4.0f, std::max(4.0f, maxX));
        if (p.Y < -80.0f) { p.Y = -80.0f; p.Vy = std::max(0.0f, p.Vy); }
        if (p.AtkCd <= 0.0f && ctl.Ap)
        {
            p.Atk = p.Hero.AtkTime; p.AtkCd = p.Hero.AtkCd;
            p.Swing++; p.Fired = false;
            if (p.Hero.AtkKind == "magic")
            {
                Emit(FxKind::Charge, p.X + p.W / 2.0f + p.Facing * 18.0f, p.Y + 18.0f, p.Facing);
            }
            else
            {
                p.Vx += p.Facing * (p.OnGround ? 1.1f : 0.35f);
                if (Sfx) Sfx("attack");
            }
        }
        if (p.Atk > 0.0f)
        {
            float st = 1.0f - p.Atk / std::max(0.01f, p.Hero.AtkTime);
            if (p.Hero.AtkKind == "magic")
            {
                if (!p.Fired && st >= 0.35f) { p.Fired = true; FireMagic(p); }
            }
            else if (st >= 0.12f && st <= 0.88f) MeleeSweep(p, st);
        }
        p.Anim = p.Atk > 0.0f ? "attack" : !p.OnGround ? (p.Vy < 0.0f ? "jump" : "fall") : (std::abs(p.Vx) > 0.4f && ax != 0 ? "walk" : "idle");
        RememberSafe(p, dt);
        int hz = TileAt(p.X + p.W / 2.0f, p.Y + p.H - 4.0f);
        int hzFeet = TileAt(p.X + p.W / 2.0f, p.Y + p.H - 1.0f);
        if (Hazard(hz) || Hazard(hzFeet)) HazardHit(p, (hz == T::Lava || hzFeet == T::Lava) ? 2 : 1);
        if (p.Y > Level.Rows * T::Tile + 20.0f) { HurtDirect(p, 1); Rescue(p); }
        if (Level.HasCheck && std::abs(p.X - Level.Check.x) < 40.0f && std::abs(p.Y - Level.Check.y) < 50.0f)
        {
            p.SpawnX = Level.Check.x;
            p.SpawnY = Level.Check.y - 10.0f;
        }
        if (LockedArena) p.X = ClampF(p.X, Level.ArenaX0 + 8.0f, Level.ArenaX1 - 8.0f - p.W);
    }

    void GameSim::RememberSafe(PlayerA& p, float dt)
    {
        p.SafeT -= dt;
        if (!p.OnGround || p.SafeT > 0.0f) return;
        int under = TileAt(p.X + p.W / 2.0f, p.Y + p.H + 2.0f);
        if (!(under == T::Solid || under == T::Ice || under == T::Platform || under == T::Break)) return;
        int body = TileAt(p.X + p.W / 2.0f, p.Y + p.H - 4.0f);
        int chest = TileAt(p.X + p.W / 2.0f, p.Y + p.H / 2.0f);
        if (Hazard(body) || Hazard(chest) || body == T::Water) return;
        p.SafeX = p.X; p.SafeY = p.Y - 2.0f; p.SafeT = 0.2f;
    }

    void GameSim::HazardHit(PlayerA& p, int dmg)
    {
        if (p.Inv > 0.0f) return;
        HurtDirect(p, dmg);
        if (p.Dead) return;
        Rescue(p);
    }

    void GameSim::Rescue(PlayerA& p)
    {
        float sx = p.SafeX, sy = p.SafeY;
        if (LockedArena && (sx < Level.ArenaX0 + 24.0f || sx + p.W > Level.ArenaX1 - 24.0f))
        {
            sx = Level.ArenaX0 + 60.0f;
            sy = Level.BossAt.y - p.H - 4.0f;
        }
        p.X = sx; p.Y = sy; p.Vx = 0.0f; p.Vy = 0.0f;
        p.OnGround = false; p.Jumps = 0; p.Coyote = 0.0f;
        Lift(p);
        p.Inv = std::max(p.Inv, 1.35f);
        Emit(FxKind::Rescue, p.X + p.W / 2.0f, p.Y + p.H / 2.0f);
    }

    void GameSim::Phys(Actor& a, float dt, float grav, bool fly)
    {
        constexpr float kMaxStep = 6.0f;
        if (!fly) a.Vy += grav * dt;
        a.Vy = ClampF(a.Vy, -22.0f, 16.0f);

        float dx = a.Vx * 92.0f * dt;
        float dy = a.Vy * 72.0f * dt;
        int steps = ClampI(CeilToInt(std::max(std::abs(dx), std::abs(dy)) / kMaxStep), 1, 24);
        float sx = dx / steps, sy = dy / steps;

        a.OnGround = false;
        for (int i = 0; i < steps; ++i)
        {
            MoveX(a, sx);
            if (MoveY(a, sy)) sy = 0.0f;
        }
        if (!a.OnGround && std::abs(dy) < 0.001f) MoveY(a, 0.0f);

        a.Water = TileAt(a.X + a.W / 2.0f, a.Y + a.H * 0.6f) == T::Water;
        if (a.Water) { a.Vy *= 0.86f; a.Vx *= 0.9f; }
        if (a.OnGround && TileAt(a.X + a.W / 2.0f, a.Y + a.H + 1.0f) == T::Ice && a.Vx != 0.0f) a.Vx *= 0.992f;
    }

    void GameSim::MoveX(Actor& a, float d)
    {
        if (d == 0.0f) return;
        a.X += d;
        float feet = a.Y + a.H - 2.0f, mid = a.Y + a.H * 0.5f, head = a.Y + 6.0f;
        if (d > 0.0f && (SolidAt(a.X + a.W, feet, false) || SolidAt(a.X + a.W, mid, false) || SolidAt(a.X + a.W, head, false)))
        {
            a.X = std::floor((a.X + a.W) / T::Tile) * T::Tile - a.W - 0.01f;
            a.Vx = 0.0f;
        }
        else if (d < 0.0f && (SolidAt(a.X, feet, false) || SolidAt(a.X, mid, false) || SolidAt(a.X, head, false)))
        {
            a.X = std::floor(a.X / T::Tile) * T::Tile + T::Tile + 0.01f;
            a.Vx = 0.0f;
        }
    }

    bool GameSim::MoveY(Actor& a, float d)
    {
        a.Y += d;
        if (d >= 0.0f)
        {
            float fy = a.Y + a.H + 0.5f;
            bool hit = SolidAt(a.X + 4.0f, fy, true) || SolidAt(a.X + a.W - 4.0f, fy, true) || SolidAt(a.X + a.W / 2.0f, fy, true);
            a.OnGround = hit;
            if (!hit) return false;
            a.Y = std::floor(fy / T::Tile) * T::Tile - a.H - 0.01f;
            a.Vy = 0.0f;
            return true;
        }
        if (SolidAt(a.X + 4.0f, a.Y + 2.0f, false) || SolidAt(a.X + a.W - 4.0f, a.Y + 2.0f, false))
        {
            a.Y = std::floor(a.Y / T::Tile) * T::Tile + T::Tile + 0.01f;
            a.Vy = 0.0f;
            return true;
        }
        return false;
    }

    bool GameSim::WallAhead(const Actor& e) const
    {
        float x = e.Dir > 0 ? e.X + e.W + 4.0f : e.X - 4.0f;
        return Block(TileAt(x, e.Y + e.H * 0.35f)) || Block(TileAt(x, e.Y + e.H * 0.7f));
    }

    bool GameSim::Ledge(const Actor& e) const
    {
        return HazardFloor(TileAt(e.Dir > 0 ? e.X + e.W + 6.0f : e.X - 6.0f, e.Y + e.H + 5.0f));
    }

    bool GameSim::Bounds(const Actor& e) const
    {
        return e.X < 20.0f || e.X + e.W > Level.Cols * T::Tile - 20.0f;
    }

    void GameSim::Flip(Actor& e, float cd)
    {
        e.Dir = e.Dir < 0 ? 1 : -1;
        e.Facing = e.Dir;
        e.FlipCd = cd;
    }

    void GameSim::FaceWalk(Actor& e, float dt, float speed)
    {
        e.FlipCd = std::max(0.0f, e.FlipCd - dt);
        if (e.FlipCd <= 0.0f && (WallAhead(e) || (e.OnGround && Ledge(e)) || Bounds(e))) Flip(e, 0.36f);
        e.Vx = e.Dir * speed;
        e.Facing = e.Dir;
    }

    void GameSim::Lift(Actor& e)
    {
        for (int i = 0; i < 28; ++i)
        {
            int t = TileAt(e.X + e.W * 0.5f, e.Y + e.H - 3.0f);
            if (Block(t) || t == T::Platform || t == T::Bounce) e.Y -= 5.0f;
            else break;
        }
    }

    PlayerA* GameSim::Nearest(float x, float y)
    {
        PlayerA* best = nullptr;
        float bd = 1e9f;
        for (auto& p : Players)
        {
            if (!p.Dead)
            {
                float d = std::abs(p.X - x) + std::abs(p.Y - y);
                if (d < bd) { bd = d; best = &p; }
            }
        }
        return best;
    }

    bool GameSim::Aabb(const Actor& a, const Actor& b)
    {
        return a.X < b.X + b.W && a.X + a.W > b.X && a.Y < b.Y + b.H && a.Y + a.H > b.Y;
    }

    void GameSim::CollidePlayers(Actor& e, int dmg, bool isBoss)
    {
        if (e.Dead) return;
        for (auto& pl : Players)
        {
            if (pl.Dead || !Aabb(e, pl)) continue;
            bool stomp = pl.Vy > 1.2f && pl.Y + pl.H < e.Y + e.H * 0.58f;
            if (stomp)
            {
                DmgEnemy(e, isBoss, 2);
                pl.Vy = -8.0f;
                if (Sfx) Sfx("stomp");
            }
            else HurtFrom(pl, dmg, e.X + e.W / 2.0f);
        }
    }

    void GameSim::UpdateEnemy(EnemyA& e, float dt)
    {
        if (e.Dead) return;
        e.T += dt;
        e.Flash = std::max(0.0f, e.Flash - dt);
        e.Stun = std::max(0.0f, e.Stun - dt);
        const auto& def = e.Def;
        PlayerA* p = Nearest(e.X, e.Y);
        bool flying = def.Fly || def.Ai == "fly" || def.Ai == "swoop";
        if (e.Stun > 0.0f)
        {
            if (!flying) Phys(e, dt, 26.0f, false);
            CollidePlayers(e, def.Dmg, false);
            return;
        }
        if (flying)
        {
            e.FlipCd = std::max(0.0f, e.FlipCd - dt);
            if (e.FlipCd <= 0.0f)
            {
                if (e.X > e.HomeX + 86.0f || e.X + e.W > Level.Cols * T::Tile - 24.0f) Flip(e, 0.25f);
                else if (e.X < e.HomeX - 86.0f || e.X < 24.0f) Flip(e, 0.25f);
                else if (WallAhead(e)) Flip(e, 0.3f);
            }
            e.X += e.Dir * def.Speed * 68.0f * dt;
            e.Y = e.HomeY + std::sin(e.T * 2.5f) * (def.Ai == "swoop" ? 20.0f : 13.0f);
            if (def.Ai == "swoop" && p && std::abs(p->X - e.X) < 190.0f)
                e.Y += SignF((p->Y + 10.0f) - e.Y) * 32.0f * dt;
            e.Facing = e.Dir;
            e.OnGround = false;
            CollidePlayers(e, def.Dmg, false);
            return;
        }
        Lift(e);
        if (def.Ai == "jump")
        {
            if (e.OnGround && std::fmod(e.T, 1.55f) < dt + 0.02f) e.Vy = -8.6f;
            FaceWalk(e, dt, def.Speed);
        }
        else if (def.Ai == "charge")
        {
            if (p && std::abs(p->Y - e.Y) < 58.0f && std::abs(p->X - e.X) < 230.0f)
            {
                int want = p->X + p->W / 2.0f >= e.X + e.W / 2.0f ? 1 : -1;
                e.FlipCd = std::max(0.0f, e.FlipCd - dt);
                if (e.FlipCd <= 0.0f && want != e.Dir) Flip(e, 0.4f);
                if (WallAhead(e) || (e.OnGround && Ledge(e))) { if (e.FlipCd <= 0.0f) Flip(e, 0.45f); }
                e.Vx = e.Dir * def.Speed * 1.5f;
                e.Facing = e.Dir;
            }
            else FaceWalk(e, dt, def.Speed);
        }
        else if (def.Ai == "shoot" || def.Ai == "mage")
        {
            FaceWalk(e, dt, def.Speed * 0.7f);
            e.ShotT -= dt;
            if (e.ShotT <= 0.0f && p && std::abs(p->X - e.X) < 360.0f)
            {
                FireShot(e, !def.Shot.empty() ? def.Shot : "dark");
                Emit(FxKind::Cast, e.X + e.W / 2.0f, e.Y + 16.0f);
                e.ShotT = def.Ai == "mage" ? 1.4f : 1.8f;
            }
            if (p) e.Facing = p->X > e.X ? 1 : -1;
        }
        else FaceWalk(e, dt, def.Speed);
        Phys(e, dt, 26.0f, false);
        CollidePlayers(e, def.Dmg, false);
    }

    void GameSim::UpdateBoss(BossA& b, float dt)
    {
        if (b.Dead) return;
        b.T += dt;
        b.Flash = std::max(0.0f, b.Flash - dt);
        if (b.Intro > 0.0f) { b.Intro -= dt; return; }
        b.StateT -= dt;
        PlayerA* p = Nearest(b.X, b.Y);
        float enrage = b.Hp < b.MaxHp * 0.4f ? 1.35f : 1.0f;
        const std::string& pat = b.Def.Pattern;
        if (b.State == "idle")
        {
            if (p) { b.Facing = p->X > b.X ? 1 : -1; b.Vx = b.Facing * b.Def.Speed * 0.6f * enrage; }
            if (b.StateT <= 0.0f)
            {
                std::vector<std::string> opts = {"slam", "shot", "dash"};
                if (pat == "hex" || pat == "final") opts.push_back("cast");
                if (pat == "summon" || pat == "final" || pat == "multi") opts.push_back("summon");
                if (pat == "flyfire") opts.push_back("air");
                if (pat == "wave" || pat == "lava" || pat == "ice") opts.push_back("wave");
                b.State = opts[NextRandomRange(0, static_cast<int>(opts.size()))];
                b.StateT = 0.9f;
            }
        }
        else if (b.State == "slam")
        {
            if (b.StateT > 0.55f) b.Vy = -10.0f;
            if (b.StateT < 0.25f && b.OnGround)
            {
                Shake = 14.0f;
                Emit(FxKind::Boom, b.X + b.W / 2.0f, b.Y + b.H);
                for (auto& pl : Players) if (!pl.Dead && std::abs(pl.X - (b.X + b.W / 2.0f)) < 90.0f) HurtFrom(pl, 1, b.X);
            }
            if (b.StateT <= 0.0f) { b.State = "idle"; b.StateT = 0.8f; }
        }
        else if (b.State == "shot" || b.State == "cast")
        {
            b.Vx = 0.0f;
            std::string kind = pat == "ice" ? "ice" : (pat == "lava" || pat == "flyfire") ? "hell" : "hex";
            if (b.StateT < 0.7f && FloorToInt(b.StateT * 8.0f) != FloorToInt((b.StateT + dt) * 8.0f))
            {
                FireShot(b, kind);
                Emit(FxKind::Cast, b.X + b.W / 2.0f, b.Y + 20.0f);
            }
            if (b.StateT <= 0.0f) { b.State = "idle"; b.StateT = 0.7f; }
        }
        else if (b.State == "dash")
        {
            b.Vx = b.Facing * b.Def.Speed * 3.4f * enrage;
            if (b.StateT <= 0.0f) { b.State = "idle"; b.StateT = 1.0f; b.Vx = 0.0f; }
        }
        else if (b.State == "summon")
        {
            b.Vx = 0.0f;
            int live = 0;
            for (const auto& e : Enemies) if (!e.Dead) live++;
            if (b.StateT < 0.85f && live < 6 && !Level.World.Enemies.empty())
            {
                std::string eid = Level.World.Enemies[NextRandomRange(0, static_cast<int>(Level.World.Enemies.size()))];
                const auto* d = Catalog::Enemy(eid);
                if (d && Enemies.size() < 40)
                {
                    EnemyA a;
                    a.Id = eid; a.Def = *d; a.Kind = Catalog::EnemyIndex(eid);
                    a.X = b.X + static_cast<float>(NextRandomRange(-20, 60));
                    a.Y = b.Y; a.W = static_cast<float>(d->W); a.H = static_cast<float>(d->H);
                    a.Hp = d->Hp; a.MaxHp = d->Hp; a.Dir = 1; a.HomeX = b.X; a.HomeY = b.Y; a.ShotT = 1.0f;
                    a.Facing = a.Dir;
                    Lift(a);
                    Enemies.push_back(a);
                    Emit(FxKind::Cast, a.X + a.W / 2.0f, a.Y + a.H / 2.0f);
                }
                b.State = "idle"; b.StateT = 1.4f;
            }
            if (b.StateT <= 0.0f) { b.State = "idle"; b.StateT = 1.0f; }
        }
        else if (b.State == "air")
        {
            b.Vy = -4.0f;
            b.Y -= 20.0f * dt;
            if (FloorToInt(b.T * 4.0f) != FloorToInt((b.T - dt) * 4.0f)) FireShot(b, "hell");
            if (b.StateT <= 0.0f) { b.State = "idle"; b.StateT = 0.8f; }
        }
        else if (b.State == "wave")
        {
            b.Vx = 0.0f;
            if (FloorToInt(b.StateT * 3.0f) != FloorToInt((b.StateT + dt) * 3.0f))
            {
                int dir = NextRandom01() > 0.5f ? 1 : -1;
                std::string kind = pat == "lava" ? "hell" : pat == "ice" ? "ice" : "dark";
                ProjA pr;
                pr.Kind = kind; pr.X = b.X + b.W / 2.0f; pr.Y = b.Y + b.H - 18.0f;
                pr.W = 20.0f; pr.H = 16.0f; pr.Vx = dir * 5.5f; pr.Vy = 0.0f;
                pr.Life = 2.2f; pr.Dmg = 1; pr.Friendly = false;
                Projectiles.push_back(pr);
            }
            if (b.StateT <= 0.0f) { b.State = "idle"; b.StateT = 0.9f; }
        }
        Phys(b, dt, 22.0f, false);
        if (b.X < Level.ArenaX0 + 10.0f) { b.X = Level.ArenaX0 + 10.0f; b.Facing = 1; }
        if (b.X + b.W > Level.ArenaX1 - 10.0f) { b.X = Level.ArenaX1 - 10.0f - b.W; b.Facing = -1; }
        CollidePlayers(b, 1, true);
    }

    void GameSim::FireMagic(PlayerA& p)
    {
        ProjA pr;
        pr.Kind = "magic";
        pr.X = p.X + (p.Facing > 0 ? p.W : -16.0f);
        pr.Y = p.Y + 14.0f;
        pr.W = 18.0f; pr.H = 12.0f;
        pr.Vx = p.Facing * 14.0f; pr.Vy = 0.0f;
        pr.Life = 0.9f; pr.Dmg = p.Hero.Damage; pr.Friendly = true;
        Projectiles.push_back(pr);
        p.Vx -= p.Facing * 0.9f;
        Emit(FxKind::Cast, p.X + p.W / 2.0f + p.Facing * 22.0f, p.Y + 18.0f, p.Facing);
        Shake = std::max(Shake, 3.0f);
        if (Sfx) Sfx("magic");
    }

    void GameSim::FireShot(const Actor& e, const std::string& kind)
    {
        PlayerA* p = Nearest(e.X, e.Y);
        if (!p) return;
        float dx = p->X + p->W / 2.0f - (e.X + e.W / 2.0f);
        float dy = p->Y + p->H / 2.0f - (e.Y + 16.0f);
        float len = std::max(0.01f, std::sqrt(dx * dx + dy * dy));
        ProjA pr;
        pr.Kind = kind; pr.X = e.X + e.W / 2.0f; pr.Y = e.Y + 16.0f;
        pr.W = 12.0f; pr.H = 12.0f;
        pr.Vx = dx / len * 4.2f; pr.Vy = dy / len * 4.2f;
        pr.Life = 2.4f; pr.Dmg = 1; pr.Friendly = false;
        Projectiles.push_back(pr);
    }

    void GameSim::MeleeSweep(PlayerA& p, float st)
    {
        float cx = p.X + p.W / 2.0f, cy = p.Y + p.H * 0.46f;
        float reach = static_cast<float>(p.Hero.Reach);
        float sweep = LerpF(-52.0f, 44.0f, Clamp01((st - 0.12f) / 0.76f));
        for (auto& e : Enemies) if (!e.Dead) TrySwing(p, e, false, cx, cy, reach, sweep);
        for (auto& b : Bosses) if (!b.Dead) TrySwing(p, b, true, cx, cy, reach, sweep);
    }

    void GameSim::TrySwing(PlayerA& p, Actor& e, bool isBoss, float cx, float cy, float reach, float sweep)
    {
        if ((p.Slot == 0 ? e.HitS0 : e.HitS1) == p.Swing) return;
        float nx = ClampF(cx, e.X, e.X + e.W);
        float ny = ClampF(cy, e.Y, e.Y + e.H);
        float dx = (nx - cx) * p.Facing, dy = ny - cy;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d > reach || dx < -10.0f) return;
        float ang = d < 1.0f ? sweep : std::atan2(dy, std::max(0.001f, dx)) * kRad2Deg;
        if (std::abs(DeltaAngle(ang, sweep)) > 52.0f) return;

        if (p.Slot == 0) e.HitS0 = p.Swing; else e.HitS1 = p.Swing;
        DmgEnemy(e, isBoss, p.Hero.Damage, static_cast<float>(p.Facing), isBoss ? 1.1f : 4.4f);
        Shake = std::max(Shake, 6.0f);
        HitStop = std::max(HitStop, 0.055f);
        Emit(FxKind::Slash, nx, ny, p.Facing);
    }

    void GameSim::UpdateProj(float dt)
    {
        for (int i = static_cast<int>(Projectiles.size()) - 1; i >= 0; --i)
        {
            auto& pr = Projectiles[i];
            pr.Life -= dt; pr.Age += dt;
            pr.X += pr.Vx * 110.0f * dt; pr.Y += pr.Vy * 110.0f * dt;
            int t = TileAt(pr.X + pr.W / 2.0f, pr.Y + pr.H / 2.0f);
            if (t == T::Solid || t == T::Ice || t == T::Break) { pr.Life = 0.0f; Emit(FxKind::Hit, pr.X, pr.Y); }
            Actor box; box.X = pr.X; box.Y = pr.Y; box.W = pr.W; box.H = pr.H;
            if (pr.Friendly)
            {
                float dir = pr.Vx >= 0.0f ? 1.0f : -1.0f;
                for (auto& e : Enemies)
                    if (!e.Dead && Aabb(box, e))
                    {
                        DmgEnemy(e, false, pr.Dmg, dir, 3.0f);
                        Emit(FxKind::Burst, pr.X + pr.W / 2.0f, pr.Y + pr.H / 2.0f, static_cast<int>(dir));
                        HitStop = std::max(HitStop, 0.035f);
                        pr.Life = 0.0f;
                    }
                for (auto& b : Bosses)
                    if (!b.Dead && Aabb(box, b))
                    {
                        DmgEnemy(b, true, pr.Dmg, dir, 0.8f);
                        Emit(FxKind::Burst, pr.X + pr.W / 2.0f, pr.Y + pr.H / 2.0f, static_cast<int>(dir));
                        pr.Life = 0.0f;
                    }
            }
            else
            {
                for (auto& p : Players)
                    if (!p.Dead && Aabb(box, p)) { HurtFrom(p, pr.Dmg, pr.X); pr.Life = 0.0f; }
            }
            if (pr.Life <= 0.0f) Projectiles.erase(Projectiles.begin() + i);
        }
    }

    void GameSim::UpdateItems(float dt)
    {
        for (auto& it : Items)
        {
            if (it.Taken) continue;
            it.Bob += dt;
            Actor box; box.X = it.X; box.Y = it.Y + std::sin(it.Bob * 3.0f) * 3.0f; box.W = 20.0f; box.H = 20.0f;
            for (auto& p : Players)
            {
                if (p.Dead || !Aabb(box, p)) continue;
                if (it.Type == "soul") { Souls++; Score += 25; Emit(FxKind::Pickup, it.X + 10.0f, it.Y + 10.0f); if (Sfx) Sfx("coin"); }
                else if (it.Type == "heart") { p.Hp = std::min(p.MaxHp, p.Hp + 2); Emit(FxKind::Heal, it.X + 10.0f, it.Y + 10.0f); if (Sfx) Sfx("heart"); }
                else { Emit(FxKind::Pickup, it.X + 10.0f, it.Y + 10.0f); }
                it.Taken = true;
                break;
            }
        }
    }

    void GameSim::HurtFrom(PlayerA& p, int dmg, float srcX)
    {
        if (p.Dead || p.Inv > 0.0f) return;
        p.Vx = (p.X + p.W / 2.0f < srcX ? -1.0f : 1.0f) * 3.2f;
        p.Vy = -6.0f;
        HurtDirect(p, dmg);
    }

    void GameSim::HurtDirect(PlayerA& p, int dmg)
    {
        if (p.Dead || p.Inv > 0.0f) return;
        p.Hp -= dmg; p.Inv = 1.15f; p.HurtT = 0.25f; Shake = 8.0f;
        Emit(FxKind::Hurt, p.X + p.W / 2.0f, p.Y + p.H / 2.0f);
        if (Sfx) Sfx("hurt");
        if (p.Hp > 0) return;
        p.Hp = 0; p.Lives--;
        if (p.Lives < 0)
        {
            p.Dead = true;
            Emit(FxKind::Death, p.X + p.W / 2.0f, p.Y + p.H / 2.0f);
            if (Sfx) Sfx("die");
        }
        else
        {
            p.Hp = p.MaxHp;
            p.X = p.SpawnX; p.Y = p.SpawnY; p.SafeX = p.SpawnX; p.SafeY = p.SpawnY;
            p.Vx = p.Vy = 0.0f; p.Inv = 2.0f;
            Lift(p);
            Emit(FxKind::Rescue, p.X + p.W / 2.0f, p.Y + p.H / 2.0f);
        }
    }

    void GameSim::DmgEnemy(Actor& e, bool isBoss, int dmg, float pushDir, float push)
    {
        if (e.Dead) return;
        e.Hp -= dmg;
        e.Flash = 0.14f;
        e.Stun = std::max(e.Stun, push > 2.0f ? 0.2f : 0.12f);
        if (push != 0.0f)
        {
            e.Vx = pushDir * push;
            if (e.OnGround && push > 2.0f) e.Vy = -3.2f;
        }
        Emit(FxKind::Hit, e.X + e.W / 2.0f, e.Y + e.H / 2.0f);
        if (Sfx) Sfx("hit");
        if (e.Hp > 0) return;
        if (isBoss)
        {
            auto& b = static_cast<BossA&>(e);
            b.Dead = true; Score += b.Def.Score; Souls += 15;
            Message = b.Def.Name + " caiu! Sigam o sino dourado."; MessageT = 3.0f;
            Shake = 18.0f;
            Emit(FxKind::BossDown, b.X + b.W / 2.0f, b.Y + b.H / 2.0f);
            if (Sfx) Sfx("clear");
            if (WorldIndex == 15 && b.Id != "belial" && !BelialPhase)
            {
                BelialPhase = true; BossIntro = 2.2f; Message = "Belial desperta do trono!"; MessageT = 2.6f;
                const auto& def = Catalog::Belial();
                BossA belial;
                belial.Id = def.Id; belial.Def = def;
                belial.X = (Level.ArenaX0 + Level.ArenaX1) / 2.0f - def.W / 2.0f;
                belial.Y = Level.BossAt.y - def.H;
                belial.W = static_cast<float>(def.W); belial.H = static_cast<float>(def.H);
                belial.Hp = def.Hp; belial.MaxHp = def.Hp; belial.Facing = -1;
                _spawnQueue.push_back(belial);
            }
            else if (b.Id == "belial") State = "win";
        }
        else
        {
            auto& en = static_cast<EnemyA&>(e);
            en.Dead = true; Score += en.Def.Score; Souls++;
            Emit(FxKind::Death, en.X + en.W / 2.0f, en.Y + en.H / 2.0f);
            if (Sfx) Sfx("stomp");
        }
    }

    void GameSim::MaybeLock()
    {
        if (LockedArena || !HasLevel) return;
        bool entered = false;
        for (const auto& p : Players) if (!p.Dead && p.X > Level.ArenaX0 + 40.0f) { entered = true; break; }
        if (!entered) return;
        const auto* def = Catalog::Boss(Level.World.Boss);
        if (!def) return;
        LockedArena = true; BossIntro = 1.8f;
        BossA boss;
        boss.Id = def->Id; boss.Def = *def;
        boss.X = Level.BossAt.x - def->W / 2.0f;
        boss.Y = Level.BossAt.y - def->H;
        boss.W = static_cast<float>(def->W); boss.H = static_cast<float>(def->H);
        boss.Hp = def->Hp; boss.MaxHp = def->Hp; boss.Facing = -1;
        _spawnQueue.push_back(boss);
        Message = def->Name;
        MessageT = 2.2f;
        if (Sfx) Sfx("boss");
    }

    void GameSim::EnsureSecond(const std::string& hero)
    {
        if (Players.size() >= 2 || !HasLevel) return;
        std::string h = (hero.empty() || !Catalog::Hero(hero)) ? "denyse" : hero;
        float x = !Players.empty() ? Players[0].X + 40.0f : Level.Spawn.x;
        float y = !Players.empty() ? Players[0].Y : Level.Spawn.y;
        PlayerA p = MakePlayer(1, h, x, y);
        Lift(p);
        Players.push_back(p);
    }

    void GameSim::TickRemote(float dt)
    {
        Time += dt;
        Fade = Clamp01(Fade + FadeDir * dt * 1.6f);
        MessageT = std::max(0.0f, MessageT - dt);
        Shake *= 0.88f;
        for (auto& it : Items) if (!it.Taken) it.Bob += dt;
        for (auto& p : Players)
        {
            p.AnimT += dt;
            p.Atk = std::max(0.0f, p.Atk - dt);
            p.Inv = std::max(0.0f, p.Inv - dt);
            p.HurtT = std::max(0.0f, p.HurtT - dt);
        }
        for (auto& e : Enemies) { e.T += dt; e.Flash = std::max(0.0f, e.Flash - dt); }
        for (auto& b : Bosses) { b.T += dt; b.Flash = std::max(0.0f, b.Flash - dt); }
        for (auto& pr : Projectiles) { pr.Age += dt; pr.X += pr.Vx * 110.0f * dt; pr.Y += pr.Vy * 110.0f * dt; }
        UpdateCam(dt);
    }

    void GameSim::ApplyNetEnemies(const std::vector<EntSnap>& ents)
    {
        if (Enemies.size() > ents.size()) Enemies.resize(ents.size());
        for (size_t i = 0; i < ents.size(); ++i)
        {
            const auto& s = ents[i];
            while (Enemies.size() <= i)
            {
                const auto* kd = Catalog::EnemyAt(s.Kind);
                if (!kd) return;
                EnemyA a;
                a.Id = kd->Id; a.Def = *kd; a.Kind = s.Kind;
                a.X = s.X; a.Y = s.Y; a.W = static_cast<float>(kd->W); a.H = static_cast<float>(kd->H);
                a.Hp = s.Hp; a.MaxHp = kd->Hp;
                Enemies.push_back(a);
            }
            auto& e = Enemies[i];
            if (e.Kind != s.Kind)
            {
                const auto* kd = Catalog::EnemyAt(s.Kind);
                if (kd)
                {
                    e.Id = kd->Id; e.Def = *kd; e.Kind = s.Kind;
                    e.W = static_cast<float>(kd->W); e.H = static_cast<float>(kd->H); e.MaxHp = kd->Hp;
                }
            }
            float d = std::abs(e.X - s.X) + std::abs(e.Y - s.Y);
            if (d > 220.0f) { e.X = s.X; e.Y = s.Y; }
            else { e.X = LerpF(e.X, s.X, 0.5f); e.Y = LerpF(e.Y, s.Y, 0.5f); }
            e.Facing = (s.Flags & 4) != 0 ? -1 : 1;
            e.Dir = e.Facing;
            e.Hp = s.Hp;
            bool wasDead = e.Dead;
            e.Dead = (s.Flags & 1) != 0;
            if (!wasDead && e.Dead) Emit(FxKind::Death, e.X + e.W / 2.0f, e.Y + e.H / 2.0f);
            if ((s.Flags & 2) != 0) e.Flash = 0.14f;
        }
    }

    void GameSim::ApplyNetProjectiles(const std::vector<ProjSnap>& ps)
    {
        Projectiles.clear();
        for (const auto& s : ps)
        {
            ProjA pr;
            pr.Kind = ProjKindName(s.Kind);
            pr.X = s.X; pr.Y = s.Y;
            pr.W = s.Kind == 0 ? 18.0f : 12.0f;
            pr.H = 12.0f;
            pr.Vx = s.Vx; pr.Vy = s.Vy;
            pr.Friendly = (s.Flags & 1) != 0;
            pr.Life = 1.0f;
            Projectiles.push_back(pr);
        }
    }

    uint8_t GameSim::ProjKindId(const std::string& kind)
    {
        if (kind == "magic") return 0;
        if (kind == "ice") return 1;
        if (kind == "hell") return 2;
        if (kind == "hex") return 3;
        if (kind == "bone") return 4;
        if (kind == "holy") return 5;
        return 6;
    }

    std::string GameSim::ProjKindName(uint8_t id)
    {
        switch (id)
        {
            case 0: return "magic";
            case 1: return "ice";
            case 2: return "hell";
            case 3: return "hex";
            case 4: return "bone";
            case 5: return "holy";
            default: return "dark";
        }
    }

    void GameSim::ApplyNetItems(uint64_t mask)
    {
        size_t n = std::min<size_t>(64, Items.size());
        for (size_t i = 0; i < n; ++i)
        {
            bool alive = (mask & (1ULL << i)) != 0;
            if (!Items[i].Taken && !alive) Emit(FxKind::Pickup, Items[i].X + 10.0f, Items[i].Y + 10.0f);
            Items[i].Taken = !alive;
        }
    }

    uint64_t GameSim::ItemMask() const
    {
        uint64_t m = 0;
        size_t n = std::min<size_t>(64, Items.size());
        for (size_t i = 0; i < n; ++i) if (!Items[i].Taken) m |= (1ULL << i);
        return m;
    }

    FrameSnap GameSim::CaptureSnapshot()
    {
        FrameSnap snap;
        auto packPlayer = [](const PlayerA& p) -> PlrSnap {
            PlrSnap s;
            s.X = p.X; s.Y = p.Y;
            s.Hp = static_cast<uint8_t>(std::max(0, p.Hp));
            s.MaxHp = static_cast<uint8_t>(std::max(1, p.MaxHp));
            s.Lives = static_cast<uint8_t>(std::max(0, p.Lives));
            s.Anim = p.Anim == "walk" ? 1 : p.Anim == "jump" ? 2 : p.Anim == "fall" ? 3 : p.Anim == "attack" ? 4 : 0;
            s.Atk = p.Atk > 0.0f ? static_cast<uint8_t>(ClampI(RoundToInt((1.0f - p.Atk / std::max(0.01f, p.Hero.AtkTime)) * 255.0f), 1, 255)) : 0;
            s.Flags = (p.Inv > 0.0f ? 1 : 0) | (p.HurtT > 0.0f ? 2 : 0) | (p.Dead ? 4 : 0) | (p.Facing < 0 ? 8 : 0);
            s.Hero = p.Hero.Id == "denyse" ? 1 : 0;
            return s;
        };
        if (!Players.empty()) snap.P0 = packPlayer(Players[0]);
        if (Players.size() >= 2) snap.P1 = packPlayer(Players[1]);
        snap.Score = Score;
        snap.Souls = Souls;
        snap.LockArena = LockedArena;
        snap.SimState = State;
        if (!Bosses.empty())
        {
            const auto& b = Bosses.back();
            snap.BossHp = std::max(0, b.Hp);
            snap.BossMax = std::max(1, b.MaxHp);
            snap.BossKind = static_cast<uint8_t>(Catalog::BossIndex(b.Id));
            snap.BossFlags = (b.Dead ? 1 : 0) | (b.Facing < 0 ? 2 : 0);
            snap.BossPos = {b.X, b.Y};
        }
        snap.ItemMask = ItemMask();
        snap.Shake = Shake;
        snap.Msg = Message;
        snap.MsgT = MessageT;

        std::vector<const EnemyA*> visible;
        for (const auto& e : Enemies) if (!e.Dead) visible.push_back(&e);
        std::stable_sort(visible.begin(), visible.end(), [this](const EnemyA* a, const EnemyA* b) {
            return std::abs(a->X - (CamX + T::ViewW/2)) < std::abs(b->X - (CamX + T::ViewW/2));
        });
        size_t ne = std::min<size_t>(FrameSnap::MaxEnts, visible.size());
        snap.Ents.reserve(ne);
        for (size_t i = 0; i < ne; ++i)
        {
            const auto& e = *visible[i];
            EntSnap es;
            es.X = e.X; es.Y = e.Y;
            es.Hp = static_cast<uint8_t>(std::max(0, e.Hp));
            es.Kind = static_cast<uint8_t>(e.Kind);
            es.Flags = (e.Dead ? 1 : 0) | (e.Flash > 0.0f ? 2 : 0) | (e.Facing < 0 ? 4 : 0);
            snap.Ents.push_back(es);
        }

        size_t np = std::min<size_t>(FrameSnap::MaxProjs, Projectiles.size());
        snap.Projs.reserve(np);
        for (size_t i = 0; i < np; ++i)
        {
            const auto& pr = Projectiles[i];
            ProjSnap ps;
            ps.X = pr.X; ps.Y = pr.Y; ps.Vx = pr.Vx; ps.Vy = pr.Vy;
            ps.Kind = ProjKindId(pr.Kind);
            ps.Flags = pr.Friendly ? 1 : 0;
            snap.Projs.push_back(ps);
        }

        size_t nf = std::min<size_t>(FrameSnap::MaxFx, Fx.size());
        snap.Fxs.reserve(nf);
        for (size_t i = 0; i < nf; ++i)
        {
            const auto& fx = Fx[i];
            snap.Fxs.push_back({fx.X, fx.Y, fx.Kind, static_cast<uint8_t>(fx.Dir < 0 ? 0 : 1)});
        }
        return snap;
    }

    void GameSim::ApplySnapshot(const FrameSnap& snap)
    {
        EnsureSecond("denyse");
        auto unpackPlayer = [](PlayerA& p, const PlrSnap& s) {
            const auto* hero = Catalog::Hero(s.Hero == 1 ? "denyse" : "mike");
            if (hero) p.Hero = *hero;
            p.X = s.X; p.Y = s.Y;
            p.Hp = s.Hp; p.MaxHp = s.MaxHp; p.Lives = s.Lives;
            p.Anim = s.Anim == 1 ? "walk" : s.Anim == 2 ? "jump" : s.Anim == 3 ? "fall" : s.Anim == 4 ? "attack" : "idle";
            p.Inv = (s.Flags & 1) ? 0.2f : 0.0f;
            p.HurtT = (s.Flags & 2) ? 0.2f : 0.0f;
            p.Dead = (s.Flags & 4) != 0;
            p.Facing = (s.Flags & 8) ? -1 : 1;
            p.Atk = s.Atk == 0 ? 0.0f : std::max(0.001f, (1.0f-s.Atk/255.0f)*p.Hero.AtkTime);
        };
        if (!Players.empty()) unpackPlayer(Players[0], snap.P0);
        if (Players.size() >= 2) unpackPlayer(Players[1], snap.P1);
        Score = snap.Score;
        Souls = snap.Souls;
        LockedArena = snap.LockArena;
        State = snap.SimState;
        Shake = snap.Shake;
        Message = snap.Msg;
        MessageT = snap.MsgT;
        ApplyNetItems(snap.ItemMask);
        ApplyNetEnemies(snap.Ents);
        ApplyNetProjectiles(snap.Projs);
        // The client never simulates boss activation or the Warden -> Belial transition.
        Bosses.clear();
        if (snap.BossMax > 0) {
            const auto* definition = Catalog::BossAt(snap.BossKind);
            if (definition) {
                BossA boss;
                boss.Id=definition->Id; boss.Def=*definition; boss.W=static_cast<float>(definition->W); boss.H=static_cast<float>(definition->H);
                boss.X=snap.BossPos.x; boss.Y=snap.BossPos.y; boss.Hp=snap.BossHp; boss.MaxHp=snap.BossMax;
                boss.Dead=(snap.BossFlags&1)!=0; boss.Facing=(snap.BossFlags&2)!=0 ? -1:1; boss.Dir=boss.Facing;
                Bosses.push_back(boss);
                BelialPhase=boss.Id=="belial";
            }
        }
        for (const auto& f : snap.Fxs)
        {
            Emit(static_cast<FxKind>(f.Kind), f.X, f.Y, f.Dir == 0 ? -1 : 1);
        }
    }

    void GameSim::CheckClear()
    {
        if (State == "win") return;
        if (!LockedArena || Bosses.empty()) return;
        for (const auto& b : Bosses) if (!b.Dead) return;
        if (WorldIndex == 15)
        {
            if (!BelialPhase) return;
            for (const auto& b : Bosses) if (b.Id == "belial" && !b.Dead) return;
        }
        for (const auto& p : Players)
        {
            if (!p.Dead && std::abs(p.X - Level.Exit.x) < 46.0f && std::abs(p.Y - Level.Exit.y) < 60.0f)
            {
                State = "clear";
                return;
            }
        }
    }

    void GameSim::UpdateCam(float dt)
    {
        if (!HasLevel) return;
        int liveCount = 0;
        float cx = 0.0f, cy = 0.0f;
        for (const auto& p : Players)
        {
            if (!p.Dead)
            {
                cx += p.X + p.W / 2.0f;
                cy += p.Y + p.H / 2.0f;
                liveCount++;
            }
        }
        if (liveCount == 0) return;
        cx /= liveCount;
        cy /= liveCount;
        CamX = LerpF(CamX, cx - T::ViewW * 0.4f, std::min(1.0f, 4.0f * dt));
        CamY = LerpF(CamY, cy - T::ViewH * 0.62f, std::min(1.0f, 3.2f * dt));
        CamX = ClampF(CamX, 0.0f, std::max(0.0f, static_cast<float>(Level.Cols * T::Tile - T::ViewW)));
        CamY = ClampF(CamY, 0.0f, std::max(0.0f, static_cast<float>(Level.Rows * T::Tile - T::ViewH)));
        Shake *= 0.88f;
    }

    bool GameSim::TryParseHexColor(const std::string& hex, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
    {
        if (hex.empty()) return false;
        std::string raw = hex[0] == '#' ? hex.substr(1) : hex;
        if (raw.size() != 6 && raw.size() != 8) return false;
        for (char c : raw) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
        auto byteAt = [&raw](size_t pos) -> uint8_t {
            return static_cast<uint8_t>(std::stoul(raw.substr(pos, 2), nullptr, 16));
        };
        r = byteAt(0);
        g = byteAt(2);
        b = byteAt(4);
        a = raw.size() == 8 ? byteAt(6) : 255;
        return true;
    }
}
