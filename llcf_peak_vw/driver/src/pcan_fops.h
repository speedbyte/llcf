#ifndef __PCAN_FOPS_H__
#define __PCAN_FOPS_H__

//****************************************************************************
// Copyright (C) 2001-2007  PEAK System-Technik GmbH
//
// linux@peak-system.com
// www.peak-system.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Maintainer(s): Klaus Hitschler (klaus.hitschler@gmx.de)
//
// Major contributions by:
//                Edouard Tisserant (edouard.tisserant@lolitech.fr) XENOMAI
//                Laurent Bessard   (laurent.bessard@lolitech.fr)   XENOMAI
//                Oliver Hartkopp   (oliver.hartkopp@volkswagen.de) socketCAN
//                     
//****************************************************************************

//****************************************************************************
//
// pcan_fops.h - header for struct fops only
//
// $Id: pcan_fops.h 447 2007-01-28 14:05:50Z khitschler $
//
//****************************************************************************

//****************************************************************************
// INCLUDES
#include <linux/kernel.h>   // printk()
#include <linux/fs.h>       // everything...

#ifdef XENOMAI
#include <xenomai/rtdm/rtdm_driver.h>
#endif

//****************************************************************************
// DEFINES
int  pcan_open_path(struct pcandev *dev);
void pcan_release_path(struct pcandev *dev);

//****************************************************************************
// GLOBALS
#ifdef XENOMAI
extern struct rtdm_device pcandev_rt;
#else
extern struct file_operations pcan_fops;
#endif

#endif // __PCAN_FOPS_H__

