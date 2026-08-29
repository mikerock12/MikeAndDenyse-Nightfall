const Input = {
  down: Object.create(null),
  pressed: Object.create(null),
  released: Object.create(null),
  p2Joined: false,
  touchMode: false,

  bind() {
    window.addEventListener("keydown", (e) => {
      AudioSys.resume();
      const k = this.mapKey(e.code);
      if (!k) return;
      if (k === "join") this.p2Joined = true;
      if (!this.down[k]) this.pressed[k] = true;
      this.down[k] = true;
      if (["p1j", "p2j", "p1a", "p2a", "pause", "ok"].includes(k)) e.preventDefault();
    });
    window.addEventListener("keyup", (e) => {
      const k = this.mapKey(e.code);
      if (!k) return;
      this.down[k] = false;
      this.released[k] = true;
    });

    const touchRoot = document.getElementById("touch");
    const onDown = (ev) => {
      this.touchMode = true;
      document.body.classList.add("touch-on");
      AudioSys.resume();
      const btn = ev.target.closest("button");
      if (!btn) return;
      const k = btn.getAttribute("data-k");
      if (!k) return;
      ev.preventDefault();
      if (k === "join") this.p2Joined = true;
      if (!this.down[k]) this.pressed[k] = true;
      this.down[k] = true;
    };
    const onUp = (ev) => {
      const btn = ev.target.closest("button");
      if (!btn) return;
      const k = btn.getAttribute("data-k");
      if (!k) return;
      this.down[k] = false;
      this.released[k] = true;
    };
    touchRoot.addEventListener("pointerdown", onDown);
    touchRoot.addEventListener("pointerup", onUp);
    touchRoot.addEventListener("pointercancel", onUp);
    touchRoot.addEventListener("pointerleave", onUp);

    window.addEventListener("pointerdown", () => {
      if (this._guessTouch()) {
        this.touchMode = true;
        document.body.classList.add("touch-on");
      }
    }, { once: true });
  },

  _guessTouch() {
    return navigator.maxTouchPoints > 0 || window.matchMedia("(pointer: coarse)").matches;
  },

  mapKey(code) {
    switch (code) {
      case "KeyA": return "p1l";
      case "KeyD": return "p1r";
      case "KeyW":
      case "Space":
      case "KeyJ":
      case "KeyZ": return "p1j";
      case "KeyS": return "p1d";
      case "KeyK":
      case "KeyX": return "p1a";
      case "ArrowLeft": return this.p2Joined ? "p2l" : "p1l";
      case "ArrowRight": return this.p2Joined ? "p2r" : "p1r";
      case "ArrowUp": return this.p2Joined ? "p2j" : "p1j";
      case "ArrowDown": return this.p2Joined ? "p2d" : "p1d";
      case "Numpad0":
      case "Period":
      case "KeyN": return "p2j";
      case "Numpad1":
      case "Slash":
      case "Comma": return "p2a";
      case "KeyM": return "mute";
      case "Enter": return "join";
      case "Escape":
      case "KeyP": return "pause";
      case "KeyF": return "ok";
      default: return null;
    }
  },

  left(p) { return !!this.down[p === 1 ? "p2l" : "p1l"]; },
  right(p) { return !!this.down[p === 1 ? "p2r" : "p1r"]; },
  downNow(p) { return !!this.down[p === 1 ? "p2d" : "p1d"]; },
  jumpPressed(p) { return !!this.pressed[p === 1 ? "p2j" : "p1j"]; },
  jumpDown(p) { return !!this.down[p === 1 ? "p2j" : "p1j"]; },
  attackPressed(p) { return !!this.pressed[p === 1 ? "p2a" : "p1a"]; },

  endFrame() {
    for (const k of Object.keys(this.pressed)) this.pressed[k] = false;
    for (const k of Object.keys(this.released)) this.released[k] = false;
  }
};
