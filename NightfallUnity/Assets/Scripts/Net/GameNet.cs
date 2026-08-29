using Unity.Collections;
using Unity.Netcode;
using UnityEngine;

namespace Nightfall.Net
{
    public struct InputMsg : INetworkSerializable
    {
        public bool L, R, D, Jp, Jn, Ap;
        public void NetworkSerialize<T>(BufferSerializer<T> s) where T : IReaderWriter
        {
            s.SerializeValue(ref L);
            s.SerializeValue(ref R);
            s.SerializeValue(ref D);
            s.SerializeValue(ref Jp);
            s.SerializeValue(ref Jn);
            s.SerializeValue(ref Ap);
        }
    }

    /// <summary>Hero state: position, health, animation and the attack window so the client can draw the swing.</summary>
    public struct PlrSnap : INetworkSerializable
    {
        public float X, Y;
        public byte Hp, MaxHp, Lives;
        public byte Anim;   // 0 idle · 1 walk · 2 jump · 3 fall · 4 attack
        public byte Atk;    // attack progress 0..255 (0 = not attacking)
        public byte Flags;  // bit0 invulnerable · bit1 just hurt · bit2 dead · bit3 facing left
        public byte Hero;   // 0 mike · 1 denyse

        public void NetworkSerialize<T>(BufferSerializer<T> s) where T : IReaderWriter
        {
            s.SerializeValue(ref X);
            s.SerializeValue(ref Y);
            s.SerializeValue(ref Hp);
            s.SerializeValue(ref MaxHp);
            s.SerializeValue(ref Lives);
            s.SerializeValue(ref Anim);
            s.SerializeValue(ref Atk);
            s.SerializeValue(ref Flags);
            s.SerializeValue(ref Hero);
        }
    }

    public struct EntSnap : INetworkSerializable
    {
        public float X, Y;
        public byte Hp;
        public byte Kind;   // Catalog.Enemies index
        public byte Flags;  // bit0 dead · bit1 flash · bit2 facing left
        public void NetworkSerialize<T>(BufferSerializer<T> s) where T : IReaderWriter
        {
            s.SerializeValue(ref X);
            s.SerializeValue(ref Y);
            s.SerializeValue(ref Hp);
            s.SerializeValue(ref Kind);
            s.SerializeValue(ref Flags);
        }
    }

    public struct ProjSnap : INetworkSerializable
    {
        public float X, Y, Vx, Vy;
        public byte Kind;   // GameSim.ProjKindId
        public byte Flags;  // bit0 friendly
        public void NetworkSerialize<T>(BufferSerializer<T> s) where T : IReaderWriter
        {
            s.SerializeValue(ref X);
            s.SerializeValue(ref Y);
            s.SerializeValue(ref Vx);
            s.SerializeValue(ref Vy);
            s.SerializeValue(ref Kind);
            s.SerializeValue(ref Flags);
        }
    }

    public struct FxSnap : INetworkSerializable
    {
        public float X, Y;
        public byte Kind;
        public byte Dir;    // 0 = facing left, 1 = facing right
        public void NetworkSerialize<T>(BufferSerializer<T> s) where T : IReaderWriter
        {
            s.SerializeValue(ref X);
            s.SerializeValue(ref Y);
            s.SerializeValue(ref Kind);
            s.SerializeValue(ref Dir);
        }
    }

    /// <summary>
    /// Full presentation snapshot. v1 only carried the two heroes, so the joining phone never saw
    /// item pickups, projectiles or hit effects. v2 carries items, projectiles and FX events too.
    /// </summary>
    public struct FrameSnap : INetworkSerializable
    {
        // kept small on purpose: a reliable NGO message has to fit under the transport MTU,
        // and this ships 20×/s to a phone on Wi-Fi
        public const int MaxEnts = 20, MaxProjs = 12, MaxFx = 8;

        public PlrSnap P0, P1;
        public int Score, Souls;
        public bool LockArena;
        public FixedString32Bytes SimState;
        public int BossHp, BossMax;
        public byte BossKind, BossFlags;   // BossFlags bit0 dead · bit1 facing left
        public Vector2 BossPos;
        public ulong ItemMask;
        public float Shake;
        public FixedString64Bytes Msg;
        public float MsgT;
        public EntSnap[] Ents;
        public ProjSnap[] Projs;
        public FxSnap[] Fxs;

