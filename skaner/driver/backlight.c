/* Podświetlenie: sprzętowa część z egzumera (Apache 2.0), bez zależności od jego ustawień. */
#include "backlight.h"
#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/pwmplus.h"

void BACKLIGHT_InitHardware(void)
{
	const uint32_t PWM_FREQUENCY_HZ = 1000;
	PWM_PLUS0_CLKSRC |= ((48000000 / 1024 / PWM_FREQUENCY_HZ) << 16);
	PWM_PLUS0_PERIOD = 1023;

	PORTCON_PORTB_SEL0 &= ~PORTCON_PORTB_SEL0_B6_MASK;
	PORTCON_PORTB_SEL0 |= PORTCON_PORTB_SEL0_B6_BITS_PWMP0_CH0;

	PWM_PLUS0_GEN = PWMPLUS_GEN_CH0_OE_BITS_ENABLE | PWMPLUS_GEN_CH0_OUTINV_BITS_ENABLE;
	PWM_PLUS0_CFG = PWMPLUS_CFG_CNT_REP_BITS_ENABLE | PWMPLUS_CFG_COUNTER_EN_BITS_ENABLE;
}

void BACKLIGHT_SetBrightness(uint8_t level)
{
	if (level > 10)
		level = 10;
	PWM_PLUS0_CH0_COMP = (1 << level) - 1;  // jak w egzumerze: skala wykładnicza
}
