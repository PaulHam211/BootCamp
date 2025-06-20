#include <Arduino.h>
#include "../include/sound/sound_config.h"
#include <driver/dac.h>

// Create the global sound system
SoundSystem soundSystem = {
  SOUND_OFF,  // currentState
  false,      // engineOn
  false,      // hornOn
  0,          // throttleLevel
  0,          // revLevel
  80,         // volume (80%)
  true        // soundEnabled
};

// Sound sample rates - will be set by vehicle sound modules
uint32_t globalStartSampleRate = 22050;
uint32_t globalIdleSampleRate = 22050;
uint32_t globalRevSampleRate = 22050;
uint32_t globalHornSampleRate = 22050;

// Sound volume settings - will be set by vehicle sound profiles
int startVolumePercentage = STARTUP_VOLUME;
int idleVolumePercentage = IDLE_VOLUME;
int engineIdleVolumePercentage = 70;
int fullThrottleVolumePercentage = 150;
int revVolumePercentage = REVVING_VOLUME;
int engineRevVolumePercentage = 70;
int hornVolumePercentage = HORN_VOLUME;

// Timer for sound playback
hw_timer_t *soundTimer = NULL;
portMUX_TYPE soundTimerMux = portMUX_INITIALIZER_UNLOCKED;

// Current sample positions
volatile uint32_t currentStartSample = 0;
volatile uint32_t currentIdleSample = 0;
volatile uint32_t currentRevSample = 0;
volatile uint32_t currentHornSample = 0;

// Sound sample pointers - will be set by vehicle sound modules
const uint8_t* startSoundData = NULL;
uint32_t startSoundLen = 0;

const uint8_t* idleSoundData = NULL;
uint32_t idleSoundLen = 0;

const uint8_t* revSoundData = NULL;
uint32_t revSoundLen = 0;

const uint8_t* hornSoundData = NULL;
uint32_t hornSoundLen = 0;

// Sound interrupt handler
void IRAM_ATTR onSoundTimer() {
  // Enter critical section
  portENTER_CRITICAL_ISR(&soundTimerMux);
  
  int16_t outputSample = 0;
  
  // Sound mixing logic
  switch (soundSystem.currentState) {
    case SOUND_STARTING:
      if (currentStartSample < startSoundLen) {
        outputSample = startSoundData[currentStartSample++] - 128;
        outputSample = (outputSample * STARTUP_VOLUME) / 100;
      } else {
        soundSystem.currentState = SOUND_IDLE;
        currentStartSample = 0;
      }
      break;
      
    case SOUND_IDLE:
      // Idle sound (looping)
      if (idleSoundData != NULL && idleSoundLen > 0) {
        outputSample = idleSoundData[currentIdleSample++] - 128;
        if (currentIdleSample >= idleSoundLen) {
          currentIdleSample = 0;
        }
        
        // Apply idle volume
        outputSample = (outputSample * IDLE_VOLUME) / 100;
      }
      
      // If throttle is applied, gradually transition to revving sound
      if (soundSystem.throttleLevel > 150 && revSoundData != NULL) {
        int16_t revSample = revSoundData[currentRevSample++] - 128;
        if (currentRevSample >= revSoundLen) {
          currentRevSample = 0;
        }
        
        // Calculate blend factor based on throttle
        int16_t blend = map(soundSystem.throttleLevel, 150, 1023, 0, 100);
        blend = constrain(blend, 0, 100);
        
        // Mix idle and rev sounds
        outputSample = map(blend, 0, 100, outputSample, revSample);
        
        // If we're at high throttle, transition to REVVING state
        if (blend > 80) {
          soundSystem.currentState = SOUND_REVVING;
        }
      }
      break;
      
    case SOUND_REVVING:
      // Revving sound (looping)
      if (revSoundData != NULL && revSoundLen > 0) {
        outputSample = revSoundData[currentRevSample++] - 128;
        if (currentRevSample >= revSoundLen) {
          currentRevSample = 0;
        }
        
        // Apply revving volume based on throttle
        int16_t revVolume = map(soundSystem.throttleLevel, 150, 1023, IDLE_VOLUME, REVVING_VOLUME);
        outputSample = (outputSample * revVolume) / 100;
      }
      
      // If throttle is reduced, transition back to idle
      if (soundSystem.throttleLevel < 150) {
        soundSystem.currentState = SOUND_IDLE;
      }
      break;
      
    case SOUND_OFF:
    default:
      outputSample = 0;
      break;
  }
  
  // Mix in horn sound if active
  if (soundSystem.hornOn && hornSoundData != NULL) {
    int16_t hornSample = hornSoundData[currentHornSample++] - 128;
    if (currentHornSample >= hornSoundLen) {
      currentHornSample = 0;
      soundSystem.hornOn = false;  // Auto-stop horn after one play
    }
    
    // Apply horn volume and mix with engine sound
    hornSample = (hornSample * HORN_VOLUME) / 100;
    outputSample = (outputSample + hornSample) / 2;  // Simple mixing
  }
  
  // Apply master volume
  outputSample = (outputSample * soundSystem.volume) / 100;
  
  // Convert to DAC range (0-255) and output to DAC
  uint8_t dacValue = constrain(outputSample + 128, 0, 255);
  
  // Only output if sound is enabled
  if (soundSystem.soundEnabled) {
    dac_output_voltage(DAC_CHANNEL_1, dacValue);
    // You can use the second DAC channel for stereo if desired
    // dac_output_voltage(DAC_CHANNEL_2, dacValue);
  }
  
  // Exit critical section
  portEXIT_CRITICAL_ISR(&soundTimerMux);
}

