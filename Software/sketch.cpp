#include <Arduino.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include "libraries/Thermometer/Thermometer.h"

#ifdef USELCD
#include "libraries/PCD8544/PCD8544.h"
static PCD8544 lcd (3,4,5,6,7);
#endif

//#define SERIAL_DEBUG


volatile int f_wdt=1;
//int sleepStatus = 0;             // variable to store a request for sleep

static Thermometer thermometer(A1,2000,298.15,3700); //Use pin A1 to get temperature.

const unsigned char pinDoorSW   = 2; //int0
const unsigned char pinFan      = 13;
const unsigned char pinMotor    = 9;
const unsigned char pinHeater   = 11;
const unsigned char pinLight    = 10;
const unsigned char pinBuzzer   = 8;//8;
const unsigned int buzzerfrequency = 1400;//262;

unsigned char temperature_dial = 0;
unsigned char temperature_dial_divider = 25;
float temperature_desired;
float temperature_desired_max = 10; //should be around 5 to 10 degrees
float temperature_desired_min = 4; //should be around 5 to 10 degrees
const float temperature_hysterysis = 1;

#ifdef SERIAL_DEBUG
const unsigned int heater_max_time = 40;
const unsigned int motor_min_time = 50; //3 min
const unsigned int door_open_max = 20;
#else
const unsigned int heater_max_time = 80;
const unsigned int motor_min_time = 360; //3 min
const unsigned int door_open_max = 120; //1 Minute
#endif

//bool tempbool=false;
bool state_fan=false;
bool state_door=false;
bool state_motor=false;
bool state_buzzer=false;
bool state_heater=false;
//int count = 0;                   // counter
unsigned int count_door = 0;
unsigned int cooling_state_time_since_change=0;

void setupSleep(void) {//init WDT
    MCUSR = 0x00; //clear all reset flags

    WDTCSR |= (1<<WDCE) | (1<<WDE); // In order to change WDE or the prescaler, we need to set WDCE (This will allow updates for 4 clock cycles).


    /* set new watchdog timeout prescaler value */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; /* 16ms */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 0<<WDP1 | 1<<WDP0; /* 32ms */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 1<<WDP1 | 0<<WDP0; /* 64ms */
    //WDTCSR = 0<<WDP3 | 0<<WDP2 | 1<<WDP1 | 1<<WDP0; /* 0.125 seconds */
    //WDTCSR = 0<<WDP3 | 1<<WDP2 | 0<<WDP1 | 0<<DP0; /* 0.25 seconds */
    WDTCSR = 0<<WDP3 | 1<<WDP2 | 0<<WDP1 | 1<<WDP0; /* 0.5 seconds */
    //WDTCSR = 0<<WDP3 | 1<<WDP2 | 1<<WDP1 | 0<<WDP0; /* 1.0 seconds */
    //WDTCSR = 0<<WDP3 | 1<<WDP2 | 1<<WDP1 | 1<<WDP0; /* 2.0 seconds */
    //WDTCSR = 1<<WDP3 | 0<<WDP2 | 0<<WDP1 | 0<<WDP0; /* 4.0 seconds */
    //WDTCSR = 1<<WDP3 | 0<<WDP2 | 0<<WDP1 | 1<<WDP0; /* 8.0 seconds */

    WDTCSR |= (1<<WDIE);//enable watchdog interrupt

    #ifdef SERIAL_DEBUG
        set_sleep_mode(SLEEP_MODE_IDLE);     // sleep mode is set here Power Down uses the least current
    #else
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);     // sleep mode is set here Power Down uses the least current
    #endif
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
        #ifdef SERIAL_DEBUG
            Serial.println("Buzzer()");
        #endif
        tone(pinBuzzer,buzzerfrequency,1000/4);
        #ifndef SERIAL_DEBUG
        delay(1000/4);
        noTone(pinBuzzer);
        #endif
        //tone(pinBuzzer,buzzerfrequency,1000/4);
        //delay(1000/4);

}

void updateOutputs(){ //change the states of the heater, fan and motor
    noInterrupts(); //prevent relays causing a door trigger
    digitalWrite(pinLight, state_door);
    digitalWrite(pinMotor,state_motor);
    digitalWrite(pinFan,state_fan);
    digitalWrite(pinHeater,state_heater);
    delay(75); //allow time for relay to activate
    interrupts(); //reenable interrupts
}

#ifdef USELCD
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
    //lcd.print("Count_door: "); lcd.print(count_door);
    lcd.print("TempD: "); lcd.print(temperature_desired);

    lcd.setCursor(0, 5);lcd.clearLine();
    lcd.print("Sttng: "); lcd.print(temperature_dial);
 //   lcd.print("Cool#: "); lcd.print(cooling_state_time_since_change);
}
#endif

