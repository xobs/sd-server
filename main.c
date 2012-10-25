#define DEBUG
#include <stdio.h>
#include <errno.h>

#include "sdserver.h"

static int get_offset(struct sdserver *server, int arg) {
    net_write(server, "Getting offset\n");
    return 0;
}

static int set_binmode(struct sdserver *server, int arg) {
    parse_set_mode(server, PARSE_MODE_BINARY);
    return 0;
}

static int set_linemode(struct sdserver *server, int arg) {
    parse_set_mode(server, PARSE_MODE_LINE);
    return 0;
}

static int set_r0(struct sdserver *server, int arg) {
    server->registers[0] = arg;
    return 0;
}

static int set_r1(struct sdserver *server, int arg) {
    server->registers[1] = arg;
    return 0;
}

static int set_r2(struct sdserver *server, int arg) {
    server->registers[2] = arg;
    return 0;
}

static int set_r3(struct sdserver *server, int arg) {
    server->registers[3] = arg;
    return 0;
}

int main(int argc, char **argv) {
    struct sdserver server;
    int ret;

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

    parse_set_hook(&server, "go", get_offset);
    parse_set_hook(&server, "bm", set_binmode);
    parse_set_hook(&server, "lm", set_linemode);

    parse_set_hook(&server, "r0", set_r0);
    parse_set_hook(&server, "r1", set_r1);
    parse_set_hook(&server, "r2", set_r2);
    parse_set_hook(&server, "r3", set_r3);

    while(1) {
        struct sdserver_cmd cmd;
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
