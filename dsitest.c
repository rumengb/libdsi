
/*
 * Copyright (c) 2009, Roland Roberts
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "dsi.h"

#define EXP_TIME 900
#define FILE_NAME "XXXX"

int
main(int argc, char **argv)
{
    dsi_camera_t *dsi;
    int i, exposure_ticks;
    #define SCRATCH_LENGTH 100
    char scratch[SCRATCH_LENGTH];
    char *device = &argv[1];

	dsi_inint();
    libdsi_set_verbose_init((0));
    dsi = dsi_open(argv[1]);
    if (dsi == 0) {
        fprintf(stderr, "failed to open DSI device %s\n", argv[1]);
        exit(1);
    }

    dsi_set_verbose(dsi, (0));

    fprintf(stderr, "dsi_get_camera_name(dsi)    = %s\n", dsi_get_camera_name(dsi));
    fprintf(stderr, "dsi_get_model_name(dsi)    = %s\n", dsi_get_model_name(dsi));
    fprintf(stderr, "dsi_get_chip_name(dsi)      = %s\n", dsi_get_chip_name(dsi));
    fprintf(stderr, "dsi_get_serial_number(dsi)  = %s\n", dsi_get_serial_number(dsi));

    fprintf(stderr, "dsicmd_get_row_count_odd(dsi)  = %d\n", dsicmd_get_row_count_odd(dsi));
    fprintf(stderr, "dsicmd_get_row_count_even(dsi) = %d\n", dsicmd_get_row_count_even(dsi));
    fprintf(stderr, "dsicmd_get_gain(dsi)           = %d\n", dsicmd_get_gain(dsi));
    fprintf(stderr, "dsicmd_get_offset(dsi)         = %d\n", dsicmd_get_offset(dsi));
    fprintf(stderr, "dsicmd_get_readout_speed(dsi)  = %d\n", dsicmd_get_readout_speed(dsi));
    fprintf(stderr, "dsicmd_get_readout_mode(dsi)   = %d\n", dsicmd_get_readout_mode(dsi));
    fprintf(stderr, "dsicmd_get_readout_delay(dsi)  = %d\n", dsicmd_get_readout_delay(dsi));
    fprintf(stderr, "dsicmd_get_vdd_mode(dsi)       = %d\n", dsicmd_get_vdd_mode(dsi));
    fprintf(stderr, "dsicmd_get_flush_mode(dsi)     = %d\n", dsicmd_get_flush_mode(dsi));
    fprintf(stderr, "dsi_get_temperature(dsi)    = %.1f\n", dsi_get_temperature(dsi));
    fprintf(stderr, "dsicmd_get_usb_speed(dsi)      = %d (%s)\n",
            dsicmd_get_usb_speed(dsi), dsicmd_lookup_usb_speed(dsicmd_get_usb_speed(dsi)));

    fprintf(stderr, "dsi_get_image_width(dsi)  = %d\n", dsi_get_image_width(dsi));
    fprintf(stderr, "dsi_get_image_height(dsi) = %d\n", dsi_get_image_height(dsi));

    for (i = 0; i < 1; i++) {
        int j, k, m;
        int code;
        FILE *fptr;
        char buffer[1024];
        unsigned int *image;
        /* This is a low-level approach that needs to be wrapped.  We will
           probably want both a synchronous and asynchronous API function.
           One idea for the latter will set everything up and verify that the
           exposure is running then return ticks remaining. */


        fprintf(stderr, "Starting exposure...\n");
        dsi_start_image(dsi, EXP_TIME);
        fprintf(stderr, "Reading image...\n");
        while ((code = dsi_read_image(dsi, &image, O_NONBLOCK)) != 0) {
            if (code == EWOULDBLOCK) {
                fprintf(stderr, "image not ready, sleeping...\n");
                sleep(1);
            } else {
                dsicmd_abort_exposure(dsi);
                dsicmd_reset_camera(dsi);
                dsicmd_reset_camera(dsi);
                dsi_start_image(dsi, EXP_TIME);
            }
        }
        snprintf(buffer, 1024, "%s.%04d.pgm", FILE_NAME, i);
        fptr = fopen(buffer, "w");
        fprintf(fptr, "P2\n%d %d\n65535\n", dsi_get_image_width(dsi), dsi_get_image_height(dsi));
        for (m = j = 0; j < dsi_get_image_height(dsi); j++) {
            for (k = 0; k < dsi_get_image_width(dsi); k++, m++) {
                unsigned short value = image[m];
                fprintf(fptr, "%d ", value);
            }
            fprintf(fptr, "\n");
        }
        fclose(fptr);
     }
     dsi_close(dsi);
	 //dsi_exit();
}