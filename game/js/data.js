const TILE = 40;
const VIEW_W = 960;
const VIEW_H = 540;

const T = {
  EMPTY: 0,
  SOLID: 1,
  PLATFORM: 2,
  SPIKE: 3,
  LAVA: 4,
  ICE: 5,
  WATER: 6,
  BREAK: 7,
  BOUNCE: 8,
  LADDER: 9,
  THORN: 10
};

const HEROES = {
  mike: {
    id: "mike",
    name: "Mike",
    title: "Caçador da Lâmina",
    blurb: "Espada pesada, mais vida, golpe corpo a corpo que corta vários inimigos.",
    hp: 6,
    speed: 3.55,
    jump: -12.2,
    atkKind: "melee",
    atkTime: 0.28,
    atkCd: 0.34,
    damage: 2,
    reach: 46
  },
  denyse: {
    id: "denyse",
    name: "Denyse",
    title: "Bruxa do Cristal",
    blurb: "Cajado arcano, mais rápida, disparos de longo alcance e queda mais lenta.",
    hp: 5,
    speed: 3.85,
    jump: -11.8,
    atkKind: "magic",
    atkTime: 0.22,
    atkCd: 0.38,
    damage: 2,
    reach: 220
  }
};

const ENEMIES = [
  { id: "wraith", name: "Espectro da Floresta", hp: 2, w: 38, h: 50, speed: 0.75, ai: "patrol", dmg: 1, score: 120, fly: false },
  { id: "imp", name: "Diabrete de Espinhos", hp: 1, w: 32, h: 34, speed: 1.35, ai: "jump", dmg: 1, score: 80, fly: false },
  { id: "crow", name: "Corvo Amaldiçoado", hp: 1, w: 36, h: 28, speed: 1.6, ai: "swoop", dmg: 1, score: 90, fly: true },
  { id: "doll", name: "Boneca da Cabana", hp: 2, w: 30, h: 40, speed: 0.9, ai: "charge", dmg: 1, score: 140, fly: false },
  { id: "slime", name: "Limo do Pântano", hp: 3, w: 40, h: 28, speed: 0.45, ai: "patrol", dmg: 1, score: 100, fly: false },
  { id: "leech", name: "Sanguessuga do Brejo", hp: 2, w: 36, h: 22, speed: 1.2, ai: "fly", dmg: 1, score: 110, fly: true },
  { id: "mummy", name: "Múmia Andarilha", hp: 4, w: 36, h: 50, speed: 0.55, ai: "patrol", dmg: 1, score: 160, fly: false },
  { id: "scorpion", name: "Escorpião de Cinzas", hp: 2, w: 42, h: 28, speed: 1.4, ai: "charge", dmg: 1, score: 130, fly: false },
  { id: "skelly", name: "Arqueiro de Ossos", hp: 2, w: 34, h: 48, speed: 0.5, ai: "shoot", dmg: 1, score: 170, fly: false, shot: "bone" },
  { id: "ghoul", name: "Carniçal da Cova", hp: 3, w: 38, h: 46, speed: 1.05, ai: "charge", dmg: 1, score: 150, fly: false },
  { id: "bat", name: "Morcego Vampiro", hp: 1, w: 34, h: 26, speed: 1.7, ai: "swoop", dmg: 1, score: 90, fly: true },
  { id: "thrall", name: "Servo de Sangue", hp: 3, w: 36, h: 48, speed: 0.85, ai: "patrol", dmg: 1, score: 150, fly: false },
  { id: "icewraith", name: "Espectro de Gelo", hp: 2, w: 36, h: 48, speed: 0.8, ai: "fly", dmg: 1, score: 140, fly: true },
  { id: "frostknight", name: "Cavaleiro Congelado", hp: 5, w: 40, h: 52, speed: 0.6, ai: "patrol", dmg: 2, score: 220, fly: false },
  { id: "fireimp", name: "Diabrete de Fogo", hp: 2, w: 30, h: 34, speed: 1.3, ai: "jump", dmg: 1, score: 120, fly: false },
  { id: "magmagolem", name: "Golem de Magma", hp: 6, w: 46, h: 52, speed: 0.4, ai: "tank", dmg: 2, score: 260, fly: false },
  { id: "familiar", name: "Familiar da Coven", hp: 1, w: 30, h: 26, speed: 1.8, ai: "swoop", dmg: 1, score: 100, fly: true },
  { id: "hexwitch", name: "Bruxa do Hex", hp: 3, w: 36, h: 48, speed: 0.7, ai: "mage", dmg: 1, score: 200, fly: false, shot: "hex" },
  { id: "drowned", name: "Afogado", hp: 3, w: 36, h: 46, speed: 0.65, ai: "patrol", dmg: 1, score: 140, fly: false },
  { id: "bonefish", name: "Peixe de Ossos", hp: 2, w: 40, h: 24, speed: 1.5, ai: "fly", dmg: 1, score: 120, fly: true },
  { id: "wolfpup", name: "Lobisomem Jovem", hp: 3, w: 42, h: 36, speed: 1.55, ai: "charge", dmg: 1, score: 180, fly: false },
  { id: "cultist", name: "Cultista da Lua", hp: 2, w: 34, h: 48, speed: 0.75, ai: "shoot", dmg: 1, score: 160, fly: false, shot: "dark" },
  { id: "hatchling", name: "Filhote de Dragão", hp: 3, w: 40, h: 32, speed: 1.1, ai: "jump", dmg: 1, score: 190, fly: false },
  { id: "wyvern", name: "Serpe Batedora", hp: 3, w: 48, h: 32, speed: 1.45, ai: "swoop", dmg: 1, score: 210, fly: true },
  { id: "priest", name: "Sacerdote Possuído", hp: 4, w: 36, h: 50, speed: 0.7, ai: "mage", dmg: 1, score: 220, fly: false, shot: "holy" },
  { id: "hellhound", name: "Cão do Inferno", hp: 3, w: 44, h: 32, speed: 1.7, ai: "charge", dmg: 2, score: 200, fly: false },
  { id: "plague", name: "Aldeão da Peste", hp: 2, w: 34, h: 46, speed: 0.8, ai: "patrol", dmg: 1, score: 130, fly: false },
  { id: "bloodgolem", name: "Golem de Sangue", hp: 6, w: 46, h: 52, speed: 0.45, ai: "tank", dmg: 2, score: 280, fly: false },
  { id: "crawler", name: "Rastejante do Abismo", hp: 2, w: 40, h: 24, speed: 1.25, ai: "patrol", dmg: 1, score: 150, fly: false },
  { id: "serpent", name: "Serpente das Sombras", hp: 4, w: 52, h: 28, speed: 1.15, ai: "fly", dmg: 1, score: 200, fly: true },
  { id: "acolyte", name: "Acólito Demoníaco", hp: 3, w: 36, h: 48, speed: 0.8, ai: "mage", dmg: 1, score: 230, fly: false, shot: "hell" },
  { id: "shade", name: "Sombra do Pesadelo", hp: 4, w: 38, h: 50, speed: 1.1, ai: "swoop", dmg: 2, score: 250, fly: true }
];

