 Szyfrowany komunikator mesh działający **bez Internetu i GSM**, oparty na radiowej transmisji LoRa 868 MHz.
---

## Czym jest O.G.N.R?

O.G.N.R to system komunikacji zaprojektowany z myślą o sytuacjach, gdy tradycyjna infrastruktura (Internet, GSM) jest niedostępna. 
Każdy węzeł sieci to Raspberry Pi Pico 2 W z modułem LoRa SX1262, który jednocześnie:

- rozgłasza sieć **WiFi** dla klientów w pobliżu,
- pełni rolę **bramki TCP**, tłumaczącej wiadomości z WiFi na transmisję radiową,
- szyfruje **każdy pakiet radiowy** algorytmem AES-128-CTR.

Dwa węzły mogą komunikować się na odległość kilku kilometrów w terenie otwartym.

---

## Funkcje

- **Wiadomości prywatne** — format `NICK:treść`, routing lokalny (bez LoRa) lub przez radio
- **Broadcast** — format `ALL:treść`, dociera do wszystkich użytkowników w sieci
- **Lista użytkowników** — komenda `INFO` zwraca użytkowników lokalnych i zdalnych (przez LoRa)
- **Auto-rejestracja** — pierwsze połączenie TCP = rejestracja nicku, brak kont ani haseł
- **Szyfrowanie end-to-end** — AES-128-CTR, klucz w pamięci FLASH mikrokontrolera
- **DHCP** — węzeł automatycznie rozdaje adresy IP klientom WiFi

---

## Sprzęt

| Element | Ilość |
| Raspberry Pi Pico 2 W | 2× |
| Waveshare LoRa HAT SX1262 868M (wersja na goldpiny) | 2× |
| Antena 868 MHz SMA | 2× |
| Bateria LiPo 3.7V (opcjonalnie) | 2× |

## Wymagania — oprogramowanie

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (v1.5+)
- CMake 3.13+
- ARM GCC Toolchain (`arm-none-eabi-gcc`)
- Ninja (system budowania)
- VS Code + rozszerzenie *Raspberry Pi Pico* (opcjonalnie)

Biblioteki pobierane automatycznie przez CMake (`FetchContent`):
- [RadioLib](https://github.com/jgromes/RadioLib) — gałąź `master`
- mbedTLS — wbudowana w Pico SDK

---

## Instalacja i kompilacja

### 1. Sklonuj repozytorium

```bash
git clone https://github.com/kubaki1123/OGNR.git
cd OGNR
```

### 2. Skonfiguruj ID węzła

W pliku `server.h` zmień `node_id` przed kompilacją:

```cpp
uint8_t node_id = 1;  // węzeł BOB
// uint8_t node_id = 2;  // węzeł ALICE — odkomentuj dla drugiej płytki
```

### 3. Zbuduj projekt

```bash
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2_w
cmake --build .
```

### 4. Wgraj firmware

Przytrzymaj przycisk **BOOTSEL** na Pico, podłącz USB — pojawi się dysk `RPI-RP2`. Skopiuj plik `OGNR.uf2` na ten dysk.

Powtórz kroki 2–4 dla drugiej płytki (zmień `node_id = 2`).

---

## Użycie — klient konsolowy

Połącz się z siecią WiFi węzła (`O.G.N.R.1`, hasło: `123456789`)

Po połączeniu wpisz swój nick. Następnie możesz wysyłać:

```
# Wiadomość prywatna do użytkownika BOB
BOB:cześć, słyszysz mnie?

# Wiadomość do wszystkich
ALL:hej wszystkim!

# Lista aktywnych użytkowników (lokalnych i zdalnych)
INFO
```

---

## Architektura projektu

```
OGNR/
├── main.cpp           # Pętla główna, init WiFi AP, DHCP, LoRa, serwer TCP
├── lora_mesh.h        # Interfejs MeshRadio, struktura MeshPacket
├── lora_mesh.cpp      # Inicjalizacja SX1262, send/receive, szyfrowanie AES-CTR
├── server.h           # Interfejs Server_tcp
├── server.cpp         # Callbacki lwIP, logika routingu wiadomości, kolejka pakietów
├── dhcpserver.h/.c    # Serwer DHCP (bazuje na pico-examples)
├── Client.py          # Klient konsolowy do testów
└── CMakeLists.txt     # Konfiguracja budowania + FetchContent dla RadioLib
```

### Stos technologiczny

| Warstwa | Technologia |
| Mikrokontroler | Raspberry Pi Pico W (RP2040 / RP2350) |
| Radio | Semtech SX1262, 868 MHz, nakładka Waveshare |
| Biblioteka radiowa | RadioLib v7+ (z warstwą PicoHal) |
| Stos sieciowy | lwIP (Lightweight IP) — wbudowany w Pico SDK |
| Szyfrowanie | mbedTLS AES-128-CTR |
| Język / Standard | C++17 |
| Build system | CMake + Ninja + FetchContent |

---

## Protokół komunikacyjny

Każda wiadomość LoRa to struktura `MeshPacket` (221 bajtów maks.):

```
[ type | src_id | dst_id | msg_id | ttl | payload_len | payload[215] ]
  1B      1B      1B       1B       1B    1B             max 215B
```

| Pole | Opis |
|---|---|
| `type` | 0 = wiadomość, 1 = INFO_REQ, 2 = INFO_RSP |
| `dst_id` | 0xFF = broadcast, 0xFE = nieznany odbiorca |
| `ttl` | time-to-live — pakiet ginie po 3 przeskokach |
| `payload` | szyfrowany AES-128-CTR |

---

## Znane ograniczenia

- NONCE szyfrowania (AES-CTR) oparty na `msg_id` (1 bajt) — powtarza się co 256 wiadomości od tego samego węzła. Planowane rozszerzenie o pole `node_id`.
- Brak potwierdzeń dostarczenia (ACK) — wiadomość może zaginąć bez powiadomienia nadawcy.
- Sieć dwuwęzłowa — obsługa większej liczby węzłów (prawdziwy mesh z retransmisją) planowana w kolejnych wersjach.

---

*Projekt stworzony jako hobbystyczny system komunikacji off-grid.*
