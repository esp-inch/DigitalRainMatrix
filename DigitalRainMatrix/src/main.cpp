#include <Arduino.h>
#include <WiFi.h>
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <Wire.h>
#include <FastLED.h>
#include "AsyncUDP.h"

// ============================================================================
// EFFECT CONFIGURATION - Change these to customize the display behavior
// ============================================================================
// NOTE: These can be overridden by platformio.ini build environments
// Use Esp32-S3-WintryMix, Esp32-S3-WaterOnly, or Esp32-S3-SleetOnly environments
// to automatically configure the effect mode without editing this file.
// ============================================================================

// Effect Mode Options:
// 0 = WINTRY_MIX  - Temperature-based switching between water and sleet
// 1 = WATER_ONLY  - Always water droplets with tails, ignores temperature
// 2 = SLEET_ONLY  - Always sleet droplets (no tails), ignores temperature
#ifndef EFFECT_MODE
  #define EFFECT_MODE 0  // Default to WINTRY_MIX if not set by build environment
#endif

// Primary color for droplets (RGB hex format)
// WINTRY_MIX default: 0x8AF9FF (ice blue: R=138, G=249, B=255)
// Other examples: 0x793DF4 (purple), 0x00FF00 (green), 0xFF0000 (red), 0x00FFFF (cyan)
#ifndef DROPLET_COLOR
  #if EFFECT_MODE == 0
    #define DROPLET_COLOR 0x8AF9FF  // Ice blue for wintry mix
  #else
    #define DROPLET_COLOR 0x793DF4  // Purple for other modes
  #endif
#endif

// Head color for droplet (the bright tip)
// Default: 0xFFFFFF (white)
// Other examples: 0xFFFF00 (yellow), 0x00FFFF (cyan), 0xFF00FF (magenta)
#ifndef HEAD_COLOR
  #define HEAD_COLOR 0xFFFFFF  // White head
#endif

// Temperature simulation settings (only used in WINTRY_MIX mode)
#ifndef TEMP_CHANGE_INTERVAL
  #define TEMP_CHANGE_INTERVAL 3000  // How often temperature changes (milliseconds)
#endif
#ifndef START_TEMP
  #define START_TEMP 33              // Starting temperature (32-34°F)
#endif

// ============================================================================

//ESP-32 S3 DevKitC-1-N8R2
#ifdef ESP32_S3 
  #define R1_PIN_DEFAULT 4
  #define G1_PIN_DEFAULT 5
  #define B1_PIN_DEFAULT 6
  #define R2_PIN_DEFAULT 7
  #define G2_PIN_DEFAULT 15
  #define B2_PIN_DEFAULT 16 //16
  #define A_PIN_DEFAULT  18
  #define B_PIN_DEFAULT  8
  #define C_PIN_DEFAULT  3
  #define D_PIN_DEFAULT  42
  #define E_PIN_DEFAULT  21 // required for 1/32 scan panels, like 64x64. 
  #define LAT_PIN_DEFAULT 40
  #define OE_PIN_DEFAULT  2
  #define CLK_PIN_DEFAULT 41
#else 
    #define R1_PIN_DEFAULT  25
    #define G1_PIN_DEFAULT  26
    #define B1_PIN_DEFAULT  27
    #define R2_PIN_DEFAULT  14
    #define G2_PIN_DEFAULT  12
    #define B2_PIN_DEFAULT  13

    #define A_PIN_DEFAULT   23
    #define B_PIN_DEFAULT   22
    #define C_PIN_DEFAULT   5
    #define D_PIN_DEFAULT   17
    #define E_PIN_DEFAULT   32 // IMPORTANT: Change to a valid pin if using a 64x64px panel.
              
    #define LAT_PIN_DEFAULT 4
    #define OE_PIN_DEFAULT  15
    #define CLK_PIN_DEFAULT 16
    
#endif  
#define R1 R1_PIN_DEFAULT
#define OE OE_PIN_DEFAULT

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64  	// Panel height of 64 will required PIN_E to be defined.
#define PANELS_NUMBER 2 	
#define PIN_E E_PIN_DEFAULT


const char* ssid = "";
const char* pass = "";
const int rele = 48;
AsyncUDP udp;

MatrixPanel_I2S_DMA *dma_display = nullptr;
CRGB currentColor;
CRGBPalette16 currentPalette = CloudColors_p;

// put function declarations here:

#define RBit 3
#define GBit 4
#define BBit 5

// Droplet types
enum DropletType {
  WATER = 0,
  SLEET = 1
};

struct droplet
{
  float xPos = -1.0;
  float iPos = 0.0;
  CRGB color = CRGB(0x000000);
  float length = 0;
  float speed = 0.5;
  DropletType type = WATER;
};

