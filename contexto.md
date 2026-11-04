# Contexto Operacional — Mike & Denyse: Nightfall (`contexto.md`)
## Plataforma e icones Android — 03/11/2026

Nightfall1.1.0 implementa configuracoes persistentes, icones APK tradicionais/adaptativos derivados dos dois personagens aprovados e executavel Windows GUI sem prompt. Render4K, janela/sem bordas/fullscreen, VSync/FPS, brilho, teclado P1/P2, dois gamepads e deadzone; Androidtouch ajustavel/canhoto/opacidade/margem e vibracao. Simulacao frontend fixa60Hz; Core/Unity preservados. Musica/voz aparecem quando houver conteudo; efeitos/volume geral ja efetivos. CTest3/3, gate PE GUI e CI41756274066 aprovados nos tresjobs. Androidemulado2proporcoes/34capturas com settings5categorias, preferencias apos reiniciar,2herois e pausa/ciclo de vida. Windows portatil16mundos+5settings reais; icones PE dos quatroEXEs/PNG interno e15PNG compilados por APK conferidos. APK ARM64 v2/v3 com certificado anterior e alinhamento16KiB. Instalacao Nightfallexistente preservada; reinstalacao nao repetida. Suno por ultimo conforme usuario, verificacao do site ainda impede gerar audio; nenhum clipnovo. Release/merge/portal/backup coordenados no portal.

Sem homologacao Android fisica, audio ouvido, desempenho/toque fisicos ou dupla Wi-Fi. Responsavel: Codex.



Em 03/11/2026, o CI 41589263103 expôs um gesto adb sem duração perdido no menu em GPU de software. O driver passa a injetar toque imóvel de 120 ms e aguardar a tela esperada por até 12 s, sem reenviar ações nem afrouxar a igualdade de estado. Aplicação e candidatos permanecem inalterados; repete-se o CI para esse ajuste de QA. Responsável: Codex.

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

**Última atualização:** 01/11/2026  
**Repositório:** `https://github.com/mikerock12/MikeAndDenyse-Nightfall`  
**Branch principal:** `main`  
**Documento técnico complementar:** [`CONTEXT.md`](CONTEXT.md) · [`PLAN.md`](PLAN.md) · [`MULTIAGENTS.md`](MULTIAGENTS.md)

---

## 1. Visão Geral

**Mike & Denyse: Nightfall** é um jogo de ação e plataforma 2D dark-fantasy cooperativo em pixel art com **16 mundos**, **32 inimigos regulares**, **16 chefes de mundo** (incluindo **Belial** com segunda fase no Mundo 16), geração procedural determinística de fases com garantia de travessia por BFS (`LevelBuilder` + `Ramp` + `SealUnreachable`), física com sub-stepping (`MaxStep = 6f`), arte procedural (`ArtGen`) combinada com sprites em pixel art (`StreamingAssets/art/`), síntese sonora procedural (`Sfx`, 12 timbres a 22050 Hz) e modo cooperativo em rede local Wi-Fi/LAN via UDP (descoberta na porta `47888` e sessão host-authoritative na porta `7777`).

---

## 2. Estado Atual (01/11/2026 — Fase G0 Baseline)

- **Alinhamento Git**:
  - `origin/main` em `38c72988062a53c1bc493c9c10085d9902ca132e` (commit `bcfa560` anterior preservado sem perda em `archive/local-main-bcfa560`).
  - Branch de trabalho criada: `feat/nightfall-native-cpp`.
- **Ambiente Local**:
  - Compilador MSVC 2022 (`v17.14.44`), CMake (`4.4.4` / `3.22.1`), Ninja, Python `3.14.10`, Node `v26.13.0`, Android SDK 36 + NDK `27.0.12077973` e Inno Setup 6 disponíveis e verificados.
  - `Unity.exe` não está instalado localmente (`BLOCKED_UNITY_EDITOR_LOCAL`); por isso, conforme o plano operacional, a distribuição oficial de produção Windows (`.exe` + `.zip`) e Android ARM64 (`.apk`) será gerada a partir de `NightfallNative/` (C++20 + raylib 5.5), preservando `NightfallUnity/` intacto.
