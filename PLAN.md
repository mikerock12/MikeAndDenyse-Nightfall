# PLAN — Mike & Denyse: Nightfall

Última atualização: 2026-08-17  
Estado: v3.0 — armadilhas resolvidas, coop sincronizado, arte refeita, pausa/sair no lugar.

Este arquivo é o registro vivo do plano. Toda mudança de rumo, bug encontrado, correção e verificação entra aqui. Ler junto com `CONTEXT.md` e `MULTIAGENTS.md`.

---

## Objetivo

Entregar o jogo **jogável de ponta a ponta no Android**, exatamente como pedido:

- Platformer cooperativo em landscape
- Mike (lâmina) e Denyse (cristal)
- 16 fases, 32 monstros, 16 chefes, 1 chefão (Belial)
- Coop no mesmo aparelho e LAN / Wi-Fi Direct
- Fluxo: título → heróis → (rede) → mapa → fase → pausa / morte / vitória

O APK que o usuário está rodando é o Unity (`com.mikeanddenyse.nightfall`, projeto `NightfallUnity`). A versão HTML/WebView em `game/` + `android/` é o spec de referência.

---

## Diagnóstico do screenshot

Arquivo: `Screenshot_2026-08-16-13-38-44-471_com.mikeanddenyse.nightfall.jpg`

A captura é a **tela-título** do Unity (`GameMenu.TitlePanel`), com o erro em vermelho:

```
ArgumentOutOfRangeException: Index and length must refer to a location within the string.
Parameter name: length
```

Isso é `String.Substring(start, length)` em C#. A mensagem só aparece no título porque o `Text` de erro (`_error`) foi criado **somente** no painel `title`. O jogador tenta entrar na fase, a partida quebra, volta ao menu e vê o texto.

Causa raiz confirmada em `NightfallUnity/Assets/Scripts/WorldView.cs` → `Hex`:

```csharp
h.Substring(0, 2)  // ok
h.Substring(2, 4)  // length 4 a partir do índice 2 — acidentalmente cabe em 6 chars
h.Substring(4, 6)  // length 6 a partir do índice 4 → índice final 10 > 6 → explode
```

O JS equivalente usa `slice(0,2)`, `slice(2,4)`, `slice(4,6)` (fim exclusivo). O C# foi portado como se o segundo argumento fosse índice final.

`Hex` roda em:

1. `PrepareWorld()` / `BuildAtlas()` — **ao apertar Começar a fase** (ou segundo toque na fase)
2. `OnGUI` a cada frame (`DrawBg`, `DrawCheckpoint`, `DrawBell`) — se a fase chegasse a abrir, travaria no primeiro frame

Por isso o jogo “não inicia” e parece preso na seleção de fase.

Agravante de UX: `_world` começa em `0`. O primeiro toque em “1. Floresta Amaldiçoada” já conta como “toque de novo” e chama `UiStartWorld()` imediatamente → crash na hora.

---

## Fases de trabalho

### Fase 0 — Registro (esta rodada)

- [x] Ler screenshot e identificar o APK Unity
- [x] Mapear scripts, catálogo, arte, rede
- [x] Criar `PLAN.md`, `CONTEXT.md`, `MULTIAGENTS.md`
- [x] Disparar agentes de auditoria (JS, Unity, arte)
- [x] Auditores voltaram: spec JS, 21 bugs Unity, inventário 77/98 sprites

### Fase 1 — Destravar o jogo (bloqueante)

- [x] Corrigir `WorldView.Hex` com `ColorUtility.TryParseHtmlString`
- [x] Proteger `OnGUI` (`DrawFrame` em try/catch)
- [x] Não iniciar a fase no primeiro toque da fase já pré-selecionada (`_mapTap`)
- [x] Banner de erro no root do canvas (visível no mapa, não só no título)
- [x] Limpar o erro ao abrir uma fase com sucesso
- [x] `MaybeLock` nulo-seguro
- [x] Não spawnar P2 fantasma no solo local (matava o jogo na arena)
- [x] Multi-toque: pads lidos via `Input.touches`, não EventSystem de 1 ponteiro
- [x] Cliente: `TickRemote` (fade + câmera) e spawn visual do chefe

