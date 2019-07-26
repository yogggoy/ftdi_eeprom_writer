/* LIBFTDI EEPROM writre Serial number
   This program is distributed under the GPL, version 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <ftdi.h>

int read_decode_eeprom(struct ftdi_context *ftdi)
{
    int i, j, f;
    int value;
    int size;
    unsigned char buf[256];

    f = ftdi_read_eeprom(ftdi);
    if (f < 0)
    {
        fprintf(stderr, "ftdi_read_eeprom: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
        return -1;
    }


    ftdi_get_eeprom_value(ftdi, CHIP_SIZE, & value);
    if (value <0)
    {
        fprintf(stderr, "No EEPROM found or EEPROM empty\n");
        fprintf(stderr, "On empty EEPROM, use -w option to write default values\n");
        return -1;
    }
    fprintf(stderr, "Chip type %d ftdi_eeprom_size: %d\n", ftdi->type, value);
    if (ftdi->type == TYPE_R)
        size = 0xa0;
    else
        size = value;
    ftdi_get_eeprom_buf(ftdi, buf, size);
    for (i=0; i < size; i += 16)
    {
        fprintf(stdout,"0x%03x:", i);

        for (j = 0; j< 8; j++)
            fprintf(stdout," %02x", buf[i+j]);
        fprintf(stdout," ");
        for (; j< 16; j++)
            fprintf(stdout," %02x", buf[i+j]);
        fprintf(stdout," ");
        for (j = 0; j< 8; j++)
            fprintf(stdout,"%c", isprint(buf[i+j])?buf[i+j]:'.');
        fprintf(stdout," ");
        for (; j< 16; j++)
            fprintf(stdout,"%c", isprint(buf[i+j])?buf[i+j]:'.');
        fprintf(stdout,"\n");
    }

    f = ftdi_eeprom_decode(ftdi, 1);
    if (f < 0)
    {
        fprintf(stderr, "ftdi_eeprom_decode: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
        return -1;
    }
    return 0;
}

/*============================================================================\
|                                     main.c                                  |
\============================================================================*/

