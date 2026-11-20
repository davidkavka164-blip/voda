# HOME WATER CONTROLLER — vzdálený přístup přes internet

## Stav návrhu

**Požadavek schválen podle MASTER 1.3, technická realizace zatím neověřena.**

Cílem je bezpečně otevřít hlavní web HOME WATER CONTROLLER z telefonu nebo počítače mimo domov. Vzdálený přístup nesmí být podmínkou místního řízení, měření, ochran ani komunikace mezi ESP32.

## Zjištěný domácí router

Uživatel uvedl: **TP-Link Archer AX12, AX1500, Wi-Fi 6, 2,4/5 GHz, 1× WAN + 3× LAN**.

Oficiální česká stránka podpory TP-Link pro **Archer AX12(EU) V1** uvádí:

- firmware `1.1.28 Build 20250523` (vydán 9. 7. 2025) přidal podporu VPN Client, **WireGuard VPN Server** a kill switch;
- stránka podpory nyní uvádí pro EU V1 novější firmware `1.5.3 Build 20261008` (vydán 16. 10. 2026), jehož poznámky uvádějí zlepšení zabezpečení a stability;
- některé meziverze obsahují upozornění na nevratný upgrade / nemožnost návratu na předchozí firmware;
- TP-Link výslovně požaduje ověřit hardwarovou verzi a použít firmware pro správný region a variantu zařízení.

Zdroje:
- https://www.tp-link.com/cz/support/download/archer-ax12/v1/
- https://www.tp-link.com/cz/support/download/archer-ax12/

**Důležité omezení:** Zatím neznáme HW verzi, regionální variantu, aktuální firmware ani provozní režim routeru. Podpora WireGuard je proto potvrzena pouze pro oficiální seznam firmwaru EU V1, nikoli zatím pro konkrétní kus uživatele. Firmware se nesmí aktualizovat naslepo. Před aktualizací je nutné ověřit štítek, administraci, region a přesné pokyny k přechodu mezi verzemi; zálohovat konfiguraci.

## Zvolená architektura

1. WireGuard VPN Server běží na domácím routeru, pokud jej konkrétní HW/firmware podporuje.
2. Telefon nebo notebook mimo domov používá klienta WireGuard a samostatný klientský profil/klíč.
3. Po navázání VPN se klient dostane do domácí LAN a otevře hlavní web ESP32 #1 přes jeho místní IP adresu.
4. ESP32 #1 a ESP32 #2 pokračují v místní komunikaci přes smíšenou cestu: ESP32 #1 přijímá UDP přes domácí Wi-Fi STA a ESP32 #2 vysílá přes W5500 Ethernet; obě rozhraní musí být ve stejné LAN. VPN není součástí jejich bezpečnostní logiky.
5. Vzdálený přístup používá stejné webové funkce a serverové bezpečnostní kontroly jako přístup doma.
6. ESP32 se přímo nevystavují veřejnému internetu. Na routeru se nevytváří port-forwarding HTTP/HTTPS přímo na ESP32.
7. Při výpadku internetu nebo VPN zůstává místní web přes domácí Wi-Fi a dostupný AP funkční podle implementace zařízení.

## Předpoklady a možné překážky

- Peer komunikace vyžaduje, aby router/AP propouštěl lokální UDP broadcast z Ethernetu do Wi-Fi. ESP32 #1 nesmí být připojena ke guest Wi-Fi ani k izolované VLAN; client/AP isolation musí být vypnutá.