- **Testes de Baseline P0**:
  - `python tools/test_progress.py`: **PASS** (`OK progress+lanip`).
  - Adicionado `package.json` raiz com `"type": "commonjs"` para permitir execução direta de `node tools/smoke_levels.js` sem conflito com diretórios pais ESM.

---

## 3. Histórico de Etapas

| Data | Fase / Marco | Verificações | Responsável |
|---|---|---|---|
| 01/11/2026 | **Fase G0**: Inventário, preservação de branches, criação de `AGENTS.md` e `contexto.md`, baseline P0 | `test_progress.py` e `smoke_levels.js` verificados | Coordenador SLT Games |
| 01/11/2026 | **Fase G1**: Sincronização de marco — `RuaDeAco` PR #9 (`b41dd21`) mesclado com fixtures golden | Estado de `MikeAndDenyse` íntegro aguardando Fase G5 | Coordenador SLT Games |
| 01/11/2026 | **Fases G2 e G3**: Sincronização de marco — `RuaDeAcoUnreal` C++20/CTest 44/44 aprovado | Pronto para iniciar Fase G5 após empacotamento G4 | Coordenador SLT Games |
| 01/11/2026 | **Fase G4**: Sincronização de marco — `RuaDeAcoUnreal` v0.2.0 empacotado (Win64 + Android ARM64) | Início imediato da Fase G5 (`NightfallNative`) | Coordenador SLT Games |
| 01/11/2026 | **Fases G5 e G6**: `NightfallNative` C++20 + raylib, CTest 77/77 checks + LAN UDP loopback (`3,95 ms` / 10k ticks), empacotamento Win64/Android v1.0.0 | `.exe` (`93.543.731 B`), `.zip` (`91.604.492 B`), `.apk` (`90.773.704 B`, assinado v2/v3) | Coordenador SLT Games |
| 01/11/2026 | **Fase G7**: Merge PR #1 (`07f6ac8`) + Publicação da Release GitHub estável `v1.0.0` (`https://github.com/mikerock12/MikeAndDenyse-Nightfall/releases/tag/v1.0.0`) | 4 assets publicados e verificados por digest SHA-256 | Coordenador SLT Games |
| 01/11/2026 | **Fase G8**: Publicação de `/games/mike-and-denyse/` no portal (`5fe367e`, Worker `f24ab0eb-d181-435b-b8ce-5764468138c8`) | Páginas 200, APIs 200 com SHA-256 v1.0.0 e redirects 302 validados | Coordenador SLT Games |

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


## Atualizacao de plataforma 1.1.0 — em validacao, 03/11/2026

Configuracoes persistentes de tela, qualidade, audio, teclado/controle e touch integradas ao frontend compartilhado. Android tem HUD sempre visivel, tamanho/opacidade/margem/canhoto e vibracao; icones tradicionais e adaptativos derivam da mesma arte Windows aprovada, com procedencia em Platform/Android/Branding (NightfallNative no Nightfall). Windows oferece resolucao, modos de janela, VSync, limites FPS e render interno ate 4K. Nightfall usa subsistema GUI e simulacao fixa 60Hz. Rua de Aco abre o cliente Web oficial integrado para compatibilidade de salas; combate offline continua C++.

Candidatos locais compilaram para Windows e Android x86_64; CTest anterior 4/4 RDA e 3/3 Nightfall. QA ampliado exige configuracoes por toque e persistencia apos reiniciar; CI, online real, empacotamento final, novas capturas, releases, portal e backups ainda pendentes. Suno fica por ultimo por instrucao do usuario, indisponivel por verificacao do site; nenhum audio novo gerado. Sem homologacao Android fisica. Responsavel: Codex.


### Configuracoes e icones APK — 03/11/2026, em validacao
Configuracoes de plataforma e touch persistentes, icones Android da arte Windows aprovada, pausa de audio no background e executavel Windows GUI implementados. Simulacao de frontend desacoplada do FPS; P2 aceita segundo gamepad. QA passou a conferir 15 recursos PNG compilados por jogo, dois XML adaptativos, preferencias apos reinicio e gestos reais no emulador. Primeiro CI encontrou falta de carregamento explicito da biblioteca para o JNI de lifecycle no Nightfall e seletor de borda obsoleto no driver RDA; corrigidos, gates serao repetidos. Nao ha nova release ainda. Audio Suno adiado para o fim pelo usuario, sem clips gerados; nao expor ajustes sem audio correspondente. Sem Android fisico/dupla Wi-Fi. Responsavel: Codex.


