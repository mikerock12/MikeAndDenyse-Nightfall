using System;
using System.Collections.Generic;
using Nightfall.Net;
using Unity.Collections;
using Unity.Netcode;
using Unity.Netcode.Transports.UTP;
using UnityEngine;

namespace Nightfall
{
    public class NightApp : MonoBehaviour
    {
        public static NightApp I;
        public GameSim Sim = new();
        public SpriteBank Bank = new();
        WorldView _view;
        GameMenu _menu;
        string _screen = "title";
        string _p1 = "mike", _p2 = "denyse";
        int _world;
        int _mapTap = -1;
        string _status = "Os dois no mesmo Wi-Fi. Um cria a sala, o outro procura.";
        string _error;
        readonly List<RoomInfo> _rooms = new();
        LanDiscovery _disc;
        string _mode = "local";
        float _snapT;
        GameObject _gameNetPrefab;
        bool _netReady;
        string _roomKey = "";
        bool _matchLive;
        bool _heroSent;
        string _lastH1 = "mike", _lastH2 = "denyse";
        float _backLock;

        void Awake()
        {
            I = this;
            DontDestroyOnLoad(gameObject);
            Application.targetFrameRate = 60;
            Input.multiTouchEnabled = true;
            Screen.sleepTimeout = SleepTimeout.NeverSleep;
            Screen.orientation = ScreenOrientation.LandscapeLeft;
            Application.logMessageReceived += OnLog;
            try
            {
                ArtGen.Ensure();
                Bank.LoadAll();
                Sfx.Ensure();
                var viewGo = new GameObject("View");
                DontDestroyOnLoad(viewGo);
                _view = viewGo.AddComponent<WorldView>();
                Sim.ReadCtl = ReadCtl;
                Sim.Sfx = Sfx.Play;
                var menuGo = new GameObject("GameMenu");
                DontDestroyOnLoad(menuGo);
                _menu = menuGo.AddComponent<GameMenu>();
                _menu.Bind(this);
                _menu.SetSprites(Bank);
                Progress.Load();
                _p1 = Progress.Hero;
                _world = Progress.LastWorld;
                _menu.RefreshHero(_p1);
                Show("title");
            }
            catch (Exception e)
            {
                _error = e.Message;
                Debug.LogException(e);
            }
        }

        void OnDestroy()
        {
            Application.logMessageReceived -= OnLog;
            WifiDirectBridge.ReleaseMulticastLock();
            StopNet();
        }

        void OnLog(string cond, string stack, LogType type)
        {
            if (type != LogType.Exception || string.IsNullOrEmpty(cond)) return;
            if (cond.IndexOf("Nightfall", StringComparison.OrdinalIgnoreCase) < 0
                && cond.IndexOf("NightApp", StringComparison.OrdinalIgnoreCase) < 0
                && cond.IndexOf("WorldView", StringComparison.OrdinalIgnoreCase) < 0)
                return;
            _error = cond;
            _menu?.SetError(cond);
        }

        public void Show(string id)
        {
            _screen = id;
            // the world stays visible behind pause / death / victory so the screen is never blank
            if (_view != null) _view.Visible = id is "play" or "pause" or "dead" or "clear" or "win";
            _menu?.SetScreen(id);
            _menu?.SetStatus(_status);
            _menu?.SetError(_error);
            if (id == "map")
            {
                _mapTap = -1;
                if (!Progress.CanPlay(_world)) _world = Mathf.Clamp(Progress.Unlocked - 1, 0, 15);
                _menu?.RefreshMap(_world, Progress.Unlocked);
            }
            if (id == "select")
            {
                _menu?.RefreshHero(_p1);
                _menu?.SetSelectCopy(_mode != "local");
            }
            if (id == "net")
            {
                _roomKey = "";
                RefreshRoomsIfChanged();
            }
            if (id == "dead") _menu?.SetDeadCopy(_mode == "client");
            if (id == "clear") _menu?.SetClearCopy(_mode == "client", _world);
        }

        // ───────────────────────────── ui actions ─────────────────────────────

        public void UiLocal() { _mode = "local"; Show("select"); }
        public void UiGotoNetFromTitle() { _mode = "net"; Show("select"); }
        public void UiShow(string id)
        {
            if (id == "title") StopNet();
            if (id == "map" && _mode == "client") { Show("net"); return; }
            Show(id);
        }

