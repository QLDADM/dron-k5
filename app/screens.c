/* RID-K5: ekrany i klawisze (opis w screens.h). Ekran 128x64: pasek stanu 8 px + 7 linii po 8 px. */
#include <string.h>

#include "app/scan.h"
#include "app/screens.h"
#include "app/sound.h"
#include "app/store.h"
#include "board.h"
#include "driver/backlight.h"
#include "driver/eeprom.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "external/printf/printf.h"
#include "ui/helper.h"

enum { SCR_MAIN, SCR_BAND, SCR_HUNT, SCR_LOG, SCR_MENU };

bool gBacklightForcedOff;

static uint8_t    screen = SCR_MAIN;
static uint8_t    logTop, menuSel;
static KEY_Code_t prevKey = KEY_INVALID, heldKey = KEY_INVALID;
static uint16_t   batteryVoltages[4], batteryCal[6];
static uint8_t    batteryIdx;
static char       line[32];

// ---------------------------------------------------------------- bateria

static unsigned BatteryPercent(void)
{
	static const uint16_t V[] = {828, 814, 760, 729, 630};  // 10 mV, akumulator 1600 mAh (jak w egzumerze)
	static const uint8_t  P[] = {100, 97, 25, 6, 0};
	const uint32_t v = (batteryVoltages[0] + batteryVoltages[1] + batteryVoltages[2] + batteryVoltages[3]) / 4
	                   * 760 / batteryCal[3];
	if (v >= V[0])
		return 100;
	for (int i = 1; i < 5; i++)
		if (v >= V[i])
			return P[i] + (P[i - 1] - P[i]) * (v - V[i]) / (V[i - 1] - V[i]);
	return 0;
}

void SCREENS_UpdateBattery(void)
{
	uint16_t current;
	BOARD_ADC_GetBatteryInfo(&batteryVoltages[batteryIdx++ % 4], &current);
}

// ---------------------------------------------------------------- czas (dziennik)

// czas Unix -> lokalny czas polski (CET/CEST: ostatnia niedziela marca / października, 1:00 UTC)
static void LocalTime(uint32_t t, int *day, int *mon, int *hour, int *min)
{
	int32_t days = t / 86400, secs = t % 86400;
	// civil_from_days (H. Hinnant)
	int32_t z = days + 719468, era = z / 146097;
	uint32_t doe = z - era * 146097, yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	int32_t y = yoe + era * 400;
	uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
	int d = doy - (153 * mp + 2) / 5 + 1, m = mp < 10 ? mp + 3 : mp - 9;
	if (m <= 2)
		y++;
	// ostatnia niedziela miesiąca m (dzień tygodnia 31-go: Zeller dla m/31)
	int offset = 1;
	if (m > 3 && m < 10) {
		offset = 2;
	} else if (m == 3 || m == 10) {
		int32_t d31 = days - d + 31;               // dni do 31. dnia tego miesiąca
		int lastSun = 31 - (int)((d31 + 4) % 7);   // 1970-01-01 to czwartek
		bool summer = m == 3 ? (d > lastSun || (d == lastSun && secs >= 3600))
		                     : (d < lastSun || (d == lastSun && secs < 3600));
		offset = summer ? 2 : 1;
	}
	secs += offset * 3600;
	if (secs >= 86400) {  // przejście na następny dzień (bez zmiany miesiąca - wystarczy do listy)
		secs -= 86400;
		d++;
	}
	*day = d;
	*mon = m;
	*hour = secs / 3600;
	*min = secs % 3600 / 60;
}

static void EventTime(uint32_t t, char *out)
{
	if (t & STORE_TIME_UPTIME) {  // radio nie znało czasu: numer włączenia + czas od włączenia
		const uint32_t up = t & 0xFFFFF;
		sprintf(out, "#%lu+%lu:%02lu", (unsigned long)((t >> 20) & 0x7FF), (unsigned long)(up / 3600),
		        (unsigned long)(up / 60 % 60));
		return;
	}
	int day, mon, hour, min;
	LocalTime(t, &day, &mon, &hour, &min);
	sprintf(out, "%02d.%02d %02d:%02d", day, mon, hour, min);
}

// ---------------------------------------------------------------- rysowanie

static void PutPixel(uint8_t x, uint8_t y)
{
	if (x < 128 && y < 56)
		gFrameBuffer[y >> 3][x] |= 1 << (y & 7);
}

static void FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	for (uint8_t i = 0; i < w; i++)
		for (uint8_t j = 0; j < h; j++)
			PutPixel(x + i, y + j);
}