        public void NetworkSerialize<T>(BufferSerializer<T> s) where T : IReaderWriter
        {
            P0.NetworkSerialize(s);
            P1.NetworkSerialize(s);
            s.SerializeValue(ref Score);
            s.SerializeValue(ref Souls);
            s.SerializeValue(ref LockArena);
            s.SerializeValue(ref SimState);
            s.SerializeValue(ref BossHp);
            s.SerializeValue(ref BossMax);
            s.SerializeValue(ref BossKind);
            s.SerializeValue(ref BossFlags);
            s.SerializeValue(ref BossPos);
            s.SerializeValue(ref ItemMask);
            s.SerializeValue(ref Shake);
            s.SerializeValue(ref Msg);
            s.SerializeValue(ref MsgT);
            int ne = Clamp(Ents, MaxEnts);
            s.SerializeValue(ref ne);
            if (ne < 0 || ne > MaxEnts) ne = 0;
            if (s.IsReader) Ents = new EntSnap[ne];
            for (int i = 0; i < ne; i++)
            {
                var e = (Ents != null && i < Ents.Length) ? Ents[i] : default;
                e.NetworkSerialize(s);
                if (s.IsReader) Ents[i] = e;
            }

            int np = Clamp(Projs, MaxProjs);
            s.SerializeValue(ref np);
            if (np < 0 || np > MaxProjs) np = 0;
            if (s.IsReader) Projs = new ProjSnap[np];
            for (int i = 0; i < np; i++)
            {
                var e = (Projs != null && i < Projs.Length) ? Projs[i] : default;
                e.NetworkSerialize(s);
                if (s.IsReader) Projs[i] = e;
            }

            int nf = Clamp(Fxs, MaxFx);
            s.SerializeValue(ref nf);
            if (nf < 0 || nf > MaxFx) nf = 0;
            if (s.IsReader) Fxs = new FxSnap[nf];
            for (int i = 0; i < nf; i++)
            {
                var e = (Fxs != null && i < Fxs.Length) ? Fxs[i] : default;
                e.NetworkSerialize(s);
                if (s.IsReader) Fxs[i] = e;
            }
        }

        static int Clamp<E>(E[] a, int cap)
        {
            int n = a != null ? a.Length : 0;
            return n > cap ? cap : n;
        }
    }

    public class GameNet : NetworkBehaviour
    {
        public static GameNet Instance;
        public static InputMsg LastFromClient;

        public override void OnNetworkSpawn()
        {
            Instance = this;
        }

        public override void OnNetworkDespawn()
        {
            if (Instance == this) Instance = null;
        }

        public override void OnDestroy()
        {
            base.OnDestroy();
            if (Instance == this) Instance = null;
        }

        [Rpc(SendTo.Server, InvokePermission = RpcInvokePermission.Everyone)]
        public void SubmitInputRpc(InputMsg input)
        {
            // sticky one-shot bits: the host consumes them after its own tick
            bool jp = LastFromClient.Jp || input.Jp;
            bool ap = LastFromClient.Ap || input.Ap;
            LastFromClient = input;
            LastFromClient.Jp = jp;
            LastFromClient.Ap = ap;
        }

        [Rpc(SendTo.Server, InvokePermission = RpcInvokePermission.Everyone)]
        public void ClaimHeroRpc(FixedString64Bytes hero)
        {
            NightApp.I?.OnClientHero(hero.ToString());
        }

        [Rpc(SendTo.ClientsAndHost)]
        public void StartMatchRpc(int world, FixedString64Bytes hostHero, FixedString64Bytes clientHero)
        {
            NightApp.I?.BeginMatch(world, hostHero.ToString(), clientHero.ToString(), true);
        }

        /// <summary>
        /// Presentation snapshot, 20×/s. Unreliable on purpose: it is state, not a command —
        /// a dropped frame is replaced 50 ms later, and it keeps the big payload off the
        /// reliable pipeline (which does not fragment and would choke on it).
        /// </summary>
        [Rpc(SendTo.NotServer, Delivery = RpcDelivery.Unreliable)]
        public void StateRpc(FrameSnap snap)
        {
            NightApp.I?.ApplyRemote(snap);
        }

        /// <summary>Host tells the clients to leave the match (abandon / back to the map).</summary>
        [Rpc(SendTo.NotServer)]
        public void EndMatchRpc(FixedString32Bytes reason)
        {
            NightApp.I?.OnHostEndedMatch(reason.ToString());
        }
    }
}
