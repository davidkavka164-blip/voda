#pragma once

#include <stdint.h>

// HOME WATER CONTROLLER — provisional GPIO map rev. C.
// For classic ESP32 DevKit / ESP32-WROOM-32 only.
// Defining a pin here does not mean the corresponding hardware output is enabled.
namespace HwcPins {
namespace Esp32_1 {
constexpr uint8_t W5500_SCK  = 18;
constexpr uint8_t W5500_MISO = 19;
constexpr uint8_t W5500_MOSI = 23;
constexpr uint8_t W5500_CS   = 27;
constexpr uint8_t W5500_RST  = 26;
constexpr uint8_t WELL_LEVEL_ADC = 34;
constexpr uint8_t LOWER_TANK_LEVEL_ADC = 35;
// Manual emergency paths: well -> lower and upper -> lower.
constexpr uint8_t VALVE_1_RELAY = 16;
constexpr uint8_t VALVE_2_RELAY = 17;
// Relay-board logic/module supply: 5 V; active-HIGH input. Verify 3.3 V trigger compatibility.
constexpr bool VALVE_RELAY_ACTIVE_HIGH = true;
// ESP32 #1: independent safety cut-off outputs for the well pump and lower-tank pump.
constexpr uint8_t WELL_PUMP_SAFETY_RELAY = 25;
constexpr uint8_t LOWER_TANK_PUMP_SAFETY_RELAY = 32;
// Dratek 24V/30A DIN relay, same HIGH-mode module and wiring as the working standalone well project.
// GPIO HIGH energizes the relay and permits motor supply; GPIO LOW drops the relay and interrupts it.
constexpr bool PUMP_RELAY_ACTIVE_HIGH = true;
// Enabled using the user-confirmed, already working standalone-well relay configuration.
constexpr bool PUMP_SAFETY_OUTPUTS_ENABLED = true;
}
namespace Esp32_2 {
constexpr uint8_t W5500_SCK  = 18;
constexpr uint8_t W5500_MISO = 19;
constexpr uint8_t W5500_MOSI = 23;
constexpr uint8_t W5500_CS   = 27;
constexpr uint8_t W5500_RST  = 26;
constexpr uint8_t UPPER_TANK_LEVEL_ADC = 34;
constexpr uint8_t WATER_TEMPERATURE_DATA = 4; // assumes DS18B20
// ESP32 #2: local safety cut-off for the upper-tank pump.
constexpr uint8_t UPPER_TANK_PUMP_SAFETY_RELAY = 25;
// Same Dratek 24V/30A HIGH-mode module and proven standalone-well wiring.
constexpr bool PUMP_RELAY_ACTIVE_HIGH = true;
// Enabled using the user-confirmed, already working standalone-well relay configuration.
constexpr bool PUMP_SAFETY_OUTPUTS_ENABLED = true;
}
}