### Fase 2 — Jogabilidade alinhada ao spec JS

- [x] Gelo: accel 6 + `vx *= 0.992`
- [x] Mola (`T.Bounce`): sólida + `vy = -11.2` (Unity e JS)
- [x] Vento nas beats `cliffs` / `wind`
- [x] Padrões de chefe: `cast`, `summon`, `air`, `wave`
- [x] Controles P2 iguais ao JS (N, Numpad, vírgula, Z no P1)
- [x] LAN: `Clock()` + device id cacheado na main thread
- [x] `CheckClear` não sobrescreve `win`
- [x] Voltar na tela de heróis
- [x] Consumir `Jp`/`Ap` no cliente e no host

### Fase 3 — Arte, itens, FX

- [x] Inventário: 10 herói + 32 inimigos + 17 chefes + 16 bgs + soul/heart = 77 PNG presentes
- [x] `mike_idle` Resources já tem alpha (0 magenta opaco)
- [x] Faltam (fallback ok): 16 `*_ground`, key, bell, slash, bolt, title
- [x] Sprite.Create protegido + punch magenta se a textura for readable
- [ ] Retratos na seleção dependem de Sprite.Create (texturas `isReadable: 0` — fallback se falhar)

### Fase 4 — Rede

- [x] Host LAN + advertise UDP 47888 (thread-safe)
- [x] Cliente scan + join (já existia)
- [x] Cliente não fica tela preta (fade decai)
- [x] Wi-Fi Direct ainda só cria grupo / discover — LAN é o caminho que funciona
- [ ] Handshake P2P completo + permissão runtime (não bloqueia local)

### Fase 6 — v3.0 (2026-08-17)

- [x] Armadilha: chão seguro + resgate; `Vy = -8` por frame removido
- [x] Geometria: `rooms`, `towers`, `nave`, `hall`, `trees`, `ruins`, `cliffs` destravados
- [x] `Ramp` + `SealUnreachable`: rota garantida e poços sem saída selados
- [x] Belial spawnado por fila (fim do `InvalidOperationException` no meio do tick)
- [x] `OnGUI` com `try/finally` — grupo IMGUI nunca fica aberto
- [x] `SyncScreen()` reconcilia tela · sim · painel todo frame
- [x] Mundo continua visível atrás de pausa / morte / vitória
- [x] `FrameSnap` v2: itens, projéteis, FX, tipo de inimigo, vidas, mensagem
- [x] Cliente não inicia partida local em Renascem / Próxima fase
- [x] `EndMatchRpc` quando o host abandona
- [x] Botão voltar hierárquico; pausa com Continuar / Mapa / Menu inicial
- [x] Sair no título (`Application.Quit`)
- [x] `ArtGen.cs`: atlas com silhuetas, FX, ícones, molduras, céu, cristas, vinheta
- [x] Variação por tile (tom + espelho + profundidade) — fim da grade
- [x] Céu corrigido (horizonte claro); parallax volta a aparecer
- [x] `Sfx.cs`: 12 efeitos sintetizados
- [x] Texturas de fase liberadas em `PrepareWorld` / `OnDestroy` (vazamento por partida)
- [x] Smoke 20 → 55 checks; `ArtDump` para conferir arte sem aparelho
- [ ] Teste em aparelho real (sem `adb devices` nesta máquina)

### Fase 5 — Verificação

- [x] Fluxo título → herói → mapa → `BeginMatch` não chama mais `Substring(4,6)`
- [x] Smoke JS das 16 fases (`tools/smoke_levels.js`) — OK
- [x] Prova Python: Hex antigo explode em `#2a4a22`; o novo devolve RGB
- [x] Rebuild do APK Unity — sucesso 2026-08-16 14:13, `MikeAndDenyse-Nightfall-NGO.apk` (163 MB), 0 `error CS`
- [ ] Instalar no aparelho (nenhum `adb devices` ligado nesta máquina) e confirmar que a exception vermelha sumiu

