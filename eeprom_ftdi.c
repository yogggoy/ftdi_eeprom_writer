/* LIBFTDI EEPROM writre Serial number
   This program is distributed under the GPL, version 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <ftdi.h>
#include <string.h>

struct eeprom_ftdi_param
{
    struct ftdi_context *ftdi;
    char manufacturer[128];
    char description[128];
    char read_serial[128];

    int search_vid;
    int search_pid;
    char *search_serial;

    int new_pid;
    char *new_serial;
    char *new_manufacturer;
    char *new_product;

    char *multi_param;
    int _multi_device;
    int _multi;
};


void get_ftdi_eeprom_type(struct eeprom_ftdi_param * param, int ftret)
{
    int value;
    if (ftdi_get_eeprom_value(param->ftdi, CHIP_TYPE, & value) <0)
    {
        printf("ftdi_get_eeprom_value: %d (%s)\n", ftret, ftdi_get_error_string(param->ftdi));
    }
    if (value == -1)
        printf("No EEPROM\n");
    else if (value == 0)
        printf("Internal EEPROM\n");
    else
        printf("Found 93x%02x\n", value);
}

int read_decode_eeprom(struct ftdi_context *ftdi)
{
    int i, j, f;
    int value;
    int size;
    unsigned char buf[256];

    f = ftdi_read_eeprom(ftdi);
    if (f < 0)
    {
        printf("ftdi_read_eeprom: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        return -1;
    }

    ftdi_get_eeprom_value(ftdi, CHIP_SIZE, & value);
    if (value < 0)
    {
        printf("No EEPROM found or EEPROM empty\n");
        printf("On empty EEPROM, use -z option to write default values\n");
        return -1;
    }
    printf("Chip type %d ftdi_eeprom_size: %d\n", ftdi->type, value);
    if (ftdi->type == TYPE_R)
        size = 0xa0;
    else
        size = value;
    ftdi_get_eeprom_buf(ftdi, buf, size);
    for (i=0; i < size; i += 16)
    {
        printf("0x%03x:", i);

        for (j = 0; j< 8; j++)
            printf(" %02x", buf[i+j]);
        printf(" ");
        for (; j< 16; j++)
            printf(" %02x", buf[i+j]);
        printf(" ");
        for (j = 0; j< 8; j++)
            printf("%c", isprint(buf[i+j])?buf[i+j]:'.');
        printf(" ");
        for (; j< 16; j++)
            printf("%c", isprint(buf[i+j])?buf[i+j]:'.');
        printf("\n");
    }

    f = ftdi_eeprom_decode(ftdi, 1);
    if (f < 0)
    {
        printf("ftdi_eeprom_decode: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        return -1;
    }
    return 0;
}

void debug_output(struct eeprom_ftdi_param * param)
{
    printf("search vendor : 0x%x\n", param->search_vid);
    printf("search product: 0x%x\n", param->search_pid);
    printf("search serial : %s\n", param->search_serial);

    if (param->new_pid < 0)
        printf("[NULL]");
    printf("new pid : 0x%x\n", param->new_pid);

    if (param->new_serial == NULL)
        printf("[NULL]");
    printf("new serial : %s\n", param->new_serial);

    if (param->new_manufacturer == NULL)
        printf("[NULL]");
    printf("new manuf : %s\n", param->new_manufacturer);

    if (param->new_product == NULL)
        printf("[NULL]");
    printf("new product : %s\n", param->new_product);
}

int find_ftdi_serial(struct eeprom_ftdi_param * param, struct ftdi_device_list * devlist, int* duplicates, int* index)
{
    int res, cnt=0;
    struct ftdi_device_list *curdev;
    for (curdev = devlist; curdev != NULL; curdev = curdev->next, cnt++)
    {
        printf("%d) ", cnt);
        res = ftdi_usb_get_strings2(param->ftdi, curdev->dev, param->manufacturer, 128, param->description, 128, param->read_serial, 128);
        if ((res < 0) & (-9 != res))
        {
            printf("FAIL ftdi_usb_get_strings2 return : %d\n(%s)\n", res, ftdi_get_error_string(param->ftdi));
            return -1;
        }
        printf("Manufacturer: \"%s\"; Description: \"%s\";", param->manufacturer, param->description);
        if (-9 != res)
            printf(" Serial: \"%s\"\n", param->read_serial);
        else
            printf(" Serial not found\n");

        if (param->search_serial != NULL)
        {
            if (strcmp(param->read_serial, param->search_serial) == 0)
            {
                (*duplicates)++;
                *index = cnt;
            }
        }
    }
    if (param->search_serial == NULL)
        printf("    Use flag -S for search with serial ID\n");
    return 0;
}

int multi_device_disabled(struct eeprom_ftdi_param * param, struct ftdi_device_list * devlist, int* duplicates, int* index)
{
    if (*duplicates <= 0)
    {
        printf("[ERROR] Device with VID/PID and Serial 0x%x:0x%x '%s' NOT found: (%d)\n",
            param->search_vid, param->search_pid, param->search_serial, *duplicates
        );
        printf("    Please check you connection.\n");
        return -1;
    }
    else if (*duplicates > 1)
    {
        printf("[ERROR] Number of device with VID/PID and Serial 0x%x:0x%x '%s' found: (%d)\n",
            param->search_vid, param->search_pid, param->search_serial, *duplicates
        );
        printf("    Please disconnect any other FTDI devices.\n");
        return -1;
    }
    else
    {
        for (;*index != 0; (*index)--)
            devlist = devlist->next;

        if ( (ftdi_usb_open_dev(param->ftdi, devlist->dev)) < 0 )
        {
            printf("Unable to open device (%s)", ftdi_get_error_string(param->ftdi));
            return -2;
        }
    }
    return 0;
}

int multi_device_enabled(struct eeprom_ftdi_param * param, struct ftdi_device_list * devlist, int* devs_found)
{
    if (param->_multi_device > (*devs_found - 1))
    {
        printf("[ERROR] chosen device [@] %d out of range, should be in range [%d:0]\n", param->_multi_device, (*devs_found - 1));
        return -1;
    }
    for (;param->_multi_device != 0; param->_multi_device--)
        devlist = devlist->next;
    if ( (ftdi_usb_open_dev(param->ftdi, devlist->dev)) < 0 )
    {
        printf("Unable to open device (%s)", ftdi_get_error_string(param->ftdi));
        return -2;
    }
    return 0;
}

int single_device_found(struct eeprom_ftdi_param * param, struct ftdi_device_list * devlist)
{
    int res;
    res = ftdi_usb_get_strings2(param->ftdi, devlist->dev, NULL, 0, NULL, 0, param->read_serial, 128);
    if (res == -9)
        printf("WARNING: get serial number failed\n");
    else if (res < 0)
    {
        printf("FAIL ftdi_usb_get_strings2 return : %d\n(%s)\n", res, ftdi_get_error_string(param->ftdi));
        return -1;
    }

    if ( (ftdi_usb_open_dev(param->ftdi,  devlist->dev)) < 0 )
    {
        printf("Unable to open device (%s)", ftdi_get_error_string(param->ftdi));
        return -2;
    }
    printf("Device is found\n");
    return 0;
}


/*============================================================================\
|                                     main.c                                  |
\============================================================================*/

