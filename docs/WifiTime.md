# Automatický čas přes Wi-Fi a NTP

## Nastavení

1. Otevři `esp32/include/WifiConfig.h` a nastav `SSID` a `PASSWORD` pro svoji
   2,4GHz Wi-Fi. Soubor je ignorovaný Gitem. Heslo neposílej do logů ani do commitu.
2. V novém checkoutu vytvoř tento soubor z `WifiConfig.example.h`, pokud ještě
   neexistuje. Bez konfigurace firmware sestavíš, ale vypíše chybějící konfiguraci.
3. Výchozí timeout Wi-Fi je 15 s, timeout NTP dalších 15 s. Po neúspěchu se další
   pokus spustí za 5 minut. Hodnoty a dva NTP servery lze upravit v konfiguraci.
4. Sestav a nahraj projekt `esp32/`, potom otevři monitor na 115200 baud.

Z terminálu ve složce `esp32`:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32c3 -t upload
& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" device monitor -b 115200
```

Před nahráváním zavři Serial Monitor. Stejná synchronizace funguje i v prostředí
`esp32c3-production`, které má vypnutou debug konzoli.

## Průběh po zapnutí

- Robot nejdříve provede dosavadní hardwarovou diagnostiku. V ERROR nezačne síťový pokus.
- `TimeManager` zahájí připojení vlastním Wi-Fi rádiem ESP32. Počítač mu nepředává
  přihlašovací údaje ani svůj čas. USB je potřeba pouze pro nahrání a případný monitor.
- Chování, animace, vstupy i konzole běží dál. Není použita čekací smyčka na síť,
  `delay()` ani blokující `getLocalTime()`.
- Po připojení se spustí SNTP. Za úspěch se považuje až nové oznámení o synchronizaci
  a platná hodnota času, nikoli samotné připojení nebo starý čas z minulého běhu.
- Následně se zastaví SNTP, odpojí Wi-Fi a přepne režim na WIFI_OFF. Stav rádia se
  kontroluje; případné selhání vypnutí se vypíše a vypnutí se znovu zkusí.
- Systémové hodiny pak běží bez Wi-Fi, internetu i počítače. Při každém restartu
  se znovu požaduje čerstvá synchronizace. Po úspěchu se další síťové pokusy v témže
  běhu neplánují.

Používá se přesné pásmo `CET-1CEST,M3.5.0/2,M10.5.0/3`. Základem je UTC systémový
čas; místní čas se průběžně převádí podle českých pravidel, takže přechod na letní
nebo zimní čas nevyžaduje nové internetové připojení. Systémové hodiny mohou časem
vykazovat odchylku; mezi restarty zde není implementovaná periodická resynchronizace.

## Výstup a chyby

Úspěšná cesta:

```text
[TIME] Connecting to configured Wi-Fi (credentials hidden)
[TIME] Wi-Fi connected; waiting for verified NTP response
[TIME] NTP synchronization VERIFIED; system RTC continues offline
[TIME] Wi-Fi radio OFF; SNTP stopped
```

Příkaz `status` doplňuje například `source=NTP_SYNCED wifi=OFF` a aktuální platný
místní čas. Mezi stavy patří také WIFI_CONNECTING, NTP_WAITING, RETRY_WAIT a MANUAL.
`[TIME_ERROR]` obsahuje důvod posledního selhání. Chyba sítě není kritická chyba
OLED: robot pokračuje, pouze nemá aktivní denní rozvrh, pokud nemá jiný platný čas.

Log rozlišuje chybějící konfiguraci, nenalezený přístupový bod, odmítnuté spojení,
timeout Wi-Fi, ztrátu spojení během NTP, timeout NTP a neplatný přijatý čas.
Po neúspěchu se rádio vypne a pokus se opakuje po nakonfigurovaném intervalu.

Při opakovaných problémech AUTH_EXPIRED na více sítích se nyní zkouší diagnostický
profil: vypnutý modem sleep a TX limit 8,5 dBm. Log `Radio profile` ukazuje skutečně
přečtený výkon a úspěch jeho nastavení. Jde o pokus o odlišení problémů rádiové
části/napájení, nikoli potvrzení příčiny. Nižší výkon může zmenšit dosah.
Pro návrat na vyšší limit lze v build_flags definovat například
`-D ROBOT_WIFI_TX_POWER_QDBM=78` (19,5 dBm); jednotka je čtvrt dBm.
Nastavení úsporného režimu a výkonu podporuje
[oficiální API Espressif](https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32c3/api-reference/network/esp_wifi.html).

## Ruční čas a simulace

- Číselný příkaz `TIME <local epoch seconds>` nastaví ruční místní hodiny.
  Zruší probíhající SNTP, vypne Wi-Fi a zakáže automatické pokusy do restartu.
  Pozdější callback ani platná stará systémová hodnota ruční čas nepřepíší.
- `time HH:MM` stále používá oddělený simulovaný čas. Automatický zdroj může běžet
  na pozadí, ale nemění čas, podle kterého právě běží simulovaný scénář.
- `time auto` se vrátí k reálnému zdroji: NTP nebo ručně nastaveným hodinám.
  Ruční zámek neruší. Bez platného zdroje zůstává AUTO UNSET a obnoví se IDLE.
- Ruční číselný příkaz má původní význam místního civilního času, nikoli UTC.
  Automatické změny letního/zimního času se vztahují na NTP zdroj; ruční hodiny
  slouží jako testovací záloha.

## Soubory a testování

- `esp32/src/time/TimeManager.h/.cpp`: neblokující připojení, SNTP, timeouty,
  rádio OFF, ruční zámek a převod do `LocalTime`.
- `esp32/src/Robot.h/.cpp`: inicializace po diagnostice, pravidelný update a předání
  místního času existujícímu `BehaviourProgram`. V animacích se nic nemění.
- `esp32/include/WifiConfig.example.h`: veřejná ukázka; `WifiConfig.h` je soukromý.
- `esp32/host/TimeManagerTests.h`: řízený síťový adaptér a testy času bez sítě.

Automatické testy:

```powershell
./esp32/host/run.ps1
```

Kontrolují nový NTP callback oproti staré hodnotě RTC, platnost času, rádio OFF,
pokračování hodin offline, timeouty, opakování, ztrátu připojení, špatnou konfiguraci,
restart a ruční nastavení během připojování i během NTP. Převod se testuje v zimě,
v létě i přesně na změnách DST v letech 2026 a 2028. Dosavadní testy konzole a
chování dál ověřují hranice 07:00, 21:00 a 21:30.

Pro fyzické ověření vyplň konfiguraci, nahraj firmware a zkontroluj VERIFIED,
`valid=yes`, správný místní čas a `wifi=OFF`. Potom odpoj počítač a ověř samostatný
provoz. Reset musí znovu spustit pokus. Chybnou Wi-Fi nebo nedostupný NTP testuj
změnou své lokální konfigurace a opětovným nahráním; sleduj konkrétní timeout,
vypnutí rádia, další pokus a průběžnou odezvu na dotyk.

Testovací síťový adaptér nepředstavuje skutečné připojení ESP32. Výsledky fyzických
testů musí být uváděny odděleně od výsledků automatických testů.
