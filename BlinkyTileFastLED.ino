#include <DmxSimple.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_LSM303.h>

#define LED_PIN     0
#define NUM_LEDS    12
#define LED_TYPE    DMXSIMPLE
#define COLOR_ORDER BGR

#define PATTERN_SWITCH_PIN 3
#define BRIGHTNESS_SWITCH_PIN    4

CRGB leds[NUM_LEDS];

unsigned int patternCount = 5;
unsigned int currentPatternIndex = 0;

boolean ignorePatternSwitch = false;
boolean ignoreBrightnessSwitch = false;

// The brightness of our pixels (0 to 255).
// 0 is off.
// 16 is dim.
// 64 is medium.
// 128 is bright.
// 255 is blindingly bright!
byte brightness = 32;

Adafruit_LSM303 lsm;

boolean lsmAvailable = false;

void setup() {
  //  Serial.begin(9600);

  pinMode(PATTERN_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BRIGHTNESS_SWITCH_PIN, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);

  lsmAvailable = lsm.begin();
}

byte hue = 0;
byte currentLed = 0;

void loop() {
  uint16_t delay = 0;

  switch (currentPatternIndex) {
  case 0:
    delay = tilt();
    break;

  case 1:
    delay = colorPaletteExample();
    break;

  case 2:
    delay = solidHueShift();
    break;

  case 3:
    delay = sequenceHueShift();
    break;

  case 4:
    delay = antiAliasedLightBar();
    break;
  }

  FastLED.show();
  FastLED.delay(delay);

  handleInput();
}

void handleInput() {
  if (digitalRead(PATTERN_SWITCH_PIN) == LOW) {
    if (!ignorePatternSwitch) {
      ignorePatternSwitch = true;

      currentPatternIndex++;
      if (currentPatternIndex >= patternCount)
        currentPatternIndex = 0;
    }
  }
  else {
    ignorePatternSwitch = false;
  }

  if (digitalRead(BRIGHTNESS_SWITCH_PIN) == LOW) {
    if (!ignoreBrightnessSwitch) {
      ignoreBrightnessSwitch = true;

      brightness += 32;
      if (brightness < 32)
        brightness = 32;

      FastLED.setBrightness(brightness);
    }
  }
  else {
    ignoreBrightnessSwitch = false;
  }
}

uint16_t solidHueShift() {
  fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
  hue++;
  return 30;
}

uint16_t sequenceHueShift() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  leds[currentLed] = CHSV(hue, 255, 255);
  hue++;
  currentLed++;
  if (currentLed >= NUM_LEDS) {
    currentLed = 0;
  }
  return 120;
}

// half segment size
float s = 2048.0 / 5.0;  // 409.6F
// segment size
float s2 = s * 2.0; // 819.2F

byte topLed = 0;
byte spinIndex = 0;

byte tiltSpin[12][5] = 
{
  { 1, 2, 3, 4, 5 }, // 0
  { 2, 0, 5, 6, 7 }, // 1
  { 3, 0, 1, 7, 8 }, // 2
  { 4, 0, 2, 8, 9 }, // 3
  { 5, 0, 3, 9, 10 }, // 4
  { 6, 1, 0, 4, 10 }, // 5
  { 7, 1, 5, 10, 11 }, // 6
  { 8, 2, 1, 6, 11 }, // 7
  { 9, 3, 2, 7, 11 }, // 8
  { 10, 4, 3, 8, 11 }, // 9
  { 11, 6, 5, 4, 9 }, // 10
  { 10, 9, 8, 7, 6 } // 11
};