static void DrawScoreBar(uint8_t row, uint8_t score)
{
	for (uint8_t i = 0; i < SCAN_SCORE_MAX; i++) {
		const uint8_t x = 80 + i * 4;
		const uint8_t v = i < score ? 0b00111110 : 0b00100000;
		gFrameBuffer[row][x] |= v;
		gFrameBuffer[row][x + 1] |= v;
		gFrameBuffer[row][x + 2] |= v;
	}
	gFrameBuffer[row][80 + SCAN_SCORE_ALARM * 4 - 1] |= 0b01111111;
}

static void DrawHeadline(void)
{
	const int8_t best = SCAN_BestAlarmBand();
	if (best >= 0) {
		sprintf(line, "DRON %s", SCAN_BANDS[best].name);
	} else {
		strcpy(line, "BRAK");
		for (uint8_t i = 0; i < SCAN_BAND_COUNT; i++)
			if (gScanState[i].score >= SCAN_SCORE_MAYBE)
				strcpy(line, "DRON?");
	}
	UI_PrintString(line, 0, 127, 0, 10);
}

static void DrawStatusLine(const char *title)
{
	memset(gStatusLine, 0, sizeof(gStatusLine));
	UI_PrintStringSmallBufferNormal(title, gStatusLine);
	sprintf(line, "%u%%", BatteryPercent());
	UI_PrintStringSmallBufferNormal(line, gStatusLine + (strlen(line) == 4 ? 98 : 105));
	ST7565_BlitStatusLine();
}

static void RenderMain(void)
{
	DrawHeadline();
	for (uint8_t i = 0; i < SCAN_BAND_COUNT; i++) {
		const ScanBandState_t *s = &gScanState[i];
		const char mark = i == gScanCurBand ? '>' : ' ';
		if (!(gScanBandMask & (1 << i)))
			sprintf(line, " %s  wyl.", SCAN_BANDS[i].name);
		else if (s->windows)
			sprintf(line, "%c%s%4d %2u", mark, SCAN_BANDS[i].name, SCAN_RssiToDbm(s->peak), s->distinct);
		else
			sprintf(line, "%c%s ---  -", mark, SCAN_BANDS[i].name);
		UI_PrintStringSmallNormal(line, 0, 0, 2 + i);
		if (gScanBandMask & (1 << i))
			DrawScoreBar(2 + i, s->score);
	}
	sprintf(line, "%udB WSZ %s", gScanThresholdDb, gSettings.sound ? "DZW" : "CICHO");
	UI_PrintStringSmallNormal(line, 0, 0, 6);
}

// wodospad: kolumny = kanały pasma, wiersze = kolejne okna (najnowsze u góry)
static void DrawWaterfall(const ScanBand_t *b, uint8_t y0, uint8_t rows)
{
	const uint8_t count = b->count > 64 ? 64 : b->count;
	const uint8_t w = 128 / count;
	const uint8_t x0 = (128 - w * count) / 2;
	for (uint8_t r = 0; r < rows && r < gScanWfCount; r++)
		for (uint8_t i = 0; i < count; i++)
			if (gScanWaterfall[r][i >> 3] & (1 << (i & 7)))
				FillRect(x0 + i * w, y0 + r, w > 1 ? w - 1 : 1, 1);
	for (uint8_t x = 0; x < 128; x += 2)  // linia bazowa pod wodospadem
		PutPixel(x, y0 + rows);
}

static void RenderBand(void)
{
	const ScanBand_t      *b = &SCAN_BANDS[gScanCurBand];
	const ScanBandState_t *s = &gScanState[gScanCurBand];
	DrawHeadline();
	DrawWaterfall(b, 16, 31);
	if (s->windows)
		sprintf(line, "%s %ddBm %uk %udB", b->name, SCAN_RssiToDbm(s->peak), s->distinct, gScanThresholdDb);
	else
		sprintf(line, "%s - nauka szumu", b->name);
	UI_PrintStringSmallNormal(line, 0, 0, 6);
}

static void RenderHunt(void)
{
	const ScanBand_t      *b = &SCAN_BANDS[gScanCurBand];
	const ScanBandState_t *s = &gScanState[gScanCurBand];
	const int dbm = SCAN_RssiToDbm(s->peak);
	if (gScanHuntSignal)
		sprintf(line, "%d dBm", dbm);
	else
		strcpy(line, "---");
	UI_PrintString(line, 0, 127, 0, 10);
	// pasek siły sygnału -125 ... -40 dBm
	int w = gScanHuntSignal ? (dbm + 125) * 128 / 85 : 0;
	w = w < 0 ? 0 : w > 128 ? 128 : w;
	FillRect(0, 17, w, 6);
	for (uint8_t x = 0; x < 128; x += 16)
		PutPixel(x, 24);
	sprintf(line, "%s max %d", b->name, gScanHuntMax ? SCAN_RssiToDbm(gScanHuntMax) : 0);
	UI_PrintStringSmallNormal(line, 0, 0, 3);
	// aktywne kanały w ostatnim oknie
	const uint8_t count = b->count;
	const uint8_t bw = 128 / count;
	for (uint8_t i = 0; i < count; i++) {
		uint8_t hits = gScanLastHits[b->offset + i];
		uint8_t h = hits >= 3 ? 15 : hits * 5;
		FillRect(i * bw, 47 - h, bw > 1 ? bw - 1 : 1, h ? h : 1);
	}
	UI_PrintStringSmallNormal("5:koniec 9:zeruj", 0, 0, 6);
}