// Initialize the sound system
void initSoundSystem() {
  // Initialize DAC
  dac_output_enable(DAC_CHANNEL_1);
  // dac_output_enable(DAC_CHANNEL_2); // For stereo output
  
  // Setup timer
  soundTimerSetup();
  
  // Initialize sound system state
  soundSystem.currentState = SOUND_OFF;
  soundSystem.engineOn = false;
  soundSystem.hornOn = false;
  soundSystem.throttleLevel = 0;
  soundSystem.revLevel = 0;
  soundSystem.volume = 80; // Default to 80%
  soundSystem.soundEnabled = true;
}

// Setup timer for sound playback
void soundTimerSetup() {
  // Setup timer for sound playback at SAMPLE_RATE
  soundTimer = timerBegin(0, 80, true); // Timer 0, prescaler 80, count up
  timerAttachInterrupt(soundTimer, &onSoundTimer, true); // Attach interrupt
  timerAlarmWrite(soundTimer, 1000000 / SAMPLE_RATE, true); // Set alarm to match sample rate
  timerAlarmEnable(soundTimer); // Enable the alarm
}

// Update the sound system with current inputs
void updateSoundSystem(int16_t throttleValue, bool hornRequested) {
  soundSystem.throttleLevel = throttleValue;
  
  // Process horn button
  if (hornRequested && !soundSystem.hornOn) {
    soundSystem.hornOn = true;
    currentHornSample = 0; // Reset horn position
  }
  
  // Engine sound is updated in the timer interrupt
}

// Start the engine
void startEngine() {
  if (soundSystem.currentState == SOUND_OFF) {
    soundSystem.engineOn = true;
    soundSystem.currentState = SOUND_STARTING;
    currentStartSample = 0; // Start from the beginning of the start sound
  }
}

// Stop the engine
void stopEngine() {
  soundSystem.engineOn = false;
  soundSystem.currentState = SOUND_OFF;
}

// Play the horn sound
void playHorn(bool on) {
  if (on && !soundSystem.hornOn) {
    soundSystem.hornOn = true;
    currentHornSample = 0; // Reset horn position
  } else {
    soundSystem.hornOn = false;
  }
}
