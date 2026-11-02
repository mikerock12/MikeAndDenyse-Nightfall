#pragma once

#include "NightfallCore.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Nightfall::Net
{
    struct RoomInfo
    {
        std::string id;
        std::string name;
        std::string host;
        std::string kind = "lan";
        int port = 7777;
        float lastSeen = 0.0f;
    };

    class LanSession
    {
    public:
        static constexpr int DefaultAdvertisePort = 47888;
        static constexpr int DefaultGamePort = 7777;

        LanSession();
        ~LanSession();

        bool StartHost(const std::string& roomName, int gamePort = DefaultGamePort, int advertisePort = DefaultAdvertisePort);
        bool StartClientListen(int advertisePort = DefaultAdvertisePort);
        bool ConnectToHost(const std::string& hostIp, int gamePort = DefaultGamePort, const std::string& preferredHero = "denyse");
        void Stop();

        void TickDiscovery(float nowSeconds);
        void BroadcastSnapshot(const FrameSnap& snap);
        bool PollHostInput(InputMsg& outInput, std::string& outClientHero);
        bool PollClientSnapshot(FrameSnap& outSnap, int& outStartWorld, std::string& outHostHero, std::string& outClientHero, bool& outMatchStarted);
        void SendClientInput(const InputMsg& inp);
        void SendStartMatch(int world, const std::string& hostHero, const std::string& clientHero);

        bool IsHosting() const { return _hosting; }
        bool IsConnectedClient() const { return _clientConnected; }
        const std::vector<RoomInfo>& Rooms() const { return _rooms; }

        static std::string BuildDiscoveryPacket(const std::string& id, const std::string& name, const std::string& host, int port);
        static bool ParseDiscoveryPacket(const std::string& pkt, RoomInfo& outRoom);

    private:
        intptr_t _advSock = -1;
        intptr_t _gameSock = -1;
        bool _hosting = false;
        bool _clientConnected = false;
        int _advPort = DefaultAdvertisePort;
        int _gamePort = DefaultGamePort;
        std::string _roomName = "Nightfall";
        std::string _deviceId = "nf-native";
        std::string _advHost = "127.0.0.1";
        float _lastAdvertiseTime = -10.0f;

        uint32_t _peerAddr = 0; // network byte order
        uint16_t _peerPort = 0; // network byte order
        bool _hasPeer = false;

        InputMsg _latestClientInput{};
        bool _hasClientInput = false;
        std::string _clientHero = "denyse";
        std::vector<RoomInfo> _rooms;
    };
}
