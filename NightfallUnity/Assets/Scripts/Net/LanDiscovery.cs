using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

namespace Nightfall.Net
{
    [Serializable]
    public class RoomInfo
    {
        public string id;
        public string name;
        public string host;
        public string kind;
        public int port = 7777;
        [NonSerialized] public float lastSeen;
    }

    public static class LanIp
    {
        public static string Pick()
        {
            var best = "";
            int bestScore = -1;
            try
            {
                foreach (var ni in NetworkInterface.GetAllNetworkInterfaces())
                {
                    if (ni.OperationalStatus != OperationalStatus.Up) continue;
                    if (ni.NetworkInterfaceType == NetworkInterfaceType.Loopback) continue;
                    int iface = ScoreIface(ni);
                    if (iface < 0) continue;
                    foreach (var ua in ni.GetIPProperties().UnicastAddresses)
                    {
                        int s = ScoreAddress(ua.Address) + iface;
                        if (s > bestScore)
                        {
                            bestScore = s;
                            best = V4(ua.Address);
                        }
                    }
                }
            }
            catch { }
            return string.IsNullOrEmpty(best) ? "0.0.0.0" : best;
        }

        public static string V4(IPAddress ip)
        {
            if (ip == null) return "";
            if (ip.IsIPv4MappedToIPv6) ip = ip.MapToIPv4();
            if (ip.AddressFamily != AddressFamily.InterNetwork) return "";
            return ip.ToString();
        }

        public static bool IsUsable(string host)
        {
            if (string.IsNullOrEmpty(host)) return false;
            if (!IPAddress.TryParse(host, out var ip)) return false;
            return ScoreAddress(ip) >= 10;
        }

        public static int ScoreAddress(IPAddress ip)
        {
            if (ip == null) return -1;
            if (ip.IsIPv4MappedToIPv6) ip = ip.MapToIPv4();
            if (ip.AddressFamily != AddressFamily.InterNetwork) return -1;
            var b = ip.GetAddressBytes();
            if (b[0] == 0 || b[0] == 127 || b[0] == 255) return -1;
            if (b[0] == 169 && b[1] == 254) return 1;
            if (b[0] == 192 && b[1] == 168) return 40;
            if (b[0] == 10) return 30;
            if (b[0] == 172 && b[1] >= 16 && b[1] <= 31) return 28;
            return 10;
        }

        static int ScoreIface(NetworkInterface ni)
        {
            string n = ((ni.Name ?? "") + " " + (ni.Description ?? "")).ToLowerInvariant();
            if (n.Contains("loopback") || n.Contains("vmware") || n.Contains("virtualbox")
                || n.Contains("hyper-v") || n.Contains("vethernet")) return -1;
            if (n.Contains("rmnet") || n.Contains("wwan") || n.Contains("cellular") || n.Contains("mobile")) return 0;
            if (n.Contains("wlan") || n.Contains("wifi") || n.Contains("wi-fi") || n.Contains("p2p")) return 20;
            return 5;
        }
    }

    public sealed class LanDiscovery : IDisposable
    {
        public const int AdvertisePort = 47888;
        readonly List<RoomInfo> _rooms = new();
        readonly ConcurrentQueue<(string json, string src)> _inbox = new();
        UdpClient _udp;
        Thread _thread;
        volatile bool _run;
        public string RoomName = "Nightfall";
        public int GamePort = 7777;
        public bool Advertising;
        public string DeviceId = "device";
        public string AdvertiseHost = "";

        static float Clock() => Time.unscaledTime;

        public IReadOnlyList<RoomInfo> Rooms
        {
            get
            {
                lock (_rooms)
                {
                    float now = Clock();
                    _rooms.RemoveAll(r => now - r.lastSeen > 8f);
                    return _rooms.ToArray();
                }
            }
        }

        public string Fingerprint()
        {
            var sb = new StringBuilder();
            foreach (var r in Rooms)
                sb.Append(r.id).Append('|').Append(r.host).Append('|').Append(r.port).Append(';');
            return sb.ToString();
        }

        public void StartAdvertise(string name, int port)
        {
            RoomName = name;
            GamePort = port;
            Advertising = true;
            CacheIdentity();
            EnsureSocket();
        }

        public void StartListen()
        {
            Advertising = false;
            CacheIdentity();
            EnsureSocket();
        }

        void CacheIdentity()
        {
            try
            {
                var id = SystemInfo.deviceUniqueIdentifier;
                if (!string.IsNullOrEmpty(id))
                    DeviceId = id.Length > 12 ? id.Substring(0, 12) : id;
            }
            catch { DeviceId = "device"; }
            AdvertiseHost = LanIp.Pick();
        }

        void EnsureSocket()
        {
            Stop();
            _run = true;
            try
            {
                WifiDirectBridge.AcquireMulticastLock();
                _udp = new UdpClient();
                _udp.EnableBroadcast = true;
                _udp.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                _udp.Client.ReceiveTimeout = 200;
                _udp.Client.Bind(new IPEndPoint(IPAddress.Any, AdvertisePort));
                _thread = new Thread(Loop) { IsBackground = true, Name = "NightfallLAN" };
                _thread.Start();
            }
            catch (Exception e)
            {
                Debug.LogWarning("LAN discovery: " + e.Message);
                _run = false;
            }
        }

