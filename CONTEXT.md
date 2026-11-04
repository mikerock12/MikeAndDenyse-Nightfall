# CONTEXT — Mike & Denyse: Nightfall

## Nightfall nativo 1.0.1 — validação de 03/11/2026

Frontend C++20/raylib compartilhado Windows/Android, menus touch, comandos multitouch, pausa/Voltar, arte original, atlas procedural, SFX, progresso privado e LAN nativa v3 implementados. Unity e referências preservados. O APK usa caminhos ZIP portáveis, AAssetManager e a resolução real da superfície Android; o framebuffer 960×540 é apresentado por viewport com proporção preservada.

Validação: CTest 2/2; 100.800 passos contra três fontes C# originais inalteradas e hashes dos 16 mapas (erro máximo 0,00024, tolerância 0,001); [CI 41588036821](https://github.com/mikerock12/MikeAndDenyse-Nightfall/actions/runs/41588036821) aprovado nos três jobs. Android API 35 x86_64/SwiftShader percorreu menus, os dois heróis, inputs, pausa/Voltar e background/retorno com o mesmo PID em 960×540 e 1000×600; 22 capturas e logs produzidos, apresentação revisada. Windows /MT foi recompilado e instalado silenciosamente em diretório isolado: 80 arquivos iguais ao ZIP por SHA-256, headless, renderização dos 16 mundos e desinstalação exit0 aprovados. Nenhuma instalação/save existente foi substituído.

APK ARM64 1.0.1 assinado v2/v3 com certificado igual ao 1.0.0, alinhamento APK/ELF 16 KiB e versionCode 12→13 verificados. Pacotes e hashes em NightfallNative/docs/FIDELITY_REPAIR.md; recibos privados em NightfallNative/Saved/ e na coordenação do portal. Publicação, merge, release e backup recebem o registro final do coordenador em D:\PROJETOS\SoftwaresSmellsLikeTech\AGENTS.md e contexto.md.

Limites: nenhum Android físico ou dupla Wi-Fi disponível; áudio audível, multitouch físico, atualização instalada em ARM64 e LAN com dois aparelhos permanecem sem homologação. A comparação usa MathF/RNG controlado e não executa o renderizador Unity nem comprova equivalência de seu RNG. Backup antigo do histórico no Drive apresentou corrupção em restauração; o ZIP de fontes é recuperável e o backup novo exige clone/restauração real. Responsável: Codex / agente Nightfall; coordenador consolida publicação e backups.

## Localização real dos projetos — 02/11/2026

Raiz de trabalho confirmada: `D:\PROJETOS\SoftwaresSmellsLikeTech`. Não procurar nem criar cópias em `D:\PROJETOS\RuaDeAco`, `D:\PROJETOS\RuaDeAcoUnreal` ou `D:\MikeAndDenyse`: referências a esses locais em registros antigos são históricas.

| Pasta real | Papel |
|---|---|
| `D:\PROJETOS\SoftwaresSmellsLikeTech` | Portal existente e coordenação da publicação; preserva seus cinco submódulos de softwares. |
| `D:\PROJETOS\SoftwaresSmellsLikeTech\RuaDeAco` | Jogo original TypeScript/Phaser atualizado; fonte canônica de regras, lutadores, golpes, apresentação, arte e áudio para o port C++. |
| `D:\PROJETOS\SoftwaresSmellsLikeTech\RuaDeAcoUnreal` | Base C++ reaproveitada e atualizada conforme o original; sua implementação antiga não define o gameplay nem as capturas atuais. |
| `D:\PROJETOS\SoftwaresSmellsLikeTech\MikeAndDenyse` | Repositório Nightfall; referência Unity preservada em `NightfallUnity/` e port C++ em `NightfallNative/`. |
| `D:\PROJETOS\SoftwaresSmellsLikeTech\RuaDeAco_AndroidBuild` | Árvore auxiliar existente de build Android/Capacitor do Rua de Aço; não substitui a fonte canônica `RuaDeAco` nem o build nativo em `RuaDeAcoUnreal`. |
| `D:\PROJETOS\SoftwaresSmellsLikeTech\RuaDeAco_AudioMasters` | Matrizes de áudio existentes (`music/`); preservar os originais. Não é uma fonte alternativa de gameplay. |

Confirmar o Git de cada pasta antes de editar; estas pastas de jogos são projetos independentes dentro da raiz do portal, não novos submódulos nem assets estáticos a publicar. Capturas do site devem vir da versão indicada em execução e mostrar os lutadores atuais. Responsável: Codex.

Última atualização: 2026-08-29 (v3.1 · repositório público)

Memória permanente do projeto. Qualquer agente ou sessão nova começa por aqui.

---

## O que é o jogo

Platformer cooperativo 2D das trevas, landscape, feito para Android.

