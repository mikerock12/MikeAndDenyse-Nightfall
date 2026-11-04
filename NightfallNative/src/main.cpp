#include "NightfallCore.h"
#include "NightfallNet.h"
#include "NativeWorldArt.h"
#include "raylib.h"
#include "GamePreferences.h"
#include "MobileServices.h"
#if defined(PLATFORM_ANDROID)
extern "C" JNIEXPORT void JNICALL Java_com_mikeanddenyse_nightfall_MainActivity_nativeSetPaused(JNIEnv*,jclass,jboolean paused){SLTMobile::SetBackground(paused);}
#endif
#include "rlgl.h"
#if defined(_WIN32)
#include "WindowsBranding.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cstdlib>
#ifdef PLATFORM_ANDROID
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
extern "C" struct android_app* GetAndroidApp(void);
#endif

namespace
{
    using namespace Nightfall;
    SLTPrefs::Preferences* Preferences=nullptr;

#ifdef PLATFORM_ANDROID
    unsigned char* LoadAndroidAsset(const char* name,int* size)
    {
        *size=0;
        AAssetManager* manager=GetAndroidApp()->activity->assetManager;
        AAsset* asset=AAssetManager_open(manager,name,AASSET_MODE_STREAMING);
        if(!asset) {
            TraceLog(LOG_ERROR,"NIGHTFALL_ASSET: missing %s",name);
            AAssetDir* dir=AAssetManager_openDir(manager,"art");
            if(dir) {
                const char* entry;
                while((entry=AAssetDir_getNextFileName(dir))) TraceLog(LOG_ERROR,"NIGHTFALL_ASSET_DIR: art/%s",entry);
                AAssetDir_close(dir);
            }
            return nullptr;
        }
        off64_t length=AAsset_getLength64(asset);
        if(length<=0 || length>0x7fffffff) { AAsset_close(asset);return nullptr; }
        auto data=static_cast<unsigned char*>(MemAlloc(static_cast<unsigned int>(length)));
        int read=0;
        while(data && read<length) {
            int count=AAsset_read(asset,data+read,static_cast<size_t>(length-read));
            if(count<=0)break;
            read+=count;
        }
        AAsset_close(asset);
        if(read!=length) { if(data)MemFree(data);return nullptr; }
        *size=read;return data;
    }
    char* LoadAndroidAssetText(const char* name)
    {
        int size=0;auto data=LoadAndroidAsset(name,&size);
        if(!data)return nullptr;
        auto text=static_cast<char*>(MemAlloc(static_cast<unsigned int>(size+1)));
        if(text) { std::memcpy(text,data,size);text[size]='\0'; }
        MemFree(data);return text;
    }
#endif

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
            _sounds["hit"] = Mix(ToneSamples(0.09f,220,130,WaveKind::Square,0.16f,0),NoiseSamples(0.07f,0.14f,0.3f));
            _sounds["stomp"] = Mix(ToneSamples(0.12f,150,70,WaveKind::Square,0.18f,0),NoiseSamples(0.09f,0.16f,0.2f));
            _sounds["hurt"] = BuildTone(0.26f, 420.0f, 110.0f, WaveKind::Saw, 0.20f, 6.0f);
            _sounds["die"] = BuildTone(0.70f, 330.0f, 55.0f, WaveKind::Saw, 0.22f, 4.0f);
            _sounds["coin"] = BuildArp({880.0f, 1320.0f}, 0.07f, 0.15f, WaveKind::Square);
            _sounds["heart"] = BuildArp({660.0f, 880.0f, 1100.0f}, 0.07f, 0.15f, WaveKind::Sine);
            _sounds["boss"] = Mix(ToneSamples(0.85f,90,62,WaveKind::Saw,0.22f,3),NoiseSamples(0.5f,0.10f,0.12f));
            _sounds["clear"] = BuildArp({523.0f, 659.0f, 784.0f, 1046.0f}, 0.12f, 0.16f, WaveKind::Square);
            _sounds["ui"] = BuildTone(0.06f, 620.0f, 900.0f, WaveKind::Square, 0.10f, 0.0f);
        }

        void Play(const std::string& id)
        {
            if (!_ready) return;
            if(Preferences && Preferences->value.vibration && (id=="hurt"||id=="stomp"))SLTMobile::Call("vibrateCombat");
            double now=GetTime();
            if(_lastPlayed.count(id) && now-_lastPlayed[id]<0.035) return;
            _lastPlayed[id]=now;
            auto it = _sounds.find(id);
            if (it != _sounds.end()) {
                SetSoundPitch(it->second,0.94f+GetRandomValue(0,120)/1000.0f);
                SetSoundVolume(it->second,Preferences?Preferences->value.effects:.85f);
                PlaySound(it->second);
            }
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
        std::unordered_map<std::string, double> _lastPlayed;

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

        static Sound BuildTone(float dur,float f0,float f1,WaveKind w,float vol,float vibrato) { return ToSound(ToneSamples(dur,f0,f1,w,vol,vibrato)); }
        static Sound BuildNoise(float dur,float vol,float decay) { return ToSound(NoiseSamples(dur,vol,decay)); }
        static Sound Mix(const std::vector<float>& a,const std::vector<float>& b) {
            std::vector<float> mixed(std::max(a.size(),b.size()));
            for(size_t i=0;i<mixed.size();++i) mixed[i]=std::clamp((i<a.size()?a[i]:0)+(i<b.size()?b[i]:0),-1.0f,1.0f);
            return ToSound(mixed);
        }
        static std::vector<float> ToneSamples(float dur, float f0, float f1, WaveKind w, float vol, float vibrato)
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
            return data;
        }

        static std::vector<float> NoiseSamples(float dur, float vol, float decay)
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
            return data;
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
        size_t AssetCount() const { return _paths.size(); }
        void LoadAll()
        {
#ifdef PLATFORM_ANDROID
            char* index = LoadFileText("art/index.txt");
            if (index) {
                std::istringstream lines(index);
                std::string rel;
                while (std::getline(lines, rel)) {
                    if (!rel.empty() && rel.back() == '\r') rel.pop_back();
                    if (!rel.empty()) _paths[std::filesystem::path(rel).stem().string()] = "art/" + rel;
                }
                UnloadFileText(index);
            }
#else
            std::vector<std::filesystem::path> roots = {
                std::filesystem::path(GetApplicationDirectory()) / "Content/art",
                "Content/art", "assets/art", "../NightfallUnity/Assets/StreamingAssets/art",
                "../../NightfallUnity/Assets/StreamingAssets/art", "NightfallUnity/Assets/StreamingAssets/art"
            };
            for (const auto& root : roots) {
                std::error_code ec;
                if (!std::filesystem::is_directory(root, ec)) continue;
                for (const auto& e : std::filesystem::recursive_directory_iterator(root, ec))
                    if (e.is_regular_file() && e.path().extension() == ".png")
                        _paths[e.path().stem().string()] = e.path().string();
                break;
            }
#endif
            TraceLog(LOG_INFO, "NIGHTFALL: indexed %d original sprite assets", static_cast<int>(_paths.size()));
        }

        const Texture2D* Get(const std::string& key)
        {
            auto loaded = _textures.find(key);
            if (loaded != _textures.end()) return &loaded->second;
            auto path = _paths.find(key);
            if (path == _paths.end()) return nullptr;
            Image img = LoadImage(path->second.c_str());
            if (!img.data) return nullptr;
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            bool bg = key.size() >= 3 && key.substr(key.size() - 3) == "_bg";
            if (!bg) {
                Color* pixels = static_cast<Color*>(img.data);
                for (int i = 0; i < img.width * img.height; ++i)
                    if (pixels[i].r > 180 && pixels[i].b > 180 && pixels[i].g < 160)
                        pixels[i] = Color{0,0,0,0};
            }
            // Decode original artwork lazily, cap GPU working set to the displayed resolution.
            int limit = bg ? 1024 : 512;
            if (img.width > limit || img.height > limit) {
                float scale = limit / static_cast<float>(std::max(img.width, img.height));
                ImageResizeNN(&img, std::max(1, static_cast<int>(img.width * scale)), std::max(1, static_cast<int>(img.height * scale)));
            }
            Texture2D texture = LoadTextureFromImage(img);
            UnloadImage(img);
            SetTextureFilter(texture, bg ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
            _textures[key] = texture;
            return &_textures[key];
        }

        void UnloadAll()
        {
            for (auto& kv : _textures) UnloadTexture(kv.second);
            _textures.clear();
        }

    private:
        std::unordered_map<std::string, Texture2D> _textures;
        std::unordered_map<std::string, std::string> _paths;
    };

    enum class ScreenMode
    {
        Title,
        HeroSelect,
        WorldMap,
        LobbyLan,
        Playing,
        Help
    };

    Rectangle Viewport()
    {
        float scale = std::min(GetScreenWidth() / 960.0f, GetScreenHeight() / 540.0f);
        if(Preferences&&Preferences->value.integerScale&&scale>=1)scale=std::floor(scale);
        return {(GetScreenWidth() - 960.0f * scale) / 2, (GetScreenHeight() - 540.0f * scale) / 2, 960.0f * scale, 540.0f * scale};
    }

    Vector2 CanvasPoint(Vector2 point, Rectangle viewport)
    {
        return {(point.x - viewport.x) * 960.0f / viewport.width, (point.y - viewport.y) * 540.0f / viewport.height};
    }

    void Button(Rectangle rect, const char* label, bool enabled = true)
    {
        DrawRectangleRounded(rect, 0.18f, 5, enabled ? Color{48,32,54,245} : Color{25,22,30,230});
        DrawRectangleLinesEx(rect, 2, enabled ? GOLD : GRAY);
        int size = 20;
        DrawText(label, static_cast<int>(rect.x + (rect.width - MeasureText(label, size)) / 2), static_cast<int>(rect.y + (rect.height-size)/2), size, enabled ? RAYWHITE : GRAY);
    }

    void DrawTexFitBottom(const Texture2D* tex, float x, float y, float w, float h, int facing, Color tint = WHITE)
    {
        if (!tex || tex->id == 0)
        {
            DrawRectangleRec(Rectangle{x, y, w, h}, tint);
            return;
        }
        Rectangle src{0.0f, 0.0f, static_cast<float>(tex->width * (facing < 0 ? -1 : 1)), static_cast<float>(tex->height)};
        float scale=std::min(w/tex->width,h/tex->height);
        float drawW=tex->width*scale,drawH=tex->height*scale;
        Rectangle dst{x+(w-drawW)/2,y+h-drawH,drawW,drawH};
        DrawTexturePro(*tex, src, dst, Vector2{0.0f, 0.0f}, 0.0f, tint);
    }
}