int main(int argc, char **argv)
{
    int ftret;
    struct eeprom_ftdi_param _param;
    _param.search_vid = 0x0;
    _param.search_pid = 0x0;
    _param.search_serial = NULL;
    _param.new_pid = -1;
    _param.new_serial = NULL,
    _param.new_manufacturer = NULL,
    _param.new_product = NULL,
    _param.multi_param = NULL;
    _param._multi_device = -1;
    _param._multi = 0;

    int erase = 0;
    int do_write = 0;
    int use_defaults = 0;
    int print_eeprom =0;
    int _debug = 0;

    int args;
    while ((args = getopt(argc, argv, "eV:P:S:p:s:m:d:z!:@:oqh?")) != -1)
    {
        switch (args)
        {
            case 'e':
                erase = 1;
                break;
            case 'V':
                _param.search_vid = strtoul(optarg, NULL, 0);
                break;
            case 'P':
                _param.search_pid = strtoul(optarg, NULL, 0);
                break;
            case 'S':
                _param.search_serial = optarg;
                break;
            case 'p':
                do_write  = 1;
                _param.new_pid = strtoul(optarg, NULL, 0);
                break;
            case 's':
                do_write  = 1;
                _param.new_serial = optarg;
                break;
            case 'm':
                do_write  = 1;
                _param.new_manufacturer = optarg;
                break;
            case 'd':
                do_write  = 1;
                _param.new_product = optarg;
                break;
            case 'z':
                do_write  = 1;
                use_defaults = 1;
                break;
            case '!':
                _param.multi_param = optarg;
                break;
            case '@':
                _param._multi_device = strtoul(optarg, NULL, 0);
                break;
            case 'o':
                print_eeprom = 1;
                break;
            case 'q':
                _debug = 1;
                break;
            case 'h':
            case '?':
            default:
                printf("User must be root\n");
                printf("usage: %s [options]\n", *argv);
                printf("\t  -e \t\terase\n");
                printf("\t  -q \t\tshow parameters\n");
                printf("\tFor search devices use flags[V/P/S]:\n");
                printf("\t  -V \t\tVendor ID, default 0x0\n");
                printf("\t  -P \t\tProduct ID, default 0x0\n");
                printf("\t  -S \t\tSerial ID, default None - search all devices\n");
                printf("\tIf flags [p/s] not used, application only read EEPROM\n");
                printf("\tFor set new parameters use:\n");
                printf("\t  -p <pid>\tnew PID, for ft2232h : 0x6010\n");
                printf("\t  -s <serial> \twrite, with serial number\n");
                printf("\t  -m <manufacturer> \twrite new manufacturer\n");
                printf("\t  -d <device> \twrite new device description\n");
                printf("\t  \n");
                printf("\tFor write default EEPROM:\n");
                printf("\t  -z \t\tdefaul parameters\n");
                printf("\t  \n");
                printf("\tPrint EEPROM to console:\n");
                printf("\t  -o\n");
                printf("\t  \n");
                printf("\tWhen you have many FTDI devices and there is no way to leave\n");
                printf("\tonly one connected, use this flags to work with multi-devices.\n");
                printf("\tAttention! This is an exceptional case, it is preferable to\n");
                printf("\tkeep only one device if possible. Otherwise, there is a chance\n");
                printf("\tof making a mistake and ruining the device.\n");
                printf("\tMulti-device access not sensitive to [-S] serial flag!\n");
                printf("\t  -! multi \t\tactivate multi_device mode\n");
                printf("\t  -@ <device>\t\tnumber of device\n");
                printf("\t  \n");
                printf("Examples:\n");
                printf("find devices        : ./ftdi-eeprom-config -V 0x403 -P 0x6010\n");
                printf("find devices        : ./ftdi-eeprom-config -V 0x403 -P 0x6010 -S 1234567\n");
                printf("search all FTDI dev : ./ftdi-eeprom-config -V 0x0 -P 0x0\n");
                printf("write new parameters: ./ftdi-eeprom-config -V 0x403 -P 0x6010 -S 1234567 -p 0x6030 -s New_1234\n");
                printf("list multi-device mode: ./ftdi-eeprom-config -V 0x403 -P 0x6010 -! multi -@ 0\n");
                printf("program multi-device: ./ftdi-eeprom-config -V 0x403 -P 0x6010 -! multi -@ 0 -s New_1234\n");
                printf("\n");
                return 0;
        }
    }

    if ((_param.ftdi = ftdi_new()) == 0)
    {
        printf("Failed to allocate ftdi structure :%s \n", ftdi_get_error_string(_param.ftdi));
        return EXIT_FAILURE;
    }

    if (NULL != _param.multi_param)
    {
        if (strcmp(_param.multi_param, "multi") == 0)
        {
            if (_param._multi_device < 0)
                printf("multimode disable. For activate multimode need set [-@ {device}].\n");
            else
            {
                _param._multi = 1;
                printf( "[ATTENTION] multimode enable! This mode can be dammage wrong FTDI device!\n");
            }
        }
    }

    if (_debug)
        debug_output(& _param);

    // Select first interface
    ftdi_set_interface(_param.ftdi, INTERFACE_ANY);

    struct ftdi_device_list *devlist;
    int devs_found;

    if ((devs_found = ftdi_usb_find_all(_param.ftdi, &devlist, _param.search_vid, _param.search_pid)) <= 0)
    {
        printf("No FTDI with VID/PID:0x%x:0x%x found\n", _param.search_vid, _param.search_pid);
        ftdi_free(_param.ftdi);
        return EXIT_FAILURE;
    }
    else if (devs_found > 1)
    {
        int index=0;
        int duplicates=0;

        printf("Number devices with VID/PID 0x%x:0x%x will be found: (%d)\n", _param.search_vid, _param.search_pid, devs_found);

        if (find_ftdi_serial(& _param, devlist, &duplicates, &index) < 0)
        {
            ftdi_list_free(&devlist);
            ftdi_free(_param.ftdi);
            return EXIT_FAILURE;
        }

        if (_debug)
            printf("[DEBUG] Serial duplicates %d; index %d;\n", duplicates, index);

        if (0 == _param._multi)
        {
            if (multi_device_disabled(& _param, devlist, &duplicates, &index) < 0)
            {
                ftdi_list_free(&devlist);
                ftdi_free(_param.ftdi);
                return EXIT_FAILURE;
            }
        }
        else
        {
            if (multi_device_enabled(& _param, devlist, &devs_found) < 0)
            {
                ftdi_list_free(&devlist);
                ftdi_free(_param.ftdi);
                return EXIT_FAILURE;
            }
        }
    }
    else if (devs_found == 1)
    {
        if (single_device_found(& _param, devlist) < 0)
        {
            ftdi_list_free(&devlist);
            ftdi_free(_param.ftdi);
            return EXIT_FAILURE;
        }
    }
    else
    {
        printf("No devices found\n");
        ftdi_list_free(&devlist);
        ftdi_free(_param.ftdi);
        return 0;
    }

    if (erase)
    {
        /* needed to determine EEPROM chip type */
        if ((ftret = ftdi_erase_eeprom(_param.ftdi)) < 0)
        {
            printf("Erase failed: %s", ftdi_get_error_string(_param.ftdi));
            ftdi_free(_param.ftdi);
            return -2;
        }
        get_ftdi_eeprom_type(& _param, ftret);
        ftdi_free(_param.ftdi);
        return 0;
    }

    if (do_write)
    {
        if (use_defaults)
            ftdi_eeprom_initdefaults(_param.ftdi, NULL, NULL, NULL);
        else
            ftdi_eeprom_initdefaults(_param.ftdi, _param.new_manufacturer, _param.new_product, _param.new_serial);

        ftret = ftdi_erase_eeprom(_param.ftdi);

        if (_param.new_pid >= 0)
        {
            if (ftdi_set_eeprom_value(_param.ftdi, PRODUCT_ID, _param.new_pid) < 0)
                printf("ftdi_set_eeprom_value PRODUCT_ID: %d (%s)\n", ftret, ftdi_get_error_string(_param.ftdi));
        }
        if (ftdi_set_eeprom_value(_param.ftdi, MAX_POWER, 500) <0)
            printf("ftdi_set_eeprom_value MAX_POWER: %d (%s)\n", ftret, ftdi_get_error_string(_param.ftdi));

        /* needed to determine EEPROM chip type */
        ftret = ftdi_erase_eeprom(_param.ftdi);
        get_ftdi_eeprom_type(& _param, ftret);

        if (ftdi_eeprom_build(_param.ftdi) < 0)
        {
            printf("Erase failed: %s", ftdi_get_error_string(_param.ftdi));
            ftdi_free(_param.ftdi);
            return 0;
        }

        ftret = ftdi_write_eeprom(_param.ftdi);
        {
            printf("ftdi_eeprom_decode: %d (%s)\n", ftret, ftdi_get_error_string(_param.ftdi));
            ftdi_free(_param.ftdi);
            return 0;
        }
    }

    if ( print_eeprom )
        read_decode_eeprom(_param.ftdi);

    return 0;
}
