#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

volatile int f_wdt=1;
boolean led_state=false;

word Analog0 = 0;    // first analog sensor
short temp = 0;   // second analog sensor

/*
Thermistor Specifications:
T0 = 25^C
R0 = 1000 ohm
K  = 3700

Voltage divider Specifications:
Vin = 5V
 |
 R = 10k
 +- Vout
 |
Thermistor = 1000 @ 25 / B=3700
 |
GND
*/
double Rinf;

int sleepStatus = 0;             // variable to store a request for sleep
int count = 0;                   // counter
  
void setup()
{
  // start serial port at 9600 bps:
  Serial.begin(9600);
  Rinf = 1000 * exp(-3700 / 298.15); //R0 * exp(-B / T0)
  
  
  pinMode(2, OUTPUT);
  setupSleep();
  power_spi_disable();
  power_timer1_disable(); //Timer 0 will also stop WDT, which I need
  power_timer2_disable();
  power_twi_disable();
}

void loop()
{ 
  f_wdt=0;
  //Serial.println("About to sleep");
  enterSleep();
  //Serial.println("Awake");
  led_state = !led_state;
  digitalWrite(13, led_state);
  Serial.println(tempC());
  //delay(300);
}

float tempC()
{
  long Rval = 0;
  for (byte count = 0; count < 32; count++){ //get an average reading of Vout
    Rval += analogRead(A0);
  }
  Rval /= 32;
  Rval = (Rval * 10000) / (1023 - Rval); //Convert average Vout to Rtherm

  return 3700 / log(Rval / Rinf) - 273.15; //Convert Rtherm to temperature in degrees centigrade.
}

ISR(WDT_vect)
{
  if(f_wdt == 0)
  {
    f_wdt=1;
  }
  else
  {
    Serial.println("WDT Overrun!!!");
  }
}

void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_SAVE);
  
  sleep_enable();
  power_usart0_disable();
  sleep_mode();
  
  /* The program will continue from here after the WDT timeout*/
  power_usart0_enable();
  delay(10);
  Serial.println();
  //Serial.println("Continuing");
  sleep_disable(); /* First thing to do is disable sleep. */
  
  //power_all_enable();
  //power_adc_enable();
}

void setupSleep(void) //init WDT
{
  // Clear the reset flag.
  MCUSR &= ~(1<<WDRF);
  
  // In order to change WDE or the prescaler, we need to set WDCE (This will allow updates for 4 clock cycles).
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  // set new watchdog timeout prescaler value
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
  
  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
}
