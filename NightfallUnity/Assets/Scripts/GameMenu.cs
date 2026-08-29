using System;
using System.Collections.Generic;
using Nightfall.Net;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace Nightfall
{
    public class GameMenu : MonoBehaviour
    {
        public string ScreenId = "title";
        public Ctl Touch;
        public bool P2Join;

        NightApp _app;
        Canvas _canvas;
        readonly Dictionary<string, GameObject> _panels = new();
        Text _status, _error, _banner, _p2Status, _mapLabel, _clearTitle, _clearBody, _deadBody, _winBody, _mapHint;
        readonly List<Button> _roomBtns = new();
        Transform _roomRoot;
        Text _roomEmpty;
        readonly Button[] _worldBtns = new Button[16];
        readonly Image[] _worldChips = new Image[16];
        readonly Text[] _worldLocks = new Text[16];
        Image _mikeImg, _denyseImg;
        GameObject _mikeSel, _denyseSel, _bgRoot;
        GameObject _touchRoot;
        Font _font;

        static readonly Color Gold = Hex("d4b46a");
        static readonly Color Paper = Hex("f0e2c8");
        static readonly Color Mute = Hex("9d8896");
        static readonly Color Story = Hex("cbbdb4");
        static readonly Color Blood = Hex("c33a42");

        public void Bind(NightApp app)
        {
            _app = app;
            ArtGen.Ensure();
            _font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (_font == null) _font = Font.CreateDynamicFontFromOSFont("Arial", 16);
            Build();
            SetScreen("title");
        }

        public void SetSprites(SpriteBank bank)
        {
            if (_mikeImg != null) _mikeImg.sprite = bank.Spr("mike_idle", "chars/mike_idle");
            if (_denyseImg != null) _denyseImg.sprite = bank.Spr("denyse_idle", "chars/denyse_idle");
            if (_mikeImg != null) _mikeImg.enabled = _mikeImg.sprite != null;
            if (_denyseImg != null) _denyseImg.enabled = _denyseImg.sprite != null;
        }

        public void SetStatus(string s) { if (_status) _status.text = s ?? ""; }

        public void SetError(string s)
        {
            bool on = !string.IsNullOrEmpty(s);
            if (_error) { _error.text = s ?? ""; _error.gameObject.SetActive(on); }
            if (_banner) { _banner.text = s ?? ""; _banner.gameObject.SetActive(on && ScreenId != "play"); }
        }

        public void SetP2(string s) { if (_p2Status) _p2Status.text = s ?? ""; }

        public void SetScreen(string id)
        {
            ScreenId = id;
            foreach (var kv in _panels) if (kv.Value) kv.Value.SetActive(kv.Key == id);
            if (_touchRoot) _touchRoot.SetActive(id == "play");
            if (_bgRoot) _bgRoot.SetActive(id is "title" or "howto" or "select" or "net" or "map");
            if (_banner) _banner.gameObject.SetActive(!string.IsNullOrEmpty(_banner.text) && id != "play");
        }

        public void SetDeadCopy(bool client)
        {
            if (_deadBody) _deadBody.text = client
                ? "As trevas tomaram o campo. O host decide se voltam a tentar."
                : "As trevas tomaram o campo. Voltem do último santuário e tentem outra vez.";
        }

        public void SetClearCopy(bool client, int world)
        {
            if (_clearTitle) _clearTitle.text = "Noite " + (world + 1) + " vencida";
            if (_clearBody) _clearBody.text = client
                ? "O sino calou. Aguarde — o host abre a próxima noite."
                : "O sino calou. A próxima noite já se move.";
        }

        public void RefreshMap(int world, int unlocked = 1)
        {
            unlocked = Mathf.Clamp(unlocked, 1, 16);
            world = Mathf.Clamp(world, 0, 15);
            bool open = world < unlocked;
            if (_mapLabel)
                _mapLabel.text = open
                    ? "Noite " + (world + 1) + " · " + Catalog.Worlds[world].Name
                    : "Bloqueada — vença a noite " + unlocked + " para abrir esta";
            if (_mapHint)
                _mapHint.text = open
                    ? "toque de novo na fase ou use COMEÇAR A FASE"
                    : "as noites abrem em ordem";

            for (int i = 0; i < _worldBtns.Length; i++)
            {
                if (_worldBtns[i] == null) continue;
                bool can = i < unlocked;
                bool sel = i == world && can;
                var lab = _worldBtns[i].GetComponentInChildren<Text>();
                if (lab)
                {
                    lab.text = can ? (i + 1) + ".  " + Catalog.Worlds[i].Name : (i + 1) + ".  — — —";
                    lab.color = can ? (sel ? Paper : Story) : new Color(0.42f, 0.36f, 0.40f);
                }
                if (_worldChips[i])
                {
                    var c = Hex(Catalog.Worlds[i].Lip.TrimStart('#'));
                    _worldChips[i].color = can ? new Color(c.r, c.g, c.b, sel ? 1f : 0.75f) : new Color(0.3f, 0.26f, 0.3f, 0.6f);
                }
                if (_worldLocks[i]) _worldLocks[i].gameObject.SetActive(!can);
                var img = _worldBtns[i].GetComponent<Image>();
                if (img) img.color = sel ? new Color(1f, 0.86f, 0.66f, 1f)
                    : can ? new Color(0.85f, 0.80f, 0.78f, 0.85f)
                    : new Color(0.45f, 0.42f, 0.46f, 0.55f);
                _worldBtns[i].interactable = true;
            }
        }

        public void SetSelectCopy(bool coop)
        {
            if (_p2Status) _p2Status.text = coop
                ? "No outro celular o segundo caçador entra na sala."
                : "Um jogador · escolha Mike ou Denyse";
        }

        public void RefreshHero(string p1)
        {
            if (_mikeSel) _mikeSel.SetActive(p1 == "mike");
            if (_denyseSel) _denyseSel.SetActive(p1 == "denyse");
        }

        public void RefreshRooms(IReadOnlyList<RoomInfo> rooms)
        {
            if (_roomRoot == null) return;
            foreach (var b in _roomBtns) if (b) Destroy(b.gameObject);
            _roomBtns.Clear();
            int n = rooms?.Count ?? 0;
            if (_roomEmpty) _roomEmpty.gameObject.SetActive(n == 0);
            if (rooms == null) return;
            for (int i = 0; i < n && i < 4; i++)
            {
                var r = rooms[i];
                var btn = Btn(_roomRoot, r.name + "   ·   " + r.host, 0, 62 - i * 48, 660, 42, null, true);
                var copy = r;
                btn.onClick.AddListener(() => _app.UiJoin(copy));
                _roomBtns.Add(btn);
            }
        }

        void Update()
        {
            if (ScreenId != "play") Touch = new Ctl();
        }

        // ───────────────────────────── build ─────────────────────────────

        void Build()
        {
            if (FindAnyObjectByType<EventSystem>() == null)
            {
                var es = new GameObject("EventSystem");
                es.AddComponent<EventSystem>();
                es.AddComponent<StandaloneInputModule>();
                DontDestroyOnLoad(es);
            }

            var root = new GameObject("MenuCanvas");
            DontDestroyOnLoad(root);
            _canvas = root.AddComponent<Canvas>();
            _canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            _canvas.sortingOrder = 100;
            var scaler = root.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(960, 540);
            scaler.screenMatchMode = CanvasScaler.ScreenMatchMode.MatchWidthOrHeight;
            scaler.matchWidthOrHeight = 0.5f;
            root.AddComponent<GraphicRaycaster>();

            _bgRoot = Backdrop(root.transform);

            _panels["title"] = TitlePanel(root.transform);
            _panels["howto"] = HowToPanel(root.transform);
            _panels["select"] = SelectPanel(root.transform);
            _panels["net"] = NetPanel(root.transform);
            _panels["map"] = MapPanel(root.transform);
            _panels["pause"] = PausePanel(root.transform);
            _panels["clear"] = ClearPanel(root.transform);
            _panels["dead"] = DeadPanel(root.transform);
            _panels["win"] = WinPanel(root.transform);
            _panels["play"] = PlayHud(root.transform);

            _banner = Label(root.transform, "", 0, -252, 920, 26, 12, new Color(1f, 0.45f, 0.4f), true);
            _banner.gameObject.SetActive(false);
        }

        /// <summary>Shared painted backdrop so every menu sits inside the same night.</summary>
        GameObject Backdrop(Transform parent)
        {
            var go = new GameObject("backdrop", typeof(RectTransform));
            go.transform.SetParent(parent, false);
            Stretch(go.GetComponent<RectTransform>());

            var artGo = new GameObject("art", typeof(RectTransform), typeof(RawImage));
            artGo.transform.SetParent(go.transform, false);
            Stretch(artGo.GetComponent<RectTransform>());
            var raw = artGo.GetComponent<RawImage>();
            raw.texture = Resources.Load<Texture2D>("Art/forest_bg");
            raw.color = new Color(0.75f, 0.72f, 0.85f, 0.30f);

            var tintGo = new GameObject("tint", typeof(RectTransform), typeof(Image));
            tintGo.transform.SetParent(go.transform, false);
            Stretch(tintGo.GetComponent<RectTransform>());
            tintGo.GetComponent<Image>().color = new Color(0.03f, 0.01f, 0.03f, 0.62f);

            var vigGo = new GameObject("vignette", typeof(RectTransform), typeof(RawImage));
            vigGo.transform.SetParent(go.transform, false);
            Stretch(vigGo.GetComponent<RectTransform>());
            var vr = vigGo.GetComponent<RawImage>();
            vr.texture = ArtGen.Vignette();
            vr.color = new Color(1, 1, 1, 0.9f);
            vr.raycastTarget = false;
            return go;
        }

        GameObject TitlePanel(Transform parent)
        {
            var p = Panel(parent, "title", 900, 500, false);
            Label(p.transform, "UM PLATFORMER COOPERATIVO DAS TREVAS", 0, 196, 860, 22, 12, Mute, true);
            Rule(p.transform, 0, 176, 520);
            var big = Label(p.transform, "MIKE & DENYSE", 0, 132, 900, 66, 46, Gold, true);
            big.fontStyle = FontStyle.Bold;
            Label(p.transform, "N I G H T F A L L", 0, 86, 860, 34, 22, Paper, true);
            Rule(p.transform, 0, 62, 520);
            Label(p.transform,
                "A lua de sangue rasgou o véu. Dois caçadores — a lâmina e o cristal —\nsão a última linha entre o vilarejo e o Senhor dos Pesadelos.",
                0, 22, 820, 60, 14, Story, true);

            Btn(p.transform, "Um jogador", -259, -76, 196, 50, () => _app.UiLocal());
            Btn(p.transform, "Dois celulares", -44, -76, 206, 50, () => _app.UiGotoNetFromTitle(), true);
            Btn(p.transform, "Controles", 148, -76, 150, 50, () => _app.UiShow("howto"), true);
            Btn(p.transform, "Sair", 297, -76, 120, 50, () => _app.UiQuit(), true);

            Rule(p.transform, 0, -122, 700);
            Label(p.transform, "16 noites · 32 monstros · 16 chefes · Belial · coop em dois celulares",
                0, -148, 860, 22, 12, Mute, true);
            _error = Label(p.transform, "", 0, -186, 860, 34, 12, new Color(1f, 0.45f, 0.4f), true);
            _error.gameObject.SetActive(false);
            return p;
        }

        GameObject HowToPanel(Transform parent)
        {
            var p = Panel(parent, "howto", 900, 500, true);
            Head(p.transform, "Controles");
            Label(p.transform,
                "TOQUE  ◀ ▶ andar  ·  ▼ descer escada  ·  botão grande PULO  ·  lâmina GOLPE  ·  II pausa\n" +
                "TECLADO  A/D andar  ·  W, Espaço ou J pular  ·  K ou X atacar  ·  Esc pausa\n" +
                "O botão VOLTAR do celular pausa a partida e abre o menu.\n \n" +
                "UM JOGADOR  as noites abrem em ordem; vencer uma libera a seguinte.\n" +
                "DOIS CELULARES  mesmo Wi-Fi: um cria a sala, o outro procura e entra.\n" +
                "Só o host escolhe a fase — a primeira, ou uma já vencida para repetir.\n \n" +
                "Pise nos monstros ou golpeie. Mike corta de perto, Denyse dispara cristal.\n" +
                "Derrotem o chefe, toquem o sino dourado. No fim da estrada, Belial.",
                0, 6, 840, 290, 14, Story, true);
            Btn(p.transform, "Voltar", 0, -206, 200, 46, () => _app.UiShow("title"), true);
            return p;
        }

        GameObject SelectPanel(Transform parent)
        {
            var p = Panel(parent, "select", 900, 500, true);
            Head(p.transform, "Escolha o caçador");

            var mike = Card(p.transform, -196, 16, 300, 286);
            _mikeImg = Portrait(mike.transform);
            Label(mike.transform, "MIKE", 0, -58, 270, 28, 20, Gold, true).fontStyle = FontStyle.Bold;
            Label(mike.transform, "Caçador da Lâmina", 0, -84, 270, 20, 12, Mute, true);
            Label(mike.transform, "6 vidas · golpe curto e pesado", 0, -106, 270, 20, 11, Story, true);
            _mikeSel = Label(mike.transform, "● ESCOLHIDO", 0, -130, 270, 20, 12, Gold, true).gameObject;
            mike.GetComponent<Button>().onClick.AddListener(() => _app.UiPickHero("mike"));

            var den = Card(p.transform, 196, 16, 300, 286);
            _denyseImg = Portrait(den.transform);
            Label(den.transform, "DENYSE", 0, -58, 270, 28, 20, Gold, true).fontStyle = FontStyle.Bold;
            Label(den.transform, "Bruxa do Cristal", 0, -84, 270, 20, 12, Mute, true);
            Label(den.transform, "5 vidas · cristal à distância, queda lenta", 0, -106, 270, 20, 11, Story, true);
            _denyseSel = Label(den.transform, "● ESCOLHIDA", 0, -130, 270, 20, 12, Gold, true).gameObject;
            den.GetComponent<Button>().onClick.AddListener(() => _app.UiPickHero("denyse"));

            _p2Status = Label(p.transform, "Um jogador · escolha Mike ou Denyse", 0, -164, 760, 22, 13, Story, true);
            Btn(p.transform, "Entrar na noite", -92, -208, 250, 48, () => _app.UiConfirmHero());
            Btn(p.transform, "Voltar", 148, -208, 160, 48, () => _app.UiShow("title"), true);
            return p;
        }

        GameObject NetPanel(Transform parent)
        {
            var p = Panel(parent, "net", 900, 500, true);
            Head(p.transform, "Dois celulares");
            Label(p.transform, "Mesmo Wi-Fi  ·  um cria a sala  ·  o outro procura e entra", 0, 168, 860, 20, 12, Mute, true);
            _status = Label(p.transform, "Os dois no mesmo Wi-Fi. Um cria, o outro procura e entra.", 0, 136, 840, 34, 13, Story, true);
            Btn(p.transform, "Criar sala", -150, 88, 230, 46, () => _app.UiHostLan());
            Btn(p.transform, "Procurar sala", 150, 88, 230, 46, () => _app.UiScan(), true);

            var list = new GameObject("rooms", typeof(RectTransform));
            list.transform.SetParent(p.transform, false);
            var rt = list.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = new Vector2(0, -46);
            rt.sizeDelta = new Vector2(720, 200);
            _roomRoot = list.transform;
            _roomEmpty = Label(list.transform, "nenhuma sala encontrada ainda…", 0, 62, 660, 42, 13, Mute, true);

            Btn(p.transform, "Continuar", -122, -206, 230, 46, () => _app.UiNetGo());
            Btn(p.transform, "Voltar", 128, -206, 160, 46, () => _app.UiNetBack(), true);
            return p;
        }

        GameObject MapPanel(Transform parent)
        {
            var p = Panel(parent, "map", 920, 512, true);
            Head(p.transform, "O caminho maldito");
            for (int i = 0; i < 16; i++)
            {
                int idx = i;
                int col = i % 4, row = i / 4;
                float x = -336 + col * 224;
                float y = 142 - row * 52;
                var b = Btn(p.transform, "", x, y, 212, 46, () => _app.UiPickWorld(idx), true);

                var chip = new GameObject("chip", typeof(RectTransform), typeof(Image));
                chip.transform.SetParent(b.transform, false);
                var crt = chip.GetComponent<RectTransform>();
                crt.anchorMin = crt.anchorMax = new Vector2(0.5f, 0.5f);
                crt.anchoredPosition = new Vector2(-92, 0);
                crt.sizeDelta = new Vector2(8, 28);
                _worldChips[i] = chip.GetComponent<Image>();
                _worldChips[i].raycastTarget = false;

                var lab = Label(b.transform, (i + 1) + ".  " + Catalog.Worlds[i].Name, 8, 0, 176, 40, 12, Story, false);
                lab.raycastTarget = false;
                lab.resizeTextForBestFit = true;
                lab.resizeTextMinSize = 9;
                lab.resizeTextMaxSize = 13;

                _worldLocks[i] = Label(b.transform, "×", 92, 0, 24, 24, 14, Mute, true);
                _worldLocks[i].raycastTarget = false;

                _worldBtns[i] = b;
            }
            Rule(p.transform, 0, -62, 720);
            _mapLabel = Label(p.transform, "Noite 1 · Floresta Amaldiçoada", 0, -88, 880, 24, 15, Paper, true);
            _mapHint = Label(p.transform, "toque de novo na fase ou use COMEÇAR A FASE", 0, -110, 880, 20, 11, Mute, true);
            Btn(p.transform, "Começar a fase", -100, -172, 290, 52, () => _app.UiStartWorld());
            Btn(p.transform, "Menu", 160, -172, 150, 52, () => _app.UiShow("title"), true);
            return p;
        }

        GameObject PausePanel(Transform parent)
        {
            var p = Panel(parent, "pause", 620, 330, true);
            Label(p.transform, "PAUSA", 0, 106, 560, 42, 30, Gold, true).fontStyle = FontStyle.Bold;
            Rule(p.transform, 0, 80, 380);
            Label(p.transform, "A noite espera. Respire.", 0, 52, 560, 24, 13, Story, true);
            Btn(p.transform, "Continuar", 0, 4, 320, 50, () => _app.UiResume());
            Btn(p.transform, "Voltar ao mapa", 0, -54, 320, 46, () => _app.UiAbandon(), true);
            Btn(p.transform, "Menu inicial", 0, -110, 320, 46, () => _app.UiToTitle(), true);
            return p;
        }

        GameObject ClearPanel(Transform parent)
        {
            var p = Panel(parent, "clear", 700, 340, true);
            _clearTitle = Label(p.transform, "Noite vencida", 0, 108, 640, 42, 26, Gold, true);
            _clearTitle.fontStyle = FontStyle.Bold;
            Rule(p.transform, 0, 82, 420);
            _clearBody = Label(p.transform, "O sino calou. A próxima noite já se move.", 0, 44, 620, 44, 14, Story, true);
            Btn(p.transform, "Próxima fase", -92, -30, 250, 50, () => _app.UiNext());
            Btn(p.transform, "Mapa", 138, -30, 160, 50, () => _app.UiShow("map"), true);
            Btn(p.transform, "Menu inicial", 0, -96, 250, 42, () => _app.UiToTitle(), true);
            return p;
        }

        GameObject DeadPanel(Transform parent)
        {
            var p = Panel(parent, "dead", 700, 340, true);
            var t = Label(p.transform, "OS CAÇADORES CAÍRAM", 0, 108, 640, 42, 24, Blood, true);
            t.fontStyle = FontStyle.Bold;
            Rule(p.transform, 0, 82, 420);
            _deadBody = Label(p.transform, "As trevas tomaram o campo. Voltem do último santuário e tentem outra vez.",
                0, 44, 620, 46, 14, Story, true);
            Btn(p.transform, "Renascem", -92, -30, 250, 50, () => _app.UiRetry());
            Btn(p.transform, "Mapa", 138, -30, 160, 50, () => _app.UiShow("map"), true);
            Btn(p.transform, "Menu inicial", 0, -96, 250, 42, () => _app.UiToTitle(), true);
            return p;
        }

        GameObject WinPanel(Transform parent)
        {
            var p = Panel(parent, "win", 820, 420, true);
            var t = Label(p.transform, "AURORA", 0, 136, 780, 56, 42, Gold, true);
            t.fontStyle = FontStyle.Bold;
            Rule(p.transform, 0, 104, 500);
            Label(p.transform, "Belial foi selado", 0, 74, 780, 32, 21, Paper, true);
            _winBody = Label(p.transform,
                "Mike e Denyse atravessaram dezesseis noites. O trono racha, a lua empalidece,\ne o vilarejo respira pela primeira vez em muito tempo.",
                0, 16, 760, 70, 14, Story, true);
            Btn(p.transform, "Título", -80, -84, 220, 48, () => _app.UiShow("title"));
            Btn(p.transform, "Mapa", 140, -84, 160, 48, () => _app.UiShow("map"), true);
            return p;
        }

        GameObject PlayHud(Transform parent)
        {
            var p = new GameObject("play", typeof(RectTransform));
            p.transform.SetParent(parent, false);
            Stretch(p.GetComponent<RectTransform>());
            _touchRoot = p;
            return p;
        }

        // ───────────────────────────── widgets ─────────────────────────────

        GameObject Panel(Transform parent, string name, float w, float h, bool dim)
        {
            var go = new GameObject(name, typeof(RectTransform));
            go.transform.SetParent(parent, false);
            Stretch(go.GetComponent<RectTransform>());

            if (dim)
            {
                var shade = new GameObject("shade", typeof(RectTransform), typeof(Image));
                shade.transform.SetParent(go.transform, false);
                Stretch(shade.GetComponent<RectTransform>());
                shade.GetComponent<Image>().color = new Color(0.02f, 0.01f, 0.02f, 0.55f);
            }

            var card = new GameObject("card", typeof(RectTransform), typeof(Image));
            card.transform.SetParent(go.transform, false);
            var rt = card.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.sizeDelta = new Vector2(w, h);
            var img = card.GetComponent<Image>();
            img.sprite = ArtGen.PanelSpr;
            img.type = Image.Type.Sliced;
            img.color = Color.white;

            var inner = new GameObject("inner", typeof(RectTransform));
            inner.transform.SetParent(card.transform, false);
            Stretch(inner.GetComponent<RectTransform>());
            go.name = name;
            // children go on the card so layout stays centred
            return Wrap(go, inner.transform);
        }

        // keeps Panel() returning the root object while children attach to the card
        GameObject Wrap(GameObject root, Transform content)
        {
            var holder = root.AddComponent<PanelContent>();
            holder.Content = content;
            return root;
        }

        Transform Body(Transform t)
        {
            var pc = t.GetComponent<PanelContent>();
            return pc != null && pc.Content != null ? pc.Content : t;
        }

        void Head(Transform parent, string text)
        {
            var l = Label(parent, text, 0, 200, 800, 40, 24, Gold, true);
            l.fontStyle = FontStyle.Bold;
            Rule(parent, 0, 178, 460);
        }

        void Rule(Transform parent, float x, float y, float w)
        {
            var go = new GameObject("rule", typeof(RectTransform), typeof(Image));
            go.transform.SetParent(Body(parent), false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = new Vector2(x, y);
            rt.sizeDelta = new Vector2(w, 2);
            var img = go.GetComponent<Image>();
            img.color = new Color(Gold.r, Gold.g, Gold.b, 0.38f);
            img.raycastTarget = false;
        }

        GameObject Card(Transform parent, float x, float y, float w, float h)
        {
            var go = new GameObject("card", typeof(RectTransform), typeof(Image), typeof(Button));
            go.transform.SetParent(Body(parent), false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = new Vector2(x, y);
            rt.sizeDelta = new Vector2(w, h);
            var img = go.GetComponent<Image>();
            img.sprite = ArtGen.PanelSoftSpr;
            img.type = Image.Type.Sliced;
            img.color = new Color(0.92f, 0.88f, 0.9f, 1f);
            StyleBtn(go.GetComponent<Button>(), true);
            return go;
        }

        Image Portrait(Transform parent)
        {
            var go = new GameObject("art", typeof(RectTransform), typeof(Image));
            go.transform.SetParent(parent, false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = new Vector2(0, 52);
            rt.sizeDelta = new Vector2(174, 174);
            var img = go.GetComponent<Image>();
            img.preserveAspect = true;
            img.color = Color.white;
            img.raycastTarget = false;
            return img;
        }

        Text Label(Transform parent, string text, float x, float y, float w, float h, int size, Color col, bool center)
        {
            var go = new GameObject("lab", typeof(RectTransform), typeof(Text));
            go.transform.SetParent(Body(parent), false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = new Vector2(x, y);
            rt.sizeDelta = new Vector2(w, h);
            var t = go.GetComponent<Text>();
            t.font = _font;
            t.text = text;
            t.fontSize = size;
            t.color = col;
            t.alignment = center ? TextAnchor.MiddleCenter : TextAnchor.MiddleLeft;
            t.horizontalOverflow = HorizontalWrapMode.Wrap;
            t.verticalOverflow = VerticalWrapMode.Overflow;
            t.lineSpacing = 1.15f;
            t.raycastTarget = false;
            var sh = go.AddComponent<Shadow>();
            sh.effectColor = new Color(0, 0, 0, 0.75f);
            sh.effectDistance = new Vector2(1, -1);
            return t;
        }

        Button Btn(Transform parent, string text, float x, float y, float w, float h, Action click, bool ghost = false)
        {
            var go = new GameObject("btn", typeof(RectTransform), typeof(Image), typeof(Button));
            go.transform.SetParent(Body(parent), false);
            var rt = go.GetComponent<RectTransform>();
            rt.anchorMin = rt.anchorMax = new Vector2(0.5f, 0.5f);
            rt.anchoredPosition = new Vector2(x, y);
            rt.sizeDelta = new Vector2(w, h);
            var img = go.GetComponent<Image>();
            img.sprite = ghost ? ArtGen.ButtonGhostSpr : ArtGen.ButtonSpr;
            img.type = Image.Type.Sliced;
            img.color = Color.white;
            var b = go.GetComponent<Button>();
            StyleBtn(b, ghost);
            if (!string.IsNullOrEmpty(text))
            {
                var lab = Label(go.transform, text.ToUpperInvariant(), 0, 0, w - 20, h, 13, ghost ? Story : Paper, true);
                lab.resizeTextForBestFit = true;
                lab.resizeTextMinSize = 9;
                lab.resizeTextMaxSize = 14;
            }
            if (click != null) b.onClick.AddListener(() => click());
            return b;
        }

        static void StyleBtn(Button b, bool ghost)
        {
            var c = b.colors;
            c.normalColor = Color.white;
            c.highlightedColor = new Color(1f, 0.95f, 0.85f);
            c.pressedColor = new Color(1f, 0.72f, 0.55f);
            c.selectedColor = Color.white;
            c.disabledColor = new Color(0.5f, 0.5f, 0.5f, 0.5f);
            c.fadeDuration = 0.06f;
            b.colors = c;
            b.transition = Selectable.Transition.ColorTint;
        }

        static void Stretch(RectTransform rt)
        {
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = rt.offsetMax = Vector2.zero;
        }

        static Color Hex(string h)
        {
            if (string.IsNullOrEmpty(h)) return Color.gray;
            ColorUtility.TryParseHtmlString("#" + h.TrimStart('#'), out var c);
            return c;
        }
    }

    /// <summary>Marks where a panel's children should be parented (inside the framed card).</summary>
    public class PanelContent : MonoBehaviour { public Transform Content; }

    public class HoldPad : MonoBehaviour
    {
        public Action<bool> Set;
        public bool Held;
    }
}
