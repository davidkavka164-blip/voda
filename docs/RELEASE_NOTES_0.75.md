# Release notes — DEV KIT 0.75

## P0-3: inicializační marker persistentního anti-replay journalu
- NVS replay store nově zapisuje marker `init` až po zápisu a zpětném ověření prvního platného checkpointu.
- Pokud marker existuje a oba checkpoint sloty chybí, `load()` vrátí `PEER_REPLAY_STORE_CORRUPT`, nikoli `EMPTY`; nový handshake floor se neposkytne.
- Neznámá hodnota markeru je rovněž považována za korupci.
- Pokud dojde k přerušení napájení po zápisu prvního checkpointu, ale před markerem, platný checkpoint zůstává čitelný a další úspěšný zápis doplní marker.
- Nativní replay test pokrývá ztrátu obou slotů po inicializaci.

## Omezení a neověřené body
Marker ve stejné běžné NVS partition nechrání proti smazání celé partition, factory resetu ani obnovení starší kopie celé flash. Nejde o hardwarově vynucený monotónní čítač. Pro tento threat model je nutná samostatná hardwarová ochrana. Skutečný ESP32 power-cut test a Arduino build nebyly provedeny. Peer command path a fyzické výstupy zůstávají zakázané.

## Testy
- Standardní nativní testy: 45/45.
- ASan/UBSan: 45/45.
- Boot identity a W5500 recovery statické kontroly: PASS.
