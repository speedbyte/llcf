#****************************************************************************
# Copyright (C) 2001-2006  PEAK System-Technik GmbH
#
# linux@peak-system.com
# www.peak-system.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Maintainer(s): Klaus Hitschler (klaus.hitschler@gmx.de)
#****************************************************************************

#****************************************************************************
#
# Makefile - Makefile for receivetest and transmittest programs
#
# $Id: Makefile 410 2006-08-30 16:51:58Z edouard $
#
#****************************************************************************

SRC     = src
INC     = -I. -I../lib -I../driver
RT      = NO_XENOMAI

ifeq ($(RT), XENOMAI)
SKIN = xeno

### Xenomai directory, xeno-config and library directory ###########
XENO_DIR          ?= /usr/xenomai
XENO_CONFIG       ?= $(XENO_DIR)/bin/xeno-config
XENO_LIB_DIR      ?= $(shell $(XENO_CONFIG) --library-dir)

### User space application compile options #########################
USERAPP_LIBS      ?= -lnative -lpcan
USERAPP_LDFLAGS   ?= $(shell $(XENO_CONFIG) --$(SKIN)-ldflags)
USERAPP_CFLAGS    ?= $(shell $(XENO_CONFIG) --$(SKIN)-cflags)
else
USERAPP_LIBS = -lpcan
endif

ifeq ($(HOSTTYPE),x86_64)
  LDLIBS  = -L../lib -L/lib64 -L/usr/lib64 -L/usr/local/lib64
else
  LDLIBS  = -L../lib -L/lib -L/usr/lib -L/usr/local/lib
endif

ifeq ($(RT), XENOMAI)
DBGFLAGS   =
else
DBGFLAGS   = -g
endif

ifeq ($(DBG), DEBUG)
CFLAGS  = $(DBGFLAGS) $(INC) $(LDLIBS)
else
CFLAGS  = $(INC) $(LDLIBS)
endif

TARGET1 = receivetest
FILES1  = $(SRC)/$(TARGET1).c $(SRC)/common.c

TARGET2 = transmitest
FILES2  = $(SRC)/$(TARGET2).cpp $(SRC)/common.c $(SRC)/parser.cpp

TARGET3 = bitratetest
FILES3  = $(SRC)/$(TARGET3).c $(SRC)/common.c

all:    $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(FILES1)
	$(CC) $(FILES1) $(CFLAGS) -o $(TARGET1) $(USERAPP_CFLAGS) $(USERAPP_LDFLAGS) $(USERAPP_LIBS) -D$(RT)

$(TARGET2): $(FILES2)
	$(CC) $(FILES2) $(CFLAGS) -o $(TARGET2) $(USERAPP_CFLAGS) $(USERAPP_LDFLAGS) $(USERAPP_LIBS) -lstdc++ -D$(RT)
	
$(TARGET3): $(FILES3)
	$(CC) $(FILES3) $(CFLAGS) -o $(TARGET3) $(USERAPP_CFLAGS) $(USERAPP_LDFLAGS) $(USERAPP_LIBS) -D$(RT)


clean:
	rm -f $(SRC)/*~ $(SRC)/*.o *~ $(TARGET1) $(TARGET2) $(TARGET3)
	
install:
	cp $(TARGET1) /usr/local/bin
	cp $(TARGET2) /usr/local/bin
	cp $(TARGET3) /usr/local/bin



