/* Drone link detector for UV-K5 (egzumer firmware mod)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * How it works:
 *  - each band is swept bin by bin, RSSI of every bin is compared with its
 *    own learned noise floor,
 *  - FHSS links (ELRS, Crossfire, LRS) light up many different bins for a
 *    short time; constant carriers are slowly absorbed by the floor and
 *    single channel users (LoRaWAN, meters, PMR) light up only a few bins,
 *  - a band whose activity looks like hopping for several windows in a row
 *    raises its score; above DD_SCORE_ALARM the screen shows "DRON!",
 *  - the 1.2 GHz band uses a continuous carrier rule (analog FPV video).
 * The radio never transmits here, PTT is ignored.
 *
 * With ENABLE_UART every band window is reported on the programming cable
 * (38400 8N1) as one text line, read by rid-decoder agent (--uvk5 COMx):
 *   DD,<band>,<score>,<distinct bins>,<peak dBm>,<alarm 0/1>\r\n
 */

#include <string.h>

#include "app/dronedet.h"
#include "audio.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/backlight.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "driver/st7565.h"
#include "driver/system.h"
#include "driver/systick.h"
#ifdef ENABLE_UART
	#include "driver/uart.h"
#endif
#include "external/printf/printf.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "ui/helper.h"
#include "ui/ui.h"

#define DD_WINDOW_TICKS   50    // 10 ms ticks per band window (0.5 s)
#define DD_REMIND_TICKS   1000  // repeat alarm beep every 10 s
#define DD_WARMUP_WINDOWS 2     // windows used only for learning the floor
#define DD_SCORE_MAX      12
#define DD_SCORE_ALARM    6
#define DD_SCORE_CLEAR    2
#define DD_SCORE_MAYBE    3
#define DD_MIN_DISTINCT   3     // bins with activity in one window
#define DD_MIN_HITS       4     // samples above floor in one window
#define DD_CARRIER_PCT    70    // video: bin busy in >= 70% of samples
// BK4819 noise floor varies by >10 dB across 1.08-1.28 GHz, so a relative
// rule alone fires on noise; real video close by is far above this level
#define DD_CARRIER_MIN_DBM -105
#define DD_SETTLE_US      300

// widest RX filter, same value spectrum uses for 25 kHz listening
#define DD_REG43_WIDE     0b0011011000101000

typedef struct {
	const char *name;
	uint32_t    fStart;   // 10 Hz units
	uint32_t    step;     // 10 Hz units
	uint8_t     count;
	uint8_t     offset;   // first bin in the shared arrays
	bool        carrier;  // continuous carrier rule instead of hopping rule
} Band_t;

static const Band_t bands[] = {
	{"433",  43200000,   20000, 20,  0, false}, // 432.0 - 435.8 MHz, 200 kHz
	{"868",  86300000,   25000, 30, 20, false}, // 863.0 - 870.25 MHz, 250 kHz
	{"915",  90200000,   65000, 40, 50, false}, // 902.0 - 927.35 MHz, 650 kHz
	{"1G2", 108000000, 2000000, 11, 90, true }, // 1080 - 1280 MHz, 20 MHz
};

#define BAND_COUNT ARRAY_SIZE(bands)
#define BIN_COUNT  101

typedef struct {
	uint16_t ref;         // lowest floor in band (carrier rule)
	uint16_t peak;        // strongest sample of last window
	uint16_t peakNow;
	uint8_t  distinct;    // bins with activity in last window
	uint8_t  score;
	uint8_t  windows;
	bool     primed;      // floor initialised by a first sweep
	bool     alarm;
} BandState_t;

static uint16_t    binFloor[BIN_COUNT];
static uint8_t     binHits[BIN_COUNT];
static uint8_t     binSamples[BIN_COUNT];
static BandState_t state[BAND_COUNT];

static uint8_t  curBand;
static uint8_t  curBin;
static int8_t   lockedBand = -1;  // -1 = cycle all bands
static uint8_t  thresholdDb = 8;
static uint8_t  windowTicks;
static uint16_t remindTicks;
static bool     soundOn = true;
static bool     backlightOn;
static bool     alarmActive;
static bool     running;
static KEY_Code_t prevKey = KEY_INVALID;
static KEY_Code_t heldKey = KEY_INVALID;

