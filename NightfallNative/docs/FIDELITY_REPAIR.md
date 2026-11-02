# Correção de fidelidade nativa — 02/11/2026

Responsável pela implementação: Codex / agente Nightfall. O coordenador consolida QA, publicação e backups dos quatro projetos. **1.0.1 é um candidato; este documento não declara publicação nem homologação física.**

## Falhas corrigidas

- O APK 1.0.0 tinha `SurfaceView` e loop JNI sem renderização. `MainActivity` agora estende `NativeActivity`; `libnightfall.so` contém raylib e o mesmo `src/main.cpp` usado no Windows. A antiga ponte de simulação foi removida.
- Menus funcionam por toque, mouse, teclado e confirmação por gamepad. Há seleção dos dois heróis, mapa, sala LAN, pausa, retorno, morte e conclusão. O atalho de depuração que liberava todas as fases foi retirado da interface.
- Todos os dedos participam dos controles. Pulo e ataque disparam na borda, enquanto movimento e pulo sustentado permanecem ativos. Coordenadas usam o viewport 16:9, inclusive em telas mais largas. O botão Android Voltar pausa ou retorna ao menu anterior.
- Os 77 PNG originais são indexados no APK e carregados por callbacks explícitos de `AAssetManager_open/read`, inclusive para entradas comprimidas. O teste emulado detectou que o fallback `fopen/funopen` não abria o índice apesar de ele estar no APK; a correção está no aplicativo, sem reduzir a assertion de 77 assets. Sprites preservam proporção, pés e espelho horizontal; não usam largura de destino negativa. Texturas são carregadas sob demanda, limitadas a 512 px nos sprites e 1024 px nos cenários, sem alterar os fontes originais.
- `NativeWorldArt.h` porta os algoritmos de atlas 11 × 2, chão, plataformas, espinhos, sarças, líquidos, gelo, mola e escada de `ArtGen.cs`, além de céu, cristas, vinheta e corações. Decoração, atmosfera, golpes, projéteis, santuário e HUD dos dois heróis voltam a aparecer. O seed cosmético usa FNV estável em lugar do `String.GetHashCode` variável por processo do .NET; partículas e composição de menus não são uma comparação pixel a pixel com Unity.
- SFX incluem as misturas originais de impacto, pisada e chefe, guarda de 35 ms, volume e variação cosmética de pitch. Não usam o RNG da simulação.
- Progresso Windows vai para `%LOCALAPPDATA%/SmellsLikeTech/Nightfall`, com importação única do arquivo legado ao lado do executável. Android usa o diretório privado da atividade. A chave de assinatura existente não é substituída.
- `LanIp::Pick()` deixou de devolver `192.168.1.100` fixo e passa a escolher uma interface ativa real. Descoberta Android mantém um multicast lock enquanto a atividade está ativa.
- O cliente aplica identidade e tempo de golpe dos heróis, roster de inimigos, posição/vida/estado do chefe e transição para Belial. Não deixa inimigos antigos na tela. Snapshots priorizam inimigos vivos próximos da câmera.
- Início/reinício é retransmitido com identificador para não reiniciar a cada snapshot; saída do host e timeout encerram o cliente. Inputs de borda são retidos até consumo e movimento remoto expira quando deixam de chegar pacotes. Pausa do host é refletida ao cliente. A fila de FX é drenada também no solo.

## Transporte LAN nativo

O esquema de estado continua representado por `FrameSnap`, mas o transporte nativo é **v3**: escalares little-endian, sem copiar padding de structs C++. Fixtures verificam os bytes, tamanho, roundtrip, rejeição de truncamento, versão antiga, lixo final e NaN. Snapshots carregam geração da partida e sequência; pacotes atrasados/duplicados não reaplicam estado antigo nem reiniciam uma fase já trocada. Descoberta exige `nativeProtocol: 3`; mensagens usam `NF_HERO3`, `NF_START3`, `NF_END3`, `INP3` e `SNP3`.

Clientes nativos 1.0.1 comunicam entre si por UDP 47888/7777. Não há interoperabilidade com Unity/NGO nem com o protótipo nativo 1.0.0; essas salas incompatíveis são recusadas. Loopback real foi verificado. Dois celulares físicos continuam pendentes.

## Validação executada

`cmake --preset windows-msvc-release`, build MSVC e CTest passaram: duas suítes, incluindo as 77 verificações legadas, 16 mundos × 3 seeds, loops completos das fases 1/8/16 e regressões novas de snapshot, Belial, wire e sessão UDP.

O projeto `tests/reference/NightfallReference.csproj` compila **sem alterações** `Sim.cs`, `LevelBuilder.cs` e `GameData.cs` do Unity. O adaptador substitui somente APIs de matemática/vetor e a fonte de aleatoriedade para execução headless. Não é uma execução do Editor Unity. A comparação passou para:

