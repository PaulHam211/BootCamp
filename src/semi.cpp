#include <Arduino.h>
#include <ESP32Servo.h>  // by Kevin Harrington
#include <esp_now.h>
#include <WiFi.h>
#include "sounds/horn.h"  // Horn sound data
#include "sounds/start.h"  // Engine start sound data
#include "sounds/idle.h"  // Engine idle sound data
#include "sounds/rev.h"  // Engine rev sound data


uint32_t thisReceiverIndex = 4;
// Structure to receive message
typedef struct struct_message {
    uint32_t receiverIndex;
    uint16_t buttons;
    uint8_t dpad;
    int32_t axisX, axisY;
    int32_t axisRX, axisRY;
    uint32_t brake, throttle;
    uint16_t miscButtons;
    bool thumbR, thumbL, r1, l1, r2, l2;
} struct_message;
bool dataUpdated;
bool connectionActive = false; // Tracks if connection is currently active
unsigned long lastPacketTime = 0; // Timestamp of last received packet
const unsigned long CONNECTION_TIMEOUT = 3000; // 3 seconds timeout for connection
struct_message receivedData;
uint16_t buttonMaskY = 8;      // Triangle on PS4
uint16_t buttonMaskA = 1;      // Cross on PS4
uint16_t buttonMaskB = 2;      // Circle on PS4
uint16_t buttonMaskX = 4;      // Square on PS4
// ControllerPtr myControllers[BP32_MAX_GAMEPADS];

/*What the different Serial commands for the trailer esp32 daughter board do
1-Trailer Legs Up
2-Trailer Legs Down
3-Ramp Up
4-Ramp Down
5-auxMotor1 Forward
6-auxMotor1 Reverse
7-auxMotor1 STOP
8-auxMotor2 Forward
9-auxMotor2 Reverse
10-auxMotor2 STOP
11- LT1 LOW
12- LT1 HIGH
13- LT2 LOW
14- LT2 HIGH
15- LT3 LOW
16- LT3 HIGH
*/
#define LT1 15
#define LT2 27
#define LT3 14

#define RX0 3
#define TX0 1

#define frontSteeringServoPin 23
#define hitchServoPin 22

Servo frontSteeringServo;
Servo hitchServo;

#define frontMotor0 33  // \ Used for controlling front drive motor movement
#define frontMotor1 32  // /
#define rearMotor0 2    // \ Used for controlling rear drive motor movement
#define rearMotor1 4    // /
#define rearMotor2 12   // \ Used for controlling second rear drive motor movement.
#define rearMotor3 13   // /

#define auxAttach0 18  // \ "Aux1" on PCB. Used for controlling auxillary motor or lights.  Keep in mind this will always breifly turn on when the model is powered on.
#define auxAttach1 5   // /
#define auxAttach2 17  // \ "AUX2" on PCB. Used for controlling auxillary motors or lights.
#define auxAttach3 16  // /
#define auxAttach4 25  // \ "Aux3" on PCB. Used for controlling auxillary motors or lights.
#define auxAttach5 26  // /

// Forward declarations
void flashConnectionIndicator();
void playStartupSound();
void playIdleSound();

// Timer and sound variables
hw_timer_t *soundTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t soundTimerTicks = 0;
volatile uint32_t currentIdleSample = 0;
volatile bool idleSoundActive = false;
volatile uint32_t currentHornSample = 0;
volatile bool hornActive = false;
volatile uint32_t currentRevSample = 0;
volatile bool revSoundActive = false;

// Rev sound configuration (based on Rc_Engine_Sound_ESP32 system)
volatile int revVolumePercentage = 120; // Rev sound volume
volatile int engineRevVolumePercentage = 50; // Engine volume when revving (increased from 80)
volatile const uint16_t revSwitchPoint = 10; // Switch from idle to rev above this throttle point (increased from 2)
volatile const uint16_t idleEndPoint = 70; // Above this point: 100% rev, 0% idle (increased from 50)
volatile const uint16_t idleVolumeProportionPercentage = 75; // Idle proportion below revSwitchPoint (reduced from 90)

