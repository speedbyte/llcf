/*
 *  $Id: tp20-sniffer.c,v 1.9 2005/12/19 13:39:15 hartko Exp $
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
#include <libgen.h>
#include <ctype.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "af_can.h"
#include "tp20.h"
#include "raw.h"

#undef DEBUG

#ifdef DEBUG
#define DBG(args...) printf(args)
#else
#define DBG(args...)
#endif

#define CANDEV "can0"

#define FORMAT_HEX 1
#define FORMAT_ASCII 2
#define FORMAT_DEFAULT (FORMAT_ASCII | FORMAT_HEX)

void print_usage(char *prg)
{
	fprintf(stderr, "Usage: %s [options]\n", prg);
	fprintf(stderr, "Options: -s <source> (logical source component HEX)\n");
	fprintf(stderr, "	  -d <dest>   (logical destination component HEX)\n");
	fprintf(stderr, "	  -c <device> (used CAN bus - default: '%s')\n", CANDEV);
	fprintf(stderr, "	  -f <format> (1 = HEX, 2 = ASCII, 3 = HEX & ASCII - default: %d)\n", FORMAT_DEFAULT);
	fprintf(stderr, "	  -i	      (ID-Mode: <source> and <dest> are CAN-IDs)\n");
	fprintf(stderr, "	  -t	      (print timestamp)\n");
}

int main(int argc, char **argv)
{
	fd_set rdfs;
	int r = 0;    /* raw socket */
	int s, t = 0; /* tp sockets */
	int opt, nbytes, ret, i;
	int quit = 0;

	struct sockaddr_can addr, addr1, addr2;
	struct ifreq ifr;
	struct can_tp20_options opts;

	struct can_filter rfilter;
	struct can_frame frame;

	struct timeval tv;

	char candevice[8];   /* commandline option */ 
	int src    = 0;
	int dest   = 0;
	int idmode = 0;
	int format = FORMAT_DEFAULT;
	int src_id, dest_id; /* CAN-IDs */
	unsigned char print_timestamp = 0;

	unsigned char buffer[4096];

	strcpy(candevice, CANDEV);
    
        while ((opt = getopt(argc, argv, "s:d:c:f:it")) != -1) {
                switch (opt) {
		case 's':
			src = strtoul(optarg, (char **)NULL, 16);
			break;

		case 'd':
			dest = strtoul(optarg, (char **)NULL, 16);
			break;

		case 'c':
			if (strlen(optarg) >= IFNAMSIZ) {
				printf("Name of CAN device '%s' is too long!\n\n", optarg);
				exit(1);
			}
			else
				strcpy(candevice, optarg);
			break;
	    
		case 'f':
			format = (atoi(optarg) & (FORMAT_ASCII | FORMAT_HEX));
			break;

		case 'i':
			idmode = 1;
			break;

		case 't':
			print_timestamp = 1;
			break;

		case '?':
			break;

		default:
			fprintf(stderr, "Unknown option %c\n", opt);
			print_usage((char*) basename(argv[0]));
			return 1;
                }
        }

	if (src == dest) {
		print_usage((char*) basename(argv[0]));
		return 1;
	}

	printf("\n%s: ", basename(argv[0]));
        
	if (idmode)
		printf("searching for TP channel on CAN-IDs 0x%03X / 0x%03X on %s ...\n", src, dest, candevice);
	else
		printf("searching for logical components 0x%X / 0x%X on %s ...\n", src, dest, candevice);

	if (idmode) {

		/* simple way to find the CAN-IDs */
		src_id  = src;
		dest_id = dest;
	}
	else {

		/* find Channel Setup Positive reply */

		if ((r = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
			perror("raw socket");
			return 1;
		}

		/* listen only for CAN-IDs 0x200 - 0x27F */
		rfilter.can_id   = 0x200;
		rfilter.can_mask = 0xF80;

		setsockopt(r, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

		addr.can_family = AF_CAN;
		strcpy(ifr.ifr_name, candevice);
		ioctl(r, SIOCGIFINDEX, &ifr);
		addr.can_ifindex = ifr.ifr_ifindex;

		if (bind(r, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("raw bind");
			return 1;
		}

		memset(&frame, 0, sizeof(frame)); /* clear receive frame */

		while (frame.can_dlc == 0) {
			
			if ((nbytes = read(r, &frame, sizeof(frame))) < 0)
				perror("raw read");
			
			if (frame.data[1] != 0xD0) {
				DBG("No TP-Positive Reply\n");
				frame.can_dlc = 0; /* stay in while () */
				continue;
			}

			if (frame.data[0] != src) {
				DBG("No CHS for me: Source ID %d(%d)\n",
				    frame.data[0], src);
				frame.can_dlc = 0; /* stay in while () */
				continue;
			}

			if ((frame.can_id & 0x7F) != dest) {
				DBG("No CHS for me: Destination ID %d(%d)\n",
				    (frame.can_id & 0x7F), dest);
				frame.can_dlc = 0; /* stay in while () */
				continue;
			}

			if (frame.can_dlc != 7 ) {
				DBG("No CHS for me: can_dlc %d(7)\n",
				    frame.can_dlc);
				frame.can_dlc = 0; /* stay in while () */
				continue;
			}
		}

		close(r); /* raw socket not needed anymore */

		src_id  = frame.data[2] + ((frame.data[3] & 0x07)<<8);
		dest_id = frame.data[4] + ((frame.data[5] & 0x07)<<8);
	}

	printf("%s: sniffing  for CAN-IDs 0x%03X / 0x%03X on %s ...\n\n", basename(argv[0]), src_id, dest_id, candevice);

	if ((s = socket(PF_CAN, SOCK_SEQPACKET, CAN_TP20)) < 0) {
		perror("tp20 socket1");
		exit(1);
	}

	if ((t = socket(PF_CAN, SOCK_SEQPACKET, CAN_TP20)) < 0) {
		perror("tp20 socket2");
		exit(1);
	}

	opts.blocksize = 15;
		strcpy(ifr.ifr_name, candevice);
		ioctl(r, SIOCGIFINDEX, &ifr);
	opts.config = TRACE_MODE;
	setsockopt(s, SOL_CAN_TP20, CAN_TP20_OPT, &opts, sizeof(opts));
	setsockopt(t, SOL_CAN_TP20, CAN_TP20_OPT, &opts, sizeof(opts));

	strcpy(ifr.ifr_name, candevice);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr1.can_family = AF_CAN;
	addr1.can_ifindex = ifr.ifr_ifindex;
	addr1.can_addr.tp20.tx_id  = src_id;
	addr1.can_addr.tp20.rx_id  = dest_id;

	addr2.can_family = AF_CAN;
	addr2.can_ifindex = ifr.ifr_ifindex;
	addr2.can_addr.tp20.tx_id  = dest_id;
	addr2.can_addr.tp20.rx_id  = src_id;

	if (connect(s, (struct sockaddr *)&addr1, sizeof(addr1)) < 0) {
		perror("tp20 connect1");
		close(s);
		exit(1);
	}

	if (connect(t, (struct sockaddr *)&addr2, sizeof(addr2)) < 0) {
		perror("tp20 connect2");
		close(s);
		close(t);
		exit(1);
	}

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
			printf("quit due to keyboard intput.\n");
		}

		if (FD_ISSET(s, &rdfs)) {
			nbytes = read(s, buffer, 4096);

			if (print_timestamp) {
				ioctl(s, SIOCGSTAMP, &tv);
				printf("(%ld.%06ld)   ", tv.tv_sec, tv.tv_usec);
			}
	    
			printf("<%03X> >> ", src_id);
			if (format & FORMAT_HEX) {
				for (i=0; i<nbytes; i++)
					printf("%02X ", buffer[i]);
				if (format & FORMAT_ASCII)
					printf(" - ");
			}
			if (format & FORMAT_ASCII) {
				printf("'");
				for (i=0; i<nbytes; i++) {
					if ((buffer[i] > 0x1F) && (buffer[i] < 0x7F))
						printf("%c", buffer[i]);
					else
						printf(".");
				}
				printf("'");
			}

			printf("\n");
			fflush(stdout);
		}
		if (FD_ISSET(t, &rdfs)) {
			nbytes = read(t, buffer, 4096);

			if (print_timestamp) {
				ioctl(t, SIOCGSTAMP, &tv);
				printf("(%ld.%06ld)   ", tv.tv_sec, tv.tv_usec);
			}

			printf("<%03X> << ", dest_id);
			if (format & FORMAT_HEX) {
				for (i=0; i<nbytes; i++)
					printf("%02X ", buffer[i]);
				if (format & FORMAT_ASCII)
					printf(" - ");
			}
			if (format & FORMAT_ASCII) {
				printf("'");
				for (i=0; i<nbytes; i++) {
					if ((buffer[i] > 0x1F) && (buffer[i] < 0x7F))
						printf("%c", buffer[i]);
					else
						printf(".");
				}
				printf("'");
			}

			printf("\n");
			fflush(stdout);
		}
	}

	close(s);
	close(t);


	return 0;
}
