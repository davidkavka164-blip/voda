# REV39 – oddělení elektrického nastavení snímače a objemové kalibrace

- Zpřesněno názvosloví typů, stavů, proměnných a diagnostiky.
- Výslovně zdokumentováno, že `SensorConfig` převádí 4–20 mA na výšku hladiny, zatímco `WellCalibration` převádí výšku na litry.
- Objemová kalibrace studny zůstává mimo pumpové ochrany a transfer logiku.
- Bezpečnostní chování REV38 nebylo v této revizi změněno.
- Přidán audit `LEVEL_VS_VOLUME_CALIBRATION_AUDIT_REV39.md`.
