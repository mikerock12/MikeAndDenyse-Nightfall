# Mike & Denyse: Nightfall

> Platformer cooperativo 2D das trevas para **Android**, em landscape, com **multiplayer local via Wi-Fi** — dois celulares na mesma rede, sem servidor e sem internet.

Dois caçadores — **Mike** (lâmina, mais vida, corpo a corpo) e **Denyse** (cristal, mais rápida, projétil, queda lenta) — atravessam **16 noites** até **Belial, Senhor dos Pesadelos**.

`16 fases · 32 monstros · 16 chefes · 1 chefão · coop em dois celulares`

| | |
|---|---|
| Pacote | `com.mikeanddenyse.nightfall` |
| Engine | Unity **6000.5.8f1** |
| Rede | Unity Netcode for GameObjects 2.13 + Unity Transport |
| Alvo | Android, arm64-v8a + armeabi-v7a, `sensorLandscape` |
| Versão | 3.1 |
| Idioma | UI em português · IDs em inglês |

---

## Índice

- [Como o jogo funciona](#como-o-jogo-funciona)
- [Arquitetura do código](#arquitetura-do-código)
- [Multiplayer local — como funciona de verdade](#multiplayer-local--como-funciona-de-verdade)
- [Detalhes do projeto](#detalhes-do-projeto)
- [Como buildar e rodar](#como-buildar-e-rodar)
- [Testes](#testes)
- [O que está no repositório e o que não está](#o-que-está-no-repositório-e-o-que-não-está)
- [Documentação viva](#documentação-viva)

---

## Como o jogo funciona

### Fluxo de telas

```
título ── UM JOGADOR ─────────► seleção de herói ──► mapa ──► fase
   │                                 │
   ├── DOIS CELULARES ───► rede ─────┘  (no coop, só o host vê o mapa e escolhe)
   ├── CONTROLES
   └── SAIR

fase ── II / Esc / botão VOLTAR ──► pausa ── Continuar · Mapa · Menu inicial
fase ── os dois morrem ───────────► morte ── Renascem · Mapa · Menu inicial
fase ── sino após o chefe ────────► fim de fase ── Próxima · Mapa · Menu
fase ── Belial morre ─────────────► vitória
```

O **botão VOLTAR do Android** (`KeyCode.Escape`) é hierárquico: jogo pausa → pausa retoma →
menu volta um nível → título fecha o app. Trava de 0,25 s contra toque duplo.
O mundo continua desenhado atrás de pausa, morte e vitória — a tela nunca fica preta.

### Controles de toque

Os pads são desenhados no `OnGUI`, **por cima** do mundo, e lidos direto de `Input.touches`
(`PlayTouch.Sample()`), não pelo EventSystem do uGUI. Isso é proposital: o EventSystem só
entrega **um** ponteiro, e o jogador precisa andar e pular ao mesmo tempo. O teste de acerto
é circular, em espaço virtual 960×540.

| pad | ação |
|---|---|
| ◀ ▶ | andar |
| ▼ | descer pela plataforma de sentido único |
| PULO | pular (coyote 0,12 s · buffer 0,14 s · pulo variável) |
| GOLPE | lâmina (Mike) ou cristal (Denyse) |
| II | pausa |

Teclado (útil no Editor): `A`/`D` andar · `W`, `Espaço`, `J`, `Z` pular · `K`, `X` atacar · `Esc`/`P` pausa.

### Progressão

`Progress` grava em `PlayerPrefs`: fase liberada, herói escolhido, almas e pontuação.
Só a fase 1 começa aberta; vencer libera a próxima; fases já abertas podem ser repetidas.

---

## Arquitetura do código

O motor é **100 % código**. Não há Tilemap, nem Rigidbody2D, nem `Collider2D`, nem prefabs de
personagem — a simulação é determinística e o desenho é IMGUI. O eixo **Y cresce para baixo**
(estilo canvas HTML), herança do cliente HTML5 que serviu de especificação.

```
Assets/Scripts/
  Bootstrap.cs      [RuntimeInitializeOnLoadMethod] cria NightApp se a cena não tiver
  NightApp.cs       orquestrador: telas, input, botão voltar, rede, snapshot   (906 l.)
  GameMenu.cs       toda a UI uGUI construída em código                        (616 l.)
  GameData.cs       catálogo: 2 heróis, 32 inimigos, 17 chefes, 16 mundos      (154 l.)
  LevelBuilder.cs   gera as 16 fases por "beats" + reparo de rota + selagem    (526 l.)
  Sim.cs            física, combate, IA, arena, Belial, resgate de armadilha   (883 l.)
  WorldView.cs      render IMGUI: parallax, tiles, FX, HUD, pads               (862 l.)
  ArtGen.cs         TODA a arte procedural, gerada no boot                     (732 l.)
  Sfx.cs            12 efeitos sonoros sintetizados em runtime                 (153 l.)
  PlayTouch.cs      pads redondos lidos de Input.touches                        (88 l.)
  Progress.cs       PlayerPrefs                                                 (72 l.)
  SpriteBank.cs     carrega Resources/Art com fallback                          (85 l.)
  Net/GameNet.cs           RPCs + FrameSnap v2                                 (236 l.)
  Net/LanDiscovery.cs      descoberta UDP na porta 47888                       (315 l.)
  Net/WifiDirectBridge.cs  multicast lock + grupo Wi-Fi Direct (JNI)            (63 l.)

Assets/Editor/
  AndroidBuilder.cs  monta cena/prefabs, prepara arte, builda o APK (roda o smoke antes)
  Smoke.cs           77 checks — inclui um bot que joga as 16 fases
  ArtDump.cs         escreve a arte procedural e frames compostos em tools/artdump/
```

### O laço principal

`NightApp.Update()` é o único dono do frame, e a ordem importa:

```csharp
PlayTouch.Sample();          // 1. dedo → estado, ANTES do tick (senão o input chega 1 frame tarde)
BackPressed();               // 2. botão voltar / pausa
_disc?.Tick();               // 3. drena a fila da thread de descoberta LAN na main thread
SubmitInputRpc(...);         // 4. se for cliente: manda o próprio input ao host
Sim.Tick(dt)                 // 5a. host/solo: simula
  ou Sim.TickRemote(dt);     // 5b. cliente: só câmera, fade e animação
StateRpc(BuildSnap());       // 6. host: snapshot a 20 Hz
PumpFx();                    // 7. eventos de FX → view (depois do snapshot, para o cliente ver)
SyncScreen();                // 8. reconcilia tela · sim · painel
```

Todo o `Update` está dentro de um `try/catch`: uma exceção vira banner de erro na UI e pausa,
nunca uma tela congelada. O `OnGUI` do `WorldView` usa `try/finally` para garantir que nenhum
`GUI.BeginGroup` fique aberto — um grupo aberto quebra o IMGUI pelo resto da execução e o
mundo inteiro some.

### Física — o detalhe que não pode ser mexido

`GameSim.Phys` move em **sub-passos de no máximo 6 px**, nunca o frame inteiro de uma vez.

Uma plataforma de sentido único só conta como chão quando o pé cai dentro de uma janela de
~12 px do topo do tile. Um deslocamento de frame inteiro chega a 57 px — o pé **pula por cima
da janela** e o herói atravessa a plataforma. Quanto pior o frame rate, mais plataformas ele
atravessava; como várias estruturas só são escaláveis pousando na plataforma do topo, elas
viravam paredes, e a fase 1 ficava impossível.

Outros invariantes:

- `MoveX` testa pé, **meio** e cabeça (corpos altos passavam por saliências)
- `MoveY` sonda meio pixel abaixo do pé; depois de encaixar, o pé fica em `topo − 0,01`
- `OnGround` é recalculado em **todo** sub-passo descendente
- **Armadilha nunca prende.** Espinho/lava/espinheiro cobram 1 dano (lava 2) e `Rescue()` devolve
  o herói ao último chão seguro (`SafeX/SafeY`, gravado a cada 0,2 s) com 1,35 s de invulnerabilidade

### Combate

O golpe corpo a corpo não é uma hitbox instantânea: o arco fica **ativo entre 12 % e 88 %** da
animação, varrendo de −52° a +44°, testando o ponto mais próximo da caixa do alvo, e cada golpe
acerta cada alvo **uma vez** (`Actor.HitS0/HitS1` contra `PlayerA.Swing`). Acerto limpo dá
empurrão, tremor de câmera e *hit-stop* de 55 ms (`dt *= 0.2`).

O arco desenhado usa `reach * 1.39` porque a banda clara da textura `Slash` fica a 0,718 da
largura — assim **o que se vê coincide com o que corta**.

### Geração de fases

`LevelBuilder` compõe cada fase por "beats" (`intro`, `gauntlet`, `hall`, `check`, `lava`,
`cliffs`, `towers`, `nave`, `rooms`, `arena`…) e depois roda dois passes automáticos:

1. **`Ramp`** — monta um grafo de pontos de apoio e faz BFS do spawn até a coluna do sino.
   Uma aresta exige subida ≤ 5 tiles, `|dx| ≤ 4` **e silhueta livre entre as colunas**. Se o sino
   ficar inalcançável, empilha plataformas na frente da parede e tenta de novo (até 40 passes).
2. **`SealUnreachable`** — todo apoio abaixo da linha do chão que o BFS não alcança recebe hazard,
   para o resgate do `Sim` tirar o herói de lá. Nenhuma gaiola silenciosa sobrevive.

> A checagem de silhueta é o coração disso. Sem ela o BFS "pulava por cima" de paredes maciças de
> 8 tiles e declarava atravessáveis 16 fases cuja arena estava murada.

### Arte e som

**Nada de arte de cenário existe em disco.** `ArtGen.Ensure()` gera no boot: atlas de tiles
(11 células × 2 variantes, exposta e enterrada), FX, ícones, molduras 9-slice da UI, céu com
gradiente e estrelas, duas cristas de parallax e vinheta. Cada tile desenhado recebe tom por hash,
espelho horizontal em ~50 % e escurecimento por profundidade — é o que mata a aparência de grade.

Os **sprites de personagem** (77 PNG: 10 frames de herói, 32 inimigos, 17 chefes, 16 fundos,
`soul`, `heart`) ficam em `Assets/StreamingAssets/art/`. O que falta (`*_ground`, `key`, `bell`,
`slash`, `bolt`, `title`) tem fallback procedural.

`Sfx.cs` sintetiza 12 clipes a 22 050 Hz no boot (`jump attack magic hit stomp hurt die coin heart
boss clear ui`), tocados por um pool de 6 `AudioSource`, com guarda de 35 ms por id.

---

## Multiplayer local — como funciona de verdade

Não há servidor, nem conta, nem internet. **Os dois celulares precisam estar no mesmo Wi-Fi.**
Um cria a sala, o outro procura e entra.

```
   CELULAR A (host)                                CELULAR B (cliente)
┌───────────────────────┐                       ┌───────────────────────┐
│ DOIS CELULARES        │                       │ DOIS CELULARES        │
│ → CRIAR SALA          │                       │ → PROCURAR SALA       │
│                       │                       │                       │
│ NGO StartHost()       │                       │ LanDiscovery.Listen() │
│ UTP 0.0.0.0:7777      │                       │ UDP :47888            │
│                       │                       │                       │
│ LanDiscovery          │  UDP broadcast :47888 │                       │
│ .StartAdvertise() ────┼──────────────────────►│ lista de salas        │
│   "NIGHTFALL|{json}"  │      a cada 900 ms    │ "MIKE & DENYSE ·      │
│                       │                       │  192.168.1.3"         │
│                       │                       │        │              │
│                       │◄──────────────────────┼── StartClient()       │
│                       │      UDP/UTP :7777    │   toca na sala        │
│ OnClientConnected     │                       │                       │
│ SpawnGameNet() ───────┼──────────────────────►│ ClaimHeroRpc("denyse")│
│                       │                       │                       │
│ host escolhe a fase   │                       │ "espere — só o host   │
│ StartMatchRpc(7,…) ───┼──────────────────────►│  escolhe a fase"      │
│                       │                       │                       │
│ ═══ partida ═══       │                       │ ═══ partida ═══       │
│ Sim.Tick(dt)          │◄── SubmitInputRpc ────│ PollP1() → input      │
│ (autoridade total)    │       todo frame      │                       │
│ BuildSnap()           │                       │                       │
│ StateRpc(FrameSnap)───┼──────────────────────►│ ApplyRemote(snap)     │
│    20 Hz, unreliable  │                       │ Sim.TickRemote(dt)    │
└───────────────────────┘                       └───────────────────────┘
```

### 1. Descoberta — `Net/LanDiscovery.cs`

O host abre um `UdpClient` com broadcast na porta **47888** e, a cada 900 ms, envia:

```
NIGHTFALL|{"id":"a1b2c3d4e5f6","name":"Mike & Denyse","host":"192.168.1.3","port":7777,"kind":"lan"}
```

O pacote vai para `255.255.255.255` **e** para o broadcast calculado de cada sub-rede
(`ip | ~mask`), porque muitos roteadores domésticos descartam o broadcast global.

O trabalho de socket roda numa **thread de fundo**; os pacotes recebidos entram numa
`ConcurrentQueue` e só são interpretados em `LanDiscovery.Tick()`, na main thread. Isso não é
purismo: `Time.unscaledTime` e `SystemInfo.deviceUniqueIdentifier` lançam exceção fora da main
thread do Unity, e a versão anterior travava por isso.

**`LanIp.Pick()` — o IP certo, não qualquer IP.** Um celular Android tem várias interfaces ao
mesmo tempo (Wi-Fi, dados móveis `rmnet`, loopback, às vezes `p2p`). Cada uma é pontuada:

| endereço | pontos |   | interface | pontos |
|---|---|---|---|---|
| `192.168.x.x` | 40 | | `wlan` / `wifi` / `p2p` | +20 |
| `10.x.x.x` | 30 | | genérica | +5 |
| `172.16–31.x.x` | 28 | | `rmnet` / `wwan` / `cellular` | 0 |
| outro | 10 | | loopback / vmware / hyper-v | descartada |
| `169.254.x.x` (APIPA) | 1 | | | |
| `0.x`, `127.x`, `255.x` | descartado | | | |

Sem isso, a sala era anunciada com o IP dos **dados móveis** — o outro celular via um endereço
que não existia na rede dele. Era o sintoma de "IPs estranhos que mudam sozinhos".

Além disso: o aparelho **ignora o próprio `id`** (não aparece na própria lista), o `host` do JSON
vence o endereço de origem do pacote UDP quando tem pontuação maior, e a UI só se redesenha
quando o `Fingerprint()` da lista muda — antes os botões de sala eram destruídos e recriados
**todo frame**, a lista piscava e o toque não pegava.

No Android, `WifiDirectBridge.AcquireMulticastLock()` (JNI) pega o `WifiManager.MulticastLock`.
Sem ele, o sistema descarta pacotes de broadcast quando a tela apaga.

### 2. Transporte e sessão

Unity Netcode for GameObjects sobre Unity Transport, porta **7777**.

- **Host:** `SetConnectionData("0.0.0.0", 7777, "0.0.0.0")` + `StartHost()`, e então
  `SpawnGameNet()` instancia o prefab `Resources/GameNet` como `NetworkObject`.
- **Cliente:** `SetConnectionData(sala.host, sala.port)` + `StartClient()`.

O `GameNet` é o **único** `NetworkBehaviour` do jogo. Nenhum herói, inimigo ou projétil é um
`NetworkObject` — não há `NetworkTransform`, não há replicação de cena.

### 3. Modelo de autoridade: host-authoritative, cliente = espelho

**Só o host simula.** O cliente nunca roda `Sim.Tick`.

```csharp
if (_mode != "client") Sim.Tick(dt);        // host e solo
else                   Sim.TickRemote(dt);  // cliente: câmera, fade, animação — nada de física
```

Isso elimina de saída toda uma classe de bug de dessincronização: não existe "duas verdades".
O preço é latência de input, aceitável numa LAN (~2–10 ms de RTT).

### 4. Os cinco RPCs — `Net/GameNet.cs`

| RPC | direção | entrega | o que faz |
|---|---|---|---|
| `SubmitInputRpc(InputMsg)` | cliente → host | confiável | 6 bits: `L R D Jp Jn Ap` |
| `ClaimHeroRpc(string)` | cliente → host | confiável | cliente informa qual herói escolheu |
| `StartMatchRpc(world, h1, h2)` | host → todos | confiável | host abriu a fase; ambos entram |
| `StateRpc(FrameSnap)` | host → clientes | **não confiável** | snapshot de apresentação, 20 Hz |
| `EndMatchRpc(reason)` | host → clientes | confiável | host abandonou a fase / voltou ao título |

**`Jp`/`Ap` são pegajosos.** O input do cliente chega mais rápido que o tick do host; um pulo
enviado entre dois ticks se perderia. Então o `SubmitInputRpc` faz `OR` dos bits de "pressionar"
e o host os limpa **depois** de consumir:

```csharp
// no RPC, no host
bool jp = LastFromClient.Jp || input.Jp;
LastFromClient = input;
LastFromClient.Jp = jp;

// no Update, depois do Sim.Tick
GameNet.LastFromClient.Jp = false;
```

**Cliente que chega atrasado.** Se o jogador 2 conecta depois de o host já ter começado a fase,
`OnClientConnected` reenvia `StartMatchRpc` — sem isso o cliente ficava parado no menu para sempre.

**Pausa não é propagada.** O host manda `SimState = "play"` mesmo pausado; o cliente só vê o
mundo parar. Propagar a pausa abriria o painel de pausa no celular do outro jogador.

**Renascer / próxima fase.** No cliente, `UiRetry` e `UiNext` **não** iniciam partida local —
quem manda é o `StartMatchRpc` do host. Iniciar dos dois lados dessincronizava a fase.

### 5. `FrameSnap` v2 — o snapshot

A v1 só levava os dois heróis. O resultado: no celular do jogador 2, item coletado continuava na
tela, projétil não existia, golpe não aparecia e inimigo morto ficava em pé. A v2 carrega
**tudo que é preciso para desenhar o frame**:

| campo | conteúdo | por que existe |
|---|---|---|
| `PlrSnap P0, P1` | x, y, hp, vidas, facing, anim, **progresso do golpe**, flags (inv/hurt/morto), herói | o cliente desenha o arco do golpe na fase certa da animação |
| `ItemMask` (`ulong`) | 1 bit por item ainda no chão | coleta some para os dois. `ItemA.Taken` substituiu `RemoveAt` para o índice do bit ficar estável |
| `ProjSnap[12]` | x, y, vx, vy, tipo, amigo/inimigo | cristais e projéteis de chefe; o cliente reconstrói a lista a cada snapshot |
| `EntSnap[20]` | x, y, facing, hp, **`Kind`** (índice do catálogo), flags morto/flash | inimigos **invocados pelo chefe** nascem com a arte certa no cliente |
| `FxSnap[8]` | corte, faísca, morte, coleta, cura, dano, impacto, resgate | são eventos, não estado — por isso vão em fila |
| `Boss*` | hp, max, pos, facing, `BossKind` | `BossKind` troca para Belial no mundo 16 |
| `Msg`, `MsgT`, `Shake`, `Score`, `Souls`, `LockArena`, `SimState` | HUD e estado da tela | |

Dois cuidados que custaram caro:

- **Tamanho.** A primeira v2 chegou a ~1,1 KB. O pipeline **confiável** do NGO não fragmenta e
  engasga perto do MTU. Os campos viraram `byte`/flags, os arrays foram cortados para 20/12/8
  (~800 B) e o `StateRpc` passou a `RpcDelivery.Unreliable` — que é o certo para snapshot de
  estado: um frame perdido é substituído 50 ms depois, e não vale a pena reenviá-lo.
- **Interpolação.** O chefe faz `Lerp(pos, alvo, 0.5)` para movimento suave, mas **teleporta** se
  a distância passar de 260 px — senão um respawn vira um deslizar lento pela tela inteira.

### 6. Wi-Fi Direct

`WifiDirectBridge` sabe criar grupo e descobrir peers via JNI, mas o caminho que **funciona de
verdade** é *mesmo Wi-Fi + Criar sala (LAN)*. O Wi-Fi Direct saiu do menu porque produzia
endereços `192.168.49.x` / `169.254.x.x` que confundiam a descoberta.

### Permissões Android usadas

`INTERNET`, `ACCESS_NETWORK_STATE`, `ACCESS_WIFI_STATE`, `CHANGE_WIFI_STATE`,
`CHANGE_WIFI_MULTICAST_STATE`, `ACCESS_FINE_LOCATION`, `ACCESS_COARSE_LOCATION`,
`NEARBY_WIFI_DEVICES`. `usesCleartextTraffic="true"` — o tráfego é LAN puro.

---

## Detalhes do projeto

### Catálogo

**Heróis**

| id | arma | hp | speed | jump | alcance |
|---|---|---|---|---|---|
| `mike` | lâmina (melee) | 6 | 3.55 | −12.2 | 58 px |
| `denyse` | cristal (projétil) | 5 | 3.85 | −11.8 | 220 px |

Denyse tem gravidade 23 e *float*: segurar o pulo no ar limita a queda a 3.2.

**Os 16 mundos**

| # | id | nome | hazard | chefe |
|---|---|---|---|---|
| 1 | `forest` | Floresta Amaldiçoada | thorn | treant |
| 2 | `cabin` | Cabana da Bruxa | spike | babawitch |
| 3 | `swamp` | Pântano de Ossos | water | hydra |
| 4 | `desert` | Deserto de Cinzas | spike | pharaoh |
| 5 | `grave` | Cemitério Sombrio | spike | gravetitan |
| 6 | `castle` | Castelo Vampírico | spike | nosferatu |
| 7 | `ice` | Ermos Congelados | ice | icequeen |
| 8 | `volcano` | Inferno Vulcânico | lava | magmatitan |
| 9 | `coven` | Covil das Bruxas | thorn | morgana |
| 10 | `catacombs` | Catacumbas Afogadas | water | drownedking |
| 11 | `woods` | Bosque do Lobisomem | thorn | alpha |
| 12 | `peak` | Pico do Dragão | spike | dragon |
| 13 | `cathedral` | Catedral Demoníaca | spike | cathedral |
| 14 | `village` | Vila da Lua de Sangue | spike | priestess |
| 15 | `abyss` | Cavernas do Abismo | spike | leviathan |
| 16 | `throne` | Trono Infernal | lava | warden → **belial** |

32 inimigos com IA `patrol`, `jump`, `swoop`, `charge`, `fly`, `shoot`, `mage`, `tank`.
16 chefes + Belial, com padrões `slam`, `hex`, `multi`, `summon`, `bat`, `ice`, `lava`, `wave`,
`charge`, `flyfire`, `final`.

**Tiles** — `0` vazio · `1` sólido · `2` plataforma · `3` espinho · `4` lava · `5` gelo ·
`6` água · `7` quebrável · `8` mola · `9` escada · `10` espinheiro. `TILE = 40`, view `960×540`.

### Regras de jogo

- Coyote 0,12 s · jump buffer 0,14 s · pulo variável (soltar com `vy < −3` corta o impulso)
- Queda do penhasco dá 1 pulo extra se o coyote já expirou
- *Stomp*: `vy > 1.2` e pés acima de 58 % do corpo do inimigo
- 1,15 s de invulnerabilidade ao tomar dano; 3 vidas por herói; os dois mortos = `dead`
- A arena trava quando um vivo passa de `arenaX0 + 40`
- Depois do chefe, tocar o **sino** encerra a fase
- Mundo 16: Warden morre → Belial nasce; Belial morre → vitória
- Gelo: piso sólido com menos aderência (`accel 6`, `vx *= 0.992`)
- Mola: plataforma **sólida** que lança para cima (`vy = −11.2`)
- Vento nas beats `cliffs` e `wind`

### Cliente HTML5 de referência

`game/` é o mesmo jogo em HTML5/Canvas (`engine.js`, `data.js`, `levels.js`, `input.js`,
`net.js`, `audio.js`), empacotado em `android/` como WebView. **Ele é o spec**: mesmos IDs, mesmas
fases, mesmas constantes. Quando Unity e JS discordam sem motivo, o JS é a regra — exceto na mola,
que o JS também erra (deve ser sólida **e** dar impulso).

---

## Como buildar e rodar

**Pré-requisitos:** Unity `6000.5.8f1` com o módulo Android, Android SDK/NDK, Python 3 + Pillow
(só para as ferramentas de arte).

### APK

```bash
"D:\UnityEditors\6000.5.8f1\Editor\Unity.exe" -quit -batchmode -nographics -projectPath "NightfallUnity" -executeMethod Nightfall.Editor.AndroidBuilder.Build -logFile "unity-build.log"
```

`AndroidBuilder.Build()` roda o smoke antes e **aborta se algum check falhar**. Depois ele:
monta a cena `Boot.unity` e o prefab `GameNet`, copia `StreamingAssets/art` → `Resources/Art`,
fura o magenta residual dos sprites, aplica os importers (point filter nos sprites, bilinear nos
`*_bg`) e chama o `BuildPipeline`.

> O caminho de saída do APK está fixo em `AndroidBuilder.cs` (`outPath`). Ajuste para a sua máquina.

### Jogar em dois celulares

1. Instale o mesmo APK nos dois aparelhos.
2. Ponha os dois **no mesmo Wi-Fi** (mesma rede — não Wi-Fi Direct, não dados móveis).
3. Celular A: **DOIS CELULARES → CRIAR SALA**. Aparece `Sala pronta · 192.168.x.x:7777`.
4. Celular B: **DOIS CELULARES → PROCURAR SALA**. A sala aparece na lista com o IP do host.
5. Celular B toca na sala → **CONTINUAR** → escolhe o caçador.
6. Celular A escolhe a fase no mapa e toca **COMEÇAR A FASE**. Os dois entram juntos.

**Se a sala não aparece:** confirme que os dois estão na mesma rede. Algumas redes de hotel,
corporativas ou com *AP isolation* bloqueiam broadcast UDP entre clientes — um hotspot de celular
resolve.

### Cliente HTML5

```bash
python tools/package.py
```

Gera os ícones do launcher e copia `game/` → `android/app/src/main/assets/www`.

---

## Testes

O jogo é testado **sem aparelho**, em batchmode. Isso não é conveniência — é o que impediu que um
jogo intransponível fosse entregue duas vezes.

```bash
"D:\UnityEditors\6000.5.8f1\Editor\Unity.exe" -quit -batchmode -nographics -projectPath "NightfallUnity" -executeMethod Nightfall.Editor.Smoke.Run -logFile "unity-smoke.log"
```

**77 checks**, entre eles:

| check | o que garante |
|---|---|
| `CheckTraversal` | BFS do spawn alcança o sino nas 16 fases, **com silhueta livre entre as colunas** |
| `CheckNoTrap` | nenhum piso inalcançável sem hazard — nenhuma gaiola silenciosa |
| `CheckPlatforms` | um herói solto sobre uma plataforma **pousa** a 60, 30 e 20 fps |
| `CheckPlayable` | um **bot** segura direita e pula em parede, buraco e hazard; precisa chegar à arena nas 16 fases |
| `CheckFullLoop` | o mesmo bot fecha as fases 1, 8 e 16 inteiras — rota, chefe, sino e a 2ª fase de Belial |
| `CheckSnapshot` | máscara de itens ida e volta, índices de catálogo, 10 s de sim parada sem morrer |

`Smoke.Survey` roda **8 sementes × 16 fases** fora do build e mede a taxa de sucesso.
Estado atual: **16/16 fases, 8/8 sementes, 128 execuções sem falha.**

> Lição gravada no projeto: checagem de *geometria* não é teste de *jogabilidade*. A v3.0 declarou
> as 16 fases atravessáveis por um BFS e entregou um jogo impossível de terminar. Só um bot que
> roda a física real pega isso. E teste sem semente fixa não vale: o mesmo commit passou solto e
> falhou dentro do build.

Outras ferramentas:

```bash
Unity.exe ... -executeMethod Nightfall.Editor.ArtDump.Run   # atlas, mapas de fase e frames em tools/artdump/
python tools/test_progress.py                               # regras de progressão / PlayerPrefs
node  tools/smoke_levels.js                                 # as 16 fases do cliente JS
```

---

## O que está no repositório e o que não está

Só entra o que é **fonte**. Tudo que uma ferramenta regenera fica de fora:

| fora do repo | regenerado por |
|---|---|
| `NightfallUnity/Library/`, `Logs/`, `.utmp/` | o próprio Unity, ao abrir o projeto |
| `NightfallUnity/Assets/Resources/Art/` | `AndroidBuilder.PrepareProject()`, a partir de `StreamingAssets/art` (que **está** versionado) |
| `android/app/src/main/assets/www/` | `python tools/package.py`, a partir de `game/` (cópia byte a byte) |
| `android/app/build/`, `.gradle/`, `local.properties` | Gradle |
| `*.apk`, `*_BurstDebugInformation_DoNotShip/` | o build |
| `tools/artdump/`, `*.log`, `*.pid` | `ArtDump.Run` e execuções de ferramenta |
| `_unused/`, `UnityNightfall/` | cópias mortas (ver `CONTEXT.md` → "Mapa do repositório") |

Os 77 sprites de personagem vivem em `NightfallUnity/Assets/StreamingAssets/art/` e são a fonte
da verdade. A arte de cenário não existe em disco: nasce em `ArtGen.cs` no boot.

---

## Documentação viva

Três arquivos que valem tanto quanto o código, e que **toda mudança deve atualizar**:

| arquivo | papel |
|---|---|
| [`CONTEXT.md`](CONTEXT.md) | o que o jogo **é**: catálogo, invariantes, mapa do repositório, regras que não podem quebrar |
| [`PLAN.md`](PLAN.md) | o que já foi feito e o que falta — com a causa raiz de cada bug, não só o sintoma |
| [`MULTIAGENTS.md`](MULTIAGENTS.md) | diário de trabalho: quem fez o quê, o que foi descoberto, o que foi decidido |

### Convenções

- Não introduzir Tilemap nem Rigidbody2D — a simulação é 100 % código, Y para baixo
- IDs de sprite = id do catálogo (`treant.png`, `mike_walk1.png`, `forest_bg.png`)
- Português nos textos de UI, inglês minúsculo nos IDs
- Crash > softlock > gameplay > polish
- Não commitar `Library/`, APKs ou caches do Unity Hub
