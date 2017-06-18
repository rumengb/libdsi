/*
 * Copyright (c) 2009, Roland Roberts <roland@astrofoto.org>
 *
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>

#include <usb.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "dsi.h"

#define MILLISEC 2

int dsicmd_usb_command(dsi_camera_t *dsi, unsigned char *ibuf, int ibuf_len, int obuf_len);

static int verbose_init = 0;

struct DSI_CAMERA {
    struct usb_device *device;
    struct usb_dev_handle *handle;
    unsigned char command_sequence_number;

    int is_simulation;
    int eeprom_length;
    int test_pattern;

    int exposure_time;
    int read_height_even;
    int read_height_odd;
    int read_height;
    int read_width;
    int read_bpp;
    int image_width;
    int image_height;
    int image_offset_x;
    int image_offset_y;
    int is_color;
    int is_binnable;

    int amp_gain_pct;
    int amp_offset_pct;

    char chip_name[32];
    char camera_name[32];
    char model_name[32];
    char serial_number[32];

    enum DSI_FW_DEBUG fw_debug;
    enum DSI_USB_SPEED usb_speed;

    union {
        int value;
        unsigned char s[4];
        struct {
            unsigned char family;
            unsigned char model;
            unsigned char version;
            unsigned char revision;
        } c;
    } version;

    int read_command_timeout;
    int write_command_timeout;
    int read_image_timeout;

    enum DSI_IMAGE_STATE imaging_state;

    unsigned int last_time;
    int log_commands;

    size_t read_size_odd, read_size_even;
    unsigned char *read_buffer_odd;
    unsigned char *read_buffer_even;
    unsigned int *image_buffer;
    size_t image_buffer_size;
};


/**
 * Look up the human-readable mnemonic for a numeric command code.
 *
 * @param cmd Command code to look up.
 * @param buffer Space to write the string mnemonic.
 * @param bufsize Size of write buffer.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *
dsicmd_lookup_command_name_r(dsi_command_t cmd, char *buffer, int bufsize) {
    const char *bufptr = 0;

    switch(cmd) {
        case PING:
            bufptr = "PING";
            break;
        case RESET:
            bufptr = "RESET";
            break;
        case ABORT:
            bufptr = "ABORT";
            break;
        case TRIGGER:
            bufptr = "TRIGGER";
            break;
        case CLEAR_TS:
            bufptr = "CLEAR_TS";
            break;
        case GET_VERSION:
            bufptr = "GET_VERSION";
            break;
        case GET_STATUS:
            bufptr = "GET_STATUS";
            break;
        case GET_TIMESTAMP:
            bufptr = "GET_TIMESTAMP";
            break;
        case GET_EEPROM_LENGTH:
            bufptr = "GET_EEPROM_LENGTH";
            break;
        case GET_EEPROM_BYTE:
            bufptr = "GET_EEPROM_BYTE";
            break;
        case SET_EEPROM_BYTE:
            bufptr = "SET_EEPROM_BYTE";
            break;
        case GET_GAIN:
            bufptr = "GET_GAIN";
            break;
        case SET_GAIN:
            bufptr = "SET_GAIN";
            break;
        case GET_OFFSET:
            bufptr = "GET_OFFSET";
            break;
        case SET_OFFSET:
            bufptr = "SET_OFFSET";
            break;
        case GET_EXP_TIME:
            bufptr = "GET_EXP_TIME";
            break;
        case SET_EXP_TIME:
            bufptr = "SET_EXP_TIME";
            break;
        case GET_EXP_MODE:
            bufptr = "GET_EXP_MODE";
            break;
        case SET_EXP_MODE:
            bufptr = "SET_EXP_MODE";
            break;
        case GET_VDD_MODE:
            bufptr = "GET_VDD_MODE";
            break;
        case SET_VDD_MODE:
            bufptr = "SET_VDD_MODE";
            break;
        case GET_FLUSH_MODE:
            bufptr = "GET_FLUSH_MODE";
            break;
        case SET_FLUSH_MODE:
            bufptr = "SET_FLUSH_MODE";
            break;
        case GET_CLEAN_MODE:
            bufptr = "GET_CLEAN_MODE";
            break;
        case SET_CLEAN_MODE:
            bufptr = "SET_CLEAN_MODE";
            break;
        case GET_READOUT_SPEED:
            bufptr = "GET_READOUT_SPEED";
            break;
        case SET_READOUT_SPEED:
            bufptr = "SET_READOUT_SPEED";
            break;
        case GET_READOUT_MODE:
            bufptr = "GET_READOUT_MODE";
            break;
        case SET_READOUT_MODE:
            bufptr = "SET_READOUT_MODE";
            break;
        case GET_READOUT_DELAY:
            bufptr = "GET_NORM_READOUT_DELAY";
            break;
        case SET_READOUT_DELAY:
            bufptr = "SET_NORM_READOUT_DELAY";
            break;
        case GET_ROW_COUNT_ODD:
            bufptr = "GET_ROW_COUNT_ODD";
            break;
        case SET_ROW_COUNT_ODD:
            bufptr = "SET_ROW_COUNT_ODD";
            break;
        case GET_ROW_COUNT_EVEN:
            bufptr = "GET_ROW_COUNT_EVEN";
            break;
        case SET_ROW_COUNT_EVEN:
            bufptr = "SET_ROW_COUNT_EVEN";
            break;
        case GET_TEMP:
            bufptr = "GET_TEMP";
            break;
        case GET_EXP_TIMER_COUNT:
            bufptr = "GET_EXP_TIMER_COUNT";
            break;
        case PS_ON:
            bufptr = "PS_ON";
            break;
        case PS_OFF:
            bufptr = "PS_OFF";
            break;
        case CCD_VDD_ON:
            bufptr = "CCD_VDD_ON";
            break;
        case CCD_VDD_OFF:
            bufptr = "CCD_VDD_OFF";
            break;
        case AD_READ:
            bufptr = "AD_READ";
            break;
        case AD_WRITE:
            bufptr = "AD_WRITE";
            break;
        case TEST_PATTERN:
            bufptr = "TEST_PATTERN";
            break;
        case GET_DEBUG_VALUE:
            bufptr = "GET_DEBUG_VALUE";
            break;
        case GET_EEPROM_VIDPID:
            bufptr = "GET_EEPROM_VIDPID";
            break;
        case SET_EEPROM_VIDPID:
            bufptr = "SET_EEPROM_VIDPID";
            break;
        case ERASE_EEPROM:
            bufptr = "ERASE_EEPROM";
            break;
    }
    if (bufptr != 0) {
        snprintf(buffer, bufsize, "%s", bufptr);
    } else {
        snprintf(buffer, bufsize, "CMD_UNKNOWN, 0x%02x", cmd);
    }
    return buffer;
}

/**
 * Look up the human-readable mnemonic for a numeric command code, non-reentrant.
 *
 * @param cmd Command code to look up.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *
dsicmd_lookup_command_name(dsi_command_t cmd) {
    static char scratch[100];
    return dsicmd_lookup_command_name_r(cmd, scratch, 100);
}

/**
 * Look up the human-readable mnemonic for a numeric imaging state code,
 * non-reentrant.
 *
 * @param state Imaging state code to look up.
 * @param buffer Space to write the string mnemonic.
 * @param bufsize Size of write buffer.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *
dsicmd_lookup_image_state_r(enum DSI_IMAGE_STATE state, char *buffer, int bufsize) {
    char *bufptr = 0;

    switch(state) {
        case DSI_IMAGE_IDLE:
            bufptr = "DSI_IMAGE_IDLE";
            break;
        case DSI_IMAGE_EXPOSING:
            bufptr = "DSI_IMAGE_EXPOSING";
            break;
        case DSI_IMAGE_ABORTING:
            bufptr = "DSI_IMAGE_ABORTING";
            break;
    }
    if (bufptr != 0) {
        snprintf(buffer, bufsize, "%s", bufptr);
    } else {
        snprintf(buffer, bufsize, "DSI_IMAGE_UNKNOWN, 0x%02x", state);
    }
    return buffer;
}

/**
 * Look up the human-readable mnemonic for a numeric imaging state code,
 * non-reentrant.
 *
 * @param state Imaging state code to look up.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *
dsicmd_lookup_image_state(enum DSI_IMAGE_STATE state) {
    /* not thread safe. */
    static char unknown[100];
    return dsicmd_lookup_image_state_r(state, unknown, 100);
}