// Throttle and RPM variables
volatile int currentThrottle = 0;
volatile int currentThrottleFaded = 0;
volatile int throttleDependentVolume = 60;
volatile int throttleDependentRevVolume = 60;

int lightSwitchButtonTime = 0;
int lightSwitchTime = 0;
int hitchButtonTime = 0;
const int hitchDebounceDelay = 500; // Debounce delay in milliseconds
int adjustedSteeringValue = 90;
int rawSteeringValue = 90; // Raw steering value from controller before trim
int hitchServoValueEngaged = 155;
int hitchServoValueDisengaged = 100;
int steeringTrim = 0;
int lightMode = 0;
bool lightsOn = false;
bool auxLightsOn = false;
bool blinkLT = false;
bool hazardLT = false;
bool hazardsOn = false;
bool smokeGenOn = false;
bool hornPlaying = false;
bool startupSoundPlaying = false;
bool startupSoundPlayedThisConnection = false;
bool idleSoundPlaying = false;
bool trailerAuxMtr1Forward = false;
bool trailerAuxMtr1Reverse = false;
bool trailerAuxMtr2Forward = false;
bool trailerAuxMtr2Reverse = false;
bool hitchUp = true;
bool trailerRampUp = true;
bool trailerLegsUp = true;
bool reducedSpeedMode = false;
unsigned long speedModeButtonTime = 0;
unsigned long rampButtonTime = 0;
unsigned long legsButtonTime = 0;
const int speedModeDebounceDelay = 500; // Debounce delay for speed mode toggle
const int rampDebounceDelay = 500; // Debounce delay for ramp toggle
const int legsDebounceDelay = 500; // Debounce delay for legs toggle

// Callback function for received data
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    struct_message tempReceivedData;
    memcpy(&tempReceivedData, incomingData, sizeof(receivedData));
    if (tempReceivedData.receiverIndex == thisReceiverIndex){
      memcpy(&receivedData, &tempReceivedData, sizeof(receivedData));
      dataUpdated = true;
      
      // Update connection timestamp
      lastPacketTime = millis();
      
      // Check if connection needs to be re-established
      if (!connectionActive) {
        connectionActive = true;
        Serial.println("*** CONTROLLER CONNECTED ***");
        flashConnectionIndicator();
        
        // Play startup sound only once per connection
        if (!startupSoundPlayedThisConnection && !hornPlaying && !startupSoundPlaying && !idleSoundPlaying) {
          // Add a small delay after light flash, then play startup sound
          Serial.println("Playing engine startup sound...");
          playStartupSound();
          startupSoundPlayedThisConnection = true;
        }
      }
    }
}

// Function to flash lights as a connection indicator
void flashConnectionIndicator() {
  // Flash the lights 3 times when connected
  for (int i = 0; i < 3; i++) {
    digitalWrite(LT1, HIGH);
    digitalWrite(LT2, HIGH);
    digitalWrite(LT3, HIGH);
    delay(200);
    digitalWrite(LT1, LOW);
    digitalWrite(LT2, LOW);
    digitalWrite(LT3, LOW);
    delay(200);
  }
}

void moveMotor(int motorPin0, int motorPin1, int velocity) {
  if (velocity > 15) {
    analogWrite(motorPin0, velocity);
    analogWrite(motorPin1, LOW);
  } else if (velocity < -15) {
    analogWrite(motorPin0, LOW);
    analogWrite(motorPin1, (-1 * velocity));
  } else {
    analogWrite(motorPin0, 0);
    analogWrite(motorPin1, 0);
  }
}

