# MULTIAGENTS — Mike & Denyse: Nightfall

## Nightfall1.2.0 publicado — 04/11/2026

Release estavel [v1.2.0](https://github.com/mikerock12/MikeAndDenyse-Nightfall/releases/tag/v1.2.0), PR6/merge82bd25e619223ffee212842442f27f8706723153. Instalador e ZIP Windows, APK ARM64 e SHA256SUMS conferidos contra os digests remotos. Mesma identidade Android/v2/v3/16KiB e icones proprios; Windows GUI sem console. As releases anteriores permanecem imutaveis.

Audio Suno aplicado:16 loops de cenario,51 cortes PT-BR (Mike/Denyse/17 chefes) e8 variantes editadas de uma paleta gerada. O proprietario ouviu/aprovou a amostra de voz. Volumes separados, musica reduzida no dialogo, pausa/background e uma musica/uma fala carregadas por vez. Regras C#/C++ e LAN preservados. Matrizes18WAV/292665120bytes em D:\PROJETOS\SoftwaresSmellsLikeTech\MikeAndDenyse\NightfallNative\AudioMasters\Suno-20261104, ignoradas no Git e nos pacotes; incluir no backup privado. Procedencia/hashes dos75OGGs em NightfallNative/Content/audio/provenance.json. Nao afirmar que a paleta original cumpriu os16segundos/oito eventos pedidos: ela tem4,72s e recebeu edicao local. Nenhuma compra/assinatura foi alterada ou voz pessoal clonada.

CIs PR41795196087 e main41797055296 aprovados nos tres jobs; CTest3/3,100.800 passos contra C#/16mapas;75 payloads verificados noAPK, inicializacao real de musica/efeitos/banco noAndroid emulado16:9/5:3 com34capturas, ambos herois, preferencias,pausa/retorno. Audio-smoke do jogo Windows exit0 decodificou16streams/51falas/8efeitos e conferiu filas dos17chefes. Corrigida a orientacao Android transitoria com dimensoes fisicas antes do framebuffer; titulo e toques alinhados nas capturas reais. Windows portatil renderizou16mundos e cinco configuracoes; recursos PE/icone conferidos. Instalacoes existentes preservadas; nao afirmar nova reinstalacao desta release.

Consolidacao dos backups privados de fontes/historico,pacotes,evidencias e matrizes pelo portal; recibos privados ficam somente em .work. Os registros anteriores de implementacao/publicacao pendente sao historicos. Sem homologacao em ARM64 fisico (atualizacao instalada,audio/desempenho,multitouch) nem coop entre dois aparelhos. Responsavel: Codex.

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


### 2026-11-02 — agente Nightfall / correção da publicação nativa

- Reutilizou o core C++ existente e preservou Unity/WebView. NativeActivity agora usa o mesmo frontend raylib do Windows, com menus por toque, pads multitouch, audio e atlas original portado, save privado e LAN nativo versionado.
- Corrigiu IPv4 falso, recebimento incompleto de chefe/Belial, roster fantasma, ataques remotos, retransmissão de início/retry, ordenação de snapshots e drenagem de FX.
- Compilou três fontes C# originais inalterados com adaptador MathF/RNG controlado: 100.800 passos e 16 hashes de mapa passaram com erro máximo de 0,00024 (tolerância 0,001). Isso não executa o renderizador nem o RNG Unity.
- Windows real: instalador /CURRENTUSER isolado, 80 arquivos iguais ao ZIP, headless e render dos 16 mundos, uninstall exit0; runtime /MT e imports PE sem MSVCP/VCRUNTIME.
- CI real revelou tutorial fullscreen e, depois dele, assets zero apesar dos 77 PNG no APK. Corrigiu leitura do aplicativo via callbacks AAssetManager diretos; não reduziu as assertions. PR #2 coordena a entrega; QA Android final aprovado em 03/11, publicação registrada pelo coordenador.
- Keystore Android existente só foi reutilizado após autorização explícita do usuário; certificado do candidato comparado ao APK público 1.0.0, sem expor senha. APK final do código aprovado foi assinado em 03/11, com certificado idêntico à versão pública anterior.
- Coordenação/portal/release/Drive permanecem com o coordenador. Nenhuma homologação física declarada.

## Empacotamento Android — 03/11/2026

Identificada a causa comprovada dos assets ausentes no emulador: o aapt2 Windows gravava nomes ZIP com barras invertidas nos cabeçalhos locais e centrais. Python no Windows normalizava `ZipInfo.filename` durante a leitura, ocultando o defeito; Android e Linux preservam os nomes literais. O build agora normaliza `orig_filename` antes de alinhar/assinar e valida cabeçalhos, CRC, índice e todos os 77 PNG. Teste local no APK real detectou 78 entradas defeituosas e verificou a correção. Callbacks AAssetManager permanecem; CI Android 41588036821 e assinatura do APK final foram aprovados em 03/11. O coordenador registra a publicação. Responsável: Codex / agente Nightfall.

## Superfície Android — 03/11/2026

CI 41586931399 confirmou o índice de 77 assets e o título com arte original após normalizar o ZIP. O gate seguinte detectou escala vertical incorreta e menus inacessíveis: a raylib 5.5 combinava o tamanho lógico fixo com seu próprio escalonamento da superfície Android. O frontend passa a iniciar na resolução real da superfície (`InitWindow(0,0)` apenas Android), preservando o framebuffer de jogo 960×540 e seu viewport. A janela é configurada em paisagem antes de iniciar a thread nativa. QA passa a exigir o estado esperado em cada captura e sempre exportar logcat/Activity/Window quando falha. CI final 41588036821 aprovado nos três jobs em 03/11; registro de release/publicação pelo coordenador. Responsável: Codex / agente Nightfall.

QA Android também executa o mesmo APK em superfícies 16:9 (960×540) e 5:3 (1000×600), limpando somente os dados do pacote descartável `.qa`, exigindo cada estado de menu e o mesmo PID após background/retorno. Ambos os percursos passaram no CI 41588036821, com 22 capturas e logs de evidência. Responsável: Codex / agente Nightfall.

## Validação final da entrega 1.0.1 — 03/11/2026

CI 41588036821 aprovado (Windows/100.800 comparações/Android e dois percursos emulados 16:9 e 5:3). Windows instalado e desinstalado isoladamente, 80 hashes e render dos 16 mundos aprovados; APK ARM64 assinado com identidade preservada. Detalhes e hashes em NightfallNative/docs/FIDELITY_REPAIR.md. Publicação e backup têm registro final nos contextos do portal pelo coordenador. Limites físicos descritos em AGENTS.md. Responsável: Codex / agente Nightfall.


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

## Rotacao Android ainda transitoria — correcao em validacao, 04/11/2026

O CI Nightfall41793542473 passou Windows e builds Android/banco75, mas a captura de titulo mostrou compressao vertical. Log comprovou SLT_BUFFER_GEOMETRY960x540/result0 seguido de display540x960 e offsets0,656: ANativeWindow continuou reportando retrato durante a rotacao, mesmo apos aceitar o novo buffer. O audio inicializou corretamente (16musicas/51falas/8efeitos); ele nao explica o defeito visual.

A copia identificada do backend raylib agora usa diretamente as dimensoes fisicas obtidas da Activity para SetupFramebuffer, conservando o tamanho bruto reportado apenas para diagnostico. A matriz nao deve incorporar a orientacao transitoria. Upstream5.5 e seu SHA continuam intactos. QA exige SLT_PHYSICAL_RENDER_SIZE igual a captura, alem das assertions visuais e toques existentes; nenhuma assertion foi afrouxada. Mesma correcao aplicada aos dois jogos: Nightfall1.2.0 ainda candidato e manutencao Rua de Aco0.4.1/code401, sem substituir tags0.4.0/1.1.1 publicadas. Novos gates/builds/publicacao pendentes. Responsavel: Codex.
