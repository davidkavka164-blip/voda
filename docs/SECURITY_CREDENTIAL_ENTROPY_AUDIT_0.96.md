# HOME WATER CONTROLLER — audit entropy při generování prvních hesel

Revize: 0.96  
Datum: 4. 11. 2026  
Rozsah: generování výchozích AP/admin hesel v `ESP32_1.ino` a `ESP32_2.ino`.

## 1. Nález v revizi 0.95

Obě jednotky vytvářely výchozí hesla voláním `esp_random()` ještě před tím, než jejich sketch inicializoval Wi-Fi driver. Původní šablony poskytovaly:

- AP heslo: 32 bitů z jednoho slova RNG.
- Admin heslo: 48 bitů ze 32bitového slova a dalších 16 bitů.

To je méně než žádoucí pro první produkční přihlašovací údaje. Závažnější je pořadí inicializace: dokumentace ESP-IDF 3.3.x uvádí, že výstup RNG lze považovat za skutečně náhodný při aktivním Wi-Fi/Bluetooth; bez těchto RF subsystémů je RNG pseudonáhodný. V době původního generování nebyl Wi-Fi driver sketchů aktivní.

## 2. Oprava 0.96

Přidán společný `CredentialRandom.h`:

- Formátuje celá 32bitová slova jako velká hexadecimální čísla.
- Čtyři slova vytvoří 32 hex znaků, tedy 128 bitů vstupu z random source.
- Používá pevný výstupní buffer, kontrolu kapacity a při chybě vyčistí celý výstupní buffer.
- Nevolá sám hardware RNG a netvrdí, že zdroj je bezpečný; odpovědnost za inicializaci entropy source zůstává v board call-site.

V obou sketchích je přidána funkce `enableCredentialRandomSource()`:

1. Nastaví `WiFi.persistent(false)`, aby se Wi-Fi konfigurace ukládala do RAM režimu Arduino Wi-Fi driveru.
2. Zavolá `WiFi.mode(WIFI_STA)` a vyžaduje úspěšný návrat.
3. Teprve po úspěchu volá `esp_random()` prostřednictvím `credentialRandomWordSource`.
4. Pokud inicializace Wi-Fi selže, nová přístupová hesla se nevytvoří a AP/web zůstane fail-closed.

Pro Arduino-ESP32 1.0.6 je `WiFiGenericClass::mode(wifi_mode_t)` deklarováno jako `bool`; při přechodu z režimu NULL inicializuje Wi-Fi driver a volá `esp_wifi_start()`. Předpoklad je ověřen proti upstream zdroji tagu 1.0.6. Reálný překlad aktuálního sketchu ale proveden nebyl.

## 3. Nové parametry výchozích tajemství

- AP heslo: 32 velkých hex znaků (128 bitů).
- Admin heslo: 32 velkých hex znaků (128 bitů).
- SSID zůstává odvozené z MAC a není považováno za tajemství.
- Existující legacy hesla se automaticky nerotují. Jejich změna bez hotového fyzického provisioningu by mohla uživatele odříznout od zařízení.
- Jednorázový Serial handoff nově vygenerovaných hesel po úspěšném commitu zůstává dočasný a stále je bezpečnostním omezením.

## 4. Testování

- `test_credential_random.cpp`: deterministický zdroj, přesný 128bitový výstup, délka, nedostatečný buffer, null source a nulování výstupu při odmítnutí.
- `check_credential_entropy_integration.sh`: kontroluje obě `.ino`, inicializaci Wi-Fi před generováním, čtyři random slova pro nové AP/admin tajemství a zákaz návratu starých šablon.
- CredentialRandom.h je bajtově shodný v `firmware/common/` a obou sketch `src/`.
- Standardní a ASan/UBSan testy: 56/56 native test programs.
- Python provision client: 17/17.

## 5. Omezení

- Hostitelský test ověřuje hex formátování a integrační pořadí volání, nikoli kvalitu fyzického RNG na konkrétním čipu.
- Wi-Fi driver start se musí ověřit skutečným Arduino-ESP32 buildem a hardwarem.
- Staré již existující heslo v legacy NVS se automaticky nemění; 0.96 zvyšuje sílu pouze nově generovaných údajů.
- `hwcCred` ani legacy namespace nejsou šifrovány. NVS Encryption/Flash Encryption zůstává otevřená platformní práce.
- Jednorázový výpis nově generovaných hesel přes Serial zůstává dočasný; fyzicky autorizovaný provisioning není připojen do `.ino`.
- Arduino build, fyzická NVS a test skutečných hesel/AP nebyly provedeny.
