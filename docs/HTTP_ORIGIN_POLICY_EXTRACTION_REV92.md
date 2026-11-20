# Extrakce politiky Origin/Host — REV92

## Provedená změna

Původní kontrola stejného původu byla dvakrát implementována přímo v hlavních `.ino` souborech. REV92 ji přesouvá do společné, na Arduino ani síťové knihovny nezávislé funkce `HWC::isSameHttpOriginAuthority()`.

Funkce je uložena v `firmware/common/HttpOriginPolicy.h` a synchronizována do `src` obou jednotek. Oba dosavadní autentizační vstupy ji volají před Basic Auth a před jakýmkoli POST handlerem.

## Pravidla

- Vyžaduje neprázdné `Origin` i `Host`.
- Přijímá pouze `http://`, protože současný webserver nepoužívá TLS.
- Porovnává celou autoritu včetně portu bez rozlišení velikosti písmen.
- Odmítá cesty, query, fragment, userinfo, bílé znaky a autority delší než 127 znaků.
- Chybějící nebo nejednoznačný vstup znamená odmítnutí POST.

## Rozsah a omezení

Toto je první skutečný refaktoring sdílené bezpečnostní politiky, nikoli ještě Ethernetový HTTP adaptér. Stávající `WebServer`, Basic Auth, rate limiter, callbacky i OTA zůstávají zachovány. Žádná cesta ani výstup čerpadla/ventilu se nemění.

Hostitelský test pokrývá pozitivní i negativní případy. Arduino target build a fyzický test Wi-Fi/W5500 jsou stále otevřené brány.