/**
 * Look up the human-readable mnemonic for a USB speed code.
 *
 * @param speed USB speed code to look up.
 * @param buffer Space to write the string mnemonic.
 * @param bufsize Size of write buffer.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *
dsicmd_lookup_usb_speed_r(enum DSI_USB_SPEED speed, char *buffer, int bufsize) {
    char *bufptr = 0;
    switch(speed) {
        case DSI_USB_SPEED_FULL:
            bufptr = "DSI_USB_SPEED_FULL";
            break;
        case DSI_USB_SPEED_HIGH:
            bufptr = "DSI_USB_SPEED_HIGH";
            break;
    }
    if (bufptr != 0) {
        snprintf(buffer, bufsize, "%s", bufptr);
    } else {
        snprintf(buffer, bufsize, "DSI_USB_SPEED_UNKNOWN, 0x%02x", speed);
    }
    return buffer;
}

/**
 * Look up the human-readable mnemonic for a USB speed code, non-reentrant.
 *
 * @param speed USB speed code to look up.
 *
 * @return Pointer to buffer containing the mnemonic.
 */
const char *
dsicmd_lookup_usb_speed(enum DSI_USB_SPEED speed) {
    /* not thread safe. */
    static char unknown[100];
    return dsicmd_lookup_usb_speed_r(speed, unknown, 100);
}


/**
 * Utility to return system clock time in milliseconds.
 *
 * @return integer value of system clock in milliseconds.
 */
static unsigned int
dsi_get_sysclock_ms() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec * 1000 + tv.tv_usec/1000);
}

/**
 * Pretty-print a DSI command buffer for logging purposes.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param iswrite Was this command a write to the DSI (true), or a read from
 *        the DSI (false).
 * @param prefix String prefix for logging message.
 * @param length Buffer length
 * @param buffer Buffer sent to/received from DSI.
 * @param result If the command had a return value, this is a pointer to that value.
 */
static void
dsi_log_command_info(dsi_camera_t *dsi,
                      int iswrite, const char *prefix, unsigned int length,
                      char *buffer, unsigned int *result) {
    if (!dsi->log_commands)
        return;

    unsigned int now = dsi_get_sysclock_ms();
    int i, count;

    fprintf(stderr, "%-4s %02x %-4s [dt=%d]",
            prefix, length, "", now-dsi->last_time);

    if (strcmp(prefix, "r 86") != 0) {
        char scratch[50];
        for (i = count = 0; i < length; i++) {
            if (i == 8)
                break;
            if ((i % 8) == 0) {
                fprintf(stderr, "\n    %08x:", i);
            }
            fprintf(stderr, " %02x", (unsigned char) buffer[i]);
        }
        for (i = 8-(length%8); i != 8 && i > 0; i--) {
            fprintf(stderr, "   ");
        }

        fprintf(stderr, "    %s",
                iswrite ? dsicmd_lookup_command_name_r(buffer[2], scratch, 50) : "ACK");

        if (result) {
            fprintf(stderr, " %d", (unsigned int) *result);
            if (*result < 128 && isprint(*result)) {
                fprintf(stderr, " (%c)", *result);
            }
        }

    }
    fprintf(stderr, "\n");
    dsi->last_time = now;
    return;
}

/**
 * Decode byte 4 of the buffer as an 8-bit unsigned integer.
 *
 * @param buffer raw query result buffer from DSI.
 *
 * @return decoded value as an unsigned integer.
 */
static unsigned int
dsi_get_byte_result(unsigned char *buffer) {
    return (unsigned int) buffer[3];
}

/**
 * Decode bytes 4-5 of the buffer as a 16-bit big-endian unsigned integer.
 *
 * @param buffer raw query result buffer from DSI.
 *
 * @return decoded value as an unsigned integer.
 */
static unsigned int
dsi_get_short_result(unsigned char *buffer) {
    return (unsigned int) ((buffer[4] << 8) | buffer[3]);
}

/**
 * Decode bytes 4-7 of the buffer as a 32-bit big-endian unsigned integer.
 *
 * @param buffer raw query result buffer from DSI.
 *
 * @return decoded value as an unsigned integer.
 */
static unsigned int
dsi_get_int_result(unsigned char *buffer) {
    unsigned int result;
    result =                 buffer[6];
    result = (result << 8) | buffer[5];
    result = (result << 8) | buffer[4];
    result = (result << 8) | buffer[3];
    return result;
}

/**
 * Internal helper for sending a command to the DSI device.  If the command is
 * one which requires no parameters, then the actual execution will be
 * delegated to command(DeviceCommand,int,int)
 *
 * @param cmd command to be executed.
 *
 * @return decoded command response.
 */
int
dsicmd_command_1(dsi_camera_t *dsi, dsi_command_t cmd) {
    if (dsi->is_simulation) {
        return 0;
    }

    // This is the one place where having class-based enums instead of
    // built-in enums is annoying: you can't use a switch statement here.
    switch (cmd) {
        case PING:
        case RESET:
        case ABORT:
        case TRIGGER:
        case PS_ON:
        case PS_OFF:
        case CCD_VDD_ON:
        case CCD_VDD_OFF:
        case TEST_PATTERN:
        case ERASE_EEPROM:
        case GET_VERSION:
        case GET_STATUS:
        case GET_TIMESTAMP:
        case GET_EXP_TIME:
        case GET_EXP_TIMER_COUNT:
        case GET_EEPROM_VIDPID:
        case GET_EEPROM_LENGTH:
        case GET_GAIN:
        case GET_EXP_MODE:
        case GET_VDD_MODE:
        case GET_FLUSH_MODE:
        case GET_CLEAN_MODE:
        case GET_READOUT_SPEED:
        case GET_READOUT_MODE:
        case GET_OFFSET:
        case GET_READOUT_DELAY:
        case GET_TEMP:
        case GET_ROW_COUNT_ODD:
        case GET_ROW_COUNT_EVEN:
            return dsicmd_command_3(dsi, cmd, 0, 3);

        default:
            return -1;
    }
}

/**
 * Internal helper for sending a command to the DSI device.  This determines
 * what the length of the actual command will be and then delgates to
 * command(DeviceCommand,int,int) or command(DeviceCommand).
 *
 * @param cmd command to be executed.
 * @param param command parameter, ignored for SOME commands.
 *
 * @return decoded command response.
 */
int
dsicmd_command_2(dsi_camera_t *dsi, dsi_command_t cmd, int param) {
    if (dsi->is_simulation) {
        return 0;
    }

    // This is the one place where having class-based enums instead of
    // built-in enums is annoying: you can't use a switch statement here.
    switch (cmd) {

        case GET_EEPROM_BYTE:
        case SET_GAIN:
        case SET_EXP_MODE:
        case SET_VDD_MODE:
        case SET_FLUSH_MODE:
        case SET_CLEAN_MODE:
        case SET_READOUT_SPEED:
        case SET_READOUT_MODE:
        case AD_READ:
        case GET_DEBUG_VALUE:
            return dsicmd_command_3(dsi, cmd, param, 4);

        case SET_EEPROM_BYTE:
        case SET_OFFSET:
        case SET_READOUT_DELAY:
        case SET_ROW_COUNT_ODD:
        case SET_ROW_COUNT_EVEN:
        case AD_WRITE:
            return dsicmd_command_3(dsi, cmd, param, 5);

        case SET_EXP_TIME:
        case SET_EEPROM_VIDPID:
            return dsicmd_command_3(dsi, cmd, param, 7);

        default:
            return dsicmd_command_1(dsi, cmd);
    }


}

/**
 * Internal helper for sending a command to the DSI device.  This determines
 * what the expected response length is and then delegates actually processing
 * to command(DeviceCommand,int,int,int).
 *
 * @param cmd command to be executed.
 * @param param
 * @param param_len
 *
 * @return decoded command response.
 */
