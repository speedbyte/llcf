#
#  $Id: Makefile,v 1.4 2005/06/30 10:56:55 hartko Exp $
#

# 
#  Low Level CAN Framework
#
#  Driver layer Makefile for Infineon TwinCAN TC1920
# 
#  Copyright (c) 2005 Volkswagen Group Electronic Research
#  38436 Wolfsburg, GERMANY
#
#  contact email: llcf@volkswagen.de
#
#  Idea, Design, Implementation:
#  Oliver Hartkopp <oliver.hartkopp@volkswagen.de>
#  Dr. Urs Thuermann <urs.thuermann@volkswagen.de>
#  Matthias Brukner <m.brukner@trajet.de>
#
#  Neither Volkswagen Group nor the authors admit liability
#  nor provide any warranty for any of this software.
#  This material is provided "AS-IS".
#
#  Until the distribution is granted by the Volkswagen rights
#  department this sourcecode is under non disclosure and must
#  only be used within projects with the Volkswagen Group.
#

#KERNELDIR = /home/eandesa/tricore/kernel

#CC      = tricore-gcc
#LINK    = tricore-ld
CFLAGS  = -Wall -O2 -D__Kernel__ -DMODULE -I$(KERNELDIR)/include -I../.. -mtc13 -mall-errata #-DTWINCAN_DEBUG
LDFLAGS =

OUTFILE = tc1920.o
OBJ = twincan.o twincan_hw.o

all: $(OUTFILE)

$(OUTFILE): $(OBJ)
	$(LINK) $(LDFLAGS) -r -o $(OUTFILE) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o "$(*F).o" $<

clean:
	rm -rf $(OUTFILE)
	rm -rf $(OBJ)
