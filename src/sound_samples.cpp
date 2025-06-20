// Sound samples implementation file
//
// This file is responsible for including all the sound sample data
// and making it available via the sound_samples namespace.
// The actual implementation flags like BENFORD3TON_START_IMPL are defined
// in platformio.ini as build_flags

// Include the actual sample data
#include "../include/sound/samples/BENFORD3TONStart.h"
#include "../include/sound/samples/BENFORD3TONIdle.h"
#include "../include/sound/samples/BENFORD3TONRev.h"
#include "../include/sound/samples/BENFORD3TONKnock2.h"
#include "../include/sound/samples/CarHorn.h"
#include "../include/sound/samples/TruckReversingBeep.h"

namespace sound_samples {
  // Sound samples from the Benford dumper - using extern to make these visible to other files
  extern const unsigned int startSampleRate;
  extern const unsigned int startSampleCount;
  extern const unsigned int idleSampleRate;
  extern const unsigned int idleSampleCount;
  extern const unsigned int revSampleRate;
  extern const unsigned int revSampleCount;
  extern const unsigned int knockSampleRate;
  extern const unsigned int knockSampleCount;
  extern const unsigned int hornSampleRate;
  extern const unsigned int hornSampleCount;
  extern const unsigned int hornLoopBegin;
  extern const unsigned int hornLoopEnd;
  extern const unsigned int reversingBeepSampleRate;
  extern const unsigned int reversingBeepSampleCount;

  // Pointers to the actual arrays - using extern to make these visible to other files
  extern const signed char* startSamplesPtr;
  extern const signed char* idleSamplesPtr;
  extern const signed char* revSamplesPtr;
  extern const signed char* knockSamplesPtr;
  extern const signed char* hornSamplesPtr;
  extern const signed char* reversingBeepSamplesPtr;
}

// Now define the values outside the namespace block
namespace sound_samples {
  // Sound samples from the Benford dumper
  const unsigned int startSampleRate = ::startSampleRate;
  const unsigned int startSampleCount = ::startSampleCount;
  const unsigned int idleSampleRate = ::idleSampleRate;
  const unsigned int idleSampleCount = ::idleSampleCount;
  const unsigned int revSampleRate = ::revSampleRate;
  const unsigned int revSampleCount = ::revSampleCount;
  const unsigned int knockSampleRate = ::knockSampleRate;
  const unsigned int knockSampleCount = ::knockSampleCount;
  const unsigned int hornSampleRate = ::hornSampleRate;
  const unsigned int hornSampleCount = ::hornSampleCount;
  const unsigned int hornLoopBegin = ::hornLoopBegin;
  const unsigned int hornLoopEnd = ::hornLoopEnd;
  const unsigned int reversingBeepSampleRate = ::reversingSampleRate;
  const unsigned int reversingBeepSampleCount = ::reversingSampleCount;

  // Pointers to the actual arrays
  const signed char* startSamplesPtr = ::startSamples;
  const signed char* idleSamplesPtr = ::idleSamples;
  const signed char* revSamplesPtr = ::revSamples;
  const signed char* knockSamplesPtr = ::knockSamples;
  const signed char* hornSamplesPtr = ::hornSamples;
  const signed char* reversingBeepSamplesPtr = ::reversingSamples;
}
