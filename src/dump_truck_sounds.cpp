#include "../include/sound/dump_truck_sounds.h"

// Sample placeholder data
// In a real implementation, these would be arrays of actual audio data
// generated from the Audio2Header.html tool

// Placeholder for idle sound (this would be a much larger array in reality)
const uint8_t dumpTruckIdleData[] = {
  128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159
  // ... many more values would go here
};
const uint32_t dumpTruckIdleLen = sizeof(dumpTruckIdleData);

// Placeholder for start sound
const uint8_t dumpTruckStartData[] = {
  128, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200,
  200, 195, 190, 185, 180, 175, 170, 165, 160, 155, 150, 145, 140, 135, 130, 128
  // ... many more values would go here
};
const uint32_t dumpTruckStartLen = sizeof(dumpTruckStartData);

// Placeholder for revving sound
const uint8_t dumpTruckRevData[] = {
  128, 135, 142, 149, 156, 163, 170, 177, 184, 191, 198, 205, 212, 219, 226, 233,
  233, 226, 219, 212, 205, 198, 191, 184, 177, 170, 163, 156, 149, 142, 135, 128
  // ... many more values would go here
};
const uint32_t dumpTruckRevLen = sizeof(dumpTruckRevData);

// Placeholder for horn sound
const uint8_t dumpTruckHornData[] = {
  128, 158, 188, 218, 248, 218, 188, 158, 128, 98, 68, 38, 8, 38, 68, 98,
  128, 158, 188, 218, 248, 218, 188, 158, 128, 98, 68, 38, 8, 38, 68, 98
  // ... many more values would go here
};
const uint32_t dumpTruckHornLen = sizeof(dumpTruckHornData);

// Function to load dump truck sounds
void loadDumpTruckSounds() {
  // These functions are defined in sound_system.cpp
  extern const uint8_t* startSoundData;
  extern uint32_t startSoundLen;
  extern const uint8_t* idleSoundData;
  extern uint32_t idleSoundLen;
  extern const uint8_t* revSoundData;
  extern uint32_t revSoundLen;
  extern const uint8_t* hornSoundData;
  extern uint32_t hornSoundLen;
  
  // Load the dump truck sounds
  startSoundData = dumpTruckStartData;
  startSoundLen = dumpTruckStartLen;
  
  idleSoundData = dumpTruckIdleData;
  idleSoundLen = dumpTruckIdleLen;
  
  revSoundData = dumpTruckRevData;
  revSoundLen = dumpTruckRevLen;
  
  hornSoundData = dumpTruckHornData;
  hornSoundLen = dumpTruckHornLen;
}
