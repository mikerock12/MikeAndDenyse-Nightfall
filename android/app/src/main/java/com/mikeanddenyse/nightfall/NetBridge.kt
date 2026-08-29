package com.mikeanddenyse.nightfall

import android.os.Handler
import android.os.Looper
import android.webkit.JavascriptInterface
import android.webkit.WebView
import org.json.JSONArray
import org.json.JSONObject
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.net.InetSocketAddress
import java.net.ServerSocket
import java.net.Socket
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class NetBridge(private val web: WebView, private val wifi: WifiDirectController) {
    private val main = Handler(Looper.getMainLooper())
    private val io = Executors.newCachedThreadPool()
    private val running = AtomicBoolean(false)
    private var mode = "idle"
    private var roomName = "Nightfall"
    private var roomId = UUID.randomUUID().toString().take(8)
    private var tcpPort = 7777
    private var server: ServerSocket? = null
    private var client: Socket? = null
    private var writer: BufferedWriter? = null
    private var udp: DatagramSocket? = null
    private val rooms = ConcurrentHashMap<String, JSONObject>()

    @JavascriptInterface
    fun send(raw: String) {
        try {
            val o = JSONObject(raw)
            when (o.optString("cmd")) {
                "host" -> startHost(o.optString("kind", "lan"), o.optString("name", "Nightfall"))
                "scan" -> startScan()
                "join" -> joinRoom(o.optString("id"))
                "peer" -> writePeer(o.opt("payload"))
                "stop" -> stopAll()
            }
        } catch (e: Exception) {
            emitStatus("Falha de rede: ${e.message}")
        }
    }

    private fun startHost(kind: String, name: String) {
        stopAll()
        mode = "host"
        roomName = name
        roomId = UUID.randomUUID().toString().take(8)
        running.set(true)
        io.execute { hostTcp() }
        if (kind == "p2p") {
            emitStatus("Abrindo Wi-Fi Direct…")
            wifi.hostGroup { ok, msg ->
                emitStatus(msg)
                if (ok) io.execute { hostAnnounce() }
            }
        } else {
            io.execute { hostAnnounce() }
            emitStatus("Sala na LAN. Peça ao outro celular Procurar partidas.")
        }
    }

    private fun hostTcp() {
        try {
            val ss = ServerSocket()
            ss.reuseAddress = true
            ss.bind(InetSocketAddress(tcpPort))
            server = ss
            emitStatus("Aguardando o segundo caçador…")
            val sock = ss.accept()
            attachSocket(sock)
            emitPeer(true, "Par conectado")
        } catch (e: Exception) {
            if (running.get()) emitStatus("Host TCP: ${e.message}")
        }
    }

    private fun hostAnnounce() {
        try {
            val ds = DatagramSocket()
            ds.broadcast = true
            udp = ds
            val payload = JSONObject()
                .put("id", roomId)
                .put("name", roomName)
                .put("port", tcpPort)
                .put("kind", if (wifi.isGroupOwner) "p2p" else "lan")
                .toString()
            val bytes = ("NIGHTFALL|$payload").toByteArray(Charsets.UTF_8)
            while (running.get() && mode == "host") {
                val pkt = DatagramPacket(bytes, bytes.size, InetAddress.getByName("255.255.255.255"), 47888)
                try { ds.send(pkt) } catch (_: Exception) {}
                Thread.sleep(800)
            }
        } catch (_: Exception) { }
    }

    private fun startScan() {
        stopAll()
        mode = "client"
        running.set(true)
        rooms.clear()
        emitStatus("Procurando na LAN e no Wi-Fi Direct…")
        wifi.discover { }
        io.execute { listenUdp() }
    }

    private fun listenUdp() {
        try {
            val ds = DatagramSocket(null)
            ds.reuseAddress = true
            ds.bind(InetSocketAddress(47888))
            udp = ds
            val buf = ByteArray(1024)
            while (running.get()) {
                val pkt = DatagramPacket(buf, buf.size)
                ds.receive(pkt)
                val txt = String(pkt.data, 0, pkt.length, Charsets.UTF_8)
                if (!txt.startsWith("NIGHTFALL|")) continue
                val json = JSONObject(txt.substringAfter("|"))
                json.put("host", pkt.address.hostAddress)
                rooms[json.optString("id")] = json
                emitRooms()
            }
        } catch (_: Exception) { }
    }

    private fun joinRoom(id: String) {
        val room = rooms[id]
        if (room == null) {
            emitStatus("Sala não encontrada.")
            return
        }
        val host = room.optString("host")
        val port = room.optInt("port", 7777)
        val kind = room.optString("kind")
        emitStatus("Conectando a $host…")
        if (kind == "p2p-peer") {
            emitStatus("Ligando Wi-Fi Direct a $host…")
            wifi.connectAddress(host) { ok, msg -> emitStatus(msg) }
            return
        }
        io.execute { connectTcp(host, port) }
    }

    private fun connectTcp(host: String, port: Int) {
        try {
            val sock = Socket()
            sock.connect(InetSocketAddress(host, port), 7000)
            attachSocket(sock)
            emitPeer(true, "Conectado ao host")
        } catch (e: Exception) {
            emitStatus("Falha ao entrar: ${e.message}")
            emitPeer(false, e.message ?: "erro")
        }
    }

    @Synchronized
    private fun attachSocket(sock: Socket) {
        client = sock
        writer = BufferedWriter(OutputStreamWriter(sock.getOutputStream(), Charsets.UTF_8))
        io.execute {
            try {
                val reader = BufferedReader(InputStreamReader(sock.getInputStream(), Charsets.UTF_8))
                while (running.get()) {
                    val line = reader.readLine() ?: break
                    emitJs(JSONObject().put("t", "data").put("p", JSONObject(line)))
                }
            } catch (_: Exception) {
            } finally {
                emitPeer(false, "Par saiu")
            }
        }
    }

    @Synchronized
    private fun writePeer(payload: Any?) {
        val w = writer ?: return
        io.execute {
            try {
                val line = when (payload) {
                    is JSONObject -> payload.toString()
                    is String -> payload
                    else -> (JSONObject.wrap(payload)?.toString() ?: "{}")
                }
                w.write(line)
                w.write("\n")
                w.flush()
            } catch (_: Exception) { }
        }
    }

    fun noteOwner(ip: String) {
        if (mode == "client" && ip.isNotBlank()) {
            val fake = org.json.JSONObject()
                .put("id", "p2p-owner")
                .put("name", "Host Wi-Fi Direct")
                .put("host", ip)
                .put("port", tcpPort)
                .put("kind", "p2p")
            rooms["p2p-owner"] = fake
            emitRooms()
            emitStatus("Grupo Direct pronto em $ip")
            if (client == null) io.execute { connectTcp(ip, tcpPort) }
        }
    }

    fun noteDirectPeers(peers: List<Pair<String, String>>) {
        peers.forEach { (name, addr) ->
            val id = "p2p-$addr"
            rooms[id] = org.json.JSONObject()
                .put("id", id)
                .put("name", name)
                .put("host", addr)
                .put("port", tcpPort)
                .put("kind", "p2p-peer")
        }
        if (peers.isNotEmpty()) emitRooms()
    }

    fun stopAll() {
        running.set(false)
        try { server?.close() } catch (_: Exception) {}
        try { client?.close() } catch (_: Exception) {}
        try { udp?.close() } catch (_: Exception) {}
        server = null
        client = null
        writer = null
        udp = null
        wifi.stop()
        mode = "idle"
    }

    private fun emitStatus(m: String) {
        emitJs(JSONObject().put("t", "status").put("m", m))
    }

    private fun emitPeer(ok: Boolean, m: String) {
        emitJs(JSONObject().put("t", "peer").put("ok", ok).put("m", m))
    }

    private fun emitRooms() {
        val arr = JSONArray()
        rooms.values.forEach { arr.put(it) }
        emitJs(JSONObject().put("t", "rooms").put("list", arr))
    }

    private fun emitJs(obj: JSONObject) {
        val escaped = obj.toString()
            .replace("\\", "\\\\")
            .replace("'", "\\'")
            .replace("\n", "\\n")
            .replace("\r", "")
        main.post {
            web.evaluateJavascript("window.__nightNet && window.__nightNet('$escaped')", null)
        }
    }
}
