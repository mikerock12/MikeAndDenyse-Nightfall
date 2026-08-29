const AudioSys = {
  ctx: null,
  master: null,
  musicGain: null,
  sfxGain: null,
  muted: false,
  musicTimer: 0,
  theme: "title",
  step: 0,

  init() {
    if (this.ctx) return;
    const AC = window.AudioContext || window.webkitAudioContext;
    this.ctx = new AC();
    this.master = this.ctx.createGain();
    this.musicGain = this.ctx.createGain();
    this.sfxGain = this.ctx.createGain();
    this.master.gain.value = 0.28;
    this.musicGain.gain.value = 0.22;
    this.sfxGain.gain.value = 0.55;
    this.musicGain.connect(this.master);
    this.sfxGain.connect(this.master);
    this.master.connect(this.ctx.destination);
  },

  resume() {
    this.init();
    if (this.ctx.state === "suspended") this.ctx.resume();
  },

  toggleMute() {
    this.muted = !this.muted;
    if (this.master) this.master.gain.value = this.muted ? 0 : 0.28;
  },

  tone(freq, dur, type, vol, dest) {
    if (!this.ctx) return;
    const t = this.ctx.currentTime;
    const o = this.ctx.createOscillator();
    const g = this.ctx.createGain();
    o.type = type || "square";
    o.frequency.setValueAtTime(freq, t);
    g.gain.setValueAtTime(0.0001, t);
    g.gain.exponentialRampToValueAtTime(vol || 0.12, t + 0.01);
    g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
    o.connect(g);
    g.connect(dest || this.sfxGain);
    o.start(t);
    o.stop(t + dur + 0.02);
  },

  noise(dur, vol) {
    if (!this.ctx) return;
    const n = this.ctx.sampleRate * dur;
    const buf = this.ctx.createBuffer(1, n, this.ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n);
    const src = this.ctx.createBufferSource();
    const g = this.ctx.createGain();
    const f = this.ctx.createBiquadFilter();
    f.type = "lowpass";
    f.frequency.value = 900;
    src.buffer = buf;
    g.gain.value = vol || 0.12;
    src.connect(f);
    f.connect(g);
    g.connect(this.sfxGain);
    src.start();
  },

  sfx(name) {
    this.resume();
    if (name === "jump") { this.tone(520, 0.09, "square", 0.08); this.tone(780, 0.08, "square", 0.05); }
    if (name === "attack") { this.tone(240, 0.08, "sawtooth", 0.09); this.noise(0.06, 0.06); }
    if (name === "magic") { this.tone(660, 0.12, "triangle", 0.08); this.tone(990, 0.16, "sine", 0.06); }
    if (name === "hit") { this.tone(160, 0.1, "square", 0.1); this.noise(0.08, 0.1); }
    if (name === "hurt") { this.tone(140, 0.18, "sawtooth", 0.1); this.tone(90, 0.2, "square", 0.07); }
    if (name === "coin") { this.tone(880, 0.07, "square", 0.07); this.tone(1320, 0.1, "square", 0.05); }
    if (name === "heart") { this.tone(520, 0.08, "sine", 0.07); this.tone(780, 0.12, "sine", 0.06); }
    if (name === "stomp") { this.tone(110, 0.09, "triangle", 0.1); this.noise(0.05, 0.08); }
    if (name === "boss") { this.tone(55, 0.4, "sawtooth", 0.12); this.tone(70, 0.35, "square", 0.08); }
    if (name === "clear") { [523, 659, 784, 1046].forEach((f, i) => setTimeout(() => this.tone(f, 0.16, "square", 0.07), i * 120)); }
    if (name === "die") { this.tone(200, 0.3, "sawtooth", 0.1); this.tone(80, 0.45, "triangle", 0.08); }
    if (name === "select") { this.tone(440, 0.06, "square", 0.06); }
    if (name === "land") { this.noise(0.04, 0.05); }
  },

  setTheme(theme) { this.theme = theme; this.step = 0; },

  tick(dt) {
    if (!this.ctx || this.muted) return;
    this.musicTimer += dt;
    const bpm = this.theme === "boss" ? 112 : this.theme === "final" ? 96 : 78;
    const beat = 60 / bpm;
    if (this.musicTimer < beat / 2) return;
    this.musicTimer = 0;
    this.step++;

    const minor = [220, 261, 293, 329, 349, 392, 440];
    const dark = [196, 233, 261, 294, 311, 349, 392];
    const scale = this.theme === "title" ? dark : minor;
    const root = this.theme === "final" ? 98 : this.theme === "boss" ? 110 : 130.8;

    if (this.step % 8 === 0) this.tone(root, 0.55, "triangle", 0.045, this.musicGain);
    if (this.step % 4 === 2) this.tone(root * 1.5, 0.28, "sine", 0.03, this.musicGain);

    const idx = [0, 2, 3, 2, 4, 3, 2, 0][this.step % 8];
    const extra = this.theme === "boss" ? 1.33 : 1;
    this.tone(scale[idx] * extra * 0.5, 0.18, "square", 0.028, this.musicGain);
    if (this.step % 2 === 0) this.tone(scale[(idx + 2) % scale.length] * 0.25, 0.22, "triangle", 0.03, this.musicGain);
  }
};
