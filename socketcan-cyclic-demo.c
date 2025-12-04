/*
The MIT License (MIT)

Copyright (c) 2015 Jacob McGladdery

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

-------------------------------------------------------------------------------

Broadcast Manager Cyclic Demo

This program demonstrates sending a set of cyclic messages out to the CAN bus
using SocketCAN's broadcast manager interface. The intended behavior of this
program is to send four cyclic messages out to the CAN bus. These messages
have IDs ranging from 0x0C0 to 0x0C3. These messages will be sent out one at
a time every 1200 milliseconds. Once all messages have been sent,
transmission will begin again with message 0x0C0.
*/

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <error.h>
#include <getopt.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/can.h>
#include <linux/can/bcm.h>

#define VERSION  "2.0.0"

#define MSGID (0x0C0)
#define MSGLEN (3)
#define NFRAMES (4)

struct args
{
    const char *iface;
};

static void on_signal(int)
{
    /* Do nothing.
     * The only reason this handler exists is to make sure sigsuspend(2) returns.
     */
}

static void init_signals(void)
{
    struct sigaction sa;
    sa.sa_handler = on_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static int init_socket(const char *iface)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    int sfd;
    int rc;

    /* Create a broadcast manager CAN socket */
    sfd = socket(PF_CAN, SOCK_DGRAM, CAN_BCM);
    if (-1 == sfd) {
        error(EXIT_FAILURE, errno, "socket");
    }

    /* Determine the interface index */
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    rc = ioctl(sfd, SIOCGIFINDEX, &ifr);
    if (-1 == rc) {
        error(EXIT_FAILURE, errno, "ioctl");
    }

    /* Set the local address to connect to */
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    /* Connect the socket to the address */
    rc = connect(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if (-1 == rc) {
        error(EXIT_FAILURE, errno, "bind");
    }

    return sfd;
}

static void cleanup(int sfd)
{
    sigset_t mask;
    int rc;

    /* Block signals from interfering with graceful shutdown */
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    /* Close the socket */
    rc = close(sfd);
    if (-1 == rc) {
        error(EXIT_FAILURE, errno, "close");
    }
}

static void print_help(const char *progname)
{
    printf(
        "Usage: %s [OPTIONS] IFACE\n"
        "\n"
        "Arguments:\n"
        "  IFACE    CAN network interface (e.g. can0)\n"
        "\n"
        "Options:\n"
        "  --help, -h       Display this help then exit\n"
        "  --version, -V    Display version info then exit\n",
        progname
    );
}

static void print_version(void)
{
    puts(VERSION);
}

static void parse_args(int argc, char **argv, struct args *args)
{
    const char *progname = program_invocation_short_name;

    static const struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {0, 0, 0, 0}
    };

    for (;;) {
        const int opt = getopt_long(argc, argv, "Vh", long_options, NULL);
        if (opt == -1) {
            break;
        }

        switch (opt) {
        case 'V':
            print_version();
            exit(EXIT_SUCCESS);
        case 'h':
            print_help(progname);
            exit(EXIT_SUCCESS);
        default:
            print_help(progname);
            exit(EXIT_FAILURE);
        }
    }

    if ((argc - optind) != 1) {
        error(0, 0, "exactly one CAN interface argument expected");
        print_help(progname);
        exit(EXIT_FAILURE);
    }

    args->iface = argv[optind];
}

int main(int argc, char **argv)
{
    struct args args;
    sigset_t mask;
    ssize_t n;
    int sfd;
    int i;

    struct can_msg
    {
        struct bcm_msg_head msg_head;
        struct can_frame frames[NFRAMES];
    } msg;

    program_invocation_name = program_invocation_short_name;

    parse_args(argc, argv, &args);
    init_signals();
    sfd = init_socket(args.iface);

    /* Create cyclic transmission task */
    memset(&msg, 0, sizeof(msg));
    msg.msg_head.opcode = TX_SETUP;
    msg.msg_head.can_id = 0;
    msg.msg_head.flags = SETTIMER | STARTTIMER;
    msg.msg_head.nframes = NFRAMES;
    msg.msg_head.count = 0;

    /* Set the time interval for sending messages to 1200ms */
    msg.msg_head.ival2.tv_sec = 1;
    msg.msg_head.ival2.tv_usec = 200000;

    /* Set the example messages */
    for (i = 0; i < NFRAMES; i++) {
        struct can_frame *frame = &msg.frames[i];
        frame->can_id = MSGID + i;
        frame->len = MSGLEN;
        memset(frame->data, i, MSGLEN);
    }

    /* Register the cyclic messages and begin transmitting immediately.
     * Note, all of the messages will be sent with the same periodicity
     * because they share the same bcm_msg_head setup.
     */
    n = write(sfd, &msg, sizeof(msg));
    if (-1 == n) {
        error(EXIT_FAILURE, errno, "write");
    }

    printf(
        "Cyclic messages registed with SocketCAN!\n"
        "Use a tool such as \"candump %s\" to view the messages.\n"
        "These messages will continue to transmit so long as the socket\n"
        "used to communicate with SocketCAN remains open. In other words,\n"
        "close this program with SIGINT or SIGTERM in order to gracefully\n"
        "stop transmitting.\n",
        args.iface
    );

    /* Suspend this thread until SIGINT or SIGTERM is received.
     * The cyclic CAN messages will continue to be transmitted by the kernel.
     */
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGTERM);
    sigsuspend(&mask);

    cleanup(sfd);
    puts("Goodbye!");
    return EXIT_SUCCESS;
}
