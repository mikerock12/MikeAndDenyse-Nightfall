# Mike & Denyse: Nightfall — Guia de Agentes (`AGENTS.md`)

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

Leia `README.md`, `CONTEXT.md`, `PLAN.md`, `MULTIAGENTS.md` e `contexto.md` antes de alterar qualquer arquivo deste repositório (`mikerock12/MikeAndDenyse-Nightfall`).

## Regras Inegociáveis

1. **Preservação do legado**:
   - É proibido apagar `NightfallUnity/`, `game/` ou `android/`. O projeto nativo em C++20 + raylib vive em `NightfallNative/` lado a lado com o projeto Unity existente, reutilizando os sprites em `NightfallUnity/Assets/StreamingAssets/art/`.
2. **Paridade 1:1 com a especificação de gameplay (`NightfallUnity/Assets/Scripts/`)**:
   - Heróis **Mike** e **Denyse**, **16 mundos**, **32 inimigos regulares**, **16 chefes + fase Belial** no Mundo 16, física sub-step (`MaxStep = 6f`, `Gravity = 980f`, `TerminalVy = -620f`), gerador `LevelBuilder` com `Ramp` BFS + `Clear` skyline + `SealUnreachable`, `RememberSafe`/`Rescue`, arte procedural `ArtGen`, áudio procedural `Sfx` (12 sons a 22050 Hz) e protocolo LAN UDP (`47888` descoberta + `7777` estado `FrameSnap` v2 a 20 Hz).
3. **Validação determinística**:
   - O runner C++ (`NightfallTests`) deve portar e aprovar todas as **77 verificações** de `Smoke.cs`, incluindo o bot autônomo nos 16 mundos × 3 seeds e o loop completo nos mundos 1, 8 e 16 (com derrota de Belial).
4. **Sem comandos Git destrutivos e sem assinatura de IA**:
   - Nunca executar `git reset --hard`, `git clean -fdx`, `git restore .`, `git checkout -- .` ou `git push --force`.
   - Nunca incluir `Co-Authored-By` de IA em commits ou PRs.
5. **Documentação obrigatória por etapa**:
   - Atualizar sempre `AGENTS.md`, `contexto.md` e `CONTEXT.md` ao final de cada etapa concluída.

## Comandos Operacionais

- Testes de ferramentas existentes:
  - `python tools/test_progress.py`
  - `node tools/smoke_levels.js`
- Build e testes C++ (`NightfallNative`, a partir da Fase G5):
  - `cmake --preset windows-msvc-release`
  - `cmake --build --preset windows-msvc-release`
  - `ctest --preset windows-msvc-release --output-on-failure`

## Registro de Etapas

- **2026-11-01 / Fase G0 (Inventário, Alinhamento de Branch e Baseline P0)**: Preservado estado local anterior (`bcfa560`) na branch local `archive/local-main-bcfa560` e alinhada `main` com `origin/main` (`38c72988062a53c1bc493c9c10085d9902ca132e`). Criados `AGENTS.md`, `contexto.md` e `package.json` raiz (`{"type": "commonjs"}` para isolar `tools/smoke_levels.js` quando o repositório está dentro de um diretório pai ESM). Verificações baseline executadas: `python tools/test_progress.py` (`OK progress+lanip`) e `node tools/smoke_levels.js`. Ausência de `Unity.exe` local registrada com transparência (`BLOCKED_UNITY_EDITOR_LOCAL`). Próxima etapa: Fase G5 (fundação de `NightfallNative` em C++20 + raylib e port das 77 verificações de `Smoke.cs`). Responsável: Coordenador SLT Games.

