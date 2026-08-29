# MULTIAGENTS — Mike & Denyse: Nightfall

Última atualização: 2026-08-29

Registro vivo dos agentes, papéis, entregas e o que cada um já fez.  
Nada de trabalho “só na cabeça”: se um agente descobriu ou mudou algo, a linha entra aqui **e** em `PLAN.md` / `CONTEXT.md`.

---

## Como usar

1. Ler `CONTEXT.md` (o que o jogo é) e `PLAN.md` (o que falta).
2. Pegar um papel abaixo ou abrir um agente novo com o prompt do papel.
3. Trabalhar só no recorte do papel.
4. Ao terminar, **obrigatório**:
   - anexar um bloco em `## Diário` neste arquivo
   - atualizar checkboxes / bugs em `PLAN.md`
   - se a verdade do jogo mudou, atualizar `CONTEXT.md`

---

## Papéis

| Papel | Tipo | Escopo | Pode editar? |
|---|---|---|---|
| Orquestrador | sessão principal | prioriza, escreve os 3 arquivos, integra patches | sim |
| Auditor JS | explore / read-only | `game/js/*`, `android/` WebView — extrair o spec | não |
| Auditor Unity | explore / read-only | `NightfallUnity/Assets/Scripts/**` — bugs jogáveis | não |
| Auditor Arte | explore / read-only | sprites vs catálogo, chroma-key, missing files | não |
| Implementador Unity | general-purpose | C# do jogo (Sim, View, Menu, Net, Builder) | sim |
| Implementador JS | general-purpose | `game/js` + cópia `android/.../www/js` | sim |
| Builder | execute | Unity batch build + install APK | sim (não o jogo) |
| Verificador | explore / read-only | conferir que o patch fecha o bug do screenshot | não |

---

## Agentes desta sessão

| id / apelido | papel | estado | recorte |
|---|---|---|---|
| orquestrador | Orquestrador | ativo | screenshot → causa Hex → plano → patches |
| `01a00b7a-…d2e4ba8a5eec` | Auditor JS | concluído | spec HTML5 + bugs para não clonar |
| `01a00b7a-…d2f954da62d2` | Auditor Unity | concluído | 21 bugs; Hex confirmado P0 |
| `01a00b7a-…d308408f3023` | Auditor Arte | concluído | 77 presentes / 21 ausentes (fallback) |

Na rodada de 2026-08-17 nenhum subagente foi disparado: o usuário não pediu, e o custo de
re-derivar o contexto do motor superava o ganho. Os papéis foram exercidos em sequência pela
sessão principal — ver o diário de 2026-08-17.

Quando um auditor voltar, o orquestrador cola o resumo no diário e só então marca o item correspondente em `PLAN.md` como confirmado por agente.

---

## Contratos (o que cada um deve devolver)

### Auditor JS

Estrutura: fluxo de telas, controles, física (bounce/ice/break/vento), combate, arena/Belial, itens, net, assets faltando, bugs JS que o Unity copiou.

### Auditor Unity

Para cada bug: arquivo, linha, impacto (`crash` / `softlock` / `gameplay` / `rede` / `ux`), correção sugerida. O Hex do `WorldView` já é conhecido — procurar o resto.

### Auditor Arte

Tabela id → existe em `Resources/Art`, `StreamingAssets/art`, `game/assets`. Notas visuais das amostras. Missing list (fx, ui, ground, key, bell).

### Implementador

Diff mínimo, sem refatorar o motor. Depois do patch, atualizar os 3 arquivos. Não declarar “fase abre” sem o Hex estar morto.

### Verificador

Reproduzir o caminho do usuário: título → Mesmo aparelho → herói → mapa → toque na Floresta / Começar a fase. Confirmar que `Substring(4,6)` não existe mais e que `BeginMatch` não cai no `catch`.

---

## Regras de coordenação

- Um implementador por arquivo quente (`WorldView.cs`, `Sim.cs`, `NightApp.cs`) para não divergir.
- Spec vence: se Unity e JS discordam sem motivo, **o JS é a regra**, exceto bounce (JS também fura a mola — os dois devem ficar sólidos + impulso).
- Crash > softlock > gameplay > polish.
- Não gerar arte nova nesta rodada a menos que um sprite do catálogo esteja ausente.
- Não tocar em `Library/`, `_unused/`, `UnityNightfall/`.

