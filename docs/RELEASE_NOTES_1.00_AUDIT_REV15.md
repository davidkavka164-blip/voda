# Release notes — auditní REV15

- Přidán `MANUAL_VALVE_CONTROL_AUDIT_REV15.md` podle HOME_WATER_CONTROLLER_MASTER_1.3.
- Ověřeno, že nouzový `/pump-emergency` na ESP32 #1 a #2 ovládá pouze logiku ochrany příslušného čerpadla; nejde o ruční ovládání ventilů.
- Potvrzeno, že transferové UI a `TransferLogic` v REV14 zůstávají simulační; fyzické výstupy ventilů nejsou zapojeny.
- Popsán oddělený návrh stavového automatu ručních ventilů, zachování dostupných ochran, webového rozhraní a bezpečnostních scénářů.
- Vypsány chybějící hardwarové a provozní informace, které je nutné ověřit před implementací.
- Žádný firmware, test, bezpečnostní limit ani GPIO konfigurace nebyly změněny.
- Žádné fyzické výstupy nebyly aktivovány. REV15 je auditní dokumentační revize, nikoli firmware připravený pro řízení ventilů.