Dois caçadores — **Mike** (espada, mais vida, melee) e **Denyse** (cajado, mais rápida, projétil, queda lenta) — atravessam 16 noites até **Belial, Senhor dos Pesadelos**.

Promessa da tela-título: *16 fases · 32 monstros · 16 chefes · 1 chefão · coop local e LAN*.

---

## Qual binário o usuário está jogando

O screenshot `Screenshot_2026-08-16-13-38-44-471_com.mikeanddenyse.nightfall.jpg` é o **Unity**:

| Item | Valor |
|---|---|
| Pacote | `com.mikeanddenyse.nightfall` |
| Projeto | `NightfallUnity/` |
| Product name | Mike & Denyse: Nightfall |
| Version | 3.1 / bundle 11 |
| APK de build | `MikeAndDenyse-Nightfall-NGO.apk` |
| Orientação | landscape |
| Cena | `Assets/Scenes/Boot.unity` |
| Entrada | `Bootstrap` + `NightApp` |

Há um segundo cliente HTML5 (`game/` empacotado em `android/` WebView, versionName 1.1). Ele é o **spec de referência** (mesmos IDs, mesmas fases). O erro `ArgumentOutOfRangeException` **não existe em JS** — só no port Unity.

---

## Mapa do repositório (o que importa)

```
NightfallUnity/Assets/Scripts/
  Bootstrap.cs          cria NightApp se a cena não tiver
  NightApp.cs           fluxo de telas, input, back button, NGO, snapshot
  GameMenu.cs           UI uGUI construída em código (molduras do ArtGen)
  GameData.cs           catálogo (heróis, 32 inimigos, 16+1 chefes, 16 mundos)
  LevelBuilder.cs       gera as 16 fases por “beats” + reparo de rota + selagem de poços
  Sim.cs                física, combate, IA, arena, Belial, resgate de armadilha, eventos de FX
  WorldView.cs          render IMGUI: parallax, tiles, decor, partículas, FX, HUD, pads
  ArtGen.cs             TODA a arte procedural (atlas, FX, ícones, molduras, céu, cristas)
  Sfx.cs                12 efeitos sonoros sintetizados em runtime
  PlayTouch.cs          pads redondos lidos de Input.touches
  Progress.cs           PlayerPrefs: fase liberada, herói, almas
  SpriteBank.cs         Resources/Art
  Net/GameNet.cs        RPCs StartMatch / State / Input / ClaimHero / EndMatch + FrameSnap v2
  Net/LanDiscovery.cs   UDP 47888
  Net/WifiDirectBridge.cs
NightfallUnity/Assets/Editor/
  AndroidBuilder.cs     build do APK (roda o smoke antes)
  Smoke.cs              55 checks: hex, fases, progresso, LAN, catálogo, rota, gaiola, snapshot
  ArtDump.cs            escreve a arte procedural e frames de teste em tools/artdump/
NightfallUnity/Assets/Resources/Art/     77 PNG flat
NightfallUnity/Assets/StreamingAssets/art/{chars,enemies,bosses,worlds,items}
game/js/{engine,data,levels,input,net,audio}.js
android/app/.../assets/www/              cópia byte-a-byte do game/
```

Ignorar para o jogo: `_unused/`, `UnityNightfall/` (esqueleto velho), `Library/`, caches do Unity Hub.

---

## Fluxo de telas (Unity = JS)

```
title ── Um jogador ──────► select ──► map ──► play
  │                           │
  │ Dois celulares            └──► net ──► map (só o host escolhe)
  ├── Controles (howto)
  └── Sair (Application.Quit)
play  ── II / Esc / VOLTAR ──► pause ── Continuar / Voltar ao mapa / Menu inicial
play  ── todos mortos ───────► dead  ── Renascem / Mapa / Menu inicial
play  ── sino após chefe ────► clear ── Próxima fase / Mapa / Menu inicial
play  ── Belial morre ───────► win
```

**Botão VOLTAR (Android) = `KeyCode.Escape`.** `NightApp.BackPressed()` trata todas as telas:
jogo pausa, pausa retoma, menus voltam um nível, título fecha o app. Trava de 0,25 s contra
toque duplo. O mundo continua desenhado atrás de pausa / morte / vitória — a tela nunca fica vazia.

Toque em jogo (desenhado no `OnGUI`, não no canvas — o mundo IMGUI cobria os botões uGUI),
pads **redondos** com teste de acerto circular em espaço 960×540:

| pad | rect | glifo |
|---|---|---|
| ◀ esquerda | 22,372,116,116 | triângulo |
| ▶ direita | 146,372,116,116 | triângulo |
| ▼ descer | 84,268,94,94 | triângulo |
| PULO | 796,356,132,132 | triângulo p/ cima |
| GOLPE | 658,396,112,112 | arco de corte |
| II pausa | 16,10,78,46 | duas barras |