---

## Ordem de patches (esta sessão)

1. `WorldView.cs` — Hex + try/catch no OnGUI + cores bounce/break
2. `GameMenu.cs` — banner, multi-toque, voltar, tap copy
3. `NightApp.cs` — tap do mapa, limpar erro, teclas P2, solo sem P2 fantasma, fade cliente
4. `Sim.cs` — bounce, gelo, vento, chefes, MaybeLock, EnsureSecond, TickRemote
5. `LevelBuilder.cs` / `GameData.cs` — zonas de vento, Boss(null)
6. `LanDiscovery.cs` — relógio e device id thread-safe
7. `SpriteBank.cs` — Sprite.Create protegido
8. JS espelho (`game/js/engine.js` + `android/.../www/js`) — bounce + gelo
9. Registrar tudo nos três arquivos

---

## Fora de escopo nesta rodada

- Reescrever o motor em sprites Unity 2D / Tilemap
- Áudio procedural completo (JS tem `audio.js`; Unity `Sfx` está vazio — não bloqueia jogar)
- Netcode com sync de todos os inimigos (cliente só interpola heróis/chefe)
- Gerar sprites de FX, sino, chave e ground tiles se o jogo já tem fallback

---

## Rodada 2026-08-16 (single / save / coop)

Pedido: erro.jpeg (mesma exception no título), tirar “mesmo aparelho”, single player, fases em ordem com save, coop só em dois celulares, testar antes do APK.

- Título: **Um jogador** / **Dois celulares** / Controles
- Sem P2 no mesmo aparelho
- `Progress` (PlayerPrefs): só a fase 1 aberta; vencer libera a seguinte; dá para repetir as já abertas
- Coop: só o host vê o mapa e escolhe; cliente espera
- Exception crua de pacote Unity **não** pinta mais o título
- Fonte: `LegacyRuntime.ttf` (evita CreateDynamicFont no Android)
- Smoke `Nightfall.Editor.Smoke` + `tools/test_progress.py` antes do APK
- Wi-Fi Direct saiu do menu (era fonte de IP 192.168.49 / 169.254)
- Testes: `tools/test_progress.py` OK; Unity `SMOKE OK 20 checks`; APK 2.6 (20:48) `MikeAndDenyse-Nightfall-NGO.apk`

---

## Rodada 2026-08-16 (coop Wi-Fi)

Relato: IPs estranhos que mudam sozinhos, lista instável, partida não sincroniza.

Causas:
1. O host escutava o próprio broadcast em várias interfaces (127, 169.254, celular, Wi-Fi). Cada pacote sobrescrevia o IP da sala.
2. `LocalIp()` usava rota para 8.8.8.8 → podia ser IP de dados móveis.
3. `RefreshRooms` destruía os botões **todo frame** — a lista piscava e o toque falhava.
4. `JsonUtility` e o IP vinham do endereço UDP, não do IPv4 anunciado.
5. Cliente que ligava depois do host iniciar a fase nunca recebia `StartMatchRpc`.
6. Snapshot só tinha os dois heróis — inimigos no cliente ficavam congelados.

Patches:
- `LanIp.Pick()` escolhe 192.168/10/172 e ignora loopback/APIPA/celular
- Pacote `NIGHTFALL|` agora leva `host` estável; eco do próprio aparelho é descartado
- Parse na main thread; UI só redesenha se o fingerprint mudou
- Cliente tardio recebe `StartMatchRpc` de novo
- `FrameSnap` manda até 20 inimigos + chefe
- Rebuild 2026-08-16 19:20 — `MikeAndDenyse-Nightfall-NGO.apk` (163 628 685 bytes), 0 `error CS`

---

## Rodada 2026-08-16 (controles / sprites)

Relato do usuário: sem controles, boneco e inimigos de cabeça para baixo, atores fora do cenário.

Causas:

