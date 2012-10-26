#ifndef __sd_H__
#define __sd_H__

#include <stdint.h>
#include <arpa/inet.h>

#define SD_DEFAULT_BLKLEN 512

#define NET_PORT 7283
#define NET_MAX_CONNECTIONS 20
#define NET_PROMPT "cmd> "
#define NET_MAX_TRIES 20

struct sd;

enum sd_parse_mode {
    PARSE_MODE_BINARY,
    PARSE_MODE_LINE,
};

struct sd_syscmd {
    const uint8_t cmd[2];
    const uint32_t flags;
    const char *description;
    int(*handle_cmd)(struct sd *server, int arg);
};

/* Command as it travels over the wire in binary mode */
struct sd_cmd {
    uint8_t cmd[2]; /* -\                   */
                    /*    > Network packet  */
    uint32_t arg;   /* _/                   */


    struct sd_syscmd *syscmd;
};


struct sd {
    enum sd_parse_mode    parse_mode;

    int                         net_socket;
    int                         net_fd;
    struct sockaddr_in          net_sockaddr;
    uint32_t                    net_buf_len;
    uint8_t                     net_bfr[512];
    uint32_t                    net_bfr_ptr;

    struct sd_syscmd     *cmds;

    /* Raw SD commands */
    uint8_t                     sd_registers[4];
    uint32_t                    sd_blklen;
    uint8_t                    *sd_buffer;

    /* GPIO pins */
    uint32_t                    sd_data_in, sd_data_out;
    uint32_t                    sd_clk, sd_cs, sd_power;
};


#define CMD_FLAG_ARG 0x01 /* True if the command has an arg */

int parse_init(struct sd *server);
int parse_get_next_command(struct sd *server, struct sd_cmd *cmd);
int parse_set_mode(struct sd *server, enum sd_parse_mode mode);
int parse_deinit(struct sd *server);
int parse_set_hook(struct sd *server, char cmd[2], int
        (*hook)(struct sd *, int));

int net_init(struct sd *server);
int net_accept(struct sd *server);
int net_write(struct sd *server, char *txt);
int net_get_packet(struct sd *server, uint8_t **data);
int net_deinit(struct sd *server);

int sd_init(struct sd *sd, uint8_t data_in, uint8_t data_out,
                     uint8_t clk, uint8_t cs, uint8_t power);
int sd_deinit(struct sd *sd);


#endif /* __sd_H__ */
