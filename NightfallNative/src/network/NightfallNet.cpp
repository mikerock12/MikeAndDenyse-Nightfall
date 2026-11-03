#include "NightfallNet.h"

#include <algorithm>
#include <cstring>
#include <chrono>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

namespace Nightfall
{
    std::string LanIp::Pick()
    {
        std::string best;
        int bestScore = 0;
#ifdef _WIN32
        ULONG size = 16384;
        std::vector<unsigned char> buffer(size);
        auto adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        ULONG result = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, adapters, &size);
        if (result == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(size); adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
            result = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, adapters, &size);
        }
        if (result == NO_ERROR) for (auto adapter = adapters; adapter; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            std::string description;
            if (adapter->Description) for (const wchar_t* c=adapter->Description; *c; ++c) description += *c < 128 ? static_cast<char>(*c) : '?';
            int ifaceScore = ScoreIface(description);
            if (ifaceScore < 1) continue;
            for (auto address=adapter->FirstUnicastAddress; address; address=address->Next) {
                char text[INET_ADDRSTRLEN]{};
                auto in=reinterpret_cast<sockaddr_in*>(address->Address.lpSockaddr);
                if (!in || in->sin_family != AF_INET || !inet_ntop(AF_INET,&in->sin_addr,text,sizeof(text))) continue;
                int score=ScoreAddress(text);
                if (score < 10) continue;
                score += ifaceScore + (adapter->IfType == IF_TYPE_IEEE80211 ? 20 : 0);
                if (score>bestScore) { bestScore=score; best=text; }
            }
        }
#else
        ifaddrs* interfaces=nullptr;
        if (getifaddrs(&interfaces)==0) {
            for(auto item=interfaces;item;item=item->ifa_next) {
                if(!item->ifa_addr || item->ifa_addr->sa_family!=AF_INET || !(item->ifa_flags&IFF_UP) || (item->ifa_flags&IFF_LOOPBACK)) continue;
                int ifaceScore=ScoreIface(item->ifa_name);
                if(ifaceScore<1) continue;
                char text[INET_ADDRSTRLEN]{};
                auto in=reinterpret_cast<sockaddr_in*>(item->ifa_addr);
                if(!inet_ntop(AF_INET,&in->sin_addr,text,sizeof(text))) continue;
                int score=ScoreAddress(text);
                if(score<10) continue;
                score+=ifaceScore;
                if(score>bestScore) { bestScore=score; best=text; }
            }
            freeifaddrs(interfaces);
        }
#endif
        return best;
    }
}

namespace Nightfall::Net
{
    namespace
    {
        double Now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
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

    LanSession::LanSession()
    {
        std::ostringstream id;
        id << "nf-" << std::hex << std::chrono::steady_clock::now().time_since_epoch().count() << reinterpret_cast<uintptr_t>(this);
        _deviceId=id.str();
    }

    LanSession::~LanSession()
    {
        Stop();
    }

    std::string LanSession::BuildDiscoveryPacket(const std::string& id, const std::string& name, const std::string& host, int port)
    {
        return "NIGHTFALL|{\"id\":\"" + id + "\",\"name\":\"" + name + "\",\"host\":\"" + host + "\",\"port\":" + std::to_string(port) + ",\"kind\":\"lan\",\"nativeProtocol\":3}";
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
        return !outRoom.id.empty() && !outRoom.name.empty() && outRoom.port > 0 && outRoom.port <= 65535 && outRoom.kind == "lan" && ExtractJsonField(json,"nativeProtocol") == "3";
    }