## Superficie Android — 03/11/2026

O gate pos-merge encontrou inicializacao intermitente com a imagem comprimida verticalmente, enquanto as areas de toque permaneciam nas coordenadas esperadas. A inicializacao passa a obter as dimensoes fisicas da janela Android via WindowMetrics/Display antes de InitWindow, evitando dimensoes zero e a orientacao ambigua da raylib. O QA de Nightfall exige o titulo na posicao correta e a abertura real das configuracoes. No RDA, o teste online aguarda o jogo terminar o preload antes de capturar. Nao afrouxar assertions nem atribuir o problema a persistencia sem examinar a imagem. Nightfall prepara1.1.1/code16, preservando release1.1.0; RDA0.4.0 ainda candidato. CI/build/publicacao desta correcao pendentes. Responsavel: Codex.


Em 04/11/2026, o log do segundo formato Nightfall comprovou NativeWindow temporaria de1x1, apesar de WindowMetrics1000x600. A raylib5.5 calculava screenScale com esse buffer temporario e mantinha o jogo preto/comprimido. cmake/AndroidSurface.cmake preserva o upstream imutavel (SHA-256 a17a8c75...) e compila apenas uma copia de rcore com ANativeWindow_setBuffersGeometry aplicado antes da consulta inicial de dimensoes. A alteracao e marcada no codigo gerado e no log SLT_BUFFER_GEOMETRY. Validacao em andamento, sem nova publicacao desta manutencao. Responsavel: Codex.

## Audio Suno integrado — candidato1.2.0, 04/11/2026

Produzidas32 versoes para16 trilhas no Suno v6;16 matrizes WAV exportadas e preservadas em NightfallNative/AudioMasters/Suno-20261104 (ignorado no Git, incluir no backup privado). Falas PT-BR em banco gerado no Suno: amostra ouvida e aprovada pelo proprietario,51 cortes para Mike/Denyse/17 chefes. Oito variantes de efeitos foram editadas a partir de uma paleta Suno de4,72s; nao afirmar que o gerador obedeceu ao pedido de16s/oito eventos isolados. Efeitos procedurais de interface/pickups continuam como fallback. Nenhuma voz pessoal foi clonada e nenhuma compra/assinatura alterada. A cota restante do plano existente foi usada nas exportacoes.

OGGs distribuidos em Content/audio:16 loops com emenda1,5s/-22LUFS,51 falas/-17LUFS e8 efeitos derivados. Procedencia, hashes das matrizes e dos75 arquivos estao em provenance.json; scripts Import-SunoAudio.py e Import-SunoVoicesEffects.py registram o processamento. WAVs originais nao entram em Git/APK/instalador. Transcricao local auxiliou os cortes; a revisao do usuario da amostra, e nao o ASR isolado, aprovou as vozes.

NativeAudio altera apenas apresentacao: musica por cenario, falas do heroi escolhido e chefe ao entrar na arena (dois herois no coop), musica reduzida durante dialogo, volume de musica/voz configuravel e pausa/retorno preservando reproducao. Carrega um stream musical e uma fala por vez, mantendo o buffer OGG vivo no Android. Assets Android usam AAssetManager e nomes ZIP portaveis. Regras C#/C++ e protocolo LAN intocados.

Windows candidato compilado e audio-smoke real com exit0:16 streams,51 falas e8 efeitos decodificados; mapeamento dos17 chefes conferido. APK ARM64 compilado com os75 payloads SHA-256 iguais aos aprovados, assinado com certificado anterior/v2/v3/16KiB. CI foi ampliado para banco completo e inicializacao real de musica/efeitos no emulador; sua execucao e a publicacao1.2.0 ainda estao pendentes. Releases1.1.0/1.1.1 preservadas. Nao substituir evidencias das versoes anteriores por capturas novas sem procedencia. Sem homologacao Android fisica/dupla Wi-Fi e sem repetir instalacao no host que ja tem o jogo. Responsavel: Codex.