void moveServo(int movement, Servo &servo, int &servoValue) {
  switch (movement) {
    case 1:
      if (servoValue >= 10 && servoValue < 170) {
        servoValue = servoValue + 5;
        servo.write(servoValue);
        delay(10);
      }
      break;
    case -1:
      if (servoValue <= 170 && servoValue > 10) {
        servoValue = servoValue - 5;
        servo.write(servoValue);
        delay(10);
      }
      break;
  }
}

// Horn function - now triggers horn in timer interrupt
void playHorn() {
  if (hornPlaying || startupSoundPlaying) return; // Don't start new horn if already playing or during startup
  
  hornPlaying = true;
  hornActive = true;
  currentHornSample = 0;
  
  // Start timer if not already running for idle
  if (!idleSoundActive) {
    soundTimerTicks = 4000000 / 22050; // Same rate as idle for consistent mixing
    timerAlarmWrite(soundTimer, soundTimerTicks, true);
    timerAlarmEnable(soundTimer);
  }
  
  // Wait for horn to finish playing
  while (hornActive) {
    vTaskDelay(1);
  }
  
  hornPlaying = false;
  
  // Stop timer if idle sound isn't supposed to be playing
  if (!idleSoundPlaying) {
    timerAlarmDisable(soundTimer);
  }
}

// Startup sound function - plays engine start sound using both DAC pins
void playStartupSound() {
  if (hornPlaying || startupSoundPlaying) return; // Allow during idle
  
  startupSoundPlaying = true;
  
  // Temporarily disable idle and horn during startup
  bool wasIdleActive = idleSoundActive;
  bool wasHornActive = hornActive;
  if (wasIdleActive) {
    idleSoundActive = false;
    timerAlarmDisable(soundTimer);
  }
  if (wasHornActive) {
    hornActive = false;
  }
  
  // Play all startup samples
  for (int i = 0; i < startSampleCount; i++) {
    // Convert from signed char (-128 to 127) to unsigned DAC value (0-255)
    int dacValue = (int)startSamples[i] + 128;
    
    // Apply volume scaling (adjust volume here if needed)
    dacValue = (dacValue * 120) / 255; // 120/255 = ~47% volume (slightly quieter than horn)
    
    // Output to both DACs
    dacWrite(auxAttach4, dacValue);  // Pin 25 (DAC1)
    dacWrite(auxAttach5, dacValue);  // Pin 26 (DAC2)
    
    // Delay to control playback speed (~22kHz)
    delayMicroseconds(45);
  }
  
  startupSoundPlaying = false;
  Serial.println("Engine startup sound complete");
  
  // Start idle sound after startup completes
  idleSoundPlaying = true;
  Serial.println("Starting engine idle sound...");
}

