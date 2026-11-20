# ESP32 #1 — hlavní jednotka

REV85: Wi-Fi STA + konfigurační AP zůstávají zachovány; W5500 přidává samostatné DHCP Ethernet rozhraní. Dashboard zobrazuje jeho stav a IP. Produkční HTTP ovládání zůstává v REV85 pouze na Wi-Fi/AP; Ethernetové HTTP routy nejsou implementovány.

ESP32 #1 ovládá ruční i automatické cesty ventilů přes dvě aktivně-vysoká relé: GPIO16 = studna → dolní nádrž, GPIO17 = horní → dolní nádrž. Reléová deska je napájena 5 V; kompatibilitu vstupu s logikou 3,3 V ESP32 je nutné ověřit. Výstupy se při startu nastaví LOW; STOP a ztráta bezpečnostní podmínky vydají CLOSE_ALL. Automatický transfer nespíná čerpadla; ta řídí vlastní tlaková automatika. Bez zpětné vazby není potvrzena fyzická poloha ventilu ani průtok.

Historická poznámka k revizi 0.23 (již překonáno REV18): automatická transferová obrazovka tehdy pouze simulovala rozhodovací logiku a fyzické ventily neovládala.

REV16: přidána samostatná logická/webová vrstva nouzového ručního ovládání ventilů. Fyzické GPIO a relé zůstávají vypnuté; viz `docs/MANUAL_VALVE_CONTROL_IMPLEMENTATION_REV16.md`.

REV17: přidán fyzický výstup dvou relé pro nouzové ruční ovládání. REV18 připojuje stejný driver také k automatickému transferu. Viz `docs/RELEASE_NOTES_1.00_AUDIT_REV18.md`; před připojením ventilových cívek je nutné ověřit reléový modul, COM/NO zapojení, 3,3V spouštění, 5V napájení desky a externí pull-down rezistory.


REV22: Nouzový režim při platném měření studny respektuje `pumpProtectionState.pumpPermit`; při neplatném měření rozhoduje obsluha po fyzické kontrole. Logické vyhodnocení ochrany horního čerpadla běží na ESP32 #2 nezávisle. Fyzické výstupy pump safety relé nejsou v REV22 aktivovány.

REV25: Doplněna samostatná ochrana čerpadla dolní nádrže na GPIO32 s webově nastavitelným minimem a hysterezí, NVS dvouslotovým záznamem a fail-closed startem. Pracovní minimum 11 cm se nemění a není automaticky ochranným prahem.

REV28: Fyzické výstupy bezpečnostních relé čerpadel jsou povoleny. GPIO25 = studna, GPIO32 = dolní nádrž. Obě relé používají ověřené zapojení Drátek 24 V / 30 A HIGH–LOW ze samostatného projektu studny: 24 V DC, jumper HIGH, GPIO HIGH povoluje motor a LOW jej odpojuje. Každé čerpadlo používá výhradně vlastní lokální ochranný stav a čerstvost vlastního měření; podrobnosti viz `docs/RELEASE_NOTES_1.00_AUDIT_REV28.md`.

REV29: Audit potvrdil nezávislost studniční a dolní ochrany a jejich oddělení od lokální horní ochrany na ESP32 #2. Nouzové otevření ventilu samo o sobě nepovoluje čerpadlo. Viz `docs/RELEASE_NOTES_1.00_AUDIT_REV29.md`.


REV30: Doplněn samostatný nouzový režim čerpadla dolní nádrže při poruše vlastního čidla. Aktivace vyžaduje fyzické potvrzení hladiny; kritický latch, neplatná konfigurace a chyba NVS zůstávají blokující. Nouzový režim se po restartu neobnovuje. Všechna tři čerpadla nyní mají stejný princip výslovného nouzového povolení.

REV91: Detailní návrh bezpečného společného HTTP transportu pro Wi-Fi a W5500 je v `docs/HTTP_TRANSPORT_ADAPTER_DESIGN_REV91.md`. Produkční HTTP zůstává zatím Wi-Fi/AP; žádná routa ani ochranná logika nebyla změněna.
