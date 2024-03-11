#include <Arduino.h>
#include <WiFi.h>
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <Wire.h>
#include <FastLED.h>

//ESP-32 S3 DevKitC-1-N8R2
#define R1_PIN_DEFAULT 4
#define G1_PIN_DEFAULT 5
#define B1_PIN_DEFAULT 6
#define R2_PIN_DEFAULT 7
#define G2_PIN_DEFAULT 15
#define B2_PIN_DEFAULT 16 //16
#define A_PIN_DEFAULT  18 //11  //18
#define B_PIN_DEFAULT  8
#define C_PIN_DEFAULT  3
#define D_PIN_DEFAULT  42
#define E_PIN_DEFAULT  21 // required for 1/32 scan panels, like 64x64. 
#define LAT_PIN_DEFAULT 40
#define OE_PIN_DEFAULT  2
#define CLK_PIN_DEFAULT 41

#define R1 R1_PIN_DEFAULT
#define OE OE_PIN_DEFAULT

// #include "Arduino_GFX_Library.h"
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64  	// Panel height of 64 will required PIN_E to be defined.
#define PANELS_NUMBER 1 	
#define PIN_E E_PIN_DEFAULT

MatrixPanel_I2S_DMA *dma_display = nullptr;
CRGB currentColor;
CRGBPalette16 currentPalette = ForestColors_p;

struct droplet
{
  double xPos = -1.0;
  double iPos = 0.0;
  CRGB color = CRGB(0x000000);
  int length = 0;
  double speed = 0.5;
};

#define MAX_DROPLETS 92
droplet droplets[MAX_DROPLETS];
int cnt_droplets = 0;

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
  Serial.println();

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  Serial.println("Wifi disabled");

  HUB75_I2S_CFG mxconfig;
  mxconfig.mx_height = PANEL_HEIGHT;      
  mxconfig.chain_length = PANELS_NUMBER;  
  mxconfig.gpio.e = PIN_E;                // we MUST assign pin e to some free pin on a board to drive 64 pix height panels with 1/32 scan

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  
  dma_display->setBrightness8(192);    // range is 0-255, 0 - 0%, 255 - 100%

  if( not dma_display->begin() )
      Serial.println("****** !KABOOM! I2S memory allocation failed ***********");

  Serial.println("Display configured");
  Serial.printf("PANEL_WIDTH=%d\n", PANEL_WIDTH);

  for(int ndx=0; ndx<MAX_DROPLETS; ndx++)
  {
    droplets[ndx].xPos = random(0, PANEL_WIDTH);
    droplets[ndx].length = random(10, 40);
    droplets[ndx].speed = randfrom(0.01, 0.5); 
    droplets[ndx].color = ColorFromPalette(currentPalette, random(40,85));
    Serial.printf("Drop X(%d)=%d\n", ndx, droplets[ndx].xPos);
  }
}

void loop() {
    const byte fadeAmt = 16;


     for(int ndx=0; ndx<MAX_DROPLETS; ndx++)
     {

      if(droplets[ndx].iPos >= (PANEL_HEIGHT + droplets[ndx].length) )
      {
        int proposed = random(0, PANEL_WIDTH);
        while (droplets[proposed].iPos < (PANEL_HEIGHT/2) && droplets[proposed].xPos > -1.0)
        {
          proposed = random(0, PANEL_WIDTH);
        };
        
        droplets[ndx].xPos = proposed;
        droplets[ndx].iPos = 0.0;
        droplets[ndx].length = random(10, 40);
        droplets[ndx].speed = randfrom(0.01, 0.5); 
        droplets[ndx].color = ColorFromPalette(currentPalette, random(40,85));        
      }

      if(droplets[ndx].xPos > -1)
      {
        droplets[ndx].iPos += droplets[ndx].speed;
        CRGB headColor = ColorFromPalette(currentPalette, 192);
        dma_display->drawPixelRGB888(droplets[ndx].xPos, droplets[ndx].iPos, headColor.r, headColor.g, headColor.b);

        int lengthShowing = droplets[ndx].iPos < droplets[ndx].length ? droplets[ndx].iPos : droplets[ndx].length;
        CRGB tailColor = droplets[ndx].color; 

        //this routine will not completely black out the tail everytime producing a 'trace' effect
        for(int i = 1; i <= lengthShowing; i++)
        {
          dma_display->drawPixelRGB888(droplets[ndx].xPos, droplets[ndx].iPos - i, tailColor.r, tailColor.g, tailColor.b);
          tailColor = tailColor.fadeToBlackBy(fadeAmt);
        }      
          
      }

    }
    delay(20); 
}