1. **Toque morto.** O mundo é desenhado em `OnGUI` (por cima do canvas). Os pads uGUI ficavam invisíveis e o EventSystem não recebia o dedo. `PollTouches` em `GameMenu.Update` ainda rodava *depois* do `Sim.Tick`.
2. **Sprite invertido ao virar.** `GUI.DrawTexture(..., ScaleToFit)` com `width` negativo vira X **e** Y. Inimigo/herói olhando para a esquerda = de cabeça para baixo.
3. **Fora do cenário.** Sem clamp de X, um passo no vazio ou um stomp mandava o ator para fora da câmera; o flip quebrado piorava a leitura.

Patches desta rodada:

- `PlayTouch.cs` — zonas grandes em espaço 960×540, lidas de `Input.touches` **antes** do Tick
- `WorldView` desenha os pads por cima do mundo (◀ ▶ ▼ PULO GOLPE II P2)
- `DrawSpr` usa `DrawTextureWithTexCoords` + UV-X, pés alinhados, **nunca** width negativo
- `Sim` prende o herói em `4 … cols*tile` e não deixa Y < −80
- `PlayHud` uGUI vazio (não compete mais com o OnGUI)
- Rebuild 2026-08-16 18:38 — `MikeAndDenyse-Nightfall-NGO.apk` (163 609 085 bytes), 0 `error CS`

---

## Rodada 2026-08-17 · v3.1 (física de plataforma, arena murada, combate)

Relato do usuário: fases ainda intransponíveis (**inclusive a primeira**), "torre reta que não tem
como pular", plataformas que o personagem atravessa, e golpes sem peso.

### A. Plataformas atravessadas — a causa de tudo

`Sim.Phys` aplicava o deslocamento do frame **inteiro de uma vez**, e uma plataforma de sentido
único só conta como chão quando o pé cai dentro dos 10 px do topo do tile:

```csharp
a.Y += a.Vy * 72 * dt;                                  // 14 px a 60 fps, 28 px a 30 fps
if (t == T.Platform && falling && ModTile(py) < 10) …   // janela de 10 px
```

O pé **pulava por cima da janela**. Não era aleatório: quanto pior o frame rate, mais plataformas
o herói atravessava. E como várias estruturas só são escaláveis pousando na plataforma do topo,
elas viraram paredes — a "torre reta" da fase 1 é o tronco de árvore, cuja única saída é a
plataforma da copa.

Correção: a física move em **sub-passos de no máximo 6 px** (`Phys` → `MoveX` / `MoveY`), então
nada mais tunela — nem plataforma, nem chão fino, nem parede em velocidade. A sondagem de repouso
mede meio pixel abaixo do pé, porque depois do encaixe o pé fica logo acima da superfície.

### B. Arena do chefe murada — em todas as 16 fases

`StampArena` erguia `Solid(x, b-8, 1, 8)` nas duas bordas: colunas maciças da linha 7 até o chão.
**Nenhuma fase podia ser terminada, em nenhum mundo.** As batentes agora ficam suspensas
(linhas 7–11), deixando passagem.

Por que o teste de rota da v3.0 não pegou: o BFS ligava duas colunas olhando só altura e distância,
**sem olhar o que havia no meio** — então "pulava por cima" de uma parede de 8 tiles. Agora cada
salto verifica a silhueta das colunas intermediárias (`Clear`).

### C. Verificação que faltava: um bot que joga

Checagem de geometria não bastava — ela passava enquanto o jogo estava intransponível. Foram
adicionados dois testes que rodam a **física real**:

- `CheckPlatforms` — derruba o herói sobre uma plataforma a 60, 30 e 20 fps e exige que ele pouse
- `CheckPlayable` — um bot segura direita e pula em parede, buraco e hazard; precisa chegar à arena
  nas **16 fases**
- `CheckFullLoop` — o mesmo bot fecha as fases 1, 8 e 16 inteiras: rota, chefe e sino
  (inclui a segunda fase do Belial)

Smoke: 55 → **77 checks**.

### D. Combate

