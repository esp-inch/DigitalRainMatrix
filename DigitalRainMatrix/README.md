# DigitalRainMatrix

A Matrix-style digital rain effect for ESP32 with HUB75 LED matrix panels. Creates mesmerizing cascading droplets with customizable colors, controllable via WiFi/UDP.

## Features

- **Three Effect Modes**:
  - **Wintry Mix**: Temperature-based switching between water and sleet
  - **Water Only**: Classic rain with colored tails
  - **Sleet Only**: Minimalist white dots
- **Matrix Rain Effect**: 128 simultaneous animated droplets with customizable appearance
- **Automatic Color Cycling**: Smoothly transitions through colors every 45 seconds (Red → Yellow → Green → Cyan → Blue → Magenta)
- **Network Control**: Change colors remotely via UDP packets (ESP32-S3 only)
- **High Performance**: Hardware DMA for flicker-free display
- **Dual Panel Support**: Runs on 128x64 total resolution (two 64x64 panels)

## Hardware Requirements

### ESP32-S3 DevKitC-1 (Recommended)
- WiFi/UDP color control enabled
- Enhanced performance

### Alternative: Standard ESP32 Dev Board
- Runs without network features
- WiFi disabled for performance

### LED Matrix Panels
- **Panels**: 2x 64x64 HUB75 RGB LED Matrix Panels
- **Total Resolution**: 128x64 pixels
- **Scan Rate**: 1/32 scan
- **Driver IC**: FM6124

### Optional Components
- Push button on GPIO 36 (currently unused but available)

## Pin Configuration

### ESP32-S3 Pin Mapping
```
R1:  GPIO 4      G1:  GPIO 5      B1:  GPIO 6
R2:  GPIO 7      G2:  GPIO 15     B2:  GPIO 16
A:   GPIO 18     B:   GPIO 8      C:   GPIO 3
D:   GPIO 42     E:   GPIO 21
LAT: GPIO 40     OE:  GPIO 2      CLK: GPIO 41
```

### Standard ESP32 Pin Mapping
```
R1:  GPIO 25     G1:  GPIO 26     B1:  GPIO 27
R2:  GPIO 14     G2:  GPIO 12     B2:  GPIO 13
A:   GPIO 23     B:   GPIO 22     C:   GPIO 5
D:   GPIO 17     E:   GPIO 32
LAT: GPIO 4      OE:  GPIO 15     CLK: GPIO 16
```

## Software Dependencies

This project uses [PlatformIO](https://platformio.org/). Dependencies are managed automatically:

- [FastLED](https://github.com/FastLED/FastLED) ^3.6.0
- [ESP32 HUB75 LED MATRIX PANEL DMA Display](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) ^3.0.10
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) ^1.11.9
- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) ^1.15.0

## Installation