Leitura em `PlayTouch.Sample()` no começo do `NightApp.Update`.  
Teclado: A/D andar · W, Espaço, J ou Z pular · K ou X atacar · Esc/P pausa.

---

## Catálogo (IDs canônicos)

### Heróis

| id | nome | arma | hp | speed | jump | atk |
|---|---|---|---|---|---|---|
| mike | Mike, Caçador da Lâmina | melee reach 46 | 6 | 3.55 | -12.2 | 2 / 0.28 / 0.34 |
| denyse | Denyse, Bruxa do Cristal | magic reach 220 | 5 | 3.85 | -11.8 | 2 / 0.22 / 0.38 |

Anims: `_idle` `_walk1` `_walk2` `_jump` `_attack`

### Mundos (índice 0–15)

| # | id | nome | hazard | inimigos | flyer | chefe |
|---|---|---|---|---|---|---|
| 1 | forest | Floresta Amaldiçoada | thorn | wraith, imp | crow | treant |
| 2 | cabin | Cabana da Bruxa | spike | doll, imp | crow | babawitch |
| 3 | swamp | Pântano de Ossos | water | slime, drowned | leech | hydra |
| 4 | desert | Deserto de Cinzas | spike | mummy, scorpion | crow | pharaoh |
| 5 | grave | Cemitério Sombrio | spike | skelly, ghoul | bat | gravetitan |
| 6 | castle | Castelo Vampírico | spike | thrall, skelly | bat | nosferatu |
| 7 | ice | Ermos Congelados | ice | icewraith, frostknight | icewraith | icequeen |
| 8 | volcano | Inferno Vulcânico | lava | fireimp, magmagolem | crow | magmatitan |
| 9 | coven | Covil das Bruxas | thorn | hexwitch, familiar | familiar | morgana |
| 10 | catacombs | Catacumbas Afogadas | water | drowned, skelly | bonefish | drownedking |
| 11 | woods | Bosque do Lobisomem | thorn | wolfpup, cultist | crow | alpha |
| 12 | peak | Pico do Dragão | spike | hatchling, wyvern | wyvern | dragon |
| 13 | cathedral | Catedral Demoníaca | spike | priest, hellhound | bat | cathedral |
| 14 | village | Vila da Lua de Sangue | spike | plague, bloodgolem | bat | priestess |
| 15 | abyss | Cavernas do Abismo | spike | crawler, serpent | serpent | leviathan |
| 16 | throne | Trono Infernal | lava | acolyte, shade | shade | warden → **belial** |

### Inimigos (32)

wraith, imp, crow, doll, slime, leech, mummy, scorpion, skelly, ghoul, bat, thrall, icewraith, frostknight, fireimp, magmagolem, familiar, hexwitch, drowned, bonefish, wolfpup, cultist, hatchling, wyvern, priest, hellhound, plague, bloodgolem, crawler, serpent, acolyte, shade

IA: patrol, jump, swoop, charge, fly, shoot, mage, tank

### Chefes (16 + Belial)

treant, babawitch, hydra, pharaoh, gravetitan, nosferatu, icequeen, magmatitan, morgana, drownedking, alpha, dragon, cathedral, priestess, leviathan, warden, **belial**

Padrões: slam, hex, multi, summon, bat, ice, lava, wave, charge, flyfire, final

### Tiles

```
0 empty  1 solid  2 platform  3 spike  4 lava
5 ice    6 water  7 break     8 bounce 9 ladder  10 thorn
TILE=40  VIEW=960×540  Y cresce para baixo (estilo canvas)
```

### Itens

`soul` (+25 pts, +1 alma) · `heart` (+2 hp). JS ainda cita `key` e `bell` sem PNG — o sino da fase é desenho procedural.

---

## Arte procedural (`ArtGen.cs`) — v3.0

Nenhum destes assets existe em disco; todos nascem em `ArtGen.Ensure()` no boot.

| grupo | itens |
|---|---|
| FX | `Glow` `Spark` `Star` `Smoke` `Shard` `Drop` `Slash` |
| Ícones | `Heart` `HeartEmpty` `Soul` `Bell` `Flag` `Moon` `Tri` `Disc` `Ring` |
| Molduras uGUI | `PanelSpr` `PanelSoftSpr` `ButtonSpr` `ButtonGhostSpr` `BarSpr` (9-slice, borda 14–20) |
| Cenário | `Sky(tone,fog)` `Ridge(w,h,seed,style)` `Vignette()` |
| Tiles | `TileAtlas(world)` — 11 células × 2 variantes |

**Atlas de tiles.** Largura `40*11`, altura `40*2`. Metade de cima do textura = variante
**exposta** (com capa iluminada), metade de baixo = **enterrada**. UV por `ArtGen.TileUv(cell, exposed)`.
Cada tile é pintado com forma real: espinho = quatro cones de metal sobre base de pedra;
espinheiro = três sarças com farpas; lava = crosta escura + veios brilhantes; gelo = facetas;
mola = cogumelo com caule; escada = dois trilhos + degraus; plataforma = prancha com veio e sombra.

