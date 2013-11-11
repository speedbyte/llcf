#****************************************************************************
# Copyright (C) 2001-2007  PEAK System-Technik GmbH
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
# Makefile - Makefile for the shared library libpcan.so.x.x
#
# $Id: Makefile 450 2007-01-31 22:20:12Z khitschler $
#
#****************************************************************************

SRC     = src
INC     = -I. -I../driver
FILES   = $(SRC)/libpcan.c
DBG     = -g
RT      = NO_XENOMAI

ifeq ($(RT), XENOMAI)
#****************************************************************************
# Define flags for XENOMAI installation only
#
USB = NO_USB_SUPPORT
PCC = NO_PCCARD_SUPPORT

INC     = -I. -I../driver -I/usr/xenomai/include

SKIN = xeno
XENO_DIR          ?= /usr/xenomai
XENO_CONFIG       ?= $(XENO_DIR)/bin/xeno-config
XENO_LIB_DIR      ?= $(shell $(XENO_CONFIG) --library-dir)
XENO_CFLAGS       ?= $(shell $(XENO_CONFIG) --$(SKIN)-cflags) -lrtdm
endif

ifeq ($(HOSTTYPE),x86_64)
  LIBPATH = /usr/lib64
else
  LIBPATH = /usr/lib
endif

LDNAME  = libpcan.so
SONAME  = $(LDNAME).0
TARGET  = $(SONAME).5

ifeq ($(RT), XENOMAI)
  CFLAGS = -fPIC -shared -O2 -Wall -Wl,-soname,$(SONAME) -lc $(INC) -D$(RT) $(XENO_CFLAGS) -L$(XENO_LIB_DIR)
else
  CFLAGS = -fPIC -shared -O2 -Wall -Wl,-soname,$(SONAME) -lc $(INC)
endif

all:    $(TARGET)

$(TARGET) : $(FILES)
	# ln -sf ../driver/pcan.h pcan.h
	$(CC) $(FILES) $(CFLAGS) -o $(TARGET)
	ln -sf $(TARGET) $(LDNAME)

clean:
	rm -f $(SRC)/*~ $(SRC)/*.o *~ *.so.*

#********** this entry is reserved for root access only *********************
install:
	cp $(TARGET) $(LIBPATH)/$(TARGET)
	ln -sf $(LIBPATH)/$(TARGET) $(LIBPATH)/$(SONAME)
	ln -sf $(LIBPATH)/$(SONAME) $(LIBPATH)/$(LDNAME)
	cp libpcan.h /usr/include/libpcan.h
	chmod 644 /usr/include/libpcan.h
	/sbin/ldconfig
  
  

