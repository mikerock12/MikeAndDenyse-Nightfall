#include "NightfallCore.h"
#include "NightfallNet.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    using namespace Nightfall;

    class Bot
    {
    public:
        Ctl CurrentCtl{};

        void Think(GameSim& sim, float dt)
        {
            const auto& p = sim.Players[0];
            _jumpCd -= dt;
            _atkCd -= dt;

            int dir = 1;
            if (sim.LockedArena)
            {
                const BossA* boss = nullptr;
                for (const auto& b : sim.Bosses) if (!b.Dead) boss = &b;
                float goal = boss ? (boss->X + boss->W / 2.0f) : sim.Level.Exit.x;
                dir = goal > (p.X + p.W / 2.0f) ? 1 : -1;
            }

            float side = dir > 0 ? p.X + p.W + 6.0f : p.X - 6.0f;
            float probe1 = dir > 0 ? p.X + p.W + 24.0f : p.X - 24.0f;
            float probe2 = dir > 0 ? p.X + p.W + 52.0f : p.X - 52.0f;
            bool wall = Blocks(sim.PeekTile(side, p.Y + p.H - 8.0f)) || Blocks(sim.PeekTile(side, p.Y + p.H * 0.5f));
            bool gap = !Stands(sim, probe1, p.Y + p.H + 8.0f) && !Stands(sim, probe2, p.Y + p.H + 8.0f);
            bool burn = IsHazard(sim.PeekTile(probe1, p.Y + p.H + 8.0f)) ||
                        IsHazard(sim.PeekTile(probe1, p.Y + p.H - 8.0f));

            if (std::abs(p.X - _lastX) < 0.35f) _stuck += dt;
            else _stuck = 0.0f;
            _lastX = p.X;

            bool want = (p.OnGround && (wall || gap || burn)) || _stuck > 0.45f;
            Ctl c{};
            c.R = dir > 0;
            c.L = dir < 0;
            if (want && _jumpCd <= 0.0f)
            {
                c.Jp = true;
                c.Jn = true;
                _jumpCd = 0.30f;
            }
            else if (p.Vy < 0.0f)
            {
                c.Jn = true;
            }
            if (_atkCd <= 0.0f)
            {
                c.Ap = true;
                _atkCd = 0.40f;
            }
            CurrentCtl = c;
        }

    private:
        float _jumpCd = 0.0f;
        float _atkCd = 0.0f;
        float _stuck = 0.0f;
        float _lastX = -1e9f;

        static bool Blocks(int t) { return t == T::Solid || t == T::Ice || t == T::Break; }
        static bool IsHazard(int t) { return t == T::Spike || t == T::Lava || t == T::Thorn; }
        static bool Stands(const GameSim& sim, float x, float y)
        {
            for (int k = 0; k < 3; ++k)
            {
                int t = sim.PeekTile(x, y + k * T::Tile);
                if (t == T::Solid || t == T::Ice || t == T::Break || t == T::Platform || t == T::Bounce) return true;
                if (IsHazard(t)) return false;
            }
            return false;
        }
    };

    void MustParseHex(const std::string& h)
    {
        uint8_t r = 0, g = 0, b = 0, a = 0;
        if (!GameSim::TryParseHexColor(h, r, g, b, a))
            throw std::runtime_error("bad hex " + h);
    }

    int CheckHex()
    {
        for (const auto& w : Catalog::Worlds())
        {
            MustParseHex(w.Tone);
            MustParseHex(w.Ground);
            MustParseHex(w.Lip);
            MustParseHex(w.Fog);
        }
        MustParseHex("#d4b46a");
        MustParseHex("8ec8dc");
        return 1;
    }

    int CheckLevels()
    {
        for (int i = 0; i < 16; ++i)
        {
            LevelData L = LevelBuilder::Compile(i);
            if (L.Cols < 40 || L.Rows != 20) throw std::runtime_error("size " + std::to_string(i));
            if (static_cast<int>(L.Tiles.size()) != L.Rows) throw std::runtime_error("tiles " + std::to_string(i));
            if (L.World.Boss.empty()) throw std::runtime_error("boss " + std::to_string(i));
            if (!Catalog::Boss(L.World.Boss)) throw std::runtime_error("boss def " + L.World.Boss);
            if (L.Ents.empty()) throw std::runtime_error("no ents " + std::to_string(i));
        }
        return 16;
    }

    int CheckProgress()
    {
        Progress::ResetAll();
        if (Progress::Unlocked != 1) throw std::runtime_error("start unlock");
        if (!Progress::CanPlay(0)) throw std::runtime_error("world0");
        if (Progress::CanPlay(1)) throw std::runtime_error("world1 locked");
        Progress::OnClear(0);
        if (Progress::Unlocked != 2) throw std::runtime_error("unlock 2");
        if (!Progress::CanPlay(1)) throw std::runtime_error("world1 open");
        Progress::OnClear(0);
        if (Progress::Unlocked != 2) throw std::runtime_error("replay no skip");
        for (int i = 1; i < 15; ++i) Progress::OnClear(i);
        if (Progress::Unlocked != 16) throw std::runtime_error("all " + std::to_string(Progress::Unlocked));
        Progress::OnClear(15);
        if (Progress::Unlocked != 16) throw std::runtime_error("cap");
        Progress::ResetAll();
        return 1;
    }

    int CheckLanIp()
    {
        if (LanIp::ScoreAddress("127.0.0.1") >= 0) throw std::runtime_error("loopback");
        if (LanIp::ScoreAddress("0.0.0.0") >= 0) throw std::runtime_error("zero");
        if (LanIp::ScoreAddress("169.254.1.1") >= 10) throw std::runtime_error("apipa");
        if (LanIp::ScoreAddress("192.168.0.14") < 30) throw std::runtime_error("lan");
        if (LanIp::IsUsable("127.0.0.1")) throw std::runtime_error("usable loop");
        if (!LanIp::IsUsable("192.168.1.5")) throw std::runtime_error("usable lan");
        if (LanIp::IsUsable("")) throw std::runtime_error("empty");
        return 1;
    }

    int CheckCatalog()
    {
        if (Catalog::Worlds().size() != 16) throw std::runtime_error("worlds");
        if (Catalog::Enemies().size() != 32) throw std::runtime_error("enemies");
        if (Catalog::Bosses().size() != 16) throw std::runtime_error("bosses");
        if (Catalog::Belial().Id != "belial") throw std::runtime_error("belial");
        if (!Catalog::Hero("mike") || !Catalog::Hero("denyse")) throw std::runtime_error("heroes");
        return 1;
    }

    int CheckTraversal()
    {
        for (int i = 0; i < 16; ++i)
        {
            LevelData L = LevelBuilder::Compile(i);
            std::string why;
            if (!LevelBuilder::Walkable(L, why))
                throw std::runtime_error("fase " + std::to_string(i + 1) + " intransponivel: " + why);
        }
        return 16;
    }

    int CheckNoTrap()
    {
        for (int i = 0; i < 16; ++i)
        {
            LevelData L = LevelBuilder::Compile(i);
            auto reach = LevelBuilder::Reach(L);
            for (int x = 0; x < L.Cols; ++x)
            {
                for (int y = 1; y < L.Rows - 1; ++y)
                {
                    if (!LevelBuilder::Walk(L.Tiles[y][x]) || LevelBuilder::Walk(L.Tiles[y - 1][x])) continue;
                    if (reach.count(x * 64 + y) > 0) continue;
                    if (x * T::Tile >= L.ArenaX0 - T::Tile && x * T::Tile <= L.ArenaX1 + T::Tile) continue;
                    if (y <= 15) continue;
                    int above = L.Tiles[y - 1][x];
                    if (above != T::Spike && above != T::Lava && above != T::Thorn)
                        throw std::runtime_error("fase " + std::to_string(i + 1) + " tem poco sem saida em " + std::to_string(x) + "," + std::to_string(y));
                }
            }
        }
        return 16;
    }

    int CheckSnapshot()
    {
        FrameSnap wireFixture;
        wireFixture.P0.X=1.0f; wireFixture.P0.Hp=6; wireFixture.Score=0x01020304;
        auto wire=wireFixture.Serialize();
        if(wire.size()!=92 || wire[0]!=3 || wire[1]!=0 || wire[2]!=0 || wire[3]!=0x80 || wire[4]!=0x3f || wire[9]!=6 || wire[31]!=4 || wire[32]!=3 || wire[33]!=2 || wire[34]!=1)
            throw std::runtime_error("wire native v3 depende de ABI/padding ou endian incorreto");
        FrameSnap wireRead;
        if(!FrameSnap::Deserialize(wire.data(),wire.size(),wireRead) || wireRead.Serialize()!=wire)
            throw std::runtime_error("wire v3 nao faz roundtrip estavel byte a byte");
        auto invalidWire=wire; invalidWire.push_back(0);
        if(FrameSnap::Deserialize(invalidWire.data(),invalidWire.size(),wireRead)) throw std::runtime_error("wire aceitou lixo final");
        invalidWire=wire; invalidWire[0]=2;
        if(FrameSnap::Deserialize(invalidWire.data(),invalidWire.size(),wireRead)) throw std::runtime_error("wire aceitou protocolo antigo");
        invalidWire=wire; invalidWire[1]=0;invalidWire[2]=0;invalidWire[3]=0xc0;invalidWire[4]=0x7f;
        if(FrameSnap::Deserialize(invalidWire.data(),invalidWire.size(),wireRead)) throw std::runtime_error("wire aceitou posicao NaN");
        std::vector<uint8_t> packet={'S','N','P','3',1,0,0,0,2,0,0,0};
        packet.insert(packet.end(),wire.begin(),wire.end());uint32_t sequence=0;
        if(!Net::LanSession::DecodeSnapshotPacket(packet.data(),packet.size(),1,sequence,wireRead) || sequence!=2)
            throw std::runtime_error("envelope snapshot v3 invalido");
        if(Net::LanSession::DecodeSnapshotPacket(packet.data(),packet.size(),1,sequence,wireRead)) throw std::runtime_error("snapshot duplicado aceito");
        packet[8]=1;
        if(Net::LanSession::DecodeSnapshotPacket(packet.data(),packet.size(),1,sequence,wireRead)) throw std::runtime_error("snapshot fora de ordem aceito");
        packet[8]=3;packet[4]=0;
        if(Net::LanSession::DecodeSnapshotPacket(packet.data(),packet.size(),1,sequence,wireRead)) throw std::runtime_error("snapshot de outra partida aceito");
        if (GameSim::ProjKindId("magic") != 0) throw std::runtime_error("proj magic");
        if (GameSim::ProjKindName(GameSim::ProjKindId("hell")) != "hell") throw std::runtime_error("proj hell");
        if (GameSim::ProjKindName(GameSim::ProjKindId("ice")) != "ice") throw std::runtime_error("proj ice");

        GameSim sim;
        sim.ReadCtl = [](int) { return Ctl{}; };
        sim.Start(0, "mike", "denyse", true);
        if (sim.Players.size() != 2) throw std::runtime_error("dois herois");
        if (sim.Items.empty()) throw std::runtime_error("sem itens");
        uint64_t m = sim.ItemMask();
        if (m == 0) throw std::runtime_error("mascara vazia");
        sim.Items[0].Taken = true;
        if (sim.ItemMask() == m) throw std::runtime_error("mascara nao muda");
        sim.ApplyNetItems(m);
        if (sim.Items[0].Taken) throw std::runtime_error("mascara nao restaura");

        if (!Catalog::EnemyAt(Catalog::EnemyIndex("wraith")) || Catalog::EnemyAt(Catalog::EnemyIndex("wraith"))->Id != "wraith")
            throw std::runtime_error("enemy index");
        if (!Catalog::BossAt(Catalog::BossIndex("belial")) || Catalog::BossAt(Catalog::BossIndex("belial"))->Id != "belial")
            throw std::runtime_error("belial index");
        if (!Catalog::BossAt(Catalog::BossIndex("treant")) || Catalog::BossAt(Catalog::BossIndex("treant"))->Id != "treant")
            throw std::runtime_error("treant index");

        FrameSnap snap = sim.CaptureSnapshot();
        auto bytes = snap.Serialize();
        FrameSnap decoded;
        if (!FrameSnap::Deserialize(bytes.data(), bytes.size(), decoded))
            throw std::runtime_error("falha roundtrip FrameSnap");
        FrameSnap truncated;
        if (FrameSnap::Deserialize(bytes.data(), bytes.size()-1, truncated))
            throw std::runtime_error("snapshot truncado foi aceito");

        // Exercise the receiving simulation, not only serialization: the old native
        // client ignored boss activation, hero identity, attack timing and stale enemies.
        GameSim hostState;
        hostState.Start(15,"denyse","mike",true);
        hostState.Players[0].X=4000; hostState.Players[0].Atk=0.12f;
        hostState.Players[1].X=4050;
        hostState.LockedArena=true; hostState.Items[0].Taken=true;
        BossA belial;
        belial.Def=Catalog::Belial(); belial.Id="belial"; belial.Hp=17; belial.MaxHp=Catalog::Belial().Hp;
        belial.X=4200; belial.Y=360; belial.Facing=-1;
        hostState.Bosses.push_back(belial);
        hostState.Enemies.resize(1);
        auto remoteSnap=hostState.CaptureSnapshot();
        GameSim remote;
        remote.Start(15,"mike","denyse",true);
        remote.ApplySnapshot(remoteSnap);
        remote.TickRemote(1.0f/60.0f);
        if(remote.Players[0].Hero.Id!="denyse" || remote.Players[1].Hero.Id!="mike" || remote.Players[0].Atk<=0)
            throw std::runtime_error("identidade e golpe do heroi nao aplicados no cliente");
        if(remote.Bosses.size()!=1 || remote.Bosses[0].Id!="belial" || remote.Bosses[0].Hp!=17 || remote.Bosses[0].Facing!=-1 || !remote.BelialPhase)
            throw std::runtime_error("Belial nao reconstruido no cliente");
        if(remote.Enemies.size()!=remoteSnap.Ents.size() || !remote.Items[0].Taken || remote.CamX<=0)
            throw std::runtime_error("estado remoto deixa inimigos fantasmas, item ou camera incorreta");

        for (int i = 0; i < 600; ++i) sim.Tick(1.0f / 60.0f);
        if (sim.Players[0].Dead) throw std::runtime_error("heroi morreu parado no spawn");
        if (sim.State != "play") throw std::runtime_error("estado " + sim.State + " parado no spawn");
        return 3;
    }

    int CheckPlatforms()
    {
        for (float dt : {1.0f / 60.0f, 1.0f / 30.0f, 1.0f / 20.0f})
        {
            GameSim sim;
            sim.ReadCtl = [](int) { return Ctl{}; };
            sim.Start(0, "mike", "denyse", false);
            auto& p = sim.Players[0];
            int px = -1, py = -1;
            for (int x = 4; x < sim.Level.Cols && px < 0; ++x)
            {
                for (int y = 1; y < sim.Level.Rows; ++y)
                {
                    if (sim.Level.Tiles[y][x] == T::Platform && sim.Level.Tiles[y - 1][x] == T::Empty)
                    {
                        px = x; py = y; break;
                    }
                }
            }
            if (px < 0) throw std::runtime_error("nenhuma plataforma na fase 1");

            p.X = px * T::Tile + 5.0f;
            p.Y = py * T::Tile - p.H - 150.0f;
            p.Vx = 0.0f; p.Vy = 0.0f; p.Inv = 99.0f;
            float floor = py * T::Tile - p.H;
            bool landed = false;
            for (int i = 0; i < 240 && !landed; ++i)
            {
                sim.Tick(dt);
                if (p.OnGround && std::abs(p.Y - floor) < 3.0f) landed = true;
                if (p.Y > floor + T::Tile * 2) break;
            }
            if (!landed)
                throw std::runtime_error("heroi atravessou a plataforma");
        }
        return 3;
    }

    int CheckPlayable()
    {
        for (int i = 0; i < 16; ++i)
        {
            for (int seed = 0; seed < 3; ++seed)
            {
                GameSim::InitRandomState(static_cast<uint32_t>(9001 + i * 31 + seed));
                GameSim sim;
                Bot bot;
                sim.ReadCtl = [&bot](int) { return bot.CurrentCtl; };
                sim.Start(i, "mike", "denyse", false);
                sim.Players[0].Lives = 999;

                constexpr float dt = 1.0f / 60.0f;
                float best = sim.Players[0].X;
                bool arrived = false;
                for (int f = 0; f < 60 * 200 && !arrived; ++f)
                {
                    bot.Think(sim, dt);
                    sim.Tick(dt);
                    const auto& p = sim.Players[0];
                    if (p.X > best) best = p.X;
                    if (sim.LockedArena) arrived = true;
                    if (sim.State != "play") break;
                }
                if (!arrived)
                {
                    throw std::runtime_error("fase " + std::to_string(i + 1) + " (" + Catalog::Worlds()[i].Name +
                        ") nao jogavel [seed " + std::to_string(seed) + "]: parou na coluna " +
                        std::to_string( static_cast<int>(std::lrint(best / T::Tile))) + " de " +
                        std::to_string( static_cast<int>(std::lrint(sim.Level.ArenaX0 / T::Tile))));
                }
            }
        }
        return 16;
    }

    int CheckFullLoop()
    {
        for (int i : {0, 7, 15})
        {
            GameSim::InitRandomState(static_cast<uint32_t>(4200 + i));
            GameSim sim;
            Bot bot;
            sim.ReadCtl = [&bot](int) { return bot.CurrentCtl; };
            sim.Start(i, "mike", "denyse", false);
            sim.Players[0].Lives = 999;

            constexpr float dt = 1.0f / 60.0f;
            bool done = false;
            for (int f = 0; f < 60 * 300 && !done; ++f)
            {
                bot.Think(sim, dt);
                sim.Tick(dt);
                if (sim.State == "clear" || sim.State == "win") done = true;
                if (sim.State == "dead") throw std::runtime_error("fase " + std::to_string(i + 1) + ": heroi morreu com 999 vidas");
            }
            if (!done)
            {
                std::string where = !sim.LockedArena ? "nao chegou na arena" : "nao fechou loop";
                throw std::runtime_error("fase " + std::to_string(i + 1) + " nao fecha: " + where);
            }
        }
        return 3;
    }

    void CheckLanLoopbackSession()
    {
        Net::RoomInfo parsed;
        std::string pkt = Net::LanSession::BuildDiscoveryPacket("dev-host", "Sala Nightfall", "192.168.1.3", 17777);
        if (!Net::LanSession::ParseDiscoveryPacket(pkt, parsed) || parsed.host != "192.168.1.3" || parsed.port != 17777)
            throw std::runtime_error("falha no parser de descoberta LAN");
        if(Net::LanSession::ParseDiscoveryPacket("NIGHTFALL|{\"id\":\"unity\",\"name\":\"Unity room\",\"host\":\"192.168.1.3\",\"port\":7777,\"kind\":\"lan\"}",parsed))
            throw std::runtime_error("cliente nativo aceitou sala Unity incompativel");

        Net::LanSession host;
        Net::LanSession client;
        if (!host.StartHost("CoopLoopback", 17777, 47889))
            throw std::runtime_error("falha ao abrir socket UDP host loopback");
        if (!client.ConnectToHost("127.0.0.1", 17777, "denyse"))
            throw std::runtime_error("falha ao conectar cliente UDP loopback");

        InputMsg sendInp{};
        sendInp.R = true;
        sendInp.Jp = true;
        client.SendClientInput(sendInp);

        InputMsg recvInp{};
        std::string hero;
        bool gotInput = false;
        for (int tries = 0; tries < 20 && !gotInput; ++tries)
        {
            gotInput = host.PollHostInput(recvInp, hero);
        }
        if (!gotInput || !recvInp.R || !recvInp.Jp || hero != "denyse")
            throw std::runtime_error("falha ao receber InputMsg/Hero do cliente via UDP loopback");

        GameSim hostSim;
        hostSim.Start(0, "mike", "denyse", true);
        hostSim.Score = 750;
        hostSim.Souls = 12;
        host.SendStartMatch(0, "mike", "denyse");
        host.BroadcastSnapshot(hostSim.CaptureSnapshot());

        FrameSnap clientSnap;
        int startWorld = -1;
        std::string hHero, cHero;
        bool started = false;
        bool gotSnap = false;
        for (int tries = 0; tries < 20 && !gotSnap; ++tries)
        {
            bool st = false;
            if (client.PollClientSnapshot(clientSnap, startWorld, hHero, cHero, st))
                gotSnap = true;
            if (st) started = true;
        }
        if (!gotSnap || clientSnap.Score != 750 || clientSnap.Souls != 12)
            throw std::runtime_error("falha ao receber FrameSnap v2 via UDP loopback");
        if(!started || startWorld!=0 || hHero!="mike" || cHero!="denyse")
            throw std::runtime_error("inicio de partida nao recebido");
        host.BroadcastSnapshot(hostSim.CaptureSnapshot());
        bool duplicate=false;
        client.PollClientSnapshot(clientSnap,startWorld,hHero,cHero,duplicate);
        if(duplicate) throw std::runtime_error("inicio retransmitido reinicia a partida");
        host.SendStartMatch(0,"mike","denyse");
        bool retry=false;
        client.PollClientSnapshot(clientSnap,startWorld,hHero,cHero,retry);
        if(!retry) throw std::runtime_error("retry na mesma fase nao reinicia cliente");
        host.Stop();
        bool ended=false;
        client.PollClientSnapshot(clientSnap,startWorld,hHero,cHero,ended);
        if(client.IsConnectedClient()) throw std::runtime_error("saida do host nao encerra cliente");
    }
}

int main()
{
    try
    {
        int n = 0;
        n += CheckHex();
        n += CheckLevels();
        n += CheckProgress();
        n += CheckLanIp();
        n += CheckCatalog();
        n += CheckTraversal();
        n += CheckNoTrap();
        n += CheckSnapshot();
        n += CheckPlatforms();
        n += CheckPlayable();
        n += CheckFullLoop();

        CheckLanLoopbackSession();

        // Benchmark 10,000 ticks headless
        GameSim benchSim;
        Bot benchBot;
        benchSim.ReadCtl = [&benchBot](int) { return benchBot.CurrentCtl; };
        benchSim.Start(0, "mike", "denyse", true);
        benchSim.Players[0].Lives = 999;
        benchSim.Players[1].Lives = 999;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; ++i)
        {
            benchBot.Think(benchSim, 1.0f / 60.0f);
            benchSim.Tick(1.0f / 60.0f);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << "SMOKE OK " << n << " checks + LAN UDP loopback OK\n";
        std::cout << "[BENCHMARK] 10,000 ticks headless (60 Hz) completed in " << ms << " ms (" << (ms / 10000.0) << " ms/tick)\n";
        return n == 77 ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "SMOKE FAIL: " << e.what() << "\n";
        return 2;
    }
}