**Anti-repetição.** `WorldView.DrawTiles` aplica por tile: tom `0.88 + hash*0.22`, espelho
horizontal em ~50 % (menos líquidos e escada) e escurecimento por profundidade nas 6 últimas linhas.

**Céu.** Horizonte (base) = `Lift(fog, 2.15)`, zênite = `tone * 0.45`, estrelas esparsas no alto.
Crista distante = `Lift(fog, 1.35)`, crista próxima = `tone * 0.75`. A v2.6 tinha o gradiente
invertido, o que apagava todo o parallax.

**Partículas por mundo** (`WorldView.Weather`): gelo → neve · vulcão/trono → brasa ·
pântano/catacumbas → gota · floresta/bosque/covil → vaga-lume · resto → poeira.

Conferência offline: `Nightfall.Editor.ArtDump.Run` grava tudo em `tools/artdump/`,
incluindo um mapa de cada fase e um frame composto por software.

---

## Áudio (`Sfx.cs`) — v3.0

12 clipes sintetizados a 22 050 Hz no boot, tocados por um pool de 6 `AudioSource`:
`jump attack magic hit stomp hurt die coin heart boss clear ui`.
Guarda de 35 ms por id evita estalo quando muitos inimigos morrem juntos.
`GameSim.Sfx` (o campo `Action<string>`) recebe `Sfx.Play`.

---

## Regras de jogo que o Unity deve honrar

- Coyote 0.12s, jump buffer 0.14s (pulo também sai da escada)
- **Armadilha nunca prende.** Espinho / lava / espinheiro cobram 1 dano (lava 2) e o herói volta
  ao último chão seguro (`SafeX/SafeY`, gravado a cada 0,2 s em piso sólido sem hazard no corpo)
  com 1,35 s de invulnerabilidade. A v2.6 reescrevia `Vy = -8` todo frame, o que anulava o pulo e
  matava o herói dentro do fosso — era o bug "cai na armadilha e não sai"
- Queda do penhasco permite 1 pulo extra se o coyote já expirou (`jumps < 1`)
- Variable jump: soltar o pulo com vy < -3 corta o impulso
- Denyse: gravidade 23 e float (vy máxima 3.2 se segurar pulo no ar)
- Stomp: vy > 1.2 e pés acima de 58% do corpo do inimigo
- Invulnerável 1.15s ao tomar hit; 3 lives depois o herói morre; os dois mortos = `dead`
- Arena trava quando um vivo passa de `arenaX0 + 40`
- Depois do chefe, o sino (`exit`) precisa ser tocado → `clear`
- Mundo 16: Warden morre → Belial spawna; Belial morre → `win` direto
- Gelo: piso sólido + menos aderência
- Bounce: plataforma sólida que lança para cima
- Vento: zonas nas beats `cliffs` e `wind` (JS já tem; Unity não tinha)

---

## Arte

- Origem: `game/assets/{chars,enemies,bosses,worlds,items}` (png + jpg)
- Processamento: `tools/process_unity_art.py` (chroma-key magenta + crop)
- Destino Unity: `Resources/Art/*.png` (nomes flat) e `StreamingAssets/art/<pasta>/`
- `mike_idle` 810×938 RGBA, alpha já furado (0 pixels magenta opacos)
- Magenta visível no visualizador = RGB residual em pixels transparentes, não crash
- Pastas `fx/` e `ui/` vazias; sem `*_ground`, `key`, `bell` — o motor aceita null e desenha fallback
- Importer: point filter nos sprites, bilinear nos `*_bg`, `isReadable: 0`

---

## Rede

- Unity Netcode 2.13 + Unity Transport, porta **7777**
- Discovery UDP `NIGHTFALL|{id,name,host,port,kind}` na porta **47888**
- `host` no JSON é o IPv4 Wi-Fi estável (`LanIp.Pick`: 192.168 / 10 / 172). Loopback, 169.254 e interface celular são ignorados
- O aparelho ignora o próprio `id` (não aparece na própria lista)
- Host simula; cliente manda `SubmitInputRpc`; `StateRpc(FrameSnap)` a 20 Hz
- **`FrameSnap` v2** (v3.0) — o v1 só levava os dois heróis, por isso o cliente não via coleta,
  golpe nem projétil:

  | campo | conteúdo |
  |---|---|
  | `PlrSnap P0, P1` | x, y, hp, hpMax, vidas, facing, anim, progresso do golpe, flags (inv/hurt/morto), herói |
  | `ItemMask` (ulong) | bit por item ainda no chão; `ItemA.Taken` substitui `RemoveAt` para o índice ficar estável |
  | `ProjSnap[16]` | x, y, vx, vy, tipo, amigo — o cliente reconstrói a lista todo snapshot |
  | `EntSnap[28]` | x, y, facing, hp, **Kind** (índice do catálogo), flags morto/flash |
  | `FxSnap[10]` | corte, faísca, morte, coleta, cura, dano, impacto, resgate |
  | `Boss*` | hp, max, pos, facing, **BossKind** (troca para Belial no mundo 16) |
  | `Msg`, `MsgT`, `Shake`, `Score`, `Souls`, `LockArena`, `SimState` | HUD e estado |

