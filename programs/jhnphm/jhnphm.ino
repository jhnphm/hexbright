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


#include <EEPROMWearLeveler.h>

#define DEBUG DEBUG_PROGRAM

#include <click_counter.h>
#include <print_power.h>

// These next two lines must come after all other library #includes
#define BUILD_HACK
#include <hexbright.h>



//static const int CLICK = 250; // maximum time for a "short" click
static const int HOLD_TIME = 350; // milliseconds before going to spin
static const int CLICK_TIME = 200; // time before click == off

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
#define MODE_SW 4
#define MODE_HOLD 5
#define MODE_NIGHTLIGHT 7

hexbright hb;
EEPROMWearLeveler eepromwl(1024, 2); //class(eeprom_size, num_of_addresses) 1024/2 = 512x more read/write cycles


byte mode = MODE_OFF;
int brightness_level = 0;
boolean ignore_held = false;

void setup() {
//    eepromwl.clear();
    hb = hexbright();
    hb.init_hardware();
    //config_click_count(CLICK);
    brightness_level = eepromwl.read(EEPROM_BRIGHTNESS1) << 8;
    brightness_level |= eepromwl.read(EEPROM_BRIGHTNESS2);
    DBG(Serial.print("Load Brightness:"); Serial.println(brightness_level));
    config_click_count(CLICK_TIME);
}

byte updateEEPROM(word location, byte value) {
    byte c = eepromwl.read(location);
    if(c!=value) {
        DBG(Serial.print("Write to EEPROM: "); Serial.print(value); Serial.print(" to loc: "); Serial.println(location));
        eepromwl.write(location, value);
    }
    return value;
}

//void sw_start(){
//    mode = MODE_SW;
//    click_count = 0;
//}


boolean button_held(){
    if(ignore_held){
        return false;
    }else{
        return hb.button_pressed() && hb.button_pressed_time()>HOLD_TIME;
    }
}

boolean button_clicked(){
    return hb.button_just_released() && hb.button_pressed_time()<HOLD_TIME;
}

void loop() {
    static long t_mode_sw;
    int clicks = click_count();
    hb.update();

    if(hb.button_just_released()){
        ignore_held = false;
    }

    switch(mode){
        case MODE_SW:
            hb.set_light(CURRENT_LEVEL, 0, NOW);
            DBG(Serial.print("t_mode_sw: "); Serial.println(t_mode_sw));
            DBG(Serial.print("millis: "); Serial.println(millis()));

            if (millis() - t_mode_sw > CLICK_TIME*5){ //if bouncy switch weirdness puts us here forever, exit and go to on
                mode = MODE_ON;  
                t_mode_sw = millis();
            }else if (clicks < 0){ // Clicks not settled
                if(hexbright::get_led_state(RLED) == LED_OFF) {
                    hexbright::set_led(RLED, 50, 50);
                }
            }else{
                switch(clicks){
                    case 1: 
                        mode = MODE_OFF;
                        break;
                    case 2:
                        mode = MODE_SPIN_LEVEL;
                        break;
                    case 3:
                        mode = MODE_NIGHTLIGHT;
                        break;
                    default:
                        mode = MODE_ON;
                }
                DBG(Serial.print("Click Count: "); Serial.println(clicks));
            }
            break;
        case MODE_OFF:
            if(hb.button_pressed()) { 
                mode = MODE_HOLD;
            }else{
                hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
            }
            break;
        case MODE_HOLD:
            if(button_clicked()) { //    mode = MODE_SPIN_LEVEL;
                mode = MODE_ON;
            }else if(hb.button_just_released()) {
                mode = MODE_OFF;
            }else{
                hb.set_light(CURRENT_LEVEL, brightness_level, NOW); 
            }
            break;
        case MODE_ON:
            hb.set_light(CURRENT_LEVEL, brightness_level, NOW); 
            if(button_held()) { //    mode = MODE_SPIN_LEVEL;
                mode = MODE_FLASH;
            }else if(button_clicked()) {
                mode = MODE_SW;
                t_mode_sw = millis();
            }
            break;
        case MODE_SPIN_LEVEL:
            if(hexbright::get_led_state(RLED) == LED_OFF) {
                hexbright::set_led(RLED, 350, 350);
            }

            if(abs(hb.difference_from_down()-.5)<.35) { // acceleration is not along the light axis, where noise causes random fluctuations.
                char spin = hb.get_spin();
                brightness_level = brightness_level + spin;
                brightness_level = brightness_level>1000 ? 1000 : brightness_level;
                brightness_level = brightness_level<1 ? 1 : brightness_level;
                hb.set_light(CURRENT_LEVEL, brightness_level, 100);
            }
            if(button_held()) { //    mode = MODE_SPIN_LEVEL;
                DBG(Serial.print("Save Brightness: "); Serial.println(brightness_level));
                updateEEPROM(EEPROM_BRIGHTNESS1, brightness_level >> 8);
                updateEEPROM(EEPROM_BRIGHTNESS2, brightness_level & 0xFF);
                mode = MODE_ON;
                ignore_held = true;
            }else if(button_clicked()) {
                mode = MODE_ON;
            }
            DBG(Serial.print("Brightness: "); Serial.println(brightness_level));

            break;
        case MODE_NIGHTLIGHT:
            if(!hb.low_voltage_state())
            hb.set_led(RLED, 100, 0, 255);
            if(button_clicked()) {
                mode = MODE_ON;
            }
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
            if(hb.button_just_released()) {
                mode = MODE_ON;
            } 
            break;

    }
    // Low battery
    print_power();
}