// Hardware timer interrupt for sound playback with mixing
void IRAM_ATTR soundTimerISR() {
  portENTER_CRITICAL_ISR(&timerMux);
  
  int finalDacValue = 128; // Start with silence (middle of DAC range)
  
  // Mix idle sound if active
  if (idleSoundActive && !startupSoundPlaying) {
    int idleDacValue = (int)samples[currentIdleSample] + 128;
    idleDacValue = (idleDacValue * throttleDependentVolume) / 255;
    finalDacValue = idleDacValue;
    
    // Move to next idle sample
    currentIdleSample++;
    if (currentIdleSample >= sampleCount) {
      currentIdleSample = 0; // Loop back to beginning
    }
  }
  
  // Mix rev sound if active
  if (revSoundActive && !startupSoundPlaying) {
    int revDacValue = (int)revSamples[currentRevSample] + 128;
    revDacValue = (revDacValue * throttleDependentRevVolume) / 180; // Improved divisor for better rev volume
    
    // Mix with idle or replace idle based on throttle
    if (idleSoundActive) {
      // Calculate mixing ratio based on throttle
      int idleWeight = 100;
      int revWeight = 0;
      
      if (currentThrottle > revSwitchPoint) {
        if (currentThrottle < idleEndPoint) {
          // Gradual transition from idle to rev
          revWeight = map(currentThrottle, revSwitchPoint, idleEndPoint, 20, 100); // Start rev at 20% minimum
          idleWeight = map(currentThrottle, revSwitchPoint, idleEndPoint, idleVolumeProportionPercentage, 0);
        } else {
          // Full rev, minimal idle for base tone
          revWeight = 100;
          idleWeight = 10; // Keep some idle for base tone
        }
      }
      
      // Apply mixing with emphasis on rev sound
      finalDacValue = (finalDacValue * idleWeight + revDacValue * revWeight) / (idleWeight + revWeight);
    } else {
      finalDacValue = revDacValue;
    }
    
    // Move to next rev sample
    currentRevSample++;
    if (currentRevSample >= revSampleCount) {
      currentRevSample = 0; // Loop back to beginning
    }
  }
  
  // Mix horn sound if active (horn takes priority in mix)
  if (hornActive) {
    int hornDacValue = (int)hornSamples[currentHornSample] + 128;
    hornDacValue = (hornDacValue * 180) / 255; // Horn at higher volume
    
    // Simple audio mixing - average the two signals with horn emphasis
    if (idleSoundActive || revSoundActive) {
      finalDacValue = (finalDacValue + hornDacValue * 2) / 3; // Horn gets 2/3 weight
    } else {
      finalDacValue = hornDacValue;
    }
    
    // Move to next horn sample
    currentHornSample++;
    if (currentHornSample >= hornSampleCount) {
      hornActive = false; // Horn finished
      currentHornSample = 0;
    }
  }
  
  // Clamp to valid DAC range
  if (finalDacValue < 0) finalDacValue = 0;
  if (finalDacValue > 255) finalDacValue = 255;
  
  // Output mixed audio to both DACs
  dacWrite(auxAttach4, finalDacValue);  // Pin 25 (DAC1)
  dacWrite(auxAttach5, finalDacValue);  // Pin 26 (DAC2)
  
  portEXIT_CRITICAL_ISR(&timerMux);
}

// Idle sound function - now just controls the timer
void playIdleSound() {
  if (!idleSoundPlaying || startupSoundPlaying) {
    if (idleSoundActive) {
      idleSoundActive = false;
      // Only disable timer if horn and rev aren't also using it
      if (!hornActive && !revSoundActive) {
        timerAlarmDisable(soundTimer);
      }
    }
    return;
  }
  
  if (!idleSoundActive) {
    // Calculate timer ticks for correct sample rate
    soundTimerTicks = 4000000 / sampleRate; // 4MHz / 22050 = ~181 ticks per sample
    
    // Start the timer
    timerAlarmWrite(soundTimer, soundTimerTicks, true);
    timerAlarmEnable(soundTimer);
    idleSoundActive = true;
    currentIdleSample = 0;
    Serial.println("Engine idle timer started");
  }
}

// Process horn input (left thumbstick press)
void processHorn(bool buttonValue) {
  if (buttonValue && !hornPlaying && !startupSoundPlaying) {
    Serial.println("Horn activated!");
    playHorn(); // Horn will now play over idle sound
  }
}