uint16_t tilt() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  if(lsmAvailable){
    leds[topLed] += CRGB::Blue;
    leds[tiltSpin[topLed][spinIndex]] += CRGB::Blue;
  }
  else {
    return solidHueShift();
  }

  spinIndex++;
  if(spinIndex >=5)
    spinIndex = 0;

  lsm.read();

  float x = lsm.accelData.x;
  float y = lsm.accelData.y;
  float z = lsm.accelData.z;

  if(z >= 768) {
    // top
    topLed = 0;
  }
  else if (z >= 0) {
    // upper middle

    if(between(x, -1024.0F, -409.6) && between(y, -204.8F, 614.4F))
      topLed = 1;
    else if(between(x, -409.6F, 409.6F) && between(y, 614.4F, 1024.0F))
      topLed = 2;
    else if(between(x, 409.6F, 1024.0F) && between(y, -204.8F, 614.4F))
      topLed = 3;
    else if(between(x, 0.0F, 819.2F) && between(y, -1024.0F, -204.8F))
      topLed = 4;
    else if(between(x, -819.2F, 0.0F) && between(y, -1024.0, -204.8))
      topLed = 5;
  }
  else if (z >= -768) {
    // lower middle

    if(between(x,-1024.0F, -409.6F) && between(y, -614.4F, 204.8F))
      topLed = 6;
    else if(between(x,-819.2F, 0) && between(y, 204.8F, 1024.0F))
      topLed = 7;
    else if(between(x, 0, 819.2F) && between(y, 204.8, 1024))
      topLed = 8;
    else if(between(x,409.6F, 1024.0F) && between(y, -614.4F, 204.8F))
      topLed = 9;
    else if(between(x,-409.6F, 409.6F) && between(y, -1024.2F, -614.4F))
      topLed = 10;
  }
  else // z < -512
  {
    // bottom
    topLed = 11;
  }

  return 60;
}

boolean between(float value, float minV, float maxV) {
  return value >= minV && value <= maxV;
}

int     F16pos = 0; // position of the "fraction-based bar"
int     F16delta = 1; // how many 16ths of a pixel to move the Fractional Bar
uint16_t Fhue16 = 0; // color for Fractional Bar

int Width  = 2; // width of each light bar, in whole pixels

uint16_t antiAliasedLightBar() {
  // Update the "Fraction Bar" by 1/16th pixel every time
  F16pos += F16delta;

  // wrap around at end
  // remember that F16pos contains position in "16ths of a pixel"
  // so the 'end of the strip' is (NUM_LEDS * 16)
  if( F16pos >= (NUM_LEDS * 16)) {
    F16pos -= (NUM_LEDS * 16);
  }

  // Draw everything:
  // clear the pixel buffer
  memset8( leds, 0, NUM_LEDS * sizeof(CRGB));

  // advance to the next hue
  Fhue16 = Fhue16 + 19;

  // draw the Fractional Bar, length=4px
  drawFractionalBar( F16pos, Width, Fhue16 / 256);

  return 40;
}

// Draw a "Fractional Bar" of light starting at position 'pos16', which is counted in
// sixteenths of a pixel from the start of the strip.  Fractional positions are
// rendered using 'anti-aliasing' of pixel brightness.
// The bar width is specified in whole pixels.
// Arguably, this is the interesting code.
void drawFractionalBar( int pos16, int width, uint8_t hue)
{
  int i = pos16 / 16; // convert from pos to raw pixel number
  uint8_t frac = pos16 & 0x0F; // extract the 'factional' part of the position

  // brightness of the first pixel in the bar is 1.0 - (fractional part of position)
  // e.g., if the light bar starts drawing at pixel "57.9", then
  // pixel #57 should only be lit at 10% brightness, because only 1/10th of it
  // is "in" the light bar:
  //
  //                       57.9 . . . . . . . . . . . . . . . . . 61.9
  //                        v                                      v
  //  ---+---56----+---57----+---58----+---59----+---60----+---61----+---62---->
  //     |         |        X|XXXXXXXXX|XXXXXXXXX|XXXXXXXXX|XXXXXXXX |  
  //  ---+---------+---------+---------+---------+---------+---------+--------->
  //                   10%       100%      100%      100%      90%        
  //
  // the fraction we get is in 16ths and needs to be converted to 256ths,
  // so we multiply by 16.  We subtract from 255 because we want a high
  // fraction (e.g. 0.9) to turn into a low brightness (e.g. 0.1)
  uint8_t firstpixelbrightness = 255 - (frac * 16);

  // if the bar is of integer length, the last pixel's brightness is the
  // reverse of the first pixel's; see illustration above.
  uint8_t lastpixelbrightness  = 255 - firstpixelbrightness;

  // For a bar of width "N", the code has to consider "N+1" pixel positions,
  // which is why the "<= width" below instead of "< width".

  uint8_t bright;
  for( int n = 0; n <= width; n++) {
    if( n == 0) {
      // first pixel in the bar
      bright = firstpixelbrightness;
    } 
    else if( n == width ) {
      // last pixel in the bar
      bright = lastpixelbrightness;
    } 
    else {
      // middle pixels
      bright = 255;
    }

    leds[i] += CHSV( hue, 255, bright);
    i++;
    if( i == NUM_LEDS) i = 0; // wrap around
  }
}

