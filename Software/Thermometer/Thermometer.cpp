#include "Thermometer.h"

Thermometer::Thermometer(unsigned char analogInPin, unsigned int resistance_zero,float temperature_zero, int B_zero){
    Rzero=resistance_zero;
    Tzero=temperature_zero;
    Bzero=B_zero;
    readPin = analogInPin;

    analogReference(DEFAULT);

    Rinfinity = exp( (float)((-1 * (float)Bzero) / (float)Tzero) ) * (float) Rzero;
    //Rinfinity = 0.008156351347609;
}

void Thermometer::update(){
    unsigned char reads=32; //amount of reads to do on the ADC
    ADCvalue = 0;
    for (unsigned char count=0; count < reads; count++){
        ADCvalue = ADCvalue + analogRead(readPin);
        delay(5);
    }
    ADCvalue = round(ADCvalue / reads);

    unsigned int Rtop = 10000;
    unsigned int scale = 1023; //ADC scale
    Resistance_therm = 0;
    Resistance_therm = round((float)((float) Rtop * (float)ADCvalue) /((float)(scale - ADCvalue)));

    //temperature = (((float)Bzero) / log(Resistance_therm / Rinfinity));
    //temperature = log(Resistance_therm / Rinfinity);
    temperature = (float)Resistance_therm / (float)Rinfinity * 2;
    temperature = log(temperature);
    temperature = (float)Bzero / temperature;
    temperature -= 275.15; //Convert Kelvin back to Celcius
}