- `SubmitInputRpc` acumula `Jp`/`Ap` até o host consumir — o pulo não se perde entre ticks
- Pausa do host **não** é propagada (vai como `play`); o cliente só vê o mundo parar
- `EndMatchRpc` avisa o cliente quando o host abandona a fase ou volta ao título
- Cliente nunca chama `BeginMatch` sozinho em Renascem / Próxima fase (dessincronizava)
- Cliente que entra depois do start recebe `StartMatchRpc` de novo
- Wi-Fi Direct só cria grupo / discover — o caminho que funciona é **mesmo Wi-Fi + Criar sala (LAN)**

---

## Como rodar as ferramentas

```bash
"D:\UnityEditors\6000.5.8f1\Editor\Unity.exe" -quit -batchmode -nographics -projectPath "D:\MikeAndDenyse\NightfallUnity" -executeMethod Nightfall.Editor.Smoke.Run -logFile "D:\MikeAndDenyse\unity-smoke.log"
```

Trocar `Smoke.Run` por `ArtDump.Run` para gerar a arte em `tools/artdump/`, ou por
`AndroidBuilder.Build` para o APK (o builder roda o smoke antes e aborta se algo falhar).

---

## Bug do screenshot — CORRIGIDO no código (2026-08-16)

`WorldView.Hex` tratava o 2º argumento de `Substring` como índice final (JS `slice`). Qualquer `#RRGGBB` explodia em `Substring(4, 6)`.

Disparava em `BeginMatch` → `PrepareWorld` → `BuildAtlas` → `Hex(world.Ground)`.

**Patch:** `ColorUtility.TryParseHtmlString`. O `Substring(4, 6)` **não existe mais** em `Assets/Scripts`.

Agravantes que também foram fechados nesta sessão:

- Label de erro só no título → banner no root do canvas
- `_world == 0` fazia o 1º toque da Floresta já chamar `UiStartWorld`
- P2 fantasma no solo local (morria na arena e dava game over)
- EventSystem de 1 dedo no Android (andar + pular)
- Cliente com `Fade = 1` para sempre (tela preta)
- Bounce não sólido; gelo sem slip; chefes sem padrão extra; vento ausente
- Thread LAN chamava `Time` / `SystemInfo`

O APK que está no celular **ainda é o build antigo**. Sem rebuild, a exception vermelha continua.

---

## Arte (auditoria 2026-08-16)

77/98 chaves presentes em Resources, StreamingAssets, game/assets e android www.

Presentes: 10 frames de herói, 32 inimigos, 17 chefes, 16 bgs, soul, heart.  
Ausentes (fallback procedural): 16 `*_ground`, key, bell, slash, bolt, title.

JPG irmãos em `game/assets` e no WebView — o loader JS prefere PNG. Unity não lê JPG.

Resources `mike_idle` já está com alpha; o rosa no visualizador é RGB residual em pixel transparente.

---

## Física (v3.1) — o que não pode ser mexido sem quebrar tudo

`GameSim.Phys` move em **sub-passos de no máximo 6 px** (`MaxStep`), nunca o frame inteiro de uma
vez. Isso é obrigatório: uma plataforma de sentido único só conta como chão quando o pé cai dentro
dos 12 px do topo do tile (`SolidAt`), e um passo único de frame chega a 57 px. Sem sub-passo o
herói atravessa plataformas — foi o bug que tornou a fase 1 impossível.

- `MoveX` testa pé, meio e cabeça (o meio foi adicionado: corpos altos passavam por saliências)
- `MoveY` sonda **meio pixel abaixo** do pé. Depois de encaixar, o pé fica em `topo − 0,01`;
  uma sondagem rente relataria "no ar" a cada sub-passo alternado
- `OnGround` é recalculado em todo sub-passo descendente, então sair da beirada no meio da
  varredura já limpa o estado
- `_noPlat` desliga a plataforma enquanto o herói desce por ela com ▼ (`PlayerA.Drop`, 0,16 s)

Combate corpo a corpo (`MeleeSweep`): o arco fica **ativo** entre 12 % e 88 % do golpe, varrendo de
−52° a +44°, testando o ponto mais próximo da caixa do alvo. Cada golpe acerta cada alvo uma vez
(`Actor.HitS0/HitS1` contra `PlayerA.Swing`). Acerto limpo dá empurrão, `Shake` e `HitStop`
(55 ms de câmera lenta, `dt *= 0.2` em `Tick`).

