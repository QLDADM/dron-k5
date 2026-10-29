/* DRON-K5: ustawienia i dziennik alarmów w EEPROM radia.
 * Obszar 0x1D00-0x1DFF: nieużywany przez egzumer i oryginalne oprogramowanie, a reset
 * ustawień egzumera go pomija - po powrocie do egzumera kanały i kalibracja są nietknięte.
 *   0x1D00  ustawienia (8 B)
 *   0x1D08  nagłówek dziennika (8 B)
 *   0x1D10  30 wpisów po 8 B (bufor kołowy)
 */
#ifndef APP_STORE_H
#define APP_STORE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint8_t threshold;   // dB
	int8_t  locked;      // -1 = wszystkie pasma
	uint8_t sound;       // 0/1
	uint8_t brightness;  // 1..10
	uint8_t lightTime;   // 0 = zawsze, 1 = 30 s, 2 = 2 min
	uint8_t bandMask;    // pasma skanowane w trybie "wszystkie"
} StoreSettings_t;

typedef struct {
	uint8_t  band;
	int8_t   peakDbm;
	uint16_t durationS;
	uint32_t time;       // czas Unix albo STORE_TIME_UPTIME | (start << 20) | sekundy od włączenia
} StoreEvent_t;

#define STORE_EVENTS        30
#define STORE_TIME_UPTIME   0x80000000u  // czas nieznany (radio nie ma zegara): numer włączenia + sekundy

extern StoreSettings_t gSettings;
extern uint32_t        gUnixBase;   // czas Unix = gUnixBase + sekundy od włączenia (0 = nieznany)
extern uint32_t        gUptimeS;

void    STORE_Load(void);
void    STORE_SaveSettings(void);
uint8_t STORE_EventCount(void);
bool    STORE_GetEvent(uint8_t newestIndex, StoreEvent_t *e);  // 0 = najnowszy
void    STORE_EventStart(uint8_t band, int8_t peakDbm);
void    STORE_EventUpdate(int8_t peakDbm);
void    STORE_EventEnd(void);
void    STORE_ClearLog(void);
uint32_t STORE_Now(void);

#endif
