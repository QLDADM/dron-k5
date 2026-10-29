/* DRON-K5: ekrany i klawisze.
 *   0          wszystkie pasma (ekran główny)
 *   1..4       jedno pasmo: 433 / 868 / 915 / 1,2 GHz z wodospadem kanałów
 *   5          tryb szukania (namierzanie aparatury/pilota po sile sygnału), ponownie - koniec
 *   9          tryb szukania: zeruj maksimum
 *   7          dziennik alarmów (strzałki - przewijanie)
 *   MENU       ustawienia (strzałki - wybór, MENU / * / F - zmiana)
 *   * / F      próg czułości +1 / -1 dB
 *   boczny 1   dźwięk wł./wył.,  boczny 2  podświetlenie wł./wył.
 *   EXIT       powrót do ekranu głównego
 * PTT nic nie robi - radio z tym oprogramowaniem nie nadaje.
 */
#ifndef APP_SCREENS_H
#define APP_SCREENS_H

#include <stdbool.h>
#include <stdint.h>

void SCREENS_Init(void);
void SCREENS_Render(void);
bool SCREENS_PollKeys(void);   // true, gdy naciśnięto klawisz (budzi podświetlenie)
void SCREENS_OnWindow(void);   // po każdym oknie pomiaru
void SCREENS_UpdateBattery(void);

extern bool gBacklightForcedOff;

#endif