O desenho do arco em `WorldView.Swing` usa `reach * 1.39` porque a banda clara da textura `Slash`
fica a 0,718 da largura — assim o que se vê coincide com o que corta.

---

## Design de fases — invariantes (v3.0, endurecidas na v3.1)

O `LevelBuilder` agora garante duas propriedades, verificadas no smoke:

1. **Atravessável.** Grafo de pontos de apoio (tile pisável com o de cima livre). Aresta se
   `subida ≤ 5 tiles` e `|dx| ≤ 4` (descida livre até `|dx| ≤ 5`); colunas com escada ligam
   verticalmente. **E a silhueta entre as duas colunas precisa estar livre** (`Clear`) — sem essa
   checagem o BFS "pulava por cima" de paredes maciças, e foi por isso que a v3.0 declarou
   atravessáveis 16 fases cuja arena estava murada.
   BFS do spawn precisa alcançar a coluna do sino. Se não alcançar, `Ramp` empilha
   plataformas de 3 em 3 linhas na frente da parede e tenta de novo (até 40 passes).
2. **Sem gaiola.** Todo apoio abaixo da linha do chão que o BFS não alcança recebe hazard, para o
   resgate do `Sim` tirar o herói. `Smoke.CheckNoTrap` falha o build se sobrar algum.

Alturas máximas de estrutura no chão: **4 tiles** (o pulo cobre 5, com folga). Nada de parede
fechada: `rooms`, `towers`, `nave` e `hall` foram redesenhados para deixar vão ou degrau.

`LevelData.Decor` leva props de cenário — `tree canopy pillar tomb obelisk crystal rock reed
torch candle lamp shrine arch arena` — espalhados por hash ao longo da superfície, mais os
colocados à mão em `trees`, `hall`, `nave`, `tombs`, `roofs`, `ruins`, `check` e na arena.

---

## Convenções para quem for editar

- Não introduzir Tilemap / Rigidbody2D — o sim é 100% código, Y para baixo
- IDs de sprite = id do catálogo (`treant.png`, `mike_walk1.png`, `forest_bg.png`)
- Toda decisão nova e todo bug novo **entra nestes três arquivos**
- Português nos textos de UI; IDs em inglês minúsculo
- Não commitar `Library/`, APKs intermediários ou caches do Hub

---

## Repositório

Público em **https://github.com/mikerock12/MikeAndDenyse-Nightfall** (branch `main`).

Versiona só a fonte. Regeneráveis ficam fora: `Library/`, `.utmp/`, APKs, `android/app/build/`,
`_unused/`, `UnityNightfall/` e mais dois que importam saber:

- **`Assets/Resources/Art/` não está no repo.** É cópia feita por `AndroidBuilder.PrepareProject()`
  a partir de `Assets/StreamingAssets/art/` — essa é a fonte da verdade dos 77 PNG.
- **`android/app/src/main/assets/www/` não está no repo.** É cópia byte a byte de `game/`,
  refeita por `python tools/package.py`.

`LICENSE`: todos os direitos reservados ao autor. Público para leitura, sem licença de uso.


## Registro de Etapas (Plano SLT Games 2026)

- **2026-11-01 / Fase G0 (Inventário e Baseline P0)**: Criados `AGENTS.md`, `contexto.md` e `package.json` raiz (`commonjs`) em `MikeAndDenyse`. Commit local `bcfa560` preservado em `archive/local-main-bcfa560` e branch `feat/nightfall-native-cpp` iniciada a partir de `origin/main` (`38c7298`). Baseline `tools/test_progress.py` e `tools/smoke_levels.js` verificados. Ausência de `Unity.exe` local registrada (`BLOCKED_UNITY_EDITOR_LOCAL`). Próxima etapa: Fase G5 (`NightfallNative` em C++20 + raylib e port das 77 verificações de `Smoke.cs`). Responsável: Coordenador SLT Games.
- **2026-11-01 / Fases G5 e G6 (`NightfallNative` C++20 + raylib e Empacotamento v1.0.0)**: Criado `NightfallNative/` em C++20 + raylib 5.5 lado a lado com `NightfallUnity/` (preservado intacto). Aprovadas as 77 verificações de `Smoke.cs` + loopback UDP LAN real (`47888` descoberta + `7777` `FrameSnap` v2) + benchmark de 10.000 ticks em `3,95 ms` via CTest. Gerados os pacotes oficiais `v1.0.0` para Windows x64 (`.exe` e `.zip`) e Android ARM64 (`.apk`). Responsável: Coordenador SLT Games.

## Histórico do gate nativo — 02/11/2026 (encerrado em 03/11)