| antes | agora |
|---|---|
| hitbox instantânea no frame do toque | arco ativo varrendo de cima para baixo entre 12 % e 88 % do golpe, uma vez por alvo |
| inimigo só piscava | empurrão de 4,4 (chefe 1,1), leve pop vertical, atordoamento maior |
| sem retorno de impacto | *hit-stop* de 55 ms em câmera lenta + tremor |
| golpe não movia o herói | passo de investida (1,1 no chão, 0,35 no ar) |
| arco desenhado solto no mundo | arco preso ao herói, com 3 rastros e ponta luminosa, no mesmo raio que a hitbox testa |
| cristal saía instantâneo | conjuração de 0,35: anel que implode na mão, depois disparo com recuo |
| projétil era um borrão | cauda de cometa de 5 ecos, anel rúnico girando, estrela no núcleo, estouro de cacos no impacto |

Alcance do Mike 46 → 58 px (o arco desenhado agora bate com o alcance real).

Também: **▼ desce pela plataforma** de sentido único.

Ajuste feito por causa do bot: a investida no ar era 1,9 e o golpe aéreo prendia a queda em 1,5 —
virava planeio, e o herói passava voando das plataformas. Reduzido, e o freio de queda removido.

### E. O que o levantamento com o bot encontrou depois

O teste inicial era **não-determinístico** (`Sim.Start` sorteia direção de inimigo, o chefe sorteia
padrão): passou solto e falhou dentro do build, no mesmo commit. Agora cada fase roda com sementes
fixas, e `Smoke.Survey` faz um levantamento de 8 sementes × 16 fases fora do build.

Primeiro levantamento: **15 fases 8/8, Trono Infernal 6/8**. O diagnóstico veio da própria medição,
não de palpite — `mortes=14, terminou na coluna 63` com melhor avanço na 89. Não estava travado:
morria na travessia de lava e voltava ao **início da fase**, porque o santuário do mundo 16 ficava
*depois* da lava.

- travessia de lava com pedras de 4 e 5 tiles (eram 3) e vãos de 1 tile, e o inimigo voador tirado
  de cima do salto mais difícil
- santuário do Trono Infernal movido para antes da lava:
  `intro,gauntlet,hall,check,lava,…`

Levantamento final: **16/16 fases, 8/8 sementes, 128 execuções sem falha.**

---

## Rodada 2026-08-17 (v3.0 — armadilhas, coop, arte, pausa)

Relato do usuário, item por item, e o que foi feito.

### 1. Armadilha prende o herói até morrer  ✅

**Causa raiz.** `Sim.UpdatePlayer` fazia, *todo frame* em que o corpo tocava espinho/lava/espinheiro:

```csharp
if (hz == Spike || hz == Lava || hz == Thorn) { Hurt(p, ...); p.Vy = -8; }
```

`Hurt` respeitava a invulnerabilidade, mas `p.Vy = -8` **não**. Dentro do fosso o herói ficava
com a velocidade vertical reescrita a cada frame: o pulo nunca saía (o impulso era sobrescrito)
e ele quicava a ~88 px, abaixo dos 120 px necessários para sair, perdendo vida a cada 1,15 s
até acabarem as três. Era exatamente o "não consegue sair, o pulo não sai, fica até morrer".

**Correção.**

- `PlayerA.SafeX/SafeY/SafeT` — memória do último chão honesto (sólido, sem hazard no corpo),
  atualizada a cada 0,2 s enquanto o herói está apoiado.
- `HazardHit` cobra **um** dano e chama `Rescue`, que devolve o herói ao chão seguro com 1,35 s
  de invulnerabilidade e FX de resgate. Nada de `Vy` sobrescrito.
- Cair para fora do mapa usa o mesmo caminho.

### 2. Fases intransponíveis (bug estrutural, não relatado mas real)  ✅

Auditando o `LevelBuilder` apareceram **paredes que fecham a fase**:

