#include <Arduino.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "libraries/Thermometer/Thermometer.h"
#include "libraries/PCD8544/PCD8544.h"

#define NODISABLELCD
#define NOBUZZER
/* Assigned Digital Pins:
    Serial  = 0,1
    LCD     = 3,4,5,6,7
    Buzzer  = 8
    DoorSW  = 3 //int1
    Fan     = 9
    Motor   = 10
    Heater  = 11
    Light   = 13
*/


static PCD8544 lcd (3,4,5,6,7);

volatile int f_wdt=1;
//int sleepStatus = 0;             // variable to store a request for sleep

static Thermometer thermometer(A1,2000,298.15,3700);

const unsigned char pinDoorSW   = 2; //int0
const unsigned char pinFan      = 9;
const unsigned char pinMotor    = 10;
const unsigned char pinHeater   = 11;
const unsigned char pinLight    = 13;
const unsigned char pinBuzzer   = 8;;
const unsigned int buzzerfrequency = 1400;//262;

float desiredtemperature = 20; //should be around 5 to 10 degrees
float temperaturehysterysis = 1.5;

bool tempbool=false;
bool state_fan=false;
bool state_door=false;
bool state_motor=false;
bool state_buzzer=false;
bool state_heater=false;
//int count = 0;                   // counter
unsigned int count_door = 0;
unsigned int count_motor = 0; //used to count the time the motor is on or off
unsigned int count_heater = 0; //used to count the time the heater is on;
unsigned int count_fan = 0; //used to count time time the fan is on;

const unsigned int max_count_fan = 10;

void setupSleep(void) {//init WDT
    MCUSR = 0x00; //clear all reset flags

    WDTCSR |= (1<<WDCE) | (1<<WDE); // In order to change WDE or the prescaler, we need to set WDCE (This will allow updates for 4 clock cycles).


    /* set new watchdog timeout prescaler value */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; /* 16ms */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 0<<WDP1 | 1<<WDP0; /* 32ms */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 1<<WDP1 | 0<<WDP0; /* 64ms */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 1<<WDP1 | 1<<WDP0; /* 0.125 seconds */
    //WDTCSR = 0<<WDP3 | 1<<WDP2 | 0<<WDP1 | 0<<DP0; /* 0.25 seconds */
    //WDTCSR = 0<<WDP3 | 1<<WDP2 | 0<<WDP1 | 1<<WDP0; /* 0.5 seconds */
    WDTCSR = 0<<WDP3 | 1<<WDP2 | 1<<WDP1 | 0<<WDP0; /* 1.0 seconds */
    //WDTCSR = 0<<WDP3 | 1<<WDP2 | 1<<WDP1 | 1<<WDP0; /* 2.0 seconds */
    //WDTCSR = 1<<WDP3 | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; /* 4.0 seconds */
    //WDTCSR = 1<<WDP3 | 0<<WDP2 | 0<<WDP1 | 1<<WDP0; /* 8.0 seconds */

    WDTCSR |= (1<<WDIE);//enable watchdog interrupt
}

ISR(WDT_vect){ //Runs whenever the sleep timer is reached
}

void enterSleep(void){
    sleep_enable();
    sleep_mode();

    /* The program will continue from here after the WDT timeout*/
    sleep_disable(); /* First thing to do is disable sleep. */

    //power_all_enable();
    //power_adc_enable();
}

void buzzer(){
    #ifndef NOBUZZER
        tone(pinBuzzer,buzzerfrequency,1000/4);
        delay(1000/4);
        //tone(pinBuzzer,buzzerfrequency,1000/4);
        //delay(1000/4);
    #endif
}

void updateOutputs(){ //change the states of the heater, fan and motor
    digitalWrite(pinLight, state_door);
    digitalWrite(pinMotor,state_motor);
    digitalWrite(pinFan,state_fan);
    digitalWrite(pinHeater,state_heater);
}

void lcdupdate(){
    String displaystr;

    //lcd.clear();
    lcd.setCursor(0, 0); lcd.clearLine();
    lcd.print("Temp: ");
    lcd.print(thermometer.temperature ,2);
    lcd.println("'C");

    lcd.setCursor(0, 1);lcd.clearLine();

    if (state_fan) lcd.print("FAN: ON"); else lcd.print("FAN: OFF");

    lcd.setCursor(0, 2);lcd.clearLine();
    if (state_motor) lcd.print("Motor: ON"); else lcd.print("Motor: OFF");

    lcd.setCursor(0, 3);lcd.clearLine();
    if (state_heater) lcd.print("Heater: ON"); else lcd.print("Heater: OFF");

    lcd.setCursor(0, 4);lcd.clearLine();
    lcd.print("Count_fan: "); lcd.print(count_fan);

    lcd.setCursor(0, 5);lcd.clearLine();
    lcd.print("Count_door: "); lcd.print(count_door);
}


void doorTrigger(){
    state_door=!digitalRead(pinDoorSW);
    count_door=0;
    if (state_door) {
        lcd.setPower(state_door); //Powers on the LCD if the door opens
        lcdupdate();
        state_fan = false;
        count_fan = 0;
    }

    updateOutputs();
}

void checkTemperature(){
    static float oldadc;
    /* This code needs to be revised. Causes conflict with the other operations of the fan, motor and heater
    if (thermometer.temperature >= (desiredtemperature + (temperaturehysterysis / 2))){
        state_fan = true;
        state_motor = true;
        state_heater = false;
        delay(5);
    }
    if (thermometer.temperature <= (desiredtemperature - (temperaturehysterysis / 2))){
        state_fan = false;
        state_motor = false;
        //state_heater = true;
        delay(5);
    }
    */
    oldadc = thermometer.ADCvalue;
}

void activatefan(){
    state_fan = true;
    count_fan = 0;
}

void setup(){
    EIMSK != _BV(INT0);
    sei();
    #ifdef SERIAL_DEBUG
        pinMode(2, OUTPUT);
        Serial.begin(SERIAL_DEBUG_SPEED);
    #else
        lcd.begin(84, 48);
    #endif

    setupSleep();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);     // sleep mode is set here Power Down uses the least current
    /*Check these
    power_spi_disable();
    power_timer1_disable(); //Timer 0 will also stop WDT, which I need
    power_timer2_disable();
    power_twi_disable();
    */

    //Setup pins
    delay(10);
    pinMode(pinDoorSW,INPUT);
    pinMode(pinLight,OUTPUT);
    pinMode(pinMotor,OUTPUT);
    pinMode(pinHeater,OUTPUT);
    pinMode(pinFan,OUTPUT);

    attachInterrupt(0,doorTrigger,CHANGE);
    doorTrigger(); //read the door state on power on
}

void loop(){
    f_wdt=0;

    if (state_door){ //Door open
        if (count_door < 10) {
            count_door++;
        } else {
            state_buzzer = true;
            lcd.setInverse(false);
        }
    } else { //Door closed
        state_buzzer=false; //always make it false
        switch (count_door){
            case 6:
                #ifndef NODISABLELCD
                    lcd.setPower(false);
                #else
                    lcd.setInverse(true);
                #endif
                activatefan();
                count_door++;
                break;
            case 65530 :
                count_door = 7;
                break;
            default:
                count_door++;
        }
    }

    if (state_buzzer) buzzer();

    if (state_fan){
        state_fan = (count_fan < max_count_fan); //change the state of the fan if the fan is on longer than allowed
        count_fan++;
    }


    thermometer.update();
    checkTemperature();
    updateOutputs();
    lcdupdate();

    enterSleep();
}