static const BK4819_REGISTER_t registersToSave[] = {
	BK4819_REG_30,
	BK4819_REG_37,
	BK4819_REG_3D,
	BK4819_REG_43,
	BK4819_REG_47,
	BK4819_REG_48,
	BK4819_REG_7E,
};

static uint16_t registersStack[ARRAY_SIZE(registersToSave)];

static void BackupRegisters(void)
{
	for (unsigned i = 0; i < ARRAY_SIZE(registersToSave); i++)
		registersStack[i] = BK4819_ReadRegister(registersToSave[i]);
}

static void RestoreRegisters(void)
{
	for (unsigned i = 0; i < ARRAY_SIZE(registersToSave); i++)
		BK4819_WriteRegister(registersToSave[i], registersStack[i]);
}

static void SetF(uint32_t f)
{
	BK4819_SetFrequency(f);
	BK4819_PickRXFilterPathBasedOnFrequency(f);
	const uint16_t reg = BK4819_ReadRegister(BK4819_REG_30);
	BK4819_WriteRegister(BK4819_REG_30, 0);
	BK4819_WriteRegister(BK4819_REG_30, reg);
}

static uint16_t MeasureRssi(uint32_t f)
{
	SetF(f);
	SYSTICK_DelayUs(DD_SETTLE_US);
	for (uint8_t i = 0; i < 50 && BK4819_GetGlitchIndicator() >= 255; i++)
		SYSTICK_DelayUs(100);
	return BK4819_GetRSSI();
}

static int RssiToDbm(uint16_t rssi)
{
	return rssi / 2 - 160;
}

// move v towards target by 1/2^shift of the distance, never overshooting
static uint16_t Approach(uint16_t v, uint16_t target, uint8_t shift)
{
	if (target > v)
		return v + ((target - v) >> shift);
	return v - ((v - target + (1u << shift) - 1) >> shift);
}

static void ClearWindow(uint8_t band)
{
	const Band_t *b = &bands[band];
	memset(&binHits[b->offset], 0, b->count);
	memset(&binSamples[b->offset], 0, b->count);
	state[band].peakNow = 0;
}

static void SampleBin(void)
{
	const Band_t *b   = &bands[curBand];
	BandState_t  *s   = &state[curBand];
	const uint8_t idx = b->offset + curBin;
	const uint16_t rssi = MeasureRssi(b->fStart + b->step * curBin);
	const uint16_t thr  = thresholdDb * 2;

	if (!s->primed) {
		binFloor[idx] = rssi;
	} else if (b->carrier) {
		if (rssi > (uint32_t)s->ref + thr && RssiToDbm(rssi) >= DD_CARRIER_MIN_DBM)
			binHits[idx]++;
		binFloor[idx] = Approach(binFloor[idx], rssi, 2);
	} else if (rssi > binFloor[idx] + thr) {
		binHits[idx]++;
		binFloor[idx]++;   // a constant carrier gets absorbed after a while
	} else {
		binFloor[idx] = Approach(binFloor[idx], rssi, 3);
	}

	if (binSamples[idx] < 255)
		binSamples[idx]++;
	if (rssi > s->peakNow)
		s->peakNow = rssi;

	if (++curBin >= b->count) {
		curBin = 0;
		s->primed = true;
	}
}