int
dsicmd_command_3(dsi_camera_t *dsi, dsi_command_t cmd, int param, int param_len) {
    switch(cmd) {
        case PING:
        case RESET:
        case ABORT:
        case TRIGGER:
        case TEST_PATTERN:
        case SET_EEPROM_BYTE:
        case SET_GAIN:
        case SET_OFFSET:
        case SET_EXP_TIME:
        case SET_VDD_MODE:
        case SET_FLUSH_MODE:
        case SET_CLEAN_MODE:
        case SET_READOUT_SPEED:
        case SET_READOUT_MODE:
        case SET_READOUT_DELAY:
        case SET_ROW_COUNT_ODD:
        case SET_ROW_COUNT_EVEN:
        case PS_ON:
        case PS_OFF:
        case CCD_VDD_ON:
        case CCD_VDD_OFF:
        case AD_WRITE:
        case SET_EEPROM_VIDPID:
        case ERASE_EEPROM:
            return dsicmd_command_4(dsi, cmd, param, param_len, 3);

        case GET_EEPROM_LENGTH:
        case GET_EEPROM_BYTE:
        case GET_GAIN:
        case GET_EXP_MODE:
        case GET_VDD_MODE:
        case GET_FLUSH_MODE:
        case GET_CLEAN_MODE:
        case GET_READOUT_SPEED:
        case GET_READOUT_MODE:
            return dsicmd_command_4(dsi, cmd, param, param_len, 4);

        case GET_OFFSET:
        case GET_READOUT_DELAY:
        case SET_EXP_MODE:
        case GET_ROW_COUNT_ODD:
        case GET_ROW_COUNT_EVEN:
        case GET_TEMP:
        case AD_READ:
        case GET_DEBUG_VALUE:
            return dsicmd_command_4(dsi, cmd, param, param_len, 5);

        case GET_VERSION:
        case GET_STATUS:
        case GET_TIMESTAMP:
        case GET_EXP_TIME:
        case GET_EXP_TIMER_COUNT:
        case GET_EEPROM_VIDPID:
            return dsicmd_command_4(dsi, cmd, param, param_len, 7);

        default:
            return -1;
    }
}

/**
 * Internal helper for sending a command to the DSI device.  This formats the
 * command as a sequence of bytes and delegates to command(unsigned char *,int,int)
 *
 * @param cmd command to be executed.
 * @param val command parameter value.
 * @param val_bytes size of the parameter value field.
 * @param ret_bytes size of the return value field.
 *
 * @return decoded command response.
 */
int
dsicmd_command_4(dsi_camera_t *dsi, dsi_command_t cmd,
              int val, int val_bytes, int ret_bytes) {
    const size_t size = 0x40;
    unsigned char buffer[0x40];
    dsi->command_sequence_number++;

    buffer[0] = val_bytes;
    buffer[1] = dsi->command_sequence_number;
    buffer[2] = cmd;

    switch (val_bytes) {
        case 3:
            break;
        case 4:
            buffer[3] = val;
            break;
        case 5:
            buffer[3] = 0x0ff & val;
            buffer[4] = 0x0ff & (val >> 8);
            break;
        case 7:
            buffer[3] = 0x0ff & val;
            buffer[4] = 0x0ff & (val >>  8);
            buffer[5] = 0x0ff & (val >> 16);
            buffer[6] = 0x0ff & (val >> 24);
            break;
        default:
            return -1;
    }
    return dsicmd_usb_command(dsi, buffer, val_bytes, ret_bytes);
}

/**
 * Write a command buffer to the DSI device and return the decoded result value.
 *
 * @param ibuf raw command buffer, sent as input to the DSI.
 * @param ibuf_len length of the command buffer, in bytes.
 * @param obuf_len expected length of the response buffer, in bytes.
 *
 * @return decoded response as appropriate for the command.
 *
 * DSI commands return either 0, 1, 2, or 4 byte results.  The results are
 * nominally unsigned integers although in some cases (e.g. GET_VERSION),
 * the 4 bytes are actually 4 separate bytes.  However, all 4-byte responses
 * are treated as 32-bit unsigned integers and are decoded and returned that
 * way.  Similarly, 2-byte responses are treated as 16-bit unsigned integers
 * and are decoded and returned that way.
 */
int
dsicmd_usb_command(dsi_camera_t *dsi, unsigned char *ibuf, int ibuf_len, int obuf_len) {
    /* Yes, there is a conflict here.  Our decoded result is logically
     * unsigned, but we need to be able to return negative values to indicate
     * errors.  Worse, the GET_VERSION command seems to always return a buffer
     * with the high bit set making it logically negative.  The command
     * dsi_get_version will ignore the sign meaning we have one case where a
     * failure can escape notice.  */
    unsigned int value = 0, result = 0;
    int retcode;

    /* The DSI endpoint for commands has is defined to only be able to return
     * 64 bytes. */
    size_t obuf_size = 0x40;
    char obuf[0x40];

    switch (ibuf_len) {
        case 3:
            value = 0;
            break;
        case 4:
            value = dsi_get_byte_result((unsigned char *) ibuf);
            value = (unsigned int) ibuf[3];
            break;
        case 5:
            value = dsi_get_short_result((unsigned char *) ibuf);
            break;
        case 7:
            value = dsi_get_int_result((unsigned char *) ibuf);
            break;
        default:
            assert((ibuf_len >= 3) || (ibuf_len <= 7) || (ibuf_len != 6));
            break;
    }
    if (dsi->log_commands)
        dsi_log_command_info(dsi, 1, "w 1", (unsigned int) ibuf[0],
                         (char *) ibuf, (ibuf_len > 3 ? &value : 0));


    if (dsi->last_time == 0) {
        dsi->last_time = dsi_get_sysclock_ms();
    }

    retcode = usb_bulk_write(dsi->handle, 0x01,
                             (char *) ibuf, ibuf[0], dsi->write_command_timeout);
    if (retcode < 0)
        return retcode;

    retcode = usb_bulk_read(dsi->handle, 0x81, obuf, obuf_size, dsi->read_command_timeout);
    if (retcode < 0)
        return retcode;

    assert(obuf[0] == retcode);
    assert((unsigned char) obuf[1] == dsi->command_sequence_number);
    assert(obuf[2] == 6);

    switch (obuf_len) {
        case 3:
            result = 0;
            break;
        case 4:
            result = dsi_get_byte_result((unsigned char *) obuf);
            break;
        case 5:
            result = dsi_get_short_result((unsigned char *) obuf);
            break;
        case 7:
            result = dsi_get_int_result((unsigned char *) obuf);
            break;
        default:
            assert((obuf_len >= 3) && (obuf_len <= 7) && (obuf_len != 6));
            break;
    }

    if (dsi->log_commands)
        dsi_log_command_info(dsi, 0, "r 81", obuf[0], obuf, (obuf_len > 3 ? &result : 0));

    return result;
}

int dsicmd_wake_camera(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, PING);
}

int dsicmd_reset_camera(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, RESET);
}

int dsicmd_set_exposure_time(dsi_camera_t *dsi, int ticks) {
    /* FIXME: check time for validity */
    dsi->exposure_time = ticks;
    return dsicmd_command_2(dsi, SET_EXP_TIME, ticks);
}

int dsicmd_get_exposure_time(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_EXP_TIME);
}

int dsicmd_get_exposure_time_left(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_EXP_TIMER_COUNT);
}

int dsicmd_start_exposure(dsi_camera_t *dsi) {
    dsi->imaging_state = DSI_IMAGE_EXPOSING;
    return dsicmd_command_1(dsi, TRIGGER);
}

int dsicmd_abort_exposure(dsi_camera_t *dsi) {
    dsi->imaging_state = DSI_IMAGE_ABORTING;
    return dsicmd_command_1(dsi, ABORT);
}

int dsicmd_set_gain(dsi_camera_t *dsi, int gain) {
    if (gain < 0 || gain > 63)
        return -1;
    return dsicmd_command_2(dsi, SET_GAIN, gain);
}

int dsicmd_get_gain(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_GAIN);
}

