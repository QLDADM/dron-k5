/* Wspólne drobiazgi dla sterowników przeniesionych z egzumera (skaner nie ma reszty radia). */
#ifndef MISC_H
#define MISC_H

#include <stdbool.h>
#include <stdint.h>

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

extern volatile bool gNextTimeslice;  // co 10 ms (SystickHandler)

#endif