| beat | antes | efeito |
|---|---|---|
| `rooms` | paredes laterais de 8 tiles até o chão | cabana (fase 2) fechava logo na entrada |
| `towers` | torres de 8 e 10 tiles no chão | castelo (6) e catedral (13) travavam |
| `nave` | colunas do teto até o chão | catedral (13) travava |
| `hall` | pilares de 6 tiles | corredores intransponíveis |
| `trees`, `ruins` | 5 tiles (limite exato do pulo) | passagem por sorte |
| `cliffs` | abismo 9 tiles abaixo da borda, sem hazard | **softlock permanente**: caía e não morria |

Correções: toda estrutura no chão agora sobe no máximo 4 tiles ou deixa vão de passagem;
o abismo do `cliffs` ganhou piso de hazard e plataformas de saída.

Além disso, dois passes automáticos no fim de `Compile`:

- **`Ramp`** — grafo de pontos de apoio, BFS do spawn; se o sino ficar inalcançável, empilha
  plataformas na frente da parede que bloqueia, e repete (até 40 vezes).
- **`SealUnreachable`** — todo apoio abaixo da linha do chão que o BFS não alcança recebe hazard,
  para o resgate do `Sim` tirar o herói de lá. Nenhuma gaiola silenciosa sobrevive.

Hoje as 16 fases compilam com `sealed = 0`: a geometria já está correta sem precisar dos reparos.

### 3. Tela travada ao morrer  ✅

Três causas somadas:

1. `DmgEnemy` fazia `Bosses.Add(Belial)` **durante** o `foreach (var b in Bosses)` do `Melee` /
   `UpdateProj` → `InvalidOperationException` no meio do tick.
2. `WorldView.OnGUI` abria `GUI.BeginGroup` e só fechava no fim. Uma exceção no meio deixava o
   clip aberto: o IMGUI quebra para o resto da execução e **o mundo inteiro some** — preto.
3. `Show("dead")` escondia a view; se a transição falhasse, não sobrava view nem painel.

Correções: fila `_spawnQueue` esvaziada fora dos laços; `OnGUI` com `try/finally` que fecha todo
grupo aberto; `NightApp.SyncScreen()` reconcilia telaꞏsimꞏpainel **todo frame**; e a view continua
visível atrás de pausa, morte, vitória e fim de fase.

### 4. Coop Wi-Fi: o cliente não via nada  ✅

O `FrameSnap` v1 levava só os dois heróis + chefe + 20 inimigos. O cliente não roda simulação,
então **item coletado continuava na tela, projétil não existia e golpe não aparecia**.

`FrameSnap` v2 (`Net/GameNet.cs`):

| campo | resolve |
|---|---|
| `PlrSnap` (anim, progresso do golpe, inv, hurt, vidas, herói) | golpe e dano visíveis no outro celular |
| `ItemMask` (64 bits) | coleta some para os dois; `ItemA.Taken` no lugar de `RemoveAt` mantém o índice estável |
| `ProjSnap[16]` | cristais e projéteis de chefe aparecem e se movem |
| `FxSnap[10]` | faísca, corte, morte, coleta, cura, impacto de chefe |
| `EntSnap.Kind` | inimigos invocados pelo chefe nascem certos no cliente |
| `Msg` / `MsgT` / `Shake` | mensagens e tremor iguais nos dois |

Também: `SubmitInputRpc` guarda `Jp`/`Ap` até o host consumir (pulo não se perde entre ticks);
`UiRetry`/`UiNext` do cliente não iniciam mais partida local (dessincronizava); `EndMatchRpc`
avisa o cliente quando o host abandona.

### 5. Botão voltar e sair  ✅

- `BackPressed()` hierárquico: jogo → pausa · pausa → continua · qualquer menu → tela anterior ·
  título → fecha o jogo.
- Pausa com **Continuar · Voltar ao mapa · Menu inicial**.
- Título com **Sair** (`Application.Quit`).

### 6. Arte  ✅

`ArtGen.cs` (novo) gera tudo por código:

- **Tiles com silhueta de verdade** — espinho vira cone de metal, espinheiro vira sarça,
  lava tem crosta e veios, gelo tem facetas, mola é cogumelo, escada tem degraus, plataforma tem
  veio de madeira. Duas variantes por tile (exposto / enterrado).