int main(int argc, char** argv)
{
    std::string renderSmokeDirectory;
    std::string settingsSmokeDirectory;
    for (int i = 1; i < argc; ++i)
    {
        if(std::string(argv[i])=="--settings-smoke" && i+1<argc)settingsSmokeDirectory=argv[++i];
        if(std::string(argv[i])=="--render-smoke" && i+1<argc) renderSmokeDirectory=argv[++i];
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

    SetConfigFlags(renderSmokeDirectory.empty() && settingsSmokeDirectory.empty() ? FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT : FLAG_WINDOW_HIDDEN);
#ifdef PLATFORM_ANDROID
    // Use the actual Android surface size. raylib 5.5's fixed logical window
    // path applies its own framebuffer scale in addition to our 16:9 target.
    InitWindow(0, 0, "Mike & Denyse: Nightfall");
#else
    InitWindow(T::ViewW, T::ViewH, "Mike & Denyse: Nightfall");
#endif
#if defined(_WIN32)
    const auto iconBytes = SLTBranding::ApplicationIconPng();
    Image appIcon = LoadImageFromMemory(".png", iconBytes.data(), static_cast<int>(iconBytes.size()));
    if (appIcon.data) { SetWindowIcon(appIcon); UnloadImage(appIcon); TraceLog(LOG_INFO, "NIGHTFALL_APP_ICON: embedded icon applied"); }
    else TraceLog(LOG_ERROR, "NIGHTFALL_APP_ICON: embedded icon missing or invalid");
#endif
    std::filesystem::path settingsPath;
#ifdef PLATFORM_ANDROID
    settingsPath=std::filesystem::path(GetAndroidApp()->activity->internalDataPath)/"nightfall_preferences.cfg";
#else
    const char* prefsData=std::getenv("LOCALAPPDATA");settingsPath=std::filesystem::path(prefsData?prefsData:GetApplicationDirectory())/"SmellsLikeTech/Nightfall/preferences.cfg";
#endif
    SLTPrefs::Preferences settings(settingsPath,false);Preferences=&settings;
    if(!renderSmokeDirectory.empty()){settings.Defaults();settings.value.quality=0;}
    settings.ApplyWindow(!renderSmokeDirectory.empty()||!settingsSmokeDirectory.empty());
    if(!renderSmokeDirectory.empty())SetTargetFPS(0);

    SetExitKey(KEY_NULL);
#ifdef PLATFORM_ANDROID
    Progress::SetStoragePath(std::string(GetAndroidApp()->activity->internalDataPath) + "/nightfall_progress.cfg");
#else
    const char* localData = std::getenv("LOCALAPPDATA");
    if (localData) {
        std::filesystem::path saveDir = std::filesystem::path(localData) / "SmellsLikeTech/Nightfall";
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);
        if (!ec) {
            auto destination=saveDir / "nightfall_progress.cfg";
            auto legacy=std::filesystem::path(GetApplicationDirectory()) / "nightfall_progress.cfg";
            if(!std::filesystem::exists(destination,ec) && std::filesystem::exists(legacy,ec))
                std::filesystem::copy_file(legacy,destination,std::filesystem::copy_options::none,ec);
            Progress::SetStoragePath(destination.string());
        }
    }
#endif
    Progress::Load();
    NativeSfxBank sfx;
    if(renderSmokeDirectory.empty()) sfx.Init();
    NativeWorldArt worldArt;
    NativeSpriteBank sprites;
#ifdef PLATFORM_ANDROID
    // Read compressed APK entries directly; avoid libc/funopen filename fallback.
    SetLoadFileDataCallback(LoadAndroidAsset);
    SetLoadFileTextCallback(LoadAndroidAssetText);
#endif
    sprites.LoadAll();
    if(!renderSmokeDirectory.empty() && sprites.AssetCount()!=77) {
        TraceLog(LOG_ERROR,"Render smoke requires all 77 original assets"); CloseWindow(); return 3;
    }

    GameSim sim;
    sim.Sfx = [&sfx](const std::string& id) { sfx.Play(id); };

    Net::LanSession lan;
    ScreenMode screen = ScreenMode::Title;
    int selectedWorld = Progress::LastWorld;
    std::string heroP1 = Progress::Hero;
    std::string heroP2 = "denyse";
    bool twoPlayersLocal = false;
    bool paused = false;
#ifdef PLATFORM_ANDROID
    bool showTouchHud = true;
#else
    bool showTouchHud = false;
#endif
    bool wantLan = false;
    bool quit = false;
    bool previousJump = false, previousAttack = false;
    int previousTouches = 0;
    bool resultSaved = false;
    struct VisualFx { FxEvent event; float age; };
    std::vector<VisualFx> visualFx;
    std::vector<FxSnap> pendingNetworkFx;
    float netSnapTimer = 0.0f;
    InputMsg latestRemoteCtl{};

    Ctl touchCtl{};

    sim.ReadCtl = [&](int slot) -> Ctl {
        if (slot == 0)
        {
            Ctl c = touchCtl;
            c.L = c.L || settings.Down(0,0);
            c.R = c.R || settings.Down(0,1);
            c.D = c.D || settings.Down(0,2);
            c.Jp = c.Jp || settings.Pressed(0,3);
            c.Jn = c.Jn || settings.Down(0,3);
            c.Ap = c.Ap || settings.Pressed(0,4);
            if (IsGamepadAvailable(0))
            {
                float gx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                c.L = c.L || gx < -settings.value.deadzone || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
                c.R = c.R || gx > settings.value.deadzone || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
                c.D = c.D || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
                c.Jp = c.Jp || IsGamepadButtonPressed(0, settings.value.buttons[8]);
                c.Jn = c.Jn || IsGamepadButtonDown(0, settings.value.buttons[8]);
                c.Ap = c.Ap || IsGamepadButtonPressed(0, settings.value.buttons[9]);
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
        c.L = settings.Down(1,0);
        c.R = settings.Down(1,1);
        c.D = settings.Down(1,2);
        c.Jp = settings.Pressed(1,3);
        c.Jn = settings.Down(1,3);
        c.Ap = settings.Pressed(1,4);
        return c;
    };

    RenderTexture2D target = LoadRenderTexture(T::ViewW*settings.RenderScale(), T::ViewH*settings.RenderScale());
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    auto ReadPhysicalCtl=sim.ReadCtl;
    std::array<Ctl,2> tickControls{}, pendingControls{};
    sim.ReadCtl=[&](int slot){return tickControls[std::clamp(slot,0,1)];};
    int renderSmokeFrame=0;
    int settingsSmokeFrame=0;
    if(!settingsSmokeDirectory.empty())std::filesystem::create_directories(settingsSmokeDirectory);
    if(!renderSmokeDirectory.empty()) {
        std::filesystem::create_directories(renderSmokeDirectory);
        sim.ReadCtl=[](int){return Ctl{};};
        screen=ScreenMode::Playing;sim.Start(0,"mike","denyse",true);
    }
    ScreenMode loggedScreen=ScreenMode::Title;
    double simulationAccumulator=0;
    bool loggedPause=false;
    TraceLog(LOG_INFO,"NIGHTFALL_SCREEN: 0");

    while (!quit && !WindowShouldClose())
    {
        float dt = renderSmokeDirectory.empty()?std::min(GetFrameTime(), 1.0f / 20.0f):1.0f/60;
        Rectangle viewport = Viewport();
        if(!settingsSmokeDirectory.empty())settings.Show(settingsSmokeFrame/2);
        const bool settingsWasOpen=settings.open;
        settings.Update();
        if(settings.changed) {
            if(settings.windowChanged)settings.ApplyWindow(!renderSmokeDirectory.empty()||!settingsSmokeDirectory.empty());
            if(target.texture.width!=T::ViewW*settings.RenderScale()){UnloadRenderTexture(target);target=LoadRenderTexture(T::ViewW*settings.RenderScale(),T::ViewH*settings.RenderScale());SetTextureFilter(target.texture,TEXTURE_FILTER_POINT);}
        }
        if(IsAudioDeviceReady())SetMasterVolume(SLTMobile::Background?0:settings.value.master);
        showTouchHud=settings.value.touch;
        if(settings.open&&screen==ScreenMode::Playing)paused=true;
        int tc = GetTouchPointCount();
        Vector2 pointer = CanvasPoint(tc > 0 ? GetTouchPosition(0) : GetMousePosition(), viewport);
        bool clicked = !settingsWasOpen && !settings.open && (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || (tc > 0 && previousTouches == 0));
        previousTouches = tc;
        auto click = [&](Rectangle rect) { return clicked && CheckCollisionPointRec(pointer, rect); };
        bool confirm = !settingsWasOpen && !settings.open && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        bool back = !settingsWasOpen && !settings.open && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACK) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) || click({820, 12, 124, 44}));
        if(screen!=ScreenMode::Playing && click({680,480,260,44}))settings.Show();
        if(screen==ScreenMode::Playing && paused && click({680,145,260,44}))settings.Show();
        auto leaveMatch = [&](ScreenMode dest) {
            lan.Stop(); latestRemoteCtl = {}; touchCtl = {}; paused = false; screen = dest;
        };
        if (renderSmokeDirectory.empty() && (SLTPrefs::Mobile||settings.value.autoPause) && !IsWindowFocused() && screen == ScreenMode::Playing) paused = true;
        if (!SLTPrefs::Mobile && !settingsWasOpen && IsKeyPressed(KEY_F4)) {settings.value.touch=!settings.value.touch;settings.Save();}

        // All held fingers contribute; jump/attack edges fire once, never every rendered frame.
        touchCtl = Ctl{};
        if (SLTPrefs::Mobile)showTouchHud=true;
        if (screen == ScreenMode::Playing && !paused && !settingsWasOpen && !settings.open && sim.State == "play") {
            int points = tc > 0 ? tc : (IsMouseButtonDown(MOUSE_LEFT_BUTTON) ? 1 : 0);
            for (int i = 0; i < points; ++i) {
                Vector2 point = settings.ControlInput(CanvasPoint(tc > 0 ? GetTouchPosition(i) : GetMousePosition(), viewport),960,540);
                if (CheckCollisionPointCircle(point, {80,430},58*settings.value.touchSize)) touchCtl.L = true;
                if (CheckCollisionPointCircle(point, {204,430},58*settings.value.touchSize)) touchCtl.R = true;
                if (CheckCollisionPointCircle(point, {131,315},47*settings.value.touchSize)) touchCtl.D = true;
                if (CheckCollisionPointCircle(point, {690,452},56*settings.value.touchSize)) touchCtl.Ap = true;
                if (CheckCollisionPointCircle(point, {862,422},66*settings.value.touchSize)) touchCtl.Jn = true;
            }
        }
        bool heldAttack = touchCtl.Ap;
        touchCtl.Ap = heldAttack && !previousAttack;
        touchCtl.Jp = touchCtl.Jn && !previousJump;
        previousAttack = heldAttack; previousJump = touchCtl.Jn;

        for(int slot=0;slot<2;slot++) {
            Ctl now=ReadPhysicalCtl(slot);
            now.Jp|=pendingControls[slot].Jp;now.Ap|=pendingControls[slot].Ap;pendingControls[slot]=now;
        }
        if(paused||settings.open||settingsWasOpen||screen!=ScreenMode::Playing){pendingControls={};simulationAccumulator=0;}
        if(!settings.open && !settingsWasOpen) {
        if (screen == ScreenMode::Title) {
            if (confirm || click({300,300,360,52})) { wantLan = false; screen = ScreenMode::HeroSelect; sfx.Play("ui"); }
            else if (click({300,362,360,52})) { wantLan = true; screen = ScreenMode::HeroSelect; sfx.Play("ui"); }
            else if (click({300,424,170,48})) screen = ScreenMode::Help;
            else if (back || click({490,424,170,48})) quit = true;
        }
        else if (screen == ScreenMode::Help) {
            if (back || confirm) screen = ScreenMode::Title;
        }
        else if (screen == ScreenMode::HeroSelect) {
            if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT) || click({160,110,290,280})) heroP1 = "mike";
            if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT) || click({500,110,290,280})) heroP1 = "denyse";
            heroP2 = heroP1 == "mike" ? "denyse" : "mike";
