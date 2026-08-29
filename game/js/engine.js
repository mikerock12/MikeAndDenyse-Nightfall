/* Mike & Denyse: Nightfall — engine */
const $ = (id) => document.getElementById(id);

const Game = {
  state: "boot",
  canvas: null,
  ctx: null,
  sprites: {},
  atlases: {},
  worldIndex: 0,
  level: null,
  players: [],
  enemies: [],
  bosses: [],
  projectiles: [],
  items: [],
  particles: [],
  floaters: [],
  camera: { x: 0, y: 0, shake: 0 },
  score: 0,
  souls: 0,
  time: 0,
  p1Hero: "mike",
  p2Hero: "denyse",
  unlocked: 16,
  continues: 3,
  fade: 1,
  fadeDir: -1,
  message: "",
  messageT: 0,
  bossIntro: 0,
  lockedArena: false,
  belialPhase: false,
  last: 0,
  acc: 0
};

function clamp(v, a, b) { return v < a ? a : v > b ? b : v; }
function aabb(a, b) {
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
function irand(n) { return Math.floor(Math.random() * n); }

function showScreen(id) {
  $("overlay").classList.remove("hidden");
  document.querySelectorAll("#overlay > .panel").forEach((p) => p.classList.add("hidden"));
  $(id).classList.remove("hidden");
}
function hideOverlay() { $("overlay").classList.add("hidden"); }

function loadImage(src) {
  return new Promise((resolve) => {
    const im = new Image();
    im.onload = () => resolve(im);
    im.onerror = () => resolve(null);
    im.src = src;
  });
}

async function loadEither(base) {
  const png = await loadImage(base + ".png");
  if (png) return png;
  return loadImage(base + ".jpg");
}

async function loadAllSprites() {
  const jobs = [];
  const put = (key, path) => {
    jobs.push(loadEither(path).then((im) => { Game.sprites[key] = im; }));
  };

  for (const id of ["mike", "denyse"]) {
    put(id + "_idle", "assets/chars/" + id + "_idle");
    put(id + "_walk1", "assets/chars/" + id + "_walk1");
    put(id + "_walk2", "assets/chars/" + id + "_walk2");
    put(id + "_jump", "assets/chars/" + id + "_jump");
    put(id + "_attack", "assets/chars/" + id + "_attack");
  }
  for (const e of ENEMIES) put("en_" + e.id, "assets/enemies/" + e.id);
  for (const b of BOSSES) put("boss_" + b.id, "assets/bosses/" + b.id);
  put("boss_belial", "assets/bosses/belial");
  for (const w of WORLDS) {
    put("bg_" + w.id, "assets/worlds/" + w.id + "_bg");
    put("ground_" + w.id, "assets/worlds/" + w.id + "_ground");
  }
  for (const it of ["soul", "heart", "key", "bell"]) put("it_" + it, "assets/items/" + it);
  put("fx_slash", "assets/fx/slash");
  put("fx_bolt", "assets/fx/bolt");
  put("ui_title", "assets/ui/title");

  await Promise.all(jobs);
}

function hash2(x, y) {
  let n = x * 374761393 + y * 668265263;
  n = (n ^ (n >> 13)) * 1274126177;
  return ((n ^ (n >> 16)) >>> 0) / 4294967296;
}

function buildAtlas(world) {
  const n = 11;
  const c = document.createElement("canvas");
  c.width = TILE * n;
  c.height = TILE;
  const g = c.getContext("2d");
  const pal = {
    1: [world.ground, world.lip],
    2: [world.lip, "#d8c878"],
    3: ["#4a2030", "#c04050"],
    4: ["#4a1008", "#ff6020"],
    5: ["#8ec8dc", "#e8f6ff"],
    6: ["#143848", "#2a7088"],
    7: ["#3a2a20", "#8a6a40"],
    8: ["#305028", "#80d060"],
    9: ["#4a3020", "#c4a060"],
    10: ["#203010", "#68a030"]
  };
  const hexTo = (h) => {
    const s = h.replace("#", "");
    return [parseInt(s.slice(0, 2), 16), parseInt(s.slice(2, 4), 16), parseInt(s.slice(4, 6), 16)];
  };
  for (let t = 1; t < n; t++) {
    const pair = pal[t] || [world.ground, world.lip];
    const a = hexTo(pair[0]);
    const b = hexTo(pair[1]);
    const ox = t * TILE;
    for (let y = 0; y < TILE; y++) {
      for (let x = 0; x < TILE; x++) {
        const nse = hash2(x * 3 + t * 17, y * 5 + t * 9);
        const nse2 = hash2(x + 40, y + t * 13);
        const top = y < 8;
        const src = top ? b : a;
        const k = top ? 0.25 : 0.18;
        const r = clamp(src[0] + (nse - 0.5) * 70 + (top ? 18 : 0), 0, 255);
        const gg = clamp(src[1] + (nse2 - 0.5) * 60, 0, 255);
        const bb = clamp(src[2] + (nse - 0.45) * 50, 0, 255);
        g.fillStyle = `rgb(${r | 0},${gg | 0},${bb | 0})`;
        g.fillRect(ox + x, y, 1, 1);
        if (nse > 0.86) {
          g.fillStyle = `rgba(255,230,180,${k})`;
          g.fillRect(ox + x, y, 1, 1);
        }
        if (nse2 < 0.08) {
          g.fillStyle = "rgba(0,0,0,0.28)";
          g.fillRect(ox + x, y, 1, 1);
        }
      }
    }
    if (t === T.SPIKE || t === T.THORN) {
      g.fillStyle = t === T.SPIKE ? "#d8d8e8" : "#6a8a30";
      for (let i = 0; i < 4; i++) {
        g.beginPath();
        g.moveTo(ox + 4 + i * 10, TILE);
        g.lineTo(ox + 8 + i * 10, 8);
        g.lineTo(ox + 12 + i * 10, TILE);
        g.fill();
      }
    }
    if (t === T.LADDER) {
      g.strokeStyle = "#c4a060";
      g.lineWidth = 3;
      g.strokeRect(ox + 8, 2, TILE - 16, TILE - 4);
      for (let y = 8; y < TILE; y += 10) {
        g.beginPath();
        g.moveTo(ox + 8, y);
        g.lineTo(ox + TILE - 8, y);
        g.stroke();
      }
    }
  }
  const tex = Game.sprites["ground_" + world.id];
  if (tex) {
    const pat = g.createPattern(tex, "repeat");
    if (pat) {
      g.globalAlpha = 0.85;
      g.fillStyle = pat;
      g.fillRect(TILE, 0, TILE, TILE);
      g.fillRect(T.ICE * TILE, 0, TILE, TILE);
      g.globalAlpha = 1;
    }
  }
  return c;
}

function tileAt(level, px, py) {
  const tx = Math.floor(px / TILE);
  const ty = Math.floor(py / TILE);
  if (ty >= level.rows) return T.EMPTY;
  if (tx < 0 || ty < 0 || tx >= level.cols) return T.SOLID;
  return level.tiles[ty][tx];
}

function solidAt(level, px, py, falling) {
  const t = tileAt(level, px, py);
  if (t === T.SOLID || t === T.ICE || t === T.BREAK) return t;
  const ly = ((py % TILE) + TILE) % TILE;
  if (t === T.PLATFORM && falling && ly < 10) return t;
  if (t === T.BOUNCE && falling && ly < 14) return t;
  return 0;
}

function spawnPlayer(slot, heroId, x, y) {
  const def = HEROES[heroId];
  return {
    slot,
    hero: def,
    x, y,
    w: 30,
    h: 48,
    vx: 0, vy: 0,
    facing: 1,
    onGround: false,
    coyote: 0,
    jumpBuf: 0,
    jumps: 0,
    hp: def.hp,
    maxHp: def.hp,
    inv: 0,
    atk: 0,
    atkCd: 0,
    dead: false,
    anim: "idle",
    animT: 0,
    hurtT: 0,
    lives: 3,
    spawnX: x,
    spawnY: y
  };
}

function spawnEnemy(spec) {
  const def = enemyById(spec.id);
  const dir = Math.random() > 0.5 ? 1 : -1;
  return {
    kind: "enemy",
    def,
    id: spec.id,
    x: spec.x,
    y: spec.y,
    w: def.w,
    h: def.h,
    vx: dir * def.speed,
    vy: 0,
    dir,
    facing: dir,
    flipCd: 0.15,
    hp: def.hp,
    dead: false,
    onGround: false,
    t: Math.random() * 10,
    shotT: 1 + Math.random(),
    stun: 0,
    flash: 0,
    homeX: spec.x,
    homeY: spec.y
  };
}

function spawnBoss(def, x, y) {
  return {
    kind: "boss",
    def,
    id: def.id,
    x: x - def.w / 2,
    y: y - def.h,
    w: def.w,
    h: def.h,
    vx: 0, vy: 0,
    facing: -1,
    hp: def.hp,
    maxHp: def.hp,
    dead: false,
    onGround: false,
    t: 0,
    phase: 0,
    state: "idle",
    stateT: 1.2,
    flash: 0,
    intro: 1.6
  };
}

function burst(x, y, color, n, spd) {
  for (let i = 0; i < n; i++) {
    const a = Math.random() * Math.PI * 2;
    const s = (spd || 80) * (0.3 + Math.random());
    Game.particles.push({
      x, y, vx: Math.cos(a) * s, vy: Math.sin(a) * s - 30,
      life: 0.35 + Math.random() * 0.35, max: 0.7,
      c: color, s: 2 + Math.random() * 3
    });
  }
}

function floater(x, y, text, c) {
  Game.floaters.push({ x, y, text, c: c || "#ffe9a8", t: 0.9 });
}

function shake(n) { Game.camera.shake = Math.max(Game.camera.shake, n); }

function livingPlayers() { return Game.players.filter((p) => !p.dead); }

function nearestPlayer(x, y) {
  let best = null, bd = 1e9;
  for (const p of livingPlayers()) {
    const d = Math.abs(p.x - x) + Math.abs(p.y - y);
    if (d < bd) { bd = d; best = p; }
  }
  return best;
}

function applyPhysics(a, level, dt, opts) {
  opts = opts || {};
  const grav = opts.grav == null ? 26 : opts.grav;
  const fly = !!opts.fly;
  if (!fly) a.vy += grav * dt;
  if (a.vy > 16) a.vy = 16;

  const ice = tileAt(level, a.x + a.w / 2, a.y + a.h + 1) === T.ICE;
  if (ice && a.onGround && a.vx) a.vx *= 0.992;

  a.x += a.vx * 92 * dt * (opts.xScale || 1);
  const feetY = a.y + a.h - 2;
  const headY = a.y + 6;
  if (a.vx > 0) {
    if (solidAt(level, a.x + a.w, feetY, false) || solidAt(level, a.x + a.w, headY, false)) {
      a.x = Math.floor((a.x + a.w) / TILE) * TILE - a.w - 0.01;
      a.vx = 0;
      a._wall = 1;
    }
  } else if (a.vx < 0) {
    if (solidAt(level, a.x, feetY, false) || solidAt(level, a.x, headY, false)) {
      a.x = Math.floor(a.x / TILE) * TILE + TILE + 0.01;
      a.vx = 0;
      a._wall = -1;
    }
  } else a._wall = 0;

  a.y += a.vy * 72 * dt;
  a.onGround = false;
  const falling = a.vy >= 0;
  const mid = a.x + a.w / 2;
  if (falling) {
    if (solidAt(level, a.x + 4, a.y + a.h, true) || solidAt(level, a.x + a.w - 4, a.y + a.h, true) || solidAt(level, mid, a.y + a.h, true)) {
      a.y = Math.floor((a.y + a.h) / TILE) * TILE - a.h - 0.01;
      if (a.vy > 8) burst(mid, a.y + a.h, "#c8b090", 4, 40);
      a.vy = 0;
      a.onGround = true;
    }
  } else {
    if (solidAt(level, a.x + 4, a.y + 2, false) || solidAt(level, a.x + a.w - 4, a.y + 2, false)) {
      a.y = Math.floor(a.y / TILE) * TILE + TILE + 0.01;
      a.vy = 0;
    }
  }

  const water = tileAt(level, mid, a.y + a.h * 0.6) === T.WATER;
  if (water) {
    a.vy *= 0.86;
    a.vx *= 0.9;
    a._water = true;
  } else a._water = false;
}

function hurtPlayer(p, dmg, srcX) {
  if (p.dead || p.inv > 0) return;
  p.hp -= dmg;
  p.inv = 1.15;
  p.hurtT = 0.25;
  p.vx = (p.x + p.w / 2 < srcX ? -1 : 1) * 3.2;
  p.vy = -6;
  AudioSys.sfx("hurt");
  shake(8);
  burst(p.x + p.w / 2, p.y + p.h / 2, "#ff6677", 10, 90);
  if (p.hp <= 0) {
    p.hp = 0;
    p.lives -= 1;
    if (p.lives < 0) {
      p.dead = true;
      AudioSys.sfx("die");
      burst(p.x + p.w / 2, p.y + p.h / 2, "#ff2244", 22, 140);
    } else {
      p.hp = p.maxHp;
      p.x = p.spawnX;
      p.y = p.spawnY;
      p.inv = 2;
      floater(p.x, p.y, "renasce", "#ffd0a0");
    }
  }
}

function killEnemy(e) {
  if (e.dead) return;
  e.dead = true;
  Game.score += e.def.score;
  Game.souls += 1;
  AudioSys.sfx("stomp");
  burst(e.x + e.w / 2, e.y + e.h / 2, "#c060ff", 14, 110);
  floater(e.x, e.y, "+" + e.def.score, "#e8c6ff");
  if (Math.random() < 0.12) Game.items.push({ type: "soul", x: e.x + 8, y: e.y, vx: 0, vy: -2, bob: 0 });
}

function damageEnemy(e, dmg) {
  if (e.dead || e.flash > 0.05 && e.hp <= 0) return;
  e.hp -= dmg;
  e.flash = 0.12;
  e.stun = 0.12;
  shake(3);
  AudioSys.sfx("hit");
  if (e.hp <= 0) {
    if (e.kind === "boss") {
      e.dead = true;
      Game.score += e.def.score;
      Game.souls += 15;
      AudioSys.sfx("clear");
      burst(e.x + e.w / 2, e.y + e.h / 2, "#ffd060", 40, 180);
      Game.message = e.def.name + " caiu! Sigam o sino dourado.";
      Game.messageT = 2.8;
      onBossDead(e);
    } else killEnemy(e);
  }
}

function onBossDead(b) {
  if (Game.level.worldIndex === 15 && b.id !== "belial" && !Game.belialPhase) {
    Game.belialPhase = true;
    Game.bossIntro = 2.2;
    Game.message = "Belial desperta do trono!";
    Game.messageT = 2.5;
    AudioSys.sfx("boss");
    AudioSys.setTheme("final");
    setTimeout(() => {
      const def = FINAL_BOSS;
      const arena = Game.level;
      Game.bosses.push(spawnBoss(def, (arena.arenaX0 + arena.arenaX1) / 2, arena.bossAt.y));
    }, 900);
    return;
  }
  if (b.id === "belial") {
    Game.state = "win";
    persist();
    $("win-body").textContent = "Mike e Denyse atravessaram dezesseis noites e o próprio pesadelo. Almas: " + Game.souls + " · Pontos: " + Game.score + ".";
    showScreen("screen-win");
    AudioSys.setTheme("title");
  }
}

function fireMagic(p) {
  const dir = p.facing;
  Game.projectiles.push({
    kind: "magic",
    x: p.x + (dir > 0 ? p.w : -16),
    y: p.y + 14,
    w: 18, h: 12,
    vx: dir * 14,
    vy: 0,
    life: 0.7,
    dmg: p.hero.damage,
    friendly: true,
    owner: p
  });
  AudioSys.sfx("magic");
}

function fireShot(e, kind) {
  const p = nearestPlayer(e.x, e.y);
  if (!p) return;
  const dx = (p.x + p.w / 2) - (e.x + e.w / 2);
  const dy = (p.y + p.h / 2) - (e.y + 16);
  const len = Math.hypot(dx, dy) || 1;
  Game.projectiles.push({
    kind: kind || "dark",
    x: e.x + e.w / 2,
    y: e.y + 16,
    w: 12, h: 12,
    vx: (dx / len) * 4.2,
    vy: (dy / len) * 4.2,
    life: 2.4,
    dmg: 1,
    friendly: false
  });
}

function meleeHit(p) {
  const box = {
    x: p.facing > 0 ? p.x + p.w - 4 : p.x - p.hero.reach,
    y: p.y + 10,
    w: p.hero.reach,
    h: 28
  };
  for (const e of Game.enemies) {
    if (!e.dead && aabb(box, e)) damageEnemy(e, p.hero.damage);
  }
  for (const b of Game.bosses) {
    if (!b.dead && aabb(box, b)) damageEnemy(b, p.hero.damage);
  }
  burst(box.x + box.w / 2, box.y + 10, "#ffe8a0", 6, 60);
  AudioSys.sfx("attack");
}

function updatePlayer(p, dt) {
  if (p.dead) return;
  const level = Game.level;
  p.atk = Math.max(0, p.atk - dt);
  p.atkCd = Math.max(0, p.atkCd - dt);
  p.inv = Math.max(0, p.inv - dt);
  p.hurtT = Math.max(0, p.hurtT - dt);
  p.animT += dt;
  p.coyote = p.onGround ? 0.1 : p.coyote - dt;
  const ctl = (typeof Net !== "undefined") ? Net.readCtl(p.slot) : {
    l: Input.left(p.slot), r: Input.right(p.slot), d: Input.downNow(p.slot),
    jp: Input.jumpPressed(p.slot), jn: Input.jumpDown(p.slot), ap: Input.attackPressed(p.slot)
  };
  if (ctl.jp) p.jumpBuf = 0.12;
  else p.jumpBuf -= dt;
  let ax = 0;
  if (ctl.l) ax -= 1;
  if (ctl.r) ax += 1;
  if (ax) p.facing = ax;

  const spd = p.hero.speed * (p._water ? 0.7 : 1);
  const target = ax * spd;
  const ice = tileAt(level, p.x + p.w / 2, p.y + p.h + 1) === T.ICE;
  const acc = p.onGround ? (ice ? 6 : 18) : 10;
  p.vx += (target - p.vx) * Math.min(1, acc * dt);

  for (const z of level.zones) {
    if (z.type === "wind" && p.x > z.x0 && p.x < z.x1) p.vx += z.vx * dt * 3;
  }

  if (p.jumpBuf > 0 && (p.coyote > 0 || (p.jumps < 1 && !p.onGround) || p._water)) {
    p.vy = p.hero.jump * (p._water ? 0.55 : 1);
    p.onGround = false;
    p.coyote = 0;
    p.jumpBuf = 0;
    p.jumps += 1;
    AudioSys.sfx("jump");
  }
  if (p.onGround) p.jumps = 0;
  if (!ctl.jn && p.vy < -3) p.vy += 28 * dt;

  if (tileAt(level, p.x + p.w / 2, p.y + p.h / 2) === T.LADDER && (ctl.jn || ctl.d)) {
    p.vy = ctl.d ? 3 : -3.4;
    p.jumps = 0;
  }

  if (p.hero.id === "denyse" && !p.onGround && ctl.jn && p.vy > 1) {
    p.vy = Math.min(p.vy, 3.2);
  }

  applyPhysics(p, level, dt, { grav: p.hero.id === "denyse" ? 23 : 26 });
  if (p.onGround && tileAt(level, p.x + p.w / 2, p.y + p.h + 2) === T.BOUNCE) {
    p.vy = -11.2;
    p.onGround = false;
    p.jumps = 0;
    AudioSys.sfx("jump");
  }

  if (p.atkCd <= 0 && ctl.ap) {
    p.atk = p.hero.atkTime;
    p.atkCd = p.hero.atkCd;
    if (p.hero.atkKind === "magic") fireMagic(p);
    else meleeHit(p);
  }

  if (p.atk > 0) p.anim = "attack";
  else if (!p.onGround) p.anim = p.vy < 0 ? "jump" : "fall";
  else if (Math.abs(p.vx) > 0.4 && ax) p.anim = "walk";
  else p.anim = "idle";

  const mid = p.x + p.w / 2;
  const hazard = tileAt(level, mid, p.y + p.h - 4);
  if (hazard === T.SPIKE || hazard === T.LAVA || hazard === T.THORN) {
    hurtPlayer(p, hazard === T.LAVA ? 2 : 1, mid);
    p.vy = -8;
  }
  if (p.y > level.rows * TILE + 20) {
    hurtPlayer(p, 2, p.x);
    p.x = p.spawnX;
    p.y = p.spawnY;
    p.vx = p.vy = 0;
  }

  if (level.check && Math.abs(p.x - level.check.x) < 40 && Math.abs(p.y - level.check.y) < 50) {
    p.spawnX = level.check.x;
    p.spawnY = level.check.y - 10;
  }

  if (Game.lockedArena) {
    if (p.x < Game.level.arenaX0 + 8) p.x = Game.level.arenaX0 + 8;
    if (p.x + p.w > Game.level.arenaX1 - 8) p.x = Game.level.arenaX1 - 8 - p.w;
  }
}

function isBlocking(t) {
  return t === T.SOLID || t === T.ICE || t === T.BREAK;
}

function isHazardFloor(t) {
  return t === T.EMPTY || t === T.SPIKE || t === T.LAVA || t === T.WATER || t === T.THORN;
}

function liftOutOfGround(e, level) {
  for (let i = 0; i < 14; i++) {
    const t = tileAt(level, e.x + e.w * 0.5, e.y + e.h - 3);
    if (isBlocking(t) || t === T.PLATFORM) e.y -= 5;
    else break;
  }
}

function wallAhead(e, level) {
  const dir = e.dir || 1;
  const x = dir > 0 ? e.x + e.w + 4 : e.x - 4;
  return isBlocking(tileAt(level, x, e.y + e.h * 0.35)) ||
    isBlocking(tileAt(level, x, e.y + e.h * 0.7));
}

function ledgeAhead(e, level) {
  const dir = e.dir || 1;
  const x = dir > 0 ? e.x + e.w + 6 : e.x - 6;
  return isHazardFloor(tileAt(level, x, e.y + e.h + 5));
}

function atBounds(e, level) {
  return e.x < 20 || e.x + e.w > level.cols * TILE - 20;
}

function flipDir(e, cd) {
  e.dir = e.dir < 0 ? 1 : -1;
  e.facing = e.dir;
  e.flipCd = cd == null ? 0.32 : cd;
  e.vx = e.dir * Math.abs(e.vx || e.def.speed);
}

function faceWalk(e, level, dt, speed) {
  if (e.dir !== 1 && e.dir !== -1) e.dir = 1;
  e.flipCd = Math.max(0, e.flipCd - dt);
  if (e.flipCd <= 0 && (wallAhead(e, level) || (e.onGround && ledgeAhead(e, level)) || atBounds(e, level))) {
    flipDir(e, 0.36);
  }
  e.vx = e.dir * speed;
  e.facing = e.dir;
}

function collidePlayers(e) {
  if (e.dead) return;
  for (const pl of livingPlayers()) {
    if (!aabb(e, pl)) continue;
    const stomp = pl.vy > 1.2 && pl.y + pl.h < e.y + e.h * 0.58;
    if (stomp) {
      damageEnemy(e, 2);
      pl.vy = -8;
      AudioSys.sfx("stomp");
    } else {
      hurtPlayer(pl, e.def.dmg, e.x + e.w / 2);
    }
  }
}

function updateFlyer(e, dt, p, level) {
  if (e.homeX == null) e.homeX = e.x;
  if (e.homeY == null) e.homeY = e.y;
  e.flipCd = Math.max(0, e.flipCd - dt);
  const range = 86;
  if (e.flipCd <= 0) {
    if (e.x > e.homeX + range || e.x + e.w > level.cols * TILE - 24) flipDir(e, 0.25);
    else if (e.x < e.homeX - range || e.x < 24) flipDir(e, 0.25);
    else if (wallAhead(e, level)) flipDir(e, 0.3);
  }
  e.x += e.dir * e.def.speed * 68 * dt;
  const amp = e.def.ai === "swoop" ? 20 : 13;
  e.y = e.homeY + Math.sin(e.t * 2.5) * amp;
  if (e.def.ai === "swoop" && p && Math.abs(p.x - e.x) < 190) {
    e.y += Math.sign((p.y + 10) - e.y) * 32 * dt;
  }
  e.facing = e.dir;
  e.onGround = false;
  collidePlayers(e);
}

function updateEnemy(e, dt) {
  if (e.dead) return;
  const level = Game.level;
  e.t += dt;
  e.flash = Math.max(0, e.flash - dt);
  e.stun = Math.max(0, e.stun - dt);
  const def = e.def;
  const p = nearestPlayer(e.x, e.y);
  const flying = !!(def.fly || def.ai === "fly" || def.ai === "swoop");

  if (e.stun > 0) {
    if (flying) { collidePlayers(e); return; }
    applyPhysics(e, level, dt, { fly: false, grav: 26 });
    collidePlayers(e);
    return;
  }

  if (flying) {
    updateFlyer(e, dt, p, level);
    return;
  }

  liftOutOfGround(e, level);

  if (def.ai === "jump") {
    if (e.onGround && (e.t % 1.55) < dt + 0.02) e.vy = -8.6;
    faceWalk(e, level, dt, def.speed);
  } else if (def.ai === "charge") {
    if (p && Math.abs(p.y - e.y) < 58 && Math.abs(p.x - e.x) < 230) {
      const want = p.x + p.w / 2 >= e.x + e.w / 2 ? 1 : -1;
      e.flipCd = Math.max(0, e.flipCd - dt);
      if (e.flipCd <= 0 && want !== e.dir) flipDir(e, 0.4);
      if (wallAhead(e, level) || (e.onGround && ledgeAhead(e, level))) {
        if (e.flipCd <= 0) flipDir(e, 0.45);
      }
      e.vx = e.dir * def.speed * 1.5;
      e.facing = e.dir;
    } else {
      faceWalk(e, level, dt, def.speed);
    }
  } else if (def.ai === "shoot" || def.ai === "mage") {
    faceWalk(e, level, dt, def.speed * 0.7);
    e.shotT -= dt;
    if (e.shotT <= 0 && p && Math.abs(p.x - e.x) < 360) {
      fireShot(e, def.shot || "dark");
      e.shotT = def.ai === "mage" ? 1.4 : 1.8;
    }
    if (p) e.facing = p.x > e.x ? 1 : -1;
  } else {
    faceWalk(e, level, dt, def.speed);
  }

  applyPhysics(e, level, dt, { fly: false, grav: 26 });
  collidePlayers(e);
}

function updateBoss(b, dt) {
  if (b.dead) return;
  const level = Game.level;
  b.t += dt;
  b.flash = Math.max(0, b.flash - dt);
  if (b.intro > 0) {
    b.intro -= dt;
    return;
  }
  b.stateT -= dt;
  const p = nearestPlayer(b.x, b.y);
  const pat = b.def.pattern;
  const enrage = b.hp < b.maxHp * 0.4 ? 1.35 : 1;

  if (b.state === "idle") {
    if (p) {
      b.facing = p.x > b.x ? 1 : -1;
      b.vx = b.facing * b.def.speed * 0.6 * enrage;
    }
    if (b.stateT <= 0) {
      const opts = ["slam", "shot", "dash"];
      if (pat === "hex" || pat === "final") opts.push("cast");
      if (pat === "summon" || pat === "final" || pat === "multi") opts.push("summon");
      if (pat === "flyfire") opts.push("air");
      if (pat === "wave" || pat === "lava" || pat === "ice") opts.push("wave");
      b.state = opts[irand(opts.length)];
      b.stateT = 0.9;
    }
  } else if (b.state === "slam") {
    if (b.stateT > 0.55) b.vy = -10;
    if (b.stateT < 0.25 && b.onGround) {
      shake(14);
      AudioSys.sfx("boss");
      for (const pl of livingPlayers()) {
        if (Math.abs(pl.x - (b.x + b.w / 2)) < 90) hurtPlayer(pl, 1, b.x);
      }
      burst(b.x + b.w / 2, b.y + b.h, "#d0a070", 16, 120);
    }
    if (b.stateT <= 0) { b.state = "idle"; b.stateT = 0.8; }
  } else if (b.state === "shot" || b.state === "cast") {
    b.vx = 0;
    if (b.stateT < 0.7 && Math.floor(b.stateT * 8) !== Math.floor((b.stateT + dt) * 8)) {
      fireShot(b, pat === "ice" ? "ice" : pat === "lava" || pat === "flyfire" ? "hell" : "hex");
    }
    if (b.stateT <= 0) { b.state = "idle"; b.stateT = 0.7; }
  } else if (b.state === "dash") {
    b.vx = b.facing * b.def.speed * 3.4 * enrage;
    if (b.stateT <= 0) { b.state = "idle"; b.stateT = 1.0; b.vx = 0; }
  } else if (b.state === "summon") {
    b.vx = 0;
    if (b.stateT < 0.85 && Game.enemies.filter((e) => !e.dead).length < 6) {
      const eid = Game.level.world.enemies[irand(2)];
      Game.enemies.push(spawnEnemy({ id: eid, x: b.x + irand(80) - 20, y: b.y }));
      b.state = "idle";
      b.stateT = 1.4;
    }
    if (b.stateT <= 0) { b.state = "idle"; b.stateT = 1; }
  } else if (b.state === "air") {
    b.vy = -4;
    b.y -= 20 * dt;
    if (Math.floor(b.t * 4) !== Math.floor((b.t - dt) * 4)) fireShot(b, "hell");
    if (b.stateT <= 0) { b.state = "idle"; b.stateT = 0.8; }
  } else if (b.state === "wave") {
    b.vx = 0;
    if (Math.floor(b.stateT * 3) !== Math.floor((b.stateT + dt) * 3)) {
      const dir = Math.random() > 0.5 ? 1 : -1;
      Game.projectiles.push({
        kind: pat === "lava" ? "hell" : pat === "ice" ? "ice" : "dark",
        x: b.x + b.w / 2, y: b.y + b.h - 18, w: 20, h: 16,
        vx: dir * 5.5, vy: 0, life: 2.2, dmg: 1, friendly: false
      });
    }
    if (b.stateT <= 0) { b.state = "idle"; b.stateT = 0.9; }
  }

  applyPhysics(b, level, dt, { grav: 22 });
  if (b.x < level.arenaX0 + 10) { b.x = level.arenaX0 + 10; b.facing = 1; }
  if (b.x + b.w > level.arenaX1 - 10) { b.x = level.arenaX1 - 10 - b.w; b.facing = -1; }

  for (const pl of livingPlayers()) {
    if (aabb(b, pl)) {
      const stomp = pl.vy > 1.4 && pl.y + pl.h < b.y + 28;
      if (stomp) {
        damageEnemy(b, 1);
        pl.vy = -9;
      } else hurtPlayer(pl, 1, b.x + b.w / 2);
    }
  }
}

function updateProjectiles(dt) {
  const level = Game.level;
  for (const pr of Game.projectiles) {
    pr.life -= dt;
    pr.x += pr.vx * 110 * dt;
    pr.y += pr.vy * 110 * dt;
    const t = tileAt(level, pr.x + pr.w / 2, pr.y + pr.h / 2);
    if (t === T.SOLID || t === T.ICE || t === T.BREAK) pr.life = 0;
    if (pr.friendly) {
      for (const e of Game.enemies) if (!e.dead && aabb(pr, e)) { damageEnemy(e, pr.dmg); pr.life = 0; }
      for (const b of Game.bosses) if (!b.dead && aabb(pr, b)) { damageEnemy(b, pr.dmg); pr.life = 0; }
    } else {
      for (const p of livingPlayers()) if (aabb(pr, p)) { hurtPlayer(p, pr.dmg, pr.x); pr.life = 0; }
    }
  }
  Game.projectiles = Game.projectiles.filter((p) => p.life > 0);
}

function updateItems(dt) {
  for (const it of Game.items) {
    it.bob = (it.bob || 0) + dt;
    const box = { x: it.x, y: it.y + Math.sin(it.bob * 3) * 3, w: 20, h: 20 };
    for (const p of livingPlayers()) {
      if (aabb(box, p)) {
        it._got = true;
        if (it.type === "soul") { Game.souls++; Game.score += 25; AudioSys.sfx("coin"); floater(it.x, it.y, "+alma", "#d0a0ff"); }
        if (it.type === "heart") { p.hp = Math.min(p.maxHp, p.hp + 2); AudioSys.sfx("heart"); floater(it.x, it.y, "+vida", "#ff8090"); }
      }
    }
  }
  Game.items = Game.items.filter((i) => !i._got);
}

function updateParticles(dt) {
  for (const q of Game.particles) {
    q.life -= dt;
    q.x += q.vx * dt;
    q.y += q.vy * dt;
    q.vy += 180 * dt;
  }
  Game.particles = Game.particles.filter((q) => q.life > 0);
  for (const f of Game.floaters) { f.t -= dt; f.y -= 22 * dt; }
  Game.floaters = Game.floaters.filter((f) => f.t > 0);
}

function maybeLockArena() {
  if (Game.lockedArena) return;
  const any = livingPlayers().some((p) => p.x > Game.level.arenaX0 + 40);
  if (!any) return;
  Game.lockedArena = true;
  Game.bossIntro = 1.8;
  AudioSys.sfx("boss");
  AudioSys.setTheme(Game.level.isFinal || Game.worldIndex === 15 ? "final" : "boss");
  const def = bossById(Game.level.world.boss);
  Game.bosses.push(spawnBoss(def, Game.level.bossAt.x, Game.level.bossAt.y));
  Game.message = def.name.toUpperCase();
  Game.messageT = 2;
}

function checkClear() {
  if (!Game.lockedArena) return;
  const bossesDead = Game.bosses.length && Game.bosses.every((b) => b.dead);
  if (!bossesDead) return;
  if (Game.worldIndex === 15 && !Game.belialPhase) return;
  if (Game.worldIndex === 15 && Game.belialPhase) return;
  for (const p of livingPlayers()) {
    if (Math.abs(p.x - Game.level.exit.x) < 46 && Math.abs(p.y - Game.level.exit.y) < 60) {
      winLevel();
      return;
    }
  }
}

function winLevel() {
  Game.unlocked = Math.max(Game.unlocked, Game.worldIndex + 2);
  persist();
  AudioSys.sfx("clear");
  AudioSys.setTheme("title");
  Game.state = "clear";
  const w = WORLDS[Game.worldIndex];
  $("clear-title").textContent = w.name + " silenciada";
  $("clear-body").textContent = "Almas " + Game.souls + " · Pontos " + Game.score + " · Próximo: " +
    (Game.worldIndex >= 15 ? "o fim da noite" : WORLDS[Math.min(15, Game.worldIndex + 1)].name);
  showScreen("screen-clear");
}

function persist() {
  try {
    localStorage.setItem("nightfall-save", JSON.stringify({
      unlocked: Game.unlocked, souls: Game.souls, score: Game.score
    }));
  } catch (e) { /* ignore */ }
}

function restore() {
  try {
    const s = JSON.parse(localStorage.getItem("nightfall-save") || "null");
    if (s) {
      Game.unlocked = Math.max(16, s.unlocked || 16);
      Game.souls = s.souls || 0;
      Game.score = s.score || 0;
    }
  } catch (e) { /* ignore */ }
}

function startLevel(index) {
  Game.worldIndex = index;
  Game.level = compileLevel(index, false);
  Game.atlases[WORLDS[index].id] = Game.atlases[WORLDS[index].id] || buildAtlas(WORLDS[index]);
  BACKDROPS[WORLDS[index].id] = buildBackdrop(WORLDS[index]);
  Game.enemies = Game.level.ents.map(spawnEnemy);
  Game.enemies.forEach((e) => {
    liftOutOfGround(e, Game.level);
    e.homeX = e.x;
    e.homeY = e.y;
  });
  Game.bosses = [];
  Game.projectiles = [];
  Game.items = Game.level.items.map((i) => ({ ...i, bob: Math.random() * 4 }));
  Game.particles = [];
  Game.floaters = [];
  Game.lockedArena = false;
  Game.belialPhase = false;
  Game.bossIntro = 0;
  Game.fade = 1;
  Game.fadeDir = -1;
  const s = Game.level.spawn;
  Game.players = [spawnPlayer(0, Game.p1Hero, s.x, s.y)];
  if (Input.p2Joined || (typeof Net !== "undefined" && Net.mode !== "local")) {
    Game.players.push(spawnPlayer(1, Game.p2Hero, s.x + 36, s.y));
    Input.p2Joined = true;
  }
  Game.camera.x = s.x - VIEW_W / 3;
  Game.camera.y = s.y - VIEW_H / 2;
  Game.state = "play";
  hideOverlay();
  AudioSys.setTheme("play");
  Game.message = WORLDS[index].name;
  Game.messageT = 2.2;
}

function joinP2() {
  if (Game.players.length > 1) return;
  Input.p2Joined = true;
  const ref = Game.players[0];
  Game.players.push(spawnPlayer(1, Game.p2Hero, ref.x + 28, ref.y));
  Game.message = "Denyse e Mike caçam juntos";
  if (Game.p2Hero === Game.p1Hero) Game.message = HEROES[Game.p2Hero].name + " entra na caçada";
  Game.messageT = 1.6;
}

function updateCamera(dt) {
  const alive = livingPlayers();
  if (!alive.length) return;
  let cx = 0, cy = 0;
  for (const p of alive) { cx += p.x + p.w / 2; cy += p.y + p.h / 2; }
  cx /= alive.length;
  cy /= alive.length;
  const tx = cx - VIEW_W * 0.4;
  const ty = cy - VIEW_H * 0.62;
  Game.camera.x += (tx - Game.camera.x) * Math.min(1, 4 * dt);
  Game.camera.y += (ty - Game.camera.y) * Math.min(1, 3.2 * dt);
  const maxX = Game.level.cols * TILE - VIEW_W;
  const maxY = Game.level.rows * TILE - VIEW_H;
  Game.camera.x = clamp(Game.camera.x, 0, Math.max(0, maxX));
  Game.camera.y = clamp(Game.camera.y, 0, Math.max(0, maxY));
  if (alive.length === 2) {
    const dx = alive[1].x - alive[0].x;
    if (Math.abs(dx) > 430) {
      const trail = dx > 0 ? alive[0] : alive[1];
      trail.x += Math.sign(dx) * 2.4;
    }
  }
  Game.camera.shake *= 0.88;
}

function update(dt) {
  AudioSys.tick(dt);
  Game.time += dt;
  Game.fade = clamp(Game.fade + Game.fadeDir * dt * 1.6, 0, 1);
  Game.messageT = Math.max(0, Game.messageT - dt);

  if (Input.pressed.mute) AudioSys.toggleMute();
  const p2el = $("p2-status");
  if (p2el) p2el.textContent = Input.p2Joined ? "Jogador 2 entrou — setas + ponto e barra" : "Jogador 2: Enter para entrar";
  if (typeof Net !== "undefined") Net.tick(dt);
  if (Game.state !== "play") return;
  if (typeof Net !== "undefined" && Net.mode === "client") {
    for (const p of Game.players) {
      if (p.slot === 1) updatePlayer(p, dt);
    }
    updateCamera(dt);
    if (Input.pressed.pause) { Game.state = "pause"; showScreen("screen-pause"); }
    return;
  }

  if (Input.pressed.pause) {
    Game.state = "pause";
    showScreen("screen-pause");
    return;
  }
  if (Input.p2Joined && Game.players.length === 1) joinP2();

  if (Game.bossIntro > 0) Game.bossIntro -= dt;

  for (const p of Game.players) updatePlayer(p, dt);
  for (const e of Game.enemies) updateEnemy(e, dt);
  for (const b of Game.bosses) updateBoss(b, dt);
  updateProjectiles(dt);
  updateItems(dt);
  updateParticles(dt);
  maybeLockArena();
  checkClear();
  updateCamera(dt);

  if (!livingPlayers().length) {
    Game.state = "dead";
    showScreen("screen-dead");
    AudioSys.setTheme("title");
  }
}

function drawImageFit(ctx, img, x, y, w, h, flip, flash) {
  if (!img) return false;
  ctx.save();
  ctx.translate(x + (flip ? w : 0), y);
  if (flip) ctx.scale(-1, 1);
  if (flash) ctx.filter = "brightness(2.4) saturate(0.4)";
  ctx.drawImage(img, 0, 0, w, h);
  ctx.restore();
  return true;
}

function heroSprite(p) {
  const id = p.hero.id;
  if (p.anim === "attack") return Game.sprites[id + "_attack"] || Game.sprites[id + "_idle"];
  if (p.anim === "jump" || p.anim === "fall") return Game.sprites[id + "_jump"] || Game.sprites[id + "_idle"];
  if (p.anim === "walk") {
    const f = Math.floor(p.animT * 7) % 2;
    return Game.sprites[id + "_walk" + (f + 1)] || Game.sprites[id + "_idle"];
  }
  return Game.sprites[id + "_idle"];
}

const BACKDROPS = {};

function hexRgb(h) {
  const s = (h || "#000").replace("#", "");
  return [parseInt(s.slice(0, 2), 16) || 0, parseInt(s.slice(2, 4), 16) || 0, parseInt(s.slice(4, 6), 16) || 0];
}

function buildBackdrop(world) {
  const img = Game.sprites["bg_" + world.id];
  const W = 2560;
  const H = VIEW_H;
  const c = document.createElement("canvas");
  c.width = W;
  c.height = H;
  const g = c.getContext("2d");
  const tone = hexRgb(world.tone);
  const fog = hexRgb(world.fog);
  const sky = g.createLinearGradient(0, 0, 0, H);
  sky.addColorStop(0, world.tone);
  sky.addColorStop(0.5, world.fog);
  sky.addColorStop(1, "#050308");
  g.fillStyle = sky;
  g.fillRect(0, 0, W, H);

  if (img && img.width) {
    const scale = H / img.height;
    const iw = img.width * scale;
    const cx = (W - iw) / 2;
    if (cx > 0) {
      g.drawImage(img, 0, 0, 6, img.height, 0, 0, cx + 3, H);
      g.drawImage(img, img.width - 6, 0, 6, img.height, cx + iw - 3, 0, W - (cx + iw) + 3, H);
    }
    g.drawImage(img, cx, 0, iw, H);
    const fadeL = g.createLinearGradient(0, 0, Math.max(80, cx + 50), 0);
    fadeL.addColorStop(0, "rgba(" + tone[0] + "," + tone[1] + "," + tone[2] + ",0.72)");
    fadeL.addColorStop(1, "rgba(0,0,0,0)");
    g.fillStyle = fadeL;
    g.fillRect(0, 0, Math.max(80, cx + 50), H);
    const fadeR = g.createLinearGradient(Math.min(W - 80, cx + iw - 50), 0, W, 0);
    fadeR.addColorStop(0, "rgba(0,0,0,0)");
    fadeR.addColorStop(1, "rgba(" + fog[0] + "," + fog[1] + "," + fog[2] + ",0.72)");
    g.fillStyle = fadeR;
    g.fillRect(Math.min(W - 80, cx + iw - 50), 0, 90 + Math.max(0, W - (cx + iw)), H);
  }

  const vg = g.createRadialGradient(W / 2, H * 0.42, H * 0.18, W / 2, H * 0.5, W * 0.52);
  vg.addColorStop(0, "rgba(0,0,0,0)");
  vg.addColorStop(1, "rgba(0,0,0,0.42)");
  g.fillStyle = vg;
  g.fillRect(0, 0, W, H);
  return c;
}

function drawBackground(ctx, world) {
  let bd = BACKDROPS[world.id];
  if (!bd) {
    bd = BACKDROPS[world.id] = buildBackdrop(world);
  }
  const maxScroll = Math.max(0, bd.width - VIEW_W);
  const span = Game.level ? Math.max(1, Game.level.cols * TILE - VIEW_W) : 1;
  const t = Game.level ? clamp(Game.camera.x / span, 0, 1) : 0;
  const x = -t * maxScroll;
  ctx.drawImage(bd, x, 0);
}

function drawTiles(ctx, level) {
  const atlas = Game.atlases[level.world.id];
  const x0 = Math.max(0, Math.floor(Game.camera.x / TILE) - 1);
  const x1 = Math.min(level.cols - 1, Math.ceil((Game.camera.x + VIEW_W) / TILE) + 1);
  const y0 = Math.max(0, Math.floor(Game.camera.y / TILE) - 1);
  const y1 = Math.min(level.rows - 1, Math.ceil((Game.camera.y + VIEW_H) / TILE) + 1);
  const pulse = 0.5 + Math.sin(Game.time * 4) * 0.5;
  for (let y = y0; y <= y1; y++) {
    for (let x = x0; x <= x1; x++) {
      const t = level.tiles[y][x];
      if (!t) continue;
      const sx = x * TILE - Game.camera.x;
      const sy = y * TILE - Game.camera.y;
      if (atlas && t > 0 && t < 11) {
        ctx.drawImage(atlas, t * TILE, 0, TILE, TILE, sx, sy, TILE, TILE);
      }
      if (t === T.LAVA) {
        ctx.fillStyle = `rgba(255,${80 + pulse * 80 | 0},20,0.35)`;
        ctx.fillRect(sx, sy, TILE, TILE);
      }
      if (t === T.WATER) {
        ctx.fillStyle = `rgba(40,120,160,${0.22 + pulse * 0.1})`;
        ctx.fillRect(sx, sy, TILE, TILE);
      }
    }
  }
}

function drawEntitySprite(ctx, img, e, flip, flash) {
  const sx = e.x - Game.camera.x;
  const sy = e.y - Game.camera.y;
  if (img) {
    drawImageFit(ctx, img, sx - 8, sy - 10, e.w + 16, e.h + 12, flip, flash);
    return;
  }
  // richly shaded fallback silhouette — never a flat rectangle
  ctx.save();
  ctx.translate(sx + e.w / 2, sy + e.h);
  ctx.scale(flip ? -1 : 1, 1);
  const grd = ctx.createLinearGradient(0, -e.h, 0, 0);
  grd.addColorStop(0, flash ? "#fff" : "#2a1020");
  grd.addColorStop(0.5, "#5a2038");
  grd.addColorStop(1, "#12080c");
  ctx.fillStyle = grd;
  ctx.beginPath();
  ctx.moveTo(-e.w * 0.35, 0);
  ctx.quadraticCurveTo(-e.w * 0.55, -e.h * 0.35, -e.w * 0.25, -e.h * 0.7);
  ctx.quadraticCurveTo(-e.w * 0.2, -e.h * 1.02, 0, -e.h);
  ctx.quadraticCurveTo(e.w * 0.28, -e.h * 0.95, e.w * 0.22, -e.h * 0.62);
  ctx.quadraticCurveTo(e.w * 0.5, -e.h * 0.3, e.w * 0.3, 0);
  ctx.closePath();
  ctx.fill();
  ctx.fillStyle = "rgba(180,40,40,0.8)";
  ctx.beginPath();
  ctx.arc(-e.w * 0.08, -e.h * 0.78, 3, 0, 7);
  ctx.arc(e.w * 0.1, -e.h * 0.78, 3, 0, 7);
  ctx.fill();
  ctx.restore();
}

function drawGame() {
  const ctx = Game.ctx;
  const level = Game.level;
  const shx = (Math.random() - 0.5) * Game.camera.shake;
  const shy = (Math.random() - 0.5) * Game.camera.shake;
  ctx.save();
  ctx.translate(shx, shy);
  drawBackground(ctx, level.world);
  drawTiles(ctx, level);

  if (level.check) {
    const x = level.check.x - Game.camera.x;
    const y = level.check.y - Game.camera.y;
    ctx.fillStyle = "#d4b46a";
    ctx.fillRect(x + 8, y - 20, 4, 40);
    ctx.fillStyle = "#8b1e2d";
    ctx.beginPath();
    ctx.moveTo(x + 12, y - 20);
    ctx.lineTo(x + 28, y - 10);
    ctx.lineTo(x + 12, y - 4);
    ctx.fill();
  }

  for (const it of Game.items) {
    const img = Game.sprites["it_" + it.type];
    const x = it.x - Game.camera.x;
    const y = it.y + Math.sin((it.bob || 0) * 3) * 3 - Game.camera.y;
    if (img) ctx.drawImage(img, x - 4, y - 4, 24, 24);
    else {
      ctx.fillStyle = it.type === "heart" ? "#ff4d66" : "#b060ff";
      ctx.beginPath();
      ctx.arc(x + 10, y + 10, 7, 0, 7);
      ctx.fill();
      ctx.fillStyle = "rgba(255,255,255,0.5)";
      ctx.beginPath();
      ctx.arc(x + 7, y + 7, 2.2, 0, 7);
      ctx.fill();
    }
  }

  for (const e of Game.enemies) {
    if (e.dead) continue;
    drawEntitySprite(ctx, Game.sprites["en_" + e.id], e, e.facing < 0, e.flash > 0);
  }
  for (const b of Game.bosses) {
    if (b.dead) continue;
    drawEntitySprite(ctx, Game.sprites["boss_" + b.id], b, b.facing < 0, b.flash > 0);
    const bx = b.x - Game.camera.x;
    const by = b.y - Game.camera.y - 14;
    ctx.fillStyle = "#20080c";
    ctx.fillRect(bx, by, b.w, 6);
    ctx.fillStyle = "#d43040";
    ctx.fillRect(bx, by, b.w * (b.hp / b.maxHp), 6);
    ctx.strokeStyle = "#f0d090";
    ctx.strokeRect(bx, by, b.w, 6);
  }

  for (const p of Game.players) {
    if (p.dead) continue;
    if (p.inv > 0 && Math.floor(Game.time * 18) % 2 === 0) continue;
    const img = heroSprite(p);
    const bob = p.anim === "idle" ? Math.sin(Game.time * 3) * 1.2 : 0;
    drawImageFit(ctx, img, p.x - Game.camera.x - 14, p.y - Game.camera.y - 16 + bob, p.w + 28, p.h + 18, p.facing < 0, p.hurtT > 0);
  }

  for (const pr of Game.projectiles) {
    const x = pr.x - Game.camera.x, y = pr.y - Game.camera.y;
    const img = pr.kind === "magic" ? Game.sprites.fx_bolt : null;
    if (img) ctx.drawImage(img, x, y, 20, 14);
    else {
      ctx.fillStyle = pr.kind === "magic" ? "#c080ff" : pr.kind === "hell" ? "#ff6020" : pr.kind === "ice" ? "#a0e0ff" : "#8020c0";
      ctx.beginPath();
      ctx.ellipse(x + 8, y + 6, 9, 5, 0, 0, 7);
      ctx.fill();
      ctx.fillStyle = "rgba(255,255,255,0.7)";
      ctx.beginPath();
      ctx.ellipse(x + 6, y + 5, 3, 2, 0, 0, 7);
      ctx.fill();
    }
  }

  for (const q of Game.particles) {
    ctx.globalAlpha = q.life / q.max;
    ctx.fillStyle = q.c;
    ctx.fillRect(q.x - Game.camera.x, q.y - Game.camera.y, q.s, q.s);
  }
  ctx.globalAlpha = 1;

  for (const f of Game.floaters) {
    ctx.globalAlpha = Math.min(1, f.t * 2);
    ctx.fillStyle = f.c;
    ctx.font = "12px Cinzel, serif";
    ctx.fillText(f.text, f.x - Game.camera.x, f.y - Game.camera.y);
  }
  ctx.globalAlpha = 1;

  if (Game.lockedArena && Game.bosses.some((b) => !b.dead) === false && Game.worldIndex !== 15) {
    const x = Game.level.exit.x - Game.camera.x;
    const y = Game.level.exit.y - Game.camera.y;
    ctx.fillStyle = "#d4b46a";
    ctx.beginPath();
    ctx.arc(x + 10, y, 16, 0, 7);
    ctx.fill();
    ctx.fillStyle = "#2a1810";
    ctx.fillRect(x + 8, y + 4, 4, 22);
  }

  ctx.restore();

  // vignette
  const vg = ctx.createRadialGradient(VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.25, VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.78);
  vg.addColorStop(0, "rgba(0,0,0,0)");
  vg.addColorStop(1, "rgba(0,0,0,0.55)");
  ctx.fillStyle = vg;
  ctx.fillRect(0, 0, VIEW_W, VIEW_H);

  drawHUD(ctx);
  if (Game.fade > 0) {
    ctx.fillStyle = `rgba(0,0,0,${Game.fade})`;
    ctx.fillRect(0, 0, VIEW_W, VIEW_H);
  }
}

function drawHUD(ctx) {
  ctx.save();
  ctx.font = "13px Cinzel, serif";
  Game.players.forEach((p, i) => {
    const x = 16 + i * 230;
    const y = 16;
    ctx.fillStyle = "rgba(10,4,8,0.55)";
    ctx.fillRect(x, y, 214, 46);
    ctx.strokeStyle = "rgba(212,180,106,0.45)";
    ctx.strokeRect(x, y, 214, 46);
    ctx.fillStyle = "#d4b46a";
    ctx.fillText((i === 0 ? "P1 " : "P2 ") + p.hero.name, x + 8, y + 16);
    for (let h = 0; h < p.maxHp; h++) {
      ctx.fillStyle = h < p.hp ? "#e04050" : "#3a1820";
      ctx.beginPath();
      const hx = x + 10 + h * 16;
      const hy = y + 30;
      ctx.moveTo(hx, hy);
      ctx.bezierCurveTo(hx, hy - 8, hx + 12, hy - 8, hx + 12, hy);
      ctx.bezierCurveTo(hx + 12, hy + 8, hx, hy + 10, hx, hy);
      ctx.fill();
    }
    ctx.fillStyle = "#c9b48a";
    ctx.fillText("x" + Math.max(0, p.lives), x + 180, y + 34);
  });
  ctx.fillStyle = "#d4b46a";
  ctx.textAlign = "right";
  ctx.fillText("Almas " + Game.souls, VIEW_W - 18, 24);
  ctx.fillText("Pts " + Game.score, VIEW_W - 18, 42);
  ctx.fillText((Game.worldIndex + 1) + " / 16  " + WORLDS[Game.worldIndex].name, VIEW_W - 18, 60);
  ctx.textAlign = "left";

  if (Game.messageT > 0) {
    ctx.textAlign = "center";
    ctx.globalAlpha = Math.min(1, Game.messageT);
    ctx.fillStyle = "#f0e2c8";
    ctx.font = "22px Cinzel, serif";
    ctx.fillText(Game.message, VIEW_W / 2, 92);
    ctx.globalAlpha = 1;
    ctx.textAlign = "left";
  }
  if (Game.bossIntro > 0 && Game.bosses[0]) {
    ctx.textAlign = "center";
    ctx.fillStyle = "#ffd0a0";
    ctx.font = "28px Cinzel, serif";
    ctx.fillText(Game.bosses[Game.bosses.length - 1].def.name, VIEW_W / 2, VIEW_H * 0.4);
    ctx.font = "13px Cinzel, serif";
    ctx.fillStyle = "#c9b48a";
    ctx.fillText("chefe da noite", VIEW_W / 2, VIEW_H * 0.4 + 26);
    ctx.textAlign = "left";
  }
  ctx.restore();
}

function buildHeroSelect() {
  const row = $("hero-row");
  row.innerHTML = "";
  for (const id of ["mike", "denyse"]) {
    const h = HEROES[id];
    const card = document.createElement("div");
    card.className = "hero-card" + (Game.p1Hero === id ? " selected" : "");
    card.dataset.id = id;
    const img = Game.sprites[id + "_idle"];
    card.innerHTML = `<h3>${h.name}</h3><p>${h.title}</p>`;
    if (img) {
      const el = document.createElement("img");
      el.src = img.src;
      el.alt = h.name;
      card.insertBefore(el, card.firstChild);
    }
    const p = document.createElement("p");
    p.textContent = h.blurb;
    card.appendChild(p);
    card.addEventListener("click", () => {
      Game.p1Hero = id;
      Game.p2Hero = id === "mike" ? "denyse" : "mike";
      AudioSys.sfx("select");
      buildHeroSelect();
    });
    row.appendChild(card);
  }
}

function buildMap() {
  const grid = $("world-grid");
  grid.innerHTML = "";
  WORLDS.forEach((w, i) => {
    const cell = document.createElement("div");
    const locked = i >= Game.unlocked;
    cell.className = "world-cell" + (locked ? " lock" : "") + (i === Game.worldIndex ? " cur" : "");
    cell.textContent = (i + 1) + ". " + w.name;
    if (!locked) {
      cell.addEventListener("click", () => {
        Game.worldIndex = i;
        AudioSys.sfx("select");
        buildMap();
      });
    }
    grid.appendChild(cell);
  });
}

function loop(t) {
  requestAnimationFrame(loop);
  if (!Game.last) Game.last = t;
  let dt = (t - Game.last) / 1000;
  Game.last = t;
  if (dt > 0.05) dt = 0.05;
  Game.acc += dt;
  const step = 1 / 60;
  while (Game.acc >= step) {
    update(step);
    Game.acc -= step;
    Input.endFrame();
  }
  const ctx = Game.ctx;
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.clearRect(0, 0, Game.canvas.width, Game.canvas.height);
  ctx.save();
  ctx.scale(Game.canvas.width / VIEW_W, Game.canvas.height / VIEW_H);
  if (Game.state === "play" || Game.state === "pause") drawGame();
  else {
    ctx.fillStyle = "#07040c";
    ctx.fillRect(0, 0, VIEW_W, VIEW_H);
  }
  ctx.restore();
}

function fitCanvas() {
  const c = Game.canvas;
  const r = window.devicePixelRatio || 1;
  const w = window.innerWidth;
  const h = window.innerHeight;
  c.style.width = w + "px";
  c.style.height = h + "px";
  c.width = Math.floor(w * r);
  c.height = Math.floor(h * r);
}

function bindUI() {
  $("btn-start").onclick = () => {
    AudioSys.resume(); AudioSys.sfx("select");
    if (typeof Net !== "undefined") { Net.mode = "local"; }
    buildHeroSelect(); showScreen("screen-select");
  };
  $("btn-net").onclick = () => {
    AudioSys.resume(); AudioSys.sfx("select");
    buildHeroSelect(); showScreen("screen-select");
    $("btn-play").dataset.next = "net";
  };
  $("btn-howto").onclick = () => { AudioSys.sfx("select"); showScreen("screen-howto"); };
  $("btn-howto-back").onclick = () => showScreen("screen-title");
  $("btn-play").onclick = () => {
    AudioSys.resume();
    if ($("btn-play").dataset.next === "net") {
      $("btn-play").dataset.next = "";
      if (typeof Net !== "undefined") Net.render();
      showScreen("screen-net");
      return;
    }
    buildMap(); showScreen("screen-map");
  };
  $("btn-net-host").onclick = () => { AudioSys.sfx("select"); Net.host("lan"); };
  $("btn-net-p2p").onclick = () => { AudioSys.sfx("select"); Net.host("p2p"); };
  $("btn-net-scan").onclick = () => { AudioSys.sfx("select"); Net.scan(); };
  $("btn-net-go").onclick = () => {
    AudioSys.sfx("select");
    if (Net.mode === "client" && !Net.connected) { Net.status = "Entre numa sala primeiro."; Net.render(); return; }
    buildMap();
    showScreen("screen-map");
  };
  $("btn-net-back").onclick = () => { Net.stop(); showScreen("screen-title"); };
  $("btn-map-go").onclick = () => {
    if (typeof Net !== "undefined" && Net.mode === "host") { Net.startMatch(); return; }
    if (typeof Net !== "undefined" && Net.mode === "client") {
      Net.status = "Aguardando o host iniciar a fase…";
      Net.render();
      return;
    }
    startLevel(Game.worldIndex);
  };
  $("btn-map-back").onclick = () => showScreen("screen-title");
  $("btn-resume").onclick = () => { Game.state = "play"; hideOverlay(); };
  $("btn-quit").onclick = () => { Game.state = "map"; buildMap(); showScreen("screen-map"); AudioSys.setTheme("title"); };
  $("btn-next").onclick = () => {
    if (Game.worldIndex >= 15) { showScreen("screen-win"); return; }
    Game.worldIndex += 1;
    startLevel(Game.worldIndex);
  };
  $("btn-retry").onclick = () => startLevel(Game.worldIndex);
  $("btn-dead-map").onclick = () => { buildMap(); showScreen("screen-map"); };
  $("btn-win").onclick = () => showScreen("screen-title");
}

async function boot() {
  Game.canvas = $("game");
  Game.ctx = Game.canvas.getContext("2d");
  Game.ctx.imageSmoothingEnabled = true;
  fitCanvas();
  window.addEventListener("resize", fitCanvas);
  Input.bind();
  bindUI();
  restore();
  $("boot-msg").textContent = "Forjando lâminas, cristal e pesadelos…";
  await loadAllSprites();
  $("boot-msg").textContent = "O véu está rasgado.";
  showScreen("screen-title");
  Game.state = "title";
  requestAnimationFrame(loop);
}

window.addEventListener("load", boot);
