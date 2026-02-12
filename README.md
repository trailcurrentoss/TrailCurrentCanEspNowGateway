# TrailCurrent CAN to ESP-NOW Gateway

A wireless relay gateway that bridges CAN bus messages to ESP-NOW protocol for remote vehicle monitoring in tow vehicles.

## Overview

This project enables wireless relaying of CAN bus messages using the ESP-NOW protocol. The gateway hardware and firmware allow real-time data from a trailer's CAN bus to be transmitted reliably to remote displays and monitoring devices in a vehicle.

**Use Case:** Display trailer sensor data (battery voltage, temperature, weight sensors, etc.) on an in-vehicle display while the vehicle and trailer are separated or connected.

## Hardware Overview

- **Function:** Bidirectional CAN-to-ESP-NOW gateway with wireless relay capability
- **Key Features:**
  - CAN bus interface for trailer systems
  - ESP32 with integrated WiFi for robust wireless communication
  - ESP-NOW protocol for low-latency, long-range communication
  - Power management for vehicle and trailer power systems
  - Hierarchical schematic design (5 sheets) for system modularity
  - Compact design for integration into vehicle tow systems

## Hardware Requirements

### Components

- **Microcontroller:**
  - ESP32-WROOM or ESP32-S3 with WiFi and Bluetooth

- **CAN Interface:**
  - CAN transceiver for reliable CAN bus communication
  - Protection circuits for robust automotive operation

- **Power Management:**
  - Buck converter (AP63203WU-7) for regulated power supply
  - Filtering and protection circuits

- **Connectors:**
  - CAN bus interface connectors
  - Power connectors
  - Debug/programming connectors

### KiCAD Library Dependencies

This project uses the consolidated [TrailCurrentKiCADLibraries](https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries).

**Setup:**

```bash
# Clone the library
git clone https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries.git

# Set environment variables (add to ~/.bashrc or ~/.zshrc)
export TRAILCURRENT_SYMBOL_DIR="/path/to/TrailCurrentKiCADLibraries/symbols"
export TRAILCURRENT_FOOTPRINT_DIR="/path/to/TrailCurrentKiCADLibraries/footprints"
export TRAILCURRENT_3DMODEL_DIR="/path/to/TrailCurrentKiCADLibraries/3d_models"
```

See [KICAD_ENVIRONMENT_SETUP.md](https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries/blob/main/KICAD_ENVIRONMENT_SETUP.md) in the library repository for detailed setup instructions.

## Opening the Project

1. **Set up environment variables** (see Library Dependencies above)
2. **Open KiCAD:**
   ```bash
   kicad /path/to/EDA/trailecurrent-can-to-espnow-gateway/trailecurrent-can-to-espnow-gateway.kicad_pro
   ```
3. **Verify libraries load** - All symbol and footprint libraries should resolve without errors
4. **Navigate hierarchical sheets:**
   - Main gateway sheet with module interconnects
   - CAN interface circuit (isolation and termination)
   - ESP32 microcontroller and WiFi section
   - Power management circuits
   - Support and protection circuits
5. **View 3D models** - Open PCB and press `Alt+3` to view the 3D visualization
   - All 3D models should load from the consolidated library path
   - Models use relative `${TRAILCURRENT_3DMODEL_DIR}` references

## Firmware

See `src/` directory for PlatformIO-based firmware implementing CAN-to-ESP-NOW bridging.

**Key Features:**
- CAN message reception and parsing
- ESP-NOW wireless transmission
- Bidirectional communication capability
- Configuration management for CAN filters and routing
- Low-power sleep modes for extended battery life

**Setup:**
```bash
# Install PlatformIO (if not already installed)
pip install platformio

# Build firmware
pio run -d /path/to/project

# Upload to board
pio run -t upload -d /path/to/project

# Monitor serial output
pio device monitor -d /path/to/project
```

**WiFi Credentials:**

The gateway requires WiFi credentials for OTA updates and network communication.

- **Production**: WiFi credentials are delivered via CAN Bus (ID 0x01) from the TrailCurrent system
- **Development**: For standalone testing without the TrailCurrent system, create `src/secrets.h`:

```bash
# Copy the example template
cp src/secrets.h.example src/secrets.h

# Edit with your WiFi credentials
nano src/secrets.h
```

```cpp
// src/secrets.h (DO NOT COMMIT - it's in .gitignore)
const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_PASSWORD_HERE";
uint8_t broadcastAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
```

**Important:**
- Never commit `src/secrets.h` (it's in .gitignore)
- Use `src/secrets.h.example` as a template
- The gateway MAC address should match your hardware

**Future Enhancement:** WiFi credential configuration will be fully implemented via CAN Bus protocol, allowing credentials to be distributed from the TrailCurrent system without firmware modifications.

## System Architecture

The gateway architecture:

1. **CAN Interface:** Receives messages from trailer CAN bus
2. **ESP32 Processor:** Parses and routes messages based on configured rules
3. **ESP-NOW Radio:** Transmits to remote devices with low latency
4. **Remote Device:** Receives and displays trailer data

Supports multiple remote devices through ESP-NOW group addressing.

## Manufacturing

- **PCB Files:** Ready for fabrication via standard PCB services (JLCPCB, OSH Park, etc.)
- **BOM Generation:** Export BOM from KiCAD schematic (Tools → Generate BOM)
- **JLCPCB Assembly:** This project supports automated assembly using JLCPCB
  - See [BOM_ASSEMBLY_WORKFLOW.md](https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries/blob/main/BOM_ASSEMBLY_WORKFLOW.md) for detailed assembly workflow
  - Component library includes LCSC, Mouser, and DigiKey part numbers for flexible sourcing

## Project Structure

- **EDA/trailecurrent-can-to-espnow-gateway/** - KiCAD schematic (5 sheets) and PCB design files
  - Gateway main sheet with module organization
  - CAN interface and protection circuits
  - ESP32 microcontroller and radio circuits
  - Power management and support circuits
- **src/** - Firmware source code for CAN message bridging and ESP-NOW communication
- **lib/** - Custom libraries and CAN protocol drivers
- **include/** - Header files for configuration and messaging

## Documentation

- **Main Schematic:** EDA/trailecurrent-can-to-espnow-gateway/trailecurrent-can-to-espnow-gateway.kicad_sch (with 5 sheet hierarchy)
- **PCB Layout:** EDA/trailecurrent-can-to-espnow-gateway/trailecurrent-can-to-espnow-gateway.kicad_pcb

## License

MIT License - See LICENSE file for details

This is open source hardware. You are free to use, modify, and distribute these designs for personal or commercial purposes.

## Contributing

Improvements and contributions are welcome! Please submit issues or pull requests to the main repository.

## Support

For questions about:
- **KiCAD setup:** See [KICAD_ENVIRONMENT_SETUP.md](https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries/blob/main/KICAD_ENVIRONMENT_SETUP.md)
- **Library consolidation:** See [CONNECTOR_CONSOLIDATION_SUMMARY.md](https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries/blob/main/CONNECTOR_CONSOLIDATION_SUMMARY.md)
- **Assembly workflow:** See [BOM_ASSEMBLY_WORKFLOW.md](https://codeberg.org/trailcurrent/TrailCurrentKiCADLibraries/blob/main/BOM_ASSEMBLY_WORKFLOW.md)