// Update throttle value and calculate engine sound volumes
void updateThrottle(int axisYValue) {
  // Map joystick input to throttle range (0-100, better for small joystick values)
  if (abs(axisYValue) > 50) { // Lower threshold for detection
    currentThrottle = map(abs(axisYValue), 50, 600, 0, 100); // Map 50-600 range to 0-100
    currentThrottle = constrain(currentThrottle, 0, 100);
  } else {
    currentThrottle = 0; // Idle
  }
  
  // Smooth throttle changes (fading)
  static unsigned long lastThrottleUpdate = 0;
  if (millis() - lastThrottleUpdate > 5) { // Update every 5ms
    if (currentThrottleFaded < currentThrottle && currentThrottleFaded < 99) {
      currentThrottleFaded += 2; // Acceleration rate
    }
    if (currentThrottleFaded > currentThrottle && currentThrottleFaded > 1) {
      currentThrottleFaded -= 1; // Deceleration rate
    }
    lastThrottleUpdate = millis();
  }
  
  // Calculate throttle dependent volumes
  if (connectionActive && idleSoundPlaying) {
    // Idle volume decreases as throttle increases
    throttleDependentVolume = map(currentThrottleFaded, 0, 100, 120, 60); // 120 at idle, 60 at full throttle
    
    // Rev volume increases with throttle
    throttleDependentRevVolume = map(currentThrottleFaded, 0, 100, engineRevVolumePercentage, 180); // Increased max from 150 to 180
    
    // Start rev sound if throttle is above switch point
    if (currentThrottle > revSwitchPoint && !revSoundActive) {
      revSoundActive = true;
      currentRevSample = 0;
      
      // Make sure timer is running for rev sound
      if (!idleSoundActive && !hornActive) {
        soundTimerTicks = 4000000 / revSampleRate; // Use rev sample rate
        timerAlarmWrite(soundTimer, soundTimerTicks, true);
        timerAlarmEnable(soundTimer);
      }
    }
    
    // Stop rev sound if throttle drops below switch point
    if (currentThrottle <= revSwitchPoint && revSoundActive) {
      revSoundActive = false;
      
      // Stop timer if nothing else is using it
      if (!idleSoundActive && !hornActive) {
        timerAlarmDisable(soundTimer);
      }
    }
  }
}


void processTrailerLegs(int value) {
  // Use Cross button (buttonMaskA) for toggling the trailer legs position
  if ((value & buttonMaskA) && (millis() - legsButtonTime > legsDebounceDelay)) {
    // Toggle the legs position
    if (trailerLegsUp) {
      Serial.println(2); // Legs Down
      Serial.println("Trailer Legs: Down");
      trailerLegsUp = false;
    } else {
      Serial.println(1); // Legs Up
      Serial.println("Trailer Legs: Up");
      trailerLegsUp = true;
    }
    delay(10);
    
    // Update the last button press time
    legsButtonTime = millis();
  }
}
void processTrailerRamp(int value) {
  // Use Circle button (buttonMaskB) for toggling the ramp position
  if ((value & buttonMaskB) && (millis() - rampButtonTime > rampDebounceDelay)) {
    // Toggle the ramp position
    if (trailerRampUp) {
      Serial.println(4); // Ramp Down
      Serial.println("Ramp: Down");
      trailerRampUp = false;
    } else {
      Serial.println(3); // Ramp Up
      Serial.println("Ramp: Up");
      trailerRampUp = true;
    }
    delay(10);
    
    // Update the last button press time
    rampButtonTime = millis();
  }
}

void processSpeedMode(int value) {
  // Triangle button toggles reduced speed mode with debouncing
  if ((value & buttonMaskY) && (millis() - speedModeButtonTime > speedModeDebounceDelay)) {
    reducedSpeedMode = !reducedSpeedMode; // Toggle the speed mode
    Serial.print("Speed Mode: ");
    Serial.println(reducedSpeedMode ? "Reduced (50%)" : "Normal (100%)");
    speedModeButtonTime = millis();
  }
}

void processThrottle(int axisYValue) {
  // Update engine sound based on throttle
  updateThrottle(axisYValue);
  
  int adjustedThrottleValue = axisYValue / 2;
  
  // Apply 50% speed reduction if reduced speed mode is enabled
  if (reducedSpeedMode) {
    adjustedThrottleValue = adjustedThrottleValue / 2; // Further reduce to 50%
  }
  
  int smokeThrottle = adjustedThrottleValue / 3;
  
  moveMotor(rearMotor0, rearMotor1, adjustedThrottleValue);
  moveMotor(rearMotor2, rearMotor3, adjustedThrottleValue);
  moveMotor(frontMotor0, frontMotor1, adjustedThrottleValue);
  moveMotor(auxAttach2, auxAttach3, smokeThrottle);
}

