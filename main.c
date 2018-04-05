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
//            |             P1.0|<---- CH2 (throtle)
//            |             P1.1|<---- CH3 (light)
//            |                 |
//            |             P1.2|<---- BATTERY ADC
//            |                 |
//            |             P1.4|----> MOTOR IN1
//            |             P1.5|----> MOTOR IN2
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

#define IN1_HIGH() do{P1OUT|=0x10;}while(0)
#define IN1_LOW() do{P1OUT&=~0x10;}while(0)
#define IN2_HIGH() do{P1OUT|=0x20;}while(0)
#define IN2_LOW() do{P1OUT&=~0x20;}while(0)

#define CH2_M (1<<0)
#define CH3_M (1<<1)

#define THOLD 70
#define CENTER 1500
#define MAX 2200
#define MIN 800

#define PWM_MAX 1023

#define PWR_CNT 8
#define PWR_DIV PWR_CNT*1023/4.3/3.3
#define PWR_HIGH (uint16_t)(7.8*PWR_DIV)
#define PWR_OK (uint16_t)(7.0*PWR_DIV)
#define PWR_LOW (uint16_t)(6.5*PWR_DIV)

uint16_t pwm;

bool pwr_low = false;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ; // Set DCO
	DCOCTL = CALDCO_1MHZ;

    // motor outputs
	P1DIR |= 0x30; P1OUT &= ~0x30;

    // servo signal input
    P1DIR &= ~0x03;

    // led outputs    
    P2SEL &= ~0xC0; P2DIR |= 0xC0; P2OUT &= ~0xC0;

    ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE; // ADC10ON, interrupt enabled
    ADC10CTL1 = INCH_2; // input A2
    ADC10AE0 |= 0x04; // PA.2 ADC option select

    // start timer
    TACTL = TASSEL_2 + MC_2; // SMCLK, continuous
}

void adc(void)
{
    IN2_HIGH();
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit
    static uint16_t res = 0;
    static uint8_t cnt = 0;
    res += ADC10MEM;
    cnt ++;
    if (cnt>=PWR_CNT) {
        if (res>PWR_HIGH) {
            GREEN_SWAP();
            RED_OFF();
            pwr_low = false;
        }
        else if (res>PWR_OK) {
            GREEN_ON();
            RED_OFF();
            pwr_low = false;
        }
        else if (res>PWR_LOW) {
            GREEN_OFF();
            RED_ON();
        }
        else {
            GREEN_OFF();
            RED_SWAP();
            pwr_low = true;
        }
        res = 0; cnt = 0;
    }
    IN2_LOW();
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds

    uint16_t now = 0;

    while (1) { // loop forever

        // wait both signals low
        while(P1IN&CH2_M) ;

        while (!(P1IN&CH2_M)) {
            if ((TAR-now)>=50000) {
                adc();
                now += 50000;
            }
        }
        now = TAR;
        adc();
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

// ADC10 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
#else
#error Compiler not supported!
#endif
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
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


