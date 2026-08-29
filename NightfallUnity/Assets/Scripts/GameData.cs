using System;
using System.Collections.Generic;
using UnityEngine;

namespace Nightfall
{
    public static class T
    {
        public const int Empty = 0, Solid = 1, Platform = 2, Spike = 3, Lava = 4, Ice = 5, Water = 6, Break = 7, Bounce = 8, Ladder = 9, Thorn = 10;
        public const int Tile = 40;
        public const int ViewW = 960, ViewH = 540;
    }

    public class HeroDef
    {
        public string Id, Name, Title;
        public int Hp; public float Speed, Jump; public string AtkKind;
        public float AtkTime, AtkCd; public int Damage, Reach;
    }

    public class EnemyDef
    {
        public string Id, Name, Ai, Shot;
        public int Hp, W, H, Dmg, Score; public float Speed; public bool Fly;
    }

    public class BossDef
    {
        public string Id, Name, Pattern;
        public int Hp, W, H, Score; public float Speed;
    }

    public class WorldDef
    {
        public string Id, Name, Tone, Fog, Ground, Lip, Hazard, Flyer, Boss;
        public string[] Enemies, Beats; public int Cols;
    }

    public static class Catalog
    {
        public static readonly Dictionary<string, HeroDef> Heroes = new()
        {
            ["mike"] = new HeroDef { Id = "mike", Name = "Mike", Title = "Caçador da Lâmina", Hp = 6, Speed = 3.55f, Jump = -12.2f, AtkKind = "melee", AtkTime = 0.30f, AtkCd = 0.34f, Damage = 2, Reach = 58 },
            ["denyse"] = new HeroDef { Id = "denyse", Name = "Denyse", Title = "Bruxa do Cristal", Hp = 5, Speed = 3.85f, Jump = -11.8f, AtkKind = "magic", AtkTime = 0.22f, AtkCd = 0.38f, Damage = 2, Reach = 220 }
        };

        public static readonly List<EnemyDef> Enemies = new()
        {
            E("wraith","Espectro da Floresta",2,38,50,0.75f,"patrol",1,120,false),
            E("imp","Diabrete de Espinhos",1,32,34,1.35f,"jump",1,80,false),
            E("crow","Corvo Amaldiçoado",1,36,28,1.6f,"swoop",1,90,true),
            E("doll","Boneca da Cabana",2,30,40,0.9f,"charge",1,140,false),
            E("slime","Limo do Pântano",3,40,28,0.45f,"patrol",1,100,false),
            E("leech","Sanguessuga do Brejo",2,36,22,1.2f,"fly",1,110,true),
            E("mummy","Múmia Andarilha",4,36,50,0.55f,"patrol",1,160,false),
            E("scorpion","Escorpião de Cinzas",2,42,28,1.4f,"charge",1,130,false),
            E("skelly","Arqueiro de Ossos",2,34,48,0.5f,"shoot",1,170,false,"bone"),
            E("ghoul","Carniçal da Cova",3,38,46,1.05f,"charge",1,150,false),
            E("bat","Morcego Vampiro",1,34,26,1.7f,"swoop",1,90,true),
            E("thrall","Servo de Sangue",3,36,48,0.85f,"patrol",1,150,false),
            E("icewraith","Espectro de Gelo",2,36,48,0.8f,"fly",1,140,true),
            E("frostknight","Cavaleiro Congelado",5,40,52,0.6f,"patrol",2,220,false),
            E("fireimp","Diabrete de Fogo",2,30,34,1.3f,"jump",1,120,false),
            E("magmagolem","Golem de Magma",6,46,52,0.4f,"tank",2,260,false),
            E("familiar","Familiar da Coven",1,30,26,1.8f,"swoop",1,100,true),
            E("hexwitch","Bruxa do Hex",3,36,48,0.7f,"mage",1,200,false,"hex"),
            E("drowned","Afogado",3,36,46,0.65f,"patrol",1,140,false),
            E("bonefish","Peixe de Ossos",2,40,24,1.5f,"fly",1,120,true),
            E("wolfpup","Lobisomem Jovem",3,42,36,1.55f,"charge",1,180,false),
            E("cultist","Cultista da Lua",2,34,48,0.75f,"shoot",1,160,false,"dark"),
            E("hatchling","Filhote de Dragão",3,40,32,1.1f,"jump",1,190,false),
            E("wyvern","Serpe Batedora",3,48,32,1.45f,"swoop",1,210,true),
            E("priest","Sacerdote Possuído",4,36,50,0.7f,"mage",1,220,false,"holy"),
            E("hellhound","Cão do Inferno",3,44,32,1.7f,"charge",2,200,false),
            E("plague","Aldeão da Peste",2,34,46,0.8f,"patrol",1,130,false),
            E("bloodgolem","Golem de Sangue",6,46,52,0.45f,"tank",2,280,false),
            E("crawler","Rastejante do Abismo",2,40,24,1.25f,"patrol",1,150,false),
            E("serpent","Serpente das Sombras",4,52,28,1.15f,"fly",1,200,true),
            E("acolyte","Acólito Demoníaco",3,36,48,0.8f,"mage",1,230,false,"hell"),
            E("shade","Sombra do Pesadelo",4,38,50,1.1f,"swoop",2,250,true)
        };