void processTrimAndHitch(int dpadValue) {
  if (dpadValue == 4 && steeringTrim < 20) {
    steeringTrim = steeringTrim + 1;
    delay(50);
  } else if (dpadValue == 8 && steeringTrim > -20) {
    steeringTrim = steeringTrim - 1;
    delay(50);
  }
  
  // Hitch toggle with debounce - only process if sufficient time has passed since last button press
  if (dpadValue == 2 && (millis() - hitchButtonTime > hitchDebounceDelay)) {
    // Toggle the hitch state
    if (hitchUp) {
      hitchServo.write(hitchServoValueDisengaged);
      hitchUp = false;
    } else {
      hitchServo.write(hitchServoValueEngaged);
      hitchUp = true;
    }
    delay(10);
    
    // Update the last button press time
    hitchButtonTime = millis();
  }
}
void processSteering(int axisRXValue) {
  rawSteeringValue = 90 - (axisRXValue / 9); // Store raw steering value without trim
  adjustedSteeringValue = rawSteeringValue - steeringTrim; // Apply trim for actual steering
  frontSteeringServo.write(180 - adjustedSteeringValue);
}

void processLights(bool buttonValue) {
  if (buttonValue && (millis() - lightSwitchButtonTime) > 300) {
    lightMode++;
    if (lightMode == 1) {
      digitalWrite(LT1, HIGH);
      digitalWrite(LT2, HIGH);
      Serial.println(12);
      delay(10);
      Serial.println(14);
    } else if (lightMode == 2) {
      digitalWrite(LT1, LOW);
      digitalWrite(LT2, LOW);
      delay(100);
      digitalWrite(LT1, HIGH);
      digitalWrite(LT2, HIGH);
      blinkLT = true;
    } else if (lightMode == 3) {
      blinkLT = false;
      hazardLT = true;
    } else if (lightMode == 4) {
      hazardLT = false;
      digitalWrite(LT1, LOW);
      digitalWrite(LT2, LOW);
      Serial.println(11);
      delay(10);
      Serial.println(13);
      lightMode = 0;
      if (!auxLightsOn) {
        digitalWrite(LT3, HIGH);
        Serial.println(16);
        auxLightsOn = true;
      } else {
        digitalWrite(LT3, LOW);
        Serial.println(15);
        auxLightsOn = false;
      }
    }
    lightSwitchButtonTime = millis();
  }
}

void processSmokeGen(bool buttonValue) {
  if (buttonValue) {
    if (!smokeGenOn) {
      digitalWrite(auxAttach0, LOW);
      digitalWrite(auxAttach1, HIGH);
      smokeGenOn = true;
    } else {
      digitalWrite(auxAttach0, LOW);
      digitalWrite(auxAttach1, LOW);
      smokeGenOn = false;
    }
  }
}

void processTrailerAuxMtr1Forward(bool value) {
  if (value) {
      Serial.println(5);
      delay(10);
      trailerAuxMtr1Forward = true;
  } else if (trailerAuxMtr1Forward) {
    Serial.println(7);
    delay(10);
    trailerAuxMtr1Forward = false;
  }
}
void processTrailerAuxMtr1Reverse(bool value) {
  if (value) {
      Serial.println(6);
      delay(10);
      trailerAuxMtr1Reverse = true;
  } else if (trailerAuxMtr1Reverse) {
    Serial.println(7);
    delay(10);
    trailerAuxMtr1Reverse = false;
  }
}
void processTrailerAuxMtr2Forward(bool value) {
  if (value) {
      Serial.println(8);
      delay(10);
      trailerAuxMtr2Forward = true;
  } else if (trailerAuxMtr2Forward) {
    Serial.println(10);
    delay(10);
    trailerAuxMtr2Forward = false;
  }
}
void processTrailerAuxMtr2Reverse(bool value) {
  if (value) {
      Serial.println(9);
      delay(10);
      trailerAuxMtr2Reverse = true;
  } else if (trailerAuxMtr2Reverse) {
    Serial.println(10);
    delay(10);
    trailerAuxMtr2Reverse = false;
  }
}