        public void UiQuit()
        {
            try { Progress.Save(); StopNet(); } catch { }
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying = false;
#else
            Application.Quit();
#endif
        }

        public void UiPickHero(string id)
        {
            _p1 = id == "denyse" ? "denyse" : "mike";
            _p2 = _p1 == "mike" ? "denyse" : "mike";
            Progress.Hero = _p1;
            Progress.Save();
            _menu.RefreshHero(_p1);
            Sfx.Play("ui");
        }
        public void UiConfirmHero() { Show(_mode == "local" ? "map" : "net"); }
        public void UiPickWorld(int i)
        {
            if (!Progress.CanPlay(i))
            {
                _menu?.RefreshMap(_world, Progress.Unlocked);
                _menu?.SetStatus("Passe a fase " + Progress.Unlocked + " para abrir esta.");
                return;
            }
            if (i == _world && _mapTap == i)
            {
                UiStartWorld();
                return;
            }
            _world = i;
            _mapTap = i;
            Progress.LastWorld = i;
            Progress.Save();
            Sfx.Play("ui");
            _menu.RefreshMap(_world, Progress.Unlocked);
        }
        public void UiHostLan() => HostLan();
        public void UiHostP2P() => HostP2P();
        public void UiScan() => Scan();
        public void UiJoin(RoomInfo r) => Join(r);
        public void UiNetGo()
        {
            if (_mode == "client")
            {
                if (NetworkManager.Singleton == null || !NetworkManager.Singleton.IsClient)
                {
                    _status = "Entre numa sala primeiro — toque Procurar sala.";
                    _menu.SetStatus(_status);
                    return;
                }
                _status = "Conectado. O host escolhe a fase e começa.";
                _menu.SetStatus(_status);
                return;
            }
            if (_mode == "host" && (NetworkManager.Singleton == null || !NetworkManager.Singleton.IsHost))
            {
                _status = "Crie a sala primeiro.";
                _menu.SetStatus(_status);
                return;
            }
            Show("map");
        }
        public void UiNetBack() { StopNet(); Show("title"); }

        public void UiStartWorld()
        {
            try
            {
                if (_mode == "client")
                {
                    _status = "Aguardando o host iniciar a fase…";
                    _menu.SetStatus(_status);
                    Show("net");
                    return;
                }
                if (!Progress.CanPlay(_world))
                {
                    _status = "Essa fase ainda está fechada.";
                    _menu.SetStatus(_status);
                    Show("map");
                    return;
                }
                _error = null;
                _menu?.SetError(null);
                if (_mode == "host" && GameNet.Instance != null)
                {
                    GameNet.Instance.StartMatchRpc(_world, new FixedString64Bytes(_p1), new FixedString64Bytes(_p2));
                    return;
                }
                BeginMatch(_world, _p1, _p2, false);
            }
            catch (Exception e)
            {
                _error = "Falha ao abrir a fase: " + e.Message;
                _menu?.SetError(_error);
                Show("map");
                Debug.LogException(e);
            }
        }

        public void UiPause()
        {
            if (_screen != "play") return;
            if (_mode != "client") Sim.State = "pause";
            Show("pause");
            Sfx.Play("ui");
        }
        public void UiResume()
        {
            if (_mode != "client") Sim.State = "play";
            Show("play");
        }
        /// <summary>Pause → back to the level select (host tells the other phone too).</summary>
        public void UiAbandon()
        {
            Sim.State = "map";
            _matchLive = false;
            if (_mode == "host" && GameNet.Instance != null)
                GameNet.Instance.EndMatchRpc(new FixedString32Bytes("map"));
            Show(_mode == "client" ? "net" : "map");
        }
        /// <summary>Pause → all the way back to the title.</summary>
        public void UiToTitle()
        {
            Sim.State = "map";
            _matchLive = false;
            if (_mode == "host" && GameNet.Instance != null)
                GameNet.Instance.EndMatchRpc(new FixedString32Bytes("title"));
            StopNet();
            Show("title");
        }