static void EndWindow(void)
{
	const Band_t *b = &bands[curBand];
	BandState_t  *s = &state[curBand];
	uint8_t  distinct = 0;
	uint16_t hits     = 0;
	uint16_t ref      = 0xFFFF;
	bool     carrier  = false;

	for (uint8_t i = 0; i < b->count; i++) {
		const uint8_t idx = b->offset + i;
		if (binHits[idx])
			distinct++;
		hits += binHits[idx];
		if (binSamples[idx] && binHits[idx] * 100u >= binSamples[idx] * (unsigned)DD_CARRIER_PCT)
			carrier = true;
		if (binFloor[idx] < ref)
			ref = binFloor[idx];
	}

	s->ref      = ref;
	s->distinct = distinct;
	s->peak     = s->peakNow;
	ClearWindow(curBand);

	if (s->windows < DD_WARMUP_WINDOWS) {
		s->windows++;
	} else {
		const bool active = b->carrier ? carrier
		                               : (distinct >= DD_MIN_DISTINCT && hits >= DD_MIN_HITS);
		// silence drops the score faster than sporadic activity
		const uint8_t dec = distinct ? 1 : 2;
		if (active)
			s->score = s->score + 3 > DD_SCORE_MAX ? DD_SCORE_MAX : s->score + 3;
		else
			s->score = s->score > dec ? s->score - dec : 0;

		if (s->score >= DD_SCORE_ALARM)
			s->alarm = true;
		else if (s->score <= DD_SCORE_CLEAR)
			s->alarm = false;

#ifdef ENABLE_UART
		char line[32];
		const int len = sprintf(line, "DD,%s,%u,%u,%d,%u\r\n", b->name, s->score,
		                        distinct, RssiToDbm(s->peak), s->alarm);
		UART_Send(line, len);
#endif
	}

	if (lockedBand < 0) {
		curBand = (curBand + 1) % BAND_COUNT;
		curBin  = 0;
		ClearWindow(curBand);
	}
}

static void Beep(BEEP_Type_t type)
{
	AUDIO_PlayBeep(type);
	AUDIO_AudioPathOff();
	BK4819_SetAF(BK4819_AF_MUTE);
	BK4819_WriteRegister(BK4819_REG_43, DD_REG43_WIDE);
}

static void UpdateAlarm(void)
{
	bool any = false;
	for (uint8_t i = 0; i < BAND_COUNT; i++)
		any |= state[i].alarm;

	if (any == alarmActive)
		return;

	alarmActive = any;
	remindTicks = 0;
	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, any);
	if (any && soundOn)
		Beep(BEEP_880HZ_60MS_TRIPLE_BEEP);
}

static void DrawScoreBar(uint8_t line, uint8_t score)
{
	for (uint8_t i = 0; i < DD_SCORE_MAX; i++) {
		const uint8_t x = 80 + i * 4;
		const uint8_t v = i < score ? 0b00111110 : 0b00100000;
		gFrameBuffer[line][x]     |= v;
		gFrameBuffer[line][x + 1] |= v;
		gFrameBuffer[line][x + 2] |= v;
	}
	gFrameBuffer[line][80 + DD_SCORE_ALARM * 4 - 1] |= 0b01111111;
}

static void Render(void)
{
	char String[24];
	uint8_t best = 0;
	int8_t  bestBand = -1;

	for (uint8_t i = 0; i < BAND_COUNT; i++) {
		if (state[i].alarm && state[i].score >= best) {
			best = state[i].score;
			bestBand = i;
		}
	}

	UI_DisplayClear();

	if (bestBand >= 0) {
		sprintf(String, "DRON %s", bands[bestBand].name);
	} else {
		strcpy(String, "BRAK");
		for (uint8_t i = 0; i < BAND_COUNT; i++)
			if (state[i].score >= DD_SCORE_MAYBE)
				strcpy(String, "DRON?");
	}
	UI_PrintString(String, 0, 127, 0, 10);

	for (uint8_t i = 0; i < BAND_COUNT; i++) {
		const BandState_t *s = &state[i];
		const char mark = (i == curBand) ? (lockedBand >= 0 ? '*' : '>') : ' ';
		if (s->windows)
			sprintf(String, "%c%s%4d %2u", mark, bands[i].name, RssiToDbm(s->peak), s->distinct);
		else
			sprintf(String, "%c%s ---  -", mark, bands[i].name);
		UI_PrintStringSmallNormal(String, 0, 0, 2 + i);
		DrawScoreBar(2 + i, s->score);
	}

	sprintf(String, "%udB %s %s", thresholdDb,
	        lockedBand < 0 ? "WSZ" : bands[lockedBand].name,
	        soundOn ? "DZW" : "CICHO");
	UI_PrintStringSmallNormal(String, 0, 0, 6);
	ST7565_BlitFullScreen();

	BOARD_ADC_GetBatteryInfo(&gBatteryVoltages[gBatteryCheckCounter++ % 4], &gBatteryCurrent);
	const uint16_t voltage = (gBatteryVoltages[0] + gBatteryVoltages[1] +
	                          gBatteryVoltages[2] + gBatteryVoltages[3]) /
	                         4 * 760 / gBatteryCalibration[3];

	memset(gStatusLine, 0, sizeof(gStatusLine));
	UI_PrintStringSmallBufferNormal("DRON DETEKTOR", gStatusLine);
	sprintf(String, "%u%%", BATTERY_VoltsToPercent(voltage));
	UI_PrintStringSmallBufferNormal(String, gStatusLine + 98);
	ST7565_BlitStatusLine();
}