---

## Diário

### 2026-08-16 — orquestrador

- Leu o screenshot. Pacote `com.mikeanddenyse.nightfall`. UI casa 1:1 com `GameMenu.TitlePanel`.
- Exceção = `String.Substring` com `length` inválido.
- Achado em `WorldView.Hex`: port literal de `slice(4,6)` → `Substring(4,6)` em string de 6 chars.
- Confirmou que `Hex` roda em `BuildAtlas` (start da fase) e em `OnGUI` (todo frame).
- Confirmou que o label de erro só existe no título, e que `_world == 0` faz o primeiro toque da Floresta já chamar `UiStartWorld`.
- Inventário de arte: 16 bgs, 32 inimigos, 16+1 chefes, 10 frames de herói, heart/soul — arquivos presentes.
- `mike_idle` já tem alpha (0 magenta opaco).
- Criou `PLAN.md`, `CONTEXT.md`, este arquivo.
- Disparou os 3 auditores em paralelo.
- Aplicou patches da Fase 1 e 2 sem esperar (Hex já estava fechado).

### 2026-08-16 — Auditor JS (`…d2e4ba8a5eec`)

- Spec de telas, controles, física, combate, Belial, net e assets faltando colado em `CONTEXT.md`.
- Alertou para **não** clonar: double-jump quebrado, bounce oco, gelo inerte, P2 fantasma, unlocked=16.
- Decisão do orquestrador: bounce/gelo/vento/padrões de chefe = intenção, não o bug JS.

### 2026-08-16 — Auditor Unity (`…d2f954da62d2`)

- Confirmou Hex P0 e listou 21 itens. Integrados nesta sessão: 1, 2, 3 (fade/cam/chefe), 4, 5, 7, 8, 9 (ice/bounce), 10, 11 (try/catch), 13, 14, 15, 16, 21.
- Deixados de propósito: P2P handshake completo, segundo pad de toque, `*_ground` art, FX bolt.

### 2026-08-16 — Auditor Arte (`…d308408f3023`)

- 10+32+17+16+2 = 77 PNG em todos os destinos. 21 chaves JS-only ausentes com fallback.
- Sprites são ilustrações reais, não placeholders. Magenta nos JPG/G; Resources já keyed.

### 2026-08-16 — orquestrador (patches)

Arquivos tocados: `WorldView.cs`, `GameMenu.cs`, `NightApp.cs`, `Sim.cs`, `LevelBuilder.cs`, `GameData.cs`, `LanDiscovery.cs`, `SpriteBank.cs`, `game/js/engine.js`, cópia android www.
- Hex morto (prova Python: `#2a4a22` explode no old, RGB no new).
- Smoke 16 fases JS: OK.
- Próximo: rebuild APK (`Unity.exe` + AndroidPlayer em `D:\UnityEditors\6000.5.8f1`).

### 2026-08-16 — Builder (Unity batch)

- `Nightfall.Editor.AndroidBuilder.Build` via `D:\UnityEditors\6000.5.8f1\Editor\Unity.exe`
- Scripts compilaram: **0 error CS**
- APK: `D:\MikeAndDenyse\MikeAndDenyse-Nightfall-NGO.apk` (163 608 461 bytes, 14:13)
- `adb devices` vazio — instalação no celular fica com o usuário
- Sem dispositivo, a verificação no aparelho do screenshot não rodou aqui

### 2026-08-16 — orquestrador (single + save)

- `Progress.cs`, smoke Editor, `tools/test_progress.py` (OK)
- Título sem “mesmo aparelho”; mapa linear; host-only pick no coop
- `OnLog` não mostra mais exception de pacote na title
- Próximo: `Smoke.Run` no Unity e só então o APK

### 2026-08-16 — orquestrador (coop Wi-Fi)

Usuário: IPs estranhos mudando sozinhos, sem sync.