        public void UiNext()
        {
            if (_mode == "client")
            {
                _status = "O host escolhe a próxima fase.";
                _menu.SetStatus(_status);
                Show("net");
                return;
            }
            Progress.OnClear(_world);
            Progress.RememberRun(Sim.Souls, Sim.Score, _p1);
            if (_world >= 15) { Show("win"); return; }
            _world = Mathf.Min(15, _world + 1);
            if (!Progress.CanPlay(_world)) { Show("map"); return; }
            if (_mode == "host" && GameNet.Instance != null)
                GameNet.Instance.StartMatchRpc(_world, new FixedString64Bytes(_p1), new FixedString64Bytes(_p2));
            else
                BeginMatch(_world, _p1, _p2, _mode == "host");
        }

        public void UiRetry()
        {
            if (_mode == "client")
            {
                _status = "Aguarde — o host reinicia a fase.";
                _menu.SetStatus(_status);
                Show("net");
                return;
            }
            if (_mode == "host" && GameNet.Instance != null)
                GameNet.Instance.StartMatchRpc(_world, new FixedString64Bytes(_p1), new FixedString64Bytes(_p2));
            else
                BeginMatch(_world, _p1, _p2, _mode == "host");
        }

        /// <summary>Android back / Esc. Never dead-ends: every screen knows where it came from.</summary>
        public void BackPressed()
        {
            if (_backLock > 0) return;
            _backLock = 0.25f;
            switch (_screen)
            {
                case "play": UiPause(); break;
                case "pause": UiResume(); break;
                case "howto": Show("title"); break;
                case "select": UiShow("title"); break;
                case "net": UiNetBack(); break;
                case "map": UiShow("title"); break;
                case "dead": UiShow(_mode == "client" ? "net" : "map"); break;
                case "clear": UiShow(_mode == "client" ? "net" : "map"); break;
                case "win": UiShow("title"); break;
                default: UiQuit(); break;
            }
        }

        // ───────────────────────────── loop ─────────────────────────────

        void Update()
        {
            try
            {
                _backLock = Mathf.Max(0, _backLock - Time.deltaTime);

                if (_screen == "play") PlayTouch.Sample();
                else PlayTouch.Reset();

                bool back = Input.GetKeyDown(KeyCode.Escape) || Input.GetKeyDown(KeyCode.P);
                if (back || (PlayTouch.PauseDown && _screen == "play")) BackPressed();

                if (_mode == "client" && GameNet.Instance != null && !_heroSent)
                {
                    GameNet.Instance.ClaimHeroRpc(new FixedString64Bytes(_p1));
                    _heroSent = true;
                }

                _disc?.Tick();
                if (_screen == "net") RefreshRoomsIfChanged();

                if (_mode == "client" && GameNet.Instance != null && _screen == "play")
                {
                    var c = PollP1();
                    GameNet.Instance.SubmitInputRpc(new InputMsg { L = c.L, R = c.R, D = c.D, Jp = c.Jp, Jn = c.Jn, Ap = c.Ap });
                }

                if (_screen == "play" && Sim != null && Sim.Level != null)
                {
                    if (_mode != "client")
                    {
                        Sim.Tick(Mathf.Min(Time.deltaTime, 0.05f));
                        if (_mode == "host" && GameNet.Instance != null)
                        {
                            GameNet.LastFromClient.Jp = false;
                            GameNet.LastFromClient.Ap = false;
                            _snapT += Time.deltaTime;
                            if (_snapT >= 0.05f && Sim.Players.Count >= 1)
                            {
                                _snapT = 0;
                                GameNet.Instance.StateRpc(BuildSnap());
                            }
                        }
                    }
                    else Sim.TickRemote(Mathf.Min(Time.deltaTime, 0.05f));

                    PumpFx();
                }
                else if (Sim != null) Sim.Fx.Clear();

                SyncScreen();
            }
            catch (Exception e)
            {
                _error = e.Message;
                _menu?.SetError(_error);
                Debug.LogException(e);
                // never leave the player staring at nothing
                if (_screen == "play" && Sim != null && Sim.State == "play") Show("pause");
            }
        }

        /// <summary>Single owner of the FX queue: the view can't steal events before the snapshot ships.</summary>
        void PumpFx()
        {
            if (Sim.Fx.Count == 0) return;
            if (_view != null) foreach (var f in Sim.Fx) _view.PushFx(f);
            Sim.Fx.Clear();
        }