void processGamepad() {
  //Throttle
  processThrottle(receivedData.axisY);
  //Steering
  processSteering(receivedData.axisRX);
  //Steering trim and hitch
  processTrimAndHitch(receivedData.dpad);
  //Lights
  processLights(receivedData.thumbR);
  //Horn (left thumbstick press)
  processHorn(receivedData.thumbL);
  
  // Process speed mode toggle (Triangle button)
  processSpeedMode(receivedData.buttons);

  processTrailerLegs(receivedData.buttons);
  processTrailerRamp(receivedData.buttons);

  processTrailerAuxMtr1Forward(receivedData.r1);
  processTrailerAuxMtr1Reverse(receivedData.r2);
  processTrailerAuxMtr2Forward(receivedData.l1);
  processTrailerAuxMtr2Reverse(receivedData.l2);

  if (blinkLT && (millis() - lightSwitchTime) > 300) {
    if (!lightsOn) {
      if (rawSteeringValue <= 85) {
        digitalWrite(LT1, HIGH);
        Serial.println(12);
      } else if (rawSteeringValue >= 95) {
        digitalWrite(LT2, HIGH);
        Serial.println(14);
      }
      lightsOn = true;
    } else {
      if (rawSteeringValue <= 85) {
        digitalWrite(LT2, HIGH);
        digitalWrite(LT1, LOW);
        Serial.println(11);
        delay(10);
        Serial.println(14);
      } else if (rawSteeringValue >= 95) {
        digitalWrite(LT1, HIGH);
        digitalWrite(LT2, LOW);
        Serial.println(13);
        delay(10);
        Serial.println(12);
      }
      lightsOn = false;
    }
    lightSwitchTime = millis();
  }
  if (blinkLT && rawSteeringValue > 85 && rawSteeringValue < 95) {
    digitalWrite(LT1, HIGH);
    digitalWrite(LT2, HIGH);
    Serial.println(12);
    delay(10);
    Serial.println(14);
  }
  if (hazardLT && (millis() - lightSwitchTime) > 300) {
    if (!hazardsOn) {
      digitalWrite(LT1, HIGH);
      digitalWrite(LT2, HIGH);
      Serial.println(12);
      delay(10);
      Serial.println(14);
      hazardsOn = true;
    } else {
      digitalWrite(LT1, LOW);
      digitalWrite(LT2, LOW);
      Serial.println(11);
      delay(10);
      Serial.println(13);
      hazardsOn = false;
    }
    lightSwitchTime = millis();
  }
}

