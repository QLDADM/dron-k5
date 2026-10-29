/* Tor audio (głośnik) - tylko przełącznik, jak w egzumerze. */
#ifndef AUDIO_H
#define AUDIO_H

#include "bsp/dp32g030/gpio.h"
#include "driver/gpio.h"

static inline void AUDIO_AudioPathOn(void) { GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH); }
static inline void AUDIO_AudioPathOff(void) { GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_AUDIO_PATH); }

#endif
