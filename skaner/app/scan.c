/* DRON-K5: silnik skanowania (opis w scan.h). Algorytm jak w trybie DRON DETEKTOR dla egzumera. */
#include <string.h>

#include "app/scan.h"
#include "driver/bk4819-regs.h"
#include "driver/bk4819.h"
#include "driver/systick.h"
#include "audio.h"

#define WINDOW_TICKS      50    // 10 ms na okno pasma (0,5 s)
#define HUNT_TICKS        25    // tryb szukania: 0,25 s - szybsza reakcja przy chodzeniu
#define WARMUP_WINDOWS    2     // pierwsze okna tylko uczą poziomu szumu
#define SCORE_CLEAR       2
#define MIN_DISTINCT      3     // kanały z aktywnością w oknie
#define MIN_HITS          4     // próbki ponad szumem w oknie
#define CARRIER_PCT       70    // wideo: kanał zajęty w >= 70% próbek
#define CARRIER_MIN_DBM   -105  // szum BK4819 na 1,08-1,28 GHz różni się o >10 dB między kanałami
#define SETTLE_US         300
#define REG43_WIDE        0b0011011000101000  // najszerszy filtr odbiornika (25 kHz)

const ScanBand_t SCAN_BANDS[SCAN_BAND_COUNT] = {
	{"433",  43200000,   20000, 20,  0, false}, // 432,0 - 435,8 MHz, 200 kHz
	{"868",  86300000,   25000, 30, 20, false}, // 863,0 - 870,25 MHz, 250 kHz
	{"915",  90200000,   65000, 40, 50, false}, // 902,0 - 927,35 MHz, 650 kHz
	{"1G2", 108000000, 2000000, 11, 90, true }, // 1080 - 1280 MHz, 20 MHz
};

ScanBandState_t gScanState[SCAN_BAND_COUNT];
uint8_t  gScanLastHits[SCAN_BIN_COUNT];
uint8_t  gScanCurBand;
uint8_t  gScanLastBand;
int8_t   gScanLockedBand = -1;
uint8_t  gScanBandMask = 0x0F;
uint8_t  gScanThresholdDb = 8;
bool     gScanHunt;
bool     gScanHuntSignal;
uint16_t gScanHuntMax;
uint8_t  gScanWaterfall[SCAN_WF_ROWS][8];
uint8_t  gScanWfCount;

static uint16_t binFloor[SCAN_BIN_COUNT];
static uint8_t  binHits[SCAN_BIN_COUNT];
static uint8_t  binSamples[SCAN_BIN_COUNT];
static uint8_t  curBin;
static uint8_t  windowTicks;

int SCAN_RssiToDbm(uint16_t rssi)
{
	return rssi / 2 - 160;
}

// przesuwa v w stronę target o 1/2^shift odległości, bez przeskoczenia
static uint16_t Approach(uint16_t v, uint16_t target, uint8_t shift)
{
	if (target > v)
		return v + ((target - v) >> shift);
	return v - ((v - target + (1u << shift) - 1) >> shift);
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
	SYSTICK_DelayUs(SETTLE_US);
	for (uint8_t i = 0; i < 50 && BK4819_GetGlitchIndicator() >= 255; i++)
		SYSTICK_DelayUs(100);
	return BK4819_GetRSSI();
}

static uint8_t NextBand(uint8_t band);

void SCAN_RestoreRx(void)
{
	AUDIO_AudioPathOff();
	BK4819_SetAF(BK4819_AF_MUTE);
	BK4819_WriteRegister(BK4819_REG_43, REG43_WIDE);
}

void SCAN_Init(void)
{
	// tylko odbiór: tor RX, AGC, szeroki filtr, wyciszony dźwięk; nadajnik nie jest nigdy włączany
	BK4819_WriteRegister(BK4819_REG_3F, 0);  // bez przerwań (squelch, CTCSS...)
	BK4819_RX_TurnOn();
	BK4819_SetAGC(true);
	BK4819_InitAGC(false);
	BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);
	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
	SCAN_RestoreRx();
	memset(gScanState, 0, sizeof(gScanState));
	memset(binHits, 0, sizeof(binHits));
	memset(binSamples, 0, sizeof(binSamples));
	curBin = 0;
	windowTicks = 0;
	gScanCurBand = gScanLockedBand >= 0 ? gScanLockedBand : NextBand(SCAN_BAND_COUNT - 1);
}

static void ClearWindow(uint8_t band)
{
	const ScanBand_t *b = &SCAN_BANDS[band];
	memset(&binHits[b->offset], 0, b->count);
	memset(&binSamples[b->offset], 0, b->count);
	gScanState[band].peakNow = 0;
}