    bool LanSession::StartHost(const std::string& roomName, int gamePort, int advertisePort)
    {
        Stop();
        _roomName = roomName.empty() ? "Nightfall" : roomName;
        _gamePort = gamePort;
        _advPort = advertisePort;
        _lastAdvertiseTime = -10.0f;
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
        _lastReceived = Now();
        _receivedEpoch = 0;
        _preferredHero = Catalog::Hero(preferredHero) ? preferredHero : "denyse";

        std::string msg = "NF_HERO3|" + (preferredHero.empty() ? std::string("denyse") : preferredHero);
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
        if (_hasPeer && _gameSock >= 0) {
            sockaddr_in destination{}; destination.sin_family=AF_INET; destination.sin_addr.s_addr=_peerAddr; destination.sin_port=_peerPort;
#ifdef _WIN32
            sendto(static_cast<SOCKET>(_gameSock), "NF_END3", 7, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
#else
            sendto(static_cast<int>(_gameSock), "NF_END3", 7, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
#endif
        }
        CloseSock(_advSock);
        CloseSock(_gameSock);
        _hosting = false;
        _clientConnected = false;
        _hasPeer = false;
        _hasClientInput = false;
        _matchStarted = false;
        _receivedEpoch = 0;
        _latestClientInput = {};
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
                if (!LanIp::IsUsable(info.host)) {
                    char sender[INET_ADDRSTRLEN]{};
                    if (inet_ntop(AF_INET, &from.sin_addr, sender, sizeof(sender))) info.host=sender;
                }
                if (!LanIp::IsUsable(info.host)) continue;
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
        TransmitStart();
        std::vector<uint8_t> payload = snap.Serialize();
        std::vector<uint8_t> pkt;
        pkt.reserve(12 + payload.size());
        pkt.push_back('S'); pkt.push_back('N'); pkt.push_back('P'); pkt.push_back('3');
        ++_snapshotSequence;
        for(uint32_t value:{_matchEpoch,_snapshotSequence})
            for(int shift=0;shift<32;shift+=8)pkt.push_back(static_cast<uint8_t>(value>>shift));
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
        _matchWorld=world; _hostHero=hostHero; _matchClientHero=clientHero;
        _matchStarted=true; ++_matchEpoch; _snapshotSequence=0;
        TransmitStart();
    }

    void LanSession::TransmitStart()
    {
        if (!_matchStarted) return;
        if (!_hosting || _gameSock < 0 || !_hasPeer) return;
        std::string msg = "NF_START3|" + std::to_string(_matchWorld) + "|" + _hostHero + "|" + _matchClientHero + "|" + std::to_string(_matchEpoch);
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
        pkt[0] = 'I'; pkt[1] = 'N'; pkt[2] = 'P'; pkt[3] = '3';
        pkt[4] = (inp.L ? 1 : 0) | (inp.R ? 2 : 0) | (inp.D ? 4 : 0) | (inp.Jp ? 8 : 0) | (inp.Jn ? 16 : 0) | (inp.Ap ? 32 : 0);

        sockaddr_in dst{};
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = _peerAddr;
        dst.sin_port = _peerPort;
        if (_receivedEpoch == 0) {
            std::string hello="NF_HERO3|"+_preferredHero;
#ifdef _WIN32
            sendto(static_cast<SOCKET>(_gameSock), hello.data(), static_cast<int>(hello.size()), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
#else
            sendto(static_cast<int>(_gameSock), hello.data(), static_cast<int>(hello.size()), 0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
#endif
        }
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
            bool isInput=n==5 && std::memcmp(buf,"INP3",4)==0;
            bool isHero=n>9 && std::memcmp(buf,"NF_HERO3|",9)==0;
            bool isEnd=n==7 && std::memcmp(buf,"NF_END3",7)==0;
            if (!isInput && !isHero && !isEnd) continue;
            if (_hasPeer && (from.sin_addr.s_addr != _peerAddr || from.sin_port != _peerPort)) continue;
            if (isEnd) { _hasPeer=false; _latestClientInput={}; _hasClientInput=false; continue; }
            _peerAddr = from.sin_addr.s_addr;
            _peerPort = from.sin_port;
            _hasPeer = true;
            if (n == 5 && buf[0] == 'I' && buf[1] == 'N' && buf[2] == 'P' && buf[3] == '3')
            {
                _lastInputReceived = Now();
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
            else if (n > 9 && std::memcmp(buf, "NF_HERO3|", 9) == 0)
            {
                std::string hero(reinterpret_cast<const char*>(buf + 9), static_cast<size_t>(n - 9));
                if (Catalog::Hero(hero)) _clientHero=hero;
                TransmitStart();
            }
        }
        outClientHero = _clientHero;
        if (Now()-_lastInputReceived>0.75) _latestClientInput={};
        if (!_hasClientInput) return false;
        outInput = _latestClientInput;
        _latestClientInput.Jp = false;
        _latestClientInput.Ap = false;
        return true;
    }

    bool LanSession::DecodeSnapshotPacket(const uint8_t* data,size_t length,uint32_t generation,uint32_t& lastSequence,FrameSnap& snapshot)
    {
        if(!data || length<13 || std::memcmp(data,"SNP3",4)!=0)return false;
        auto read32=[](const uint8_t* bytes){return static_cast<uint32_t>(bytes[0])|(static_cast<uint32_t>(bytes[1])<<8)|(static_cast<uint32_t>(bytes[2])<<16)|(static_cast<uint32_t>(bytes[3])<<24);};
        uint32_t packetGeneration=read32(data+4),sequence=read32(data+8);
        if(packetGeneration!=generation || sequence<=lastSequence)return false;
        if(!FrameSnap::Deserialize(data+12,length-12,snapshot))return false;
        lastSequence=sequence;
        return true;
    }

    bool LanSession::PollClientSnapshot(FrameSnap& outSnap, int& outStartWorld, std::string& outHostHero, std::string& outClientHero, bool& outMatchStarted)
    {
        outMatchStarted = false;
        if (!_clientConnected || _gameSock < 0) return false;
        if(Now()-_lastReceived>8.0) { _clientConnected=false; _hasPeer=false; return false; }
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
            if (from.sin_addr.s_addr != _peerAddr || from.sin_port != _peerPort) continue;
            _lastReceived=Now();
            if(n==7 && std::memcmp(buf,"NF_END3",7)==0) { _clientConnected=false; return false; }
            if (buf[0] == 'S' && buf[1] == 'N' && buf[2] == 'P' && buf[3] == '3')
            {
                if (DecodeSnapshotPacket(buf,static_cast<size_t>(n),_receivedEpoch,_receivedSequence,outSnap))
                {
                    gotSnap = true;
                }
            }
            else if (n > 10 && std::memcmp(buf, "NF_START3|", 10) == 0)
            {
                std::string s(reinterpret_cast<const char*>(buf + 10), static_cast<size_t>(n - 10));
                size_t p1 = s.find('|');
                size_t p2 = p1 != std::string::npos ? s.find('|', p1 + 1) : std::string::npos;
                if (p1 != std::string::npos && p2 != std::string::npos)
                {
                    int world = std::atoi(s.substr(0, p1).c_str());
                    size_t p3=s.find('|',p2+1);
                    uint32_t epoch=p3==std::string::npos ? 1u : static_cast<uint32_t>(std::strtoul(s.substr(p3+1).c_str(),nullptr,10));
                    if(world<0 || world>=16 || epoch==0 || epoch<=_receivedEpoch) continue;
                    outStartWorld = world;
                    outHostHero = s.substr(p1 + 1, p2 - p1 - 1);
                    outClientHero = s.substr(p2 + 1, p3==std::string::npos ? std::string::npos : p3-p2-1);
                    if(!Catalog::Hero(outHostHero) || !Catalog::Hero(outClientHero)) continue;
                    _receivedEpoch=epoch;
                    _receivedSequence=0;
                    outMatchStarted = true;
                }
            }
        }
        return gotSnap;
    }
}