static void RenderLog(void)
{
	const uint8_t n = STORE_EventCount();
	if (!n) {
		UI_PrintStringSmallNormal("Brak alarmow", 0, 127, 2);
		UI_PrintStringSmallNormal("w dzienniku", 0, 127, 3);
		return;
	}
	StoreEvent_t e;
	char when[16];
	for (uint8_t r = 0; r < 7 && STORE_GetEvent(logTop + r, &e); r++) {
		EventTime(e.time, when);
		const unsigned dur = e.durationS < 60 ? e.durationS : e.durationS / 60;
		sprintf(line, "%s %s %u%s", e.band < SCAN_BAND_COUNT ? SCAN_BANDS[e.band].name : "?", when, dur,
		        e.durationS < 60 ? "s" : "m");
		UI_PrintStringSmallNormal(line, 0, 0, r);
	}
}

#define MENU_ITEMS 10
static void MenuItemText(uint8_t i, char *out)
{
	static const char *LIGHT[] = {"zawsze", "30 s", "2 min"};
	switch (i) {
	case 0: sprintf(out, "Prog: %u dB", gScanThresholdDb); break;
	case 1: sprintf(out, "Dzwiek: %s", gSettings.sound ? "tak" : "nie"); break;
	case 2: sprintf(out, "Jasnosc: %u", gSettings.brightness); break;
	case 3: sprintf(out, "Podswietl: %s", LIGHT[gSettings.lightTime]); break;
	case 4: case 5: case 6: case 7:
		sprintf(out, "Pasmo %s: %s", SCAN_BANDS[i - 4].name, (gScanBandMask & (1 << (i - 4))) ? "tak" : "nie");
		break;
	case 8: strcpy(out, "Wyczysc dziennik"); break;
	default: strcpy(out, "Wersja " VERSION_STRING); break;
	}
}

static void RenderMenu(void)
{
	const uint8_t top = menuSel < 7 ? 0 : menuSel - 6;
	for (uint8_t r = 0; r < 7 && top + r < MENU_ITEMS; r++) {
		char text[24];
		MenuItemText(top + r, text);
		sprintf(line, "%c%s", top + r == menuSel ? '>' : ' ', text);
		UI_PrintStringSmallNormal(line, 0, 0, r);
	}
}

void SCREENS_Render(void)
{
	static const char *TITLES[] = {"RID-K5 SKANER", "RID-K5 PASMO", "SZUKANIE", "DZIENNIK", "USTAWIENIA"};
	UI_DisplayClear();
	switch (screen) {
	case SCR_BAND: RenderBand(); break;
	case SCR_HUNT: RenderHunt(); break;
	case SCR_LOG:  RenderLog();  break;
	case SCR_MENU: RenderMenu(); break;
	default:       RenderMain(); break;
	}
	ST7565_BlitFullScreen();
	DrawStatusLine(TITLES[screen]);
}

// ---------------------------------------------------------------- ustawienia i klawisze

static void ApplySettings(void)
{
	gScanThresholdDb = gSettings.threshold;
	gScanBandMask = gSettings.bandMask;
	BACKLIGHT_SetBrightness(gBacklightForcedOff ? 0 : gSettings.brightness);
}

static void SaveSettings(void)
{
	gSettings.threshold = gScanThresholdDb;
	gSettings.locked = gScanLockedBand;
	gSettings.bandMask = gScanBandMask;
	STORE_SaveSettings();
}

static void SetBand(int8_t band)
{
	gScanHunt = false;
	SCAN_SelectBand(band);
	screen = band < 0 ? SCR_MAIN : SCR_BAND;
	SaveSettings();
}

