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

#define LIGHTS_ON() do{P1OUT|=0x40;}while(0)
#define LIGHTS_OFF() do{P1OUT&=~0x40;}while(0)
#define LIGHTS_SWAP() do{P1OUT^=0x40;}while(0)

#define CH2_M (1<<0)
#define CH3_M (1<<1)

#define CENTER 1500
#define MAX 4200

#define PWM_MAX 12000

#define CH3_ON 1300

#define PWR_CNT 8
#define PWR_DIV PWR_CNT*1023/4.3/3.3
#define PWR_HIGH (uint16_t)(7.8*PWR_DIV)
#define PWR_OK (uint16_t)(7.0*PWR_DIV)
#define PWR_LOW (uint16_t)(6.5*PWR_DIV)

int16_t pwm;

bool pwr_low = false;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ; // Set DCO
	DCOCTL = CALDCO_1MHZ;

    // motor and lights outputs
	P1DIR |= 0x70; P1OUT &= ~0x70;

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
}

void use_ch(uint16_t ch2, uint16_t ch3)
{
    if ((ch2)&&(!pwr_low)) {
        int16_t ich2 = ch2-CENTER;
        if (ich2>0) {
            pwm = ich2<<5;
            if (pwm>PWM_MAX) pwm=PWM_MAX;
            if (pwm>MAX) {
                IN1_HIGH();
                IN2_LOW();
            }
            else {
                IN1_LOW();
                IN2_LOW();
            }
        }
        else {
            pwm = (-ich2)<<5;
            if (pwm>PWM_MAX) pwm=PWM_MAX;
            if (pwm>MAX) {
                IN1_LOW();
                IN2_HIGH();
            }
            else {
                IN1_LOW();
                IN2_LOW();
            }
        }
    }
    else {
        IN1_LOW();
        IN2_LOW();
    }

    if ((ch3>800)) {//&&(!pwr_low)) {
        if (ch3>CH3_ON)
            LIGHTS_ON();
        else 
            LIGHTS_OFF();
    }
    else {
        static uint8_t cnt = 0;
        cnt++;
        if ((cnt&0x3F)==0)
            LIGHTS_ON();
        else
            LIGHTS_OFF();
    }
    //LIGHTS_ON();
}

// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init(); // init dco and leds

    uint16_t start=0, start_ch2=0, ch2=0, ch3=0;

    while (1) { // loop forever

        while (!(P1IN&CH2_M)) {};

        start = TAR;

        adc();
        use_ch(ch2, ch3);
        ch2 = ch3 = 0;

        while (1) {
            uint8_t in = P1IN;
            uint16_t t = TAR;
            if ((!ch2) && (!(in&CH2_M))) {
                ch2 = t-start;
                start_ch2 = t;
                break;
            }
            if ((t-start) > MAX)
                break;
        }
        while (1) {
            uint8_t in = P1IN;
            uint16_t t = TAR;
            if ((!ch3) && (!(in&CH3_M))) {
                ch3 = t-start_ch2;
                break;
            }
            if ((t-start) > MAX)
                break;
        }

        while ((TAR-start)<pwm) {};
        IN1_LOW();
        IN2_LOW();
    }
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