int dsicmd_set_offset(dsi_camera_t *dsi, int offset) {
    /* FIXME: check offset for validity */
    return dsicmd_command_2(dsi, SET_OFFSET, offset);
}

int dsicmd_get_offset(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_OFFSET);
}

int dsicmd_set_vdd_mode(dsi_camera_t *dsi, int mode) {
    /* FIXME: check mode for validity */
    return dsicmd_command_2(dsi, SET_VDD_MODE, mode);
}

int dsicmd_get_vdd_mode(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_VDD_MODE);
}

int dsicmd_set_flush_mode(dsi_camera_t *dsi, int mode) {
    /* FIXME: check mode for validity */
    return dsicmd_command_2(dsi, SET_FLUSH_MODE, mode);
}

int dsicmd_get_flush_mode(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_FLUSH_MODE);
}

int dsicmd_set_readout_mode(dsi_camera_t *dsi, int mode) {
    /* FIXME: check mode for validity */
    return dsicmd_command_2(dsi, SET_READOUT_MODE, mode);
}

int dsicmd_get_readout_mode(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_READOUT_MODE);
}

int dsicmd_set_readout_delay(dsi_camera_t *dsi, int delay) {
    /* FIXME: check mode for validity */
    return dsicmd_command_2(dsi, SET_READOUT_DELAY, delay);
}

int dsicmd_get_readout_delay(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_READOUT_DELAY);
}

int dsicmd_set_readout_speed(dsi_camera_t *dsi, int speed) {
    /* FIXME: check speed for validity */
    return dsicmd_command_2(dsi, SET_READOUT_SPEED, speed);
}

int dsicmd_get_readout_speed(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_READOUT_SPEED);
}

int dsicmd_get_temperature(dsi_camera_t *dsi) {
    return dsicmd_command_1(dsi, GET_TEMP);
}

int dsicmd_get_row_count_odd(dsi_camera_t *dsi) {
    /* While we read the value from the camera, it lies except for the
       original DSI.  So if it has been set, we just use it as-is. */
    if (dsi->read_height_odd <= 0)
        dsi->read_height_odd = dsicmd_command_1(dsi, GET_ROW_COUNT_ODD);
    return dsi->read_height_odd;
}

int dsicmd_get_row_count_even(dsi_camera_t *dsi) {
    /* While we read the value from the camera, it lies except for the
       original DSI.  So if it has been set, we just use it as-is. */
    if (dsi->read_height_even <= 0)
        dsi->read_height_even = dsicmd_command_1(dsi, GET_ROW_COUNT_EVEN);
    return dsi->read_height_even;
}

int dsi_set_amp_gain(dsi_camera_t *dsi, int gain) {
    if (gain > 100)
        dsi->amp_gain_pct = 100;
    else if (gain < 0)
        dsi->amp_gain_pct = 0;
    else
        dsi->amp_gain_pct = gain;
    return dsi->amp_gain_pct;
}

int dsi_get_amp_gain(dsi_camera_t *dsi, int gain) {
    return dsi->amp_gain_pct;
}

int dsi_set_amp_offset(dsi_camera_t *dsi, int offset) {
    if (offset > 100)
        dsi->amp_offset_pct = 100;
    else if (offset < 0)
        dsi->amp_offset_pct = 0;
    else
        dsi->amp_offset_pct = offset;
    return dsi->amp_offset_pct;
}

int dsi_get_amp_offset(dsi_camera_t *dsi) {
    return dsi->amp_offset_pct;
}

int dsi_get_image_width(dsi_camera_t *dsi) {
    return dsi->image_width;
}

int dsi_get_image_height(dsi_camera_t *dsi) {
    return dsi->image_height;
}

int
dsi_get_image(dsi_camera_t *dsi, unsigned char **buffer, size_t *size) {
    switch (dsi->imaging_state) {
        case DSI_IMAGE_IDLE:
            dsicmd_set_exposure_time(dsi, dsi->exposure_time);
            dsicmd_start_exposure(dsi);
            break;

        case DSI_IMAGE_EXPOSING:
            break;

        default:
            break;

    }
    return 0;
}

double dsi_get_temperature(dsi_camera_t *dsi) {
	int raw_temp = dsicmd_get_temperature(dsi);
	return floor((float) raw_temp / 25.6) / 10.0;
}

unsigned char
dsicmd_get_eeprom_byte(dsi_camera_t *dsi, int offset) {
    if (dsi->eeprom_length < 0) {
        dsi->eeprom_length = dsicmd_command_1(dsi, GET_EEPROM_LENGTH);
    }
    if ((offset < 0) || (offset > dsi->eeprom_length))
        return 0xff;
    return dsicmd_command_2(dsi, GET_EEPROM_BYTE, offset);
}

unsigned char
dsicmd_set_eeprom_byte(dsi_camera_t *dsi, char byte, int offset) {
    if (dsi->eeprom_length < 0) {
        dsi->eeprom_length = dsicmd_command_1(dsi, GET_EEPROM_LENGTH);
    }
    if ((offset < 0) || (offset > dsi->eeprom_length))
        return 0xff;
    return dsicmd_command_2(dsi, SET_EEPROM_BYTE, offset | (byte << 8));
}

static int
dsicmd_get_eeprom_data(dsi_camera_t *dsi, char *buffer, int start, int length) {
    int i;
    for (i = 0; i < length; i++) {
        buffer[i] = dsicmd_get_eeprom_byte(dsi, start+i);
    }
    return length;
}

static int
dsicmd_set_eeprom_data(dsi_camera_t *dsi, char *buffer, int start, int length) {
    int i;
    for (i = 0; i < length; i++) {
        dsicmd_set_eeprom_byte(dsi, buffer[i], start+i);
    }
    return length;
}

static int
dsicmd_get_eeprom_string(dsi_camera_t *dsi, char *buffer, int start, int length) {
    int i;
    dsicmd_get_eeprom_data(dsi, buffer, start, length);
    if ((buffer[0] == 0xff) || (buffer[1] == 0xff) || (buffer[2] == 0xff)) {
        strncpy(buffer, "None", length);
    } else {
        strncpy(buffer, buffer+1, length-1);
    }
    for (i = 0; i < length; i++) {
        if ((unsigned char) buffer[i] == 0xff) {
            buffer[i] = 0;
            break;
        }
    }
}

/**
 * Write the provided string to a region in the EEPROM.  WARNING: I think it
 * may be possible to brick your camera with this one which is why I've made
 * it static.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param buffer
 * @param start
 * @param length
 *
 * @return
 */
static int
dsicmd_set_eeprom_string(dsi_camera_t *dsi, char *buffer, int start, int length) {
    int i, j, n;
    char *scratch;
    /* The buffer is assumed to be a normal null-terminated C-string and has
       to be encoded for storage in the EEPROM.  EEPROM strings have their
       length as the first byte and are terminated (padded?) with 0xff. */
    scratch = malloc(length * sizeof(char));
    memset(scratch, 0xff, length);
    n = strlen(buffer);
    if (n > length-2) {
        n = length - 2;
    }
    scratch[0] = n;
    for (i = 0; i < n; i++) {
        scratch[i+1] = buffer[i];
    }
    dsicmd_set_eeprom_data(dsi, scratch, start, length);
    free(scratch);
}

const char *
dsi_get_chip_name(dsi_camera_t *dsi) {
    if (dsi->chip_name[0] == 0) {
        memset(dsi->chip_name, 0, 21);
        dsicmd_get_eeprom_string(dsi, dsi->chip_name, 8, 20);
    }
    return dsi->chip_name;
}

const char *
dsi_get_model_name(dsi_camera_t *dsi) {
    if (dsi->model_name[0] == 0) {
        memset(dsi->chip_name, 0, 21);
        dsi_get_chip_name(dsi);
        if (!strncmp(dsi->chip_name, "ICX254AL", 32)) {
            strncpy(dsi->model_name, "DSI Pro", 32);
        } else if (!strncmp(dsi->chip_name, "ICX429ALL", 32)) {
            strncpy(dsi->model_name, "DSI Pro II", 32);
        } else if (!strncmp(dsi->chip_name, "ICX429AKL", 32)) {
            strncpy(dsi->model_name, "DSI Color II", 32);
        } else if (!strncmp(dsi->chip_name, "ICX404AK", 32)) {
            strncpy(dsi->model_name, "DSI Color", 32);
        } else if (!strncmp(dsi->chip_name, "ICX285AL", 32)) {
           strncpy(dsi->model_name, "DSI Pro III", 32);
        } else {
           strncpy(dsi->model_name, "DSI Unknown", 32);
        }
    }
    return dsi->model_name;
}


