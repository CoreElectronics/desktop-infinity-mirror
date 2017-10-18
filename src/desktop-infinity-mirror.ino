/*
 * v1.1
 *
 * This program drives the Core Electronics Desktop Infinity Mirror Kit
 * Powered by Core Electronics
 * 2017
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Particle.h"
#include <math.h>
#include <neopixel.h>

// SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_MODE(AUTOMATIC);


/*******************************************************************************
 * Hardware Definitions
 *
 ******************************************************************************/

// LED strip
const int strip_pin = 0; // The digital pin that drives the LED strip
const int num_leds = 44; // Number of LEDs in the strip. Shouldn't need changing unless you hack the hardware

const int pot_1 = 0; // Potentiometer pin selects the mode

const int ADC_precision = 4095; // Particle use 12bit ADCs. If you wish to port to a different platform you might need to redefine the ADC precision eg: 1023 for Arduino UNO



/*******************************************************************************
 * Global Variables
 *
 ******************************************************************************/

// States for the state-machine
enum statevar {
    state_off,
    state_rainbow,
    state_comet,
    state_solid,
    state_nightlight,
    state_soft
};

uint8_t state; // The state that the user demands
uint8_t state_current; // The state currently being executed
Adafruit_NeoPixel strip(num_leds, strip_pin);
uint32_t ledBuffer[num_leds]; // Buffer for storing (and then scaling if necessary) LED R,G,B values.


/*******************************************************************************
 * SETUP
 *
 ******************************************************************************/

void setup()
{
    strip.begin();
    strip.show(); // Initialize all pixels to off
}



/*******************************************************************************
 * LOOP
 *
 ******************************************************************************/

void loop()
{


    if (System.buttonPushed() > 1) {
        if( !Particle.connected() ){
            Particle.connect();
        }
    }


    // Read potentiometer values for user-input
    state = getState(pot_1);    // Select the operation mode
    //int opt1 = analogRead(pot_2);   // Select animation speed
    //int opt2 = analogRead(pot_3);   // A general-purpose option for other effects
    int opt1 = 1023;
    int opt2 = 1023;


    // State Machine
    switch(state){
        case state_off:
            clearStrip();   // "Off" state.
            break;

        case state_rainbow:
            rainbow(); // Adafruit's rainbow demo, modified for seamless wraparound. We are passing the Pot # instead of the option because delay value needs to be updated WITHIN the rainbow function. Not just at the start of each main loop.
            break;

        case state_comet:
            demo(); // An under-construction comet demo.
            break;

        case state_solid:
            solid(opt1, opt2); // Show user-set solid colour.
            break;

        default:
            break;

    }
}


/*******************************************************************************
 * Functions
 *
 ******************************************************************************/

// Break potentiometer rotation into four sectors for setting mode
uint8_t getState(int pot){
    float val = analogRead(pot);
    if (val < ADC_precision / 4) {
        return state_off;
    } else if (val < ADC_precision / 2) {
        return state_rainbow;
    } else if (val < 3*ADC_precision / 4) {
        return state_comet;
    } else {
        return state_solid;
    }
}


// Convert an ADC reading into a 0-100ms delay
int getDelay(int pot){
    float potVal = analogRead(pot);
    return map(potVal,0,ADC_precision,100,0);
}


/* Run the comet demo
 * This feature is largely experimental and quite incomplete.
 * The idea is to involve multiple comets that can interact by colour-addition
 */
void demo(void){
    state_current = state_comet;
    uint16_t i, j, k;
    uint16_t ofs = 15;

    for (j=0; j<strip.numPixels(); j++){
        clearStrip();

        comet(j,1);

        strip.show();
        delay(30);
        if(getState(pot_1) != state_current) break; // Check if mode knob is still on this mode
    }
}


/*
 * Draw a comet on the strip and handle wrapping gracefully.
 * Arguments:
 *      - pos: the pixel index of the comet's head
 *      - dir: the direction that the tail should point
 *
 * TODO:
 *      - Handle direction gracefully. In the works but broken.
 *      - Handle multiple comets
 */
void comet(uint16_t pos, bool dir) {
    float headBrightness = 255;                 // Brightness of the first LED in the comet
    uint8_t bright = uint8_t(headBrightness);   // Initialise the brightness variable
    uint16_t len = 20;                          // Length of comet tail
    double lambda = 0.3;                        // Parameter that effects how quickly the comet tail dims
    double dim = lambda;                        // initialise exponential decay function

    strip.setPixelColor(pos, strip.Color(0,bright,0)); // Head of the comet


    if(dir) {
        for(uint16_t i=1; i<len; i++){
            // Figure out if the current pixel is wrapped across the strip ends or not, light that pixel
            if( pos - i < 0 ){ // Wrapped
                strip.setPixelColor(strip.numPixels()+pos-i, strip.Color(0,bright,0));
            } else { // Not wrapped
                strip.setPixelColor(pos-i, strip.Color(0,bright,0));
            }
            bright = uint8_t(headBrightness * exp(-dim)); // Exponential decay function to dim tail LEDs
            dim += lambda;
        }

    } else { // Comet is going backwards *** BROKEN: TODO fix ***
        for(uint16_t i=1; i<len; i++){
            // Figure out if the current pixel is wrapped across the strip ends or not, light that pixel
            if( pos + i > strip.numPixels() ){ // Wrapped
                strip.setPixelColor(strip.numPixels()-pos-i, strip.Color(0,bright,0));
            } else { // Not wrapped
                strip.setPixelColor(pos+i, strip.Color(0,bright,0));
            }
            // Dim the tail of the worm. This probably isn't the best way to do it, but it'll do for now.
            // TODO: dim while respecting the length of the worm. For long worms this will dim to zero before the end of worm is reached.
            bright *= 0.75;
        }
    }
}


