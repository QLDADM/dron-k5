# RID-K5: skaner łączy dronów na radiu Quansheng UV-K5

Firmware zamienia popularne radio **Quansheng UV-K5** w **skaner sygnałów sterowania dronami**.
Wykrywa łącza **ELRS, Crossfire i inne LRS** na **433, 868 i 915 MHz** oraz analogowe **wideo FPV na 1,2 GHz**.
Gdy w pobliżu pracuje aparatura drona albo sam dron, radio pokazuje napis **DRON**, zapala czerwoną diodę i daje sygnał dźwiękowy.

- Ma **tryb szukania**: im silniejszy sygnał, tym wyższy ton, więc pomaga znaleźć pilota drona.
- Pokazuje **wodospad kanałów**: widać, gdzie i jak często pojawia się sygnał.
- Prowadzi **dziennik** ostatnich 30 alarmów z godziną, czasem trwania i siłą sygnału.
- **Radio z tym firmware nie nadaje.** Nie ma w nim kodu nadawania, a przycisk PTT nic nie robi.
- Jest proste do cofnięcia: w każdej chwili możesz wgrać z powrotem zwykły firmware radia.

Wykrywanie opiera się na sile sygnału w pasmach używanych przez drony. To wskazówka, a nie pewna identyfikacja,
bo inne urządzenia nadające w tych pasmach (np. piloty bram, czujniki) też mogą dać alarm.
Drony DJI na Wi-Fi 2,4 / 5,8 GHz są poza zasięgiem tego radia. Do nich służy odbiornik Remote ID, np.
[RID Heltec](https://github.com/QLDADM/rid-heltec).

---

## Czy moje radio się nada?

| Radio | Czy działa |
|---|---|
| **Quansheng UV-K5, UV-K5(8), UV-K6, UV-5R Plus** z procesorem **DP32G030** (większość sprzedawanych w latach 2023–2024) | ✅ tak |
| Nowsze **UV-K5 V3**, **UV-K1** i inne z procesorem **PY32F071** | ❌ nie, mają inny procesor |

Jeśli na radiu działał już firmware **egzumer** albo inny alternatywny firmware UV-K5, RID-K5 też zadziała.

---

## Wgranie krok po kroku

### Co jest potrzebne

- Radio Quansheng UV-K5 (patrz tabela wyżej), **naładowane**.
- **Kabel programujący USB**, ten z dwiema wtyczkami jack (tzw. kabel K1 albo „Baofeng/Kenwood”).
  Często jest w zestawie z radiem. Kosztuje kilkanaście–kilkadziesiąt złotych.
- Komputer z przeglądarką **Google Chrome** albo **Microsoft Edge**. Firefox i Safari nie obsługują tej metody.

### Krok 1. Pobierz plik firmware

1. Wejdź na https://github.com/QLDADM/rid-k5/releases.
2. Przy najnowszej wersji w sekcji **Assets** kliknij **`rid-k5-skaner.packed.bin`**.
3. Plik trafi do folderu **Pobrane**. Nie otwieraj go, nie rozpakowuj i nie zmieniaj mu nazwy.

### Krok 2. Podłącz kabel

1. Wepnij kabel do komputera. Windows zwykle sam instaluje sterownik. Gdy w kroku 4 przeglądarka nie pokaże żadnego portu,
   zainstaluj sterownik do układu z kabla: najczęściej **CH340** (https://www.wch-ic.com/downloads/CH341SER_EXE.html)
   albo **PL2303**.
2. **Wyłącz radio.**
3. Wepnij obie wtyczki kabla do gniazda akcesoriów radia (z boku, pod gumową zaślepką). Wciśnij je **do końca**.
   Wtyczki często wchodzą ciasno, a niedociśnięty kabel to najczęstsza przyczyna błędów.

### Krok 3. Włącz radio w trybie wgrywania

1. **Przytrzymaj przycisk PTT** (duży przycisk z boku radia).
2. Trzymając PTT, **włącz radio** pokrętłem.
3. Puść PTT. Ekran pozostaje **pusty**, a latarka albo dioda się świeci. To znaczy, że radio czeka na firmware.

Jeśli radio uruchomiło się normalnie (pokazuje częstotliwość), wyłącz je i spróbuj jeszcze raz.

### Krok 4. Wgraj firmware przez przeglądarkę

1. W Chrome albo Edge wejdź na **https://egzumer.github.io/uvtools/** i wybierz **Firmware flasher**.
2. Kliknij przycisk wyboru pliku i wskaż pobrany **`rid-k5-skaner.packed.bin`**.
3. Kliknij przycisk wgrywania (**Flash**).
4. Przeglądarka pokaże okienko z portami. Wybierz port kabla (np. „USB-SERIAL CH340 (COM5)”) i kliknij **Połącz**.
5. Poczekaj, aż pasek dojdzie do 100% (kilkanaście sekund). **Nie odłączaj kabla w trakcie.**
6. Radio uruchomi się samo i pokaże ekran skanera z pasmami. Jeśli nie, wyłącz je i włącz ponownie.

Gotowe. Radio od razu skanuje pasma.

### Powrót do zwykłego radia

Zrób dokładnie to samo (kroki 2–4), ale wgraj inny plik:
- **egzumer** (rozbudowany firmware radiowy): https://github.com/egzumer/uv-k5-firmware-custom/releases,
- albo **oryginalny firmware Quansheng** ze strony producenta.

Kanały i kalibracja zapisane w radiu **zostają nietknięte**. RID-K5 zapisuje swoje ustawienia tylko w obszarze pamięci
(0x1D00–0x1DFF), którego zwykły firmware nie używa. Po powrocie radio działa jak przedtem.

Dla bezpieczeństwa możesz przed pierwszym wgraniem zrobić kopię pamięci radia: na https://egzumer.github.io/uvtools/
→ **EEPROM dump** (radio włączone normalnie, z podłączonym kablem).

---

## Obsługa

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

Ustawienia są zapamiętane w radiu.

### Jak szukać pilota drona

1. Na ekranie głównym sprawdź, w którym paśmie jest alarm.
2. Naciśnij **1–4**, żeby zostać tylko w tym paśmie, potem **5**.
3. Obracaj się powoli z radiem i idź w stronę, w której ton jest **wyższy**, a odczyt większy.
   Ciało zasłania sygnał, więc trzymaj radio przed sobą.
4. Gdy sygnał przestaje rosnąć, naciśnij **9** (zerowanie maksimum) i szukaj dalej.

### Fałszywe alarmy

Jeśli radio alarmuje, a dronów nie ma, zwiększ próg przyciskiem **\*** (o 2–3 dB) albo wyłącz w **MENU** pasmo,
na którym pracuje coś stałego w okolicy.

---

## Najczęstsze problemy

| Problem | Rozwiązanie |
|---|---|
| Po kliknięciu „Flash” lista portów jest pusta | Zainstaluj sterownik kabla (CH340 albo PL2303), wepnij kabel do innego portu USB. |
| Błąd albo przerwanie w trakcie wgrywania | Dociśnij wtyczki kabla w radiu do końca. Wyłącz radio, włącz z PTT i spróbuj ponownie. |
| Radio nie wchodzi w tryb wgrywania | PTT trzeba trzymać **przed** włączeniem i w trakcie włączania. |
| Komunikat o niepasującym firmware albo nic się nie dzieje | Radio może mieć nowszy procesor PY32F071 (UV-K5 V3 / UV-K1), na którym ten firmware nie działa. |
| Po wgraniu radio nie startuje | Wgraj ponownie (kroki 3–4). Tryb wgrywania działa zawsze, więc radia nie da się w ten sposób „zepsuć”. |

---

## Połączenie z komputerem (dla zaawansowanych)

Przez ten sam kabel programujący (38400 8N1):
- Radio → PC: `DD,<pasmo>,<wynik>,<kanały>,<dBm>,<alarm>` po każdym oknie pomiaru, a po poleceniu `LOG` linie `DL,...` z dziennikiem.
- PC → radio: `TIME <unix>` (ustawia godzinę dziennika), `LOG`, `ID`.

Bez ustawionej godziny wpis dziennika ma postać `#numer_włączenia+czas_od_włączenia`.

## Budowanie ze źródeł

Potrzebny jest Docker. W katalogu repozytorium uruchom `build.bat` (Windows) albo:

```bash
docker build -t uvk5-toolchain tools
docker run --rm -v "$PWD:/src" uvk5-toolchain make
```

Wynik to `scanner.packed.bin`. Program zajmuje około 16 KB z 60 KB dostępnych.

## Licencja i pochodzenie kodu

- Sterowniki sprzętu (`driver/`, `bsp/`, `hardware/`, `board.c`, `font.c`, `init.c`, `start.S`, `fw-pack.py`) pochodzą z
  [egzumer/uv-k5-firmware-custom](https://github.com/egzumer/uv-k5-firmware-custom), opartego na pracy
  [DualTachyon](https://github.com/DualTachyon/uv-k5-firmware). Licencja: Apache 2.0. Część plików została
  zmodyfikowana (uproszczona pod skaner).
- Aplikacja skanera (`app/`, `main.c`) powstała na potrzeby tego projektu. Licencja: Apache 2.0.
- `external/printf`: [mpaland/printf](https://github.com/mpaland/printf), licencja MIT.
- `external/CMSIS_5`: nagłówki ARM CMSIS, licencja Apache 2.0.

Pełny tekst licencji jest w pliku [LICENSE](LICENSE). Firmware jest udostępniany bez żadnej gwarancji. Wgrywasz go na własną odpowiedzialność.
