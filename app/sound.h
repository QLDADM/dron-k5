/* DRON-K5: dźwięki z głośnika radia (generator tonu BK4819 - bez nadawania). */
#ifndef APP_SOUND_H
#define APP_SOUND_H

#include <stdint.h>

void SOUND_Tone(uint16_t hz, uint16_t ms);
void SOUND_Alarm(void);        // nowe wykrycie: trzy tony
void SOUND_Click(void);        // potwierdzenie klawisza
void SOUND_HuntTone(int dbm);  // tryb szukania: im silniejszy sygnał, tym wyższy ton

#endif
