#ifndef DUMP_TRUCK_SOUNDS_H
#define DUMP_TRUCK_SOUNDS_H

#include <Arduino.h>
#include "sound_config.h"

// Sample files for Dump Truck
// These would be arrays containing raw PCM audio data
// We'll include sample files from the Rc_Engine_Sound_ESP32-master project

// Idle sound - just a placeholder for now, you would replace with actual sound data
extern const uint8_t dumpTruckIdleData[];
extern const uint32_t dumpTruckIdleLen;

// Starting sound
extern const uint8_t dumpTruckStartData[];
extern const uint32_t dumpTruckStartLen;

// Revving sound
extern const uint8_t dumpTruckRevData[];
extern const uint32_t dumpTruckRevLen;

// Horn sound
extern const uint8_t dumpTruckHornData[];
extern const uint32_t dumpTruckHornLen;

// You can add more sounds as needed

// Function to load this vehicle's sounds into the sound system
void loadDumpTruckSounds();

#endif // DUMP_TRUCK_SOUNDS_H