1. **Install PlatformIO**
   - [VS Code Extension](https://platformio.org/install/ide?install=vscode) (recommended)
   - Or [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/installation.html)

2. **Clone Repository**
   ```bash
   git clone <your-repo-url>
   cd DigitalRainMatrix/DigitalRainMatrix
   ```

3. **Configure WiFi** (ESP32-S3 only)

   Edit [src/main.cpp](src/main.cpp) lines 54-55:
   ```cpp
   const char* ssid = "YourNetworkName";
   const char* pass = "YourPassword";
   ```

4. **Select Effect Mode** (Optional)

   Choose your desired effect mode environment (see [Effect Modes](#effect-modes) for details):
   - `Esp32-S3-WintryMix` (default) - Temperature-based ice/water mix
   - `Esp32-S3-WaterOnly` - Always water droplets
   - `Esp32-S3-SleetOnly` - Always sleet droplets

5. **Build and Upload**
   ```bash
   # Using default Wintry Mix mode
   pio run -e Esp32-S3-WintryMix -t upload

   # Or use other modes
   pio run -e Esp32-S3-WaterOnly -t upload
   pio run -e Esp32-S3-SleetOnly -t upload
   ```

   Or select your desired environment and use the PlatformIO IDE upload button.

## Usage

### Basic Operation
- Power on the ESP32
- The digital rain effect starts automatically
- Colors cycle automatically every 45 seconds

### Network Control (ESP32-S3)
Send UDP packets to port `21324` with color data:

**Packet Format:**
- Byte 3: Red (0-255)
- Byte 4: Green (0-255)
- Byte 5: Blue (0-255)

**Example using Python:**
```python
import socket

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Send purple color (128, 0, 255)
data = bytearray([0, 0, 0, 128, 0, 255])
sock.sendto(data, ('ESP32_IP_ADDRESS', 21324))
```

**Example using netcat:**
```bash
echo -ne '\x00\x00\x00\x80\x00\xFF' | nc -u ESP32_IP_ADDRESS 21324
```

## Configuration

### Effect Modes

The display supports three different effect modes. You can configure them in two ways:

#### Option 1: Use PlatformIO Build Environments (Recommended)

Simply select the desired environment in PlatformIO and upload:

- **Esp32-S3-WintryMix** - Temperature-based ice/water mix (ice blue color)
- **Esp32-S3-WaterOnly** - Always water droplets (purple color)
- **Esp32-S3-SleetOnly** - Always sleet droplets (cyan color)

**In VS Code with PlatformIO:**
1. Click the environment switcher in the status bar (bottom of window)
2. Select your desired environment
3. Click Upload

**Via Command Line:**
```bash
# Wintry Mix mode
pio run -e Esp32-S3-WintryMix -t upload

# Water Only mode
pio run -e Esp32-S3-WaterOnly -t upload

# Sleet Only mode
pio run -e Esp32-S3-SleetOnly -t upload
```

#### Option 2: Edit Configuration in Code

Alternatively, you can configure modes manually in [src/main.cpp](src/main.cpp) (lines 20-41):

#### Mode 0: WINTRY_MIX (Default)
Temperature-based simulation that switches between water and sleet droplets:
- **Temperature Range**: 32-34°F
- **Update Interval**: Every 3 seconds (configurable)
- **At 32°F**: Generates sleet (white dots only, no tails)
- **Above 32°F**: Generates water (white heads with colored tails)
- **Default Color**: Ice blue (0x8AF9FF - R=138, G=249, B=255)

```cpp
#define EFFECT_MODE 0
#define TEMP_CHANGE_INTERVAL 3000  // milliseconds
#define START_TEMP 33              // Starting temperature
```

#### Mode 1: WATER_ONLY
Always generates water droplets with tails, ignoring temperature:
- All droplets have white heads with colored fading tails
- Consistent rain effect
- Default color: Purple (0x793DF4)

```cpp
#define EFFECT_MODE 1
```

#### Mode 2: SLEET_ONLY
Always generates sleet droplets without tails, ignoring temperature:
- All droplets are white dots only
- No colored tails
- Creates a "static snow" effect
- Default color: Purple (0x793DF4)

```cpp
#define EFFECT_MODE 2
```

#### Customizing Colors

Change the droplet color by editing the `DROPLET_COLOR` define:

```cpp
// Example colors:
#define DROPLET_COLOR 0x8AF9FF  // Ice blue (default for WINTRY_MIX)
#define DROPLET_COLOR 0x793DF4  // Purple (default for other modes)
#define DROPLET_COLOR 0x00FF00  // Green
#define DROPLET_COLOR 0xFF0000  // Red
#define DROPLET_COLOR 0x00FFFF  // Cyan
```

**Note**: Changes to effect mode or color require recompiling and uploading the firmware.

### Display Settings
Located in [src/main.cpp](src/main.cpp):

```cpp
#define PANEL_WIDTH 64          // Width of each panel
#define PANEL_HEIGHT 64         // Height of each panel
#define PANELS_NUMBER 2         // Number of chained panels
#define MAX_DROPLETS 128        // Number of simultaneous droplets
```

### Color Cycling
```cpp
uint32_t COLOR_CHANGE_DELAY = 45000;      // Time between color changes (ms)
uint32_t MIN_COLOR_CHANGE_DELAY = 15000;  // Minimum delay
uint32_t MAX_COLOR_CHANGE_DELAY = 60000;  // Maximum delay
bool bColorChangeEnabled = true;          // Enable/disable auto-cycling
```

### Display Brightness
```cpp
dma_display->setBrightness8(220);  // Range: 0-255
```

### Droplet Properties
- **Length**: 10-40 pixels (randomized per droplet)
- **Speed**: 0.07-0.5 pixels/frame (randomized)
- **Fade Rate**: Configurable via `fadeAmt` (currently 16)

## Project Structure

```
DigitalRainMatrix/
├── platformio.ini          # PlatformIO configuration
└── src/
    ├── main.cpp           # Main application code
    └── button.h           # Button input handler
```

## Troubleshooting

### Display Not Working
- Verify all HUB75 connections match pin configuration
- Check power supply (5V, minimum 4A recommended for dual panels)
- Ensure E pin is connected (required for 64x64 panels)

### WiFi Connection Issues
- Verify SSID and password in main.cpp
- Check serial monitor (921600 baud) for connection status
- Ensure 2.4GHz WiFi network (5GHz not supported)

### Flickering Display
- Check power supply capacity
- Verify DMA buffer allocation succeeded (check serial output)
- Try adjusting `setBrightness8()` value

## Serial Monitor

Monitor output at **921600 baud** to see:
- WiFi connection status
- IP address assignment
- UDP color change notifications
- DMA initialization status

## Future Enhancements

- [ ] Button control for manual color changes
- [ ] Multiple animation modes
- [ ] Web interface for configuration
- [ ] MQTT support
- [ ] Speed and density controls
- [ ] Save/load color presets

## License

[Add your license here]

## Acknowledgments

- [FastLED](https://github.com/FastLED/FastLED) - LED animation library
- [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA) - Matrix panel driver
- Inspired by The Matrix digital rain effect

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
