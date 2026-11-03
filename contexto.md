# Contexto Operacional — Mike & Denyse: Nightfall (`contexto.md`)

## Correção nativa em validação — 02/11/2026

Frontend C++ compartilhado Windows/Android (raylib NativeActivity), menus touch, comandos com bordas multitouch, pausa/back, arte original e atlas procedural, SFX, progresso em diretório gravável e LAN com IPv4 real implementados. Transporte nativo v3 usa campos explícitos little-endian, sem padding de ABI, e rejeita salas/protocolos incompatíveis. Unity e demais referências preservados.

Verificações locais: CTest 2/2; comparação independente de três fontes C# originais inalteradas com C++ em 100.800 passos e hashes dos 16 mapas (tolerância 0,001 px, maior diferença 0,00024 px). A referência de teste usa adaptadores MathF e RNG controlado, não o runtime gráfico Unity. Render smoke Windows exportou os 16 mundos pelo frontend real. Candidatos 1.0.1 Windows e Android compilados; APK de QA tem pacote/chave separados. CI com emulador Android, revisão visual/funcional final, assinatura da atualização, PR/merge, releases, portal e backup final ainda precisam ser concluídos; nenhum aparelho físico está disponível. Não tratar estes resultados como homologação física ou equivalência integral de toda a interface. Método e evidências: NightfallNative/docs/FIDELITY_REPAIR.md (se presente), testes de referência e relatórios em NightfallNative/Saved/. Backup antigo do histórico no Drive falhou em restauração por corrupção do bundle; ZIP de fontes recuperável, backup novo exige clone/restauração real. Responsável: Codex.

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

## Gate de distribuição nativa — 02/11/2026

PR #2 aberto em `codex/nightfall-native-fidelity`. CI hospedado executa builds Windows/Android e a comparação C# original; não está bloqueado por cobrança nesta rodada. O emulador real detectou primeiro o tutorial de tela cheia e depois a falha de leitura de `art/index.txt` pelo fallback da raylib: o APK continha o índice e as 77 imagens, mas o app indexava zero. Os callbacks agora leem diretamente pelo AAssetManager; as assertions foram preservadas e o novo resultado Android ainda é necessário.

Windows usa runtime C++ estático (/MT). Auditoria PE: executável importa somente raylib e DLLs Windows. Instalação/desinstalação reais isoladas, comparação SHA-256 dos 80 arquivos com ZIP, headless e renderização instalada dos 16 mundos passaram; recibos em `NightfallNative/Saved/InstallerSmoke/`. Nenhuma instalação ou save existente foi substituído. Compatibilidade do certificado Android publicado foi conferida após autorização explícita do usuário; o pacote final só pode ser liberado após QA e assinatura do código aprovado. Merge, release pública, portal e backup final continuam com o coordenador. Responsável: Codex / agente Nightfall.

## Empacotamento Android — 03/11/2026

Identificada a causa comprovada dos assets ausentes no emulador: o aapt2 Windows gravava nomes ZIP com barras invertidas nos cabeçalhos locais e centrais. Python no Windows normalizava `ZipInfo.filename` durante a leitura, ocultando o defeito; Android e Linux preservam os nomes literais. O build agora normaliza `orig_filename` antes de alinhar/assinar e valida cabeçalhos, CRC, índice e todos os 77 PNG. Teste local no APK real detectou 78 entradas defeituosas e verificou a correção. Callbacks AAssetManager permanecem; CI Android completo e assinatura do candidato final ainda pendentes. Não houve publicação. Responsável: Codex / agente Nightfall.

## Superfície Android — 03/11/2026

CI 41586931399 confirmou o índice de 77 assets e o título com arte original após normalizar o ZIP. O gate seguinte detectou escala vertical incorreta e menus inacessíveis: a raylib 5.5 combinava o tamanho lógico fixo com seu próprio escalonamento da superfície Android. O frontend passa a iniciar na resolução real da superfície (`InitWindow(0,0)` apenas Android), preservando o framebuffer de jogo 960×540 e seu viewport. A janela é configurada em paisagem antes de iniciar a thread nativa. QA passa a exigir o estado esperado em cada captura e sempre exportar logcat/Activity/Window quando falha. Novo resultado emulado ainda necessário; nenhum release publicado. Responsável: Codex / agente Nightfall.

QA Android também executa o mesmo APK em superfícies 16:9 (960×540) e 5:3 (1000×600), limpando somente os dados do pacote descartável `.qa`, exigindo cada estado de menu e o mesmo PID após background/retorno. Ambos os resultados continuam necessários. Responsável: Codex / agente Nightfall.