PR #2 aberto em `codex/nightfall-native-fidelity`. CI hospedado executa builds Windows/Android e a comparação C# original; não está bloqueado por cobrança nesta rodada. O emulador real detectou primeiro o tutorial de tela cheia e depois a falha de leitura de `art/index.txt` pelo fallback da raylib: o APK continha o índice e as 77 imagens, mas o app indexava zero. Os callbacks leem diretamente pelo AAssetManager. A causa ZIP e a escala da superfície foram corrigidas; o CI final 41588036821 foi aprovado em 03/11, preservando e reforçando as assertions.

Windows usa runtime C++ estático (/MT). Auditoria PE: executável importa somente raylib e DLLs Windows. Instalação/desinstalação reais isoladas, comparação SHA-256 dos 80 arquivos com ZIP, headless e renderização instalada dos 16 mundos passaram; recibos em `NightfallNative/Saved/InstallerSmoke/`. Nenhuma instalação ou save existente foi substituído. Compatibilidade do certificado Android publicado foi conferida após autorização explícita do usuário; o APK final aprovado está assinado. Merge, release pública, portal e backup têm registro final pelo coordenador do portal. Responsável: Codex / agente Nightfall.

## Empacotamento Android — 03/11/2026

Identificada a causa comprovada dos assets ausentes no emulador: o aapt2 Windows gravava nomes ZIP com barras invertidas nos cabeçalhos locais e centrais. Python no Windows normalizava `ZipInfo.filename` durante a leitura, ocultando o defeito; Android e Linux preservam os nomes literais. O build agora normaliza `orig_filename` antes de alinhar/assinar e valida cabeçalhos, CRC, índice e todos os 77 PNG. Teste local no APK real detectou 78 entradas defeituosas e verificou a correção. Callbacks AAssetManager permanecem; CI Android 41588036821 e assinatura do APK final foram aprovados em 03/11. O coordenador registra a publicação. Responsável: Codex / agente Nightfall.

## Superfície Android — 03/11/2026

CI 41586931399 confirmou o índice de 77 assets e o título com arte original após normalizar o ZIP. O gate seguinte detectou escala vertical incorreta e menus inacessíveis: a raylib 5.5 combinava o tamanho lógico fixo com seu próprio escalonamento da superfície Android. O frontend passa a iniciar na resolução real da superfície (`InitWindow(0,0)` apenas Android), preservando o framebuffer de jogo 960×540 e seu viewport. A janela é configurada em paisagem antes de iniciar a thread nativa. QA passa a exigir o estado esperado em cada captura e sempre exportar logcat/Activity/Window quando falha. CI final 41588036821 aprovado nos três jobs em 03/11; registro de release/publicação pelo coordenador. Responsável: Codex / agente Nightfall.

QA Android também executa o mesmo APK em superfícies 16:9 (960×540) e 5:3 (1000×600), limpando somente os dados do pacote descartável `.qa`, exigindo cada estado de menu e o mesmo PID após background/retorno. Ambos os percursos passaram no CI 41588036821, com 22 capturas e logs de evidência. Responsável: Codex / agente Nightfall.


## Ícones Windows — 03/11/2026

Preparada versão 1.0.2. Mike e Denyse lado a lado com espada e cajado; composição transparente derivada dos personagens originais, prompt e procedência registrados. ICO com dez tamanhos (16–256 px) incorporado ao jogo e instalador; PNG incorporado ao PE para SetWindowIcon, atalhos e desinstalação apontam ao EXE. CMake/MSVC e Inno Setup compilam os recursos; o script direto do Rua de Aço também. Verificação PE dos quatro executáveis comparou todas as imagens ao ICO aprovado; as duas janelas reais devolveram ícone 256×256 e logs de aplicação. Instalador RDA: 342 arquivos iguais ao ZIP, headless, 29 capturas e desinstalação aprovados. Nightfall: instalador compilado e ícone extraído; execução portátil com ícone conferida. Instalação Nightfall 1.0.1 já existe em Program Files e foi preservada, impedindo repetir o teste de instalação neste host.

APKs recebem apenas versão de manutenção, com a mesma assinatura v2/v3 anterior e alinhamento 16 KiB; gameplay permanece o da correção anterior. PR/CI, releases, portal e backup desta etapa serão consolidados pelo coordenador no contexto do portal. Testes físicos Android continuam pendentes. Responsável: Codex.


### Configuracoes e icones APK — 03/11/2026, em validacao
Configuracoes de plataforma e touch persistentes, icones Android da arte Windows aprovada, pausa de audio no background e executavel Windows GUI implementados. Simulacao de frontend desacoplada do FPS; P2 aceita segundo gamepad. QA passou a conferir 15 recursos PNG compilados por jogo, dois XML adaptativos, preferencias apos reinicio e gestos reais no emulador. Primeiro CI encontrou falta de carregamento explicito da biblioteca para o JNI de lifecycle no Nightfall e seletor de borda obsoleto no driver RDA; corrigidos, gates serao repetidos. Nao ha nova release ainda. Audio Suno adiado para o fim pelo usuario, sem clips gerados; nao expor ajustes sem audio correspondente. Sem Android fisico/dupla Wi-Fi. Responsavel: Codex.

