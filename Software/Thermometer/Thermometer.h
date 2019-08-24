#ifndef THERMOMETER_H
#define THERMOMETER_H
#include <arduino.h>
#include <math.h>

/*
    For NTC type thermistors
    Output Temperature in degrees Celcius
*/

class Thermometer
{
    public:
        Thermometer(unsigned char analogInPin,
                    unsigned int resistance_zero,
                    float temperature_zero=298.15,
                    int B_zero=-3700
                    );
        void update();
        float temperature;
        unsigned int ADCvalue;
        unsigned int Resistance_therm;
        float Rinfinity;
    protected:
    private:
        word Rzero;
        float Tzero;
        int Bzero;
        byte readPin;

};

#endif // THERMOMETER_H
