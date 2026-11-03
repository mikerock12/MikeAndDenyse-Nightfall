#include "NightfallCore.h"
#include <iomanip>
#include <iostream>
using namespace Nightfall;
Ctl Input(int frame)
{
    int cycle = frame % 120;
    Ctl c;
    c.R = cycle < 90;
    c.L = cycle >= 90 && cycle < 110;
    c.D = cycle >= 65 && cycle < 77;
    c.Jp = cycle == 15 || cycle == 50;
    c.Jn = (cycle >= 15 && cycle < 29) || (cycle >= 50 && cycle < 61);
    c.Ap = frame % 40 == 5;
    return c;
}
int main()
{
    std::cout << std::setprecision(9);
    for (int world = 0; world < 16; ++world)
    {
        auto level = LevelBuilder::Compile(world);
        uint32_t hash = 2166136261u;
        for (const auto &row : level.Tiles)
            for (uint8_t tile : row)
            {
                hash ^= tile;
                hash *= 16777619u;
            }
        std::cout << "LEVEL," << world << "," << level.Cols << "," << level.Rows << "," << level.Spawn.x << ","
                  << level.Spawn.y << "," << level.ArenaX0 << "," << level.Exit.x << "," << hash << "\n";
        for (const char *hero : {"mike", "denyse"})
            for (int fps : {60, 30, 20})
            {
                GameSim sim;
                Ctl ctl;
                sim.ReadCtl = [&](int) { return ctl; };
                sim.Start(world, hero, std::string(hero) == "mike" ? "denyse" : "mike", false);
                sim.Enemies.clear();
                sim.Bosses.clear();
                sim.Items.clear();
                sim.Players[0].Lives = 999;
                for (int frame = 0; frame < 720; ++frame)
                {
                    ctl = Input(frame);
                    sim.Tick(1.0f / fps);
                    sim.Fx.clear();
                    const auto &p = sim.Players[0];
                    std::cout << "STEP," << world << "," << hero << "," << fps << "," << frame << "," << p.X << ","
                              << p.Y << "," << p.Vx << "," << p.Vy << "," << p.Hp << "," << p.Lives << ","
                              << (p.OnGround ? 1 : 0) << "," << p.Atk << "," << p.SafeX << "," << p.SafeY << ","
                              << p.Anim << "\n";
                }
            }
    }
    for (const char *hero : {"mike", "denyse"})
        for (const auto &enemy : Catalog::Enemies())
        {
            GameSim::InitRandomState(1234);
            GameSim sim;
            Ctl ctl;
            sim.ReadCtl = [&](int) { return ctl; };
            sim.Start(0, hero, "denyse", false);
            sim.Enemies.clear();
            sim.Bosses.clear();
            sim.Items.clear();
            sim.Players[0].Lives = 999;
            auto &player = sim.Players[0];
            player.Inv = 99;
            EnemyA actor;
            actor.Id = enemy.Id;
            actor.Def = enemy;
            actor.Kind = Catalog::EnemyIndex(enemy.Id);
            actor.Hp = enemy.Hp;
            actor.MaxHp = enemy.Hp;
            actor.W = static_cast<float>(enemy.W);
            actor.H = static_cast<float>(enemy.H);
            actor.X = player.X + 55;
            actor.Y = player.Y + player.H - enemy.H;
            actor.Dir = -1;
            actor.Facing = -1;
            actor.HomeX = actor.X;
            actor.HomeY = actor.Y;
            sim.Enemies.push_back(actor);
            for (int frame = 0; frame < 240; ++frame)
            {
                ctl = {};
                ctl.Ap = frame % 24 == 0;
                ctl.Jp = frame == 120;
                ctl.Jn = frame >= 120 && frame < 135;
                ctl.R = frame > 150 && frame < 180;
                sim.Tick(1.f / 60);
                sim.Fx.clear();
                const auto &e = sim.Enemies[0];
                std::cout << "COMBAT," << hero << "," << enemy.Id << "," << frame << "," << player.X << "," << player.Y
                          << "," << player.Hp << "," << e.X << "," << e.Y << "," << e.Hp << "," << (e.Dead ? 1 : 0)
                          << "," << sim.Projectiles.size() << "," << sim.Score << "," << sim.Souls << "\n";
            }
        }
    std::vector<BossDef> bosses = Catalog::Bosses();
    bosses.push_back(Catalog::Belial());
    for (const char *hero : {"mike", "denyse"})
        for (const auto &boss : bosses)
        {
            GameSim::InitRandomState(5678);
            GameSim sim;
            Ctl ctl;
            sim.ReadCtl = [&](int) { return ctl; };
            sim.Start(boss.Id == "warden" || boss.Id == "belial" ? 15 : 0, hero, "denyse", false);
            sim.Enemies.clear();
            sim.Bosses.clear();
            sim.Items.clear();
            auto &player = sim.Players[0];
            player.Lives = 999;
            player.Inv = 99;
            player.X = sim.Level.ArenaX0 + 200;
            player.Y = sim.Level.BossAt.y - player.H;
            sim.LockedArena = true;
            BossA actor;
            actor.Id = boss.Id;
            actor.Def = boss;
            actor.Hp = boss.Hp;
            actor.MaxHp = boss.Hp;
            actor.W = static_cast<float>(boss.W);
            actor.H = static_cast<float>(boss.H);
            actor.X = player.X + 60;
            actor.Y = player.Y + player.H - boss.H;
            actor.Facing = -1;
            actor.Dir = -1;
            actor.Intro = 0;
            sim.Bosses.push_back(actor);
            for (int frame = 0; frame < 480; ++frame)
            {
                ctl = {};
                ctl.Ap = frame % 24 == 0;
                ctl.Jp = frame % 120 == 60;
                ctl.Jn = frame % 120 >= 60 && frame % 120 < 75;
                ctl.R = frame % 180 > 100 && frame % 180 < 130;
                ctl.L = frame % 180 >= 130 && frame % 180 < 150;
                sim.Tick(1.f / 60);
                sim.Fx.clear();
                const auto &b = sim.Bosses.back();
                std::cout << "BOSS," << hero << "," << boss.Id << "," << frame << "," << player.X << "," << player.Y
                          << "," << player.Hp << "," << b.X << "," << b.Y << "," << b.Hp << "," << (b.Dead ? 1 : 0)
                          << "," << sim.Projectiles.size() << "," << sim.Score << "," << sim.Souls << "," << b.Id << ","
                          << b.State << "," << sim.State << "\n";
            }
        }
}
