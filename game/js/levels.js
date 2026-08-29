function mulberry(seed) {
  let s = seed | 0;
  return () => {
    s = (s + 0x6d2b79f5) | 0;
    let t = Math.imul(s ^ (s >>> 15), 1 | s);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

function makeGrid(rows, cols) {
  const g = new Array(rows);
  for (let y = 0; y < rows; y++) g[y] = new Uint8Array(cols);
  return g;
}

function inb(ctx, x, y) {
  return x >= 0 && y >= 0 && x < ctx.cols && y < ctx.rows;
}

function setT(ctx, x, y, t) {
  if (inb(ctx, x, y)) ctx.grid[y][x] = t;
}

function fillDown(ctx, x, top, t) {
  for (let y = top; y < ctx.rows; y++) setT(ctx, x, y, t);
}

function rect(ctx, x, y, w, h, t) {
  for (let iy = 0; iy < h; iy++) {
    for (let ix = 0; ix < w; ix++) setT(ctx, x + ix, y + iy, t);
  }
}

function platform(ctx, x, y, w, t) {
  t = t == null ? T.PLATFORM : t;
  for (let i = 0; i < w; i++) setT(ctx, x + i, y, t);
}

function solidBlock(ctx, x, y, w, h) {
  rect(ctx, x, y, w, h, T.SOLID);
}

function addEnemy(ctx, id, tx, ty) {
  const def = enemyById(id);
  if (!def) return;
  ctx.ents.push({
    kind: "enemy",
    id,
    x: tx * TILE + (TILE - def.w) / 2,
    y: ty * TILE - def.h
  });
}

function addItem(ctx, type, tx, ty) {
  ctx.items.push({ type, x: tx * TILE + 10, y: ty * TILE + 8 });
}

function nearestFloor(ctx, tx, fromY) {
  for (let y = fromY; y < ctx.rows; y++) {
    const t = ctx.grid[y][tx];
    if (t === T.SOLID || t === T.ICE || t === T.PLATFORM) return y;
  }
  return ctx.rows - 3;
}

function paintGround(ctx, x0, x1, heightFn, tile) {
  tile = tile || T.SOLID;
  for (let x = x0; x < x1 && x < ctx.cols; x++) {
    const top = heightFn(x);
    fillDown(ctx, x, top, tile);
  }
}

function hazardFor(world) {
  if (world.hazard === "lava") return T.LAVA;
  if (world.hazard === "water") return T.WATER;
  if (world.hazard === "ice") return T.ICE;
  if (world.hazard === "thorn") return T.THORN;
  return T.SPIKE;
}

const BEATS = {
  intro(ctx) {
    const x = ctx.cursor;
    paintGround(ctx, x, x + 16, () => ctx.base, T.SOLID);
    if (!ctx.spawn) ctx.spawn = { x: (x + 3) * TILE, y: (ctx.base - 2) * TILE };
    addItem(ctx, "soul", x + 8, ctx.base - 1);
    addEnemy(ctx, ctx.world.enemies[0], x + 12, ctx.base);
    ctx.cursor += 16;
  },

  hills(ctx) {
    const x0 = ctx.cursor;
    const rnd = ctx.rnd;
    paintGround(ctx, x0, x0 + 24, (x) => {
      const w = Math.sin((x - x0) * 0.38) * 2 + Math.sin((x - x0) * 0.11) * 1;
      return Math.round(ctx.base - 1 - w);
    }, T.SOLID);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 8, ctx.base - 2);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 16, ctx.base - 2);
    addItem(ctx, "soul", x0 + 11, ctx.base - 5);
    if (rnd() > 0.4) addItem(ctx, "soul", x0 + 20, ctx.base - 6);
    ctx.cursor += 24;
  },

  pit(ctx) {
    const x0 = ctx.cursor;
    const hz = hazardFor(ctx.world);
    paintGround(ctx, x0, x0 + 4, () => ctx.base, T.SOLID);
    for (let x = x0 + 4; x < x0 + 16; x++) {
      if (hz === T.ICE) fillDown(ctx, x, ctx.base + 1, T.ICE);
      else {
        fillDown(ctx, x, ctx.rows - 2, T.SOLID);
        setT(ctx, x, ctx.rows - 3, hz === T.WATER || hz === T.LAVA ? hz : T.SPIKE);
        if (hz === T.WATER || hz === T.LAVA) {
          for (let y = ctx.base + 1; y < ctx.rows - 3; y++) setT(ctx, x, y, hz);
        }
      }
    }
    platform(ctx, x0 + 5, ctx.base - 1, 3);
    platform(ctx, x0 + 9, ctx.base - 3, 3);
    platform(ctx, x0 + 13, ctx.base - 1, 3);
    if (ctx.world.flyer) addEnemy(ctx, ctx.world.flyer, x0 + 10, ctx.base - 6);
    addItem(ctx, "soul", x0 + 10, ctx.base - 5);
    paintGround(ctx, x0 + 16, x0 + 20, () => ctx.base, T.SOLID);
    ctx.cursor += 20;
  },

  trees(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 22, () => ctx.base, T.SOLID);
    for (const px of [x0 + 4, x0 + 10, x0 + 16]) {
      solidBlock(ctx, px, ctx.base - 5, 1, 5);
      platform(ctx, px - 2, ctx.base - 5, 5);
      addItem(ctx, "soul", px, ctx.base - 6);
    }
    addEnemy(ctx, ctx.world.enemies[0], x0 + 7, ctx.base);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 13, ctx.base - 8);
    ctx.cursor += 22;
  },

  canopy(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 3, () => ctx.base, T.SOLID);
    for (let i = 0; i < 6; i++) {
      platform(ctx, x0 + 3 + i * 3, ctx.base - 2 - (i % 3), 3);
      if (i % 2 === 0) addItem(ctx, "soul", x0 + 4 + i * 3, ctx.base - 3 - (i % 3));
      if (i === 2) addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 4 + i * 3, ctx.base - 6);
    }
    paintGround(ctx, x0 + 21, x0 + 24, () => ctx.base, T.SOLID);
    ctx.cursor += 24;
  },

  rooms(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 26, () => ctx.base, T.SOLID);
    solidBlock(ctx, x0, ctx.base - 8, 1, 8);
    solidBlock(ctx, x0 + 25, ctx.base - 8, 1, 8);
    platform(ctx, x0 + 1, ctx.base - 8, 24);
    platform(ctx, x0 + 4, ctx.base - 4, 6);
    platform(ctx, x0 + 14, ctx.base - 4, 6);
    for (let y = ctx.base - 7; y < ctx.base; y++) setT(ctx, x0 + 12, y, T.LADDER);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 6, ctx.base - 4);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 18, ctx.base);
    addItem(ctx, "heart", x0 + 20, ctx.base - 5);
    ctx.cursor += 26;
  },

  stairs(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 18, () => ctx.base, T.SOLID);
    for (let i = 0; i < 6; i++) {
      solidBlock(ctx, x0 + 3 + i * 2, ctx.base - 1 - i, 2, 1 + i);
    }
    addEnemy(ctx, ctx.world.enemies[0], x0 + 14, ctx.base - 7);
    addItem(ctx, "soul", x0 + 15, ctx.base - 8);
    ctx.cursor += 18;
  },

  attic(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 20, () => ctx.base, T.SOLID);
    platform(ctx, x0 + 2, ctx.base - 5, 16);
    platform(ctx, x0 + 1, ctx.base - 9, 18);
    for (let y = ctx.base - 8; y < ctx.base; y++) setT(ctx, x0 + 3, y, T.LADDER);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 10, ctx.base - 5);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 14, ctx.base);
    addItem(ctx, "soul", x0 + 16, ctx.base - 10);
    ctx.cursor += 20;
  },

  water(ctx) {
    const x0 = ctx.cursor;
    const fluid = ctx.world.hazard === "lava" ? T.LAVA : T.WATER;
    paintGround(ctx, x0, x0 + 4, () => ctx.base, T.SOLID);
    for (let x = x0 + 4; x < x0 + 18; x++) {
      fillDown(ctx, x, ctx.base + 3, T.SOLID);
      for (let y = ctx.base; y < ctx.base + 3; y++) setT(ctx, x, y, fluid);
    }
    platform(ctx, x0 + 6, ctx.base - 1, 3);
    platform(ctx, x0 + 11, ctx.base - 2, 3);
    platform(ctx, x0 + 16, ctx.base - 1, 2);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 10, ctx.base - 5);
    paintGround(ctx, x0 + 18, x0 + 22, () => ctx.base, T.SOLID);
    ctx.cursor += 22;
  },

  isles(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 3, () => ctx.base, T.SOLID);
    const ys = [ctx.base - 1, ctx.base - 3, ctx.base - 2, ctx.base - 5, ctx.base - 3];
    for (let i = 0; i < 5; i++) {
      platform(ctx, x0 + 4 + i * 4, ys[i], 3);
      if (i % 2) addItem(ctx, "soul", x0 + 5 + i * 4, ys[i] - 1);
      if (i === 3) addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 5 + i * 4, ys[i]);
    }
    paintGround(ctx, x0 + 24, x0 + 28, () => ctx.base, T.SOLID);
    ctx.cursor += 28;
  },

  dunes(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 28, (x) => {
      const h = Math.round(Math.sin((x - x0) * 0.22) * 2.4);
      return ctx.base - h;
    }, T.SOLID);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 8, ctx.base);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 16, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 22, ctx.base);
    addItem(ctx, "soul", x0 + 14, ctx.base - 5);
    ctx.cursor += 28;
  },

  ruins(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 22, () => ctx.base, T.SOLID);
    solidBlock(ctx, x0 + 4, ctx.base - 3, 2, 3);
    solidBlock(ctx, x0 + 10, ctx.base - 5, 2, 5);
    solidBlock(ctx, x0 + 16, ctx.base - 2, 3, 2);
    platform(ctx, x0 + 6, ctx.base - 6, 5);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 12, ctx.base);
    addItem(ctx, "soul", x0 + 8, ctx.base - 7);
    ctx.cursor += 22;
  },

  tombs(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 24, () => ctx.base, T.SOLID);
    for (let i = 0; i < 5; i++) {
      solidBlock(ctx, x0 + 3 + i * 4, ctx.base - 2, 1, 2);
      platform(ctx, x0 + 2 + i * 4, ctx.base - 3, 3);
    }
    addEnemy(ctx, ctx.world.enemies[0], x0 + 8, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 16, ctx.base);
    addItem(ctx, "heart", x0 + 11, ctx.base - 4);
    ctx.cursor += 24;
  },

  crypt(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 6, () => ctx.base, T.SOLID);
    const dip = ctx.base + 3;
    for (let x = x0 + 6; x < x0 + 20; x++) fillDown(ctx, x, dip, T.SOLID);
    platform(ctx, x0 + 5, ctx.base, 2);
    platform(ctx, x0 + 19, ctx.base, 2);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 10, dip);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 15, dip);
    addItem(ctx, "soul", x0 + 13, dip - 1);
    paintGround(ctx, x0 + 20, x0 + 24, () => ctx.base, T.SOLID);
    ctx.cursor += 24;
  },

  hall(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 28, () => ctx.base, T.SOLID);
    for (let i = 0; i < 4; i++) {
      solidBlock(ctx, x0 + 5 + i * 6, ctx.base - 6, 1, 6);
      platform(ctx, x0 + 4 + i * 6, ctx.base - 6, 3);
    }
    addEnemy(ctx, ctx.world.enemies[0], x0 + 8, ctx.base);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 16, ctx.base - 9);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 22, ctx.base);
    ctx.cursor += 28;
  },

  towers(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 20, () => ctx.base, T.SOLID);
    solidBlock(ctx, x0 + 4, ctx.base - 8, 3, 8);
    solidBlock(ctx, x0 + 13, ctx.base - 10, 3, 10);
    platform(ctx, x0 + 7, ctx.base - 4, 6);
    platform(ctx, x0 + 7, ctx.base - 8, 6);
    for (let y = ctx.base - 9; y < ctx.base; y++) setT(ctx, x0 + 8, y, T.LADDER);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 16, ctx.base);
    addItem(ctx, "soul", x0 + 14, ctx.base - 11);
    ctx.cursor += 20;
  },

  icefields(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 26, (x) => ctx.base - Math.round(Math.sin((x - x0) * 0.3)), T.ICE);
    platform(ctx, x0 + 8, ctx.base - 4, 3, T.PLATFORM);
    platform(ctx, x0 + 14, ctx.base - 6, 3, T.BOUNCE);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 10, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 18, ctx.base);
    ctx.cursor += 26;
  },

  lava(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 3, () => ctx.base, T.SOLID);
    for (let x = x0 + 3; x < x0 + 20; x++) {
      fillDown(ctx, x, ctx.rows - 2, T.SOLID);
      for (let y = ctx.base + 1; y < ctx.rows - 2; y++) setT(ctx, x, y, T.LAVA);
    }
    platform(ctx, x0 + 4, ctx.base - 1, 3);
    platform(ctx, x0 + 9, ctx.base - 3, 3);
    platform(ctx, x0 + 14, ctx.base - 2, 4);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[0], x0 + 11, ctx.base - 6);
    paintGround(ctx, x0 + 20, x0 + 24, () => ctx.base, T.SOLID);
    ctx.cursor += 24;
  },

  fade(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 3, () => ctx.base, T.SOLID);
    for (let i = 0; i < 5; i++) {
      platform(ctx, x0 + 4 + i * 4, ctx.base - 1 - (i % 2), 3);
      ctx.specials.push({ type: "fade", x: x0 + 4 + i * 4, y: ctx.base - 1 - (i % 2), w: 3 });
    }
    paintGround(ctx, x0 + 24, x0 + 28, () => ctx.base, T.SOLID);
    addItem(ctx, "soul", x0 + 12, ctx.base - 4);
    ctx.cursor += 28;
  },

  cliffs(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 4, () => ctx.base, T.SOLID);
    paintGround(ctx, x0 + 4, x0 + 8, () => ctx.base - 3, T.SOLID);
    paintGround(ctx, x0 + 8, x0 + 11, () => ctx.base - 6, T.SOLID);
    for (let x = x0 + 11; x < x0 + 16; x++) fillDown(ctx, x, ctx.rows - 2, T.SOLID);
    platform(ctx, x0 + 12, ctx.base - 5, 3);
    paintGround(ctx, x0 + 16, x0 + 22, () => ctx.base - 2, T.SOLID);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 18, ctx.base - 2);
    ctx.zones.push({ type: "wind", x0: (x0 + 11) * TILE, x1: (x0 + 16) * TILE, vx: 1.1 });
    ctx.cursor += 22;
  },

  wind(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 6, () => ctx.base, T.SOLID);
    for (let i = 0; i < 4; i++) platform(ctx, x0 + 7 + i * 4, ctx.base - 2 - (i % 3), 3);
    ctx.zones.push({ type: "wind", x0: (x0 + 6) * TILE, x1: (x0 + 22) * TILE, vx: 1.4 });
    paintGround(ctx, x0 + 22, x0 + 26, () => ctx.base, T.SOLID);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[0], x0 + 14, ctx.base - 7);
    ctx.cursor += 26;
  },

  cave(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 26, () => ctx.base, T.SOLID);
    for (let x = x0; x < x0 + 26; x++) {
      const ceil = 2 + Math.round(Math.sin((x - x0) * 0.4) + 1);
      for (let y = 0; y < ceil; y++) setT(ctx, x, y, T.SOLID);
    }
    platform(ctx, x0 + 6, ctx.base - 3, 4);
    platform(ctx, x0 + 14, ctx.base - 4, 4);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 8, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 18, ctx.base);
    ctx.cursor += 26;
  },

  roofs(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 26, () => ctx.base, T.SOLID);
    for (let i = 0; i < 4; i++) {
      const bx = x0 + 2 + i * 6;
      solidBlock(ctx, bx, ctx.base - 4, 4, 4);
      platform(ctx, bx - 1, ctx.base - 5, 6);
    }
    addEnemy(ctx, ctx.world.enemies[0], x0 + 10, ctx.base - 5);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 20, ctx.base);
    addItem(ctx, "soul", x0 + 15, ctx.base - 6);
    ctx.cursor += 26;
  },

  street(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 24, () => ctx.base, T.SOLID);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 6, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 12, ctx.base);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 18, ctx.base);
    platform(ctx, x0 + 8, ctx.base - 3, 4);
    addItem(ctx, "soul", x0 + 10, ctx.base - 4);
    ctx.cursor += 24;
  },

  nave(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 24, () => ctx.base, T.SOLID);
    for (let i = 0; i < 3; i++) {
      solidBlock(ctx, x0 + 4 + i * 7, 2, 1, ctx.base - 2);
      platform(ctx, x0 + 3 + i * 7, 6, 3);
    }
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 8, 8);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 16, ctx.base);
    ctx.cursor += 24;
  },

  gauntlet(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 26, () => ctx.base, T.SOLID);
    platform(ctx, x0 + 6, ctx.base - 3, 4);
    platform(ctx, x0 + 14, ctx.base - 4, 4);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 5, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 9, ctx.base - 3);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 13, ctx.base);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 17, ctx.base - 6);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 21, ctx.base);
    addItem(ctx, "heart", x0 + 16, ctx.base - 5);
    ctx.cursor += 26;
  },

  check(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 12, () => ctx.base, T.SOLID);
    ctx.check = { x: (x0 + 6) * TILE, y: (ctx.base - 2) * TILE };
    addItem(ctx, "heart", x0 + 8, ctx.base - 1);
    addItem(ctx, "soul", x0 + 4, ctx.base - 1);
    ctx.cursor += 12;
  },

  finalgate(ctx) {
    const x0 = ctx.cursor;
    paintGround(ctx, x0, x0 + 20, () => ctx.base, T.SOLID);
    addEnemy(ctx, ctx.world.enemies[0], x0 + 6, ctx.base);
    addEnemy(ctx, ctx.world.enemies[1], x0 + 10, ctx.base);
    addEnemy(ctx, ctx.world.flyer || ctx.world.enemies[1], x0 + 14, ctx.base - 5);
    addItem(ctx, "heart", x0 + 16, ctx.base - 1);
    ctx.cursor += 20;
  }
};

