#ifndef BENFORD_DUMPER_SOUNDS_H
#define BENFORD_DUMPER_SOUNDS_H

#include <Arduino.h>
#include "sound_config.h"

// Sound samples are defined in the sound_samples namespace in sound_samples.cpp
namespace sound_samples {
  // Sample rates and counts for sound effects
  extern const unsigned int startSampleRate;
  extern const unsigned int startSampleCount;
  extern const signed char* startSamplesPtr;

  extern const unsigned int idleSampleRate;
  extern const unsigned int idleSampleCount;
  extern const signed char* idleSamplesPtr;

  extern const unsigned int revSampleRate;
  extern const unsigned int revSampleCount;
  extern const signed char* revSamplesPtr;

  extern const unsigned int knockSampleRate;
  extern const unsigned int knockSampleCount;
  extern const signed char* knockSamplesPtr;

  extern const unsigned int hornSampleRate;
  extern const unsigned int hornSampleCount;
  extern const unsigned int hornLoopBegin;
  extern const unsigned int hornLoopEnd;
  extern const signed char* hornSamplesPtr;

  extern const unsigned int reversingBeepSampleRate;
  extern const unsigned int reversingBeepSampleCount;
  extern const signed char* reversingBeepSamplesPtr;
}

// Sound volume settings (copied from Benford3TonDumper.h)
const int benfordStartVolumePercentage = 210;
const int benfordIdleVolumePercentage = 100;
const int benfordEngineIdleVolumePercentage = 70;
const int benfordFullThrottleVolumePercentage = 180;
const int benfordRevVolumePercentage = 120;
const int benfordEngineRevVolumePercentage = 70;
const int benfordDieselKnockVolumePercentage = 400;
const int benfordDieselKnockIdleVolumePercentage = 20;
const int benfordHornVolumePercentage = 160;
const int benfordReversingVolumePercentage = 50;

// Rev switching points
const uint16_t benfordRevSwitchPoint = 180;
const uint16_t benfordIdleEndPoint = 500;
const uint16_t benfordIdleVolumeProportionPercentage = 90;

// Function to load Benford dumper sounds into the sound system
void loadBenfordDumperSounds();

#endif // BENFORD_DUMPER_SOUNDS_H