        void Loop()
        {
            var lastAd = DateTime.MinValue;
            while (_run)
            {
                try
                {
                    if (Advertising && (DateTime.UtcNow - lastAd).TotalMilliseconds > 900)
                    {
                        lastAd = DateTime.UtcNow;
                        string host = string.IsNullOrEmpty(AdvertiseHost) ? "0.0.0.0" : AdvertiseHost;
                        var json = "{\"id\":\"" + DeviceId + "\",\"name\":\"" + Escape(RoomName)
                            + "\",\"host\":\"" + host + "\",\"port\":" + GamePort + ",\"kind\":\"lan\"}";
                        var bytes = Encoding.UTF8.GetBytes("NIGHTFALL|" + json);
                        try { _udp?.Send(bytes, bytes.Length, new IPEndPoint(IPAddress.Broadcast, AdvertisePort)); } catch { }
                        foreach (var bcast in SubnetBroadcasts())
                        {
                            try { _udp?.Send(bytes, bytes.Length, new IPEndPoint(bcast, AdvertisePort)); } catch { }
                        }
                    }

                    if (_udp != null && _udp.Available > 0)
                    {
                        var ep = new IPEndPoint(IPAddress.Any, AdvertisePort);
                        var data = _udp.Receive(ref ep);
                        if (data == null || data.Length < 11) continue;
                        var txt = Encoding.UTF8.GetString(data);
                        if (!txt.StartsWith("NIGHTFALL|")) continue;
                        _inbox.Enqueue((txt.Substring(10), LanIp.V4(ep.Address)));
                    }
                    else Thread.Sleep(40);
                }
                catch (SocketException) { Thread.Sleep(60); }
                catch (Exception) { Thread.Sleep(80); }
            }
        }

        public void Tick()
        {
            while (_inbox.TryDequeue(out var item))
                Ingest(item.json, item.src);

            lock (_rooms)
            {
                float now = Clock();
                _rooms.RemoveAll(r => now - r.lastSeen > 8f);
            }
        }

        void Ingest(string json, string src)
        {
            RoomInfo room;
            try { room = JsonUtility.FromJson<RoomInfo>(json); }
            catch { return; }
            if (room == null || string.IsNullOrEmpty(room.id)) return;
            if (room.id == DeviceId) return;

            string jsonHost = room.host;
            string chosen = jsonHost;
            if (!LanIp.IsUsable(chosen)) chosen = src;
            if (!LanIp.IsUsable(chosen)) return;
            if (LanIp.ScoreAddress(Parse(jsonHost)) >= LanIp.ScoreAddress(Parse(src)))
                chosen = LanIp.IsUsable(jsonHost) ? jsonHost : chosen;

            if (chosen == AdvertiseHost && !string.IsNullOrEmpty(AdvertiseHost) && AdvertiseHost != "0.0.0.0")
                return;

            room.host = chosen;
            if (room.port <= 0) room.port = 7777;
            if (string.IsNullOrEmpty(room.name)) room.name = "Nightfall";
            room.lastSeen = Clock();

            lock (_rooms)
            {
                int ix = _rooms.FindIndex(r => r.id == room.id);
                if (ix >= 0)
                {
                    var old = _rooms[ix];
                    int oldS = LanIp.ScoreAddress(Parse(old.host));
                    int newS = LanIp.ScoreAddress(Parse(room.host));
                    if (newS < oldS) room.host = old.host;
                    _rooms[ix] = room;
                }
                else _rooms.Add(room);
            }
        }

        static IPAddress Parse(string s)
        {
            return IPAddress.TryParse(s, out var ip) ? ip : null;
        }

        static string Escape(string s)
        {
            if (string.IsNullOrEmpty(s)) return "Nightfall";
            return s.Replace("\\", "").Replace("\"", "'");
        }

        static IEnumerable<IPAddress> SubnetBroadcasts()
        {
            var list = new List<IPAddress>();
            try
            {
                foreach (var ni in NetworkInterface.GetAllNetworkInterfaces())
                {
                    if (ni.OperationalStatus != OperationalStatus.Up) continue;
                    if (ni.NetworkInterfaceType == NetworkInterfaceType.Loopback) continue;
                    foreach (var addr in ni.GetIPProperties().UnicastAddresses)
                    {
                        if (LanIp.ScoreAddress(addr.Address) < 10 || addr.IPv4Mask == null) continue;
                        byte[] ip = addr.Address.GetAddressBytes();
                        byte[] mask = addr.IPv4Mask.GetAddressBytes();
                        if (ip.Length != 4 || mask.Length != 4) continue;
                        var b = new byte[4];
                        for (int i = 0; i < 4; i++) b[i] = (byte)(ip[i] | (~mask[i]));
                        var bc = new IPAddress(b);
                        if (!bc.Equals(IPAddress.Broadcast)) list.Add(bc);
                    }
                }
            }
            catch { }
            return list;
        }

        public void Stop()
        {
            _run = false;
            try { _udp?.Close(); } catch { }
            _udp = null;
            while (_inbox.TryDequeue(out _)) { }
            lock (_rooms) _rooms.Clear();
        }

        public void Dispose() => Stop();
    }
}