void processControllers() {
  processGamepad();
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow serial to initialize
  
  // Application introduction
  Serial.println("==========================================");
  Serial.println("   ESP32 Semi-Truck Controller v2.0");
  Serial.println("   with Realistic Engine Sound System");
  Serial.println("==========================================");
  Serial.println("Initializing systems...");

  // Initialize sound timer (Timer 0, prescaler 20, count up)
  soundTimer = timerBegin(0, 20, true);
  timerAttachInterrupt(soundTimer, &soundTimerISR, true);
  Serial.println("Sound system initialized");
  
  // Initialize connection variables
  connectionActive = false;
  lastPacketTime = 0;

  pinMode(rearMotor0, OUTPUT);
  pinMode(rearMotor1, OUTPUT);
  pinMode(rearMotor2, OUTPUT);
  pinMode(rearMotor3, OUTPUT);
  pinMode(frontMotor0, OUTPUT);
  pinMode(frontMotor1, OUTPUT);
  pinMode(auxAttach0, OUTPUT);
  pinMode(auxAttach1, OUTPUT);
  pinMode(auxAttach2, OUTPUT);
  pinMode(auxAttach3, OUTPUT);
  pinMode(auxAttach4, OUTPUT);
  pinMode(auxAttach5, OUTPUT);
  pinMode(LT1, OUTPUT);
  pinMode(LT2, OUTPUT);
  pinMode(LT3, OUTPUT);

  digitalWrite(rearMotor0, LOW);
  digitalWrite(rearMotor1, LOW);
  digitalWrite(rearMotor2, LOW);
  digitalWrite(rearMotor3, LOW);
  digitalWrite(frontMotor0, LOW);
  digitalWrite(frontMotor1, LOW);
  digitalWrite(auxAttach0, LOW);
  digitalWrite(auxAttach1, LOW);
  digitalWrite(auxAttach2, LOW);
  digitalWrite(auxAttach3, LOW);
  digitalWrite(auxAttach4, LOW);
  digitalWrite(auxAttach5, LOW);
  digitalWrite(LT1, LOW);
  digitalWrite(LT2, LOW);
  digitalWrite(LT3, LOW);
  Serial.println("GPIO pins configured");

  frontSteeringServo.attach(frontSteeringServoPin);
  frontSteeringServo.write(adjustedSteeringValue);
  hitchServo.attach(hitchServoPin);
  hitchServo.write(hitchServoValueDisengaged); // Set to disengaged position on boot
  hitchUp = false; // Initialize hitchUp to match the actual servo position
  trailerRampUp = true; // Initialize ramp to up position by default
  trailerLegsUp = true; // Initialize legs to up position by default
  Serial.println("Servos initialized");

    WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  Serial.println("WiFi configured in Station mode");

  if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW initialized successfully");
  Serial.println("==========================================");
  Serial.println("System ready! Waiting for controller...");
  Serial.println("==========================================");
}



// Arduino loop function. Runs in CPU 1.
void loop() {
  if (dataUpdated) {
    processControllers();
    dataUpdated = false;
  }
  else { vTaskDelay(1); }

  // Process idle sound continuously (outside of dataUpdated check)
  if (idleSoundPlaying && !startupSoundPlaying && connectionActive) {
    playIdleSound();
  }

  // Check for connection timeout
  if (connectionActive && (millis() - lastPacketTime > CONNECTION_TIMEOUT)) {
    connectionActive = false;
    Serial.println("*** CONTROLLER DISCONNECTED ***");
    Serial.println("Connection timeout - stopping all systems");
    
    // Reset sound states on disconnection
    startupSoundPlayedThisConnection = false;
    idleSoundPlaying = false;
    hornPlaying = false;
    startupSoundPlaying = false;
    
    // Stop idle sound timer
    if (idleSoundActive) {
      idleSoundActive = false;
      timerAlarmDisable(soundTimer);
      Serial.println("Engine sounds stopped");
    }
    
    // Stop horn if active
    if (hornActive) {
      hornActive = false;
    }
    
    // Stop rev sound if active
    if (revSoundActive) {
      revSoundActive = false;
    }
    
    // Reset throttle values
    currentThrottle = 0;
    currentThrottleFaded = 0;
    
    // Handle connection timeout (e.g., stop motors, reset values, etc.)
    digitalWrite(rearMotor0, LOW);
    digitalWrite(rearMotor1, LOW);
    digitalWrite(rearMotor2, LOW);
    digitalWrite(rearMotor3, LOW);
    digitalWrite(frontMotor0, LOW);
    digitalWrite(frontMotor1, LOW);
    digitalWrite(auxAttach0, LOW);
    digitalWrite(auxAttach1, LOW);
    digitalWrite(auxAttach2, LOW);
    digitalWrite(auxAttach3, LOW);
    digitalWrite(auxAttach4, LOW);
    digitalWrite(auxAttach5, LOW);
    digitalWrite(LT1, LOW);
    digitalWrite(LT2, LOW);
    digitalWrite(LT3, LOW);
  }
}