const char *
dsi_get_camera_name(dsi_camera_t *dsi) {
    if (dsi->camera_name[0] == 0) {
        memset(dsi->camera_name, 0, 0x21);
        dsicmd_get_eeprom_string(dsi, dsi->camera_name, 0x1c, 0x20);
    }
    return dsi->camera_name;
}

/**
 * Store a name for the DSI camera in its EEPROM for future reference.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param name
 *
 * @return
 */
const char *
dsi_set_camera_name(dsi_camera_t *dsi, const char *name) {
    if (dsi->camera_name[0] == 0) {
        memset(dsi->camera_name, 0, 0x21);
    }
    strncpy(dsi->camera_name, name, 0x20);
    dsicmd_set_eeprom_string(dsi, dsi->camera_name, 0x1c, 0x20);
    return dsi->camera_name;
}

const char *
dsi_get_serial_number(dsi_camera_t *dsi) {
    if (dsi->serial_number[0] == 0) {
        int i;
        char temp[10];
        dsicmd_get_eeprom_data(dsi, temp, 0, 8);
        for (i = 0; i < 8; i++) {
            sprintf(dsi->serial_number+2*i, "%02x", temp[i]);
        }
    }
    return dsi->serial_number;
}

int
dsicmd_get_version(dsi_camera_t *dsi) {
    if (dsi->version.value == -1) {
        unsigned int value = dsicmd_command_1(dsi, GET_VERSION);
        dsi->version.c.family   = 0xff & (value);
        dsi->version.c.model    = 0xff & (value >> 0x08);
        dsi->version.c.version  = 0xff & (value >> 0x10);
        dsi->version.c.revision = 0xff & (value >> 0x18);
        assert(dsi->version.c.family  == 10);
        assert(dsi->version.c.model   == 1);
        assert(dsi->version.c.version == 1);
    }
    return dsi->version.value;
}

static int
dsicmd_load_status(dsi_camera_t *dsi) {
    if ((dsi->usb_speed == -1) || (dsi->fw_debug == -1)) {
        int result = dsicmd_command_1(dsi, GET_STATUS);
        int usb_speed = (result & 0x0ff);
        int fw_debug  = ((result << 8) & 0x0ff);

        assert((usb_speed == DSI_USB_SPEED_FULL) || (usb_speed == DSI_USB_SPEED_HIGH));
        dsi->usb_speed = usb_speed;

        /* XXX: I suppose there is logically a DSI_FW_DEBUG_ON, but I don't know how
         * to turn it on, nor what I would do if I turned it on. */
        assert((fw_debug == DSI_FW_DEBUG_OFF));
        dsi->fw_debug = fw_debug;
    }
}

int
dsicmd_get_usb_speed(dsi_camera_t *dsi) {
    dsicmd_load_status(dsi);
    return dsi->usb_speed;
}

int
dsicmd_get_firmware_debug(dsi_camera_t *dsi) {
    dsicmd_load_status(dsi);
    return dsi->fw_debug;
}

/**
 * Initialize some internal parameters, wake-up the camera, and then query it
 * for some descriptive/identifying information.
 *
 * The DSI cameras all present the same USB vendor and device identifiers both
 * before and after renumeration, so it is not possible to determine what
 * camera model is attached to the bus until after you have connected and
 * (at least partially) intialized the device.
 *
 * The sequence here is mostly a combination of the sequence from the USB
 * traces of what Envisage and MaximDL 4.x do when connecting to the camera.
 * The only commands asking the camera to do something are the wakeup/reset
 * commands; everything else is querying the camera to find out what it is.
 *
 * The original DSI Pro identifies itself (dsi_get_camera_name) as "DSI1".
 * So it was somewhat surprising to find that the DSI Pro II identifies itself
 * as "Guider".  The most reliable way of identifying the camera seems to be
 * via the chip name.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 *
 * @return
 */
static dsi_camera_t *
dsicmd_init_dsi(dsi_camera_t *dsi) {
    dsi->command_sequence_number = 0;
    dsi->eeprom_length    = -1;
    dsi->log_commands     = verbose_init;
    dsi->test_pattern     = 0;
    dsi->exposure_time    = 10;

    dsi->version.value = -1;
    dsi->fw_debug  = -1;
    dsi->usb_speed = -1;

    if (!dsi->is_simulation) {
        dsicmd_command_1(dsi, PING);
        dsicmd_command_1(dsi, RESET);

        dsicmd_get_version(dsi);
        dsicmd_load_status(dsi);

        dsicmd_command_1(dsi, GET_READOUT_MODE);
    }
    dsi_get_chip_name(dsi);
    dsi_get_camera_name(dsi);
    // dsi_get_serial_number(dsi);

    // dsi->read_height_even = dsicmd_get_row_count_even(dsi);
    // dsi->read_height_odd  = dsicmd_get_row_count_odd(dsi);

    /* You would think these could be found by asking the camera, but I can't
       find an example of it happening. */
    if (strcmp(dsi->chip_name, "ICX254AL") == 0) {
        /* DSI Pro I.
         * Sony reports the following information:
         * Effective pixels: 510 x 492
         * Total pixels: 537 x 505
         * Optical black: Horizontal, front 2, rear 25
         *                Vertical, front 12, rear 1
         * Dummy bits: horizontal 16
         *             vertical 1 (even rows only)
         */

        /* Okay, there is some interesting inconsistencies here.  MaximDL
         * takes my DSI Pro I and spits out an image that is 508x489.
         * Envisage spits out an image that is 780x586.  If I ask the camera
         * firmware how many odd/even rows there are, they match the Sony
         * specs.  If I ask MaximDL to square the pixels, I don't grow any new
         * height, and the width doesn't scale out to 780.  I'm going with
         * MaximDL even though it doesn't quite match the Sony specs (why not?
         * What are the dummy bits?) */

        dsi->read_width       = 537;
        dsi->read_height_even = 253;
        dsi->read_height_odd  = 252;

        dsi->image_width      = 508;
        dsi->image_height     = 489;
        dsi->image_offset_x   = 23;
        dsi->image_offset_y   = 13;

        dsi->is_binnable      = 0;
        dsi->is_color         = 0;

    } else if (strcmp(dsi->chip_name, "ICX404AK") == 0) {
        /* DSI Color I.
         * Sony reports the following information:
         * Effective pixels: 510 x 492
         * Total pixels:     537 x 505
         * Optical black: Horizontal, front  2, rear 25
         *                Vertical,   front 12, rear  1
         * Dummy bits: horizontal 16
         *             vertical 1 (even rows only)
         */
        dsi->read_width       = 537;
        dsi->read_height_even = 253;
        dsi->read_height_odd  = 252;

        dsi->image_width      = 508;
        dsi->image_height     = 489;
        dsi->image_offset_x   = 23;
        dsi->image_offset_y   = 17;
        dsi->is_binnable      = 0;
        dsi->is_color         = 1;

    } else if (strncmp(dsi->chip_name, "ICX429", 6) == 0) {
        /* DSI Pro/Color II.
         * Sony reports the following information:
         * Effective pixels: 752 x 582
         * Total pixels:     795 x 596
         * Optical black: Horizontal, front  3, rear 40
         *                Vertical,   front 12, rear  2
         * Dummy bits: horizontal 22
         *             vertical 1 (even rows only)
         */

        dsi->read_width       = 795;
        dsi->read_height_even = 299;
        dsi->read_height_odd  = 298;

        dsi->image_width      = 748;
        dsi->image_height     = 577;
        dsi->image_offset_x   = 30;     /* In bytes, not pixels */
        dsi->image_offset_y   = 13;     /* In rows. */

        if (strcmp(dsi->chip_name, "ICX429AKL") == 0)
            dsi->is_color = 1;
        else /* ICX429ALL */
            dsi->is_color = 0;

        /* FIXME: Don't know if these are B&W specific or not. */
        dsicmd_command_2(dsi, SET_ROW_COUNT_EVEN, dsi->read_height_even);
        dsicmd_command_2(dsi, SET_ROW_COUNT_ODD,  dsi->read_height_odd);
        dsicmd_command_2(dsi, AD_WRITE,  88);
        dsicmd_command_2(dsi, AD_WRITE, 704);

    } else {
        /* Die, camera not supported. */
        fprintf(stderr, "Camera %s not supported", dsi->chip_name);
        abort();
    }

    dsi->read_bpp         = 2;
    dsi->read_height      = dsi->read_height_even + dsi->read_height_odd;
    dsi->read_width       = ((dsi->read_bpp * dsi->read_width / 512) + 1) * 256;

    dsi->read_size_odd    = dsi->read_bpp * dsi->read_width * dsi->read_height_odd;
    dsi->read_size_even   = dsi->read_bpp * dsi->read_width * dsi->read_height_even;

    dsi->read_buffer_odd  = malloc(dsi->read_size_odd);
    dsi->read_buffer_even = malloc(dsi->read_size_even);

    dsi->image_buffer_size = 0;
    dsi->image_buffer      = 0;

    dsi->read_command_timeout  = 1000;    /* milliseconds */
    dsi->write_command_timeout = 1000;    /* milliseconds */
    dsi->read_image_timeout   =  5000;    /* milliseconds */

    dsi->amp_gain_pct   = 100;
    dsi->amp_offset_pct =  50;

    dsi->imaging_state = DSI_IMAGE_IDLE;

    return dsi;
}

