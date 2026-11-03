#pragma once
// Native port of NightfallUnity/Assets/Scripts/ArtGen.cs and WorldView.cs.
// Original painted assets are preserved; procedural textures are generated per world.
#include "NightfallCore.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Nightfall
{
class NativeWorldArt
{
    struct FColor
    {
        float r = 0, g = 0, b = 0, a = 0;
        FColor() = default;
        FColor(float rr, float gg, float bb, float aa = 1) : r(rr), g(gg), b(bb), a(aa)
        {
        }
        FColor(Color c) : r(c.r / 255.f), g(c.g / 255.f), b(c.b / 255.f), a(c.a / 255.f)
        {
        }
        Color Pixel() const
        {
            return {static_cast<unsigned char>(std::clamp(r, 0.f, 1.f) * 255),
                    static_cast<unsigned char>(std::clamp(g, 0.f, 1.f) * 255),
                    static_cast<unsigned char>(std::clamp(b, 0.f, 1.f) * 255),
                    static_cast<unsigned char>(std::clamp(a, 0.f, 1.f) * 255)};
        }
    };
    struct M
    {
        static float Sin(float x)
        {
            return std::sin(x);
        }
        static float Abs(float x)
        {
            return std::abs(x);
        }
        static float Pow(float x, float y)
        {
            return std::pow(x, y);
        }
        static float Sqrt(float x)
        {
            return std::sqrt(x);
        }
        static int RoundToInt(float x)
        {
            return static_cast<int>(std::nearbyint(x));
        }
        static int FloorToInt(float x)
        {
            return static_cast<int>(std::floor(x));
        }
        static float Max(float a, float b)
        {
            return std::max(a, b);
        }
        static float Clamp01(float x)
        {
            return std::clamp(x, 0.f, 1.f);
        }
    };
    static constexpr int TS = 40;
    static FColor Mul(FColor c, float f)
    {
        return {c.r * f, c.g * f, c.b * f, c.a};
    }
    static FColor Mix(FColor a, FColor b, float t)
    {
        t = std::clamp(t, 0.f, 1.f);
        return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
    }
    static FColor Lift(FColor c, float f)
    {
        float m = std::max({0.0001f, c.r, c.g, c.b}), k = std::clamp(m * f, 0.f, 1.f) / m;
        return {std::clamp(c.r * k + 0.03f, 0.f, 1.f), std::clamp(c.g * k + 0.03f, 0.f, 1.f),
                std::clamp(c.b * k + 0.045f, 0.f, 1.f), 1};
    }
    static FColor Hex(const std::string &text)
    {
        uint8_t r, g, b, a;
        GameSim::TryParseHexColor(text, r, g, b, a);
        return FColor(Color{r, g, b, a});
    }
    static float Hash(float x, float y, float seed)
    {
        return LevelBuilder::ArtHash(x, y, seed);
    }
    static float Noise(float x, float y, float seed)
    {
        int xi = static_cast<int>(std::floor(x)), yi = static_cast<int>(std::floor(y));
        float xf = x - xi, yf = y - yi, u = xf * xf * (3 - 2 * xf), v = yf * yf * (3 - 2 * yf);
        float a = Hash(xi, yi, seed), b = Hash(xi + 1, yi, seed), c = Hash(xi, yi + 1, seed),
              d = Hash(xi + 1, yi + 1, seed);
        return (a + (b - a) * u) + ((c + (d - c) * u) - (a + (b - a) * u)) * v;
    }
    static void Put(std::vector<FColor> &pixels, int stride, int cell, bool exposed, int x, int y, FColor color)
    {
        if (x < 0 || y < 0 || x >= 40 || y >= 40)
            return;
        pixels[((exposed ? 0 : 40) + y) * stride + cell * 40 + x] = color;
    }
    static Texture2D Upload(const std::vector<FColor> &colors, int width, int height, bool point = false)
    {
        Image image = GenImageColor(width, height, BLANK);
        auto pixels = static_cast<Color *>(image.data);
        for (size_t i = 0; i < colors.size(); ++i)
            pixels[i] = colors[i].Pixel();
        auto texture = LoadTextureFromImage(image);
        UnloadImage(image);
        SetTextureFilter(texture, point ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
        return texture;
    }
    static Texture2D Ridge(float seed, int style, int height)
    {
        int width = 512;
        std::vector<FColor> colors(width * height);
        for (int x = 0; x < width; ++x)
        {
            float fx = x / static_cast<float>(width), profile;
            if (style == 0)
                profile = .42f + std::sin(fx * 9.2f + seed) * .13f + std::sin(fx * 23 + seed * 3) * .06f +
                          Noise(x * .05f, seed, seed) * .10f;
            else if (style == 1)
                profile = .30f + std::abs(std::sin(fx * 12 + seed)) * .42f + Noise(x * .09f, seed, seed) * .08f;
            else
                profile = .30f + Hash(std::floor(fx * 22), 3, seed) * .45f;
            int top = std::clamp(static_cast<int>(std::nearbyint((1 - profile) * height)), 0, height - 1);
            for (int y = top; y < height; ++y)
                colors[y * width + x] = FColor(1, 1, 1);
            if (style == 2)
                for (int y = top + 4; y < height - 2; y += 7)
                    if (Hash(x, y, seed) > .94f)
                        colors[y * width + x] = FColor(1, .75f, .35f);
        }
        return Upload(colors, width, height);
    }
    Texture2D atlas{}, sky{}, farRidge{}, nearRidge{}, vignette{}, heart{}, emptyHeart{};
    int built = -1;
    FColor tone, fog, lip;

  public:
    void Unload()
    {
        for (auto texture : {atlas, sky, farRidge, nearRidge, vignette, heart, emptyHeart})
            if (texture.id)
                UnloadTexture(texture);
        atlas = sky = farRidge = nearRidge = vignette = heart = emptyHeart = {};
        built = -1;
    }
    void Prepare(const WorldDef &world, int index)
    {
        if (index == built)
            return;
        Unload();
        built = index;
        tone = Hex(world.Tone);
        fog = Hex(world.Fog);
        lip = Hex(world.Lip);
        // .NET String.GetHashCode is process-dependent. Stable FNV seed keeps native artwork reproducible.
        uint32_t hash = 2166136261u;
        for (unsigned char c : world.Id)
        {
            hash ^= c;
            hash *= 16777619u;
        }
        float seed = static_cast<float>(hash % 997);
        std::vector<FColor> pixels(440 * 80);
        for (int cell = 1; cell < 11; ++cell)
            for (int variant = 0; variant < 2; ++variant)
                PaintTile(pixels, 440, cell, variant == 1, Hex(world.Ground), lip, seed);
        atlas = Upload(pixels, 440, 80, true);
        pixels.assign(4 * 160, FColor{});
        auto horizon = Lift(fog, 2.15f), zenith = Mul(tone, .45f);
        for (int y = 0; y < 160; ++y)
        {
            float v = 1 - y / 159.f;
            auto c = Mix(horizon, zenith, std::pow(v, .62f));
            if (v > .55f && Hash(std::floor(v * 900), 3, 11) > .985f)
                c = Mix(c, FColor(.9f, .92f, 1), .45f);
            for (int x = 0; x < 4; ++x)
                pixels[y * 4 + x] = c;
        }
        sky = Upload(pixels, 4, 160);
        int style = (world.Id == "castle" || world.Id == "cathedral" || world.Id == "village" || world.Id == "throne" ||
                     world.Id == "cabin")
                        ? 2
                    : (world.Id == "peak" || world.Id == "ice" || world.Id == "abyss" || world.Id == "volcano") ? 1
                                                                                                                : 0;
        seed = static_cast<float>(hash % 500);
        farRidge = Ridge(seed, style, 170);
        nearRidge = Ridge(seed + 31, style == 2 ? 2 : 0, 150);
        pixels.assign(96 * 64, FColor{});
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 96; ++x)
            {
                float dx = (x + .5f) / 96 * 2 - 1, dy = (y + .5f) / 64 * 2 - 1, d = std::sqrt(dx * dx * .85f + dy * dy),
                      a = std::clamp((d - .55f) / .75f, 0.f, 1.f);
                pixels[y * 96 + x] = FColor(0, 0, 0, a * a * .85f);
            }
        vignette = Upload(pixels, 96, 64);
        for(bool full:{true,false}) {
            pixels.assign(24*24,FColor{});
            for(int y=0;y<24;++y)for(int x=0;x<24;++x) {
                float fx=(x+.5f)/24*2-1,fy=1-(y+.5f)/24*2+.18f,v=fx*fx+fy*fy-.42f;
                if(v*v*v-fx*fx*fy*fy*fy>=0)continue;
                float edge=std::clamp(1.6f+fy,0.f,1.f);
                auto color=full?Mix({.98f,.34f,.38f},{.55f,.06f,.13f},1-edge*.8f):FColor(.2f,.08f,.11f);
                if(full && fx<-.15f && fy>.25f)color=Mix(color,{1,1,1},.55f);
                pixels[y*24+x]=color;
            }
            if(full)heart=Upload(pixels,24,24);else emptyHeart=Upload(pixels,24,24);
        }
    }
    void Heart(float x,float y,bool full) { DrawTexturePro(full?heart:emptyHeart,{0,0,24,24},{x,y,17,17},{0,0},0,WHITE); }
    void DrawSky(float camX, int columns, const Texture2D *painted)
    {
        float span = std::max(1, columns * 40 - 960), t = std::clamp(camX / span, 0.f, 1.f);
        auto draw = [](Texture2D tex, Rectangle rect, Color tint) {
            DrawTexturePro(tex, {0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height)}, rect, {0, 0}, 0,
                           tint);
        };
        draw(sky, {0, 0, 960, 540}, WHITE);
        DrawCircle(818 - static_cast<int>(t * 26), 82, 48, Color{180, 132, 114, 140});
        auto far = Lift(fog, 1.35f);
        far.a = .9f;
        draw(farRidge, {-t * 90, 272, 1080, 170}, far.Pixel());
        if (painted)
        {
            float scale = 540.f / painted->height, width = painted->width * scale;
            draw(*painted, {-t * std::max(0.f, width - 960), 0, width, 540}, Color{255, 255, 255, 158});
        }
        draw(nearRidge, {-t * 230, 344, 1220, 152}, FColor(tone.r * .75f, tone.g * .75f, tone.b * .85f, .98f).Pixel());
        for (int i = 0; i < 5; ++i)
            DrawRectangle(0, 380 + i * 32, 960, 32, FColor(fog.r, fog.g, fog.b, .05f + i * .025f).Pixel());
    }
    void Tile(int type, bool exposed, int x, int y, int rows, float camX, float camY, float time)
    {
        if (type < 1 || type > 10)
            return;
        float shade = (.88f + Hash(x, y, 5.1f) * .22f) * (1 - std::clamp((y - (rows - 6)) / 6.f, 0.f, 1.f) * .28f);
        bool liquid = type == T::Lava || type == T::Water;
        bool flip = !liquid && type != T::Ladder && Hash(x, y, 9.3f) > .5f;
        DrawTexturePro(atlas, {type * 40.f, exposed ? 0.f : 40.f, flip ? -40.f : 40.f, 40},
                       {x * 40.f - camX, y * 40.f - camY, 40, 40}, {0, 0}, 0,
                       liquid ? WHITE : FColor(shade, shade, shade * 1.02f).Pixel());
        float pulse = .5f + std::sin(time * 3.4f) * .5f;
        if (type == T::Lava)
            DrawRectangle(static_cast<int>(x * 40 - camX), static_cast<int>(y * 40 - camY), 40, 40,
                          FColor(1, .35f + pulse * .35f, .08f, .22f).Pixel());
        if (type == T::Water)
        {
            DrawRectangle(static_cast<int>(x * 40 - camX), static_cast<int>(y * 40 - camY), 40, 40,
                          FColor(.16f, .47f, .63f, .18f + pulse * .08f).Pixel());
            if (exposed)
                DrawRectangle(static_cast<int>(x * 40 - camX),
                              static_cast<int>(y * 40 - camY + std::sin(time * 2.4f + x) * 1.6f), 40, 3,
                              FColor(.75f, .95f, 1, .35f).Pixel());
        }
    }
    void Decor(const LevelData &level, float camX, float camY, float time)
    {
        auto rect = [](float x, float y, float w, float h, FColor color) {
            DrawRectangleRec({x, y, w, h}, color.Pixel());
        };
        auto disc = [](float x, float y, float w, float h, FColor color) {
            DrawEllipse(static_cast<int>(x + w / 2), static_cast<int>(y + h / 2), w / 2, h / 2, color.Pixel());
        };
        for (const auto &d : level.Decor)
        {
            float x = d.x - camX, y = d.y - camY, s = d.scale;
            if (x < -300 || x > 1260)
                continue;
            if (d.kind == "tree" || d.kind == "canopy")
            {
                rect(x - 5 * s, y - 74 * s, 10 * s, 74 * s, {.16f, .11f, .07f});
                disc(x - 46 * s, y - 128 * s, 92 * s, 74 * s, {lip.r * .35f, lip.g * .42f, lip.b * .3f, .95f});
                disc(x - 30 * s, y - 150 * s, 62 * s, 56 * s, {lip.r * .26f, lip.g * .34f, lip.b * .24f, .95f});
            }
            else if (d.kind == "pillar")
            {
                rect(x - 13 * s, y - 118 * s, 26 * s, 118 * s, {.2f, .17f, .2f});
                rect(x - 18 * s, y - 126 * s, 36 * s, 12 * s, {.28f, .24f, .27f});
                rect(x - 18 * s, y - 10 * s, 36 * s, 10 * s, {.24f, .2f, .23f});
            }
            else if (d.kind == "tomb")
            {
                rect(x - 13 * s, y - 40 * s, 26 * s, 40 * s, {.3f, .3f, .32f});
                disc(x - 13 * s, y - 50 * s, 26 * s, 24 * s, {.3f, .3f, .32f});
                rect(x - 3 * s, y - 34 * s, 6 * s, 18 * s, {.16f, .16f, .18f});
                rect(x - 9 * s, y - 28 * s, 18 * s, 5 * s, {.16f, .16f, .18f});
            }
            else if (d.kind == "obelisk")
            {
                rect(x - 11 * s, y - 96 * s, 22 * s, 96 * s, {.42f, .32f, .18f});
                DrawTriangle({x, y - 118 * s}, {x - 11 * s, y - 94 * s}, {x + 11 * s, y - 94 * s},
                             FColor(.52f, .40f, .22f).Pixel());
            }
            else if (d.kind == "crystal")
            {
                DrawTriangle({x, y - 62 * s}, {x - 15 * s, y - 20 * s}, {x + 15 * s, y - 20 * s},
                             FColor(.55f, .80f, .98f, .85f).Pixel());
                DrawTriangle({x, y}, {x + 15 * s, y - 20 * s}, {x - 15 * s, y - 20 * s},
                             FColor(.38f, .62f, .88f, .85f).Pixel());
            }
            else if (d.kind == "rock")
            {
                disc(x - 26 * s, y - 30 * s, 52 * s, 34 * s, {.22f, .19f, .18f});
                disc(x - 12 * s, y - 42 * s, 30 * s, 26 * s, {.26f, .22f, .21f});
            }
            else if (d.kind == "reed")
            {
                for (int i = -2; i <= 2; ++i)
                    rect(x + i * 8 * s, y - (34 - std::abs(i) * 6) * s, 3 * s, (34 - std::abs(i) * 6) * s,
                         {.22f, .3f, .2f, .95f});
            }
            else if (d.kind == "torch" || d.kind == "candle" || d.kind == "lamp")
            {
                float fl = .72f + std::sin(time * 9 + x * .05f) * .28f;
                rect(x - 3 * s, y - 26 * s, 6 * s, 26 * s, {.24f, .17f, .11f});
                for (int k = 3; k >= 1; --k)
                    disc(x - k * 12 * s * fl, y - 33 * s - k * 12 * s * fl, k * 24 * s * fl, k * 24 * s * fl,
                         {1, .66f, .28f, .025f});
                disc(x - 8 * s, y - 46 * s, 16 * s, 26 * s, {1, .72f, .28f, .95f});
            }
            else if (d.kind == "shrine")
            {
                rect(x - 22 * s, y - 16 * s, 44 * s, 16 * s, {.26f, .22f, .24f});
                DrawCircleLines(static_cast<int>(x), static_cast<int>(y - 50 * s), 26 * s, GOLD);
            }
            else if (d.kind == "arch")
            {
                rect(x - 62 * s, y - 128 * s, 20 * s, 128 * s, {.19f, .16f, .19f});
                rect(x + 42 * s, y - 128 * s, 20 * s, 128 * s, {.19f, .16f, .19f});
                rect(x - 66 * s, y - 142 * s, 132 * s, 20 * s, {.23f, .19f, .22f});
            }
        }
    }
    void Weather(const std::string &id, float time)
    {
        bool snow = id == "ice", ember = id == "volcano" || id == "throne", rain = id == "swamp" || id == "catacombs",
             firefly = id == "forest" || id == "woods" || id == "coven";
        // Cosmetic hash uses no gameplay RNG, so rendering never changes simulation or LAN state.
        for (int i = 0; i < 45; ++i)
        {
            float seed = Hash(i, 7, 11), phase = std::fmod(time * (snow    ? .07f
                                                                   : ember ? .10f
                                                                   : rain  ? .25f
                                                                           : .03f) +
                                                               seed,
                                                           1.f);
            float x = Hash(i, 3, 5) * 1040 - 40 + std::sin(time * .4f + i) * 12,
                  y = (ember ? 1 - phase : phase) * 580 - 20;
            Color color = snow      ? Color{235, 247, 255, 180}
                          : ember   ? Color{255, 150, 45, 170}
                          : rain    ? Color{140, 215, 242, 140}
                          : firefly ? Color{190, 255, 115, 140}
                                    : Color{150, 140, 115, 55};
            if (firefly)
                y = Hash(i, 5, 9) * 540 + std::sin(time + i) * 12;
            if (rain)
                DrawLineEx({x, y}, {x, y + 6}, 1.5f, color);
            else
                DrawCircleV({x, y}, snow ? 1.5f : ember ? 2 : firefly ? 2 : 1, color);
        }
    }
    void Atmosphere()
    {
        DrawTexturePro(vignette, {0, 0, 96, 64}, {0, 0, 960, 540}, {0, 0}, 0, WHITE);
    }

  private:
    static void PaintTile(std::vector<FColor> &pix, int stride, int cell, bool exposed, FColor ground, FColor lip,
                          float seed)
    {
        switch (cell)
        {
        case T::Solid:
            PaintGround(pix, stride, cell, exposed, ground, lip, seed);
            break;
        case T::Ice:
            PaintGround(pix, stride, cell, exposed, FColor(0.42f, 0.62f, 0.74f), FColor(0.88f, 0.96f, 1.0f), seed,
                        true);
            break;
        case T::Break:
            PaintBrick(pix, stride, cell, exposed, seed);
            break;
        case T::Platform:
            PaintPlatform(pix, stride, cell, exposed, lip, seed);
            break;
        case T::Spike:
            PaintSpike(pix, stride, cell, exposed);
            break;
        case T::Thorn:
            PaintThorn(pix, stride, cell, exposed, seed);
            break;
        case T::Lava:
            PaintLiquid(pix, stride, cell, exposed, FColor(0.62f, 0.09f, 0.03f), FColor(1.0f, 0.62f, 0.16f), seed,
                        true);
            break;
        case T::Water:
            PaintLiquid(pix, stride, cell, exposed, FColor(0.06f, 0.18f, 0.28f), FColor(0.28f, 0.62f, 0.78f), seed,
                        false);
            break;
        case T::Bounce:
            PaintBounce(pix, stride, cell, exposed);
            break;
        case T::Ladder:
            PaintLadder(pix, stride, cell, exposed);
            break;
        }
    }

    static void PaintGround(std::vector<FColor> &pix, int stride, int cell, bool exposed, FColor body, FColor cap,
                            float seed, bool crystal = false)
    {
        for (int x = 0; x < TS; x++)
        {
            int capH = exposed ? 7 + M::RoundToInt(Noise(x * 0.28f, 0, seed) * 4.0f) : 0;
            for (int y = 0; y < TS; y++)
            {
                float n = Noise(x * 0.35f, y * 0.35f, seed);
                float broad = Noise(x * 0.06f, y * 0.06f, seed + 13); // low frequency: breaks up the grid
                float n2 = Noise(x * 0.11f, y * 0.11f, seed + 5);
                FColor c;
                if (y < capH)
                {
                    c = Mul(cap, 0.80f + n * 0.36f);
                    if (y >= capH - 2)
                        c = Mul(c, 0.62f); // shaded underside of the cap
                    if (y < 2)
                        c = Mix(c, FColor(1, 1, 1), 0.14f);
                }
                else
                {
                    float depth = M::Clamp01((y - capH) / (float)TS);
                    c = Mul(body, 0.68f + n * 0.22f + broad * 0.26f - depth * 0.20f);
                    if (n2 > 0.80f)
                        c = Mix(c, cap, 0.16f); // embedded pebbles
                    if (crystal && ((x + y) % 13 == 0))
                        c = Mix(c, FColor(1, 1, 1), 0.26f);
                }
                // barely-there bevel: the old hard edges printed a visible grid over the whole level
                if (x == 0)
                    c = Mul(c, 0.95f);
                if (x == TS - 1)
                    c = Mul(c, 0.97f);
                c.a = 1;
                Put(pix, stride, cell, exposed, x, y, c);
            }
        }
    }

    static void PaintBrick(std::vector<FColor> &pix, int stride, int cell, bool exposed, float seed)
    {
        FColor body = FColor(0.30f, 0.21f, 0.16f), mortar = FColor(0.16f, 0.11f, 0.09f),
               crack = FColor(0.55f, 0.42f, 0.26f);
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++)
            {
                int row = y / 10;
                int off = (row % 2) * 10;
                bool line = y % 10 == 0 || (x + off) % 20 == 0;
                float n = Noise(x * 0.4f, y * 0.4f, seed);
                FColor c = line ? mortar : Mul(body, 0.85f + n * 0.35f);
                if (M::Abs(x - y) < 2 || M::Abs(TS - x - y) < 2)
                    c = Mix(c, crack, 0.35f);
                c.a = 1;
                Put(pix, stride, cell, exposed, x, y, c);
            }
    }

    static void PaintPlatform(std::vector<FColor> &pix, int stride, int cell, bool exposed, FColor cap, float seed)
    {
        int h = 15;
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < h; y++)
            {
                float n = Noise(x * 0.5f, y * 1.4f, seed);
                FColor c = Mix(FColor(0.55f, 0.40f, 0.22f), Mul(cap, 0.9f), 0.35f);
                c = Mul(c, 0.78f + n * 0.30f);
                if (y < 2)
                    c = Mix(c, FColor(1, 1, 1), 0.30f); // top light
                if (y >= h - 3)
                    c = Mul(c, 0.55f); // underside shadow
                if (y % 6 == 3)
                    c = Mul(c, 0.86f); // plank grain
                c.a = 1;
                Put(pix, stride, cell, exposed, x, y, c);
            }
        for (int x = 0; x < TS; x++)
            Put(pix, stride, cell, exposed, x, h, FColor(0, 0, 0, 0.35f));
    }

    static void PaintSpike(std::vector<FColor> &pix, int stride, int cell, bool exposed)
    {
        FColor rock = FColor(0.19f, 0.14f, 0.17f);
        FColor steel = FColor(0.78f, 0.80f, 0.86f);
        for (int x = 0; x < TS; x++)
            for (int y = 30; y < TS; y++)
            {
                float n = Noise(x * 0.4f, y * 0.4f, 11);
                Put(pix, stride, cell, exposed, x, y, Mul(rock, 0.8f + n * 0.4f));
            }
        // four cones, tips near the top of the cell
        for (int k = 0; k < 4; k++)
        {
            int cx = 5 + k * 10;
            for (int y = 4; y < 32; y++)
            {
                float t = (y - 4) / 28.0f;
                int half = M::RoundToInt(t * 4.6f);
                for (int x = cx - half; x <= cx + half; x++)
                {
                    float side = (x - (cx - half)) / M::Max(1.0f, half * 2.0f);
                    FColor c = Mix(Mul(steel, 1.0f), Mul(steel, 0.42f), side);
                    if (side < 0.28f)
                        c = Mix(c, FColor(1, 1, 1), 0.35f);
                    c.a = 1;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
            }
        }
    }

    static void PaintThorn(std::vector<FColor> &pix, int stride, int cell, bool exposed, float seed)
    {
        FColor vine = FColor(0.15f, 0.18f, 0.08f);
        FColor thorn = FColor(0.62f, 0.58f, 0.36f);
        for (int x = 0; x < TS; x++)
            for (int y = 26; y < TS; y++)
            {
                float n = Noise(x * 0.3f, y * 0.3f, seed + 2);
                Put(pix, stride, cell, exposed, x, y, Mul(FColor(0.12f, 0.13f, 0.07f), 0.8f + n * 0.5f));
            }
        for (int s = 0; s < 3; s++)
        {
            float phase = s * 2.1f + seed;
            for (int y = 4; y < TS; y++)
            {
                int x = M::RoundToInt(TS * 0.5f + M::Sin(y * 0.30f + phase) * (11 + s * 3) + (s - 1) * 3);
                for (int t = -1; t <= 1; t++)
                    Put(pix, stride, cell, exposed, x + t, y, Mul(vine, t == -1 ? 1.5f : 1.0f));
                if (y % 7 == 3)
                {
                    int dir = (y / 7 + s) % 2 == 0 ? 1 : -1;
                    for (int k = 1; k <= 4; k++)
                        Put(pix, stride, cell, exposed, x + dir * k, y - k, Mul(thorn, 1.0f - k * 0.12f));
                }
            }
        }
    }

    static void PaintLiquid(std::vector<FColor> &pix, int stride, int cell, bool exposed, FColor deep, FColor bright,
                            float seed, bool lava)
    {
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++)
            {
                float n = Noise(x * 0.18f, y * 0.18f, seed);
                float depth = y / (float)TS;
                FColor c = Mix(bright, deep, M::Clamp01(depth * 1.15f + n * 0.25f));
                float a = lava ? 1.0f : 0.72f;
                if (exposed && y < 5)
                {
                    c = Mix(c, FColor(1, 1, 1), lava ? 0.30f : 0.42f);
                    a = lava ? 1.0f : 0.88f;
                }
                if (lava)
                {
                    float vein = Noise(x * 0.5f, y * 0.16f, seed + 9);
                    if (vein > 0.72f)
                        c = Mix(c, FColor(1.0f, 0.94f, 0.62f), (vein - 0.72f) * 3.0f);
                    if (exposed && y < 3 && Noise(x * 0.7f, 0, seed + 4) > 0.55f)
                        c = Mul(FColor(0.20f, 0.09f, 0.07f), 1.0f); // crust
                }
                c.a = a;
                Put(pix, stride, cell, exposed, x, y, c);
            }
    }

    static void PaintBounce(std::vector<FColor> &pix, int stride, int cell, bool exposed)
    {
        FColor capA = FColor(0.45f, 0.82f, 0.36f), capB = FColor(0.14f, 0.35f, 0.16f),
               stem = FColor(0.86f, 0.84f, 0.72f);
        for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++)
            {
                float fx = (x + 0.5f) / TS * 2 - 1;
                bool dome = y < 17 && M::Abs(fx) <= M::Sqrt(M::Max(0, 1 - M::Pow(y / 17.0f, 2.0f))) * 1.0f;
                bool trunk = y >= 15 && y < TS && M::Abs(fx) < 0.34f;
                if (dome)
                {
                    FColor c = Mix(capA, capB, y / 17.0f);
                    if (y < 4)
                        c = Mix(c, FColor(1, 1, 1), 0.35f);
                    if (Noise(x * 0.6f, y * 0.6f, 6) > 0.80f)
                        c = Mix(c, FColor(1, 1, 1), 0.5f);
                    c.a = 1;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
                else if (trunk)
                {
                    FColor c = Mul(stem, 0.7f + (1 - M::Abs(fx)) * 0.4f);
                    c.a = 1;
                    Put(pix, stride, cell, exposed, x, y, c);
                }
            }
    }

    static void PaintLadder(std::vector<FColor> &pix, int stride, int cell, bool exposed)
    {
        FColor wood = FColor(0.52f, 0.36f, 0.20f);
        for (int y = 0; y < TS; y++)
        {
            for (int x = 7; x < 12; x++)
                Put(pix, stride, cell, exposed, x, y, Mul(wood, x == 7 ? 1.3f : 0.9f));
            for (int x = 28; x < 33; x++)
                Put(pix, stride, cell, exposed, x, y, Mul(wood, x == 28 ? 1.3f : 0.9f));
        }
        for (int y = 6; y < TS; y += 13)
            for (int x = 7; x < 33; x++)
                for (int t = 0; t < 4; t++)
                    Put(pix, stride, cell, exposed, x, y + t, Mul(wood, t == 0 ? 1.35f : 0.8f));
    }
};
} // namespace Nightfall