- hashes integrais dos tiles dos 16 mapas, dimensões, spawn, arena e saída;
- 69.120 passos de física: 16 mundos × Mike/Denyse × 60/30/20 Hz × 720 passos;
- 15.360 passos de combate/IA: os 32 inimigos contra os dois heróis;
- 16.320 passos dos 16 chefes e Belial contra os dois heróis;
- posição, velocidade, vida, vidas, chão, ataque, retorno seguro, animação e estados relevantes, com erro máximo observado de 0,00024 e tolerância de 0,001 nos campos de ponto flutuante.

O combate usa um stream xorshift32 explicitamente injetado nos dois runners. Isso verifica regras sob as mesmas escolhas aleatórias; **não** demonstra equivalência ao gerador `UnityEngine.Random`. Os cenários isolam alvos ou removem inimigos/itens para comparar física. Os bots de travessia legados usam vidas adicionais; não demonstram balanceamento de dificuldade com três vidas.

```powershell
# Executar dentro de NightfallNative; requer .NET SDK 10 e Python 3.
dotnet build tests/reference/NightfallReference.csproj -c Release --configfile tests/reference/NuGet.Config -o Saved/ReferenceRunner
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
ctest --preset windows-msvc-release --output-on-failure
python tests/compare_reference.py
```

O relatório é gerado em `Saved/reference-parity-report.json`. A opção `NightfallNative.exe --render-smoke <diretorio>` executa o frontend gráfico real em janela oculta e exporta o framebuffer de cada mundo com os dois heróis após 180 passos. As capturas são uma cena iniciada por teste, com dados sintéticos; não simulam validação humana dos menus. A execução Windows foi concluída nos 16 mundos com 77 assets indexados. Áudio é silenciado somente nesse modo de captura.

O instalador real também foi executado silenciosamente em diretório isolado, sem instalação anterior: 80 arquivos coincidiram por SHA-256 com o ZIP portátil; o executável instalado passou headless e renderizou os 16 mundos. O processo de teste recebeu `LOCALAPPDATA` próprio. A desinstalação terminou com código zero e retirou o executável e a entrada de teste do Registro. `Scripts/Test-Windows-Candidate.ps1` reproduz o teste e recusa substituir instalação existente.

## Builds e assinatura

```powershell
./Scripts/Build-Windows-Installer.ps1 -Version 1.0.1
./Scripts/Build-Android.ps1 -Version 1.0.1 -Abi arm64-v8a -UnsignedOnly
./Scripts/Build-Android.ps1 -Version 1.0.1 -Abi x86_64 -QaDebug
```

Android compila raylib 5.5 oficial com NDK 27.0.12077973, API mínima 26 e alvo 36. O arquivo-fonte de raylib é travado pelo SHA-256 `00e8a7c85f7a5221e56ee8e4e1c6642b773eae73d29379a91c8ac5149e803f42`. `-RaylibSource` permite reaproveitar essa fonte local. ELF ARM64 foi conferido com segmentos LOAD de alinhamento `0x4000` e runtime C++ estático; `zipalign -P 16` trata o APK.

Windows também usa runtime C++ estático (`/MT`). A auditoria PE do executável passou a listar apenas `raylib.dll`, `WS2_32.dll`, `IPHLPAPI.DLL` e `KERNEL32.dll`; a raylib distribuída importa somente DLLs do Windows. O pacote não depende de uma instalação prévia do Visual Studio ou do Visual C++ Redistributable.

`-QaDebug` cria uma identidade de teste descartável em `Saved/qa-signing`, pacote separado `com.mikeanddenyse.nightfall.qa` e aplicativo depurável. Essa chave não atualiza o app publicado e nunca deve ser enviada como release. `-UnsignedOnly` não acessa chave ou senha e gera um APK que ainda não pode ser instalado.

Para release são necessárias `SLT_GAMES_KEYSTORE`, `SLT_GAMES_STORE_PASSWORD` e, se diferente de `sltgames`, `SLT_GAMES_KEY_ALIAS`. O script reutiliza a identidade existente, recebe senha pelo ambiente e não gera chave de produção. Binários, chaves e recibos privados ficam fora do Git.

## Gates que ainda precisam de evidência

- Execução Android em emulador: workflow preparado com APK x86_64 QA, SwiftShader, navegação real dos menus, dois heróis, inputs, pausa/Voltar e background/retorno; somente o resultado efetivo e as capturas podem aprovar essa etapa.
- Toque e áudio em ARM64 físico e cooperativo Wi-Fi em dois aparelhos.
- Revisão humana da apresentação, teste interativo do instalador, assinatura de release, CI hospedado, merge e publicação coordenada no portal.
- Backup privado final e restauração depois do snapshot publicado.