        /// <summary>
        /// Keeps screen, sim state and the menu panel in agreement every frame. A missed transition
        /// used to leave the world hidden with no panel up — a black, frozen screen after dying.
        /// </summary>
        void SyncScreen()
        {
            if (Sim != null && _screen == "play" && Sim.State != "play")
            {
                switch (Sim.State)
                {
                    case "dead": Show("dead"); break;
                    case "clear": NoteClear(); Show("clear"); break;
                    case "win": NoteClear(); Show("win"); break;
                    case "pause": Show("pause"); break;
                }
            }
            if (_menu != null && _menu.ScreenId != _screen) _menu.SetScreen(_screen);
            if (_view != null)
            {
                bool want = _screen is "play" or "pause" or "dead" or "clear" or "win";
                if (_view.Visible != want) _view.Visible = want;
            }
        }

        void NoteClear()
        {
            if (_mode == "client") return;
            Progress.OnClear(_world);
            Progress.RememberRun(Sim.Souls, Sim.Score, _p1);
        }

        Ctl ReadCtl(int slot)
        {
            if (_mode == "local") return slot == 0 ? PollP1() : new Ctl();
            if (_mode == "host") return slot == 0 ? PollP1() : ToCtl(GameNet.LastFromClient);
            return slot == 1 ? PollP1() : new Ctl();
        }

        static Ctl ToCtl(InputMsg m) => new() { L = m.L, R = m.R, D = m.D, Jp = m.Jp, Jn = m.Jn, Ap = m.Ap };