void doorTrigger(){
    detachInterrupt(0);
    sei();
    delay(50);
    state_door=digitalRead(pinDoorSW);
    if (state_door){ //switch off fan immediately on door open
            state_fan = false;
    } else if (state_motor){ //switch on fan immediately on door close and motor running
        state_fan = true;
    };
    count_door=0; //reset door counter to zero on every door action
    updateOutputs();

    #ifdef SERIAL_DEBUG
    Serial.println();
    Serial.println("**************************************************");
    Serial.print("DOOR STATE CHANGED "); Serial.println(state_door);
    Serial.println("**************************************************");
    #endif
    delay(200);
    attachInterrupt(0,doorTrigger,CHANGE);
}

void setup(){

    EIMSK != _BV(INT0);
    sei();

    analogReference(DEFAULT);

    #ifdef SERIAL_DEBUG
        pinMode(2, OUTPUT);
        Serial.begin(9600);
        delay(1000);
        Serial.println("Executing Setup()");
        delay(2000);
    #endif
    #ifdef USELCD
        lcd.begin(84, 48);
    #endif

    setupSleep();

    /*Check these
    power_spi_disable();
    power_timer1_disable(); //Timer 0 will also stop WDT, which I need
    power_timer2_disable();
    */

    power_twi_disable();

    //Setup pins
    delay(10);
    pinMode(pinDoorSW,INPUT);
    pinMode(pinLight,OUTPUT);
    pinMode(pinMotor,OUTPUT);
    pinMode(pinHeater,OUTPUT);
    pinMode(pinFan,OUTPUT);

    doorTrigger(); //read the door state on power on
    thermometer.update();



}

void heater(bool enabled){
    static bool active_state; //False = off; True = on with tomer
    static unsigned int timer;
    if (enabled && (thermometer.temperature >= temperature_desired) && (timer > (heater_max_time / 2))) enabled = false;

    if (enabled && active_state==false){
        timer = 0;
        state_heater = true;
        active_state = true;
    }
    if (enabled && active_state==true){
        if (timer > heater_max_time) {
            state_heater = false;
        } else {
            timer++;
        }
    }
    if (!enabled){
        state_heater = false;
        active_state = false;
    }
}

void setCoolingState(){
    static bool active_state; //False = warming, True = cooling

    if  (temperature_dial == (1024 /  temperature_dial_divider)){ //if dial @ 0
        //tone(pinBuzzer,buzzerfrequency,1000/4);
        state_fan = false;
        state_motor = false;
        heater(false);
        cooling_state_time_since_change=0;
    } else {
        temperature_desired = temperature_desired_max - temperature_desired_min;
        temperature_desired = temperature_desired * (float (temperature_dial) / (1024  / temperature_dial_divider));
        temperature_desired = temperature_desired_min + temperature_desired;
        if ((cooling_state_time_since_change > 0) && (cooling_state_time_since_change < (motor_min_time + 1))) { //deny change of state, unless time=0=just turned on
            cooling_state_time_since_change++;
        } else { //Allow the change of state
            if (!state_door && (thermometer.temperature > ( temperature_desired + (temperature_hysterysis / 2)))) { //Switch on cooling
                if (!active_state){
                    active_state = true;
                    cooling_state_time_since_change = 1;
                }
            }
            if (thermometer.temperature < ( temperature_desired - (temperature_hysterysis / 2))) { //Switch off cooling
                if (active_state){
                    active_state = false;
                    cooling_state_time_since_change = 1;
                }
            }
        }
        switch (active_state){
        case true :
            if (!state_door) {state_fan = true;}
            state_motor = true;
            heater(false);
            break;
        case false:
            state_fan = false;
            state_motor = false;
            heater(true);
        }
    }
}

void doorState(){
    #ifdef USELCD
    lcd.setPower(state_door);
    #endif
    if (state_door){ //Door open
        if (count_door < door_open_max) {
            count_door++;
        } else {
            state_buzzer = true;
        }
    } else { //Door closed
        state_buzzer=false; //always make it false
        switch (count_door){
            case 6:
                #ifdef USELCD
                lcd.setPower(false);
                #endif
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
}

void loop(){
    f_wdt=0;

    state_door=digitalRead(pinDoorSW);
    doorState();
    temperature_dial = round(analogRead(A2) / temperature_dial_divider);
    thermometer.update();
    setCoolingState();
    updateOutputs();

    #ifdef USELCD
    lcdupdate();
    #endif

    #ifdef SERIAL_DEBUG
    Serial.write("\r\n\r\n\r\n");
    Serial.print("Tempterature dial: "); Serial.println(temperature_dial);
    Serial.print("Temperature desired: ");Serial.println(temperature_desired);
    Serial.print("Temperature: ");Serial.println(thermometer.temperature);
    Serial.print("   State Door: "); Serial.println(state_door);
    Serial.print(" State Buzzer: "); Serial.println(state_buzzer);
    Serial.print("  State Motor: "); Serial.println(state_motor);
    Serial.print(" State Heater: "); Serial.println(state_heater);
    Serial.print(    "State Fan: "); Serial.println(state_fan);
    Serial.print("   Door count: "); Serial.println(count_door);
    Serial.print("Time since change : ");Serial.println(cooling_state_time_since_change);
    Serial.print("Digital pin 2 value:"); Serial.println(digitalRead(pinDoorSW));
    delay(500);
    #endif

    enterSleep();
}