#ifndef PLATFORM_ANDROID
            if (IsKeyPressed(KEY_TAB)) twoPlayersLocal = !twoPlayersLocal;
#endif
            if (confirm || click({300,452,360,58}) || IsKeyPressed(KEY_L)) {
                Progress::Hero = heroP1; Progress::Save(); sfx.Play("ui");
                if (wantLan || IsKeyPressed(KEY_L)) { lan.StartClientListen(); screen = ScreenMode::LobbyLan; }
                else screen = ScreenMode::WorldMap;
            }
            if (back) screen = ScreenMode::Title;
        }
        else if (screen == ScreenMode::LobbyLan) {
            lan.TickDiscovery(static_cast<float>(GetTime()));
            if (IsKeyPressed(KEY_H) || click({100,150,280,54})) { lan.StartHost("Nightfall Coop"); sfx.Play("ui"); }
            std::string clientHero; InputMsg lobbyInput{};
            if (lan.IsHosting()) lan.PollHostInput(lobbyInput, clientHero);
            if (IsKeyPressed(KEY_C) && !lan.Rooms().empty()) {
                const auto& room = lan.Rooms().front(); lan.ConnectToHost(room.host, room.port, heroP1);
            }
            for (size_t i = 0; i < lan.Rooms().size() && i < 4; ++i)
                if (click({100,230.0f + 52*i,760,44})) {
                    const auto room = lan.Rooms()[i]; lan.ConnectToHost(room.host, room.port, heroP1); break;
                }
            if ((confirm || click({500,150,360,54})) && lan.IsHosting()) screen = ScreenMode::WorldMap;
            FrameSnap incoming; int world=0; std::string hostHero, guestHero; bool started=false;
            if (lan.IsConnectedClient()) {
                lan.SendClientInput({});
                lan.PollClientSnapshot(incoming, world, hostHero, guestHero, started);
                if (started) { sim.Start(world,hostHero,guestHero,true); resultSaved=false; paused=false; screen=ScreenMode::Playing; }
            }
            if (back) { lan.Stop(); screen=ScreenMode::HeroSelect; }
        }
        else if (screen == ScreenMode::WorldMap) {
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) selectedWorld=std::max(0,selectedWorld-1);
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) selectedWorld=std::min(15,selectedWorld+1);
            bool start = confirm || click({300,474,360,54});
            for(int i=0;i<16;++i) if(click({80.0f+(i%4)*205,85.0f+(i/4)*95,190,80})) {
                if (i == selectedWorld && Progress::CanPlay(i)) start = true;
                selectedWorld=i;
            }
            if (lan.IsHosting()) { lan.TickDiscovery(static_cast<float>(GetTime())); std::string h; InputMsg c{}; lan.PollHostInput(c,h); }
            if (start && Progress::CanPlay(selectedWorld)) {
                sim.Start(selectedWorld,heroP1,heroP2,twoPlayersLocal||lan.IsHosting());
                if(lan.IsHosting()) lan.SendStartMatch(selectedWorld,heroP1,heroP2);
                resultSaved=false; paused=false; screen=ScreenMode::Playing; sfx.Play("ui");
            }
            if(back) { lan.Stop(); screen=ScreenMode::HeroSelect; }
        }
        else if (screen == ScreenMode::Playing) {
            if(back || IsKeyPressed(KEY_P) || click({16,10,78,46})) paused=!paused;
            bool terminal=sim.State=="clear" || sim.State=="win" || sim.State=="dead";
            if ((paused || terminal) && !settings.open && !settingsWasOpen) {
                bool owner=!lan.IsConnectedClient();
                if ((confirm || click({290,285,380,54})) && (paused || owner)) {
                    if(paused) paused=false;
                    else if(sim.State=="dead") { sim.Start(sim.WorldIndex,heroP1,heroP2,twoPlayersLocal||lan.IsHosting()); if(lan.IsHosting()) lan.SendStartMatch(sim.WorldIndex,heroP1,heroP2); resultSaved=false; }
                    else { selectedWorld=std::min(15,sim.WorldIndex+1); screen=ScreenMode::WorldMap; }
                }
                else if(IsKeyPressed(KEY_M) || click({290,350,380,54})) leaveMatch(ScreenMode::WorldMap);
                else if(click({290,415,380,54})) leaveMatch(ScreenMode::Title);
            }
            if(!paused && screen==ScreenMode::Playing) {
                if(lan.IsConnectedClient()) {
                    Ctl c=pendingControls[0];pendingControls[0].Jp=pendingControls[0].Ap=false; lan.SendClientInput({c.L,c.R,c.D,c.Jp,c.Jn,c.Ap});
                    FrameSnap snap; int world=sim.WorldIndex; std::string h,g; bool started=false;
                    bool got=lan.PollClientSnapshot(snap,world,h,g,started);
                    if(!lan.IsConnectedClient()) { leaveMatch(ScreenMode::HeroSelect); }
                    if(started) { sim.Start(world,h,g,true); resultSaved=false; }
                    if(got) sim.ApplySnapshot(snap);
                    sim.TickRemote(dt);
                } else {
                    if(lan.IsHosting()) {
                        lan.TickDiscovery(static_cast<float>(GetTime())); InputMsg input{}; std::string hero;
                        if(lan.PollHostInput(input,hero)) { sim.EnsureSecond(hero); latestRemoteCtl=input; }
                    }
                    simulationAccumulator+=dt;
                    while(simulationAccumulator+1e-8>=1.0/60.0){
                        if(lan.IsHosting()) {pendingControls[1].L=latestRemoteCtl.L;pendingControls[1].R=latestRemoteCtl.R;pendingControls[1].D=latestRemoteCtl.D;pendingControls[1].Jn=latestRemoteCtl.Jn;pendingControls[1].Jp|=latestRemoteCtl.Jp;pendingControls[1].Ap|=latestRemoteCtl.Ap;latestRemoteCtl.Jp=latestRemoteCtl.Ap=false;}
                        tickControls=pendingControls;sim.Tick(1.f/60.f);simulationAccumulator-=1.0/60.0;
                        for(auto& c:pendingControls)c.Jp=c.Ap=false;
                    }
                }
                if((sim.State=="clear" || sim.State=="win") && !resultSaved && !lan.IsConnectedClient()) {
                    Progress::OnClear(sim.WorldIndex); Progress::RememberRun(sim.Souls,sim.Score,heroP1); resultSaved=true;
                }
            }
        }

        }
        if(screen!=loggedScreen) { TraceLog(LOG_INFO,"NIGHTFALL_SCREEN: %d",static_cast<int>(screen));loggedScreen=screen; }
        if(paused!=loggedPause) { TraceLog(LOG_INFO,"NIGHTFALL_PAUSE: %d",paused?1:0);loggedPause=paused; }
        for (auto& fx : visualFx) fx.age += dt;
        visualFx.erase(std::remove_if(visualFx.begin(),visualFx.end(),[](const VisualFx& fx){return fx.age>0.4f;}),visualFx.end());
        for(const auto& event:sim.Fx) {
            if(visualFx.size()<160) visualFx.push_back({event,0});
            if(lan.IsHosting() && pendingNetworkFx.size()<FrameSnap::MaxFx)
                pendingNetworkFx.push_back({event.X,event.Y,event.Kind,static_cast<uint8_t>(event.Dir<0 ? 0:1)});
        }
        sim.Fx.clear();
        if(lan.IsHosting() && screen==ScreenMode::Playing) {
            // Keep the network alive while paused; guest sees the same frozen world and pause state.
            if(paused) { InputMsg input{}; std::string hero; lan.PollHostInput(input,hero); }
            netSnapTimer+=dt;
            if(netSnapTimer>=0.05f) {
                netSnapTimer=0; FrameSnap snapshot=sim.CaptureSnapshot(); snapshot.Fxs=pendingNetworkFx;
                if(paused) snapshot.SimState="pause";
                lan.BroadcastSnapshot(snapshot); pendingNetworkFx.clear();
            }
        }

        BeginTextureMode(target);
        rlPushMatrix();rlScalef(static_cast<float>(settings.RenderScale()),static_cast<float>(settings.RenderScale()),1);
        if (screen == ScreenMode::Title)
        {
            ClearBackground(Color{12, 8, 18, 255});
            const Texture2D* bg = sprites.Get("castle_bg");
            if (bg) DrawTexturePro(*bg, Rectangle{0, 0, (float)bg->width, (float)bg->height}, Rectangle{0, 0, T::ViewW, T::ViewH}, Vector2{0, 0}, 0.0f, Color{180, 160, 200, 255});
            DrawRectangle(0, 0, T::ViewW, T::ViewH, Color{10, 6, 16, 150});
            DrawText("MIKE & DENYSE", 250, 120, 56, Color{230, 195, 110, 255});
            DrawText("NIGHTFALL", 335, 185, 48, Color{200, 70, 90, 255});
            DrawText("16 noites - 32 monstros - 16 chefes - Belial", 245, 260, 20, LIGHTGRAY);
            Button({300,300,360,52},"Um jogador");
            Button({300,362,360,52},"Dois celulares - Wi-Fi LAN");
            Button({300,424,170,48},"Controles"); Button({490,424,170,48},"Sair");
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
            DrawText(wantLan ? "Coop Wi-Fi: cada cacador joga no seu celular" : "Um jogador: toque no seu cacador", 250,415,20,GOLD);
            Button({300,452,360,58},wantLan ? "Ir para sala Wi-Fi" : "Ir ao mapa");
            Button({820,12,124,44},"Voltar");
        }
        else if(screen == ScreenMode::Help) {
            ClearBackground({16,10,24,255});
            DrawText("COMO ATRAVESSAR A NOITE",190,70,34,GOLD);
            DrawText("Mike: espada e ataque corpo a corpo. Denyse: cristais e queda lenta.",90,145,21,WHITE);
            DrawText("Toque: esquerda / direita / descer, GOLPE, PULO e II para pausar.",90,195,21,WHITE);
            DrawText(SLTPrefs::Mobile?"Botoes ajustaveis: tamanho, margem, transparencia e modo canhoto.":"Teclas remapeaveis: abra Configuracoes > Controles.",90,245,21,WHITE);
            DrawText("Venca o chefe e toque o sino. As 16 noites abrem em ordem.",90,295,21,WHITE);
            DrawText("Coop: mesma rede Wi-Fi, criar sala e tocar a sala no outro celular.",90,345,21,WHITE);
            DrawText("Controle: direcional, A para pular, X para atacar, B para voltar.",90,395,21,WHITE);
            Button({820,12,124,44},"Voltar");
        }
        else if (screen == ScreenMode::LobbyLan)
        {
            ClearBackground(Color{14, 14, 24, 255});
            DrawText("SALA COOPERATIVA LAN WI-FI (UDP 47888 / 7777)", 160, 45, 28, GOLD);
            DrawText(TextFormat("IP Local Preferencial (LanIp.Pick): %s", LanIp::Pick().c_str()), 160, 95, 20, SKYBLUE);
            DrawText(lan.IsHosting() ? "Sala criada. O outro celular pode entrar." : lan.IsConnectedClient() ? "Conectado - aguardando o host iniciar" : "Procurando salas na mesma rede Wi-Fi...",100,115,21,GREEN);
            Button({100,150,280,54},"Criar sala"); Button({500,150,360,54},"Escolher fase",lan.IsHosting());
            int ry=230;
            for(const auto& room:lan.Rooms()) {
                if(ry>420) break;
                Button({100,static_cast<float>(ry),760,44},TextFormat("Entrar: %s - %s",room.name.c_str(),room.host.c_str())); ry+=52;
            }
            DrawText("Cada aparelho precisa de um heroi diferente. UDP 47888 / 7777.",100,485,19,LIGHTGRAY);
            Button({820,12,124,44},"Voltar");
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
            Button({300,474,360,54},"Comecar a fase",Progress::CanPlay(selectedWorld));
            Button({820,12,124,44},"Voltar");
        }
        else if (screen == ScreenMode::Playing && sim.HasLevel)
        {
            Color skyColor = HexToColor(sim.Level.World.Tone, Color{14, 16, 24, 255});
            Color fogColor = HexToColor(sim.Level.World.Fog, Color{28, 32, 48, 255});
            Color groundColor = HexToColor(sim.Level.World.Ground, Color{56, 48, 40, 255});
            Color lipColor = HexToColor(sim.Level.World.Lip, Color{110, 130, 70, 255});
            ClearBackground(skyColor);

            worldArt.Prepare(sim.Level.World,sim.WorldIndex);
            worldArt.DrawSky(sim.CamX,sim.Level.Cols,sprites.Get(sim.Level.World.Id+"_bg"));

            float camX = sim.CamX + (settings.value.shake && sim.Shake > 0.1f ? std::sin(sim.Time * 60.0f) * sim.Shake * 0.4f : 0.0f);
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
                    int above=y>0?sim.Level.Tiles[y-1][x]:0;
                    bool exposed=!(above==T::Solid||above==T::Ice||above==T::Break||above==t);
                    worldArt.Tile(t,exposed,x,y,sim.Level.Rows,camX,camY,sim.Time);
                }
            }

            worldArt.Decor(sim.Level,camX,camY,sim.Time);
            // The exit bell appears after the arena boss has fallen, as in the original.
            bool bossAlive=false;
            for(const auto& boss:sim.Bosses) if(!boss.Dead) bossAlive=true;
            if(sim.LockedArena && !bossAlive && (sim.WorldIndex!=15 || sim.BelialPhase)) {
                float x=sim.Level.Exit.x+16-camX, y=sim.Level.Exit.y+16-camY;
                DrawCircle(static_cast<int>(x),static_cast<int>(y),23,Fade(GOLD,0.2f));
                DrawRectangle(static_cast<int>(x-11),static_cast<int>(y-12),22,22,GOLD);
                DrawEllipse(static_cast<int>(x),static_cast<int>(y+11),16,5,GOLD);
                DrawCircle(static_cast<int>(x),static_cast<int>(y+14),3,WHITE);
            }
            if(sim.Level.HasCheck) {
                float x=sim.Level.Check.x-camX, y=sim.Level.Check.y-camY;
                DrawRectangle(static_cast<int>(x),static_cast<int>(y-46),3,46,GOLD);
                DrawTriangle({x+3,y-45},{x+3,y-23},{x+29,y-34},SKYBLUE);
            }

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
                Color tint = e.Flash > 0.0f ? Color{255,220,200,255} : WHITE;
                DrawEllipse(static_cast<int>(e.X+e.W/2-camX),static_cast<int>(e.Y+e.H-camY),e.W*.65f,5,Color{0,0,0,85});
                DrawTexFitBottom(tex, e.X - 8 - camX, e.Y - 10 - camY, e.W+16, e.H+12, e.Facing, tint);
            }

            // Bosses
            for (const auto& b : sim.Bosses)
            {
                if (b.Dead) continue;
                const Texture2D* tex = sprites.Get(b.Id);
                Color tint = b.Flash > 0.0f ? RED : WHITE;
                DrawEllipse(static_cast<int>(b.X+b.W/2-camX),static_cast<int>(b.Y+b.H-camY),b.W*.65f,5,Color{0,0,0,85});
                DrawTexFitBottom(tex, b.X - 8 - camX, b.Y - 10 - camY, b.W+16, b.H+12, b.Facing, tint);
            }

            // Players
            for (const auto& p : sim.Players)
            {
                if (p.Dead) continue;
                std::string key = p.Hero.Id + "_idle";
                if (p.Anim == "attack") key = p.Hero.Id + "_attack";
                else if (p.Anim == "jump" || p.Anim == "fall") key = p.Hero.Id + "_jump";
                else if (p.Anim == "walk") key = p.Hero.Id + ((static_cast<int>(p.AnimT * 7.0f) % 2 == 0) ? "_walk1" : "_walk2");
                const Texture2D* tex = sprites.Get(key);
                Color tint = (p.Inv > 0.0f && (static_cast<int>(p.Inv * 20.0f) % 2 == 0)) ? Color{255, 255, 255, 140} : WHITE;
                DrawEllipse(static_cast<int>(p.X+p.W/2-camX),static_cast<int>(p.Y+p.H-camY),p.W*.65f,5,Color{0,0,0,85});
                float bob=p.Anim=="idle" ? std::sin(sim.Time*3)*1.2f:0;
                DrawTexFitBottom(tex, p.X - 14 - camX, p.Y - 16 + bob - camY, p.W+28, p.H+18, p.Facing, tint);
                if(p.Atk>0) {
                    float t=std::max(0.0f,std::min(1.0f,1-p.Atk/std::max(0.01f,p.Hero.AtkTime)));
                    Vector2 center{p.X+p.W/2-camX,p.Y+p.H*0.46f-camY};
                    if(p.Hero.AtkKind=="magic") {
                        DrawCircleLines(static_cast<int>(center.x+p.Facing*20),static_cast<int>(center.y),8+20*(1-t),Fade(SKYBLUE,1-t));
                    } else {
                        float angle=(-1.22f+t*2.44f);
                        Vector2 tip{center.x+std::cos(angle)*p.Hero.Reach*p.Facing,center.y+std::sin(angle)*p.Hero.Reach};
                        for(int trail=0;trail<3;++trail) {
                            float a=angle-trail*0.18f;
                            Vector2 end{center.x+std::cos(a)*p.Hero.Reach*p.Facing,center.y+std::sin(a)*p.Hero.Reach};
                            DrawLineEx(center,end,static_cast<float>(6-trail),Fade(GOLD,(1-t)*(1-trail*0.22f)));
                        }
                        DrawCircleV(tip,4,WHITE);
                    }
                }
            }

            // Projectiles
            for (const auto& pr : sim.Projectiles)
            {
                Color pc = pr.Friendly ? SKYBLUE : ORANGE;
                Vector2 center{pr.X+pr.W*0.5f-camX,pr.Y+pr.H*0.5f-camY};
                for(int trail=4;trail>=1;--trail) DrawCircleV({center.x-pr.Vx*trail*0.8f,center.y-pr.Vy*trail*0.8f},pr.W*0.5f*(1-trail*0.15f),Fade(pc,0.12f*(5-trail)));
                DrawCircleV(center,pr.W*0.5f,pc); DrawCircleV(center,3,WHITE);
            }
            for(const auto& fx:visualFx) {
                float t=fx.age/0.4f;
                Vector2 center{fx.event.X-camX,fx.event.Y-camY};
                Color color=fx.event.Kind==static_cast<uint8_t>(FxKind::Heal) ? GREEN : fx.event.Kind==static_cast<uint8_t>(FxKind::Rescue) ? SKYBLUE : GOLD;
                DrawCircleLines(static_cast<int>(center.x),static_cast<int>(center.y),4+35*t,Fade(color,1-t));
                for(int k=0;k<6;++k) {
                    float a=k*1.0472f+fx.event.Kind;
                    Vector2 spark{center.x+std::cos(a)*45*t,center.y+std::sin(a)*35*t};
                    DrawCircleV(spark,3*(1-t),Fade(color,1-t));
                }
            }

            // Original two-player HUD: independent hearts and lives, shared souls/score.
            for(size_t slot=0;slot<sim.Players.size() && slot<2;++slot) {
                const auto& player=sim.Players[slot];
                float x=slot==0 ? 106.f:714.f;
                DrawRectangleRec({x,12,232,52},Color{8,3,8,158});
                DrawRectangleRec({x,12,232,2},Color{212,181,107,140});
                DrawCircle(static_cast<int>(x+13),25,5,slot==0 ? GOLD:SKYBLUE);
                DrawText(TextFormat("P%d  %s",static_cast<int>(slot)+1,player.Hero.Name.c_str()),static_cast<int>(x+24),17,15,Color{230,209,158,255});
                for(int hp=0;hp<player.MaxHp && hp<10;++hp) worldArt.Heart(x+24+hp*19,36,hp<player.Hp);
                for(int life=0;life<std::clamp(player.Lives,0,5);++life) DrawCircle(static_cast<int>(x+200-life*10),45,3.5f,GOLD);
                if(player.Dead)DrawText("CAIDO",static_cast<int>(x+24),36,15,RED);
            }
            DrawRectangle(350,12,300,26,Color{8,3,8,140});
            DrawText(TextFormat("Almas: %d",sim.Souls),362,18,15,Color{212,189,255,255});
            DrawText(TextFormat("%d pts",sim.Score),500,18,15,GOLD);
            std::string worldLabel=std::to_string(sim.WorldIndex+1)+" / 16 - "+sim.Level.World.Name;
            DrawText(worldLabel.c_str(),(960-MeasureText(worldLabel.c_str(),13))/2,45,13,Color{184,163,140,255});

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

            if(settings.value.particles)worldArt.Weather(sim.Level.World.Id,sim.Time);
            if(settings.value.particles)worldArt.Atmosphere();
            if (showTouchHud) {
                auto pad=[&](Vector2 c,float radius,const char* label,bool held) {
                    c=settings.ControlPoint(c,960,540);radius*=settings.value.touchSize;
                    DrawCircleV(c,radius,held ? Color{150,110,60,160}:ColorAlpha(Color{10,5,18,255},settings.value.touchOpacity));
                    DrawCircleLines(static_cast<int>(c.x),static_cast<int>(c.y),radius,GOLD);
                    DrawText(label,static_cast<int>(c.x-MeasureText(label,21)/2),static_cast<int>(c.y-10),21,WHITE);
                };
                pad({80,430},58,"<",touchCtl.L); pad({204,430},58,">",touchCtl.R);
                pad({131,315},47,"v",touchCtl.D); pad({690,452},56,"GOLPE",heldAttack); pad({862,422},66,"PULO",touchCtl.Jn);
            }
            Button({16,10,78,46},"II");
            if(sim.Fade>0) DrawRectangle(0,0,960,540,Fade(BLACK,std::min(1.0f,sim.Fade)));
            if(paused || sim.State=="pause" || sim.State=="dead" || sim.State=="clear" || sim.State=="win") {
                DrawRectangle(0,0,960,540,Color{10,0,15,195});
                const char* title=paused || sim.State=="pause" ? "PAUSADO" : sim.State=="dead" ? "OS CACADORES CAIRAM" : sim.State=="win" ? "BELIAL DERROTADO!" : "NOITE CONCLUIDA";
                DrawText(title,(960-MeasureText(title,34))/2,210,34,GOLD);
                bool owner=!lan.IsConnectedClient();
                Button({290,285,380,54},paused ? "Continuar" : sim.State=="dead" ? "Renascem" : "Proxima noite",paused||owner);
                Button({290,350,380,54},"Voltar ao mapa"); Button({290,415,380,54},"Menu inicial");
            }
        }
        if(screen!=ScreenMode::Playing)Button({680,480,260,44},"Configuracoes");
        else if(paused)Button({680,145,260,44},"Configuracoes");
        rlDrawRenderBatchActive();rlPopMatrix();
        EndTextureMode();
        if(!renderSmokeDirectory.empty() && ++renderSmokeFrame%180==0) {
            Image image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            auto path=std::filesystem::path(renderSmokeDirectory)/("native-"+sim.Level.World.Id+".png");
            if(!ExportImage(image,path.string().c_str())) { UnloadImage(image); return 4; }
            UnloadImage(image);
            int next=sim.WorldIndex+1;
            if(next<16) sim.Start(next,"mike","denyse",true);
            else { TraceLog(LOG_INFO,"NIGHTFALL_RENDER_SMOKE_OK worlds=16 assets=77");quit=true; }
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            target.texture,
            Rectangle{0.0f, 0.0f, static_cast<float>(target.texture.width), -static_cast<float>(target.texture.height)},
            viewport,
            Vector2{0.0f, 0.0f},
            0.0f,
            WHITE
        );
        if(settings.value.brightness<1)DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(BLACK,1-settings.value.brightness));
        else if(settings.value.brightness>1)DrawRectangle(0,0,GetScreenWidth(),GetScreenHeight(),Fade(WHITE,(settings.value.brightness-1)*.45f));
        if(settings.value.showFps)DrawFPS(8,GetScreenHeight()-22);
        settings.Draw();
        EndDrawing();
        if(!settingsSmokeDirectory.empty() && ++settingsSmokeFrame%2==0){Image capture=LoadImageFromScreen();std::string file=(std::filesystem::path(settingsSmokeDirectory)/("settings-"+std::to_string((settingsSmokeFrame-1)/2)+".png")).string();bool saved=ExportImage(capture,file.c_str());UnloadImage(capture);if(!saved)return 9;if(settingsSmokeFrame>=10)quit=true;}

    }

    UnloadRenderTexture(target);
    worldArt.Unload();
    sprites.UnloadAll();
    sfx.Shutdown();
    CloseWindow();
    return 0;
}
