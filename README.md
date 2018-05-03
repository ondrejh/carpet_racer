# PWM DC motor driver with servo input signal

The device based on MSP430G2231 turns one servo signal into PWM output for dc motor driver and the second servo signal into light ON/OFF.

## Schematic

For schematic see main.c - it's briefly ascii-ed there.

## How it works

Due to lack of timers it just wait for servo signal to go HIGH, than it measures its pulse width(s) and than it produces PWM signal for motor driver. Therefor the motor PWM is limited by max 70% and therefor it produces 50Hz PWM - quite noisy. On the other hand the limit is needed because the original model used only 3 AA batteries so otherwise 2S LiPo would burn the motor very soon. And the noise sounds much more like motor compared to standard 1-3kHz whisle. It also measures input voltage to inform about battery state and to kill driver if battery low.

## Information sources

[MSP430x2xx family user's guide](http://www.ti.com/lit/ug/slau144j/slau144j.pdf)
[MSP430G2231 datasheet](http://www.ti.com/lit/ds/symlink/msp430g2231.pdf)
[MSP430G2x01, MSP430G2x11, MSP430G2x21, MSP430G2x31 Code Examples](http://www.ti.com/general/docs/lit/getliterature.tsp?baseLiteratureNumber=slac463&fileType=zip)

## Model mechanical changes

- remove steering motor, increase steering range, connect 9g servo
- remove battery holder ribs
- remove all electronics
- install mcu module, 5V UBEC, H-bridge module, RC receiver
- install lights (leds)

## How it looks now

![Guts](/doc/guts.jpg)

![Battery](/doc/battery.jpg)

![Done](/doc/race.jpg)