function stampArena(ctx, final) {
  const x0 = ctx.cursor;
  const width = final ? 36 : 30;
  paintGround(ctx, x0, x0 + width, () => ctx.base, T.SOLID);
  solidBlock(ctx, x0, ctx.base - 7, 1, 7);
  solidBlock(ctx, x0 + width - 1, ctx.base - 7, 1, 7);
  platform(ctx, x0 + 4, ctx.base - 4, 4);
  platform(ctx, x0 + width - 8, ctx.base - 4, 4);
  ctx.bossAt = { x: (x0 + width - 10) * TILE, y: (ctx.base) * TILE, final: !!final };
  ctx.exit = { x: (x0 + width - 4) * TILE, y: (ctx.base - 2) * TILE };
  ctx.arenaX0 = x0 * TILE;
  ctx.arenaX1 = (x0 + width) * TILE;
  ctx.cursor += width;
}

function compileLevel(worldIndex, wantFinal) {
  const world = WORLDS[worldIndex];
  const rows = 20;
  const cols = world.cols + 40;
  const ctx = {
    world,
    worldIndex,
    cols,
    rows,
    grid: makeGrid(rows, cols),
    cursor: 2,
    base: 15,
    ents: [],
    items: [],
    specials: [],
    zones: [],
    spawn: null,
    check: null,
    exit: null,
    bossAt: null,
    rnd: mulberry(0x51f2 + worldIndex * 97)
  };

  paintGround(ctx, 0, 2, () => ctx.base, T.SOLID);
  for (const beat of world.beats) {
    const fn = BEATS[beat];
    if (fn) fn(ctx);
    else BEATS.hills(ctx);
  }
  stampArena(ctx, !!wantFinal);

  // safety floor rim so camera has bounds
  for (let x = 0; x < ctx.cols; x++) {
    if (ctx.grid[rows - 1][x] === T.EMPTY) ctx.grid[rows - 1][x] = T.SOLID;
  }

  return {
    worldIndex,
    world,
    cols: ctx.cols,
    rows,
    tiles: ctx.grid,
    spawn: ctx.spawn || { x: 80, y: 200 },
    check: ctx.check,
    exit: ctx.exit,
    bossAt: ctx.bossAt,
    ents: ctx.ents,
    items: ctx.items,
    specials: ctx.specials,
    zones: ctx.zones,
    arenaX0: ctx.arenaX0,
    arenaX1: ctx.arenaX1,
    isFinal: !!wantFinal
  };
}
