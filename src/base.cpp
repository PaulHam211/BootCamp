#include <Bluepad32.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h> // For persistent storage

// ============================================
// CONTROLLER CONFIGURATION
// ============================================
// INSTRUCTIONS:
// 1. Uncomment ONE of the following lines to select your controller type
// 2. Comment out the other line 
// 3. Upload the code to your ESP32
//
// For Xbox/Xbox-style controllers (original configuration):
// #define CONTROLLER_XBOX     
//
// For PS4/DualShock 4 controllers:
 #define CONTROLLER_PS4   
//
// The misc buttons are used to switch between receivers:
// - Forward button: increment receiver index (next vehicle)
// - Backward button: decrement receiver index (previous vehicle)  
// - Reset button: reset receiver index to 0
// ============================================

// Button mapping structures
typedef struct ButtonMapping {
    uint16_t buttonA;
    uint16_t buttonB; 
    uint16_t buttonX;
    uint16_t buttonY;
    uint16_t miscForwardMask;
    uint16_t miscBackwardMask;
    uint16_t miscResetMask;
} ButtonMapping;

// Xbox controller button mappings (original)
#ifdef CONTROLLER_XBOX
const ButtonMapping controllerMapping = {
    .buttonA = 1,           // A button
    .buttonB = 2,           // B button  
    .buttonX = 4,           // X button
    .buttonY = 8,           // Y button
    .miscForwardMask = 4,   // Forward button mask
    .miscBackwardMask = 2,  // Backward button mask
    .miscResetMask = 8      // Reset button mask
};
const char* CONTROLLER_TYPE = "Xbox";
#endif

// PS4 controller button mappings
#ifdef CONTROLLER_PS4
const ButtonMapping controllerMapping = {
    .buttonA = 2,           // Cross (X) button - typically mapped to A
    .buttonB = 1,           // Circle (O) button - typically mapped to B  
    .buttonX = 4,           // Square button - typically mapped to X
    .buttonY = 8,           // Triangle button - typically mapped to Y
    .miscForwardMask = 4, // Share button
    .miscBackwardMask = 2,// Options button
    .miscResetMask = 1   // PS button
};
const char* CONTROLLER_TYPE = "PS4";
#endif

// Fallback if no controller is selected
#if !defined(CONTROLLER_XBOX) && !defined(CONTROLLER_PS4)
#error "Please select a controller type by uncommenting either CONTROLLER_XBOX or CONTROLLER_PS4"
#endif

// ============================================

// Preferences for storing persistent data
Preferences preferences;
uint8_t lastControllerAddr[6] = {0}; // To store last controller's Bluetooth address
bool hasLastController = false;      // Flag to indicate if we have stored a controller

ControllerPtr myControllers[BP32_MAX_GAMEPADS];
// Structure to hold controller state
struct ControllerState {
    uint32_t receiverIndex;
    uint16_t buttons;
    uint8_t dpad;
    int32_t axisX, axisY;
    int32_t axisRX, axisRY;
    uint32_t brake, throttle;
    uint16_t miscButtons;
    bool thumbR, thumbL, r1, l1, r2, l2;
};
int miscButtonTime = 0;

struct CalibrationData {
    int32_t axisX, axisY, axisRX, axisRY;
    bool isCalibrated;
};
CalibrationData controllerCalibrations[BP32_MAX_GAMEPADS];
uint32_t receiverIndexes[BP32_MAX_GAMEPADS];
ControllerState gamepadStates[BP32_MAX_GAMEPADS];
// Define the MAC address of the receiver
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool dataSent = true;

