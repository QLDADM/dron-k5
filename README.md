# DRON-K5: wykrywacz łączy dronów na radiu Quansheng UV-K5

Oprogramowanie dla popularnego radia **Quansheng UV-K5**, które wykrywa **sygnały sterowania dronami**:
łącza **ELRS, Crossfire i inne LRS** na **433, 868 i 915 MHz** oraz analogowe **wideo FPV na 1,2 GHz**.
Gdy w pobliżu pracuje aparatura drona albo sam dron, radio pokazuje napis **DRON**, zapala czerwoną diodę i daje sygnał dźwiękowy.

## Dwie wersje: wybierz jedną

| | **Wersja A: radio + detektor** | **Wersja B: DRON-K5, tylko skaner** |
|---|---|---|
| Co to jest | Firmware **egzumer** (rozbudowany firmware radiowy) z **dodanym trybem DRON DETEKTOR** | **Osobny firmware**, który zamienia radio w sam skaner dronów |
| Radio dalej działa jako radio? | ✅ **Tak**: kanały, nasłuch, nadawanie, wszystko jak w egzumerze | ❌ **Nie**: radio **nie nadaje** i nie odbiera rozmów |
| Jak włączyć wykrywanie | przypisujesz tryb do bocznego przycisku i włączasz go, kiedy chcesz | działa od razu po włączeniu radia |
| Wykrywanie 433 / 868 / 915 / 1,2 GHz | ✅ | ✅ |
| Wodospad kanałów | ❌ | ✅ |
| Tryb szukania pilota (ton rośnie z sygnałem) | ❌ | ✅ |
| Dziennik alarmów (30 ostatnich) | ❌ | ✅ |
| Menu ustawień (próg, pasma, jasność, podświetlenie) | tylko próg i dźwięk | ✅ pełne |
| Plik do wgrania | `egzumer-dron-detektor.packed.bin` | `dron-k5-skaner.packed.bin` |
| Dla kogo | radio ma zostać radiem, a detektor ma być „na wszelki wypadek” | osobne radio przeznaczone tylko do wykrywania dronów |

**Obie wersje można w każdej chwili zamienić** (wgrywasz drugi plik) albo cofnąć do zwykłego firmware.
Kanały i kalibracja zapisane w radiu **zostają nietknięte** przy każdej zmianie.