- Cada tile desenhado recebe tom e espelho por hash, e escurece com a profundidade — sem grade.
- **Céu** com horizonte claro e zênite escuro (estava invertido: tudo virava um borrão),
  duas cristas de parallax, névoa, lua e vinheta.
- **FX**: corte em arco, faísca de acerto, poeira de pouso, explosão de chefe, brilho de coleta,
  partículas por mundo (neve, brasa, gota, vaga-lume, poeira).
- **HUD**: corações desenhados, pips de vida, almas, barra de chefe com nome e moldura.
- **Toque**: pads redondos com anel e glifo, área de toque circular.
- **Menus**: moldura dourada 9-slice, fundo pintado comum, mapa com cor por mundo e cadeado,
  tipografia hierárquica.
- `Sfx.cs` (novo) — 12 efeitos sintetizados em runtime; o jogo era mudo.

Verificação offline: `Nightfall.Editor.ArtDump.Run` escreve atlas, sprites, mapas de fase e um
frame composto em `tools/artdump/`. Foi assim que o céu invertido e a repetição foram pegos.

### 7. Testes e build

`Smoke.RunInternal` foi de 20 para **55 checks**, incluindo:

- `CheckTraversal` — as 16 fases têm o sino alcançável a partir do spawn
- `CheckNoTrap` — nenhum piso inalcançável sem hazard (nenhuma gaiola)
- `CheckSnapshot` — máscara de itens ida e volta, índices de catálogo, e 10 s de simulação parada
  sem o herói morrer

Resultado: **SMOKE OK 55 checks**, `sealed = 0` nas fases inspecionadas (a geometria já nasce
correta, os reparos automáticos não precisaram entrar).

APK final: `MikeAndDenyse-Nightfall-NGO.apk` — 2026-08-17 01:20, 163 860 441 bytes,
versão 3.0 / bundle 10, arm64-v8a + armeabi-v7a, 0 `error CS`.

### 8. Achados durante a revisão (não relatados, corrigidos)

- **Payload do snapshot.** A v2 do `FrameSnap` chegou a ~1,1 KB em RPC confiável — perto do MTU
  do pipeline confiável do NGO, que não fragmenta. Campos reduzidos a byte/flags, arrays cortados
  para 20 inimigos / 12 projéteis / 8 FX (~800 B) e `StateRpc` passou a `RpcDelivery.Unreliable`,
  que é o certo para snapshot de estado.
- **Vazamento de textura.** `PrepareWorld` criava atlas, céu e duas cristas a cada início de fase
  sem `Destroy` das anteriores — objetos nativos não são coletados pelo GC. Agora libera na troca
  e no `OnDestroy`.
- **Resgate na arena.** O fallback do `Rescue` dentro da arena estava atrás de um `Clamp`, então
  nunca disparava. Ordem corrigida.

---

## Critério de pronto

O jogo está pronto quando:

1. Título abre sem exception vermelha
2. Um jogador → heróis → mapa → **Começar a fase** entra na fase 1
3. Mike anda, pula e corta; Denyse anda, pula e dispara
4. Inimigos da floresta aparecem e tomam dano
5. Chefe da arena aparece; sino libera a fase
6. Morte e “Renascem” voltam para a mesma fase
7. Próxima fase avança o índice
8. Esc / II / **botão voltar** pausa; pausa oferece Continuar, Mapa e Menu inicial
9. Exception de `Substring` **não** volta a aparecer
10. Estes três arquivos refletem o estado real

Acrescentado na v3.0:

11. Cair em espinho, lava ou espinheiro custa **um** dano e devolve o herói ao chão seguro —
    nunca prende até morrer
12. As 16 fases são atravessáveis do spawn ao sino (verificado no smoke, não no olho)
13. Nenhum poço sem saída: o que o BFS não alcança recebe hazard
14. Morrer mostra o painel com o mundo atrás; a tela nunca fica preta e travada
15. No coop, o segundo celular vê coleta de item, projétil, corte, faísca e barra de chefe
16. O título tem **Sair** e ele fecha o app de verdade
