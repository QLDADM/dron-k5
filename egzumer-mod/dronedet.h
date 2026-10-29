/* Drone link detector for UV-K5 (egzumer firmware mod)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef APP_DRONEDET_H
#define APP_DRONEDET_H

// Energy based detector of drone control links (ELRS, Crossfire, LRS on
// 433/868/915 MHz) and analog FPV video around 1.2 GHz.
// It only says "something hops like a drone link here", it does not decode
// anything. Remote ID (2.4 GHz Wi-Fi/BLE) is out of BK4819 range.
void APP_RunDroneDetector(void);

#endif
