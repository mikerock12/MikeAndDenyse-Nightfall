const Net = {
  mode: "local",
  connected: false,
  rooms: [],
  status: "Pronto.",
  cin: null,
  snapT: 0,
  inT: 0,
  lastSnap: null,
  prevSnap: null,
  snapAge: 0,
  roomName: "Nightfall",

  hasBridge() {
    return !!(window.NightBridge && window.NightBridge.send);
  },

  sendRaw(obj) {
    if (!this.hasBridge()) return;
    try { window.NightBridge.send(JSON.stringify(obj)); } catch (e) { /* ignore */ }
  },

  sendPeer(obj) {
    this.sendRaw({ cmd: "peer", payload: obj });
  },

  host(kind) {
    this.mode = "host";
    this.connected = false;
    this.status = kind === "p2p" ? "Criando grupo Wi-Fi Direct…" : "Anunciando na LAN…";
    this.sendRaw({ cmd: "host", kind: kind || "lan", name: this.roomName });
    this.render();
  },

  scan() {
    this.mode = "client";
    this.status = "Procurando partidas…";
    this.sendRaw({ cmd: "scan" });
    this.render();
  },

  join(id) {
    this.mode = "client";
    this.status = "Conectando…";
    this.sendRaw({ cmd: "join", id: id });
    this.render();
  },

  stop() {
    this.sendRaw({ cmd: "stop" });
    this.mode = "local";
    this.connected = false;
    this.rooms = [];
    this.status = "Desconectado.";
    this.render();
  },

  onNative(raw) {
    let d;
    try { d = typeof raw === "string" ? JSON.parse(raw) : raw; } catch (e) { return; }
    if (d.t === "status") { this.status = d.m || ""; this.render(); }
    if (d.t === "rooms") { this.rooms = d.list || []; this.render(); }
    if (d.t === "peer") {
      this.connected = !!d.ok;
      this.status = d.ok ? "Caçador conectado." : (d.m || "Aguardando par…");
      this.render();
      if (d.ok && this.mode === "host") {
        this.sendPeer({ t: "hello", hero: Game.p1Hero, role: "host" });
      }
    }
    if (d.t === "data") this.onPeer(d.p);
  },

  onPeer(p) {
    if (typeof p === "string") {
      try { p = JSON.parse(p); } catch (e) { return; }
    }
    if (!p || !p.t) return;
    if (p.t === "hello") {
      if (this.mode === "host") Game.p2Hero = p.hero || "denyse";
      if (this.mode === "client") Game.p1Hero = p.hero || "mike";
      this.status = "Pronto para caçar.";
      this.render();
    }
    if (p.t === "start" && this.mode === "client") {
      Game.p1Hero = p.hostHero || "mike";
      Game.p2Hero = p.clientHero || Game.p2Hero;
      Input.p2Joined = true;
      startLevel(p.world | 0);
    }
    if (p.t === "i" && this.mode === "host") this.cin = p;
    if (p.t === "s" && this.mode === "client") {
      this.prevSnap = this.lastSnap;
      this.lastSnap = p;
      this.snapAge = 0;
      this.applySnap(p, false);
    }
  },

  startMatch() {
    if (this.mode !== "host") return;
    this.sendPeer({ t: "hello", hero: Game.p1Hero, role: "host" });
    this.sendPeer({ t: "start", world: Game.worldIndex, hostHero: Game.p1Hero, clientHero: Game.p2Hero });
    Input.p2Joined = true;
    startLevel(Game.worldIndex);
  },

  readCtl(slot) {
    const z = { l: false, r: false, d: false, jp: false, jn: false, ap: false };
    if (this.mode === "local") {
      return {
        l: Input.left(slot), r: Input.right(slot), d: Input.downNow(slot),
        jp: Input.jumpPressed(slot), jn: Input.jumpDown(slot), ap: Input.attackPressed(slot)
      };
    }
    if (this.mode === "host") {
      if (slot === 0) {
        return {
          l: Input.left(0), r: Input.right(0), d: Input.downNow(0),
          jp: Input.jumpPressed(0), jn: Input.jumpDown(0), ap: Input.attackPressed(0)
        };
      }
      const c = this.cin || {};
      const jp = !!c.jp;
      const ap = !!c.ap;
      if (this.cin) { this.cin.jp = false; this.cin.ap = false; }
      return { l: !!c.l, r: !!c.r, d: !!c.d, jp: jp, jn: !!c.jn, ap: ap };
    }
    if (slot === 1) {
      return {
        l: Input.left(0), r: Input.right(0), d: Input.downNow(0),
        jp: Input.jumpPressed(0), jn: Input.jumpDown(0), ap: Input.attackPressed(0)
      };
    }
    return z;
  },

  tick(dt) {
    if (this.mode === "local") return;
    this.snapAge += dt;
    if (this.mode === "client") {
      this.inT += dt;
      if (this.inT >= 1 / 30) {
        this.inT = 0;
        this.sendPeer({
          t: "i",
          l: Input.left(0), r: Input.right(0), d: Input.downNow(0),
          jp: !!Input.pressed.p1j, jn: Input.jumpDown(0), ap: !!Input.pressed.p1a
        });
      }
    }
    if (this.mode === "host" && Game.state === "play") {
      this.snapT += dt;
      if (this.snapT >= 1 / 20) {
        this.snapT = 0;
        this.sendPeer(this.makeSnap());
      }
    }
  },

  makeSnap() {
    return {
      t: "s",
      w: Game.worldIndex,
      sc: Game.score,
      so: Game.souls,
      lk: Game.lockedArena,
      msg: Game.message,
      mt: Game.messageT,
      p: Game.players.map((pl) => ({
        x: pl.x, y: pl.y, vx: pl.vx, vy: pl.vy, f: pl.facing,
        hp: pl.hp, lv: pl.lives, d: pl.dead, an: pl.anim, h: pl.hero.id
      })),
      e: Game.enemies.map((en) => ({ x: en.x, y: en.y, f: en.facing, hp: en.hp, d: en.dead, id: en.id })),
      b: Game.bosses.map((bo) => ({ x: bo.x, y: bo.y, f: bo.facing, hp: bo.hp, d: bo.dead, id: bo.id })),
      it: Game.items.map((i) => ({ type: i.type, x: i.x, y: i.y }))
    };
  },

  applySnap(s) {
    if (!s || Game.state !== "play") return;
    if (s.sc != null) Game.score = s.sc;
    if (s.so != null) Game.souls = s.so;
    Game.lockedArena = !!s.lk;
    if (s.msg) { Game.message = s.msg; Game.messageT = s.mt || 1; }
    if (s.p) {
      s.p.forEach((sp, i) => {
        let pl = Game.players[i];
        if (!pl) {
          Game.players[i] = spawnPlayer(i, sp.h || (i ? Game.p2Hero : Game.p1Hero), sp.x, sp.y);
          pl = Game.players[i];
        }
        if (i === 1) return;
        pl.x = sp.x; pl.y = sp.y; pl.vx = sp.vx; pl.vy = sp.vy;
        pl.facing = sp.f; pl.hp = sp.hp; pl.lives = sp.lv;
        pl.dead = !!sp.d; pl.anim = sp.an || pl.anim;
      });
    }
    if (s.e && Game.enemies.length === s.e.length) {
      s.e.forEach((se, i) => {
        const en = Game.enemies[i];
        if (!en) return;
        en.x = se.x; en.y = se.y; en.facing = se.f; en.hp = se.hp; en.dead = !!se.d;
      });
    } else if (s.e) {
      Game.enemies = s.e.map((se) => {
        const en = spawnEnemy({ id: se.id, x: se.x, y: se.y });
        en.facing = se.f; en.hp = se.hp; en.dead = !!se.d;
        return en;
      });
    }
    if (s.b) {
      Game.bosses = s.b.map((sb) => {
        const def = bossById(sb.id);
        if (!def) return null;
        const b = spawnBoss(def, sb.x + def.w / 2, sb.y + def.h);
        b.x = sb.x; b.y = sb.y; b.facing = sb.f; b.hp = sb.hp; b.dead = !!sb.d; b.intro = 0;
        return b;
      }).filter(Boolean);
    }
    if (s.it) Game.items = s.it.map((i) => ({ type: i.type, x: i.x, y: i.y, bob: 0 }));
  },

  render() {
    const st = $("net-status");
    if (st) st.textContent = this.status + (this.hasBridge() ? "" : " (abra o APK nos celulares para LAN/P2P)");
    const list = $("net-rooms");
    if (!list) return;
    list.innerHTML = "";
    (this.rooms || []).forEach((r) => {
      const b = document.createElement("button");
      b.className = "btn";
      b.textContent = (r.name || "Nightfall") + " · " + (r.kind || "lan") + (r.host ? " · " + r.host : "");
      b.onclick = () => { AudioSys.sfx("select"); this.join(r.id); };
      list.appendChild(b);
    });
    const go = $("btn-net-go");
    if (go) go.disabled = this.mode === "host" && !this.connected && this.hasBridge();
  }
};

window.__nightNet = (msg) => Net.onNative(msg);