- **2026-11-01 / Fase G1 Concluída no Ecossistema SLT Games**: `mikerock12/RuaDeAco` concluiu a exportação de `golden_fixtures.json` (PR #9 mesclado em `b41dd21`, 591 testes cliente + 71 servidor). Este repositório (`MikeAndDenyse`) permanece alinhado e pronto para iniciar as Fases G5–G6 (`NightfallNative` C++20 + raylib). Responsável: Coordenador SLT Games.

- **2026-11-01 / Fases G2 e G3 Concluídas no Ecossistema SLT Games**: `RuaDeAcoUnreal` concluiu a paridade C++20 + raylib + CTest (44/44 checks). Próxima etapa imediata após G4 (`RuaDeAco` Windows/Android): Fase G5 (`NightfallNative` neste repositório). Responsável: Coordenador SLT Games.

- **2026-11-01 / Fase G4 Concluída no Ecossistema SLT Games**: `RuaDeAcoUnreal` concluiu o empacotamento Windows (`.exe`/`.zip`) e Android ARM64 (`.apk`). Iniciando agora as Fases G5 e G6 neste repositório (`NightfallNative` C++20 + raylib, 77 checks de `Smoke.cs`, LAN UDP e empacotamento Windows/Android). Responsável: Coordenador SLT Games.

- **2026-11-01 / Fases G5 e G6 (`NightfallNative` C++20 + raylib, 77 Checks, UDP LAN e Empacotamento v1.0.0 Concluídas)**: Criado o projeto nativo completo `NightfallNative/` em C++20 + raylib 5.5 lado a lado com `NightfallUnity/` (preservado intacto e com `AndroidBuilder.cs` parametrizado por variáveis de ambiente). Portados 1:1 `Catalog` (2 heróis, 32 inimigos, 16 chefes + Belial, 16 mundos), `LevelBuilder` (`Ramp` BFS + `Clear` skyline + `SealUnreachable`), `GameSim` (sub-step `MaxStep = 6f`, `RememberSafe`/`Rescue`, `MeleeSweep`, `FireMagic`, `BelialPhase`), `Sfx` procedural (12 sons a 22050 Hz), `SpriteBank` (`PunchMagenta`), e protocolo cooperativo LAN UDP (`47888` descoberta + `7777` `FrameSnap` v2 a 20 Hz). CTest 2/2 suítes aprovadas (`77/77` checks de `Smoke.cs` + loopback UDP LAN + benchmark 10.000 ticks em `3,95 ms` = `0,00039 ms/tick`). Gerados e verificados os pacotes v1.0.0:
  - `MikeAndDenyse-1.0.0-Windows-x64-Setup.exe` (`93.543.731` bytes, SHA-256 `c9fc84701b71d5cac41b6ef0fa9aef63a7958f49e41a0763d553bda0ab500033`)
  - `MikeAndDenyse-1.0.0-Windows-x64.zip` (`91.604.492` bytes, SHA-256 `a03c9d6920ba3885ebe384197a05edc1dd48ee25840244a55b4a007161a0556d`)
  - `MikeAndDenyse-1.0.0-Android-arm64.apk` (`90.773.704` bytes, SHA-256 `a5663ad5551064879e9d5c1008f2a3e680558bbdfcfbf9a04162fc46b928c841`, `apksigner verify` v2/v3 OK)
  Próxima etapa: PR/merge em `main` e Fase G7 (Releases GitHub). Responsável: Coordenador SLT Games.
- **2026-11-01 / Fase G7 Concluída**: PR #1 mesclado em `main` (`07f6ac8`) e publicada a Release GitHub estável `v1.0.0` (`https://github.com/mikerock12/MikeAndDenyse-Nightfall/releases/tag/v1.0.0`) com `MikeAndDenyse-1.0.0-Windows-x64-Setup.exe` (`93.543.731 B`, SHA-256 `c9fc8470...00033`), `MikeAndDenyse-1.0.0-Windows-x64.zip` (`91.604.492 B`, SHA-256 `a03c9d69...0556d`), `MikeAndDenyse-1.0.0-Android-arm64.apk` (`90.773.704 B`, SHA-256 `a5663ad5...8c841`) e `SHA256SUMS.txt`. Próxima etapa: Fase G8 (integração de `/games/` no portal). Responsável: Coordenador SLT Games.
- **2026-11-01 / Fase G8 Concluída**: Página `/games/mike-and-denyse/` publicada no portal `https://softwares.smellsliketech.com.br/games/mike-and-denyse/` (PR #38, merge `5fe367e`, Worker `f24ab0eb-d181-435b-b8ce-5764468138c8`) com API `/api/games/releases/mike-and-denyse-*` (`200`) e redirects `/download/games/mike-and-denyse-*` (`302`) validados em produção. Próxima etapa: Fase G9 (Backups Google Drive). Responsável: Coordenador SLT Games.

## Histórico do gate nativo — 02/11/2026 (encerrado em 03/11)

PR #2 aberto em `codex/nightfall-native-fidelity`. CI hospedado executa builds Windows/Android e a comparação C# original; não está bloqueado por cobrança nesta rodada. O emulador real detectou primeiro o tutorial de tela cheia e depois a falha de leitura de `art/index.txt` pelo fallback da raylib: o APK continha o índice e as 77 imagens, mas o app indexava zero. Os callbacks leem diretamente pelo AAssetManager. A causa ZIP e a escala da superfície foram corrigidas; o CI final 41588036821 foi aprovado em 03/11, preservando e reforçando as assertions.

Windows usa runtime C++ estático (/MT). Auditoria PE: executável importa somente raylib e DLLs Windows. Instalação/desinstalação reais isoladas, comparação SHA-256 dos 80 arquivos com ZIP, headless e renderização instalada dos 16 mundos passaram; recibos em `NightfallNative/Saved/InstallerSmoke/`. Nenhuma instalação ou save existente foi substituído. Compatibilidade do certificado Android publicado foi conferida após autorização explícita do usuário; o APK final aprovado está assinado. Merge, release pública, portal e backup têm registro final pelo coordenador do portal. Responsável: Codex / agente Nightfall.

## Empacotamento Android — 03/11/2026

Identificada a causa comprovada dos assets ausentes no emulador: o aapt2 Windows gravava nomes ZIP com barras invertidas nos cabeçalhos locais e centrais. Python no Windows normalizava `ZipInfo.filename` durante a leitura, ocultando o defeito; Android e Linux preservam os nomes literais. O build agora normaliza `orig_filename` antes de alinhar/assinar e valida cabeçalhos, CRC, índice e todos os 77 PNG. Teste local no APK real detectou 78 entradas defeituosas e verificou a correção. Callbacks AAssetManager permanecem; CI Android 41588036821 e assinatura do APK final foram aprovados em 03/11. O coordenador registra a publicação. Responsável: Codex / agente Nightfall.

## Superfície Android — 03/11/2026

CI 41586931399 confirmou o índice de 77 assets e o título com arte original após normalizar o ZIP. O gate seguinte detectou escala vertical incorreta e menus inacessíveis: a raylib 5.5 combinava o tamanho lógico fixo com seu próprio escalonamento da superfície Android. O frontend passa a iniciar na resolução real da superfície (`InitWindow(0,0)` apenas Android), preservando o framebuffer de jogo 960×540 e seu viewport. A janela é configurada em paisagem antes de iniciar a thread nativa. QA passa a exigir o estado esperado em cada captura e sempre exportar logcat/Activity/Window quando falha. CI final 41588036821 aprovado nos três jobs em 03/11; registro de release/publicação pelo coordenador. Responsável: Codex / agente Nightfall.

QA Android também executa o mesmo APK em superfícies 16:9 (960×540) e 5:3 (1000×600), limpando somente os dados do pacote descartável `.qa`, exigindo cada estado de menu e o mesmo PID após background/retorno. Ambos os percursos passaram no CI 41588036821, com 22 capturas e logs de evidência. Responsável: Codex / agente Nightfall.
