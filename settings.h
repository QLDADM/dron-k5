/* Zgodność z driver/bk4819.c z egzumera (BK4819_PlayRoger czyta gEeprom.ROGER).
 * Skaner nigdy nie nadaje, więc roger jest zawsze wyłączony. */
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

enum ROGER_Mode_t { ROGER_MODE_OFF = 0, ROGER_MODE_ROGER, ROGER_MODE_MDC };

typedef struct {
	uint8_t ROGER;
} EEPROM_Config_t;

extern EEPROM_Config_t gEeprom;

#endif
