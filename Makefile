# MIT License
#
# Copyright (c) 2016 Madis Kaal
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# project name, resulting binaries will get that name
PROJECT=tiny_pwm

# mcu options, clock speed and device
F_CPU=8000000UL
GCCDEVICE=attiny85

# object files going into project
OBJECTS=tiny_pwm.o

#avrdude options
FUSES=-U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0xff:m 
#-U lock:w:0x3F:m
DEVICE=t85


# additional include directories
INCLUDEDIRS=-I..
# additional libraries to link in
LIBRARIES=

vpath %.cpp ..

#--------------------------------------------------------------
CC=avr-gcc
CXX=avr-g++
OBJCOPY=avr-objcopy
OBJDUMP=avr-objdump
SIZE=avr-size
AVRDUDE=avrdude
REMOVE=rm -f
LD=avr-g++

#generic compiler options
CFLAGS=-I. $(INCLUDEDIRS) -g -mmcu=$(GCCDEVICE) -Os \
	-fpack-struct -fshort-enums             \
	-funsigned-bitfields -funsigned-char -Wall \

CXXFLAGS=$(CFLAGS) -fno-exceptions -DF_CPU=$(F_CPU)

LDFLAGS=-Wl,-Map,$(PROJECT).map -mmcu=$(GCCDEVICE) $(LIBRARIES)	

.PHONY: erase clean

#------------------------------------------------------------

all: clean hex

hex: $(PROJECT).hex $(PROJECT).eep

$(PROJECT).elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $?
	@avr-size $(PROJECT).elf
	@avr-objdump -S $@ > $(PROJECT).lst
		
$(PROJECT).hex: $(PROJECT).elf
	@$(OBJCOPY) -j .text -j .data -O ihex $< $@

$(PROJECT).eep: $(PROJECT).elf
	@-$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

flash: all $(PROJECT).hex $(PROJECT).eep
	$(AVRDUDE) -P usb -B 10 -c usbtiny -p $(DEVICE) $(FUSES) -U flash:w:$(PROJECT).hex -U eeprom:w:$(PROJECT).eep

erase:
	$(AVRDUDE) -P usb -c usbtiny -p $(DEVICE) -e

clean:
	@rm -f $(PROJECT).hex $(PROJECT).eep $(PROJECT).elf *.o *~ *.lst *.map
						 	 		
%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDEDIRS) -c $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $< -o $@
