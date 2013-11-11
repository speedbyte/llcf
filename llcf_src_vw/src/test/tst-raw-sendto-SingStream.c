/*
 *  $Id: tst-raw-sendto.c,v 1.1 2006/02/20 14:17:50 hartko Exp $
 */

/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "af_can.h"
#include "raw.h"

int main(int argc, char **argv)
{
    int s;
    struct sockaddr_can addr;
    struct can_frame frame;
    int nbytes;
    struct ifreq ifr;
    char *ifname = "vcan2";
    int ifindex;
    int opt;

    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
        case 'i':
	    ifname = optarg;
            break;
        default:
            fprintf(stderr, "Unknown option %c\n", opt);
            break;
        }
    }


    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
      perror("socket");
      return 1;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);
    ifindex = ifr.ifr_ifindex;

    addr.can_family  = AF_CAN;
    addr.can_ifindex = 0; /* bind to all interfaces */
 
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("bind");
      return 1;
    }


    /* fill CAN frame */
    frame.can_id  = 0x123;
    frame.can_dlc = 3;
    frame.data[0] = 0x11;
    frame.data[1] = 0x22;
    frame.data[2] = 0x33;

    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifindex; /* send via this interface */
 
    nbytes = sendto(s, &frame, sizeof(struct can_frame), 0, (struct sockaddr*)&addr, sizeof(addr));

    close(s);

    return 0;
}