void dumpGamepadState(ControllerState *gamepadState) {
    Serial.printf("ID:%d | BTN:0x%04x | DPAD:0x%02x | L:%d,%d | R:%d,%d | BT:%d TH:%d | MISC:0x%04x | FWD:%d BWD:%d RST:%d | R1:%d L1:%d R2:%d L2:%d | TL:%d TR:%d\n",
        gamepadState->receiverIndex,
        gamepadState->buttons,
        gamepadState->dpad,
        gamepadState->axisX,
        gamepadState->axisY,
        gamepadState->axisRX,
        gamepadState->axisRY,
        gamepadState->brake,
        gamepadState->throttle,
        gamepadState->miscButtons,
        gamepadState->miscButtons & controllerMapping.miscForwardMask ? 1 : 0,
        gamepadState->miscButtons & controllerMapping.miscBackwardMask ? 1 : 0,
        gamepadState->miscButtons & controllerMapping.miscResetMask ? 1 : 0,
        gamepadState->r1,
        gamepadState->l1,
        gamepadState->r2,
        gamepadState->l2,
        gamepadState->thumbL,
        gamepadState->thumbR
    );
}
void sendGamepad(ControllerState *gamepadState) {
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)gamepadState, sizeof(*gamepadState));
  //dumpGamepadState(gamepadState);
}
// Controller event callback
void processGamepad(GamepadPtr gp, unsigned controllerIndex) {
    CalibrationData *calibrationData = &controllerCalibrations[controllerIndex];
    uint32_t *receiverIndex = &receiverIndexes[controllerIndex];
    // if (!calibrationData->isCalibrated) {
    //     calibrationData->axisX = gp->axisX();
    //     calibrationData->axisY = gp->axisY();
    //     calibrationData->axisRX = gp->axisRX();
    //     calibrationData->axisRY = gp->axisRY();
    //     calibrationData->isCalibrated = true;
    // }
    // Calibration seems to cause more problems than it solves so disabling for now, if you have a controller with drift you can try commenting the following code and uncommenting the previous code.
    if (!calibrationData->isCalibrated) {
        calibrationData->axisX = 0;
        calibrationData->axisY = 0;
        calibrationData->axisRX = 0;
        calibrationData->axisRY = 0;
        calibrationData->isCalibrated = true;
    }
    if (gp) {
        ControllerState *gamepadState = &gamepadStates[controllerIndex];
        // Set receiver index from the stored value
        gamepadState->receiverIndex = *receiverIndex;
        // Update all controller state values
        gamepadState->buttons = gp->buttons();
        gamepadState->dpad = gp->dpad();
        gamepadState->axisX = gp->axisX() - calibrationData->axisX;
        gamepadState->axisY = gp->axisY() - calibrationData->axisY;
        gamepadState->axisRX = gp->axisRX() - calibrationData->axisRX;
        gamepadState->axisRY = gp->axisRY() - calibrationData->axisRY;
        gamepadState->brake = gp->brake();
        gamepadState->throttle = gp->throttle();
        gamepadState->thumbR = gp->thumbR();
        gamepadState->thumbL = gp->thumbL();
        gamepadState->r1 = gp->r1();
        gamepadState->l1 = gp->l1();
        gamepadState->r2 = gp->r2();
        gamepadState->l2 = gp->l2();gamepadState->miscButtons = gp->miscButtons();        if (gamepadState->miscButtons && (millis() - miscButtonTime) > 200) {
          // Store previous receiver index to detect changes
          uint32_t previousIndex = gamepadState->receiverIndex;
            if (gamepadState->miscButtons & controllerMapping.miscForwardMask) {
            gamepadState->receiverIndex++;
            // Constrain to valid range (0-4)
            if (gamepadState->receiverIndex > 4) 
              gamepadState->receiverIndex = 4;
          } else if (gamepadState->miscButtons & controllerMapping.miscBackwardMask) {
            // Prevent going below 0 by checking before decrementing
            if (gamepadState->receiverIndex > 0)
              gamepadState->receiverIndex--;
          } else if (gamepadState->miscButtons & controllerMapping.miscResetMask) {
            gamepadState->receiverIndex = 0;
          }
          
          // Save updated receiver index back to the shared array
          *receiverIndex = gamepadState->receiverIndex;          // Add rumble feedback when vehicle changed
          if (previousIndex != gamepadState->receiverIndex && myControllers[controllerIndex]) {
            // Provide haptic feedback for vehicle change
            myControllers[controllerIndex]->playDualRumble(0, 500, 0x80, 0x40); // Rumble for 0.5 seconds
            
            // Display message about vehicle change with actual name
            const char* vehicleName;
            switch(gamepadState->receiverIndex) {
              case 0: vehicleName = "No vehicle"; break;
              case 1: vehicleName = "Excavator"; break;
              case 2: vehicleName = "Forklift"; break;
              case 3: vehicleName = "Dump Truck"; break;
              case 4: vehicleName = "Semi-Trailer"; break;
              default: vehicleName = "Unknown"; break;
            }
            Serial.printf("Switched to %s (ID: %d)\n", vehicleName, gamepadState->receiverIndex);
          }
          
          miscButtonTime = millis();
        }
        sendGamepad(gamepadState);

    }
}

