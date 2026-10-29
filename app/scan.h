/* DRON-K5: silnik skanowania - wykrywanie łączy sterowania dronów po wzorcu RSSI.
 *
 * Każde pasmo jest przemiatane kanał po kanale; RSSI kanału porównujemy z jego własnym,
 * uczonym poziomem szumu. Łącza FHSS (ELRS, Crossfire, LRS) zapalają w oknie wiele różnych
 * kanałów; stałe nośne są stopniowo wchłaniane przez poziom szumu, a pojedyncze kanały
 * (LoRaWAN, liczniki, PMR) zapalają ich mało. Pasmo 1,2 GHz: reguła ciągłej nośnej (wideo FPV).
 */
#ifndef APP_SCAN_H
#define APP_SCAN_H

#include <stdbool.h>
#include <stdint.h>

#define SCAN_SCORE_MAX   12
#define SCAN_SCORE_ALARM 6
#define SCAN_SCORE_MAYBE 3
#define SCAN_BIN_COUNT   101
#define SCAN_WF_ROWS     32   // wodospad: tyle ostatnich okien zablokowanego pasma

typedef struct {
	const char *name;
	uint32_t    fStart;   // jednostki 10 Hz
	uint32_t    step;     // jednostki 10 Hz
	uint8_t     count;
	uint8_t     offset;   // pierwszy kanał we wspólnych tablicach
	bool        carrier;  // reguła ciągłej nośnej zamiast skakania
} ScanBand_t;

typedef struct {
	uint16_t ref;         // najniższy poziom szumu w paśmie
	uint16_t peak;        // najsilniejsza próbka ostatniego okna
	uint16_t peakNow;
	uint8_t  distinct;    // kanały z aktywnością w ostatnim oknie
	uint8_t  score;
	uint8_t  windows;
	bool     primed;
	bool     alarm;
} ScanBandState_t;

extern const ScanBand_t   SCAN_BANDS[];
extern ScanBandState_t    gScanState[];
extern uint8_t            gScanLastHits[SCAN_BIN_COUNT];
extern uint8_t            gScanCurBand;
extern uint8_t            gScanLastBand;     // pasmo, którego okno właśnie się skończyło
extern int8_t             gScanLockedBand;   // -1 = wszystkie pasma po kolei
extern uint8_t            gScanBandMask;     // bit n = pasmo n włączone (tryb "wszystkie")
extern uint8_t            gScanThresholdDb;
extern bool               gScanHunt;         // tryb szukania: krótsze okno
extern bool               gScanHuntSignal;   // w ostatnim oknie był sygnał ponad szumem
extern uint16_t           gScanHuntMax;
extern uint8_t            gScanWaterfall[SCAN_WF_ROWS][8];  // bity kanałów aktywnych, [0] = najnowsze
extern uint8_t            gScanWfCount;

#define SCAN_BAND_COUNT 4

void     SCAN_Init(void);          // ustawia odbiornik na nasłuch (bez nadawania)
void     SCAN_RestoreRx(void);     // po sygnale dźwiękowym: odbiornik z powrotem w tryb skanowania
void     SCAN_SampleNext(void);    // jedna próbka RSSI kolejnego kanału
bool     SCAN_Tick10ms(void);      // true, gdy skończyło się okno pomiaru
void     SCAN_SelectBand(int8_t band);
bool     SCAN_AnyAlarm(void);
int8_t   SCAN_BestAlarmBand(void); // pasmo z najwyższym wynikiem alarmu albo -1
int      SCAN_RssiToDbm(uint16_t rssi);

#endif
