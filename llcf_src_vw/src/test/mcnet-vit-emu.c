/*
 *  $Id: mcnet-vit-emu.c,v 1.7 2005/12/16 09:04:43 hartko Exp $
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "af_can.h"
#include "mcnet.h"
#include "bcm.h"

#undef SETSOCKOPTS
#define DEBUG(x) fprintf x 

#define DEV "can0"

const char vit_mas_r_start_up_info[] = {0x15, 0x01, 0x01, 0x02, 0x00, 0x00};
const char vit_mas_r_power_on[] = {0x01, 0x01};
const char vit_mas_r_power_off[] = {0x01, 0x02};
const char vit_mas_r_get_version[] = {0x09, 0x01, 0x40, 0x40, 0x04, 0x0D};
const char vit_mas_r_get_box_status[] = {0x23, 0x00};
const char vit_mas_r_get_framemode[] = {0x27, 0x02};
const char vit_mas_r_set_projectcode[] = {0x51, 0x40, 0x01};

unsigned char buffer[4096];

char key2char(unsigned char pressed, unsigned char hi, unsigned char lo);

int main(int argc, char **argv)
{
    fd_set rdfs;
    int s, t, nbytes, ret;
    int quit = 0;
    struct sockaddr_can addr;
    struct ifreq ifr;

#ifdef SETSOCKOPTS
    struct can_mcnet_options opts;
#endif

    struct {
	struct bcm_msg_head msg_head;
	struct can_frame frame;
    } bmsg;


    if ((s = socket(PF_CAN, SOCK_SEQPACKET, CAN_MCNET)) < 0) {
	perror("mcnet socket");
	exit(1);
    }

    if ((t = socket(PF_CAN, SOCK_DGRAM, CAN_BCM)) < 0) {
	perror("bcm socket");
	exit(1);
    }

#ifdef SETSOCKOPTS

    opts.blocksize       = 11;

    opts.config = USE_CONNECTIONTEST;

    setsockopt(s, SOL_CAN_MCNET, CAN_MCNET_OPT, &opts, sizeof(opts));

#endif

    addr.can_family = AF_CAN;
    strcpy(ifr.ifr_name, DEV);
    ioctl(s, SIOCGIFINDEX, &ifr);
    addr.can_ifindex = ifr.ifr_ifindex;
    addr.can_addr.mcnet.tx_id  = 0x264;
    addr.can_addr.mcnet.rx_id  = 0x464;

    if (connect(t, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("bcm connect");
	close(t);
	exit(1);
    }

    bmsg.msg_head.opcode = RX_SETUP;
    bmsg.msg_head.can_id = 1; /* master watchdog CAN-ID */
    bmsg.msg_head.flags = SETTIMER;
    bmsg.msg_head.ival1.tv_sec = 2; /* timeout for master watchdog */
    bmsg.msg_head.ival1.tv_usec = 0;
    bmsg.msg_head.ival2.tv_sec = 0;
    bmsg.msg_head.ival2.tv_usec = 0;
    bmsg.msg_head.nframes = 1;
    memset(bmsg.frame.data, 0, 8); /* clear all bits */

    if (write(t, &bmsg, sizeof(bmsg)) < 0)
	perror("write");

    /* blocking wait for master watchdog */
    if ((nbytes = read(t, &bmsg, sizeof(bmsg))) < 0)
	perror("read");

    DEBUG((stderr, "got opcode 0x%02X from BCM\n", bmsg.msg_head.opcode));

    if (bmsg.msg_head.opcode != RX_CHANGED) {
	perror("bcm no rx_changed");
	close(t);
	exit(1);
    }

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	perror("mcnet connect");
	close(t);
	close(s);
	exit(1);
    }

    /* if the client gets connected, he has to send a startup info */ 

    write(s, vit_mas_r_start_up_info, sizeof(vit_mas_r_start_up_info));

    while (!quit) {

	FD_ZERO(&rdfs);
	FD_SET(s, &rdfs);
	FD_SET(t, &rdfs);
	FD_SET(0, &rdfs);

	if ((ret = select(t+1, &rdfs, NULL, NULL, NULL)) < 0) {
	    perror("select");
	    continue;
	}

	if (FD_ISSET(0, &rdfs)) {
	    getchar();
	    quit = 1;
	    DEBUG((stderr, "quit due to keyboard intput."));
	}

	if (FD_ISSET(t, &rdfs)) {
	    if ((nbytes = read(t, &bmsg, sizeof(bmsg))) < 0)
		perror("read");

	    DEBUG((stderr, "got opcode 0x%02X from BCM\n", bmsg.msg_head.opcode));

	    if (bmsg.msg_head.opcode != RX_TIMEOUT) {
		perror("bcm no rx_timeout");
		close(t);
		close(s);
		exit(1);
	    }
	    quit = 1;
	    DEBUG((stderr, "quit due to missing master watchdog."));
	}

	if (FD_ISSET(s, &rdfs)) {
	    nbytes = read(s, buffer, 4096);
	    
	    switch (buffer[0]) {

	    case 0x00:
		DEBUG((stderr, "got _C_POWER"));
		if (buffer[1] == 0x01) { /* power on */
		    DEBUG((stderr, "_ON '"));
		    fprintf(stderr, "*");
		    DEBUG((stderr, "'\n"));
		    write(s, vit_mas_r_power_on, sizeof(vit_mas_r_power_on));
		}
		else {
		    DEBUG((stderr, "_OFF\n"));
		    write(s, vit_mas_r_power_off, sizeof(vit_mas_r_power_off));
		    /* quit = 1; NO! - Leave this program only via missing wdog */
		}
		break;
		
	    case 0x08:
		DEBUG((stderr, "got _C_GET_VERSION\n"));
		write(s, vit_mas_r_get_version, sizeof(vit_mas_r_get_version));
		break;
		
	    case 0x20:
		DEBUG((stderr, "got _C_KEYPRESS pressed = %02X high = %02X low = %02X ",
		       buffer[1], buffer[3], buffer[2]));
		fprintf(stderr, "%c", key2char(buffer[1], buffer[3], buffer[2]));
		DEBUG((stderr, "'\n"));
		break;
		
	    case 0x22:
		DEBUG((stderr, "got _C_GET_BOX_STATUS\n"));
		write(s, vit_mas_r_get_box_status, sizeof(vit_mas_r_get_box_status));
		break;
		
	    case 0x26:
		DEBUG((stderr, "got _C_GET_FRAMEMODE\n"));
		write(s, vit_mas_r_get_framemode, sizeof(vit_mas_r_get_framemode));
		break;
		
	    case 0x50:
		DEBUG((stderr, "got _C_SET_PROJECTCODE\n"));
		write(s, vit_mas_r_set_projectcode, sizeof(vit_mas_r_set_projectcode));
		break;
		
	    default:
		DEBUG((stderr, "got command 0x%02X\n", buffer[0]));
		break;
	    }
	}
    }

    DEBUG((stderr, " '"));
    fprintf(stderr, "+");
    DEBUG((stderr, "'"));
    fprintf(stderr, "\n");

    close(s);
    close(t);

    return 0;
}

