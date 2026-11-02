# Contexto Operacional — Mike & Denyse: Nightfall (`contexto.md`)

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
