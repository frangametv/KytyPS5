# Integrazione upstream del 9 settembre 2026

Integrati i 9 commit fino a `upstream/main` **0b4e78c**, partendo da
`d16b823`. Il launcher QML Fran non ha cambiamenti in questo intervallo;
UI, configurazione, selezione GPU e integrazione Brandon RT sono preservate.

## Novita integrate

- Copie depth/stencil tramite DB_RENDER_OVERRIDE e allineamento del pitch
  dei render target lineari.
- Riserve della buffer cache orientate verso la crescita delle richieste.
- Conversioni CP932 nella libreria Ces separata da libNet.
- Diagnostica dei fault guest con registri, thread, codice e stack guest.
- Correzioni VCCZ/EXECZ, SAVEEXEC B32 e atomiche LDS INC/DEC con ritorno.
- Risoluzione unificata dei mount root e gestione dei prefissi APR vuoti.

## Risoluzione delle sovrapposizioni

- SAVEEXEC usa la semantica scalare upstream per conservare i bit inattivi
  e EXEC_HI, mantenendo anche XOR, NAND, NOR, XNOR e le altre istruzioni
  aggiuntive del fork. Tre regressioni GPU coprono wave32, wave64 e wave parziali.
- Eliminato il vecchio fallback del filesystem: richiamava un metodo rimosso
  ed e sostituito dal resolver upstream, coperto dai test dei mount root.
- Corretto il doppio download nella buffer cache, preesistente nel fork:
  ora viene selezionato un solo percorso, con attesa completa oppure lettura
  dei lavori GPU gia terminati. Il tracking viene aggiornato per tutta la
  finestra effettivamente scaricata, senza inviare letture vuote.
- Su Windows PEEK conserva i dati senza inoltrare anche WAITALL a Winsock.
  Winsock rifiuta la combinazione; POSIX consente letture parziali con PEEK.
  Il test socket locale verifica peek, conservazione e successivo consumo.
  Riferimenti: [Winsock recv](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-recv),
  [POSIX recv](https://pubs.opengroup.org/onlinepubs/007904975/functions/recv.html).
- Aggiornata la regressione HTile alla correzione Brandon `e07cf62`:
  le passate in sola lettura conservano profondita e clear pendente;
  abilitare le scritture applica DB_DEPTH_CLEAR una sola volta.
  Verificati anche i valori effettivamente riletti dalla GPU.

## Verifica

Build Release Windows con clang-cl e Qt 6.10.3 riuscita per emulatore,
launcher e tutti gli eseguibili della suite CTest.
**40/40 test CTest passati**, inclusi UI Fran, filesystem/socket, compute
completo, BVH, shader, memoria e cache GPU. Durata: 35,55 secondi.
Il precedente arresto di RenderExecutorStencilBindingDiscovery e risolto;
la suite ora arriva alla fine senza errori.

Log locali: `_Build/upstream-build-fixes.log`, `_Build/upstream-tests-final.log`.
I test non dimostrano che tutti i giochi funzionino: non e stata eseguita
una nuova sessione di Astro Bot o GTA V. I blocchi di avvio descritti in
`astrobot-handoff.md` non sono dichiarati risolti da questa integrazione.

## Recupero

Branch precedente: `backup/main-before-upstream-20260909` (`d16b823`).
Eseguibili precedenti: `_Build/pre-upstream-20260909/`.
Installazione aggiornata: `W:/KytyLab/KytyPS5/Fran/Build-Fran`.
