/* DRON-K5: łącze z komputerem przez kabel programujący (38400 8N1).
 * Radio -> PC:
 *   DD,<pasmo>,<wynik>,<kanały>,<dBm>,<alarm 0/1>      po każdym oknie (jak w trybie egzumera)
 *   DL,<nr>,<pasmo>,<dBm>,<czas trwania s>,<czas>       wpis dziennika (po poleceniu LOG)
 *   DI,DRON-K5,<wersja>                                  przedstawienie się (po ID)
 * PC -> radio (linie tekstu):
 *   TIME <unix>   ustawia zegar dla dziennika
 *   LOG           wypisuje dziennik alarmów
 *   ID            przedstawienie się
 */
#ifndef APP_LINK_H
#define APP_LINK_H

void LINK_ReportWindow(void);  // linia DD dla pasma, którego okno właśnie się skończyło
void LINK_Poll(void);          // polecenia z komputera

#endif