- Reescreveu `LanDiscovery.cs` + `LanIp`
- `GameNet` agora manda `FrameSnap` (inimigos + chefe)
- `NightApp` não recicla botões de sala todo frame; reenvia a fase se o cliente chegar tarde
- Rebuild em seguida

### 2026-08-16 — orquestrador (jogabilidade)

Usuário: sem controles, sprites invertidos, atores fora do mapa.

- Novo `PlayTouch.cs` (input de tela, não uGUI)
- `WorldView.DrawSpr` sem width negativo; `DrawControls` visível
- `Sim` clamp X/Y; Lift 28 passos
- `GameMenu.PlayHud` sem pads fantasmas
- Rebuild APK em seguida

---

### 2026-08-17 — orquestrador (v3.0: armadilhas, coop, arte, pausa)

Rodada feita **em sessão única, sem subagentes** — o usuário não pediu paralelismo e o recorte
(um motor de 3,5 k linhas) cabia numa cabeça só. Papéis exercidos em sequência pelo orquestrador:

| papel | o que rendeu |
|---|---|
| Auditor Unity | achou a causa do fosso (`p.Vy = -8` todo frame), a `Bosses.Add` dentro do `foreach`, o `BeginGroup` sem par, e **5 beats que fechavam a fase** |
| Implementador Unity | `Sim`, `LevelBuilder`, `WorldView`, `GameMenu`, `NightApp`, `GameNet`, `PlayTouch` |
| Artista | `ArtGen.cs` e `Sfx.cs` novos, do zero |
| Verificador | `Smoke.cs` de 20 → 55 checks; `ArtDump.cs` para olhar a arte sem aparelho |
| Builder | smoke em batchmode, depois APK |

**Achado que ninguém tinha relatado:** os mundos 2, 6 e 13 eram **impossíveis de terminar**.
`rooms`, `towers`, `nave` e `hall` erguiam paredes de 6 a 13 tiles apoiadas no chão; o herói pula
5. O `cliffs` tinha um abismo 9 tiles abaixo da borda, sem hazard — quem caía ficava preso para
sempre, sem nem poder morrer. Corrigido na geometria **e** blindado por dois passes automáticos
(`Ramp` e `SealUnreachable`) mais dois testes de smoke.

**Método que valeu a pena:** `ArtDump` compõe um frame por software e grava PNG. Foi olhando
esses PNGs que apareceram (a) o gradiente do céu invertido, que apagava todo o parallax, e
(b) a grade de tiles repetidos. Os dois foram corrigidos antes de gerar o APK — nenhum ciclo
gasto no aparelho.

Arquivos tocados: `Sim.cs`, `LevelBuilder.cs`, `WorldView.cs`, `GameMenu.cs`, `NightApp.cs`,
`PlayTouch.cs`, `GameData.cs`, `Net/GameNet.cs`, `Editor/Smoke.cs`, `Editor/AndroidBuilder.cs`.
Novos: `ArtGen.cs`, `Sfx.cs`, `Editor/ArtDump.cs`.

### 2026-08-17 — Builder

- `Nightfall.Editor.Smoke.Run` → **SMOKE OK 55 checks**, 0 `error CS`
- `Nightfall.Editor.ArtDump.Run` → `tools/artdump/` (atlas, FX, ícones, mapas de fase, frames)
- Primeiro APK 01:15 validou o toolchain; código de rede foi enxugado depois e o build refeito
- APK final **01:20 · 163 860 441 bytes · versão 3.0 / bundle 10 · arm64-v8a + armeabi-v7a**
- `adb devices` continua vazio nesta máquina — instalação e teste no aparelho ficam com o usuário

---

### 2026-08-17 — orquestrador (v3.1: física de plataforma e combate)

Usuário: fases ainda intransponíveis **incluindo a primeira**, "torre reta", plataformas que o
personagem atravessa, golpes sem peso.

**Lição da rodada anterior:** a v3.0 declarou as 16 fases atravessáveis com um teste de *geometria*
e entregou um jogo impossível de terminar. Dois furos, ambos meus:

1. O BFS ligava colunas olhando só altura e distância, **sem olhar o que havia no meio** — passava
   por cima de uma parede maciça de 8 tiles como se não existisse. Foi assim que 16 arenas muradas
   passaram no teste.