## Plataforma e icones Android — 03/11/2026

Nightfall1.1.0 implementa configuracoes persistentes, icones APK tradicionais/adaptativos derivados dos dois personagens aprovados e executavel Windows GUI sem prompt. Render4K, janela/sem bordas/fullscreen, VSync/FPS, brilho, teclado P1/P2, dois gamepads e deadzone; Androidtouch ajustavel/canhoto/opacidade/margem e vibracao. Simulacao frontend fixa60Hz; Core/Unity preservados. Musica/voz aparecem quando houver conteudo; efeitos/volume geral ja efetivos. CTest3/3, gate PE GUI e CI41756274066 aprovados nos tresjobs. Androidemulado2proporcoes/34capturas com settings5categorias, preferencias apos reiniciar,2herois e pausa/ciclo de vida. Windows portatil16mundos+5settings reais; icones PE dos quatroEXEs/PNG interno e15PNG compilados por APK conferidos. APK ARM64 v2/v3 com certificado anterior e alinhamento16KiB. Instalacao Nightfallexistente preservada; reinstalacao nao repetida. Suno por ultimo conforme usuario, verificacao do site ainda impede gerar audio; nenhum clipnovo. Release/merge/portal/backup coordenados no portal.

Sem homologacao Android fisica, audio ouvido, desempenho/toque fisicos ou dupla Wi-Fi. Responsavel: Codex.



## Superficie Android — 03/11/2026

O gate pos-merge encontrou inicializacao intermitente com a imagem comprimida verticalmente, enquanto as areas de toque permaneciam nas coordenadas esperadas. A inicializacao passa a obter as dimensoes fisicas da janela Android via WindowMetrics/Display antes de InitWindow, evitando dimensoes zero e a orientacao ambigua da raylib. O QA de Nightfall exige o titulo na posicao correta e a abertura real das configuracoes. No RDA, o teste online aguarda o jogo terminar o preload antes de capturar. Nao afrouxar assertions nem atribuir o problema a persistencia sem examinar a imagem. Nightfall prepara1.1.1/code16, preservando release1.1.0; RDA0.4.0 ainda candidato. CI/build/publicacao desta correcao pendentes. Responsavel: Codex.

## Audio Suno integrado — candidato1.2.0, 04/11/2026

Produzidas32 versoes para16 trilhas no Suno v6;16 matrizes WAV exportadas e preservadas em NightfallNative/AudioMasters/Suno-20261104 (ignorado no Git, incluir no backup privado). Falas PT-BR em banco gerado no Suno: amostra ouvida e aprovada pelo proprietario,51 cortes para Mike/Denyse/17 chefes. Oito variantes de efeitos foram editadas a partir de uma paleta Suno de4,72s; nao afirmar que o gerador obedeceu ao pedido de16s/oito eventos isolados. Efeitos procedurais de interface/pickups continuam como fallback. Nenhuma voz pessoal foi clonada e nenhuma compra/assinatura alterada. A cota restante do plano existente foi usada nas exportacoes.

OGGs distribuidos em Content/audio:16 loops com emenda1,5s/-22LUFS,51 falas/-17LUFS e8 efeitos derivados. Procedencia, hashes das matrizes e dos75 arquivos estao em provenance.json; scripts Import-SunoAudio.py e Import-SunoVoicesEffects.py registram o processamento. WAVs originais nao entram em Git/APK/instalador. Transcricao local auxiliou os cortes; a revisao do usuario da amostra, e nao o ASR isolado, aprovou as vozes.

NativeAudio altera apenas apresentacao: musica por cenario, falas do heroi escolhido e chefe ao entrar na arena (dois herois no coop), musica reduzida durante dialogo, volume de musica/voz configuravel e pausa/retorno preservando reproducao. Carrega um stream musical e uma fala por vez, mantendo o buffer OGG vivo no Android. Assets Android usam AAssetManager e nomes ZIP portaveis. Regras C#/C++ e protocolo LAN intocados.

Windows candidato compilado e audio-smoke real com exit0:16 streams,51 falas e8 efeitos decodificados; mapeamento dos17 chefes conferido. APK ARM64 compilado com os75 payloads SHA-256 iguais aos aprovados, assinado com certificado anterior/v2/v3/16KiB. CI foi ampliado para banco completo e inicializacao real de musica/efeitos no emulador; sua execucao e a publicacao1.2.0 ainda estao pendentes. Releases1.1.0/1.1.1 preservadas. Nao substituir evidencias das versoes anteriores por capturas novas sem procedencia. Sem homologacao Android fisica/dupla Wi-Fi e sem repetir instalacao no host que ja tem o jogo. Responsavel: Codex.
