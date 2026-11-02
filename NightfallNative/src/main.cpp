#include "NightfallCore.h"
#include "NightfallNet.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace Nightfall;

    Color HexToColor(const std::string& hex, Color fallback = BLACK)
    {
        uint8_t r = 0, g = 0, b = 0, a = 255;
        if (!GameSim::TryParseHexColor(hex, r, g, b, a)) return fallback;
        return Color{r, g, b, a};
    }

    // ───────────────────────────── Procedural Audio Bank (Sfx.cs 1:1) ─────────────────────────────

    class NativeSfxBank
    {
    public:
        void Init()
        {
            InitAudioDevice();
            _ready = IsAudioDeviceReady();
            if (!_ready) return;

            _sounds["jump"] = BuildTone(0.13f, 300.0f, 720.0f, WaveKind::Square, 0.16f, 0.0f);
            _sounds["attack"] = BuildNoise(0.10f, 0.20f, 0.55f);
            _sounds["magic"] = BuildTone(0.20f, 520.0f, 1180.0f, WaveKind::Sine, 0.18f, 14.0f);
            _sounds["hit"] = BuildTone(0.09f, 220.0f, 130.0f, WaveKind::Square, 0.18f, 0.0f);
            _sounds["stomp"] = BuildTone(0.12f, 150.0f, 70.0f, WaveKind::Square, 0.20f, 0.0f);
            _sounds["hurt"] = BuildTone(0.26f, 420.0f, 110.0f, WaveKind::Saw, 0.20f, 6.0f);
            _sounds["die"] = BuildTone(0.70f, 330.0f, 55.0f, WaveKind::Saw, 0.22f, 4.0f);
            _sounds["coin"] = BuildArp({880.0f, 1320.0f}, 0.07f, 0.15f, WaveKind::Square);
            _sounds["heart"] = BuildArp({660.0f, 880.0f, 1100.0f}, 0.07f, 0.15f, WaveKind::Sine);
            _sounds["boss"] = BuildTone(0.85f, 90.0f, 62.0f, WaveKind::Saw, 0.22f, 3.0f);
            _sounds["clear"] = BuildArp({523.0f, 659.0f, 784.0f, 1046.0f}, 0.12f, 0.16f, WaveKind::Square);
            _sounds["ui"] = BuildTone(0.06f, 620.0f, 900.0f, WaveKind::Square, 0.10f, 0.0f);
        }

        void Play(const std::string& id)
        {
            if (!_ready) return;
            auto it = _sounds.find(id);
            if (it != _sounds.end()) PlaySound(it->second);
        }

        void Shutdown()
        {
            if (!_ready) return;
            for (auto& kv : _sounds) UnloadSound(kv.second);
            _sounds.clear();
            CloseAudioDevice();
            _ready = false;
        }

    private:
        enum class WaveKind { Sine, Square, Saw };
        static constexpr int kRate = 22050;
        bool _ready = false;
        std::unordered_map<std::string, Sound> _sounds;

        static float Osc(WaveKind w, float phase)
        {
            phase -= std::floor(phase);
            switch (w)
            {
                case WaveKind::Square: return phase < 0.5f ? 1.0f : -1.0f;
                case WaveKind::Saw: return phase * 2.0f - 1.0f;
                default: return std::sin(phase * 6.2831853f);
            }
        }

        static Sound ToSound(const std::vector<float>& samples)
        {
            Wave w{};
            w.frameCount = static_cast<unsigned int>(samples.size());
            w.sampleRate = kRate;
            w.sampleSize = 16;
            w.channels = 1;
            short* pcm = static_cast<short*>(MemAlloc(static_cast<unsigned int>(samples.size() * sizeof(short))));
            for (size_t i = 0; i < samples.size(); ++i)
            {
                float v = std::max(-1.0f, std::min(1.0f, samples[i]));
                pcm[i] = static_cast<short>(v * 32767.0f);
            }
            w.data = pcm;
            Sound s = LoadSoundFromWave(w);
            UnloadWave(w);
            return s;
        }

        static Sound BuildTone(float dur, float f0, float f1, WaveKind w, float vol, float vibrato)
        {
            int n = std::max(1, static_cast<int>(std::lrint(dur * kRate)));
            std::vector<float> data(n);
            float phase = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                float t = i / static_cast<float>(n);
                float f = f0 + (f1 - f0) * t;
                if (vibrato > 0.0f) f += std::sin(t * dur * vibrato * 6.2831853f) * f * 0.05f;
                phase += f / kRate;
                float env = std::min(1.0f, t * 18.0f) * std::pow(1.0f - t, 1.6f);
                data[i] = Osc(w, phase) * env * vol;
            }
            return ToSound(data);
        }

        static Sound BuildNoise(float dur, float vol, float decay)
        {
            int n = std::max(1, static_cast<int>(std::lrint(dur * kRate)));
            std::vector<float> data(n);
            float lp = 0.0f;
            uint32_t rng = 0x9E3779B9u;
            for (int i = 0; i < n; ++i)
            {
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                float raw = (static_cast<float>(rng & 0xFFFFu) / 32768.0f) - 1.0f;
                lp = lp + (raw - lp) * 0.45f;
                float t = i / static_cast<float>(n);
                float env = std::min(1.0f, t * 30.0f) * std::pow(1.0f - t, 1.0f + decay * 4.0f);
                data[i] = lp * env * vol;
            }
            return ToSound(data);
        }

        static Sound BuildArp(std::initializer_list<float> notes, float each, float vol, WaveKind w)
        {
            int per = std::max(1, static_cast<int>(std::lrint(each * kRate)));
            std::vector<float> data(per * notes.size());
            int k = 0;
            for (float note : notes)
            {
                float phase = 0.0f;
                for (int i = 0; i < per; ++i)
                {
                    float t = i / static_cast<float>(per);
                    phase += note / kRate;
                    float env = std::min(1.0f, t * 20.0f) * std::pow(1.0f - t, 1.3f);
                    data[k * per + i] = Osc(w, phase) * env * vol;
                }
                ++k;
            }
            return ToSound(data);
        }
    };

    // ───────────────────────────── SpriteBank (StreamingAssets/art + PunchMagenta) ─────────────────────────────

    class NativeSpriteBank
    {
    public:
        void LoadAll()
        {
            std::vector<std::filesystem::path> roots = {
                "Content/art",
                "assets/art",
                "../NightfallUnity/Assets/StreamingAssets/art",
                "../../NightfallUnity/Assets/StreamingAssets/art",
                "NightfallUnity/Assets/StreamingAssets/art"
            };
            std::filesystem::path foundRoot;
            for (const auto& r : roots)
            {
                std::error_code ec;
                if (std::filesystem::exists(r, ec) && std::filesystem::is_directory(r, ec))
                {
                    foundRoot = r;
                    break;
                }
            }
            if (foundRoot.empty()) return;

            std::error_code ec;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(foundRoot, ec))
            {
                if (!entry.is_regular_file()) continue;
                auto p = entry.path();
                if (p.extension() != ".png") continue;
                std::string key = p.stem().string();
                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                Image img = LoadImage(p.string().c_str());
                if (img.data == nullptr) continue;
                ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                bool isBg = key.size() >= 3 && key.substr(key.size() - 3) == "_bg";
                if (!isBg)
                {
                    Color* pixels = static_cast<Color*>(img.data);
                    int count = img.width * img.height;
                    for (int i = 0; i < count; ++i)
                    {
                        Color c = pixels[i];
                        if (c.r > 180 && c.b > 180 && c.g < 160)
                        {
                            pixels[i] = Color{0, 0, 0, 0};
                        }
                    }
                }
                Texture2D tex = LoadTextureFromImage(img);
                SetTextureFilter(tex, isBg ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
                UnloadImage(img);
                _textures[key] = tex;
            }
        }

        const Texture2D* Get(const std::string& key) const
        {
            std::string k = key;
            std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            auto it = _textures.find(k);
            return it != _textures.end() ? &it->second : nullptr;
        }

        void UnloadAll()
        {
            for (auto& kv : _textures) UnloadTexture(kv.second);
            _textures.clear();
        }

    private:
        std::unordered_map<std::string, Texture2D> _textures;
    };

    enum class ScreenMode
    {
        Title,
        HeroSelect,
        WorldMap,
        LobbyLan,
        Playing
    };

    void DrawTexFitBottom(const Texture2D* tex, float x, float y, float w, float h, int facing, Color tint = WHITE)
    {
        if (!tex || tex->id == 0)
        {
            DrawRectangleRec(Rectangle{x, y, w, h}, tint);
            return;
        }
        Rectangle src{0.0f, 0.0f, static_cast<float>(tex->width * (facing < 0 ? -1 : 1)), static_cast<float>(tex->height)};
        Rectangle dst{x - w * 0.15f, y - h * 0.15f, w * 1.3f, h * 1.15f};
        DrawTexturePro(*tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, tint);
    }
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--headless-smoke")
        {
            GameSim sim;
            sim.ReadCtl = [](int) {
                Ctl c{};
                c.R = true;
                c.Jp = true;
                c.Jn = true;
                c.Ap = true;
                return c;
            };
            for (int w = 0; w < 16; ++w)
            {
                LevelData L = LevelBuilder::Compile(w);
                std::string why;
                if (!LevelBuilder::Walkable(L, why)) return 2;
            }
            sim.Start(0, "mike", "denyse", true);
            for (int f = 0; f < 300; ++f) sim.Tick(1.0f / 60.0f);
            std::cout << "NIGHTFALL_HEADLESS_SMOKE_OK worlds=16 frame=300\n";
            return 0;
        }
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(T::ViewW, T::ViewH, "Mike & Denyse: Nightfall (C++20 Native)");
    SetTargetFPS(60);

    Progress::Load();
    NativeSfxBank sfx;
    sfx.Init();
    NativeSpriteBank sprites;
    sprites.LoadAll();

    GameSim sim;
    sim.Sfx = [&sfx](const std::string& id) { sfx.Play(id); };

    Net::LanSession lan;
    ScreenMode screen = ScreenMode::Title;
    int selectedWorld = Progress::LastWorld;
    std::string heroP1 = Progress::Hero;
    std::string heroP2 = "denyse";
    bool twoPlayersLocal = false;
    bool paused = false;
    bool showTouchHud = false;
    float netSnapTimer = 0.0f;
    InputMsg latestRemoteCtl{};

    Ctl touchCtl{};

    sim.ReadCtl = [&](int slot) -> Ctl {
        if (slot == 0)
        {
            Ctl c = touchCtl;
            c.L = c.L || IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
            c.R = c.R || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
            c.D = c.D || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
            c.Jp = c.Jp || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_K);
            c.Jn = c.Jn || IsKeyDown(KEY_W) || IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_Z) || IsKeyDown(KEY_K);
            c.Ap = c.Ap || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_X) || IsKeyPressed(KEY_F) || IsKeyPressed(KEY_ENTER);
            if (IsGamepadAvailable(0))
            {
                float gx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                c.L = c.L || gx < -0.35f || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
                c.R = c.R || gx > 0.35f || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
                c.D = c.D || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
                c.Jp = c.Jp || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
                c.Jn = c.Jn || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
                c.Ap = c.Ap || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
            }
            return c;
        }
        if (lan.IsHosting())
        {
            Ctl c{};
            c.L = latestRemoteCtl.L;
            c.R = latestRemoteCtl.R;
            c.D = latestRemoteCtl.D;
            c.Jp = latestRemoteCtl.Jp;
            c.Jn = latestRemoteCtl.Jn;
            c.Ap = latestRemoteCtl.Ap;
            latestRemoteCtl.Jp = false;
            latestRemoteCtl.Ap = false;
            return c;
        }
        Ctl c{};
        c.L = IsKeyDown(KEY_LEFT);
        c.R = IsKeyDown(KEY_RIGHT);
        c.D = IsKeyDown(KEY_DOWN);
        c.Jp = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_KP_1);
        c.Jn = IsKeyDown(KEY_UP) || IsKeyDown(KEY_KP_1);
        c.Ap = IsKeyPressed(KEY_KP_2) || IsKeyPressed(KEY_RIGHT_CONTROL);
        return c;
    };

    RenderTexture2D target = LoadRenderTexture(T::ViewW, T::ViewH);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose())
    {
        float dt = std::min(GetFrameTime(), 1.0f / 20.0f);
        if (IsKeyPressed(KEY_F4)) showTouchHud = !showTouchHud;

        // Touch input mapping in viewport coordinates
        touchCtl = Ctl{};
        int tc = GetTouchPointCount();
        if (tc > 0) showTouchHud = true;
        if (showTouchHud && (tc > 0 || IsMouseButtonDown(MOUSE_LEFT_BUTTON)))
        {
            int points = std::max(tc, IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? 1 : 0);
            for (int i = 0; i < points; ++i)
            {
                Vector2 raw = tc > 0 ? GetTouchPosition(i) : GetMousePosition();
                float vx = raw.x * T::ViewW / std::max(1, GetScreenWidth());
                float vy = raw.y * T::ViewH / std::max(1, GetScreenHeight());
                if (vy > 380.0f)
                {
                    if (vx >= 20.0f && vx <= 95.0f) touchCtl.L = true;
                    if (vx >= 105.0f && vx <= 180.0f) touchCtl.R = true;
                    if (vx >= 740.0f && vx <= 835.0f) { touchCtl.Ap = true; }
                    if (vx >= 845.0f && vx <= 940.0f) { touchCtl.Jp = true; touchCtl.Jn = true; }
                }
            }
        }

        if (screen == ScreenMode::Title)
        {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J) || touchCtl.Jp || touchCtl.Ap)
            {
                sfx.Play("ui");
                screen = ScreenMode::HeroSelect;
            }
        }
        else if (screen == ScreenMode::HeroSelect)
        {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) { heroP1 = "mike"; heroP2 = "denyse"; sfx.Play("ui"); }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { heroP1 = "denyse"; heroP2 = "mike"; sfx.Play("ui"); }
            if (IsKeyPressed(KEY_TAB)) { twoPlayersLocal = !twoPlayersLocal; sfx.Play("ui"); }
            if (IsKeyPressed(KEY_L))
            {
                sfx.Play("ui");
                lan.StartClientListen();
                screen = ScreenMode::LobbyLan;
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J))
            {
                Progress::Hero = heroP1;
                Progress::Save();
                sfx.Play("ui");
                screen = ScreenMode::WorldMap;
            }
        }
        else if (screen == ScreenMode::LobbyLan)
        {
            lan.TickDiscovery(static_cast<float>(GetTime()));
            if (IsKeyPressed(KEY_H))
            {
                lan.StartHost("Nightfall Coop");
                sfx.Play("ui");
            }
            if (IsKeyPressed(KEY_C) && !lan.Rooms().empty())
            {
                const auto& r = lan.Rooms().front();
                lan.ConnectToHost(r.host, r.port, heroP1);
                sfx.Play("ui");
            }
            if (IsKeyPressed(KEY_ENTER) && lan.IsHosting())
            {
                sim.Start(selectedWorld, heroP1, heroP2, true);
                lan.SendStartMatch(selectedWorld, heroP1, heroP2);
                screen = ScreenMode::Playing;
            }
            FrameSnap incomingSnap;
            int startW = 0;
            std::string hHero, cHero;
            bool matchStarted = false;
            if (lan.IsConnectedClient())
            {
                lan.PollClientSnapshot(incomingSnap, startW, hHero, cHero, matchStarted);
                if (matchStarted)
                {
                    sim.Start(startW, hHero, cHero, true);
                    screen = ScreenMode::Playing;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE))
            {
                lan.Stop();
                screen = ScreenMode::HeroSelect;
            }
        }
        else if (screen == ScreenMode::WorldMap)
        {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
            {
                selectedWorld = std::max(0, selectedWorld - 1);
                sfx.Play("ui");
            }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
            {
                selectedWorld = std::min(15, selectedWorld + 1);
                sfx.Play("ui");
            }
            if (IsKeyPressed(KEY_U))
            {
                Progress::Unlocked = 16;
                Progress::Save();
                sfx.Play("clear");
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_J))
            {
                if (Progress::CanPlay(selectedWorld))
                {
                    sfx.Play("ui");
                    sim.Start(selectedWorld, heroP1, heroP2, twoPlayersLocal || lan.IsHosting());
                    paused = false;
                    screen = ScreenMode::Playing;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) screen = ScreenMode::HeroSelect;
        }
        else if (screen == ScreenMode::Playing)
        {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) paused = !paused;
            if (!paused)
            {
                if (lan.IsConnectedClient())
                {
                    Ctl localC = sim.ReadCtl(0);
                    InputMsg msg{localC.L, localC.R, localC.D, localC.Jp, localC.Jn, localC.Ap};
                    lan.SendClientInput(msg);
                    FrameSnap snap;
                    int sw = 0;
                    std::string hh, ch;
                    bool ms = false;
                    if (lan.PollClientSnapshot(snap, sw, hh, ch, ms))
                    {
                        sim.ApplySnapshot(snap);
                    }
                    sim.TickRemote(dt);
                }
                else
                {
                    if (lan.IsHosting())
                    {
                        lan.TickDiscovery(static_cast<float>(GetTime()));
                        std::string cHero;
                        InputMsg cInp{};
                        if (lan.PollHostInput(cInp, cHero))
                        {
                            sim.EnsureSecond(cHero);
                            latestRemoteCtl = cInp;
                        }
                    }
                    sim.Tick(dt);
                    if (lan.IsHosting())
                    {
                        netSnapTimer += dt;
                        if (netSnapTimer >= 0.05f)
                        {
                            netSnapTimer = 0.0f;
                            lan.BroadcastSnapshot(sim.CaptureSnapshot());
                            sim.Fx.clear();
                        }
                    }
                }

                if (sim.State == "clear" || sim.State == "win")
                {
                    Progress::OnClear(sim.WorldIndex);
                    Progress::RememberRun(sim.Souls, sim.Score, heroP1);
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    {
                        selectedWorld = std::min(15, sim.WorldIndex + 1);
                        screen = ScreenMode::WorldMap;
                    }
                }
                else if (sim.State == "dead")
                {
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    {
                        sim.Start(sim.WorldIndex, heroP1, heroP2, twoPlayersLocal || lan.IsHosting());
                    }
                }
            }
            else
            {
                if (IsKeyPressed(KEY_M)) screen = ScreenMode::WorldMap;
            }
        }

        // ───────────────────────────── Render to 960x540 Canvas ─────────────────────────────
        BeginTextureMode(target);
        if (screen == ScreenMode::Title)
        {
            ClearBackground(Color{12, 8, 18, 255});
            const Texture2D* bg = sprites.Get("castle_bg");
            if (bg) DrawTexturePro(*bg, Rectangle{0, 0, (float)bg->width, (float)bg->height}, Rectangle{0, 0, T::ViewW, T::ViewH}, Vector2{0, 0}, 0.0f, Color{180, 160, 200, 255});
            DrawRectangle(0, 0, T::ViewW, T::ViewH, Color{10, 6, 16, 150});
            DrawText("MIKE & DENYSE", 250, 120, 56, Color{230, 195, 110, 255});
            DrawText("NIGHTFALL", 335, 185, 48, Color{200, 70, 90, 255});
            DrawText("16 Noites · Coop Local & LAN Wi-Fi · C++20 Native", 230, 260, 20, LIGHTGRAY);
            DrawText("Pressione ENTER / ESPACO para iniciar", 280, 380, 22, YELLOW);
        }
        else if (screen == ScreenMode::HeroSelect)
        {
            ClearBackground(Color{16, 12, 24, 255});
            DrawText("ESCOLHA SEU CACADOR", 290, 45, 34, Color{230, 195, 110, 255});

            for (int i = 0; i < 2; ++i)
            {
                std::string id = i == 0 ? "mike" : "denyse";
                const auto* h = Catalog::Hero(id);
                bool sel = (heroP1 == id);
                Rectangle card{160.0f + i * 340.0f, 110.0f, 290.0f, 280.0f};
                DrawRectangleRec(card, sel ? Color{48, 34, 68, 255} : Color{26, 20, 36, 255});
                DrawRectangleLinesEx(card, sel ? 3.0f : 1.0f, sel ? GOLD : GRAY);
                const Texture2D* spr = sprites.Get(id + "_idle");
                DrawTexFitBottom(spr, card.x + 95.0f, card.y + 40.0f, 96.0f, 128.0f, 1);
                DrawText(h->Name.c_str(), (int)card.x + 24, (int)card.y + 185, 28, WHITE);
                DrawText(h->Title.c_str(), (int)card.x + 24, (int)card.y + 218, 18, LIGHTGRAY);
                DrawText(TextFormat("HP: %d   Alcance: %d", h->Hp, h->Reach), (int)card.x + 24, (int)card.y + 245, 16, SKYBLUE);
            }
            DrawText(TextFormat("[TAB] Modo Local: %s   |   [L] Sala Coop LAN Wi-Fi", twoPlayersLocal ? "2 JOGADORES (COOP)" : "1 JOGADOR (SOLO)"), 175, 425, 20, GOLD);
            DrawText("[A/D] Trocar Heroi   |   [ENTER] Ir ao Mapa das 16 Noites", 205, 465, 20, WHITE);
        }
        else if (screen == ScreenMode::LobbyLan)
        {
            ClearBackground(Color{14, 14, 24, 255});
            DrawText("SALA COOPERATIVA LAN WI-FI (UDP 47888 / 7777)", 160, 45, 28, GOLD);
            DrawText(TextFormat("IP Local Preferencial (LanIp.Pick): %s", LanIp::Pick().c_str()), 160, 95, 20, SKYBLUE);
            DrawText(lan.IsHosting() ? "Status: HOSPEDANDO SALA (Pressione ENTER para iniciar a partida)" : "Status: PROCURANDO SALAS NA REDE LOCAL...", 160, 135, 20, GREEN);
            DrawText("[H] Hospedar Sala   |   [C] Conectar na 1a Sala Encontrada   |   [ESC] Voltar", 160, 175, 18, LIGHTGRAY);

            int ry = 230;
            for (const auto& r : lan.Rooms())
            {
                DrawRectangle(160, ry, 640, 42, Color{32, 28, 48, 255});
                DrawText(TextFormat("%s — %s:%d", r.name.c_str(), r.host.c_str(), r.port), 175, ry + 11, 20, WHITE);
                ry += 50;
            }
        }
        else if (screen == ScreenMode::WorldMap)
        {
            ClearBackground(Color{12, 10, 20, 255});
            DrawText("MAPA DAS 16 NOITES", 315, 30, 32, GOLD);
            const auto& worlds = Catalog::Worlds();
            for (int i = 0; i < 16; ++i)
            {
                int col = i % 4;
                int row = i / 4;
                Rectangle box{80.0f + col * 205.0f, 85.0f + row * 95.0f, 190.0f, 80.0f};
                bool unlocked = Progress::CanPlay(i);
                bool sel = (i == selectedWorld);
                DrawRectangleRec(box, sel ? Color{60, 42, 82, 255} : (unlocked ? Color{28, 24, 42, 255} : Color{18, 16, 22, 255}));
                DrawRectangleLinesEx(box, sel ? 3.0f : 1.0f, sel ? GOLD : (unlocked ? LIGHTGRAY : DARKGRAY));
                DrawText(TextFormat("Noite %d", i + 1), (int)box.x + 10, (int)box.y + 10, 18, unlocked ? GOLD : GRAY);
                DrawText(worlds[i].Name.c_str(), (int)box.x + 10, (int)box.y + 36, 14, unlocked ? WHITE : DARKGRAY);
            }
            DrawText("[Setas/A/D] Escolher Noite  |  [ENTER] Jogar  |  [U] Desbloquear Todas  |  [ESC] Voltar", 95, 485, 18, LIGHTGRAY);
        }
        else if (screen == ScreenMode::Playing && sim.HasLevel)
        {
            Color skyColor = HexToColor(sim.Level.World.Tone, Color{14, 16, 24, 255});
            Color fogColor = HexToColor(sim.Level.World.Fog, Color{28, 32, 48, 255});
            Color groundColor = HexToColor(sim.Level.World.Ground, Color{56, 48, 40, 255});
            Color lipColor = HexToColor(sim.Level.World.Lip, Color{110, 130, 70, 255});
            ClearBackground(skyColor);

            const Texture2D* bg = sprites.Get(sim.Level.World.Id + "_bg");
            if (bg)
            {
                DrawTexturePro(*bg, Rectangle{0, 0, (float)bg->width, (float)bg->height}, Rectangle{0, 0, T::ViewW, T::ViewH}, Vector2{0, 0}, 0.0f, Color{200, 200, 200, 190});
            }
            else
            {
                DrawRectangleGradientV(0, 0, T::ViewW, T::ViewH, skyColor, fogColor);
            }

            float camX = sim.CamX + (sim.Shake > 0.1f ? std::sin(sim.Time * 60.0f) * sim.Shake * 0.4f : 0.0f);
            float camY = sim.CamY;

            // Tiles
            int x0 = std::max(0, static_cast<int>(camX / T::Tile) - 1);
            int x1 = std::min(sim.Level.Cols - 1, static_cast<int>((camX + T::ViewW) / T::Tile) + 2);
            for (int y = 0; y < sim.Level.Rows; ++y)
            {
                for (int x = x0; x <= x1; ++x)
                {
                    int t = sim.Level.Tiles[y][x];
                    if (t == T::Empty) continue;
                    float sx = x * T::Tile - camX;
                    float sy = y * T::Tile - camY;
                    if (t == T::Solid || t == T::Break)
                    {
                        DrawRectangle((int)sx, (int)sy, T::Tile, T::Tile, groundColor);
                        if (y > 0 && sim.Level.Tiles[y - 1][x] == T::Empty)
                            DrawRectangle((int)sx, (int)sy, T::Tile, 6, lipColor);
                    }
                    else if (t == T::Platform)
                    {
                        DrawRectangle((int)sx, (int)sy, T::Tile, 10, lipColor);
                    }
                    else if (t == T::Ice)
                    {
                        DrawRectangle((int)sx, (int)sy, T::Tile, T::Tile, Color{150, 215, 240, 255});
                    }
                    else if (t == T::Bounce)
                    {
                        DrawRectangle((int)sx, (int)sy, T::Tile, 12, Color{240, 190, 70, 255});
                    }
                    else if (t == T::Ladder)
                    {
                        DrawRectangle((int)sx + 10, (int)sy, 4, T::Tile, Color{160, 115, 65, 255});
                        DrawRectangle((int)sx + 26, (int)sy, 4, T::Tile, Color{160, 115, 65, 255});
                        for (int ry = 4; ry < T::Tile; ry += 10)
                            DrawRectangle((int)sx + 10, (int)sy + ry, 20, 3, Color{190, 145, 85, 255});
                    }
                    else if (t == T::Lava)
                    {
                        DrawRectangle((int)sx, (int)sy + 6, T::Tile, T::Tile - 6, Color{230, 70, 20, 240});
                    }
                    else if (t == T::Water)
                    {
                        DrawRectangle((int)sx, (int)sy + 6, T::Tile, T::Tile - 6, Color{40, 110, 180, 190});
                    }
                    else if (t == T::Spike || t == T::Thorn)
                    {
                        Color hc = t == T::Thorn ? Color{90, 150, 60, 255} : Color{200, 200, 215, 255};
                        for (int k = 0; k < 4; ++k)
                        {
                            float bx = sx + k * 10.0f;
                            DrawTriangle(Vector2{bx + 5.0f, sy + 10.0f}, Vector2{bx, sy + T::Tile}, Vector2{bx + 10.0f, sy + T::Tile}, hc);
                        }
                    }
                }
            }

            // Golden Exit Bell
            DrawCircle((int)(sim.Level.Exit.x + 16 - camX), (int)(sim.Level.Exit.y + 16 - camY), 16.0f, GOLD);

            // Items
            for (const auto& it : sim.Items)
            {
                if (it.Taken) continue;
                float bob = std::sin(it.Bob * 3.0f) * 3.0f;
                const Texture2D* tex = sprites.Get(it.Type);
                DrawTexFitBottom(tex, it.X - camX, it.Y + bob - camY, 22.0f, 22.0f, 1, it.Type == "heart" ? RED : SKYBLUE);
            }

            // Enemies
            for (const auto& e : sim.Enemies)
            {
                if (e.Dead) continue;
                const Texture2D* tex = sprites.Get(e.Id);
                Color tint = e.Flash > 0.0f ? RED : WHITE;
                DrawTexFitBottom(tex, e.X - camX, e.Y - camY, e.W, e.H, e.Facing, tint);
            }

            // Bosses
            for (const auto& b : sim.Bosses)
            {
                if (b.Dead) continue;
                const Texture2D* tex = sprites.Get(b.Id);
                Color tint = b.Flash > 0.0f ? RED : WHITE;
                DrawTexFitBottom(tex, b.X - camX, b.Y - camY, b.W, b.H, b.Facing, tint);
            }

            // Players
            for (const auto& p : sim.Players)
            {
                if (p.Dead) continue;
                std::string key = p.Hero.Id + "_idle";
                if (p.Anim == "attack") key = p.Hero.Id + "_attack";
                else if (p.Anim == "jump" || p.Anim == "fall") key = p.Hero.Id + "_jump";
                else if (p.Anim == "walk") key = p.Hero.Id + ((static_cast<int>(p.AnimT * 8.0f) % 2 == 0) ? "_walk1" : "_walk2");
                const Texture2D* tex = sprites.Get(key);
                Color tint = (p.Inv > 0.0f && (static_cast<int>(p.Inv * 20.0f) % 2 == 0)) ? Color{255, 255, 255, 140} : WHITE;
                DrawTexFitBottom(tex, p.X - camX, p.Y - camY, p.W, p.H, p.Facing, tint);
            }

            // Projectiles
            for (const auto& pr : sim.Projectiles)
            {
                Color pc = pr.Friendly ? SKYBLUE : ORANGE;
                DrawCircle((int)(pr.X + pr.W * 0.5f - camX), (int)(pr.Y + pr.H * 0.5f - camY), pr.W * 0.5f, pc);
            }

            // HUD
            DrawRectangle(12, 10, 310, 58, Color{0, 0, 0, 150});
            if (!sim.Players.empty())
            {
                const auto& p0 = sim.Players[0];
                DrawText(TextFormat("%s  HP: %d/%d  Vidas: %d", p0.Hero.Name.c_str(), p0.Hp, p0.MaxHp, p0.Lives), 22, 18, 18, WHITE);
                DrawText(TextFormat("Almas: %d   Pontos: %d   Noite %d/16", sim.Souls, sim.Score, sim.WorldIndex + 1), 22, 42, 16, GOLD);
            }

            for (const auto& b : sim.Bosses)
            {
                if (b.Dead) continue;
                DrawRectangle(280, 490, 400, 26, Color{20, 10, 15, 220});
                float ratio = std::max(0.0f, std::min(1.0f, b.Hp / static_cast<float>( std::max(1, b.MaxHp))));
                DrawRectangle(282, 492, static_cast<int>(396 * ratio), 22, Color{210, 45, 65, 255});
                DrawText(b.Def.Name.c_str(), 290, 495, 18, WHITE);
            }

            if (sim.MessageT > 0.0f && !sim.Message.empty())
            {
                DrawRectangle(220, 95, 520, 42, Color{0, 0, 0, 180});
                DrawText(sim.Message.c_str(), 240, 106, 22, GOLD);
            }

            if (showTouchHud)
            {
                DrawRectangleRounded(Rectangle{20, 420, 75, 75}, 0.25f, 6, Color{255, 255, 255, 70});
                DrawText("<", 48, 442, 32, WHITE);
                DrawRectangleRounded(Rectangle{105, 420, 75, 75}, 0.25f, 6, Color{255, 255, 255, 70});
                DrawText(">", 133, 442, 32, WHITE);
                DrawRectangleRounded(Rectangle{740, 420, 95, 75}, 0.25f, 6, Color{220, 80, 80, 90});
                DrawText("ATK", 762, 446, 24, WHITE);
                DrawRectangleRounded(Rectangle{845, 420, 95, 75}, 0.25f, 6, Color{80, 160, 230, 90});
                DrawText("PULO", 862, 446, 22, WHITE);
            }

            if (sim.State == "clear" || sim.State == "win")
            {
                DrawRectangle(0, 0, T::ViewW, T::ViewH, Color{0, 0, 0, 180});
                DrawText(sim.State == "win" ? "BELIAL DERROTADO! VITORIA FINAL!" : "NOITE CONCLUIDA!", 230, 210, 34, GOLD);
                DrawText("Pressione ENTER para avancar no Mapa", 275, 270, 22, WHITE);
            }
            else if (sim.State == "dead")
            {
                DrawRectangle(0, 0, T::ViewW, T::ViewH, Color{30, 0, 0, 190});
                DrawText("VOCE CAIU NA ESCURIDAO", 265, 210, 34, RED);
                DrawText("Pressione ENTER para tentar novamente", 270, 270, 22, WHITE);
            }
            else if (paused)
            {
                DrawRectangle(0, 0, T::ViewW, T::ViewH, Color{0, 0, 0, 170});
                DrawText("PAUSADO ([ESC] Continuar  |  [M] Mapa)", 260, 250, 24, WHITE);
            }
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            target.texture,
            Rectangle{0.0f, 0.0f, static_cast<float>(target.texture.width), -static_cast<float>(target.texture.height)},
            Rectangle{0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE
        );
        EndDrawing();
    }

    UnloadRenderTexture(target);
    sprites.UnloadAll();
    sfx.Shutdown();
    CloseWindow();
    return 0;
}
