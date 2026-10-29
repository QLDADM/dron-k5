/* RID-K5: ustawienia i dziennik alarmów (opis w store.h). */
#include <string.h>

#include "app/store.h"
#include "driver/eeprom.h"

#define ADDR_SETTINGS 0x1D00
#define ADDR_HEADER   0x1D08
#define ADDR_EVENTS   0x1D10
#define MAGIC0        'R'
#define MAGIC1        'K'

StoreSettings_t gSettings;
uint32_t        gUnixBase;
uint32_t        gUptimeS;

static struct {
	uint8_t  head;     // indeks następnego wpisu
	uint8_t  count;
	uint16_t boot;     // licznik włączeń radia (czas wpisów bez zegara)
	uint8_t  pad[4];
} header;

static int8_t   openIdx = -1;  // trwający alarm (wpis do uzupełnienia przy końcu)
static uint32_t openStart;
static StoreEvent_t openEvent;

static void WriteEvent(uint8_t idx, const StoreEvent_t *e)
{
	EEPROM_WriteBuffer(ADDR_EVENTS + idx * 8, e);
}

static void SaveHeader(void)
{
	EEPROM_WriteBuffer(ADDR_HEADER, &header);
}

void STORE_Load(void)
{
	uint8_t raw[8];
	EEPROM_ReadBuffer(ADDR_SETTINGS, raw, 8);
	if (raw[0] == MAGIC0 && raw[1] == MAGIC1) {
		gSettings.threshold  = raw[2] >= 3 && raw[2] <= 30 ? raw[2] : 8;
		gSettings.locked     = (int8_t)raw[3] >= -1 && (int8_t)raw[3] < 4 ? (int8_t)raw[3] : -1;
		gSettings.sound      = raw[4] & 1;
		gSettings.brightness = raw[5] >= 1 && raw[5] <= 10 ? raw[5] : 10;
		gSettings.lightTime  = raw[6] <= 2 ? raw[6] : 0;
		gSettings.bandMask   = (raw[7] & 0x0F) ? (raw[7] & 0x0F) : 0x0F;
	} else {
		gSettings = (StoreSettings_t){8, -1, 1, 10, 0, 0x0F};
	}

	EEPROM_ReadBuffer(ADDR_HEADER, &header, sizeof(header));
	if (header.head >= STORE_EVENTS || header.count > STORE_EVENTS || header.boot == 0xFFFF)
		memset(&header, 0, sizeof(header));  // pierwsze uruchomienie (pusta albo obca zawartość)
	header.boot++;
	SaveHeader();
}

void STORE_SaveSettings(void)
{
	const uint8_t raw[8] = {MAGIC0, MAGIC1, gSettings.threshold, (uint8_t)gSettings.locked,
	                        gSettings.sound, gSettings.brightness, gSettings.lightTime, gSettings.bandMask};
	EEPROM_WriteBuffer(ADDR_SETTINGS, raw);
}

uint32_t STORE_Now(void)
{
	if (gUnixBase)
		return gUnixBase + gUptimeS;
	return STORE_TIME_UPTIME | ((uint32_t)(header.boot & 0x7FF) << 20) | (gUptimeS & 0xFFFFF);
}

uint8_t STORE_EventCount(void)
{
	return header.count;
}

bool STORE_GetEvent(uint8_t newestIndex, StoreEvent_t *e)
{
	if (newestIndex >= header.count)
		return false;
	const uint8_t idx = (header.head + STORE_EVENTS - 1 - newestIndex) % STORE_EVENTS;
	if ((int8_t)idx == openIdx) {  // trwający alarm: aktualne wartości
		*e = openEvent;
		e->durationS = gUptimeS - openStart;
		return true;
	}
	EEPROM_ReadBuffer(ADDR_EVENTS + idx * 8, e, sizeof(*e));
	return true;
}

void STORE_EventStart(uint8_t band, int8_t peakDbm)
{
	openEvent = (StoreEvent_t){band, peakDbm, 0, STORE_Now()};
	openIdx   = header.head;
	openStart = gUptimeS;
	WriteEvent(header.head, &openEvent);
	header.head = (header.head + 1) % STORE_EVENTS;
	if (header.count < STORE_EVENTS)
		header.count++;
	SaveHeader();
}

void STORE_EventUpdate(int8_t peakDbm)
{
	if (openIdx >= 0 && peakDbm > openEvent.peakDbm)
		openEvent.peakDbm = peakDbm;  // zapis do EEPROM dopiero przy końcu alarmu
}

void STORE_EventEnd(void)
{
	if (openIdx < 0)
		return;
	openEvent.durationS = gUptimeS - openStart;
	WriteEvent(openIdx, &openEvent);
	openIdx = -1;
}

void STORE_ClearLog(void)
{
	openIdx = -1;
	header.head = 0;
	header.count = 0;
	SaveHeader();
}
