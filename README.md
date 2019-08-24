# SmarterFridge
Digital (AVR MCU) replacement for fridge analogue thermistat.
Circuit is essentially a custom Arduino with ATMEGA328P.

This is one of my first "major" projects (circa 2012), and a lot has been learnt since then.

Additional features (because why not)
* Fan to circulate air over the cooling radiator
  * Does not run the fan when the door is open
  * Gives an old and basic fridge more modern and expensive features.
* Door open/ajar reminder (buzzer)
* Anti-freeze / defroster (since the old fridge was a "frost-free" type.
* Does not start the compressor until the door is closed

Project notes:
* Circuit and PCB done with EagleCAD -- Only learnt basic KiCAD in a later project.
* Done with Code::Blocks (painful setup, but I wanted a half decent IDE. It should convert back to Arduno IDE fairly easily.
* In hindsight:
  * Software: not using the default Arduino bootloader may be better (doesn't pulse the pins on boot); (OptiBoot?)
  * Hardware: drive the transistors, from the MCU, via a resistor and have a pulldown
  * Hardware: There are nice modular 110V/220V mains to 12V modules that could shrink the total install package size.
  * Hardware: add simple debounce circuit to door switch. Maybe even a gated debounce.
  * Hardware: The fan turned out to be annoying in terms of humming sound. With a differnt type of fan and a custom 3D printed baffle, the idea could be promissing.
  * Hardware: Maybe port to a more modern cheap platform (ESP8266/ESP32) and/or surface mount components


Background story:
The fridge I got when I moved in to my first apartment was an old 80's/90's model that had been working reasonably well for the 5 years before this project was dreamed up. The driving reasons for this projects was: the replacement analogue thermistat was expensive; I had an interest in electronics for most of my life and had a dangerously primative knowledge of circuit (inspite of doing a few subjects in university); Arduinos were all the rage; I wanted to learn and most of the fancy fridges had some MCU type setup. Above all, the over-running of the compressor was costing me a small fortune in electricity and spoiling my fresh fruit and veg... it had to be fixed... I wanted to learn... I wasn't spending a small fortunate buying a new fridge because of 1 bad part.

My then partner, being a freshly minted professional programmer, helped me setup Code::Blocks as I wanted an IDE more akin to Visual Studio, or at least better than the basic notepad like setup that shipped for the Arduino back then (early 2010's).

The project was fun, learning about NTC resistors, voltage dividers, and even how to do lithography and make a PCB with cheap pool acid. It was also an introduction to HVAC to me -- something useful later in my career.

Sadly, 4 years later, I got home one warm summer evening and found the fridge had given up on me (I suspect a refridgerant leak, causing overheat/failure of the compressor) and was replaced with another basic fridge with analogue thermistat, the very next day -- and so this project may still be useful to necro at some point.