uint32_t lastDisplayColorChange;
uint32_t COLOR_CHANGE_DELAY = 45000; // in milliseconds
uint32_t MIN_COLOR_CHANGE_DELAY = 15000; // in milliseconds
uint32_t MAX_COLOR_CHANGE_DELAY = 60000; // in milliseconds
bool     bColorChangeEnabled = false;

// Temperature control variables (only used in WINTRY_MIX mode)
int temperature = START_TEMP;              // Current temperature
uint32_t lastTemperatureChange = 0;        // Last time temperature changed
uint32_t TEMP_CHANGE_DELAY = TEMP_CHANGE_INTERVAL;  // Change temperature interval
const int MIN_TEMP = 32;                   // Minimum temperature (freezing)
const int MAX_TEMP = 34;                   // Maximum temperature

#define MAX_DROPLETS 128 //92
droplet droplets[MAX_DROPLETS];
int cnt_droplets = 0;
CRGB primaryColor;

#define CTRL_GPIO_BUTTON 36

const byte fadeAmt = 16;

//Found this routine to get a random 'double'
double randfrom(double min, double max) 
{
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void setup()
{
  Serial.begin(115200);
  delay(100);  // Give serial time to initialize
  Serial.println("\n\n=== DigitalRainMatrix Starting ===");
  Serial.flush();



#ifdef ESP32_S3
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to WiFi: %s\n", ssid);
  Serial.flush();
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.flush();
  }
  Serial.println("\nWiFi Connected!");
  Serial.flush();
#else
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println("Wifi disabled");
  Serial.flush();

#endif  

  HUB75_I2S_CFG mxconfig(
    PANEL_WIDTH,   // module width
    PANEL_HEIGHT,   // module height
    PANELS_NUMBER    // Chain length
  );

  mxconfig.gpio.e = PIN_E;
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6124;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  
  dma_display->setBrightness8(220);    // range is 0-255, 0 - 0%, 255 - 100%

  if( not dma_display->begin() ) {
      Serial.println("****** !KABOOM! I2S memory allocation failed ***********");
      Serial.flush();
  }

  Serial.println("Display configured");
  Serial.printf("PANEL_WIDTH=%d\n", PANEL_WIDTH);
  Serial.flush();

  // Set primary color from configuration
  primaryColor = CRGB(DROPLET_COLOR);

  // Display effect mode
  #if EFFECT_MODE == 0
    Serial.println("Effect Mode: WINTRY_MIX (temperature-based)");
  #elif EFFECT_MODE == 1
    Serial.println("Effect Mode: WATER_ONLY (always with tails)");
  #elif EFFECT_MODE == 2
    Serial.println("Effect Mode: SLEET_ONLY (no tails)");
  #endif
  Serial.printf("Droplet Color: 0x%06X\n", DROPLET_COLOR);
  Serial.flush();

#ifdef ESP32_S3
  if (udp.listen(21324)) {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());
    Serial.flush();
    udp.onPacket([](AsyncUDPPacket packet) {

      uint8_t rcol = packet.data()[RBit];
      uint8_t gcol = packet.data()[GBit];
      uint8_t bcol = packet.data()[BBit];
      Serial.printf("UDP Color Change: R=%u G=%u B=%u\n", rcol, gcol, bcol);
      Serial.flush();
      primaryColor = CRGB(rcol, gcol, bcol);
      lastDisplayColorChange = millis();

    });
  }
#endif  


  for(int ndx=0; ndx<MAX_DROPLETS; ndx++)
  {
    droplets[ndx].xPos = random(0, PANEL_WIDTH * PANELS_NUMBER);
    droplets[ndx].length = random(10, 30);
    droplets[ndx].speed = randfrom(0.07, 0.5);
    droplets[ndx].color = primaryColor;

    // Initialize droplet type based on effect mode
    #if EFFECT_MODE == 0  // WINTRY_MIX
      droplets[ndx].type = (temperature <= MIN_TEMP) ? SLEET : WATER;
    #elif EFFECT_MODE == 1  // WATER_ONLY
      droplets[ndx].type = WATER;
    #elif EFFECT_MODE == 2  // SLEET_ONLY
      droplets[ndx].type = SLEET;
    #endif

    //Serial.printf("Drop X(%d)=%d\n", ndx, droplets[ndx].xPos);
  }
  lastDisplayColorChange = millis();
  lastTemperatureChange = millis();
}

CRGB ColorFraction(CRGB colorIn, float fraction)
{
  fraction = min(1.0f, fraction);
  return CRGB(colorIn).fadeToBlackBy(255 * (1.0f - fraction));
}


