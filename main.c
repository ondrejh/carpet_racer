//******************************************************************************
// RC signal DC motor drive for MSP430 launchpad
//
//
// author: Ondrej Hejda
// date:   6.1.2018
//
// hardware: MSP430G2231 (launchpad)
//
//                MSP430G2231
//             -----------------
//         /|\|                 |
//          | |             P2.7|----> GREEN LED (active high)
//          --|RST          P2.6|----> RED LED (active high)
//            |                 |
//            |             P1.2|----> MOTOR IN1
//            |             P1.3|----> MOTOR IN2
//            |                 |
//            |             P1.4|<---- CH2 (throtle)
//            |             P1.5|<---- CH3 (light)
//            |                 |
//
//******************************************************************************

// include section
//#include <msp430g2553.h>
#include <msp430g2231.h>
//#include <msp430g2452.h>

#include <stdint.h>
#include <stdbool.h>

#define GREEN_ON() do{P2OUT|=0x80;}while(0)
#define GREEN_OFF() do{P2OUT&=~0x80;}while(0)
#define GREEN_SWAP() do{P2OUT^=0x80;}while(0)

#define RED_ON() do{P2OUT|=0x40;}while(0)
#define RED_OFF() do{P2OUT&=~0x40;}while(0)
#define RED_SWAP() do{P2OUT^=0x40;}while(0)

#define IN1_HIGH() do{P1OUT|=0x04;}while(0)
#define IN1_LOW() do{P1OUT&=~0x04;}while(0)
#define IN2_HIGH() do{P1OUT|=0x08;}while(0)
#define IN2_LOW() do{P1OUT&=~0x08;}while(0)

#define CH2_M (1<<4)
#define CH3_M (1<<5)

#define THOLD 70
#define CENTER 1500
#define MAX 2200
#define MIN 800

#define PWM_MAX 1023

uint16_t pwm;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

    /*// led and motor outputs
	P1DIR |= 0x4F; P1OUT &= 0xB0;
    // servo signal input
    P1DIR &= 0xCF;*/
    
    P2SEL &= ~0xC0; P2DIR |= 0xC0; P2OUT &= ~0xC0;

    // start timer
    TACTL = TASSEL_2 + MC_2; // SMCLK, continuous

    //LED_OFF();
}

// channel 2
void ch2(uint16_t v)
{
    if ((v<MIN) | (v>MAX)) return;

    uint16_t p;

    if (v < (CENTER - THOLD)) {
        p = CENTER - v;
        p <<= 1;
        pwm = p;
        IN2_HIGH();
    }
    else if (v > (CENTER + THOLD)) {
        p = v - CENTER;
        p <<= 1;
        pwm = PWM_MAX - p;
        IN2_LOW();
    }
    else {
        pwm = PWM_MAX;
        IN2_LOW();
    }
}

// channel 3
void ch3(uint16_t v)
{
    if ((v<MIN) | (v>MAX)) return;
    
    //if (v<CENTER) LED_OFF();
    //else LED_ON();
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds

    while(1) {
        RED_ON();
        __delay_cycles(500000);
        RED_OFF();
        GREEN_ON();
        __delay_cycles(500000);
        GREEN_OFF();
    }

    /*while (1) {
        // wait both servo inputs are low
        while (P1IN&(CH2_M|CH3_M)) {};

        GREEN_ON();

        uint8_t in_last = 0;

        uint16_t ch2_start = 0;
        uint16_t ch3_start = 0;

	    while(1) {
            uint16_t now = TAR;
            uint8_t in_now = P1IN;
            
            uint8_t in_changes = in_now^in_last;
            uint8_t in_goes_up = in_changes&in_now;
            uint8_t in_goes_down = in_changes&in_last;
            
            // get channel 3 value
            if (in_goes_up & CH3_M)
                ch3_start = now;
            else if (in_goes_down & CH3_M)
                ch3(now-ch3_start);

            // get channel 2 value
            if (in_goes_up & CH2_M)
                ch2_start = now;
            else if (in_goes_down & CH2_M)
                ch2(now-ch2_start);

            // do some pwm
            if (pwm<(now&PWM_MAX))
                IN1_HIGH();
            else
                IN1_LOW();

            // save input for next round
            in_last = in_now;

            // if no servo signal switch off
            if ((now-ch3_start) > 50000)
                break;
	    }

        GREEN_OFF();

        LED_OFF();
        IN1_LOW();
        IN2_LOW();

        ch2_start = TAR;
        ch3_start = 0;

        while (!(P1IN&CH3_M)) {
            uint16_t now = TAR;
            if ((now-ch2_start)>10000) {
                ch2_start = now;
                ch3_start++;
                if (ch3_start>=200) {
                    LED_ON();
                    GREEN_ON();
                    ch3_start = 0;
                }
                else {
                    GREEN_OFF();
                    LED_OFF();
                }
            }
        }
    }*/

	return -1;
}