void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      
      // Save controller's Bluetooth address for reconnection
      memcpy(lastControllerAddr, properties.btaddr, 6);
      preferences.begin("controller", false);
      preferences.putBytes("btaddr", lastControllerAddr, 6);
      preferences.end();
      
      Serial.print("Saved controller address: ");
      for (int j = 0; j < 6; j++) {
        Serial.printf("%02X", lastControllerAddr[j]);
        if (j < 5) Serial.print(":");
      }
      Serial.println();
      
      myControllers[i] = ctl;
      foundEmptySlot = true;
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}
void onDisconnectedController(ControllerPtr ctl) {
  bool foundController = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
      myControllers[i] = nullptr;
      foundController = true;
      break;
    }
  }

  if (!foundController) {
    Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
  }
}
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Serial.print("Send Status: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}
void setup() {
    Serial.begin(115200);
    
    // Print controller configuration    Serial.println("=======================================");
    Serial.printf("Base Station Initialized\n");
    Serial.printf("Controller Type: %s\n", CONTROLLER_TYPE);
    Serial.printf("Forward Mask: 0x%04x\n", controllerMapping.miscForwardMask);
    Serial.printf("Backward Mask: 0x%04x\n", controllerMapping.miscBackwardMask);
    Serial.printf("Reset Mask: 0x%04x\n", controllerMapping.miscResetMask);
    Serial.println("Available Vehicles:");
    Serial.println("  0: No vehicle selected");
    Serial.println("  1: Excavator");
    Serial.println("  2: Forklift");
    Serial.println("  3: Dump Truck");
    Serial.println("  4: Semi-Trailer");
    Serial.println("=======================================");
      // Initialize all receiver indexes to valid values (0)
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        receiverIndexes[i] = 0;
    }
    
    // Try to retrieve last paired controller address
    preferences.begin("controller", true);
    if (preferences.isKey("btaddr")) {
        hasLastController = true;
        preferences.getBytes("btaddr", lastControllerAddr, 6);
        Serial.print("Found saved controller address: ");
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", lastControllerAddr[i]);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    } else {
        Serial.println("No previous controller saved");
    }
    preferences.end();

    // Initialize Bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);
      // If we have a saved controller, we'll attempt to reconnect automatically
    // The Bluepad32 library will handle reconnection to previously paired controllers
    if (hasLastController) {
        Serial.println("Controller data loaded - Bluepad32 will attempt automatic reconnection...");
        // Note: We can't explicitly reconnect with this version of the library
        // It will automatically try to reconnect to previously paired controllers
    }
    // Set device as Wi-Fi station
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

}

void processControllers() {
  unsigned i = 0;
  for (auto myController : myControllers) {
    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad()) {
        // dumpGamepad(myController);
        processGamepad(myController, i);
      } else {
        Serial.println("Unsupported controller");
      }
    }

    i++;
  }
  dataSent = true;
}

void loop() {
  // Fetch controller updates
  bool dataUpdated = BP32.update();
  if (dataUpdated & dataSent) {
    dataSent = false;
    processControllers();  } else { vTaskDelay(1); }
    // delay(100);  // Send updates every 100ms
    
    // Check if no controllers are connected for a while, try reconnecting
    static unsigned long lastReconnectTime = 0;
    static bool noControllersConnected = true;
    
    // Check if any controllers are connected
    noControllersConnected = true;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] != nullptr && myControllers[i]->isConnected()) {
            noControllersConnected = false;
            break;
        }
    }    // If no controllers and we have last address, show periodic reminder about reconnection
    if (noControllersConnected && hasLastController && (millis() - lastReconnectTime > 10000)) {  // Every 10 seconds
        Serial.println("No controllers connected. Turn on your controller to reconnect automatically.");
        lastReconnectTime = millis();
        
        // Count how long we've been without a controller
        static unsigned long noControllerStartTime = millis();
        static bool countingNoController = false;
        
        if (!countingNoController) {
            countingNoController = true;
            noControllerStartTime = millis();
        } 
        else if ((millis() - noControllerStartTime) > 60000) {  // After 1 minute with no controller
            // Try restarting the Bluetooth system to improve reconnection chances
            Serial.println("No controller for too long, restarting Bluetooth subsystem...");
            BP32.forgetBluetoothKeys();
            countingNoController = false;
            
            // We'll use esp_restart() only in extreme cases
            // Uncomment the following line if you want a complete system restart after 5 minutes
            // if ((millis() - noControllerStartTime) > 300000) esp_restart();
        }
    } else {
        // Reset the counter when controllers are connected
        static bool countingNoController = false;
        countingNoController = false;
    }
}

