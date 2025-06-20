#include "../include/sound/benford_dumper_sounds.h"

// External references to the global sound sample pointers defined in sound_system.cpp
extern const uint8_t* startSoundData;
extern uint32_t startSoundLen;
extern const uint8_t* idleSoundData;
extern uint32_t idleSoundLen;
extern const uint8_t* revSoundData;
extern uint32_t revSoundLen;
extern const uint8_t* hornSoundData;
extern uint32_t hornSoundLen;

// External reference to the sample rate variables
extern uint32_t globalStartSampleRate;
extern uint32_t globalIdleSampleRate;
extern uint32_t globalRevSampleRate;
extern uint32_t globalHornSampleRate;

// External reference to volume settings
extern int startVolumePercentage;
extern int idleVolumePercentage;
extern int engineIdleVolumePercentage;
extern int fullThrottleVolumePercentage;
extern int revVolumePercentage;
extern int engineRevVolumePercentage;
extern int hornVolumePercentage;

// Function to load Benford dumper sounds into the sound system
void loadBenfordDumperSounds() {
  // Load sound samples from the sound_samples namespace
  startSoundData = (const uint8_t*)sound_samples::startSamplesPtr;
  startSoundLen = sound_samples::startSampleCount;
  globalStartSampleRate = sound_samples::startSampleRate;
  
  idleSoundData = (const uint8_t*)sound_samples::idleSamplesPtr;
  idleSoundLen = sound_samples::idleSampleCount;
  globalIdleSampleRate = sound_samples::idleSampleRate;
  
  revSoundData = (const uint8_t*)sound_samples::revSamplesPtr;
  revSoundLen = sound_samples::revSampleCount;
  globalRevSampleRate = sound_samples::revSampleRate;
  
  hornSoundData = (const uint8_t*)sound_samples::hornSamplesPtr;
  hornSoundLen = sound_samples::hornSampleCount;
  globalHornSampleRate = sound_samples::hornSampleRate;

  // Set volume levels
  startVolumePercentage = benfordStartVolumePercentage;
  idleVolumePercentage = benfordIdleVolumePercentage;
  engineIdleVolumePercentage = benfordEngineIdleVolumePercentage;
  fullThrottleVolumePercentage = benfordFullThrottleVolumePercentage;
  revVolumePercentage = benfordRevVolumePercentage;
  engineRevVolumePercentage = benfordEngineRevVolumePercentage;
  hornVolumePercentage = benfordHornVolumePercentage;
}