Wykrywanie opiera się na sile sygnału w pasmach używanych przez drony. To wskazówka, a nie pewna identyfikacja,
bo inne urządzenia nadające w tych pasmach (np. piloty bram, czujniki) też mogą dać alarm.
Drony DJI na Wi-Fi 2,4 / 5,8 GHz są poza zasięgiem tego radia. Do nich służy odbiornik Remote ID, np.
[RID Heltec](https://github.com/QLDADM/rid-heltec).

---

## Czy moje radio się nada?

| Radio | Czy działa |
|---|---|
| **Quansheng UV-K5, UV-K5(8), UV-K6, UV-5R Plus** z procesorem **DP32G030** (większość sprzedawanych w latach 2023–2024) | ✅ tak, obie wersje |
| Nowsze **UV-K5 V3**, **UV-K1** i inne z procesorem **PY32F071** | ❌ nie, mają inny procesor |

Jeśli na radiu działał już firmware **egzumer** albo inny alternatywny firmware UV-K5, obie wersje też zadziałają.

---

## Wgranie krok po kroku (tak samo dla obu wersji)

### Co jest potrzebne

- Radio Quansheng UV-K5 (patrz tabela wyżej), **naładowane**.
- **Kabel programujący USB**, ten z dwiema wtyczkami jack (tzw. kabel K1 albo „Baofeng/Kenwood”).
  Często jest w zestawie z radiem. Kosztuje kilkanaście–kilkadziesiąt złotych.
- Komputer z przeglądarką **Google Chrome** albo **Microsoft Edge**. Firefox i Safari nie obsługują tej metody.

### Krok 1. Pobierz plik wybranej wersji

1. Wejdź na https://github.com/QLDADM/dron-k5/releases.
2. Przy najnowszej wersji w sekcji **Assets** kliknij:
   - **`egzumer-dron-detektor.packed.bin`**: wersja A (radio + detektor),
   - albo **`dron-k5-skaner.packed.bin`**: wersja B (tylko skaner).
3. Plik trafi do folderu **Pobrane**. Nie otwieraj go, nie rozpakowuj i nie zmieniaj mu nazwy.

### Krok 2. Zrób kopię pamięci radia (zalecane, jednorazowo)

1. Podłącz kabel (krok 3), a radio włącz **normalnie**.
2. Wejdź na **https://egzumer.github.io/uvtools/** i wybierz **EEPROM dump**, potem port kabla.
3. Zapisz plik w bezpiecznym miejscu. To kopia kanałów i kalibracji radia na wypadek problemów.

### Krok 3. Podłącz kabel

1. Wepnij kabel do komputera. Windows zwykle sam instaluje sterownik. Gdy w kroku 5 przeglądarka nie pokaże żadnego portu,
   zainstaluj sterownik do układu z kabla: najczęściej **CH340** (https://www.wch-ic.com/downloads/CH341SER_EXE.html)
   albo **PL2303**.
2. **Wyłącz radio.**
3. Wepnij obie wtyczki kabla do gniazda akcesoriów radia (z boku, pod gumową zaślepką). Wciśnij je **do końca**.
   Wtyczki często wchodzą ciasno, a niedociśnięty kabel to najczęstsza przyczyna błędów.

### Krok 4. Włącz radio w trybie wgrywania

1. **Przytrzymaj przycisk PTT** (duży przycisk z boku radia).
2. Trzymając PTT, **włącz radio** pokrętłem.
3. Puść PTT. Ekran pozostaje **pusty**, a latarka albo dioda się świeci. To znaczy, że radio czeka na firmware.

Jeśli radio uruchomiło się normalnie (pokazuje częstotliwość), wyłącz je i spróbuj jeszcze raz.

### Krok 5. Wgraj firmware przez przeglądarkę

1. W Chrome albo Edge wejdź na **https://egzumer.github.io/uvtools/** i wybierz **Firmware flasher**.
2. Kliknij przycisk wyboru pliku i wskaż pobrany plik `.packed.bin`.
3. Kliknij przycisk wgrywania (**Flash**).
4. Przeglądarka pokaże okienko z portami. Wybierz port kabla (np. „USB-SERIAL CH340 (COM5)”) i kliknij **Połącz**.
5. Poczekaj, aż pasek dojdzie do 100% (kilkanaście sekund). **Nie odłączaj kabla w trakcie.**
6. Radio uruchomi się samo. Jeśli nie, wyłącz je i włącz ponownie.

### Powrót do zwykłego radia

Zrób dokładnie to samo (kroki 3–5), ale wgraj inny plik:
- czysty **egzumer**: https://github.com/egzumer/uv-k5-firmware-custom/releases,
- albo **oryginalny firmware Quansheng** ze strony producenta.

---

## Wersja A: radio + detektor (egzumer z trybem DRON DETEKTOR)

Radio działa jak z firmware egzumer: kanały, nasłuch, nadawanie i menu egzumera zostają.
Dochodzi tryb **DRON DETEKTOR**, który włączasz bocznym przyciskiem.

Jedna rzecz z egzumera jest wyłączona, żeby detektor zmieścił się w pamięci: **selektywne wywołania DTMF**
(DTMF calling). Zwykłe tony DTMF przy nadawaniu nadal działają.

### Jak włączyć detektor (jednorazowe ustawienie)

1. Naciśnij **MENU** i przewiń do jednej z pozycji **F1Shrt**, **F1Long**, **F2Shrt** albo **F2Long**.
   To funkcje bocznych przycisków: F1 to górny, F2 dolny, Shrt to krótkie naciśnięcie, Long przytrzymanie.
   Przykład: **F1Long**, czyli przytrzymanie górnego bocznego przycisku.
2. Naciśnij **MENU**, wybierz strzałkami **DRON DETEKTOR** i zatwierdź **MENU**. Wyjdź przyciskiem **EXIT**.
3. Od teraz ten boczny przycisk (np. przytrzymanie górnego) **włącza detektor**.

### Ekran detektora

```
DRON DETEKTOR          85%
        BRAK            <- BRAK / DRON? / DRON 868
>433 -118  0  ▪▪▫▫▫|▫▫▫▫▫▫
 868  -97  7  ▪▪▪▪▪|▪▪▪▫▫▫   <- pasmo, szczyt dBm, liczba aktywnych kanałów, wynik
 915 -121  1  ▫▫▫▫▫|▫▫▫▫▫▫
 1G2 -119  0  ▫▫▫▫▫|▫▫▫▫▫▫
8dB WSZ DZW
```

`>` to pasmo skanowane w tej chwili, a `*` pasmo zablokowane. Kreska w pasku to próg alarmu.
Po przekroczeniu progu: napis **DRON**, czerwona dioda, potrójny sygnał, a potem przypomnienie co 10 s.

### Klawisze w trybie detektora

| Klawisz | Działanie |
|---|---|
| **0** | skanuj wszystkie pasma (domyślnie) |
| **1 / 2 / 3 / 4** | tylko 433 / 868 / 915 MHz / 1,2 GHz (szybsza reakcja) |
| **\*** / **F** | próg czułości +1 / −1 dB (3–30, domyślnie 8) |
| **MENU** | dźwięk wł./wył. |
| boczny przycisk 2 | podświetlenie wł./wył. |
| **EXIT** | **powrót do zwykłej pracy radia** |

W trybie detektora radio **nigdy nie nadaje**, a PTT jest ignorowany.

---

## Wersja B: DRON-K5, sam skaner

Radio po włączeniu od razu skanuje pasma. Nie ma kanałów ani nadawania. Ustawienia skanera są zapamiętane w radiu.
CHIRP (program do kanałów) z tą wersją nie działa, bo nie ma w niej kanałów.

### Ekran główny

Pokazuje 4 pasma (433, 868, 915 MHz, 1,2 GHz) z bieżącą siłą sygnału. Przy wykryciu łącza drona pojawia się **DRON**,
czerwona dioda, trzy tony i wpis do dziennika.

### Klawisze

| Klawisz | Działanie |
|---|---|
| **0** | wszystkie pasma (ekran główny) |
| **1 / 2 / 3 / 4** | tylko 433 / 868 / 915 MHz / 1,2 GHz, z **wodospadem kanałów** |
| **5** | **tryb szukania**: duży odczyt siły sygnału, pasek, maksimum i ton rosnący z siłą sygnału; ponownie **5** kończy |
| **9** | w trybie szukania zeruje maksimum |
| **7** | **dziennik** ostatnich 30 alarmów (strzałki przewijają) |
| **MENU** | ustawienia: próg, dźwięk, jasność, czas podświetlenia, włączone pasma, wyczyszczenie dziennika |
| **\*** / **F** | czułość: próg +1 / −1 dB |
| boczny przycisk 1 | dźwięk wł./wył. |
| boczny przycisk 2 | podświetlenie wł./wył. |
| **EXIT** | powrót do ekranu głównego |

### Jak szukać pilota drona

1. Na ekranie głównym sprawdź, w którym paśmie jest alarm.
2. Naciśnij **1–4**, żeby zostać tylko w tym paśmie, potem **5**.
3. Obracaj się powoli z radiem i idź w stronę, w której ton jest **wyższy**, a odczyt większy.
   Ciało zasłania sygnał, więc trzymaj radio przed sobą.
4. Gdy sygnał przestaje rosnąć, naciśnij **9** (zerowanie maksimum) i szukaj dalej.

---

## Fałszywe alarmy (obie wersje)

Jeśli radio alarmuje, a dronów nie ma, zwiększ próg przyciskiem **\*** (o 2–3 dB).
W wersji B możesz też w **MENU** wyłączyć pasmo, na którym w okolicy pracuje coś stałego.
Stała nośna (np. przemiennik) po kilku sekundach przestaje wywoływać alarm sama, bo detektor uczy się poziomu szumu.

## Najczęstsze problemy

| Problem | Rozwiązanie |
|---|---|
| Po kliknięciu „Flash” lista portów jest pusta | Zainstaluj sterownik kabla (CH340 albo PL2303), wepnij kabel do innego portu USB. |
| Błąd albo przerwanie w trakcie wgrywania | Dociśnij wtyczki kabla w radiu do końca. Wyłącz radio, włącz z PTT i spróbuj ponownie. |
| Radio nie wchodzi w tryb wgrywania | PTT trzeba trzymać **przed** włączeniem i w trakcie włączania. |
| Komunikat o niepasującym firmware albo nic się nie dzieje | Radio może mieć nowszy procesor PY32F071 (UV-K5 V3 / UV-K1), na którym ten firmware nie działa. |
| Po wgraniu radio nie startuje | Wgraj ponownie (kroki 4–5). Tryb wgrywania działa zawsze, więc radia nie da się w ten sposób „zepsuć”. |
| Wersja A: w menu nie ma DRON DETEKTOR | Sprawdź, czy wgrany jest plik `egzumer-dron-detektor`, a nie czysty egzumer. Pozycja jest w F1Shrt / F1Long / F2Shrt / F2Long. |

---

## Połączenie z komputerem (dla zaawansowanych)

Obie wersje wysyłają przez kabel programujący (38400 8N1) jedną linię na każde okno pomiaru:
`DD,<pasmo>,<wynik>,<kanały>,<dBm>,<alarm>`, np. `DD,868,9,7,-97,1`.
W wersji A dzieje się to, gdy włączony jest tryb detektora.

Wersja B przyjmuje też polecenia: `TIME <unix>` (godzina dziennika), `LOG` (dziennik jako linie `DL,...`), `ID`.
Bez ustawionej godziny wpis dziennika ma postać `#numer_włączenia+czas_od_włączenia`.

## Budowanie ze źródeł

Potrzebny jest Docker (i git dla wersji A).

- **Wersja A:** `egzumer-mod\build.bat` pobiera egzumer (commit 7607f0a), nakłada zmiany
  (`egzumer-dron-detektor.patch` + `dronedet.c/.h`) i buduje. Wynik: `egzumer-mod\egzumer\firmware.packed.bin`.
- **Wersja B:** `skaner\build.bat`. Wynik: `skaner\scanner.packed.bin`.

Oba skrypty dają pliki identyczne bajt w bajt z plikami w Releases.

## Zawartość repozytorium

- [`egzumer-mod/`](egzumer-mod/): wersja A, czyli zmiany w egzumerze (łatka i kod detektora).
- [`skaner/`](skaner/): wersja B, czyli pełny kod DRON-K5.
- [`release/`](release/): gotowe pliki obu wersji (te same co w Releases).
- [`tools/`](tools/): obraz Docker z kompilatorem.

## Licencja i pochodzenie kodu

- Wersja A to [egzumer/uv-k5-firmware-custom](https://github.com/egzumer/uv-k5-firmware-custom) (opartego na pracy
  [DualTachyon](https://github.com/DualTachyon/uv-k5-firmware)) ze zmianami opisanymi w `egzumer-mod/`. Licencja: Apache 2.0.
- W wersji B sterowniki sprzętu (`driver/`, `bsp/`, `board.c`, `font.c`, `init.c`, `start.S`, `fw-pack.py`) pochodzą z egzumera
  (Apache 2.0). Część plików została zmodyfikowana (uproszczona pod skaner).
- Detektor (`egzumer-mod/dronedet.*`) i aplikacja skanera (`skaner/app/`, `skaner/main.c`) powstały na potrzeby tego projektu. Licencja: Apache 2.0.
- `skaner/external/printf`: [mpaland/printf](https://github.com/mpaland/printf), licencja MIT.
- `skaner/external/CMSIS_5`: nagłówki ARM CMSIS, licencja Apache 2.0.

Pełny tekst licencji jest w pliku [LICENSE](LICENSE). Oprogramowanie jest udostępniane bez żadnej gwarancji. Wgrywasz je na własną odpowiedzialność.