const BOSSES = [
  { id: "treant", name: "O Ent Oco", hp: 28, w: 90, h: 110, speed: 0.55, score: 2000, pattern: "slam" },
  { id: "babawitch", name: "Baba da Cabana", hp: 26, w: 70, h: 90, speed: 0.7, score: 2200, pattern: "hex" },
  { id: "hydra", name: "Hidra do Pântano", hp: 34, w: 100, h: 80, speed: 0.6, score: 2400, pattern: "multi" },
  { id: "pharaoh", name: "Faraó de Cinzas", hp: 32, w: 78, h: 96, speed: 0.5, score: 2500, pattern: "summon" },
  { id: "gravetitan", name: "Titã da Cova", hp: 36, w: 96, h: 108, speed: 0.4, score: 2600, pattern: "slam" },
  { id: "nosferatu", name: "Conde Nosferatu", hp: 34, w: 70, h: 96, speed: 1.1, score: 2800, pattern: "bat" },
  { id: "icequeen", name: "Rainha de Gelo", hp: 32, w: 72, h: 96, speed: 0.75, score: 2800, pattern: "ice" },
  { id: "magmatitan", name: "Titã de Magma", hp: 40, w: 100, h: 110, speed: 0.45, score: 3000, pattern: "lava" },
  { id: "morgana", name: "Alta Bruxa Morgana", hp: 36, w: 74, h: 98, speed: 0.8, score: 3200, pattern: "hex" },
  { id: "drownedking", name: "Rei Afogado", hp: 38, w: 86, h: 100, speed: 0.55, score: 3200, pattern: "wave" },
  { id: "alpha", name: "Lobisomem Alfa", hp: 36, w: 92, h: 86, speed: 1.35, score: 3400, pattern: "charge" },
  { id: "dragon", name: "Dragão Carmesim", hp: 44, w: 120, h: 90, speed: 0.9, score: 3800, pattern: "flyfire" },
  { id: "cathedral", name: "Demônio da Catedral", hp: 42, w: 96, h: 112, speed: 0.7, score: 3800, pattern: "slam" },
  { id: "priestess", name: "Sacerdotisa de Sangue", hp: 38, w: 74, h: 98, speed: 0.85, score: 3600, pattern: "hex" },
  { id: "leviathan", name: "Leviatã do Abismo", hp: 46, w: 130, h: 80, speed: 0.7, score: 4000, pattern: "wave" },
  { id: "warden", name: "Guardião Infernal", hp: 42, w: 100, h: 110, speed: 0.65, score: 4200, pattern: "multi" }
];

