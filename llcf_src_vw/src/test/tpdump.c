/*
 *  $Id: tpdump.c,v 1.1 2006/02/23 10:25:42 hartko Exp $
 */

/*
 * tpdump.c
 *
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
#include <libgen.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "af_can.h"
#include "raw.h"
#include "terminal.h"

void print_usage(char *prg)
{
    fprintf(stderr, "Usage: %s <options>\n", prg);
    fprintf(stderr, "Options: -s <can_id> (source can_id)\n");
    fprintf(stderr, "         -d <can_id> (destination can_id)\n");
    fprintf(stderr, "         -i <can>    (interface)\n");
    fprintf(stderr, "         -c          (color mode)\n");
    fprintf(stderr, "         -a          (print data also in ASCII-chars)\n");
    fprintf(stderr, "         -t          (timestamp: print with date)\n");
}

int main(int argc, char **argv)
{
    int s;
    struct sockaddr_can addr;
    struct can_filter rfilter[2];
    struct can_frame frame;
    int nbytes, i;
    int src = 0;
    int dst = 0;
    int asc = 0;
    int color = 0;
    int timestamp = 0;
    struct ifreq ifr;
    char *ifname = "vcan2";
    int ifindex;
    struct timeval tv;
    int opt;

    while ((opt = getopt(argc, argv, "s:d:i:act")) != -1) {
        switch (opt) {
	case 's':
	  src = strtoul(optarg, (char **)NULL, 16);
	  break;

	case 'd':
	  dst = strtoul(optarg, (char **)NULL, 16);
	  break;

        case 'i':
	    ifname = optarg;
            break;

	case 'c':
	    color = 1;
	    break;

	case 'a':
	    asc = 1;
	    break;

	case 't':
	    timestamp = 1;
	    break;

        default:
            fprintf(stderr, "Unknown option %c\n", opt);
	    print_usage(basename(argv[0]));
	    exit(0);
            break;
        }
    }

    if (src == dst) {
	print_usage(basename(argv[0]));
	exit(0);
    }

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
      perror("socket");
      return 1;
    }

    rfilter[0].can_id   = src;
    rfilter[0].can_mask = CAN_SFF_MASK;
    rfilter[1].can_id   = dst;
    rfilter[1].can_mask = CAN_SFF_MASK;

    setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);
    ifindex = ifr.ifr_ifindex;

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("bind");
      return 1;
    }

    while (1) {

	if ((nbytes = read(s, &frame, sizeof(struct can_frame))) < 0) {
	    perror("read");
	    return 1;
	} else if (nbytes < sizeof(struct can_frame)) {
	    fprintf(stderr, "read: incomplete CAN frame\n");
	    return 1;
	} else {
	    unsigned char tpci = frame.data[0];

	    if (color)
		printf("%s", (frame.can_id == src)? FGRED:FGBLUE);

	    ioctl(s, SIOCGSTAMP, &tv);

	    if (timestamp) {
		struct tm tm;
		char timestring[25];
		tm = *localtime(&tv.tv_sec);
		strftime(timestring, 24, "%Y-%m-%d %H:%M:%S", &tm);
		printf("(%s.%06ld)", timestring, tv.tv_usec);
	    }
	    else
		printf("(%ld.%06ld)", tv.tv_sec, tv.tv_usec);

	    printf("  %3X  ", frame.can_id);
	    //printf("[%d] ", frame.can_dlc);
	    
	    switch (tpci) {
	    case 0xA0:
		printf("[CS]");
		break;

	    case 0xA1:
		printf("[CA]");
		break;

	    case 0xA3:
		printf("[CT]");
		break;

	    case 0xA4:
		printf("[BR]");
		break;

	    case 0xA8:
		printf("[DC]");
		break;

	    default:
		if (tpci & 0x80) {
		    printf("[ACK");
		    if (tpci & 0x20)
			printf(",RR");
		    printf("]");
		} else {
		    printf("[DT");
		    if (!(tpci & 0x20))
			printf(",AR");
		    if (tpci & 0x10)
			printf(",EOM");
		    printf("]");
		}
	    }

	    if (frame.can_dlc > 1) {
		printf(" ");
		for (i = 1; i < frame.can_dlc; i++) {
		    printf("%02X ", frame.data[i]);
		}

		if (asc) {
		    printf("- '");
		    for (i = 1; i < frame.can_dlc; i++) {
			printf("%c",((frame.data[i] > 0x1F) && (frame.data[i] < 0x7F))? frame.data[i] : '.');
		    }
		    printf("'");
		}
	    }

	    if (color)
		printf("%s", ATTRESET);
	    printf("\n");
	    fflush(stdout);
	}
    }

    close(s);

    return 0;
}