        Ctl PollP1()
        {
            var t = PlayTouch.Current ?? new Ctl();
            return new Ctl
            {
                L = Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow) || t.L,
                R = Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow) || t.R,
                D = Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow) || t.D,
                Jp = Input.GetKeyDown(KeyCode.W) || Input.GetKeyDown(KeyCode.Space) || Input.GetKeyDown(KeyCode.J) || Input.GetKeyDown(KeyCode.Z) || t.Jp,
                Jn = Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.Space) || Input.GetKey(KeyCode.J) || Input.GetKey(KeyCode.Z) || t.Jn,
                Ap = Input.GetKeyDown(KeyCode.K) || Input.GetKeyDown(KeyCode.X) || t.Ap
            };
        }

        // ───────────────────────────── match ─────────────────────────────

        public void OnClientHero(string hero)
        {
            if (string.IsNullOrEmpty(hero) || !Catalog.Heroes.ContainsKey(hero)) return;
            _p2 = hero;
            if (_p2 == _p1) _p2 = _p1 == "mike" ? "denyse" : "mike";
            _lastH2 = _p2;
            _status = "Jogador 2 é " + Catalog.Heroes[_p2].Name + ". Sigam — o host escolhe a fase.";
            _menu?.SetStatus(_status);
        }

        public void OnHostEndedMatch(string reason)
        {
            _matchLive = false;
            Sim.State = "map";
            _status = reason == "title" ? "O host encerrou a partida." : "O host voltou ao mapa. Aguarde a próxima fase.";
            _menu?.SetStatus(_status);
            Show(reason == "title" ? "title" : "net");
        }

        public void BeginMatch(int world, string hostHero, string clientHero, bool two)
        {
            try
            {
                world = Mathf.Clamp(world, 0, Catalog.Worlds.Count - 1);
                if (string.IsNullOrEmpty(hostHero) || !Catalog.Heroes.ContainsKey(hostHero)) hostHero = "mike";
                if (string.IsNullOrEmpty(clientHero) || !Catalog.Heroes.ContainsKey(clientHero)) clientHero = "denyse";
                _p1 = hostHero; _p2 = clientHero; _world = world;
                _lastH1 = hostHero; _lastH2 = clientHero;
                _matchLive = true;
                Sim.Start(world, hostHero, clientHero, two);
                if (_view == null)
                {
                    var viewGo = new GameObject("View");
                    DontDestroyOnLoad(viewGo);
                    _view = viewGo.AddComponent<WorldView>();
                }
                _view.Boot(Sim, Bank);
                _view.Sim = Sim;
                _view.PrepareWorld();
                Sim.Fx.Clear();
                _error = null;
                _menu?.SetError(null);
                if (_mode != "client")
                {
                    Progress.LastWorld = world;
                    Progress.Hero = hostHero;
                    Progress.Save();
                }
                Show("play");
            }
            catch (Exception e)
            {
                _error = "Fase: " + e.Message;
                _menu?.SetError(_error);
                Show(_mode == "client" ? "net" : "map");
                Debug.LogException(e);
            }
        }

        // ───────────────────────────── snapshot ─────────────────────────────

        static byte AnimId(string a) => a switch { "walk" => 1, "jump" => 2, "fall" => 3, "attack" => 4, _ => (byte)0 };
        static string AnimName(byte a) => a switch { 1 => "walk", 2 => "jump", 3 => "fall", 4 => "attack", _ => "idle" };

        static byte B(int v) => (byte)Mathf.Clamp(v, 0, 255);

        static PlrSnap Pack(PlayerA p)
        {
            if (p == null) return default;
            byte flags = 0;
            if (p.Inv > 0) flags |= 1;
            if (p.HurtT > 0) flags |= 2;
            if (p.Dead) flags |= 4;
            if (p.Facing < 0) flags |= 8;
            float atkMax = Mathf.Max(0.01f, p.Hero != null ? p.Hero.AtkTime : 0.3f);
            return new PlrSnap
            {
                X = p.X, Y = p.Y, Hp = B(p.Hp), MaxHp = B(p.MaxHp), Lives = B(p.Lives + 1),
                Anim = AnimId(p.Anim), Atk = (byte)Mathf.Clamp(Mathf.RoundToInt(p.Atk / atkMax * 255f), 0, 255),
                Flags = flags, Hero = (byte)(p.Hero != null && p.Hero.Id == "denyse" ? 1 : 0)
            };
        }

        void Unpack(PlayerA p, PlrSnap s)
        {
            if (p == null) return;
            float d = Mathf.Abs(p.X - s.X) + Mathf.Abs(p.Y - s.Y);
            if (d > 200) { p.X = s.X; p.Y = s.Y; }
            else { p.X = Mathf.Lerp(p.X, s.X, 0.5f); p.Y = Mathf.Lerp(p.Y, s.Y, 0.5f); }
            if (p.Hp > s.Hp) p.HurtT = 0.25f;
            p.Hp = s.Hp;
            if (s.MaxHp > 0) p.MaxHp = s.MaxHp;
            p.Lives = s.Lives - 1;
            p.Facing = (s.Flags & 8) != 0 ? -1 : 1;
            p.Anim = AnimName(s.Anim);
            p.Atk = s.Atk / 255f * (p.Hero != null ? p.Hero.AtkTime : 0.3f);
            p.Inv = (s.Flags & 1) != 0 ? Mathf.Max(p.Inv, 0.12f) : 0;
            if ((s.Flags & 2) != 0) p.HurtT = 0.25f;
            p.Dead = (s.Flags & 4) != 0;
        }

        FrameSnap BuildSnap()
        {
            var p0 = Sim.Players.Count > 0 ? Sim.Players[0] : null;
            var p1 = Sim.Players.Count > 1 ? Sim.Players[1] : null;

            int n = Mathf.Min(FrameSnap.MaxEnts, Sim.Enemies.Count);
            var ents = new EntSnap[n];
            for (int i = 0; i < n; i++)
            {
                var e = Sim.Enemies[i];
                byte fl = 0;
                if (e.Dead) fl |= 1;
                if (e.Flash > 0) fl |= 2;
                if (e.Facing < 0) fl |= 4;
                ents[i] = new EntSnap { X = e.X, Y = e.Y, Hp = B(e.Hp), Kind = B(e.Kind), Flags = fl };
            }

            int np = Mathf.Min(FrameSnap.MaxProjs, Sim.Projectiles.Count);
            var projs = new ProjSnap[np];
            for (int i = 0; i < np; i++)
            {
                var pr = Sim.Projectiles[i];
                projs[i] = new ProjSnap
                {
                    X = pr.X, Y = pr.Y, Vx = pr.Vx, Vy = pr.Vy,
                    Kind = GameSim.ProjKindId(pr.Kind), Flags = (byte)(pr.Friendly ? 1 : 0)
                };
            }

            int nf = Mathf.Min(FrameSnap.MaxFx, Sim.Fx.Count);
            var fxs = new FxSnap[nf];
            for (int i = 0; i < nf; i++)
                fxs[i] = new FxSnap
                {
                    X = Sim.Fx[i].X, Y = Sim.Fx[i].Y, Kind = Sim.Fx[i].Kind,
                    Dir = (byte)(Sim.Fx[i].Dir < 0 ? 0 : 1)
                };

            int bHp = 0, bMax = 0;
            byte bKind = 0, bFlags = 1;
            Vector2 bPos = Vector2.zero;
            if (Sim.Bosses.Count > 0)
            {
                var b = Sim.Bosses[Sim.Bosses.Count - 1];
                bHp = b.Hp; bMax = b.MaxHp; bPos = new Vector2(b.X, b.Y);
                bKind = B(Catalog.BossIndex(b.Id));
                bFlags = (byte)((b.Dead ? 1 : 0) | (b.Facing < 0 ? 2 : 0));
            }

            string state = Sim.State == "pause" ? "play" : Sim.State;
            return new FrameSnap
            {
                P0 = Pack(p0), P1 = Pack(p1),
                Score = Sim.Score, Souls = Sim.Souls,
                LockArena = Sim.LockedArena,
                SimState = new FixedString32Bytes(state ?? "play"),
                BossHp = bHp, BossMax = bMax, BossPos = bPos, BossKind = bKind, BossFlags = bFlags,
                ItemMask = Sim.ItemMask(),
                Shake = Sim.Shake,
                Msg = new FixedString64Bytes(Trim(Sim.Message)),
                MsgT = Sim.MessageT,
                Ents = ents, Projs = projs, Fxs = fxs
            };
        }

        static string Trim(string s)
        {
            if (string.IsNullOrEmpty(s)) return "";
            return s.Length <= 58 ? s : s.Substring(0, 58);
        }

        public void ApplyRemote(FrameSnap s)
        {
            if (Sim == null || Sim.Level == null) return;
            if (Sim.Players.Count > 0) Unpack(Sim.Players[0], s.P0);
            if (Sim.Players.Count > 1) Unpack(Sim.Players[1], s.P1);

            if (s.BossMax > 0 && Sim.Bosses.Count == 0)
            {
                var def = Catalog.BossAt(s.BossKind) ?? Catalog.Boss(Sim.Level.World.Boss) ?? Catalog.Belial;
                Sim.Bosses.Add(new BossA
                {
                    Id = def.Id, Def = def, X = s.BossPos.x, Y = s.BossPos.y,
                    W = def.W, H = def.H, Hp = s.BossHp, MaxHp = s.BossMax, Facing = -1, Intro = 0
                });
            }
            if (Sim.Bosses.Count > 0)
            {
                var b = Sim.Bosses[Sim.Bosses.Count - 1];
                var def = Catalog.BossAt(s.BossKind);
                if (def != null && b.Id != def.Id)
                {
                    b.Id = def.Id; b.Def = def; b.W = def.W; b.H = def.H;
                    Sim.BelialPhase = def.Id == "belial";
                }
                if (b.Hp > s.BossHp) b.Flash = 0.14f;
                b.Hp = s.BossHp; b.MaxHp = s.BossMax;
                b.Dead = (s.BossFlags & 1) != 0;
                b.Facing = (s.BossFlags & 2) != 0 ? -1 : 1;
                b.Intro = 0;
                float d = Mathf.Abs(b.X - s.BossPos.x) + Mathf.Abs(b.Y - s.BossPos.y);
                if (d > 260) { b.X = s.BossPos.x; b.Y = s.BossPos.y; }
                else { b.X = Mathf.Lerp(b.X, s.BossPos.x, 0.5f); b.Y = Mathf.Lerp(b.Y, s.BossPos.y, 0.5f); }
            }

            Sim.ApplyNetEnemies(s.Ents);
            Sim.ApplyNetProjectiles(s.Projs);
            Sim.ApplyNetItems(s.ItemMask);
            if (s.Fxs != null)
                foreach (var f in s.Fxs)
                    Sim.Fx.Add(new FxEvent { Kind = f.Kind, X = f.X, Y = f.Y, Dir = f.Dir });

            Sim.Score = s.Score; Sim.Souls = s.Souls; Sim.LockedArena = s.LockArena;
            Sim.Shake = Mathf.Max(Sim.Shake, s.Shake);
            string msg = s.Msg.ToString();
            if (!string.IsNullOrEmpty(msg) && msg != Sim.Message) { Sim.Message = msg; Sim.MessageT = s.MsgT; }
            if (!s.SimState.IsEmpty) Sim.State = s.SimState.ToString();
            PumpFx();
        }

        // ───────────────────────────── net plumbing ─────────────────────────────

        bool EnsureNet()
        {
            if (_netReady && NetworkManager.Singleton != null) return true;
            try
            {
                _gameNetPrefab = Resources.Load<GameObject>("GameNet");
                var nm = NetworkManager.Singleton;
                if (nm == null)
                {
                    var found = GameObject.Find("NetworkManager");
                    if (found != null) nm = found.GetComponent<NetworkManager>();
                }
                if (nm == null)
                {
                    var go = new GameObject("NetworkManager");
                    DontDestroyOnLoad(go);
                    nm = go.AddComponent<NetworkManager>();
                    var utp = go.AddComponent<UnityTransport>();
                    nm.NetworkConfig = new NetworkConfig
                    {
                        NetworkTransport = utp,
                        TickRate = 30,
                        EnableSceneManagement = false,
                        PlayerPrefab = null
                    };
                    if (_gameNetPrefab != null)
                    {
                        var list = ScriptableObject.CreateInstance<NetworkPrefabsList>();
                        list.Add(new NetworkPrefab { Prefab = _gameNetPrefab });
                        nm.NetworkConfig.Prefabs.NetworkPrefabsLists.Add(list);
                    }
                }
                else
                {
                    if (nm.NetworkConfig == null)
                    {
                        var utp = nm.GetComponent<UnityTransport>() ?? nm.gameObject.AddComponent<UnityTransport>();
                        nm.NetworkConfig = new NetworkConfig
                        {
                            NetworkTransport = utp,
                            TickRate = 30,
                            EnableSceneManagement = false
                        };
                    }
                    if (nm.NetworkConfig.NetworkTransport == null)
                    {
                        var utp = nm.GetComponent<UnityTransport>() ?? nm.gameObject.AddComponent<UnityTransport>();
                        nm.NetworkConfig.NetworkTransport = utp;
                    }
                    nm.NetworkConfig.EnableSceneManagement = false;
                    nm.NetworkConfig.PlayerPrefab = null;
                }
                nm.OnServerStarted -= OnServerStarted;
                nm.OnServerStarted += OnServerStarted;
                nm.OnClientConnectedCallback -= OnClientConnected;
                nm.OnClientConnectedCallback += OnClientConnected;
                _netReady = true;
                return true;
            }
            catch (Exception e)
            {
                _error = "Net: " + e.Message;
                _menu?.SetError(_error);
                return false;
            }
        }

        void OnServerStarted()
        {
            SpawnGameNet();
            _status = "Sala no ar em " + LocalIp() + ":7777 — o outro celular deve Procurar sala.";
            _menu?.SetStatus(_status);
        }

        void OnClientConnected(ulong id)
        {
            var nm = NetworkManager.Singleton;
            if (nm != null && nm.IsServer && id != NetworkManager.ServerClientId)
            {
                _status = "Jogador 2 conectado · " + LocalIp() + " · sigam para o mapa.";
                if (_matchLive && GameNet.Instance != null)
                    GameNet.Instance.StartMatchRpc(_world, new FixedString64Bytes(_lastH1), new FixedString64Bytes(_lastH2));
            }
            else if (nm != null && nm.IsClient)
                _status = "Conectado. Espere — só o host escolhe a fase e começa.";
            _menu?.SetStatus(_status);
        }

        void SpawnGameNet()
        {
            try
            {
                if (NetworkManager.Singleton == null || !NetworkManager.Singleton.IsServer) return;
                if (GameNet.Instance != null) return;
                if (_gameNetPrefab == null) _gameNetPrefab = Resources.Load<GameObject>("GameNet");
                if (_gameNetPrefab == null) return;
                var go = Instantiate(_gameNetPrefab);
                go.SetActive(true);
                go.GetComponent<NetworkObject>()?.Spawn();
            }
            catch (Exception e) { _error = "Spawn: " + e.Message; }
        }

        static string LocalIp() => LanIp.Pick();

        void HostLan()
        {
            if (!EnsureNet()) { _status = "Falha ao preparar a rede."; _menu.SetStatus(_status); return; }
            _mode = "host";
            try
            {
                var nm = NetworkManager.Singleton;
                if (nm.IsClient || nm.IsServer || nm.IsHost)
                {
                    _status = "Já há uma sessão. Volte ao título e crie de novo.";
                    _menu.SetStatus(_status);
                    return;
                }
                WifiDirectBridge.AcquireMulticastLock();
                string ip = LanIp.Pick();
                nm.GetComponent<UnityTransport>().SetConnectionData("0.0.0.0", 7777, "0.0.0.0");
                if (nm.StartHost())
                {
                    _disc ??= new LanDiscovery();
                    _disc.StartAdvertise("Mike & Denyse", 7777);
                    _status = ip == "0.0.0.0"
                        ? "Sem IP Wi-Fi. Liguem os dois no mesmo Wi-Fi e tentem de novo."
                        : "Sala pronta · " + ip + ":7777 · o outro toca Procurar sala";
                }
                else _status = "Não foi possível criar a sala.";
            }
            catch (Exception e) { _status = e.Message; }
            _menu.SetStatus(_status);
        }

        void HostP2P()
        {
            if (!EnsureNet()) { _status = "Falha ao preparar a rede."; _menu.SetStatus(_status); return; }
            _mode = "host";
            try
            {
                var nm = NetworkManager.Singleton;
                if (nm.IsClient || nm.IsServer || nm.IsHost)
                {
                    _status = "Já há uma sessão. Volte ao título e crie de novo.";
                    _menu.SetStatus(_status);
                    return;
                }
                WifiDirectBridge.CreateGroup();
                WifiDirectBridge.AcquireMulticastLock();
                nm.GetComponent<UnityTransport>().SetConnectionData("0.0.0.0", 7777, "0.0.0.0");
                nm.StartHost();
                _disc ??= new LanDiscovery();
                _disc.StartAdvertise("Mike & Denyse", 7777);
                string ip = LanIp.Pick();
                _status = "Wi-Fi Direct + LAN · " + ip + ":7777 · o outro toca Procurar sala";
            }
            catch (Exception e) { _status = e.Message; }
            _menu.SetStatus(_status);
        }

        void Scan()
        {
            _mode = "client";
            try
            {
                WifiDirectBridge.Discover();
                WifiDirectBridge.AcquireMulticastLock();
                _disc ??= new LanDiscovery();
                _disc.StartListen();
                _roomKey = "";
                _status = "Procurando no Wi-Fi… os dois aparelhos precisam da mesma rede.";
            }
            catch (Exception e) { _status = e.Message; }
            _menu.SetStatus(_status);
        }

        void RefreshRoomsIfChanged()
        {
            if (_disc == null) return;
            var key = _disc.Fingerprint();
            if (key == _roomKey) return;
            _roomKey = key;
            _rooms.Clear();
            foreach (var r in _disc.Rooms)
                if (LanIp.IsUsable(r.host)) _rooms.Add(r);
            _menu?.RefreshRooms(_rooms);
        }

        void Join(RoomInfo r)
        {
            if (r == null || !LanIp.IsUsable(r.host))
            {
                _status = "Sala com IP inválido. Peça ao host para criar de novo na mesma rede Wi-Fi.";
                _menu.SetStatus(_status);
                return;
            }
            if (!EnsureNet()) { _status = "Falha ao preparar a rede."; _menu.SetStatus(_status); return; }
            _mode = "client";
            try
            {
                var nm = NetworkManager.Singleton;
                if (nm.IsClient || nm.IsServer || nm.IsHost)
                {
                    _status = "Já conectado. Volte ao título para entrar noutra sala.";
                    _menu.SetStatus(_status);
                    return;
                }
                WifiDirectBridge.AcquireMulticastLock();
                ushort port = (ushort)(r.port > 0 ? r.port : 7777);
                nm.GetComponent<UnityTransport>().SetConnectionData(r.host, port);
                _status = nm.StartClient() ? "Conectando a " + r.host + ":" + port : "Falha ao conectar em " + r.host;
            }
            catch (Exception e) { _status = e.Message; }
            _menu.SetStatus(_status);
        }

        void StopNet()
        {
            try
            {
                _disc?.Stop();
                if (NetworkManager.Singleton != null &&
                    (NetworkManager.Singleton.IsHost || NetworkManager.Singleton.IsClient || NetworkManager.Singleton.IsServer))
                    NetworkManager.Singleton.Shutdown();
            }
            catch { }
            if (_mode != "local") _mode = "local";
            _netReady = NetworkManager.Singleton != null;
            _matchLive = false;
            _roomKey = "";
            _heroSent = false;
        }
    }
}