        public static readonly List<BossDef> Bosses = new()
        {
            B("treant","O Ent Oco",28,90,110,0.55f,2000,"slam"),
            B("babawitch","Baba da Cabana",26,70,90,0.7f,2200,"hex"),
            B("hydra","Hidra do Pântano",34,100,80,0.6f,2400,"multi"),
            B("pharaoh","Faraó de Cinzas",32,78,96,0.5f,2500,"summon"),
            B("gravetitan","Titã da Cova",36,96,108,0.4f,2600,"slam"),
            B("nosferatu","Conde Nosferatu",34,70,96,1.1f,2800,"bat"),
            B("icequeen","Rainha de Gelo",32,72,96,0.75f,2800,"ice"),
            B("magmatitan","Titã de Magma",40,100,110,0.45f,3000,"lava"),
            B("morgana","Alta Bruxa Morgana",36,74,98,0.8f,3200,"hex"),
            B("drownedking","Rei Afogado",38,86,100,0.55f,3200,"wave"),
            B("alpha","Lobisomem Alfa",36,92,86,1.35f,3400,"charge"),
            B("dragon","Dragão Carmesim",44,120,90,0.9f,3800,"flyfire"),
            B("cathedral","Demônio da Catedral",42,96,112,0.7f,3800,"slam"),
            B("priestess","Sacerdotisa de Sangue",38,74,98,0.85f,3600,"hex"),
            B("leviathan","Leviatã do Abismo",46,130,80,0.7f,4000,"wave"),
            B("warden","Guardião Infernal",42,100,110,0.65f,4200,"multi")
        };

        public static readonly BossDef Belial = B("belial","Belial, Senhor dos Pesadelos",72,140,130,0.85f,12000,"final");