2. Geometria não é jogabilidade. As plataformas eram alcançáveis no mapa e intocáveis na prática,
   porque a física movia o frame inteiro de uma vez e o pé pulava a janela de pouso de 10 px.

Correção de método: agora existe um **bot que joga** (`Smoke.CheckPlayable`, `CheckFullLoop`) com a
física real, e um `Smoke.Survey` que mede taxa de sucesso em 8 sementes por fase fora do build.
Foi o bot que achou a arena murada, e foi a medição do `Survey` (`mortes=14, terminou na coluna 63`)
que mostrou que o mundo 16 não travava — morria e voltava ao início por falta de santuário.

Também aprendi a desconfiar de teste sem semente: o mesmo commit passou solto e falhou no build.

Arquivos tocados: `Sim.cs` (física em sub-passos, varredura da lâmina, hit-stop, descer plataforma),
`LevelBuilder.cs` (arena aberta, `Clear` no BFS, travessia de lava), `GameData.cs` (alcance do Mike,
santuário do Trono), `WorldView.cs` (arco preso ao herói, conjuração, cauda de projétil, ondas de
choque), `Editor/Smoke.cs` (77 checks + Survey).

---

## Fila (próximos disparos, se precisar)

1. **Teste no aparelho** — é o único item que não dá para fechar aqui: `adb devices` continua
   vazio nesta máquina. Conferir na mão o coop com dois celulares, a pausa pelo botão voltar e
   o Sair.
2. Implementador JS — o cliente HTML em `game/js` ficou para trás desta rodada (bounce, gelo e
   resgate de armadilha só existem no Unity). Espelhar se o WebView voltar a ser alvo.
3. Artista — sprites de FX pintados (`slash`, `bolt`) e `*_ground` por mundo, se o procedural
   deixar de bastar.
4. Netcode — hoje o cliente é espelho puro. Predição local do próprio herói tiraria a borracha
   em Wi-Fi ruim.

---

### 2026-08-29 — orquestrador (versionamento e publicação)

O projeto não tinha git: o `.git` em `D:\PROJETOS` era uma pasta vazia, não um repositório.

- `git init` na raiz do projeto, branch `main`, commit inicial com 449 arquivos / ~198 MB
- Repositório **público**: https://github.com/mikerock12/MikeAndDenyse-Nightfall
- `.gitignore` pela regra "só entra fonte". Bloqueante de verdade eram os dois APKs
  (163 MB e 118 MB), acima do limite rígido de 100 MB por arquivo do GitHub. Também ficaram
  de fora, por serem regeneráveis:

  | fora | regenerado por |
  |---|---|
  | `NightfallUnity/Library`, `Logs`, `.utmp` (8,7 GB) | o Unity ao abrir o projeto |
  | `Assets/Resources/Art` (82 MB) | `AndroidBuilder.PrepareProject()` a partir de `StreamingAssets/art` |
  | `android/app/src/main/assets/www` (109 MB) | `python tools/package.py` a partir de `game/` |
  | `android/app/build` (350 MB), `_unused` (114 MB), `UnityNightfall` | build / cópias mortas |

  Consequência a lembrar: **a fonte da arte é `StreamingAssets/art`**. Quem clonar e abrir no
  Editor sem rodar o builder vê o fallback procedural até o primeiro `PrepareProject`.

- `README.md` novo: arquitetura, laço principal, física em sub-passos, geração de fases,
  multiplayer local (descoberta UDP → sessão NGO → autoridade → RPCs → `FrameSnap`), catálogo,
  build, testes e o que está fora do repo.
- `docs/screenshots/` com 7 capturas do usuário (título, seleção, sala no host, sala no cliente,
  mapa, e o coop nos dois celulares na noite 7). As duas telas de sala mostram o mesmo
  `192.168.1.3` nos dois aparelhos — é a prova visual de que `LanIp.Pick()` está anunciando o
  IPv4 do Wi-Fi, e não o dos dados móveis.
- `LICENSE`: **todos os direitos reservados** ao autor. Repositório público para leitura;
  nenhuma licença de uso concedida.