void clearStrip(void){
    uint16_t i;
    for(i=0; i<strip.numPixels(); i++){
            strip.setPixelColor(i, strip.Color(0,0,0));
        }
        strip.show();
        delay(1);
}


void rainbow() {
    state_current = state_rainbow;
//   uint16_t j;
  float i, baseCol;
  float colStep = 256.0 / strip.numPixels();

  for(baseCol=0; baseCol<256; baseCol++) { // Loop through all colours
    for(i=0; i<strip.numPixels(); i++) {   // Loop through all pixels
        // strip.setPixelColor( i, Wheel(int(i*(colStep)+baseCol) & 255) ); // This line seamlessly wraps the colour around the table.
        setPixel( i, Wheel(int(i*(colStep)+baseCol) & 255) ); // This line seamlessly wraps the colour around the table.
    }
    // strip.show();
    update();
    delay(10);

    if(getState(pot_1) != state_current) break; // Check if mode knob is still on this mode
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


// Display a single solid colour from the Wheel(), or show white with variable brightness
int solid(int colour, int bright){
    state_current = state_solid;
    bright = map(bright,0,ADC_precision,5,255);
    int col = map(colour,0,ADC_precision,0,255);
    uint16_t i;

    if (col > 245) {
        // Set to white
        for(i=0; i<strip.numPixels(); i++){
            strip.setPixelColor(i, strip.Color(bright,bright,bright));
        }

    } else {
        // User-defined colour
        for(i=0; i<strip.numPixels(); i++){
                // strip.setPixelColor(i, strip.Color(128,128,128)); // half bright white. DO NOT EXCEED
                setPixel(i, strip.Color(255,255,255)); // protected level
        }
    }
    // strip.show();
    update();
    delay(50);
}







/**
 * Current-Limiting code
 * As it stands, if the user manually drives the LED strip, there exists the ability to drive the strip to ~1.5 A.
 * The LED strip is powered from the Vin pin, which can supply only 1.0 A.
 * The following code serves as wrappers around Adafruit's NeoPixel function calls that scales the LED values used
 * to come in under this current limit.
 *
 */

// Wrapper for safe pixel updating
void setPixel(int ledIndex, uint32_t colour){
  ledBuffer[ledIndex] = colour;
}

// Wrapper for safe pixel updating
void update(){
  // const float iLim = 0.87; // [A] Current limit (0.9A) for external power supply
  const float iLim = 0.35; // [A] Current limit for PC USB port
  const float FSDcurrentCorrection = 0.8824; // "Full-scale deflection" correction. The LED response is nonlinear i.e. Amp/LeastSignificantBit is not a constant. This is an attempt to allow the user to specify maximum current as a real value.
  float lsbToAmp = 5.06e-5; // [LSB/Ampere] the relationship between an LED setting and current
  float sum = 0; // Initial sum of currents
  uint8_t R; uint8_t G; uint8_t B;

  // Sum the LED currents
  for(uint8_t i=0; i<strip.numPixels(); i++) {
    // Separate the 32bit colour into 8bit R,G,B and add
    B = ledBuffer[i] & 0xFF;
    G = (ledBuffer[i] >> 8) & 0xFF;
    R = (ledBuffer[i] >> 16) & 0xFF;

    sum += float(R + G + B) * lsbToAmp; // Add LED[i]'s current
  }
  sum = sum * FSDcurrentCorrection;
  float scale = float(iLim)/float(sum);
  // DEBUG
//  Serial.print("Requested: ");
//  Serial.print(sum);
//  Serial.println(" A");

  if ( sum > iLim ) { // Too much current requested
    for(uint8_t i=0; i<strip.numPixels(); i++) {
      // Separate the 32bit colour into 8bit R,G,B and add
      B = ledBuffer[i] & 0xFF;
      G = (ledBuffer[i] >> 8) & 0xFF;
      R = (ledBuffer[i] >> 16) & 0xFF;

      R = floor(R * scale);
      G = floor(G * scale);
      B = floor(B * scale);

      strip.setPixelColor(i, R, G, B);
    }
  } else {
    for(uint8_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, R, G, B);
    }
  }
  strip.show();
}