/**
 * Do the libusb part of initializing the DSI device.
 *
 * This initialization is based on USB trace logs, plus some trial-and-error.
 * The USB trace logs clearly show the first three usb_get_decriptor commands
 * and the usb_set_configuration command.  The usb_claim_interface is part of
 * the libusb mechanism for locking access to the device, and is not seen in
 * the traces (where were in Windows), but is necessary.
 *
 * The next three usb_clear_halt commands were added after some
 * trial-and-error trying to eliminate a problem with the reference
 * implementation of the DSI control program hanging after a successful
 * connect, image, disconnect sequence.  I don't know why they are necessary,
 * and, in fact, not all three seem to be necessary, but if none of the
 * endpoints are cleared, the device WILL hang.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 *
 * @return
 */
static int
dsicmd_init_usb_device(dsi_camera_t *dsi) {
    const size_t size = 0x800;
    unsigned char data[size];

    /* This is monkey code.  SniffUSB shows that the Meade driver is doing
     * this, but I can think of no reason why.  It does the equivalent of the
     * following sequence
     *
     *    - usb_get_descriptor 1
     *    - usb_get_descriptor 1
     *    - usb_get_descriptor 2
     *    - usb_set_configuration 1
     *    - get the serial number
     *    - get the chip name
     *    - ping the device
     *    - reset the device
     *    - load the firmware information
     *    - load the bus speed status
     *
     * libusb says I'm supposed to claim the interface before doing anything to
     * the device.  I'm not clear what "anything" includes, but I can't do it
     * before the usb_set_configuration command or else I get EBUSY.  So I
     * stick it in the middle of the above sequence at what appears to be the
     * first workable point.
     */

    /* According the the libusb 1.0 documentation (but remember this is libusb
     * 0.1 code), the "safe" way to set the configuration on a device is to
     *
     *  1. Query the configuration.
     *  2. If it is not the desired on, set the desired configuration.
     *  3. Claim the interface.
     *  4. Check the configuration to make sure it is what you selected.  If
     *     not, it means someone else got it.
     *
     * However, that does not seem to be what the USB trace is showing from
     * the Windows driver.  It shows the sequence below (sans the claim
     * interface call, but I'm not sure that actually sends data over the
     * wire).
     *
     */
    assert(usb_get_descriptor(dsi->handle, 0x01, 0x00, data, size) >= 0);
    assert(usb_get_descriptor(dsi->handle, 0x01, 0x00, data, size) >= 0);
    assert(usb_get_descriptor(dsi->handle, 0x02, 0x00, data, size) >= 0);
    assert(usb_set_configuration(dsi->handle, 1) >= 0);
    assert(usb_claim_interface(dsi->handle, 0) >= 0);

    /* This is included out of desperation, but it works :-|
     *
     * After running once, an attempt to run a second time appears, for some
     * unknown reason, to leave us unable to read from EP 0x81.  At the very
     * least, we need to clear this EP.  However, believing in the power of
     * magic, we clear them all.
     */
    assert(usb_clear_halt(dsi->handle, 0x01) >= 0);
    assert(usb_clear_halt(dsi->handle, 0x81) >= 0);
    assert(usb_clear_halt(dsi->handle, 0x86) >= 0);

    assert(usb_clear_halt(dsi->handle, 0x02) >= 0);
    assert(usb_clear_halt(dsi->handle, 0x04) >= 0);
    assert(usb_clear_halt(dsi->handle, 0x88) >= 0);
}

/**
 * Open a DSI camera using the named device, or the first DSI device found if
 * the name is null.
 *
 * @param name a name of the form "usb:BUSNO:DEVNO" as in "usb:5:12".
 *
 * @return a dsi_camera_t handle which should be used for subsequent calls to
 * control the camera.
 */
dsi_camera_t *
dsi_open(const char *name) {
    struct usb_bus *bus;
    struct usb_device *dev;
    struct usb_dev_handle *handle = 0;
    dsi_camera_t *dsi = 0;

    char bus_name[4], dev_name[4];
    int bus_num, dev_num;
    int retcode;

    /* If a name was supplied, parse it so we can try to match against DSI
     * devices we find on the USB bus(es). */
    if (name != 0) {
        int count;
        count = sscanf(name, "usb:%d,%d", &bus_num, &dev_num);
        if (count == 0)
            return 0;

        sprintf(bus_name, "%03d", bus_num);
        sprintf(dev_name, "%03d", dev_num);
    }

    usb_init();
    usb_find_busses();
    usb_find_devices();

    /* All DSI devices appear to present as the same USB vendor:device values.
     * There does not seem to be any better way to find the device other than
     * to iterate over and find the match.  Fortunately, this is fast. */
    for (bus = usb_get_busses(); bus != 0; bus = bus->next) {
        if ((name != 0) && (!strcmp(bus->dirname, bus_name) == 0)) {
            continue;
	}
        for (dev = bus->devices; dev != 0; dev = dev->next) {
            if ((name != 0) && (!strcmp(dev->filename, dev_name) == 0))
                continue;
            if ((  (dev->descriptor.idVendor = 0x156c))
                && (dev->descriptor.idProduct == 0x0101)) {
                if (verbose_init = 0)
                    fprintf(stderr, "Found device %04x:%04x at usb:%s,%s\n",
                            dev->descriptor.idVendor, dev->descriptor.idProduct,
                            bus->dirname, dev->filename);
                handle = usb_open(dev);
            }
            break;
        }
        if (handle != 0)
            break;
    }

    if (handle == 0)
        return 0;

    dsi = calloc(1, sizeof(dsi_camera_t));
    assert(dsi != 0);

    dsi->device = dev;
    dsi->handle = handle;
    dsi->is_simulation = 0;

    dsicmd_init_usb_device(dsi);
    dsicmd_init_dsi(dsi);

    dsi_start_image(dsi, 0.1);
    dsi_read_image(dsi, 0, 0);
    dsi_start_image(dsi, 0.1);
    dsi_read_image(dsi, 0, 0);
    dsi_start_image(dsi, 0.1);
    dsi_read_image(dsi, 0, 0);

    return dsi;
}

void
dsi_close(dsi_camera_t *dsi) {
    assert(usb_release_interface(dsi->handle, 0) >= 0);
    assert(usb_close(dsi->handle) >= 0);
}


