package com.mikeanddenyse.nightfall

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.wifi.p2p.WifiP2pConfig
import android.net.wifi.p2p.WifiP2pDevice
import android.net.wifi.p2p.WifiP2pManager
import android.os.Build
import android.os.Looper

class WifiDirectController(private val ctx: Context) {
    private val manager = ctx.getSystemService(Context.WIFI_P2P_SERVICE) as? WifiP2pManager
    private val channel: WifiP2pManager.Channel? = manager?.initialize(ctx, Looper.getMainLooper(), null)
    var isGroupOwner = false
        private set
    private var receiver: BroadcastReceiver? = null
    var onPeers: ((List<WifiP2pDevice>) -> Unit)? = null
    var onOwnerAddress: ((String) -> Unit)? = null

    @SuppressLint("MissingPermission")
    fun hostGroup(cb: (Boolean, String) -> Unit) {
        val m = manager
        val ch = channel
        if (m == null || ch == null) {
            cb(false, "Wi-Fi Direct indisponível neste aparelho.")
            return
        }
        register()
        m.removeGroup(ch, object : WifiP2pManager.ActionListener {
            override fun onSuccess() = create(cb)
            override fun onFailure(reason: Int) = create(cb)
        })
    }

    @SuppressLint("MissingPermission")
    private fun create(cb: (Boolean, String) -> Unit) {
        val m = manager ?: return
        val ch = channel ?: return
        m.createGroup(ch, object : WifiP2pManager.ActionListener {
            override fun onSuccess() {
                isGroupOwner = true
                cb(true, "Grupo Wi-Fi Direct criado. O outro celular deve Procurar partidas.")
            }
            override fun onFailure(reason: Int) {
                cb(false, "Não criou o grupo Direct ($reason). Tente LAN no mesmo Wi-Fi.")
            }
        })
    }

    @SuppressLint("MissingPermission")
    fun discover(cb: (String) -> Unit) {
        val m = manager
        val ch = channel
        if (m == null || ch == null) {
            cb("Wi-Fi Direct indisponível")
            return
        }
        register()
        m.discoverPeers(ch, object : WifiP2pManager.ActionListener {
            override fun onSuccess() { cb("Buscando pares Direct…") }
            override fun onFailure(reason: Int) { cb("Direct scan falhou ($reason)") }
        })
    }

    @SuppressLint("MissingPermission")
    fun connectAddress(addr: String, cb: (Boolean, String) -> Unit) {
        val d = WifiP2pDevice()
        d.deviceAddress = addr
        connect(d, cb)
    }

    fun connect(device: WifiP2pDevice, cb: (Boolean, String) -> Unit) {
        val m = manager ?: return
        val ch = channel ?: return
        val cfg = WifiP2pConfig()
        cfg.deviceAddress = device.deviceAddress
        m.connect(ch, cfg, object : WifiP2pManager.ActionListener {
            override fun onSuccess() = cb(true, "Pedido Direct enviado")
            override fun onFailure(reason: Int) = cb(false, "Direct connect falhou ($reason)")
        })
    }

    fun stop() {
        try {
            receiver?.let { ctx.unregisterReceiver(it) }
        } catch (_: Exception) { }
        receiver = null
        val m = manager
        val ch = channel
        if (m != null && ch != null) {
            try { m.removeGroup(ch, object : WifiP2pManager.ActionListener {
                override fun onSuccess() {}
                override fun onFailure(reason: Int) {}
            }) } catch (_: Exception) { }
        }
        isGroupOwner = false
    }

    private fun register() {
        if (receiver != null) return
        receiver = object : BroadcastReceiver() {
            @SuppressLint("MissingPermission")
            override fun onReceive(context: Context?, intent: Intent?) {
                val m = manager ?: return
                val ch = channel ?: return
                when (intent?.action) {
                    WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION -> {
                        m.requestPeers(ch) { list -> onPeers?.invoke(list.deviceList.toList()) }
                    }
                    WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION -> {
                        m.requestConnectionInfo(ch) { info ->
                            isGroupOwner = info.groupFormed && info.isGroupOwner
                            val addr = info.groupOwnerAddress?.hostAddress
                            if (info.groupFormed && addr != null) onOwnerAddress?.invoke(addr)
                        }
                    }
                }
            }
        }
        val f = IntentFilter().apply {
            addAction(WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION)
            addAction(WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION)
            addAction(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION)
            addAction(WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION)
        }
        if (Build.VERSION.SDK_INT >= 33) {
            ctx.registerReceiver(receiver, f, Context.RECEIVER_NOT_EXPORTED)
        } else {
            ctx.registerReceiver(receiver, f)
        }
    }
}