void drawDroplet(int dropletIndex, float fPos)
{
  droplet* curDroplet = &droplets[dropletIndex];
  float availFirstPixel = 1.0f - (fPos - (long)(fPos));
  float amtFirstPixel = availFirstPixel;//min(availFirstPixel, droplets[dropletIndex].length);
  float remaining = curDroplet->length;//min(droplets[dropletIndex].length, PANEL_HEIGHT - fPos);
  int iPos = fPos;
 

  if(remaining > 0.0f)
  {
    CRGB headColor = CRGB(HEAD_COLOR);

    CRGB partialColor = ColorFraction(headColor, amtFirstPixel);

    dma_display->drawPixelRGB888(curDroplet->xPos, iPos++, partialColor.r, partialColor.g, partialColor.b);
    dma_display->drawPixelRGB888(curDroplet->xPos, iPos++, headColor.r, headColor.g, headColor.b);

  }

  // Only draw tail for WATER droplets, skip for SLEET
  if (curDroplet->type == WATER)
  {
    CRGB tailColor = curDroplet->color;
    int fadeAmount = 255 / (curDroplet->length);


    int ypos = curDroplet->iPos;
    while(remaining > 1.0f)
    {
      ypos--;
      dma_display->drawPixelRGB888(curDroplet->xPos, ypos, tailColor.r, tailColor.g, tailColor.b);
      tailColor = tailColor.fadeToBlackBy(fadeAmt);

      remaining--;

    }
  }
}

void cyclePrimaryColor()
{
  if(primaryColor.r > 0 && primaryColor.b == 0)
  {
    if(primaryColor.g == 0)
    {
      primaryColor.r = 255;
      primaryColor.g = 255;
    }
    else
    {
      primaryColor.g = 255;
      primaryColor.r = 0;
    }

    primaryColor.b = 0;

    return;
  }

  if(primaryColor.g > 0)
  {
    if(primaryColor.b == 0)
    {
      primaryColor.b = 255;
      primaryColor.g = 255;
    }
    else
    {
      primaryColor.b = 255;
      primaryColor.g = 0;
    }

    primaryColor.r = 0;
    
    return;
  }        

  
  if(primaryColor.b > 0)
  {
    if(primaryColor.r == 0)
    {
      primaryColor.r = 255;
      primaryColor.b = 255;
    }
    else
    {
      primaryColor.r = 255;
      primaryColor.b = 0;
    }

    primaryColor.g = 0;
    
    return;

  }        

}


void loop() {

    if (millis() - lastDisplayColorChange > COLOR_CHANGE_DELAY && bColorChangeEnabled)
    {
      cyclePrimaryColor();

      lastDisplayColorChange = millis();
    }

    // Temperature update logic (only for WINTRY_MIX mode)
    #if EFFECT_MODE == 0
    if (millis() - lastTemperatureChange > TEMP_CHANGE_DELAY)
    {
      // Randomly increase or decrease temperature by 1
      int change = random(0, 2) == 0 ? -1 : 1;
      temperature += change;

      // Clamp temperature to min/max range
      if (temperature < MIN_TEMP) temperature = MIN_TEMP;
      if (temperature > MAX_TEMP) temperature = MAX_TEMP;

      lastTemperatureChange = millis();
      Serial.printf("Temperature: %d°F %s\n", temperature,
                    temperature <= MIN_TEMP ? "(SLEET)" : "(WATER)");
      Serial.flush();
    }
    #endif

    for(int ndx=0; ndx<MAX_DROPLETS; ndx++)
    {
      droplet* curDroplet = &droplets[ndx];
      if(curDroplet->iPos >= (PANEL_HEIGHT + curDroplet->length) )
      {
        int proposed = random(0, PANEL_WIDTH * PANELS_NUMBER);


        curDroplet->xPos = proposed;
        curDroplet->iPos = 0.0;
        curDroplet->length = random(10, 40);

        curDroplet->speed = randfrom(0.07, 0.5);

        // Set droplet type based on effect mode
        #if EFFECT_MODE == 0  // WINTRY_MIX
          if (temperature <= MIN_TEMP) {
            curDroplet->type = SLEET;  // Freezing: generate sleet
          } else {
            curDroplet->type = WATER;  // Above freezing: generate water
          }
        #elif EFFECT_MODE == 1  // WATER_ONLY
          curDroplet->type = WATER;
        #elif EFFECT_MODE == 2  // SLEET_ONLY
          curDroplet->type = SLEET;
        #endif

        long saturationVariance = random(-92, 92);

        if(saturationVariance >= 0 )
        {
          CRGB newColor = primaryColor;
          curDroplet->color = newColor.addToRGB(saturationVariance);
        }
        else
        {
          CRGB newColor = primaryColor;
          curDroplet->color = newColor.subtractFromRGB(saturationVariance * -1);
        }

        curDroplet->color = primaryColor;
      }

      if(curDroplet->xPos > -1)
      {
        curDroplet->iPos += curDroplet->speed;
        drawDroplet(ndx, curDroplet->iPos);
      }
    }
    delay(20); 
}
