# Bezpečnostní audit — HTTP autentizace, revize 0.97

## Rozsah

Kontrolována byla autentizační brána `requireWebAuth()` v ESP32 #1 a `requireLocalAuth()` v ESP32 #2. Obě používají `WebServer::authenticate("admin", password)` a HTTP Basic Auth.

## Nález

Před revizí 0.97 nebylo v aplikační vrstvě omezení opakovaných neúspěšných přihlášení. Útočník v dosahu lokální sítě mohl opakovat pokusy bez aplikační prodlevy.

## Opatření 0.97

`WebAuthRateLimiter` je pevný, bezalokacní a wrap-safe modul:

- identifikace klienta: IPv4 adresa;
- 5 neúspěšných pokusů během 60 000 ms;
- po dosažení limitu blokace na 60 000 ms;
- úspěšné přihlášení odstraní záznam dané IP;
- osm záznamů v pevné tabulce;
- aktivní blokace se nevytlačují; při zaplnění všech slotů aktivními blokacemi jsou nové neznámé IP dočasně odmítnuty;
- během blokace handler vrací HTTP 429 a nepředává požadavek do `authenticate()`.

## Bezpečnostní hranice

Toto opatření pouze zpomaluje opakované pokusy. Nezajišťuje šifrování přenosu a nezabrání útoku z mnoha různých IP. HTTP Basic Auth přes prosté HTTP stále odhaluje znovupoužitelné přihlašovací údaje každému, kdo může provoz odposlouchávat. Web proto nesmí být vystaven přímo internetu; vzdálený přístup musí být přes VPN. TLS na ESP32 zůstává samostatným otevřeným úkolem.

## Testy

`test_web_auth_rate_limiter.cpp` pokrývá prahovou hodnotu, přesnou expiraci, per-IP izolaci, úspěšné přihlášení, reset okna, přetečení `uint32_t` a saturaci pevné tabulky. Statická kontrola potvrzuje integraci do obou `.ino`.

Standardní a ASan/UBSan testy: 57/57. Arduino build a fyzické síťové testy nebyly provedeny.