void SCAN_SampleNext(void)
{
	const ScanBand_t *b  = &SCAN_BANDS[gScanCurBand];
	ScanBandState_t  *s  = &gScanState[gScanCurBand];
	const uint8_t idx    = b->offset + curBin;
	const uint16_t rssi  = MeasureRssi(b->fStart + b->step * curBin);
	const uint16_t thr   = gScanThresholdDb * 2;

	if (!s->primed) {
		binFloor[idx] = rssi;
	} else if (b->carrier) {
		if (rssi > (uint32_t)s->ref + thr && SCAN_RssiToDbm(rssi) >= CARRIER_MIN_DBM)
			binHits[idx]++;
		binFloor[idx] = Approach(binFloor[idx], rssi, 2);
	} else if (rssi > binFloor[idx] + thr) {
		binHits[idx]++;
		binFloor[idx]++;   // stała nośna zostaje po chwili wchłonięta
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

static void PushWaterfall(const ScanBand_t *b)
{
	memmove(gScanWaterfall[1], gScanWaterfall[0], sizeof(gScanWaterfall[0]) * (SCAN_WF_ROWS - 1));
	memset(gScanWaterfall[0], 0, sizeof(gScanWaterfall[0]));
	for (uint8_t i = 0; i < b->count && i < 64; i++)
		if (binHits[b->offset + i])
			gScanWaterfall[0][i >> 3] |= 1 << (i & 7);
	if (gScanWfCount < SCAN_WF_ROWS)
		gScanWfCount++;
}

static uint8_t NextBand(uint8_t band)
{
	for (uint8_t i = 1; i <= SCAN_BAND_COUNT; i++) {
		const uint8_t n = (band + i) % SCAN_BAND_COUNT;
		if (gScanBandMask & (1 << n))
			return n;
	}
	return band;
}

static void EndWindow(void)
{
	const ScanBand_t *b = &SCAN_BANDS[gScanCurBand];
	ScanBandState_t  *s = &gScanState[gScanCurBand];
	uint8_t  distinct = 0;
	uint16_t hits     = 0;
	uint16_t ref      = 0xFFFF;
	bool     carrier  = false;

	for (uint8_t i = 0; i < b->count; i++) {
		const uint8_t idx = b->offset + i;
		if (binHits[idx])
			distinct++;
		hits += binHits[idx];
		if (binSamples[idx] && binHits[idx] * 100u >= binSamples[idx] * (unsigned)CARRIER_PCT)
			carrier = true;
		if (binFloor[idx] < ref)
			ref = binFloor[idx];
	}

	s->ref      = ref;
	s->distinct = distinct;
	s->peak     = s->peakNow;
	memcpy(&gScanLastHits[b->offset], &binHits[b->offset], b->count);
	gScanHuntSignal = s->primed && s->peak > (uint32_t)ref + gScanThresholdDb * 2;
	if (gScanHunt && gScanHuntSignal && s->peak > gScanHuntMax)
		gScanHuntMax = s->peak;
	if (gScanLockedBand == gScanCurBand)
		PushWaterfall(b);
	ClearWindow(gScanCurBand);

	if (s->windows < WARMUP_WINDOWS) {
		s->windows++;
	} else {
		const bool active = b->carrier ? carrier : (distinct >= MIN_DISTINCT && hits >= MIN_HITS);
		const uint8_t dec = distinct ? 1 : 2;  // cisza obniża wynik szybciej niż sporadyczna aktywność
		if (active)
			s->score = s->score + 3 > SCAN_SCORE_MAX ? SCAN_SCORE_MAX : s->score + 3;
		else
			s->score = s->score > dec ? s->score - dec : 0;

		if (s->score >= SCAN_SCORE_ALARM)
			s->alarm = true;
		else if (s->score <= SCORE_CLEAR)
			s->alarm = false;
	}

	gScanLastBand = gScanCurBand;
	if (gScanLockedBand < 0) {
		gScanCurBand = NextBand(gScanCurBand);
		curBin = 0;
		ClearWindow(gScanCurBand);
	}
}

bool SCAN_Tick10ms(void)
{
	if (++windowTicks < (gScanHunt ? HUNT_TICKS : WINDOW_TICKS))
		return false;
	windowTicks = 0;
	EndWindow();
	return true;
}

void SCAN_SelectBand(int8_t band)
{
	ClearWindow(gScanCurBand);
	gScanLockedBand = band;
	gScanCurBand    = band < 0 ? NextBand(SCAN_BAND_COUNT - 1) : band;
	curBin          = 0;
	windowTicks     = 0;
	gScanWfCount    = 0;
	memset(gScanWaterfall, 0, sizeof(gScanWaterfall));
	ClearWindow(gScanCurBand);
}

bool SCAN_AnyAlarm(void)
{
	for (uint8_t i = 0; i < SCAN_BAND_COUNT; i++)
		if (gScanState[i].alarm)
			return true;
	return false;
}

int8_t SCAN_BestAlarmBand(void)
{
	int8_t best = -1;
	for (uint8_t i = 0; i < SCAN_BAND_COUNT; i++)
		if (gScanState[i].alarm && (best < 0 || gScanState[i].score >= gScanState[best].score))
			best = i;
	return best;
}