int main(int argc, char **argv)
{
    struct ftdi_context *ftdi;
    int f, i;
    char manufacturer[128], description[128], read_serial[128];

    int search_vid = 0x0;
    int search_pid = 0x0;
    char *search_serial = NULL;

    int new_pid = NULL;
    char *new_serial = NULL,
         *new_manufacturer = NULL,
         *new_product = NULL;

    int erase = 0;
    int do_write = 0;
    int use_defaults = 0;
    int retval = 0;
    int _debug = 0;
    int value;

    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "Failed to allocate ftdi structure :%s \n", ftdi_get_error_string(ftdi));
        return EXIT_FAILURE;
    }

    while ((i = getopt(argc, argv, "eV:P:S:p:s:m:d:zqh?")) != -1)
    {
        switch (i)
        {
            case 'e':
                erase = 1;
                break;
            case 'V':
                search_vid = strtoul(optarg, NULL, 0);
                break;
            case 'P':
                search_pid = strtoul(optarg, NULL, 0);
                break;
            case 'S':
                search_serial = optarg;
                break;
            case 'p':
                do_write  = 1;
                new_pid = strtoul(optarg, NULL, 0);
                break;
            case 's':
                do_write  = 1;
                new_serial = optarg;
                break;
            case 'm':
                do_write  = 1;
                new_manufacturer = optarg;
                break;
            case 'd':
                do_write  = 1;
                new_product = optarg;
                break;
            case 'z':
                do_write  = 1;
                use_defaults = 1;
                break;
            case 'q':
                _debug = 1;
                break;
            case 'h':
            case '?':
            default:
                fprintf(stdout, "User must be root\n", *argv);
                fprintf(stdout, "usage: %s [options]\n", *argv);
                fprintf(stdout, "\t  -e \t\terase\n");
                fprintf(stdout, "\t  -q \t\tshow parameters\n");
                fprintf(stdout, "\tFor search devices use flags[V/P/S]:\n");
                fprintf(stdout, "\t  -V \t\tVendor ID, default 0x0\n");
                fprintf(stdout, "\t  -P \t\tProduct ID, default 0x0\n");
                fprintf(stdout, "\t  -S \t\tSerial ID, default None - search all devices\n");
                fprintf(stdout, "\tIf flags [p/s] not used, application only read EEPROM\n");
                fprintf(stdout, "\tFor set new parameters use:\n");
                fprintf(stdout, "\t  -p <pid>\tnew PID, for ft2232h : 0x6010\n");
                fprintf(stdout, "\t  -s <serial> \twrite, with serial number\n");
                fprintf(stdout, "\t  -m <manufacturer> \twrite new manufacturer\n");
                fprintf(stdout, "\t  -d <device> \twrite new device description\n");
                fprintf(stdout, "\t  \n");
                fprintf(stdout, "\tFor write default EEPROM:\n");
                fprintf(stdout, "\t  -z \t\tdefaul parameters\n");
                fprintf(stdout, "Examples:\n");
                fprintf(stdout, "find devices        : ./ftdi-eeprom-config -V 0x403 -P 0x6010\n");
                fprintf(stdout, "find devices        : ./ftdi-eeprom-config -V 0x403 -P 0x6010 -S 1234567\n");
                fprintf(stdout, "search all FTDI dev : ./ftdi-eeprom-config -V 0x0 -P 0x0\n");
                fprintf(stdout, "write new parameters: ./ftdi-eeprom-config -V 0x403 -P 0x6010 -S 1234567 -p 0x6030 -s New_1234\n");
                fprintf(stdout, "\n");
                retval = -1;
                goto done;
        }
    }

    //debug------------------------------------------------------------
    if (_debug)
    {
        fprintf (stdout, "search vendor : 0x%x\n", search_vid);
        fprintf (stdout, "search product: 0x%x\n", search_pid);
        fprintf (stdout, "search serial : %s\n", search_serial);

        if (new_serial == NULL)
            fprintf (stdout, "[NULL]new serial : %s\n", new_serial);
        else
            fprintf (stdout, "new serial : %s\n", new_serial);
        if (new_pid == NULL)
            fprintf (stdout, "[NULL]new pid : %x\n", new_pid);
        else
            fprintf (stdout, "new pid : %x\n", new_pid);
    }
    //------------------------------------------------------------

    // Select first interface
    ftdi_set_interface(ftdi, INTERFACE_ANY);

    struct ftdi_device_list *devlist, *curdev;
    int res, duplicates=0, index=0, cnt=0;

    if ((res = ftdi_usb_find_all(ftdi, &devlist, search_vid, search_pid)) < 0)
    {
        fprintf(stderr, "No FTDI with VID/PID:0x%x:0x%x found\n", search_vid, search_pid);
        retval = EXIT_FAILURE;
        goto do_deinit;
    }
    else if (res > 1)
    {
        if (search_serial == NULL){
            fprintf(stderr, "[ERROR] Number devices with VID/PID %x:%x will be found: (%d)\n",
                search_vid, search_pid, res);
            fprintf(stderr, "    Use flag -S for search with serial ID\n");
            ftdi_list_free(&devlist);
            retval = EXIT_FAILURE;
            goto do_deinit;
        }
        printf("Number of FTDI devices with VID/PID:0x%x:0x%x found: %d\n", search_vid, search_pid, res);
        for (curdev = devlist; curdev != NULL; curdev = curdev->next, cnt++)
        {
            res = ftdi_usb_get_strings2(ftdi, curdev->dev, manufacturer, 128,description, 128, read_serial, 128);

            if (-9 == res)
            {
                fprintf(stderr, "[WARN] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                    ftdi_get_error_string(ftdi));
                fprintf(stderr, "\t\"get serial number failed\"\n");
            }
            else if (-8 == res)
            {
                fprintf(stderr, "[WARN] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                    ftdi_get_error_string(ftdi));
                fprintf(stderr, "\t\"get product description failed\"\n");
            }
            else if (-7 == res)
            {
                fprintf(stderr, "[WARN] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                    ftdi_get_error_string(ftdi));
                fprintf(stderr, "\t\"get product manufacturer failed\"\n");
            }
            else if (res < 0)
            {
                fprintf(stderr, "[FAIL] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                    ftdi_get_error_string(ftdi));
                ftdi_list_free(&devlist);
                retval = EXIT_FAILURE;
                goto do_deinit;
            }

            printf("Manufacturer: %s; Description: %s; Serial %s\n",
                manufacturer, description, read_serial
            );

            if ((search_serial != NULL) & (strcmp(read_serial, search_serial) == 0))
            {
                duplicates++;
                index = cnt;
            }
        }
        if (duplicates > 1)
        {
            fprintf(stderr, "[ERROR] Number of device with VID/PID and Serial %x:%x'%s' found: (%d)\n",
                search_vid, search_pid, search_serial, duplicates
            );
            fprintf(stderr, "    Please disconnect any FTDI devices.\n");
            retval = EXIT_FAILURE;
            goto do_deinit;
        }
        else
        {
            f = ftdi_usb_open_dev(ftdi, devlist[index].dev);
            if (f<0)
            {
                fprintf(stderr, "Unable to open device (%s)", ftdi_get_error_string(ftdi));
                goto done;
            }
        }
    }


    else if (res == 1)
    {
        res = ftdi_usb_get_strings2(ftdi, devlist[0].dev, NULL, 0, NULL, 0, read_serial, 128);
        if (-9 == res)
        {
            fprintf(stderr, "[WARN] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                ftdi_get_error_string(ftdi));
            fprintf(stderr, "\t\"get serial number failed\"\n");
        }
        else if (-8 == res)
        {
            fprintf(stderr, "[WARN] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                ftdi_get_error_string(ftdi));
            fprintf(stderr, "\t\"get product description failed\"\n");
        }
        else if (-7 == res)
        {
            fprintf(stderr, "[WARN] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                ftdi_get_error_string(ftdi));
            fprintf(stderr, "\t\"get product manufacturer failed\"\n");
        }

        else if (res < 0)
        {
            fprintf(stderr, "[FAIL] ftdi_usb_get_strings2 return : %d\n(%s)\n", res,
                ftdi_get_error_string(ftdi));
            ftdi_list_free(&devlist);
            retval = EXIT_FAILURE;
            goto do_deinit;
        }

        f = ftdi_usb_open_dev(ftdi,  devlist[0].dev);
        if (f<0)
        {
            fprintf(stderr, "Unable to open device (%s)", ftdi_get_error_string(ftdi));
            goto done;
        }
    }
    else
    {
        fprintf(stderr, "No devices found\n");
        ftdi_list_free(&devlist);
        goto do_deinit;
    }
    ftdi_list_free(&devlist);


    if (erase)
    {
        f = ftdi_erase_eeprom(ftdi); /* needed to determine EEPROM chip type */
        if (f < 0)
        {
            fprintf(stderr, "Erase failed: %s",
                    ftdi_get_error_string(ftdi));
            retval =  -2;
            goto done;
        }
        if (ftdi_get_eeprom_value(ftdi, CHIP_TYPE, & value) <0)
        {
            fprintf(stderr, "ftdi_get_eeprom_value: %d (%s)\n",
                    f, ftdi_get_error_string(ftdi));
        }
        if (value == -1)
            fprintf(stderr, "No EEPROM\n");
        else if (value == 0)
            fprintf(stderr, "Internal EEPROM\n");
        else
            fprintf(stderr, "Found 93x%02x\n", value);
        retval = 0;
        goto done;
    }

    if (do_write)
    {
        if (use_defaults)
        {
            ftdi_eeprom_initdefaults(ftdi, NULL, NULL, NULL);
        }
        else
        {
            ftdi_eeprom_initdefaults(ftdi, new_manufacturer, new_product, new_serial);
        }
        f = ftdi_erase_eeprom(ftdi);

        if (new_pid != NULL)
        {
            if (ftdi_set_eeprom_value(ftdi, PRODUCT_ID, new_pid) <0)
            {
                fprintf(stderr, "ftdi_set_eeprom_value PRODUCT_ID: %d (%s)\n",
                        f, ftdi_get_error_string(ftdi));
            }
        }
        if (ftdi_set_eeprom_value(ftdi, MAX_POWER, 500) <0)
        {
            fprintf(stderr, "ftdi_set_eeprom_value MAX_POWER: %d (%s)\n",
                    f, ftdi_get_error_string(ftdi));
        }
        f = ftdi_erase_eeprom(ftdi);/* needed to determine EEPROM chip type */
        if (ftdi_get_eeprom_value(ftdi, CHIP_TYPE, & value) <0)
        {
            fprintf(stderr, "ftdi_get_eeprom_value: %d (%s)\n",
                    f, ftdi_get_error_string(ftdi));
        }
        if (value == -1)
            fprintf(stderr, "No EEPROM\n");
        else if (value == 0)
            fprintf(stderr, "Internal EEPROM\n");
        else
            fprintf(stderr, "Found 93x%02x\n", value);
        f=(ftdi_eeprom_build(ftdi));
        if (f < 0)
        {
            fprintf(stderr, "Erase failed: %s",
                    ftdi_get_error_string(ftdi));
            retval = -2;
            goto done;
        }
        f = ftdi_write_eeprom(ftdi);
        {
            fprintf(stderr, "ftdi_eeprom_decode: %d (%s)\n",
                    f, ftdi_get_error_string(ftdi));
            retval = 1;
            goto done;
        }
    }
    retval = read_decode_eeprom(ftdi);

done:
    ftdi_usb_close(ftdi);
do_deinit:
    ftdi_free(ftdi);
    return retval;
}
