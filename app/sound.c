/* RID-K5: dźwięki. Ton powstaje w torze audio BK4819 (jak sygnały klawiszy w egzumerze);
 * wzmacniacz mocy nadajnika nie jest włączany. Po tonie odbiornik wraca do skanowania. */
#include "app/scan.h"
#include "app/sound.h"
#include "audio.h"
#include "driver/bk4819.h"
#include "driver/system.h"

void SOUND_Tone(uint16_t hz, uint16_t ms)
{
	BK4819_PlayTone(hz, true);
	SYSTEM_DelayMs(2);
	AUDIO_AudioPathOn();
	BK4819_ExitTxMute();
	SYSTEM_DelayMs(ms);
	BK4819_EnterTxMute();
	AUDIO_AudioPathOff();
	BK4819_TurnsOffTones_TurnsOnRX();
	SCAN_RestoreRx();
}

void SOUND_Alarm(void)
{
	for (int i = 0; i < 3; i++) {
		SOUND_Tone(880, 70);
		SYSTEM_DelayMs(50);
	}
}

void SOUND_Click(void)
{
	SOUND_Tone(1200, 15);
}

void SOUND_HuntTone(int dbm)
{
	if (dbm < -125)
		dbm = -125;
	if (dbm > -40)
		dbm = -40;
	SOUND_Tone(300 + (dbm + 125) * 20, 40);  // -125 dBm: 300 Hz ... -40 dBm: 2000 Hz
}