        public static readonly List<WorldDef> Worlds = new()
        {
            W("forest","Floresta Amaldiçoada","#0b1a10","#143018","#2a4a22","#6a8a38","thorn",new[]{"wraith","imp"},"crow","treant",196,"intro,hills,pit,trees,check,canopy,pit,gauntlet"),
            W("cabin","Cabana da Bruxa","#1a1010","#2a1814","#4a3020","#7a5030","spike",new[]{"doll","imp"},"crow","babawitch",188,"intro,rooms,stairs,attic,check,rooms,pit,gauntlet"),
            W("swamp","Pântano de Ossos","#0c1814","#1a3028","#2a3a28","#4a6a40","water",new[]{"slime","drowned"},"leech","hydra",200,"intro,water,isles,pit,check,water,hills,gauntlet"),
            W("desert","Deserto de Cinzas","#24180c","#3a2814","#6a4a24","#c4a060","spike",new[]{"mummy","scorpion"},"crow","pharaoh",208,"intro,dunes,pit,ruins,check,dunes,stairs,gauntlet"),
            W("grave","Cemitério Sombrio","#101018","#1c1c28","#2a2a30","#5a5a48","spike",new[]{"skelly","ghoul"},"bat","gravetitan",200,"intro,tombs,pit,crypt,check,tombs,stairs,gauntlet"),
            W("castle","Castelo Vampírico","#140810","#2a1020","#3a2030","#6a3048","spike",new[]{"thrall","skelly"},"bat","nosferatu",210,"intro,hall,stairs,towers,check,hall,pit,gauntlet"),
            W("ice","Ermos Congelados","#0c1824","#203848","#3a5a6a","#b0d4e4","ice",new[]{"icewraith","frostknight"},"icewraith","icequeen",204,"intro,icefields,pit,stairs,check,icefields,canopy,gauntlet"),
            W("volcano","Inferno Vulcânico","#1a0804","#3a1408","#4a2010","#c05020","lava",new[]{"fireimp","magmagolem"},"crow","magmatitan",206,"intro,lava,isles,stairs,check,lava,pit,gauntlet"),
            W("coven","Covil das Bruxas","#14081a","#281440","#302044","#7040a0","thorn",new[]{"hexwitch","familiar"},"familiar","morgana",198,"intro,rooms,fade,stairs,check,fade,pit,gauntlet"),
            W("catacombs","Catacumbas Afogadas","#081018","#102030","#243040","#486070","water",new[]{"drowned","skelly"},"bonefish","drownedking",208,"intro,crypt,water,stairs,check,crypt,isles,gauntlet"),
            W("woods","Bosque do Lobisomem","#10080c","#281018","#2a2018","#5a4030","thorn",new[]{"wolfpup","cultist"},"crow","alpha",210,"intro,hills,trees,pit,check,canopy,gauntlet,hills"),
            W("peak","Pico do Dragão","#180c10","#301820","#4a3030","#8a5050","spike",new[]{"hatchling","wyvern"},"wyvern","dragon",214,"intro,cliffs,wind,stairs,check,cliffs,pit,gauntlet"),
            W("cathedral","Catedral Demoníaca","#120810","#241028","#2a2030","#6a4058","spike",new[]{"priest","hellhound"},"bat","cathedral",212,"intro,hall,towers,stairs,check,nave,pit,gauntlet"),
            W("village","Vila da Lua de Sangue","#18080c","#301018","#3a2820","#8a5040","spike",new[]{"plague","bloodgolem"},"bat","priestess",206,"intro,roofs,street,pit,check,roofs,stairs,gauntlet"),
            W("abyss","Cavernas do Abismo","#080610","#140c24","#1a1830","#403868","spike",new[]{"crawler","serpent"},"serpent","leviathan",216,"intro,cave,isles,stairs,check,cave,pit,gauntlet"),
            // santuário antes da travessia de lava: depois dela, cada morte devolvia o jogador ao início
            W("throne","Trono Infernal","#140404","#2a0808","#3a1010","#8a2020","lava",new[]{"acolyte","shade"},"shade","warden",220,"intro,gauntlet,hall,check,lava,gauntlet,stairs,finalgate")
        };

        static EnemyDef E(string id, string n, int hp, int w, int h, float sp, string ai, int dmg, int sc, bool fly, string shot = null) =>
            new() { Id = id, Name = n, Hp = hp, W = w, H = h, Speed = sp, Ai = ai, Dmg = dmg, Score = sc, Fly = fly, Shot = shot };

        static BossDef B(string id, string n, int hp, int w, int h, float sp, int sc, string pat) =>
            new() { Id = id, Name = n, Hp = hp, W = w, H = h, Speed = sp, Score = sc, Pattern = pat };

        static WorldDef W(string id, string n, string tone, string fog, string g, string lip, string hz, string[] en, string flyer, string boss, int cols, string beats) =>
            new() { Id = id, Name = n, Tone = tone, Fog = fog, Ground = g, Lip = lip, Hazard = hz, Enemies = en, Flyer = flyer, Boss = boss, Cols = cols, Beats = beats.Split(',') };

        public static EnemyDef Enemy(string id) => Enemies.Find(e => e.Id == id);
        public static BossDef Boss(string id) =>
            string.IsNullOrEmpty(id) ? null : id == "belial" ? Belial : Bosses.Find(b => b.Id == id);

        /// <summary>Stable catalog slot, used by the LAN snapshot so the client can build summoned mobs.</summary>
        public static int EnemyIndex(string id)
        {
            for (int i = 0; i < Enemies.Count; i++) if (Enemies[i].Id == id) return i;
            return 0;
        }
        public static EnemyDef EnemyAt(int i) => i >= 0 && i < Enemies.Count ? Enemies[i] : null;
        public static int BossIndex(string id)
        {
            if (id == "belial") return 100;
            for (int i = 0; i < Bosses.Count; i++) if (Bosses[i].Id == id) return i;
            return 0;
        }
        public static BossDef BossAt(int i) => i == 100 ? Belial : (i >= 0 && i < Bosses.Count ? Bosses[i] : null);
    }
}