char key2char(unsigned char pressed, unsigned char hi, unsigned char lo) {

    int key = (hi<<8) + lo;
    unsigned char press = (pressed & 2) << 4;

    DEBUG((stderr, "key = %04X '", key));

    switch (key) {

	/* the encoder and the preset calls have no release function */
	/* so `return '<'` and  `return press + '<'` do the same */

    /* encoder */
    case 0xFF00:
	return press + '<';
	
    case 0xFF01:
	return press + '>';
    
    /* call preset */
    case 0x0002:
	return press + '1';
	
    case 0x0001:
	return press + '2';
	
    case 0x0020:
	return press + '3';
	
    case 0x2000:
	return press + '4';
	
    case 0x1000:
	return press + '5';
	
    case 0x0200:
	return press + '6';
	
    /* store preset */
    case 0x8002:
	return press + 'K';
	
    case 0x8001:
	return press + 'L';
	
    case 0x8020:
	return press + 'M';
	
    case 0xA000:
	return press + 'N';
	
    case 0x9000:
	return press + 'O';
	
    case 0x8200:
	return press + 'P';
    
    /* other keys */
    case 0x4000:
        return press + 'Q'; /* quit / escape */
	
    case 0x0003:
        return press + 'A'; /* autostore */ 

    case 0x0030:
        return press + 'S'; /* scan */

    case 0x0004:
        return press + 'B'; /* back arrow */ 

    case 0x0005:
        return press + 'F'; /* forward arrow */ 

    case 0x0500:
        return press + 'E'; /* enter */ 

    case 0x00FF:
        return press + 'V'; /* version */ 

    default:
	return '@';
    }
}
