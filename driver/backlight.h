/* Podświetlenie ekranu i klawiatury (PWM), uproszczone względem egzumera. */
#ifndef DRIVER_BACKLIGHT_H
#define DRIVER_BACKLIGHT_H

#include <stdbool.h>
#include <stdint.h>

void BACKLIGHT_InitHardware(void);
void BACKLIGHT_SetBrightness(uint8_t level);  // 0 (wył.) .. 10 (max)

#endif
