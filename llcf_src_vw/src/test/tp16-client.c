/*
 *  $Id: tp16-client.c,v 1.11 2006/01/31 17:00:37 hartko Exp $
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

#undef SETSOCKOPTS
#define DYNCHS

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "af_can.h"
#include "tp16.h"

#ifdef DYNCHS

#include "bcm.h"

#define LOCAL_TP_ID  1
#define REMOTE_TP_ID 2
#define LOCAL_TP_TX_CHANNEL 0x81
#define BUS_SPECIFIC_OFFSET 0x700
#endif

#define DEV "vcan2"

const char message[] = "einnegermitgazellezagtimregennie";
char buffer[4096];

int main(int argc, char **argv)
{
    int s, nbytes;
    struct sockaddr_can addr;
    struct ifreq ifr;

#ifdef SETSOCKOPTS
    struct can_tp16_options opts;
#endif

#ifdef DYNCHS

    struct {
	struct bcm_msg_head msg_head;
	struct can_frame frame;
    } msg;

    memset(&msg, 0, sizeof(msg));

    if ((s = socket(PF_CAN, SOCK_DGRAM, CAN_BCM)) < 0) {
	perror("bcm socket");
	return 1;
    }

    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, DEV);
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_ifindex = ifr.ifr_ifindex;

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("bcm connect");
	return 1;
    }

    /* set filter for response from TP1.6 Server */

    msg.msg_head.opcode	 = RX_SETUP;
    msg.msg_head.can_id	 = 0x200 + REMOTE_TP_ID;
    msg.msg_head.flags	 = RX_FILTER_ID; /* special case can have nframes = 0 */
    /* nframes and all the rest are set to zero due to memset() above */

    if (write(s, &msg, sizeof(msg)) < 0)
      perror("bcm write1");

    /* send channel setup request */

    msg.msg_head.opcode	 = TX_SETUP;
    msg.msg_head.can_id	 = 0x200 + LOCAL_TP_ID;
    msg.msg_head.flags	 = (SETTIMER|STARTTIMER|TX_COUNTEVT);
    msg.msg_head.nframes = 1;
    msg.msg_head.count = 10; /* max. number of retransmissions */
    msg.msg_head.ival1.tv_sec = 0;
    msg.msg_head.ival1.tv_usec = 50000; /* 50 ms retry */
    msg.msg_head.ival2.tv_sec = 0;
    msg.msg_head.ival2.tv_usec = 0;
    msg.frame.can_id	= 0x200 + LOCAL_TP_ID;
    msg.frame.can_dlc	= 3;
    msg.frame.data[0] = REMOTE_TP_ID;
    msg.frame.data[1] = 0xC0; /* TP request */
    msg.frame.data[2] = LOCAL_TP_TX_CHANNEL;

    if (write(s, &msg, sizeof(msg)) < 0)
      perror("bcm write2");

    /* read channel setup reply */

    if ((nbytes = read(s, &msg, sizeof(msg))) < 0)
      perror("bcm read");

    if (msg.msg_head.opcode == RX_CHANGED &&
	nbytes == sizeof(struct bcm_msg_head) + sizeof(struct can_frame) &&
	msg.msg_head.can_id == (0x200 + REMOTE_TP_ID) &&
	msg.frame.can_id == (0x200 + REMOTE_TP_ID))
	printf("Received correct channel setup reply.\n");
    else {
	if (msg.msg_head.opcode == TX_EXPIRED)
	    printf("Timeout: Unable to get channel setup reply!\n");
	else
	    printf("Received incorrect channel setup reply! Check BCM settings!\n");
	close(s);
	return 1;
    }

    if (msg.frame.data[1] != 0xD0) {
	printf("Negative channel setup reply 0x%02X!\n",
	       msg.frame.data[1]);
	close(s);
	return 1;
    }

    /* remark: opposite RX is my TX :o) */
    addr.can_addr.tp16.tx_id  = BUS_SPECIFIC_OFFSET + LOCAL_TP_TX_CHANNEL;
    addr.can_addr.tp16.rx_id  = BUS_SPECIFIC_OFFSET + msg.frame.data[2];

#if 0 /* the following is obsolete as close() cleans up the following anyhow */

    /* stop sending of channel setup messages */
    msg.msg_head.opcode	 = TX_DELETE;
    msg.msg_head.can_id	 = 0x200 + LOCAL_TP_ID;

    if (write(s, &msg, sizeof(struct bcm_msg_head)) < 0)
      perror("bcm write3");

    /* remove RX subscription */
    msg.msg_head.opcode	 = RX_DELETE;
    msg.msg_head.can_id	 = 0x200 + REMOTE_TP_ID;

    if (write(s, &msg, sizeof(struct bcm_msg_head)) < 0)
      perror("bcm write4");

#endif

    close(s); /* BCM socket is not needed anymore */

#endif /* DYNCHS */

    if ((s = socket(PF_CAN, SOCK_SEQPACKET, CAN_TP16)) < 0) {
	perror("socket");
	exit(1);
    }

#ifdef DYNCHS

    /* addr is already created at this point */

#else

    /* test setup without dynamic channel setup */

    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, DEV);
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_ifindex = ifr.ifr_ifindex;
    addr.can_addr.tp16.tx_id  = 0x740;
    addr.can_addr.tp16.rx_id  = 0x760;

#endif

#ifdef SETSOCKOPTS
    opts.t1.tv_sec	 = -1; /* use default value */
    opts.t2.tv_sec	 = -1; /* unused */
    opts.t3.tv_sec	 =  0; /* timeout 100ms */
    opts.t3.tv_usec	 =  100000;
    opts.t4.tv_sec	 = -1; /* unused */
    opts.blocksize	 = 11;

    opts.config = USE_CONNECTIONTEST | USE_DISCONNECT | ENABLE_BREAK;

    setsockopt(s, SOL_CAN_TP16, CAN_TP16_OPT, &opts, sizeof(opts));
#endif

    printf("\nWait for connection on %s: tx_id=0x%03X rx_id=0x%03X ... ",
	   DEV, addr.can_addr.tp16.tx_id, addr.can_addr.tp16.rx_id);
    fflush(stdout);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("connect");
	close(s);
	exit(1);
    }

    printf("connected!\n\nSending message '%s' ...", message);
    write(s, message, sizeof(message)-1);
    printf(" done!\n\n");

    printf("Waiting for reply message ... ");
    fflush(stdout);

    nbytes = read(s, buffer, 4096);
    printf("ok.\nReceived %d bytes: '", nbytes);
    buffer[nbytes] = 0; /* ensure terminated string */
    printf("%s'\n\nClosing connection.\n\n", buffer);

    sleep(4); /* to check the AUTO_DISCONNECT feature */

    close(s);

    return 0;
}
