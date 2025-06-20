#ifndef SOUND_CONFIG_H
#define SOUND_CONFIG_H

#include <Arduino.h>

// Sound system configuration
// DAC pins on ESP32
#define DAC1_PIN 25
#define DAC2_PIN 26

// Sound settings
#define SAMPLE_RATE 22050
#define STARTUP_VOLUME 140
#define IDLE_VOLUME 80
#define REVVING_VOLUME 100
#define HORN_VOLUME 140

// Sound state enum
enum SoundState {
  SOUND_OFF,
  SOUND_STARTING,
  SOUND_IDLE,
  SOUND_REVVING
};

// Sound system management structure
typedef struct {
  SoundState currentState;
  bool engineOn;
  bool hornOn;
  int16_t throttleLevel;   // Current throttle level (0-1023)
  int16_t revLevel;        // Current rev level (calculated from throttle)
  uint8_t volume;          // Master volume (0-100%)
  bool soundEnabled;       // Whether sound is enabled
} SoundSystem;

extern SoundSystem soundSystem;

// Function declarations
void initSoundSystem();
void updateSoundSystem(int16_t throttleValue, bool hornRequested);
void startEngine();
void stopEngine();
void soundTimerSetup();
void updateEngineSound();
void playHorn(bool on);

#endif // SOUND_CONFIG_H
