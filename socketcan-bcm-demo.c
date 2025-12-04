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

Broadcast Manager Interface Demo

This program demonstrates reading and writing to a CAN bus using SocketCAN's
broadcast manager interface. The intended behavior of this program is to read
in CAN messages which have an ID of 0x123, add one to the value of each data
byte in the received message, and then write that message back out to the bus
with the message ID defined by the macro MSGID.
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

#define VERSION "2.0.0"

#define MSGID (0x0BC)

struct args
{
    const char *iface;
};

static volatile sig_atomic_t run = 1;

static void on_signal(int)
{
    run = 0;
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

static void print_can_frame(const struct can_frame *const frame)
{
    const unsigned char *data = frame->data;
    const unsigned char len = frame->len;
    unsigned char i;

    printf("%03X  [%u] ", frame->can_id, len);
    for (i = 0; i < len; i++) {
        printf(" %02X", data[i]);
    }
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
    ssize_t n;
    int sfd;

    struct can_msg
    {
        struct bcm_msg_head msg_head;
        struct can_frame frames[1];
    } msg;

    program_invocation_name = program_invocation_short_name;

    parse_args(argc, argv, &args);
    init_signals();
    sfd = init_socket(args.iface);

    /* Create RX filter subscription for messages with ID 0x123 */
    memset(&msg, 0, sizeof(msg));
    msg.msg_head.opcode = RX_SETUP;
    msg.msg_head.can_id = 0x123;
    n = write(sfd, &msg, sizeof(msg));
    if (-1 == n) {
        error(EXIT_FAILURE, errno, "write");
    }

    while (run) {
        struct can_frame *frame;
        unsigned char i;

        /* Read 0x123 messages from the CAN interface */
        n = read(sfd, &msg, sizeof(msg));
        if (-1 == n) {
            if (EINTR == errno) {
                continue;
            }

            error(0, errno, "read");
            break;
        }

        frame = &msg.frames[0];

        /* Print the received CAN frame */
        printf("RX:  ");
        print_can_frame(frame);
        printf("\n");

        /* Modify the CAN frame to use our message ID */
        frame->can_id = MSGID;

        /* Increment the value of each byte in the CAN frame */
        for (i = 0; i < frame->len; i++) {
            frame->data[i] += 1;
        }

        /* Write the modified frame back out to the bus */
        msg.msg_head.opcode = TX_SEND;
        msg.msg_head.can_id = 0;
        msg.msg_head.nframes = 1;
        n = write(sfd, &msg, sizeof(msg));
        if (-1 == n) {
            if (EINTR == errno) {
                continue;
            }

            error(0, errno, "write");
            break;
        }

        /* Print the transmitted CAN frame */
        printf("TX:  ");
        print_can_frame(frame);
        printf("\n");
    }

    cleanup(sfd);
    puts("Goodbye!");
    return EXIT_SUCCESS;
}
