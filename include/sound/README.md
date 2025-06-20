# Sound Integration for BootCamp Project

This module provides sound capabilities for the BootCamp project vehicles, inspired by the Rc_Engine_Sound_ESP32-master project.

## How it works

The sound system is designed to be modular and can be enabled/disabled for each vehicle type. When enabled, it provides engine sounds (start, idle, revving) and additional sounds like horn.

## Directory Structure

- `/include/sound/` - Configuration files and headers
  - `/include/sound/sound_config.h` - Main sound system configuration 
  - `/include/sound/dump_truck_sounds.h` - Dump truck specific sound configuration
  - `/include/sound/samples/` - Contains the actual sound sample data as C header files
  
- `/src/` - Implementation files
  - `/src/sound_system.cpp` - Core sound implementation
  - `/src/dump_truck_sounds.cpp` - Dump truck specific sound implementation

## How to Use

1. **Build with Sound Support**:
   - Use the appropriate PlatformIO environment from platformio.ini
   - For example: `pio run -e dump_with_sound` 
   - This will build the dump truck firmware with sound support

2. **Add new vehicle sounds**:
   - Create a new sound profile header (e.g., `excavator_sounds.h`)
   - Create a new sound implementation file (e.g., `excavator_sounds.cpp`)
   - Add sound sample files in `/include/sound/samples/`
   - Add a new build environment in platformio.ini

3. **Convert audio files to header files**:
   - Use the `Audio2Header.html` tool from the Rc_Engine_Sound_ESP32-master project
   - Recommended: 22050 Hz, 8-bit PCM audio files

## Sound Configuration

The sound system uses the following pins:
- DAC1: GPIO 25 (Primary audio output)
- DAC2: GPIO 26 (Optional secondary audio output)

## Controls

When sound is enabled, the following controls are available:
- Engine starts automatically when controller connection is established
- Engine stops when connection is lost
- Horn activates when the L2 button is pressed
- Engine revs based on the throttle value

## Adding New Sounds

1. Convert your WAV file to a C header using the Audio2Header.html tool
2. Add the resulting header file to `/include/sound/samples/`
3. Update or create a vehicle sound profile to use the new sound
4. Update the platformio.ini file if needed

## Credits

Sound system implementation inspired by the excellent [Rc_Engine_Sound_ESP32-master](https://github.com/TheDIYGuy999/Rc_Engine_Sound_ESP32) project by TheDIYGuy999.
