#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include "sd.h"
#include "gpio.h"


/* Table for CRC-7 (polynomial x^7 + x^3 + 1) */
static const uint8_t crc7_syndrome_table[256] = {
        0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
        0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
        0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
        0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
        0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
        0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
        0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
        0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
        0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
        0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
        0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
        0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
        0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
        0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
        0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
        0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
        0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
        0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
        0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
        0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
        0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
        0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
        0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
        0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
        0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
        0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
        0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
        0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
        0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
        0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
        0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
        0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

static inline uint8_t crc7_byte(uint8_t crc, uint8_t data)
{
        return crc7_syndrome_table[(crc << 1) ^ data];
}

/**
 * crc7 - update the CRC7 for the data buffer
 * @crc:     previous CRC7 value
 * @buffer:  data pointer
 * @len:     number of bytes in the buffer
 * Context: any
 *
 * Returns the updated CRC7 value.
 */
static uint8_t crc7(uint8_t crc, const uint8_t *buffer, size_t len)
{
        while (len--)
                crc = crc7_byte(crc, *buffer++);
        return crc;
}


static int set_r0(struct sd *server, int arg) {
    server->sd_registers[0] = arg;
    return 0;
}

static int set_r1(struct sd *server, int arg) {
    server->sd_registers[1] = arg;
    return 0;
}

static int set_r2(struct sd *server, int arg) {
    server->sd_registers[2] = arg;
    return 0;
}

static int set_r3(struct sd *server, int arg) {
    server->sd_registers[3] = arg;
    return 0;
}

static int clear_regs(struct sd *server, int arg) {
    bzero(server->sd_registers, sizeof(server->sd_registers));
    return 0;
}


static int sd_tick(struct sd *sd) {
    gpio_set_value(sd->sd_clk, 1);
    usleep(2);
    gpio_set_value(sd->sd_clk, 0);
    usleep(2);
    gpio_set_value(sd->sd_clk, 1);
    return 0;
}

static int write_bit(struct sd *sd, int bit) {
    gpio_set_value(sd->sd_data_out, !!bit);
    sd_tick(sd);
    return 0;
}

#define MSB_FIRST
static int xfer_byte(struct sd *sd, uint8_t byte) {
    int bit;
    int out = 0;
    for (bit=0; bit<8; bit++) {
#ifdef MSB_FIRST
        write_bit(sd, (byte>>(7-bit))&1);
        out |= gpio_get_value(sd->sd_data_in)<<(7-bit);
#else
        write_bit(sd, (byte>>bit)&1);
        out |= gpio_get_value(sd->sd_data_in)<<(bit);
#endif
    }
    return out;
}

static int real_send_cmd(struct sd *sd, uint8_t cmd, uint8_t args[4]) {
    uint8_t bytes[7];
    uint8_t byte;
    bytes[0] = 0xff;
    bytes[1] = 0x40 | (cmd&0x3f);
    memcpy(&bytes[2], args, 4);
    bytes[6] = (crc7(0, bytes+1, 5)<<1)|1;

    printf("Sending CMD%d {0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x}\n",
        bytes[1]&0x3f,
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6]);


    for (byte=0; byte<sizeof(bytes); byte++)
        xfer_byte(sd, bytes[byte]);

    return 0;
}

static int send_cmd(struct sd *sd, int arg) {
    return real_send_cmd(sd, arg, sd->sd_registers);
}

int sd_init(struct sd *sd, uint8_t data_in, uint8_t data_out,
                           uint8_t clk, uint8_t cs, uint8_t power) {
    sd->sd_data_in = data_in;
    sd->sd_data_out = data_out;
    sd->sd_clk = clk;
    sd->sd_cs = cs;
    sd->sd_power = power;

    if (gpio_export(sd->sd_data_in)) {
        perror("Unable to export DATA IN pin");
        return -1;
    }
    gpio_set_direction(sd->sd_data_in, 0);


    if (gpio_export(sd->sd_data_out)) {
        perror("Unable to export DATA OUT pin");
        return -1;
    }
    gpio_set_direction(sd->sd_data_out, 1);
    gpio_set_value(sd->sd_data_out, 1);


    if (gpio_export(sd->sd_clk)) {
        perror("Unable to export CLK pin");
        return -1;
    }
    gpio_set_direction(sd->sd_clk, 1);
    gpio_set_value(sd->sd_clk, 1);


    if (gpio_export(sd->sd_cs)) {
        perror("Unable to export CS pin");
        return -1;
    }
    gpio_set_direction(sd->sd_cs, 1);
    gpio_set_value(sd->sd_cs, 1);

    /* Power up the card */
    if (gpio_export(sd->sd_power)) {
        perror("Unable to export power pin");
        return -1;
    }
    gpio_set_direction(sd->sd_power, 1);
    gpio_set_value(sd->sd_power, 1);

    sd->sd_blklen = SD_DEFAULT_BLKLEN;

    parse_set_hook(sd, "r0", set_r0);
    parse_set_hook(sd, "r1", set_r1);
    parse_set_hook(sd, "r2", set_r2);
    parse_set_hook(sd, "r3", set_r3);
    parse_set_hook(sd, "rr", clear_regs);
    parse_set_hook(sd, "cd", send_cmd);

    return 0;
}

int sd_deinit(struct sd *sd) {
    gpio_set_value(sd->sd_cs, 1);
    gpio_set_value(sd->sd_power, 1);

    gpio_unexport(sd->sd_data_in);
    gpio_unexport(sd->sd_data_out);
    gpio_unexport(sd->sd_clk);
    gpio_unexport(sd->sd_cs);
    gpio_unexport(sd->sd_power);

    return 0;
}

