#ifndef __SDSERVER_H__
#define __SDSERVER_H__

#include <stdint.h>
#include <arpa/inet.h>


#define SD_PORT 7283
#define SD_MAX_CONNECTIONS 20
#define SD_PROMPT "cmd> "

#define NET_MAX_TRIES 20

struct sdserver;

enum sdserver_parse_mode {
    PARSE_MODE_BINARY,
    PARSE_MODE_LINE,
};

struct sdserver_syscmd {
    const uint8_t cmd[2];
    const uint32_t flags;
    const char *description;
    int(*handle_cmd)(struct sdserver *server, int arg);
};

/* Command as it travels over the wire in binary mode */
struct sdserver_cmd {
    uint8_t cmd[2]; /* -\                   */
                    /*    > Network packet  */
    uint32_t arg;   /* _/                   */


    struct sdserver_syscmd *syscmd;
};


struct sdserver {
    enum sdserver_parse_mode    parse_mode;
    int                         net_socket;
    int                         net_fd;
    struct sockaddr_in          net_sockaddr;
    uint32_t                    net_buf_len;
    uint8_t                     net_bfr[512];
    uint32_t                    net_bfr_ptr;
    struct sdserver_syscmd      *cmds;
};


#define CMD_FLAG_ARG 0x01 /* True if the command has an arg */

int parse_init(struct sdserver *server);
int parse_get_next_command(struct sdserver *server, struct sdserver_cmd *cmd);
int parse_set_mode(struct sdserver *server, enum sdserver_parse_mode mode);
int parse_deinit(struct sdserver *server);
int parse_add_hook(struct sdserver *server, char cmd[2], int
        (*hook)(struct sdserver *, int));

int net_init(struct sdserver *server);
int net_accept(struct sdserver *server);
int net_write(struct sdserver *server, char *txt);
int net_get_packet(struct sdserver *server, uint8_t **data);
int net_deinit(struct sdserver *server);

#endif /* __SDSERVER_H__ */
