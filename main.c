#define DEBUG
#include <stdio.h>
#include <errno.h>
#include <strings.h>

#include "sd.h"

static int set_binmode(struct sd *server, int arg) {
    parse_set_mode(server, PARSE_MODE_BINARY);
    return 0;
}

static int set_linemode(struct sd *server, int arg) {
    parse_set_mode(server, PARSE_MODE_LINE);
    return 0;
}

int main(int argc, char **argv) {
    struct sd server;
    int ret;

    bzero(&server, sizeof(server));

    ret = net_init(&server);
    if (ret < 0) {
        perror("Couldn't initialize network");
        return 1;
    }

    ret = parse_init(&server);
    if (ret < 0) {
        perror("Couldn't initialize parser");
        return 1;
    }

    ret = net_accept(&server);
    if (ret < 0) {
        perror("Couldn't accept network connections");
        return 1;
    }

    parse_set_hook(&server, "bm", set_binmode);
    parse_set_hook(&server, "lm", set_linemode);

    while(1) {
        struct sd_cmd cmd;
        ret = parse_get_next_command(&server, &cmd);
        if (ret < 0) {
            perror("Quitting");
            net_deinit(&server);
            parse_deinit(&server);
            return 1;
        }

#ifdef DEBUG
        fprintf(stderr, "Got command: %c%c - %s", cmd.cmd[0], cmd.cmd[1],
                cmd.syscmd->description);
        if (cmd.syscmd->flags & CMD_FLAG_ARG)
            fprintf(stderr, " - arg: %d", cmd.arg);
        fprintf(stderr, "\n");
#endif

        /* In reality, all commands should have a handle routine */
        if (cmd.syscmd->handle_cmd)
            cmd.syscmd->handle_cmd(&server, cmd.arg);
        else
            fprintf(stderr, "WARNING: Command %c%c missing handle_cmd\n",
                    cmd.cmd[0], cmd.cmd[1]);
    }

    return 0;
}
