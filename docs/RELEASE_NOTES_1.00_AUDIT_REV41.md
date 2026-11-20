# HOME WATER CONTROLLER – REV41 (auditní revize)

- Proveden audit obnovy elektrického nastavení tří snímačů `SensorConfig`.
- Zjištěno: ESP32 #1 umožňuje přes autentizovaný POST uložit chybějící nastavení studny/dolní nádrže, ale odmítá automaticky přepsat existující poškozený záznam. Formulář zatím nevyžaduje samostatné potvrzení fyzické referenční kontroly výšky.
- Zjištěno: ESP32 #2 nemá místní webový formulář ani POST handler pro obnovu `SensorConfig` horní nádrže.
- Nedostupné NVS nelze bezpečně opravit běžným webovým zápisem; musí zůstat blokováno.
- Firmware a ochranná logika se v REV41 nemění. Nebyla přidána možnost bezpodmínečně přepsat poškozenou konfiguraci ani použít výchozí RAM hodnoty pro běžnou ochranu.
- Popsány požadavky pro budoucí servisní obnovu: autentizace, same-origin POST, fyzická referenční kontrola, oddělená oprava pouze chybějícího/neplatného záznamu, validace a read-back před povolením měření.
- Testy REV40 zopakovány: 65/65 nativních programů PASS, 17/17 Python testů PASS, compileall PASS, bash syntax PASS.
- Arduino build, skutečné NVS a fyzické testy zůstávají neprovedené.

Podrobnosti: `SENSOR_CONFIG_RECOVERY_AUDIT_REV41.md`.