/**
 * Set the verbose logging state for the library during camera
 * open/intialization.  This works the same as dsi_set_verbose, but you can't
 * call dsi_set_verbose until after you have opened a connection to the
 * camera, so this fills that gap.
 *
 * @param on turn on logging if logically true.
 */
void
libdsi_set_verbose_init(int on) {
    verbose_init = on;
}

/**
 * Return verbose logging state for camera intialization.
 *
 * @return 0 if logging is off, non-zero for logging on.
 */
int
libdsi_get_verbose_init() {
    return verbose_init;
}

/**
 * Turn on or off verbose logging state for low-level camera commands.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param on turn on logging if logically true.
 */
void
dsi_set_verbose(dsi_camera_t *dsi, int on) {
    dsi->log_commands = on;
}

/**
 * Get verbose logging state for low-level camera commands.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 *
 * @return 0 if logging is off, non-zero for logging on.
 */
int
dsi_get_verbose(dsi_camera_t *dsi) {
    return dsi->log_commands;
}

int
dsi_start_image(dsi_camera_t *dsi, double exptime) {
    int gain, offset;
    int exposure_ticks = 10000 * exptime;

    gain = 63 * dsi->amp_gain_pct / 100;

    /* FIXME: What is the mapping?
     *     20% -> 409 -> 0x199
     *     50% ->   0
     *     80% -> 153 -> 0x099
     * So, this looks like a 8-bit value with a sign bit.  Then 80% is
     * (80-50)/50*255 = 153, and 20% is the same thing, but with the high bit
     * set.
     */

    if (dsi->amp_offset_pct < 50) {
        offset = 50 - dsi->amp_offset_pct;
        offset = 255 * offset / 50;
        offset |= 0x100;
    } else {
        offset = dsi->amp_offset_pct - 50;
        offset = 255 * offset / 50;
    }

    dsicmd_set_exposure_time(dsi, exposure_ticks);
    if (exposure_ticks < 10000) {
        dsicmd_set_readout_speed(dsi, DSI_READOUT_SPEED_HIGH);
        dsicmd_set_readout_delay(dsi, 3);
        dsicmd_set_readout_mode(dsi, DSI_READOUT_MODE_DUAL);
        dsicmd_get_readout_mode(dsi);
        dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_ON);
    } else {
        dsicmd_set_readout_speed(dsi, DSI_READOUT_SPEED_LOW);
        dsicmd_set_readout_delay(dsi, 5);
        dsicmd_set_readout_mode(dsi, DSI_READOUT_MODE_SINGLE);
        dsicmd_get_readout_mode(dsi);
        dsicmd_set_vdd_mode(dsi, DSI_VDD_MODE_AUTO);
    }
    dsicmd_set_gain(dsi, 63*dsi->amp_gain_pct/100);
    dsicmd_set_offset(dsi, offset);
    dsicmd_set_flush_mode(dsi, DSI_FLUSH_MODE_CONT);
    dsicmd_get_readout_mode(dsi);
    dsicmd_get_exposure_time(dsi);

    dsicmd_start_exposure(dsi);
    dsi->imaging_state = DSI_IMAGE_EXPOSING;
    return 0;
}
/**
 * Read an image from the DSI camera.
 *
 * @param dsi Pointer to an open dsi_camera_t holding state information.
 * @param buffer pointer to a buffer pointer.  If *buffer == 0, dsi_read_image
 * will allocate the buffer.
 * @param flags set to O_NONBLOCK for asynchronous read.
 *
 * @return 0 on success, non-zero if the image was not read.
 *
 * If the dsi or buffer pointers are invalid, returns EINVAL.  If the camera
 * is not currently exposing, returns ENOTSUP.  If an I/O error occurs,
 * returns EIO.  If the image is not ready and O_NONBLOCK was specified,
 * returns EWOULDBLOCK.
 */
int
dsi_read_image(dsi_camera_t *dsi, unsigned int **buffer, int flags) {
    int status;
    int ticks_left, read_size_odd, read_size_even;

    if (dsi == 0 || buffer == 0)
        return EINVAL;

    /* FIXME: This method should really only be callable if the imager is in a
       currently imaging state. */

    if (dsi->imaging_state != DSI_IMAGE_EXPOSING)
        return ENOTSUP;

    if (dsi->exposure_time > 10000) {
        if (dsi->log_commands)
            fprintf(stderr, "long exposure, checking remaining time\n");
        /* These are in different units, so this really says "if the time left is
           greater than 1/10 of the image read timeout time, wait."  ticks_left is
           in units of 1/10 millisecond whle read_image_timeout is in units of
           milliseconds. */
        ticks_left = dsicmd_get_exposure_time_left(dsi);
        /*    if (ticks_left < 0) {
              fprintf(stderr, "ticks left < 0: %d\n", ticks_left);
              return ticks_left;
              }
        */

        while (ticks_left > dsi->read_image_timeout) {
            if (dsi->log_commands)
                fprintf(stderr, "long exposure, %d ticks remaining exceeds threshold of %d\n",
                        ticks_left, dsi->read_image_timeout);
            /* FIXME: There are other possible error codes which are just
               status codes from underlying calls and not true errors.  We
               need to fix this so that there is no possibility of overlap. */
            if ((flags & O_NONBLOCK) != 0) {
                if (dsi->log_commands)
                    fprintf(stderr, "non-blocking requested, returning now\n");
                return EWOULDBLOCK;
            }
            // usleep(100 * (ticks_left - dsi->read_image_timeout) + 100);
            if (dsi->log_commands)
                fprintf(stderr, "sleeping 1.005 sec\n");
            usleep(1005000);
            ticks_left = dsicmd_get_exposure_time_left(dsi);
        }
        /*    if (ticks_left < 0) {
              fprintf(stderr, "ticks left < 0: %d\n", ticks_left);
              return ticks_left;
              }
        */
    }

    read_size_even = dsi->read_bpp * dsi->read_width * dsi->read_height_even;
    status = usb_bulk_read(dsi->handle, 0x86, dsi->read_buffer_even, read_size_even,
                           3 * dsi->read_image_timeout);
    if (dsi->log_commands)
        dsi_log_command_info(dsi, 1, "r 86", read_size_even, dsi->read_buffer_even, 0);
    if (status < 0) {
        fprintf(stderr, "usb_bulk_read(%p, 0x86, %p, %d, %d) (even) -> returned %d\n",
                dsi->handle, dsi->read_buffer_even, read_size_even, 2*dsi->read_image_timeout, status);
        dsi->imaging_state = DSI_IMAGE_IDLE;
        return EIO;
    }

    read_size_odd = dsi->read_bpp * dsi->read_width * dsi->read_height_odd;
    status = usb_bulk_read(dsi->handle, 0x86, dsi->read_buffer_odd, read_size_odd,
                           3 * dsi->read_image_timeout);
    if (dsi->log_commands)
        dsi_log_command_info(dsi, 1, "r 86", read_size_odd, dsi->read_buffer_odd, 0);
    if (status < 0) {
        fprintf(stderr, "usb_bulk_read(%p, 0x86, %p, %d, %d) (odd) -> returned %d\n",
                dsi->handle, dsi->read_buffer_odd, read_size_odd, 2*dsi->read_image_timeout, status);
        dsi->imaging_state = DSI_IMAGE_IDLE;
        return EIO;
    }

    dsi->imaging_state = DSI_IMAGE_IDLE;
    // fprintf(stderr, "image decode started\n");
    dsicmd_decode_image(dsi);
    // fprintf(stderr, "image decode completed\n");
    if (buffer)
        *buffer = dsi->image_buffer;

    if (buffer)
        *buffer = dsi->image_buffer;

    return 0;
}

/**
 * Decode the internal image buffer from an already read image.
 */