static void MenuChange(int dir)
{
	switch (menuSel) {
	case 0:
		if (dir > 0 && gScanThresholdDb < 30) gScanThresholdDb++;
		if (dir < 0 && gScanThresholdDb > 3) gScanThresholdDb--;
		break;
	case 1: gSettings.sound ^= 1; break;
	case 2:
		gSettings.brightness = dir > 0 ? (gSettings.brightness % 10) + 1 : (gSettings.brightness > 1 ? gSettings.brightness - 1 : 10);
		break;
	case 3: gSettings.lightTime = (gSettings.lightTime + (dir > 0 ? 1 : 2)) % 3; break;
	case 4: case 5: case 6: case 7: {
		const uint8_t bit = 1 << (menuSel - 4);
		if ((gScanBandMask & ~bit) || !(gScanBandMask & bit))  // co najmniej jedno pasmo zostaje
			gScanBandMask ^= bit;
		if (gScanLockedBand < 0)
			SCAN_SelectBand(-1);
		break;
	}
	case 8: STORE_ClearLog(); break;
	default: break;
	}
	SaveSettings();
	ApplySettings();
}

static void OnKey(KEY_Code_t key)
{
	if (screen == SCR_MENU) {
		if (key == KEY_UP) menuSel = menuSel ? menuSel - 1 : MENU_ITEMS - 1;
		else if (key == KEY_DOWN) menuSel = (menuSel + 1) % MENU_ITEMS;
		else if (key == KEY_MENU || key == KEY_STAR) MenuChange(1);
		else if (key == KEY_F) MenuChange(-1);
		else if (key == KEY_EXIT) screen = gScanLockedBand >= 0 ? SCR_BAND : SCR_MAIN;
		return;
	}
	if (screen == SCR_LOG) {
		const uint8_t n = STORE_EventCount();
		if (key == KEY_UP && logTop) logTop--;
		else if (key == KEY_DOWN && logTop + 7 < n) logTop++;
		else if (key == KEY_EXIT || key == KEY_7) screen = gScanLockedBand >= 0 ? SCR_BAND : SCR_MAIN;
		return;
	}
	switch (key) {
	case KEY_0: SetBand(-1); break;
	case KEY_1: case KEY_2: case KEY_3: case KEY_4: SetBand(key - KEY_1); break;
	case KEY_5:
		if (screen == SCR_HUNT) {
			gScanHunt = false;
			screen = SCR_BAND;
		} else {
			if (gScanLockedBand < 0) {  // szukamy w paśmie z alarmem (albo bieżącym)
				const int8_t best = SCAN_BestAlarmBand();
				SCAN_SelectBand(best >= 0 ? best : gScanCurBand);
				SaveSettings();
			}
			gScanHunt = true;
			gScanHuntMax = 0;
			screen = SCR_HUNT;
		}
		break;
	case KEY_9: gScanHuntMax = 0; break;
	case KEY_7: logTop = 0; screen = SCR_LOG; break;
	case KEY_MENU: menuSel = 0; screen = SCR_MENU; break;
	case KEY_STAR: if (gScanThresholdDb < 30) gScanThresholdDb++; SaveSettings(); break;
	case KEY_F: if (gScanThresholdDb > 3) gScanThresholdDb--; SaveSettings(); break;
	case KEY_SIDE1: gSettings.sound ^= 1; SaveSettings(); break;
	case KEY_SIDE2: gBacklightForcedOff = !gBacklightForcedOff; ApplySettings(); break;
	case KEY_EXIT:
		if (screen == SCR_HUNT) { gScanHunt = false; screen = SCR_BAND; }
		else SetBand(-1);
		break;
	default: break;
	}
}

bool SCREENS_PollKeys(void)
{
	const KEY_Code_t key = KEYBOARD_Poll();  // PTT nie jest czytany - radio nie nadaje
	if (key != prevKey) {  // dwa jednakowe odczyty w odstępie 10 ms
		prevKey = key;
		return false;
	}
	if (key == heldKey)
		return false;
	heldKey = key;
	if (key == KEY_INVALID)
		return false;
	if (gSettings.sound)
		SOUND_Click();
	OnKey(key);
	SCREENS_Render();
	return true;
}

void SCREENS_OnWindow(void)
{
	if (screen == SCR_HUNT && gScanHuntSignal && gSettings.sound)
		SOUND_HuntTone(SCAN_RssiToDbm(gScanState[gScanCurBand].peak));
	if (screen != SCR_MENU && screen != SCR_LOG)
		SCREENS_Render();
}

void SCREENS_Init(void)
{
	EEPROM_ReadBuffer(0x1F40, batteryCal, 12);  // kalibracja baterii z fabryki (jak w egzumerze)
	if (batteryCal[3] == 0 || batteryCal[3] >= 5000)
		batteryCal[3] = 2200;
	for (int i = 0; i < 4; i++)
		SCREENS_UpdateBattery();
	ApplySettings();
	if (gSettings.locked >= 0) {
		SCAN_SelectBand(gSettings.locked);
		screen = SCR_BAND;
	}
}
