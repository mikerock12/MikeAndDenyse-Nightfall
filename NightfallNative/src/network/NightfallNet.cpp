#include "NightfallNet.h"

#include <algorithm>
#include <cstring>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace Nightfall::Net
{
    namespace
    {
        void EnsureSocketSubsystem()
        {
#ifdef _WIN32
            static bool s_Init = false;
            if (!s_Init)
            {
                WSADATA wsa{};
                WSAStartup(MAKEWORD(2, 2), &wsa);
                s_Init = true;
            }
#endif
        }

        void CloseSock(intptr_t& s)
        {
            if (s < 0) return;
#ifdef _WIN32
            closesocket(static_cast<SOCKET>(s));
#else
            close(static_cast<int>(s));
#endif
            s = -1;
        }

        intptr_t OpenNonBlockingUdp(uint16_t bindPort, bool broadcast)
        {
            EnsureSocketSubsystem();
#ifdef _WIN32
            SOCKET raw = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (raw == INVALID_SOCKET) return -1;
            intptr_t s = static_cast<intptr_t>(raw);
#else
            intptr_t s = static_cast<intptr_t>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
            if (s < 0) return -1;
#endif
            int reuse = 1;
            setsockopt(
#ifdef _WIN32
                static_cast<SOCKET>(s),
#else
                static_cast<int>(s),
#endif
                SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

            if (broadcast)
            {
                int bc = 1;
                setsockopt(
#ifdef _WIN32
                    static_cast<SOCKET>(s),
#else
                    static_cast<int>(s),
#endif
                    SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&bc), sizeof(bc));
            }

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            addr.sin_port = htons(bindPort);
            if (bind(
#ifdef _WIN32
                    static_cast<SOCKET>(s),
#else
                    static_cast<int>(s),
#endif
                    reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
            {
                CloseSock(s);
                return -1;
            }

#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(static_cast<SOCKET>(s), FIONBIO, &mode);
#else
            int flags = fcntl(static_cast<int>(s), F_GETFL, 0);
            fcntl(static_cast<int>(s), F_SETFL, flags | O_NONBLOCK);
#endif
            return s;
        }

        std::string ExtractJsonField(const std::string& json, const std::string& key)
        {
            std::string pat = "\"" + key + "\":";
            size_t pos = json.find(pat);
            if (pos == std::string::npos) return "";
            pos += pat.size();
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
            if (pos >= json.size()) return "";
            if (json[pos] == '"')
            {
                ++pos;
                size_t end = json.find('"', pos);
                return end != std::string::npos ? json.substr(pos, end - pos) : "";
            }
            size_t end = pos;
            while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-')) ++end;
            return json.substr(pos, end - pos);
        }
    }

    LanSession::LanSession() = default;

    LanSession::~LanSession()
    {
        Stop();
    }

    std::string LanSession::BuildDiscoveryPacket(const std::string& id, const std::string& name, const std::string& host, int port)
    {
        return "NIGHTFALL|{\"id\":\"" + id + "\",\"name\":\"" + name + "\",\"host\":\"" + host + "\",\"port\":" + std::to_string(port) + ",\"kind\":\"lan\"}";
    }

    bool LanSession::ParseDiscoveryPacket(const std::string& pkt, RoomInfo& outRoom)
    {
        if (pkt.rfind("NIGHTFALL|", 0) != 0) return false;
        std::string json = pkt.substr(10);
        outRoom.id = ExtractJsonField(json, "id");
        outRoom.name = ExtractJsonField(json, "name");
        outRoom.host = ExtractJsonField(json, "host");
        outRoom.kind = ExtractJsonField(json, "kind");
        std::string portStr = ExtractJsonField(json, "port");
        outRoom.port = !portStr.empty() ? std::atoi(portStr.c_str()) : DefaultGamePort;
        return !outRoom.id.empty();
    }

    bool LanSession::StartHost(const std::string& roomName, int gamePort, int advertisePort)
    {
        Stop();
        _roomName = roomName.empty() ? "Nightfall" : roomName;
        _gamePort = gamePort;
        _advPort = advertisePort;
        _advHost = LanIp::Pick();
        _advSock = OpenNonBlockingUdp(static_cast<uint16_t>(_advPort), true);
        _gameSock = OpenNonBlockingUdp(static_cast<uint16_t>(_gamePort), false);
        _hosting = (_gameSock >= 0);
        return _hosting;
    }

    bool LanSession::StartClientListen(int advertisePort)
    {
        Stop();
        _advPort = advertisePort;
        _advSock = OpenNonBlockingUdp(static_cast<uint16_t>(_advPort), true);
        return _advSock >= 0;
    }

    bool LanSession::ConnectToHost(const std::string& hostIp, int gamePort, const std::string& preferredHero)
    {
        CloseSock(_gameSock);
        _gameSock = OpenNonBlockingUdp(0, false);
        if (_gameSock < 0) return false;

        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_port = htons(static_cast<uint16_t>(gamePort));
        if (inet_pton(AF_INET, hostIp.c_str(), &dst.sin_addr) != 1)
        {
            CloseSock(_gameSock);
            return false;
        }
        _peerAddr = dst.sin_addr.s_addr;
        _peerPort = dst.sin_port;
        _hasPeer = true;
        _clientConnected = true;

        std::string msg = "NF_HERO|" + (preferredHero.empty() ? std::string("denyse") : preferredHero);
        sendto(
#ifdef _WIN32
            static_cast<SOCKET>(_gameSock),
#else
            static_cast<int>(_gameSock),
#endif
            msg.data(), static_cast<int>(msg.size()), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
        return true;
    }

    void LanSession::Stop()
    {
        CloseSock(_advSock);
        CloseSock(_gameSock);
        _hosting = false;
        _clientConnected = false;
        _hasPeer = false;
        _hasClientInput = false;
        _rooms.clear();
    }

    void LanSession::TickDiscovery(float nowSeconds)
    {
        if (_advSock < 0) return;

        if (_hosting && (nowSeconds - _lastAdvertiseTime) > 0.9f)
        {
            _lastAdvertiseTime = nowSeconds;
            std::string pkt = BuildDiscoveryPacket(_deviceId, _roomName, _advHost, _gamePort);
            sockaddr_in bcast{};
            bcast.sin_family = AF_INET;
            bcast.sin_port = htons(static_cast<uint16_t>(_advPort));
            bcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            sendto(
#ifdef _WIN32
                static_cast<SOCKET>(_advSock),
#else
                static_cast<int>(_advSock),
#endif
                pkt.data(), static_cast<int>(pkt.size()), 0, reinterpret_cast<const sockaddr*>(&bcast), sizeof(bcast));
        }

        char buf[1024];
        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        while (true)
        {
            int n = recvfrom(
#ifdef _WIN32
                static_cast<SOCKET>(_advSock),
#else
                static_cast<int>(_advSock),
#endif
                buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n <= 0) break;
            buf[n] = '\0';
            RoomInfo info;
            if (ParseDiscoveryPacket(std::string(buf, static_cast<size_t>(n)), info))
            {
                if (info.id == _deviceId && _hosting) continue;
                info.lastSeen = nowSeconds;
                auto it = std::find_if(_rooms.begin(), _rooms.end(), [&info](const RoomInfo& r) { return r.id == info.id; });
                if (it != _rooms.end()) *it = info;
                else _rooms.push_back(info);
            }
        }
        _rooms.erase(std::remove_if(_rooms.begin(), _rooms.end(), [nowSeconds](const RoomInfo& r) {
            return (nowSeconds - r.lastSeen) > 8.0f;
        }), _rooms.end());
    }

    void LanSession::BroadcastSnapshot(const FrameSnap& snap)
    {
        if (!_hosting || _gameSock < 0 || !_hasPeer) return;
        std::vector<uint8_t> payload = snap.Serialize();
        std::vector<uint8_t> pkt;
        pkt.reserve(4 + payload.size());
        pkt.push_back('S'); pkt.push_back('N'); pkt.push_back('P'); pkt.push_back('2');
        pkt.insert(pkt.end(), payload.begin(), payload.end());

        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = _peerAddr;
        dst.sin_port = _peerPort;
        sendto(
#ifdef _WIN32
            static_cast<SOCKET>(_gameSock),
#else
            static_cast<int>(_gameSock),
#endif
            reinterpret_cast<const char*>(pkt.data()), static_cast<int>(pkt.size()), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
    }

    void LanSession::SendStartMatch(int world, const std::string& hostHero, const std::string& clientHero)
    {
        if (!_hosting || _gameSock < 0 || !_hasPeer) return;
        std::string msg = "NF_START|" + std::to_string(world) + "|" + hostHero + "|" + clientHero;
        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = _peerAddr;
        dst.sin_port = _peerPort;
        sendto(
#ifdef _WIN32
            static_cast<SOCKET>(_gameSock),
#else
            static_cast<int>(_gameSock),
#endif
            msg.data(), static_cast<int>(msg.size()), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
    }

    void LanSession::SendClientInput(const InputMsg& inp)
    {
        if (!_clientConnected || _gameSock < 0 || !_hasPeer) return;
        uint8_t pkt[5];
        pkt[0] = 'I'; pkt[1] = 'N'; pkt[2] = 'P'; pkt[3] = '1';
        pkt[4] = (inp.L ? 1 : 0) | (inp.R ? 2 : 0) | (inp.D ? 4 : 0) | (inp.Jp ? 8 : 0) | (inp.Jn ? 16 : 0) | (inp.Ap ? 32 : 0);

        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = _peerAddr;
        dst.sin_port = _peerPort;
        sendto(
#ifdef _WIN32
            static_cast<SOCKET>(_gameSock),
#else
            static_cast<int>(_gameSock),
#endif
            reinterpret_cast<const char*>(pkt), sizeof(pkt), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
    }

    bool LanSession::PollHostInput(InputMsg& outInput, std::string& outClientHero)
    {
        if (!_hosting || _gameSock < 0) return false;
        uint8_t buf[1024];
        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        while (true)
        {
            int n = recvfrom(
#ifdef _WIN32
                static_cast<SOCKET>(_gameSock),
#else
                static_cast<int>(_gameSock),
#endif
                reinterpret_cast<char*>(buf), sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n <= 0) break;
            _peerAddr = from.sin_addr.s_addr;
            _peerPort = from.sin_port;
            _hasPeer = true;
            if (n == 5 && buf[0] == 'I' && buf[1] == 'N' && buf[2] == 'P' && buf[3] == '1')
            {
                uint8_t m = buf[4];
                bool stickyJp = _latestClientInput.Jp || ((m & 8) != 0);
                bool stickyAp = _latestClientInput.Ap || ((m & 32) != 0);
                _latestClientInput.L = (m & 1) != 0;
                _latestClientInput.R = (m & 2) != 0;
                _latestClientInput.D = (m & 4) != 0;
                _latestClientInput.Jp = stickyJp;
                _latestClientInput.Jn = (m & 16) != 0;
                _latestClientInput.Ap = stickyAp;
                _hasClientInput = true;
            }
            else if (n > 8 && std::memcmp(buf, "NF_HERO|", 8) == 0)
            {
                _clientHero.assign(reinterpret_cast<const char*>(buf + 8), static_cast<size_t>(n - 8));
            }
        }
        outClientHero = _clientHero;
        if (!_hasClientInput) return false;
        outInput = _latestClientInput;
        _latestClientInput.Jp = false;
        _latestClientInput.Ap = false;
        return true;
    }

    bool LanSession::PollClientSnapshot(FrameSnap& outSnap, int& outStartWorld, std::string& outHostHero, std::string& outClientHero, bool& outMatchStarted)
    {
        outMatchStarted = false;
        if (!_clientConnected || _gameSock < 0) return false;
        uint8_t buf[2048];
        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        bool gotSnap = false;
        while (true)
        {
            int n = recvfrom(
#ifdef _WIN32
                static_cast<SOCKET>(_gameSock),
#else
                static_cast<int>(_gameSock),
#endif
                reinterpret_cast<char*>(buf), sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n <= 4) break;
            if (buf[0] == 'S' && buf[1] == 'N' && buf[2] == 'P' && buf[3] == '2')
            {
                if (FrameSnap::Deserialize(buf + 4, static_cast<size_t>(n - 4), outSnap))
                {
                    gotSnap = true;
                }
            }
            else if (n > 9 && std::memcmp(buf, "NF_START|", 9) == 0)
            {
                std::string s(reinterpret_cast<const char*>(buf + 9), static_cast<size_t>(n - 9));
                size_t p1 = s.find('|');
                size_t p2 = p1 != std::string::npos ? s.find('|', p1 + 1) : std::string::npos;
                if (p1 != std::string::npos && p2 != std::string::npos)
                {
                    outStartWorld = std::atoi(s.substr(0, p1).c_str());
                    outHostHero = s.substr(p1 + 1, p2 - p1 - 1);
                    outClientHero = s.substr(p2 + 1);
                    outMatchStarted = true;
                }
            }
        }
        return gotSnap;
    }
}
