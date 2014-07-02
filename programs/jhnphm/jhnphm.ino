/* Modified by John Pham */
/*
   Copyright (c) 2012, "David Hilton" <dhiltonp@gmail.com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   The views and conclusions contained in the software and documentation are those
   of the authors and should not be interpreted as representing official policies, 
   either expressed or implied, of the FreeBSD Project.
 */



#include <click_counter.h>
#include <print_power.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>
#include <EEPROM.h>



//#define DEBUG DEBUG_PROGRAM
//static const int CLICK = 250; // maximum time for a "short" click
static const int HOLD_TIME = 500; // milliseconds before going to spin
static const int NOCLICK_TIME=500; // time before click == off

//#define DBG(a) a
#if (DEBUG==DEBUG_PROGRAM)
#define DBG(a) a
#else
#define DBG(a)
#endif

#define EEPROM_BRIGHTNESS1 1
#define EEPROM_BRIGHTNESS2 2

#define MODE_OFF 0
#define MODE_ON 1
#define MODE_FLASH 2
#define MODE_SPIN_LEVEL 3

hexbright hb;

int mode = MODE_OFF;
int new_mode = MODE_OFF;
//int last_pressed = 0;
int brightness_level = 0;

void setup() {
    hb = hexbright();
    hb.init_hardware();
    //config_click_count(CLICK);
    brightness_level = EEPROM.read(EEPROM_BRIGHTNESS1) << 8;
    brightness_level |= EEPROM.read(EEPROM_BRIGHTNESS2);
    DBG(Serial.print("Load Brightness:"); Serial.println(brightness_level));
}

byte updateEEPROM(word location, byte value) {
  byte c = EEPROM.read(location);
  if(c!=value) {
    DBG(Serial.print("Write to EEPROM:"); Serial.print(value); Serial.print("to loc: "); Serial.println(location));
    EEPROM.write(location, value);
  }
  return value;
}






void loop() {
    hb.update();

    if(click_count >= MODE_OFF){
        new_mode = click_count();
    }else{
        new_mode = mode;
    }

    //if(new_mode>=MODE_OFF && new_mode!=mode) {
    //    DBG(Serial.print("New mode: "); Serial.println((int)new_mode));
    //    switch(new_mode){
    //        case MODE_OFF:
    //            updateEEPROM(EEPROM_BRIGHTNESS, brightness_level/4);
    //            hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    //            mode = MODE_OFF;
    //            break;
    //        case MODE_ON:
    //            break;
    //        case MODE_FLASH:
    //            break;
    //        case MODE_SPIN_LEVEL:
    //            hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
    //            mode = MODE_OFF;
    //            break;
    //    }
    //    mode = new_mode;
    //}
    switch(mode){
        case MODE_OFF:
            //if(hb.button_pressed() && hb.button_pressed_time() > HOLD_TIME) { //    mode = MODE_SPIN_LEVEL;
            //}else 
            if(hb.button_just_released() && hb.button_pressed_time()<HOLD_TIME) {
                mode = MODE_ON;
            }else{
                hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
            }
            break;
        case MODE_ON:
            hb.set_light(brightness_level, 0, 20); // and pulse (going from max to min over 20 milliseconds)
            if(hb.button_pressed() && hb.button_pressed_time() > HOLD_TIME) { //    mode = MODE_SPIN_LEVEL;
                mode = MODE_OFF;
            }else if(hb.button_just_released() && hb.button_pressed_time()<HOLD_TIME) {
                mode = MODE_SPIN_LEVEL;
            }
            break;
        case MODE_SPIN_LEVEL:
            hb.set_led(RLED, 350, 350, 1);
            hb.set_led(GLED, 350, 350, 1);
            if(abs(hb.difference_from_down()-.5)<.35) { // acceleration is not along the light axis, where noise causes random fluctuations.
                char spin = hb.get_spin();
                brightness_level = brightness_level + spin;
                brightness_level = brightness_level>1000 ? 1000 : brightness_level;
                brightness_level = brightness_level<1 ? 1 : brightness_level;
                hb.set_light(CURRENT_LEVEL, brightness_level, 100);
            }
            if(hb.button_pressed() && hb.button_pressed_time() > HOLD_TIME) { //    mode = MODE_SPIN_LEVEL;
                updateEEPROM(EEPROM_BRIGHTNESS1, brightness_level >> 8);
                updateEEPROM(EEPROM_BRIGHTNESS2, brightness_level & 0xFF);
                mode = MODE_OFF;
            }else if(hb.button_just_released() &&  hb.button_pressed_time()<HOLD_TIME) {
                mode = MODE_FLASH;
            }

            DBG(Serial.print("Load Brightness:"); Serial.println(brightness_level));
            break;
        case MODE_FLASH:
            // held for over HOLD_TIME ms, go to strobe
            static unsigned long flash_time = millis();
            if(flash_time+70<millis()) { // flash every 70 milliseconds
                flash_time = millis(); // reset flash_time
                hb.set_light(MAX_LEVEL, 0, 20); // and pulse (going from max to min over 20 milliseconds)
                // actually, because of the refresh rate, it's more like 'go from max brightness on high
                //  to max brightness on low to off.
            }
            if(hb.button_pressed() && hb.button_pressed_time() > HOLD_TIME) { //    mode = MODE_SPIN_LEVEL;
                mode = MODE_OFF;
            }else if(hb.button_just_released() && hb.button_pressed_time()<HOLD_TIME) {
                mode = MODE_ON;
            } 
            break;
  
    }
    // Low battery
    print_power();
}