const FINAL_BOSS = {
  id: "belial",
  name: "Belial, Senhor dos Pesadelos",
  hp: 72,
  w: 140,
  h: 130,
  speed: 0.85,
  score: 12000,
  pattern: "final"
};

const WORLDS = [
  { id: "forest", name: "Floresta Amaldiçoada", tone: "#0b1a10", fog: "#143018", ground: "#2a4a22", lip: "#6a8a38", hazard: "thorn",
    enemies: ["wraith", "imp"], flyer: "crow", boss: "treant", cols: 196, beats: ["intro", "hills", "pit", "trees", "check", "canopy", "pit", "gauntlet"] },
  { id: "cabin", name: "Cabana da Bruxa", tone: "#1a1010", fog: "#2a1814", ground: "#4a3020", lip: "#7a5030", hazard: "spike",
    enemies: ["doll", "imp"], flyer: "crow", boss: "babawitch", cols: 188, beats: ["intro", "rooms", "stairs", "attic", "check", "rooms", "pit", "gauntlet"] },
  { id: "swamp", name: "Pântano de Ossos", tone: "#0c1814", fog: "#1a3028", ground: "#2a3a28", lip: "#4a6a40", hazard: "water",
    enemies: ["slime", "drowned"], flyer: "leech", boss: "hydra", cols: 200, beats: ["intro", "water", "isles", "pit", "check", "water", "hills", "gauntlet"] },
  { id: "desert", name: "Deserto de Cinzas", tone: "#24180c", fog: "#3a2814", ground: "#6a4a24", lip: "#c4a060", hazard: "spike",
    enemies: ["mummy", "scorpion"], flyer: "crow", boss: "pharaoh", cols: 208, beats: ["intro", "dunes", "pit", "ruins", "check", "dunes", "stairs", "gauntlet"] },
  { id: "grave", name: "Cemitério Sombrio", tone: "#101018", fog: "#1c1c28", ground: "#2a2a30", lip: "#5a5a48", hazard: "spike",
    enemies: ["skelly", "ghoul"], flyer: "bat", boss: "gravetitan", cols: 200, beats: ["intro", "tombs", "pit", "crypt", "check", "tombs", "stairs", "gauntlet"] },
  { id: "castle", name: "Castelo Vampírico", tone: "#140810", fog: "#2a1020", ground: "#3a2030", lip: "#6a3048", hazard: "spike",
    enemies: ["thrall", "skelly"], flyer: "bat", boss: "nosferatu", cols: 210, beats: ["intro", "hall", "stairs", "towers", "check", "hall", "pit", "gauntlet"] },
  { id: "ice", name: "Ermos Congelados", tone: "#0c1824", fog: "#203848", ground: "#3a5a6a", lip: "#b0d4e4", hazard: "ice",
    enemies: ["icewraith", "frostknight"], flyer: "icewraith", boss: "icequeen", cols: 204, beats: ["intro", "icefields", "pit", "stairs", "check", "icefields", "canopy", "gauntlet"] },
  { id: "volcano", name: "Inferno Vulcânico", tone: "#1a0804", fog: "#3a1408", ground: "#4a2010", lip: "#c05020", hazard: "lava",
    enemies: ["fireimp", "magmagolem"], flyer: "crow", boss: "magmatitan", cols: 206, beats: ["intro", "lava", "isles", "stairs", "check", "lava", "pit", "gauntlet"] },
  { id: "coven", name: "Covil das Bruxas", tone: "#14081a", fog: "#281440", ground: "#302044", lip: "#7040a0", hazard: "thorn",
    enemies: ["hexwitch", "familiar"], flyer: "familiar", boss: "morgana", cols: 198, beats: ["intro", "rooms", "fade", "stairs", "check", "fade", "pit", "gauntlet"] },
  { id: "catacombs", name: "Catacumbas Afogadas", tone: "#081018", fog: "#102030", ground: "#243040", lip: "#486070", hazard: "water",
    enemies: ["drowned", "skelly"], flyer: "bonefish", boss: "drownedking", cols: 208, beats: ["intro", "crypt", "water", "stairs", "check", "crypt", "isles", "gauntlet"] },
  { id: "woods", name: "Bosque do Lobisomem", tone: "#10080c", fog: "#281018", ground: "#2a2018", lip: "#5a4030", hazard: "thorn",
    enemies: ["wolfpup", "cultist"], flyer: "crow", boss: "alpha", cols: 210, beats: ["intro", "hills", "trees", "pit", "check", "canopy", "gauntlet", "hills"] },
  { id: "peak", name: "Pico do Dragão", tone: "#180c10", fog: "#301820", ground: "#4a3030", lip: "#8a5050", hazard: "spike",
    enemies: ["hatchling", "wyvern"], flyer: "wyvern", boss: "dragon", cols: 214, beats: ["intro", "cliffs", "wind", "stairs", "check", "cliffs", "pit", "gauntlet"] },
  { id: "cathedral", name: "Catedral Demoníaca", tone: "#120810", fog: "#241028", ground: "#2a2030", lip: "#6a4058", hazard: "spike",
    enemies: ["priest", "hellhound"], flyer: "bat", boss: "cathedral", cols: 212, beats: ["intro", "hall", "towers", "stairs", "check", "nave", "pit", "gauntlet"] },
  { id: "village", name: "Vila da Lua de Sangue", tone: "#18080c", fog: "#301018", ground: "#3a2820", lip: "#8a5040", hazard: "spike",
    enemies: ["plague", "bloodgolem"], flyer: "bat", boss: "priestess", cols: 206, beats: ["intro", "roofs", "street", "pit", "check", "roofs", "stairs", "gauntlet"] },
  { id: "abyss", name: "Cavernas do Abismo", tone: "#080610", fog: "#140c24", ground: "#1a1830", lip: "#403868", hazard: "spike",
    enemies: ["crawler", "serpent"], flyer: "serpent", boss: "leviathan", cols: 216, beats: ["intro", "cave", "isles", "stairs", "check", "cave", "pit", "gauntlet"] },
  { id: "throne", name: "Trono Infernal", tone: "#140404", fog: "#2a0808", ground: "#3a1010", lip: "#8a2020", hazard: "lava",
    enemies: ["acolyte", "shade"], flyer: "shade", boss: "warden", cols: 220, beats: ["intro", "gauntlet", "hall", "lava", "check", "gauntlet", "stairs", "finalgate"] }
];

function enemyById(id) { return ENEMIES.find((e) => e.id === id); }
function bossById(id) { return id === "belial" ? FINAL_BOSS : BOSSES.find((b) => b.id === id); }
