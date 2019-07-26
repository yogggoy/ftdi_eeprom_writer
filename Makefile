IDIR =/usr/include/libftdi1/
CFLAGS=-I$(IDIR)
LDFLAGS ?= -lftdi1 -lusb
PACKAGES = ftdi-eeprom-config

all: ${PACKAGES}

ftdi-eeprom-config: eeprom_ftdi.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PACKAGES)