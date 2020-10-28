CFLAGS:=-g -O2 -std=c99 -Wall -Wextra
PACKAGES = ftdi-eeprom-config

all: ${PACKAGES}

ftdi-eeprom-config: eeprom_ftdi.c
	$(CC) -o $@ $^ $(CFLAGS) `pkg-config --libs --cflags libftdi1`

clean:
	rm -f $(PACKAGES)