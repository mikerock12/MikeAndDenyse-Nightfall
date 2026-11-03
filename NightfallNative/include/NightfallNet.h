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
        static bool DecodeSnapshotPacket(const uint8_t* data,size_t length,uint32_t generation,uint32_t& lastSequence,FrameSnap& snapshot);

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
        bool _matchStarted = false;
        uint32_t _matchEpoch = 0;
        uint32_t _receivedEpoch = 0;
        uint32_t _snapshotSequence = 0, _receivedSequence = 0;
        double _lastReceived = 0, _lastInputReceived = 0;
        int _matchWorld = 0;
        std::string _hostHero = "mike", _matchClientHero = "denyse", _preferredHero = "denyse";
        void TransmitStart();
    };
}
