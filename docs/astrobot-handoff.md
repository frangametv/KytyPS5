> Aggiornamento 9 settembre 2026: il successivo merge upstream fino a
> `0b4e78c` risolve le verifiche depth/stencil e porta la suite CTest a
> 40/40 test passati. Vedere [il resoconto](upstream-integration-20260909.md).
> I blocchi di avvio di Astro Bot sotto riportati non sono stati riverificati.

# Astro Bot: handoff dopo il merge RTX di Brandon

Aggiornato: 9 settembre 2026.

## Obiettivo e stato
Rimosso integralmente il bypass ray tracing introdotto nella sessione precedente:
codice shader, configurazione, CLI, UI, traduzioni e test del bypass.
Importato il branch https://github.com/brandostrong/KytyPS5/tree/astrobot/rt,
tip 25da84b1aee41b432445d2c247129f651ae795c3.
Merge locale completato su main: 589ae76. Nessun push eseguito.

Il supporto BVH esegue intersezioni reali tramite SPIR-V sulla GPU; non è un
semplice risultato miss e non implica l'uso delle unità RTX native NVIDIA.
Astro Bot NON ha ancora raggiunto la scena 3D.

## Adattamenti Fran
- Conservati launcher QML, identità Fran e prefetch BDA; adattato renderCompute
  al nuovo runtime delle risorse di Brandon.
- Portata la selezione GPU nella UI Fran e nella CLI --gpu, con persistenza e test.
- Corretto un underflow nel controllo degli intervalli leggibili di SrtWalker,
  con regressione dedicata.
- Conservato il clamp dei mip delle storage view fisse quando non è possibile
  estendere il mip tail; le view dinamiche restano soggette ai limiti validi.
- Aggiornati due controlli test: registri ES ignorati da Brandon e bit DCC
  già consumati nelle slice 0/7 della texture 3D.
- Modifiche locali preesistenti a Library.qml, SettingsPage.qml, About nelle
  traduzioni e package.json del submodule SPIRV-Tools preservate fuori dal merge.

## Verifiche
Build riuscita: kyty_library, shader_recompiler_compute_tests, shader_cfg_tests,
library_ui_tests.
Passati: BVH decode, tutti i 12 casi GPU BVH, shader_cfg_tests (inclusa nuova
regressione SRT), test UI selezionati (7 risultati inclusi init/cleanup),
test GPU --storage-mip-only.
La suite compute completa NON è verde: si arresta in
RenderExecutorStencilBindingDiscovery, fase
"deferred clear with sampled read-only depth bounds".
Anche --storage-mip-host-only richiama questo test e fallisce nello stesso punto.
Il test texture 3D RenderExecutorColorVolumeDiscovery ora passa.
Non è stato stabilito se il problema depth/stencil esistesse prima del merge.

Log verifiche: _Build/brandon-rt-*.log.
Helper build locale: _Build/build-bvh.cmd (inizializza Visual Studio/LLVM/Ninja).
Per i test Qt usare PATH con C:/ProgramData/Qt/6.10.3/msvc2022_64/bin,
QT_QPA_PLATFORM=offscreen,
QT_QPA_PLATFORM_PLUGIN_PATH=C:/ProgramData/Qt/6.10.3/msvc2022_64/plugins/platforms,
QT_QPA_FONTDIR=C:/Windows/Fonts e QT_QUICK_BACKEND=software.
Questo evita il precedente mancato avvio per il plugin Qt.

## Cosa manca per Astro Bot
La prova senza bypass termina su:
MaterializeResources(resource_plan, runtime, resources, specialization)
in src/graphics/host_gpu/renderer/pipeline/pipelineCache.cpp:360,
shader compute hash 0x78af8e269b528b5c (TileBasedLighting).
Decode, CFG e IR arrivano oltre la precedente istruzione BVH non supportata.
Log preservato:
W:/KytyLab/KytyPS5/Fran/Build-Fran/astrobot-brandon-rt-log.txt
e relativo astrobot-brandon-rt-stderr.txt.

Prossimi passi:
1. Aggiungere diagnostica puntuale ai ritorni false di MaterializeResources e
   WalkSrt, correlando source, offset, user data e flattened SRT.
   La vecchia analisi sul bypass citava source 22/dword 0: è solo un indizio
   storico, NON una diagnosi riconfermata dopo questo merge.
2. Correggere la risoluzione del descrittore reale; non reintrodurre miss fittizi
   o descrittori nulli indiscriminati per nascondere l'errore.
3. Risolvere il test depth/stencil e ripetere la suite completa.
4. Rilanciare Astro Bot con shader validation, confermare emissione SPIR-V,
   pipeline e un frame 3D visibile. L'integrazione RTX da sola non lo garantisce.

## Percorsi e recupero
Repository: W:/KytyLab/KytyPS5/Fran/KytyPS5-Fran
Installazione: W:/KytyLab/KytyPS5/Fran/Build-Fran
Gioco: W:/KytyLab/Games/AstroBot
Backup completo pre-merge: _Build/pre-brandon-rt-20260909/
(inclusi working-tree.patch, spirv-tools.patch e precedente handoff).
Stash di sicurezza conservato: "Preserve Fran local UI edits and handoff before
Brandon RT merge"; è già stato applicato, NON riapplicarlo.
Non fare reset/clean globale: ci sono modifiche locali dell'utente da preservare.
