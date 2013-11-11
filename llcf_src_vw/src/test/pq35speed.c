/*
 *  $Id: pq35speed.c,v 1.6 2006/01/31 17:00:36 hartko Exp $
 *
 *  Small tool to check video-blocking at > 6km/h on navigation systems
 *
 */

/*
 *  Low Level CAN Framework
 *
 *  Copyright (c) 2005 Volkswagen Group Electronic Research
 *  38436 Wolfsburg, GERMANY
 *
 *  contact email: llcf@volkswagen.de
 *
 *  Idea, Design, Implementation:
 *  Oliver Hartkopp <oliver.hartkopp@volkswagen.de>
 *  Dr. Urs Thuermann <urs.thuermann@volkswagen.de>
 *  Matthias Brukner <m.brukner@trajet.de>
 *
 *  Neither Volkswagen Group nor the authors admit liability
 *  nor provide any warranty for any of this software.
 *  This material is provided "AS-IS".
 *
 *  Until the distribution is granted by the Volkswagen rights
 *  department this sourcecode is under non disclosure and must
 *  only be used within projects with the Volkswagen Group.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

#include "af_can.h"
#include "bcm.h"

#define U64_DATA(p) (*(unsigned long long*)(p)->data)

int main(int argc, char **argv)
{
    int s, speed;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct {
	struct bcm_msg_head msg_head;
	struct can_frame frame;
    } msg;

    if (argc != 3) {
	fprintf(stderr, "Usage: %s <candev> <speed> [km/h]\n", argv[0]);
	return -1;
    }

    speed = atoi(argv[2]);

    if ((s = socket(PF_CAN, SOCK_DGRAM, CAN_BCM)) < 0) {
	perror("socket");
	return 1;
    }

    addr.can_family = PF_CAN;

    if (strlen(argv[1]) < IFNAMSIZ) {
	strcpy(ifr.ifr_name, argv[1]);
	ioctl(s, SIOCGIFINDEX, &ifr);
	addr.can_ifindex = ifr.ifr_ifindex;
    }
    else {
	printf("CAN device name '%s' too long!", argv[1]);
	return 1;
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("connect");
	return 1;
    }

    printf("setting SPEED: %d km/h ... ", speed);
    fflush(stdout);

    msg.msg_head.opcode  = TX_SETUP;
    msg.msg_head.can_id  = 0x359;
    msg.msg_head.flags   = SETTIMER|STARTTIMER;
    msg.msg_head.nframes = 1;
    msg.msg_head.count = 0;
    msg.msg_head.ival1.tv_sec = 0;
    msg.msg_head.ival1.tv_usec = 0;
    msg.msg_head.ival2.tv_sec = 0;
    msg.msg_head.ival2.tv_usec = 500000;
    msg.frame.can_id    = 0x359;
    msg.frame.can_dlc   = 8;
    U64_DATA(&msg.frame) = (__u64) 0;
    msg.frame.data[2] = speed & 0xFF;

    if (write(s, &msg, sizeof(msg)) < 0)
      perror("write");

    getchar();

    /* restore speed to zero */

    msg.msg_head.opcode  = TX_SETUP;
    msg.msg_head.can_id  = 0x359;
    msg.msg_head.flags   = SETTIMER|STARTTIMER;
    msg.msg_head.nframes = 1;
    msg.msg_head.count = 0;
    msg.msg_head.ival1.tv_sec = 0;
    msg.msg_head.ival1.tv_usec = 0;
    msg.msg_head.ival2.tv_sec = 0;
    msg.msg_head.ival2.tv_usec = 500000;
    msg.frame.can_id    = 0x359;
    msg.frame.can_dlc   = 8;
    U64_DATA(&msg.frame) = (__u64) 0;

    if (write(s, &msg, sizeof(msg)) < 0)
      perror("write");

    printf("done\n");

    close(s);

    return 0;
}
