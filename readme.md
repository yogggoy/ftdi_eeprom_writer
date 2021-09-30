FTDI FT2232H EEPROM WRITER
------

Requirements: libftdi1*, libusb*

Build: just `make` and run `ftdi-eeprom-config`

Usage:
```
User must be root
usage: ./ftdi-eeprom-config [options]
          -e            erase
          -q            show parameters
        For search devices use flags[V/P/S]:
          -V            Vendor ID, default 0x0
          -P            Product ID, default 0x0
          -S            Serial ID, default None - search all devices
        If flags [p/s] not used, application only read EEPROM
        For set new parameters use:
          -p <pid>      new PID, for ft2232h : 0x6010
          -s <serial>   write, with serial number
          -m <manufacturer>     write new manufacturer
          -d <device>   write new device description

        For write default EEPROM:
          -z            defaul parameters

        Print EEPROM to console:
          -o

        When you have many FTDI devices and there is no way to leave
        only one connected, use this flags to work with multi-devices.
        Attention! This is an exceptional case, it is preferable to
        keep only one device if possible. Otherwise, there is a chance
        of making a mistake and ruining the device.
        Multi-device access not sensitive to [-S] serial flag!
          -! multi              activate multi_device mode
          -@ <device>           number of device

Examples:
find devices          : ./ftdi-eeprom-config -V 0x403 -P 0x6010
find devices          : ./ftdi-eeprom-config -V 0x403 -P 0x6010 -S 1234567
search all FTDI dev   : ./ftdi-eeprom-config -V 0x0 -P 0x0
write new parameters  : ./ftdi-eeprom-config -V 0x403 -P 0x6010 -S 1234567 -p 0x6030 -s New_1234
list multi-device mode: ./ftdi-eeprom-config -V 0x403 -P 0x6010 -! multi -@ 0
program multi-device  : ./ftdi-eeprom-config -V 0x403 -P 0x6010 -! multi -@ 0 -s New_1234
```