// ColorPalette example by Mark Kriegsman: https://github.com/FastLED/FastLED/blob/master/examples/ColorPalette/ColorPalette.ino
uint16_t colorPaletteExample() {
  ChangePalettePeriodically();

  static uint8_t startIndex = 0;
  startIndex++; /* motion speed */

  FillLEDsFromPaletteColors(startIndex);

  return 10;
}

// This example shows several ways to set up and use 'palettes' of colors
// with FastLED.
//
// These compact palettes provide an easy way to re-colorize your 
// animation on the fly, quickly, easily, and with low overhead.
//
// USING palettes is MUCH simpler in practice than in theory, so first just
// run this sketch, and watch the pretty lights as you then read through 
// the code.  Although this sketch has eight (or more) different color schemes,
// the entire sketch compiles down to about 6.5K on AVR.
//
// FastLED provides a few pre-configured color palettes, and makes it
// extremely easy to make up your own color schemes with palettes.
//
// Some notes on the more abstract 'theory and practice' of 
// FastLED compact palettes are at the bottom of this file.

CRGBPalette16 currentPalette = RainbowColors_p;
TBlendType    currentBlending = BLEND;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
  uint8_t secondHand = (millis() / 1000) % 60;
  static uint8_t lastSecond = 99;

  if (lastSecond != secondHand) {
    lastSecond = secondHand;
    if (secondHand == 0)  { 
      currentPalette = HeatColors_p;            
      currentBlending = BLEND; 
    }
    if (secondHand == 5)  { 
      currentPalette = RainbowColors_p;         
      currentBlending = BLEND; 
    }
    if (secondHand == 10) { 
      currentPalette = RainbowStripeColors_p;   
      currentBlending = NOBLEND; 
    }
    if (secondHand == 15) { 
      currentPalette = RainbowStripeColors_p;   
      currentBlending = BLEND; 
    }
    if (secondHand == 20) { 
      SetupPurpleAndGreenPalette();             
      currentBlending = BLEND; 
    }
    if (secondHand == 25) { 
      SetupTotallyRandomPalette();              
      currentBlending = BLEND; 
    }
    if (secondHand == 30) { 
      SetupBlackAndWhiteStripedPalette();       
      currentBlending = NOBLEND; 
    }
    if (secondHand == 35) { 
      SetupBlackAndWhiteStripedPalette();       
      currentBlending = BLEND; 
    }
    if (secondHand == 40) { 
      currentPalette = CloudColors_p;           
      currentBlending = BLEND; 
    }
    if (secondHand == 45) { 
      currentPalette = PartyColors_p;           
      currentBlending = BLEND; 
    }
    if (secondHand == 50) { 
      currentPalette = myRedWhiteBluePalette_p; 
      currentBlending = NOBLEND; 
    }
    if (secondHand == 55) { 
      currentPalette = myRedWhiteBluePalette_p; 
      currentBlending = BLEND; 
    }
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
  for (int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV(random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;

}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
  green, green, black, black,
  purple, purple, black, black,
  green, green, black, black,
  purple, purple, black, black);
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more 
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Red,
  CRGB::Gray,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black
};



// Additional notes on FastLED compact palettes:
//
// Normally, in computer graphics, the palette (or "color lookup table")
// has 256 entries, each containing a specific 24-bit RGB color.  You can then
// index into the color palette using a simple 8-bit (one byte) value.
// A 256-entry color palette takes up 768 bytes of RAM, which on Arduino
// is quite possibly "too many" bytes. 
//
// FastLED does offer traditional 256-element palettes, for setups that
// can afford the 768-byte cost in RAM.
//
// However, FastLED also offers a compact alternative.  FastLED offers
// palettes that store 16 distinct entries, but can be accessed AS IF
// they actually have 256 entries; this is accomplished by interpolating
// between the 16 explicit entries to create fifteen intermediate palette 
// entries between each pair.
//
// So for example, if you set the first two explicit entries of a compact 
// palette to Green (0,255,0) and Blue (0,0,255), and then retrieved 
// the first sixteen entries from the virtual palette (of 256), you'd get
// Green, followed by a smooth gradient from green-to-blue, and then Blue.


