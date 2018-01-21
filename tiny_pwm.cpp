/*
MIT License

Copyright (c) 2016 Madis Kaal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include <bb_terminal.hpp>

Terminal terminal;

// calibration value is added to temperature reading to make
// ADC readout 300 equal to 25degC. Or you can use it to shift
// the entire working range up and down
//
#define TEMP_CALIBRATION -3

enum FANMODE { STARTUP,FULLSPEED,RUNNING,OFF };

volatile uint16_t count;
volatile uint8_t state=STARTUP;
volatile uint16_t pwm;
volatile uint16_t temp,tempacc;
volatile uint8_t tempcount,ticks;

// 20 ADC samples are collected and averaged to reduce noise
// A/D conversion is started from timer interrupts, so the average
// temperature is updated every 600+ milliseconds
//
ISR(ADC_vect)
{
  tempacc+=((ADCL|(ADCH<<8))+TEMP_CALIBRATION);
  if (!temp)
    temp=tempacc; // on first sample initialize average too
  tempcount++;
  if (tempcount>19) {
    temp=tempacc/20;
    tempacc=0;
    tempcount=0;
  }
}
    

ISR(TIMER0_OVF_vect)
{
  ticks++;
  switch (state)
  {
    default:
    // when starting up, give an initial kick at full power
    case STARTUP:
      state=FULLSPEED;
      TCCR1=0;  // disconnect PWM
      PORTB|=2; // force driver on
      count=0;
      break;
    // once the fan has spun up, switch over to running normal PWM mode
    case FULLSPEED:
      count++;
      if (count>32) {
        state=RUNNING;
        OCR1A=255;    // keep the output high for now
        TCCR1=0xD1;   // PWM1A enabled, clear OC1A on compare match, system clock
      }
      break;
    // ADC result is roughly 1 count per degC, where 300 equal to 25degC
    // we want the fan to be off below 25 degC, and run
    // at full speed at 55 degC or more.
    // we'll let the fan run at 40% duty cycle at minimum
    // so the 30 degree difference translates directly to
    // 40..100% PWM duty cycle
    case RUNNING:
      if (temp<300) { // if temperature below threshold
        OCR1A=0;      // then force output low
        state=OFF;    // and switch to off state
      }
      else {
        if (temp>330) {
          pwm=255;    // full on above 55 degC
        }
        else {
          pwm=(temp-300)*2+40; // duty cycle %
          pwm=(pwm*255)/100;   // 8 bit PWM scale
        }
        OCR1A=pwm;    // update duty cycle
      }
      break;
    // in off state just watch the temperature
    // and if it rises above threshold then start again
    case OFF:
      if (temp>302) {
        state=STARTUP;
      }
      break;
  }
  // start next ADC conversion, this will finish faster than
  ADCSRA=0xdf;
}

ISR(WDT_vect)
{
}


/*
I/O configuration
-----------------
PB0 unused                            input        0    1
PB1 FAN (active high)                 output       1    0
PB2 unused                            input        0    1
PB3 unused                            input        0    1
PB4 serial out                        output       1    1
*/
int main(void)
{
  MCUSR=0;
  PORTB=0x1d;
  DDRB=0x12;
  // configure timer0 for periodic interrupts on overflow
  TCCR0A=0;
  TCCR0B=5; // clock/1024 prescale
  TIMSK=0x02;
  //
  // configure ADC to read temperature
  //
  ADMUX=0x8f;   // 1.1V internal reference, temperature sensor channel
  ADCSRA=0xdf; // start conversion
  //
  // configure watchdog
  //
  WDTCR=(1<<WDE) | (1<<WDCE);
  WDTCR=(1<<WDE) | (1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2sec timout, interrupt+reset
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  sei();
  terminal.init();
  while (1) {
    sleep_cpu();
    wdt_reset();
    WDTCR=(1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2sec timout, interrupt+reset
    if (ticks>32) {
      ticks=0;
      terminal.clear();
      terminal.puts("ADC:");
      terminal.putn(temp);
    }
  }
}