- Archer AX12 musí být v režimu Router a musí být schopen přijímat příchozí VPN spojení z WAN. Pokud je pouze v režimu Access Point, VPN server na něm nemusí být dostupný.
- Pokud je před AX12 ještě modem/router poskytovatele, může vzniknout dvojitý NAT. Bude nutné ověřit, zda lze VPN provoz předat na AX12, případně zda lze nadřazené zařízení přepnout do bridge režimu.
- Pokud poskytovatel používá CGNAT, běžné příchozí IPv4 VPN spojení nemusí být možné. Je třeba ověřit veřejnou IPv4, případně dostupnost a použitelnost veřejné IPv6 nebo alternativního VPN řešení.
- Při dynamické veřejné IP bude vhodné použít podporovanou službu DDNS; konkrétní volba závisí na routeru a poskytovateli.
- ESP32 #1 musí mít stabilní adresu v LAN, ideálně DHCP rezervaci v routeru. IP adresu ani podsíť zatím neurčujeme.
- Je nutné ověřit, že VPN klient má povolený přístup do domácí LAN a že se jeho VPN podsíť nepřekrývá s domácí LAN.

## Bezpečnostní požadavky

- Preferovat WireGuard; nepoužívat PPTP.
- Každé zařízení (telefon/notebook) má vlastní VPN klientský profil/klíč. Při ztrátě zařízení se jeho profil zneplatní bez změny ostatních klientů.
- VPN přístup chránit silným odemykáním telefonu/počítače a aktualizovaným systémem.
- Nepovolovat vzdálenou správu administrace routeru z veřejného WAN, pokud to není nezbytné.
- Nepřidávat přímé veřejné porty na web ESP32.
- Zvážit samostatný účet/heslo webu HWC; VPN sama nenahrazuje přihlášení do aplikace.
- Poznámka k aktuálnímu zdrojovému kódu: firmware v současnosti vypisuje některá vygenerovaná AP a webová hesla do Serial Monitoru. Před ostrým vzdáleným provozem je nutné vyřešit bezpečné prvotní předání a změnu přihlašovacích údajů a odstranit nechtěné zveřejňování tajemství v diagnostických výpisech. Prosté NVS/Preferences zatím není považováno za bezpečné úložiště dlouhodobého peer tajemství.

## Co zatím není hotové

- Není ověřena HW revize ani firmware konkrétního Archer AX12.
- Není známo, zda má přípojka veřejnou IPv4/IPv6 nebo CGNAT.
- Není ověřena topologie WAN (AX12 přímo na ONT/modemu, nebo za dalším routerem).
- Není nakonfigurován WireGuard server ani klient.
- Není ověřena DHCP rezervace a dosažitelnost webu ESP32 #1 přes VPN.
- Vzdálený přístup k samostatnému AP ESP32 #2 není součástí tohoto návrhu; hlavní vzdálené rozhraní má být ESP32 #1. Lokální servisní AP #2 zůstává místní cestou.

## Postup až bude router dostupný

1. Zapsat HW verzi ze štítku, regionální variantu a aktuální verzi firmwaru z administrace.
2. Zjistit, zda AX12 pracuje v režimu Router nebo Access Point a zda je před ním další router/modem.
3. Ověřit veřejnou IP / CGNAT u poskytovatele (bez zveřejňování IP adresy v dokumentaci projektu).
4. Podle přesné HW verze ověřit kompatibilní firmware a jeho upgrade cestu; před případnou aktualizací uložit konfiguraci.
5. Zapnout WireGuard VPN Server, vytvořit jeden testovací profil a povolit přístup pouze do potřebné domácí LAN.
6. Zajistit DHCP rezervaci pro ESP32 #1.
7. Otestovat připojení nejprve z mobilu přes mobilní data, nikoli přes domácí Wi-Fi.
8. Ověřit přístup k hlavnímu webu, přihlášení, stavové údaje a bezpečné chování po odpojení VPN/internetu.

## Akceptační kritéria

- Z mobilních dat se lze přes VPN bezpečně připojit k hlavnímu webu ESP32 #1.
- ESP32 není přímo dostupné z veřejného internetu bez VPN.
- Po odpojení VPN nebo internetu místní měření, ochrany, komunikace a místní web pokračují.
- Ztráta VPN nikdy nespustí, neobnoví ani nezmění transfer.
- Ztracený VPN klientský profil lze zrušit bez přeprogramování ESP32.
