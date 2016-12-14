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
#ifndef __terminal_hpp__
#define __terminal_hpp__
#include <avr/io.h>
#include <util/delay.h>

#define BAUDRATE 38400
#define DELAY (1000000L/BAUDRATE)

#define TXHIGH() (PORTB|=0x10)
#define TXLOW() (PORTB&=~0x10)

class Terminal
{
const char *xdigit;
public:
  Terminal()
  {
    xdigit="0123456789abcdef";
  }
  
  // bit-banged serial output 8N2 format
  void putc(uint8_t c)
  {
    uint8_t i=8;
    TXLOW();
    _delay_us(DELAY);
    while (i--) {
      if (c&1)
        TXHIGH();
      else
        TXLOW();
      c>>=1;
      _delay_us(DELAY);
    }
    TXHIGH();
    _delay_us(DELAY);
    _delay_us(DELAY);
  }
  
  void puts(const char *s)
  {
    while (*s) {
      putc(*s++);
    }
  }
  
  uint8_t ready()
  {
    return 0; // input not implemented
  }
  
  uint8_t getch()
  {
    return 0;
  }
  
  void putn(int32_t n)
  {
    if (n<0)
      n=0-n;
    if (n>9)
      putn(n/10);
    putc((n%10)+'0');
  }
  
  void putx(int16_t x)
  {
    putc(xdigit[(x>>12)&15]);
    putc(xdigit[(x>>8)&15]);
    putc(xdigit[(x>>4)&15]);
    putc(xdigit[x&15]);
  }

  void clear()
  {
    putc('\f');
  }
  
  void home()
  {
    putc('\v');
  }

  void init()
  {
    TXHIGH();
    clear();
    clear();
  }
    
};

#endif