const unsigned int *
dsicmd_decode_image(dsi_camera_t *dsi) {

    int xpix, ypix, outpos;
    int is_odd_row, row_start;

    /* FIXME: This method should really only be called if the camera is an
       post-imaging state. */

    if (dsi->image_buffer == 0) {
        dsi->image_buffer_size = dsi->read_width * dsi->read_height;
        dsi->image_buffer = (unsigned int *) calloc(sizeof(unsigned int), dsi->image_buffer_size);
    } else if (dsi->image_buffer_size < dsi->read_width * dsi->read_height) {
        dsi->image_buffer_size = dsi->read_width * dsi->read_height;
        dsi->image_buffer = (unsigned int *) realloc(dsi->image_buffer,
                                                     sizeof(unsigned int) * dsi->image_buffer_size);
    }
    memset(dsi->image_buffer, 0, sizeof(unsigned int) * dsi->image_buffer_size);

    outpos = 0;
    for (ypix = 0; ypix < dsi->image_height; ypix++) {

        unsigned int msb, lsb;
        int ixypos;
        /* The odd-even interlacing means that we advance the row start offset
           every other pass through the loop.  It is the same offset on each
           of those two passes, but we read from a different buffer. */
        is_odd_row = (ypix + dsi->image_offset_y) % 2;
        row_start  = dsi->read_width * ((ypix + dsi->image_offset_y) / 2);

        ixypos = 2 * (row_start + dsi->image_offset_x);
        /*
          fprintf(stderr, "starting image row %d, outpos=%d, is_odd_row=%d, row_start=%d, ixypos=%d\n",
          ypix, outpos, is_odd_row, row_start, ixypos);
        */
        for (xpix = 0; xpix < dsi->image_width; xpix++) {
            if (is_odd_row) {
                msb = dsi->read_buffer_odd[ixypos];
                lsb = dsi->read_buffer_odd[ixypos+1];
            } else {
                msb = dsi->read_buffer_even[ixypos];
                lsb = dsi->read_buffer_even[ixypos+1];
            }
            dsi->image_buffer[outpos++] = (msb << 8) | lsb;
            ixypos += 2;
        }
    }
    return dsi->image_buffer;
}

/**
 * Create a simulated DSI camera intialized to behave like the named camera chip.
 *
 * @param chip_name
 *
 * @return pointer to simulated DSI camera.
 */
dsi_camera_t *
dsitst_open(const char *chip_name) {
    dsi_camera_t *dsi = calloc(1, sizeof(dsi_camera_t));

    dsi->is_simulation = 1;

    dsi->command_sequence_number = 0;
    dsi->eeprom_length    = -1;
    dsi->log_commands     = verbose_init;
    dsi->test_pattern     = 0;
    dsi->exposure_time    = 10;

    dsi->version.c.family   = 10;
    dsi->version.c.model    =  1;
    dsi->version.c.version  =  1;
    dsi->version.c.revision =  0;
    dsi->fw_debug  = DSI_FW_DEBUG_OFF;
    dsi->usb_speed = DSI_USB_SPEED_HIGH;

    strncpy(dsi->chip_name, chip_name, 32);
    strncpy(dsi->serial_number, "0123456789abcdef", 32);

    dsicmd_init_dsi(dsi);

    /* Okay, this was learned the hard way.  The SniffUSB logs clearly showed
       that the actual read size is calculated by rounding the size of EACH
       ROW up to some multiple of 512 bytes.  The basic read size of the USB
       endpoint is a multiple of 512 bytes, so this kind of makes sense as the
       easiest implementation in firmware---just pad to 512 bytes for each
       row, then it won't matter how many rows you stick in.  And the C++
       driver was reading correctly, but the first implementation here was
       rounding later, at the time of the read.  That results in core dumps
       since have to size the buffers correctly.  Remember, each row must be a
       multiple of 512 bytes. */

    dsi->read_bpp         = 2;
    dsi->read_height      = dsi->read_height_even + dsi->read_height_odd;
    dsi->read_width       = ((dsi->read_bpp * dsi->read_width / 512) + 1) * 256;

    fprintf(stderr, "read_size_odd  => %ld (0x%lx)\n", dsi->read_size_odd, dsi->read_size_odd);
    fprintf(stderr, "read_size_even => %ld (0x%lx)\n", dsi->read_size_even, dsi->read_size_even);
    fprintf(stderr, "read_size_bpp  => %d (0x%x)\n", dsi->read_bpp, dsi->read_bpp);

    return dsi;
}

/**
 * Read DSI image data from FILENAME into internal buffers.
 *
 * @param dsi dsi_camera_t camera struct into which the data will be loaded.
 * @param filename read from this file.  Data is assumed to be in raw format
 * @param is_binary true if the file is raw binary data to load into the read
 * buffers.  If false, the data is assumed to be in SniffUSB/USBsnoop format.
 *
 * This is a test routine to allow injecting data into the internal image
 * buffers for post-acquisition testing.
 *
 * The data to be read should be either from the same type of camera in use
 * for the test, or have been carefully constructed to be consistent with that
 * form.  If not, then the results are undefined (read, expect crashes or
 * crap).
 *
 * The raw binary data should be the same size as the read buffers that would
 * come from reading the camera.
 *
 * SniffUSB/USBsnoop data is of the form
 *
 *    00000000: 13 45 13 49 13 4e 12 ac 49 b3 4d f2 52 40 56 67
 *    00000010: 5a 46 5e 31 62 3b 65 98 69 29 6c bb 6f b2 72 9d
 *    ...
 *
 * Note that this will read the FIRST image found in the file which means it
 * won't work :-)  The problem is that both Envisage and MaximDL seem to
 * take several short throw-away images as part of the intialization.  I don't
 * know why and they don't seem to be necessary, so we don't do it.  But if
 * you use a SniffUSB dump from Envisage or MaximDL, and it includes the
 * initialization, the first apparent image in the dump is not the image you
 * get on the screen.  The point is, you want your test data to include ONLY
 * the test image you are going to compare.
 *
 * @return
 */
int
dsitst_read_image(dsi_camera_t *dsi, const char *filename, int is_binary) {
    FILE *fptr = 0;
    int status = 0;

    /* state = 0, looking for first match.
     * state = 1, reading first buffer.
     * state = 2, looking for second match.
     * state = 3, reading second buffer.
     * state = 4, done.
     */
    int state = 0;

    if (is_binary) {
        /* Oops, don't really support this yet. */
        status = -1;
        goto OOPS;

    } else {
        char line[1000];
        regex_t preg;
        regmatch_t pmatch[32];
        const char *regex = " *([0-9a-f]{8}):( [0-9a-f]{2}){16}";
        int status;
        unsigned char *write_buffer;
        size_t buffer_size;

        status = regcomp(&preg, regex, REG_EXTENDED|REG_ICASE);
        assert(status == 0);

        fptr = fopen(filename, "r");
        assert(fptr != 0);

        fgets(line, 1000, fptr);
        while (!feof(fptr)) {

            if (regexec(&preg, line, 32, pmatch, 0) == 0) {
                if (state == 0) {
                    state = 1;
                    write_buffer = dsi->read_buffer_even;
                    buffer_size  = dsi->read_size_even;
                }

                if (state == 2) {
                    state = 3;
                    write_buffer = dsi->read_buffer_odd;
                    buffer_size  = dsi->read_size_odd;
                }

                if (state == 1 || state == 3) {
                    unsigned char *wptr;
                    int offset;
                    int v[16], i;

                    sscanf(line, "%08x:", &offset);
                    assert(offset + 16 <= buffer_size);

                    assert(write_buffer != 0);
                    wptr = write_buffer + offset;
                    sscanf(line + pmatch[1].rm_eo + 1,
                           "%02x %02x %02x %02x %02x %02x %02x %02x "
                           "%02x %02x %02x %02x %02x %02x %02x %02x",
                           v+0x0, v+0x1, v+0x2, v+0x3, v+0x4, v+0x5, v+0x6, v+0x7,
                           v+0x8, v+0x9, v+0xa, v+0xb, v+0xc, v+0xd, v+0xe, v+0xf);
                    for (i = 0; i < 16; i++) {
                        wptr[i] = v[i];
                    }
                }

            } else if (state == 1 || state == 3) {
                state++;
                write_buffer = 0;
            }

            if (state > 3)
                break;

            fgets(line, 1000, fptr);
        }
        regfree(&preg);
    }

 OOPS:
    if (fptr != 0) {
        fclose(fptr);
    }

    return 0;
}
