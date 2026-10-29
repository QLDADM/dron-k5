/* RID-K5: dedykowany firmware skanera łączy sterowania dronów dla Quansheng UV-K5.
 *
 * Wykrywa łącza ELRS / Crossfire / LRS na 433, 868 i 915 MHz oraz analogowe wideo FPV
 * na 1,2 GHz (po wzorcu RSSI - bez dekodowania), pokazuje alarmy, wodospad kanałów,
 * tryb szukania aparatury/pilota i dziennik alarmów; przez kabel programujący raportuje
 * do agenta RID Decoder (linie DD). Radio z tym oprogramowaniem nie nadaje.
 *
 * Sterowniki sprzętu pochodzą z egzumer/uv-k5-firmware-custom (Apache 2.0).
 * Powrót do egzumera lub oryginału: PTT przy włączaniu i wgranie innego pliku.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app/link.h"
#include "app/scan.h"
#include "app/screens.h"
#include "app/sound.h"
#include "app/store.h"
#include "board.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "driver/uart.h"
#include "misc.h"
#include "settings.h"

EEPROM_Config_t gEeprom = {ROGER_MODE_OFF};  // zgodność z driver/bk4819.c (nie nadajemy)
volatile bool   gNextTimeslice;

static const uint16_t LIGHT_TIMEOUT_S[] = {0, 30, 120};
static uint32_t lastActivityS;
static bool     alarmActive;

void SystickHandler(void);
void SystickHandler(void)  // co 10 ms
{
	gNextTimeslice = true;
}

void _putchar(char c);
void _putchar(char c)
{
	UART_Send((uint8_t *)&c, 1);
}

static void WakeBacklight(void)
{
	lastActivityS = gUptimeS;
	if (!gBacklightForcedOff)
		BACKLIGHT_SetBrightness(gSettings.brightness);
}

// alarm: początek / koniec wykrycia -> dziennik, dźwięk, czerwona dioda, podświetlenie
static void CheckAlarm(void)
{
	const bool any = SCAN_AnyAlarm();
	const int8_t best = SCAN_BestAlarmBand();
	if (any && best >= 0)
		STORE_EventUpdate(SCAN_RssiToDbm(gScanState[best].peak));
	if (any == alarmActive)
		return;
	alarmActive = any;
	BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, any);
	if (any) {
		STORE_EventStart(best, SCAN_RssiToDbm(gScanState[best].peak));
		WakeBacklight();
		if (gSettings.sound)
			SOUND_Alarm();
	} else {
		STORE_EventEnd();
	}
}

void Main(void);
void Main(void)
{
	SYSCON_DEV_CLK_GATE = 0
		| SYSCON_DEV_CLK_GATE_GPIOA_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_GPIOB_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_GPIOC_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_UART1_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_SPI0_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_SARADC_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_CRC_BITS_ENABLE
		| SYSCON_DEV_CLK_GATE_PWM_PLUS0_BITS_ENABLE;

	SYSTICK_Init();
	BOARD_Init();
	UART_Init();
	BK4819_Init();

	// dostrojenie kwarcu odbiornika z kalibracji fabrycznej (jak w egzumerze)
	int16_t xtal;
	EEPROM_ReadBuffer(0x1F88, &xtal, 2);
	if (xtal >= -1000 && xtal <= 1000)
		BK4819_WriteRegister(BK4819_REG_3B, 22656 + xtal);

	STORE_Load();
	gScanLockedBand = gSettings.locked;
	SCAN_Init();
	SCREENS_Init();
	WakeBacklight();

	// klawisz trzymany przy włączaniu nie może od razu niczego zmienić
	while (KEYBOARD_Poll() != KEY_INVALID)
		SYSTEM_DelayMs(10);

	SCREENS_Render();
	uint16_t ticks = 0;

	while (true) {
		SCAN_SampleNext();
		if (!gNextTimeslice)
			continue;
		gNextTimeslice = false;

		if (++ticks >= 100) {  // co sekundę
			ticks = 0;
			gUptimeS++;
			SCREENS_UpdateBattery();
			const uint16_t timeout = LIGHT_TIMEOUT_S[gSettings.lightTime];
			if (timeout && gUptimeS - lastActivityS >= timeout)
				BACKLIGHT_SetBrightness(0);
		}

		LINK_Poll();
		if (SCREENS_PollKeys())
			WakeBacklight();

		if (SCAN_Tick10ms()) {
			LINK_ReportWindow();
			CheckAlarm();
			SCREENS_OnWindow();
		}
	}
}