static void SelectBand(int8_t band)
{
	ClearWindow(curBand);
	lockedBand  = band;
	curBand     = band < 0 ? 0 : band;
	curBin      = 0;
	windowTicks = 0;
	ClearWindow(curBand);
}

static void OnKey(KEY_Code_t key)
{
	switch (key) {
	case KEY_EXIT:
		running = false;
		break;
	case KEY_0:
		SelectBand(-1);
		break;
	case KEY_1:
	case KEY_2:
	case KEY_3:
	case KEY_4:
		SelectBand(key - KEY_1);
		break;
	case KEY_STAR:
		if (thresholdDb < 30)
			thresholdDb++;
		break;
	case KEY_F:
		if (thresholdDb > 3)
			thresholdDb--;
		break;
	case KEY_MENU:
		soundOn = !soundOn;
		break;
	case KEY_SIDE2:
		backlightOn = !backlightOn;
		if (backlightOn)
			BACKLIGHT_TurnOn();
		else
			BACKLIGHT_TurnOff();
		break;
	default:
		break;
	}
}

static void HandleKeys(void)
{
	const KEY_Code_t key = KEYBOARD_Poll();
	if (key != prevKey) {   // debounce: two equal reads 10 ms apart
		prevKey = key;
		return;
	}
	if (key == heldKey)
		return;
	heldKey = key;
	if (key == KEY_INVALID)
		return;
	OnKey(key);
	Render();
}

void APP_RunDroneDetector(void)
{
	const bool audioWasOn = GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH);

	BackupRegisters();
	AUDIO_AudioPathOff();
	BK4819_SetAF(BK4819_AF_MUTE);
	RADIO_SetupAGC(false, false);
	BK4819_WriteRegister(BK4819_REG_43, DD_REG43_WIDE);
	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
	BACKLIGHT_TurnOn();
	backlightOn = true;

	memset(state, 0, sizeof(state));
	memset(binHits, 0, sizeof(binHits));
	memset(binSamples, 0, sizeof(binSamples));
	alarmActive = false;
	windowTicks = 0;
	curBin      = 0;
	curBand     = lockedBand < 0 ? 0 : lockedBand;

	// the key that started us is still pressed, wait for its release
	while (KEYBOARD_Poll() != KEY_INVALID)
		SYSTEM_DelayMs(10);
	prevKey = heldKey = KEY_INVALID;

	running = true;
	Render();

	while (running) {
		SampleBin();

		if (!gNextTimeslice)
			continue;
		gNextTimeslice = false;

		HandleKeys();

		if (++windowTicks >= DD_WINDOW_TICKS) {
			windowTicks = 0;
			EndWindow();
			UpdateAlarm();
			Render();
		}

		if (alarmActive && soundOn && ++remindTicks >= DD_REMIND_TICKS) {
			remindTicks = 0;
			Beep(BEEP_880HZ_200MS);
		}
	}

	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
	RestoreRegisters();
	SetF(gRxVfo->pRX->Frequency);
	if (audioWasOn)
		AUDIO_AudioPathOn();

	gFlagReconfigureVfos  = true;
	gRequestDisplayScreen = DISPLAY_MAIN;
}
