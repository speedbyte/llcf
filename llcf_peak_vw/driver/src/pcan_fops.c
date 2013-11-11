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
// Contributions: Marcel Offermans (marcel.offermans@luminis.nl)
//                Arno (a.vdlaan@hccnet.nl)
//                John Privitera (JohnPrivitera@dciautomation.com)
//****************************************************************************

//****************************************************************************
//
// pcan_fops.c - all file operation functions, exports only struct fops
//
// $Id: pcan_fops.c 448 2007-01-30 22:41:50Z khitschler $
//
//****************************************************************************

//****************************************************************************
// INCLUDES
#include <src/pcan_common.h>// must always be the 1st include
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/module.h>

#include <linux/kernel.h>   // DPRINTK()
#include <linux/slab.h>     // kmalloc()
#include <linux/fs.h>       // everything...
#include <linux/errno.h>    // error codes
#include <linux/types.h>    // size_t
#include <linux/proc_fs.h>  // proc
#include <linux/fcntl.h>    // O_ACCMODE
#include <linux/pci.h>      // all about pci
#include <linux/capability.h> // all about restrictions
#include <asm/system.h>     // cli(), *_flags
#include <asm/uaccess.h>    // copy_...
#include <linux/delay.h>    // mdelay()
#include <linux/poll.h>     // poll() and select()

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13)
#include <linux/moduleparam.h>
#endif

#include <src/pcan_main.h>
#include <src/pcan_pci.h>
#include <src/pcan_isa.h>
#include <src/pcan_dongle.h>
#include <src/pcan_sja1000.h>
#include <src/pcan_fifo.h>
#include <src/pcan_fops.h>
#include <src/pcan_parse.h>
#include <src/pcan_usb.h>

//****************************************************************************
// DEFINES
// this was'nt present before
#ifndef MODULE_LICENSE
#define MODULE_LICENSE(x)
#endif
#ifndef MODULE_VERSION
#define MODULE_VERSION(x)
#endif

MODULE_AUTHOR("klaus.hitschler@gmx.de");
MODULE_DESCRIPTION("Driver for PEAK-Systems CAN interfaces.");
MODULE_VERSION(CURRENT_RELEASE);
MODULE_SUPPORTED_DEVICE("PCAN-ISA, PCAN-PC/104, PCAN-Dongle, PCAN-PCI, PCAN-PCCard, PCAN-USB (compilation dependent)");
MODULE_LICENSE("GPL");

#if defined(module_param_array) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,13)
extern char   *type[8];
extern ushort io[8];
extern char   irq[8];
extern ushort bitrate;
extern char   *assign;

module_param_array(type, charp,  NULL, 0444);
module_param_array(io,   ushort, NULL, 0444);
module_param_array(irq,  byte,   NULL, 0444);
module_param(bitrate, ushort, 0444);
module_param(assign,  charp,  0444);
#else
MODULE_PARM(type,    "0-8s");
MODULE_PARM(io,      "0-8h");
MODULE_PARM(irq,     "0-8b");
MODULE_PARM(bitrate, "h");
MODULE_PARM(assign,  "s");
#endif

MODULE_PARM_DESC(type,    "The type of PCAN interface (isa, sp, epp)");
MODULE_PARM_DESC(io,      "The io-port address for either PCAN-ISA, PC/104 or Dongle");
MODULE_PARM_DESC(irq,     "The interrupt number for either PCAN-ISA, PC/104 or Dongle");
MODULE_PARM_DESC(bitrate, "The initial bitrate (BTR0BTR1) for all channels");
MODULE_PARM_DESC(assign,  "The assignment for netdevice names to CAN devices");


// wait this time in msec at max after releasing the device - give fifo a chance to flush
#define MAX_WAIT_UNTIL_CLOSE 1000

//****************************************************************************
// GLOBALS
#if defined(LINUX_24)
EXPORT_NO_SYMBOLS;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define minor(x) MINOR(x)
#endif

//****************************************************************************
// LOCALS

//****************************************************************************
// CODE

//----------------------------------------------------------------------------
// wait until write fifo is empty, max time in msec
static void wait_until_fifo_empty(struct pcandev *dev, u32 mTime)
{
  u32 dwStart = get_mtime();

  while (!atomic_read(&dev->DataSendReady) && ((get_mtime() - dwStart) < mTime))
    schedule();

  // force it
  atomic_set(&dev->DataSendReady, 1);
}

//----------------------------------------------------------------------------
// is called by pcan_open() and pcan_netdev_open()
int pcan_open_path(struct pcandev *dev)
{
  int err = 0;

  DPRINTK(KERN_DEBUG "%s: pcan_open_path, minor = %d, path = %d.\n",
    DEVICE_NAME, dev->nMinor, dev->nOpenPaths);

  // only the first open to this device makes a default init on this device
  if (!dev->nOpenPaths)
  {
    // empty all FIFOs
    err = pcan_fifo_reset(&dev->writeFifo);
    if (err)
      return err;
    err = pcan_fifo_reset(&dev->readFifo);
    if (err)
      return err;

    // open the interface special parts
    err = dev->open(dev);
    if (err)
    {
      DPRINTK(KERN_DEBUG "%s: can't open interface special parts!\n", DEVICE_NAME);
      return err;
    }

    // special handling: probe here only for dongle devices, connect after init is possible
    if ((dev->wType == HW_DONGLE_SJA) || (dev->wType == HW_DONGLE_SJA_EPP))
    {
      err = sja1000_probe(dev); // no usb here, generic sja1000 call for dongle
      if (err)
      {
        DPRINTK(KERN_ERR "%s: %s-dongle device minor %d not found (io=0x%04x,irq=%d)\n", DEVICE_NAME,
                                  dev->type, dev->nMinor, dev->port.dng.dwPort, dev->port.dng.wIrq);
        dev->release(dev);
        return err;
      }
    }

    // install irq
#ifdef XENOMAI
    err = dev->req_irq(context);
#else
    err = dev->req_irq(dev);
#endif
    if (err)
{
      DPRINTK(KERN_DEBUG "%s: can't request irq from device!\n", DEVICE_NAME);
      return err;
    }

    // open the device itself
    err = dev->device_open(dev, dev->wBTR0BTR1, dev->ucCANMsgType, dev->ucListenOnly);
    if (err)
    {
      DPRINTK(KERN_DEBUG "%s: can't open device hardware itself!\n", DEVICE_NAME);
      return err;
    }
  }

  dev->nOpenPaths++;

#ifdef XENOMAI
  /* enable IRQ interrupts */
  err = rtdm_irq_enable(&ctx->irq_handle);
  if (err)
  {
    DPRINTK(KERN_DEBUG "%s: can't enable interrupt\n", DEVICE_NAME);
    return err;
  }
#endif

  DPRINTK(KERN_DEBUG "%s: pcan_open_path() is OK\n", DEVICE_NAME);

  return 0;
}

//----------------------------------------------------------------------------
// find the pcandev to a given minor number
// returns NULL pointer in the case of no success
struct pcandev* pcan_search_dev(int minor)
{
  struct pcandev *dev = (struct pcandev *)NULL;
  struct list_head *ptr;

  DPRINTK(KERN_DEBUG "%s: pcan_search_dev(), minor = %d.\n", DEVICE_NAME, minor);

  if (list_empty(&pcan_drv.devices))
  {
    DPRINTK(KERN_DEBUG "%s: no devices to select from!\n", DEVICE_NAME);
    return NULL;
  }

  // loop through my devices
  for (ptr = pcan_drv.devices.next; ptr != &pcan_drv.devices; ptr = ptr->next)
  {
    dev = (struct pcandev *)ptr;

    if (dev->nMinor == minor)
      break;
  }

  if (dev->nMinor != minor)
  {
    DPRINTK(KERN_DEBUG "%s: didn't find my minor!\n", DEVICE_NAME);
    return NULL;
  }

  return dev;
  }

#ifdef XENOMAI
//----------------------------------------------------------------------------
// is called when the path is opened
int pcan_open_rt(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, int oflags)
{
  struct pcandev *dev;
  int err = 0;
  struct pcanctx_rt *ctx;
  int _minor = context->device->device_id;

  DPRINTK(KERN_DEBUG "%s: pcan_open_rt(), minor = %d.\n", DEVICE_NAME, _minor);

  ctx = (struct pcanctx_rt *)context->dev_private;

  /* IPC initialisation - cannot fail with used parameters */
  rtdm_lock_init(&ctx->lock);
  rtdm_event_init(&ctx->in_event, 0);
  rtdm_event_init(&ctx->out_event, 1);
  rtdm_event_init(&ctx->empty_event, 1);
  rtdm_mutex_init(&ctx->in_lock);
  rtdm_mutex_init(&ctx->out_lock);


  dev = pcan_search_dev(_minor);
  if (!dev)
    return -ENODEV;

  ctx->dev = dev;
  ctx->nReadRest = 0;
  ctx->nTotalReadCount = 0;
  ctx->pcReadPointer = ctx->pcReadBuffer;
  ctx->nWriteCount = 0;
  ctx->pcWritePointer = ctx->pcWriteBuffer;

  if (dev->wType == HW_PCI)
    ctx->irq = dev->port.pci.wIrq;
  if (dev->wType == HW_ISA_SJA)
    ctx->irq = dev->port.isa.wIrq;
  if (dev->wType == HW_DONGLE_SJA || dev->wType == HW_DONGLE_SJA_EPP)
    ctx->irq = dev->port.dng.wIrq;

  err = pcan_open_path(dev);

  return err;
}

#else /* no XENOMAI follows */

//----------------------------------------------------------------------------
// is called when the path is opened
static int pcan_open(struct inode *inode, struct file *filep)
{
  struct pcandev *dev;
  int err = 0;
  struct fileobj *fobj = (struct fileobj *)NULL;
  int _minor = minor(inode->i_rdev);

  DPRINTK(KERN_DEBUG "%s: pcan_open(), minor = %d.\n", DEVICE_NAME, _minor);

  dev = pcan_search_dev(_minor);
  if (!dev)
    return -ENODEV;

  // create file object
  fobj = kmalloc(sizeof(struct fileobj), GFP_KERNEL);
  if (!fobj)
  {
    DPRINTK(KERN_DEBUG "%s: can't allocate kernel memory!\n", DEVICE_NAME);
    return -ENOMEM;
  }

  // fill file object and init read and write method buffers
  fobj->dev = dev;
  if (filep->f_mode & FMODE_READ)
  {
    fobj->nReadRest = 0;
    fobj->nTotalReadCount = 0;
    fobj->pcReadPointer = fobj->pcReadBuffer;
  }

  if (filep->f_mode & FMODE_WRITE)
  {
    fobj->nWriteCount = 0;
    fobj->pcWritePointer = fobj->pcWriteBuffer;
  }

  filep->private_data = (void *)fobj;

  err = pcan_open_path(dev);

  if (err && fobj) /* oops */
      kfree(fobj);

  return err;
}

#endif /* XENOMAI */

//----------------------------------------------------------------------------
// is called by pcan_release() and pcan_netdev_close()
void pcan_release_path(struct pcandev *dev)
{
  DPRINTK(KERN_DEBUG "%s: pcan_release_path, minor = %d, path = %d.\n",
    DEVICE_NAME, dev->nMinor, dev->nOpenPaths);

    // if it's the last release: init the chip for non-intrusive operation
  if (dev->nOpenPaths > 1)
      dev->nOpenPaths--;
  else
  {
    // wait until fifo is empty or MAX_WAIT_UNTIL_CLOSE time is elapsed
    wait_until_fifo_empty(dev, MAX_WAIT_UNTIL_CLOSE);

    // release the device itself
    dev->device_release(dev);

    dev->release(dev);
    dev->nOpenPaths = 0;

    // release the interface depended irq, after this 'dev' is not valid
    dev->free_irq(dev);
  }
}


//----------------------------------------------------------------------------
// is called when the path is closed
#ifdef XENOMAI
int pcan_close_rt(struct rtdm_dev_context *context, rtdm_user_info_t *user_info)
{
  struct pcandev *dev;
  struct pcanctx_rt *ctx;
  //rtdm_lockctx_t lock_ctx;
  
  DPRINTK(KERN_DEBUG "%s: pcan_close_rt()\n", DEVICE_NAME);

  ctx = (struct pcanctx_rt *)context->dev_private;

  dev = ctx->dev;

  //rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);

  //if (pcan_fifo_empty(&dev->writeFifo))
    //rtdm_event_clear(&ctx->empty_event);

  //rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);

  pcan_release_path(dev);

  rtdm_irq_free(&ctx->irq_handle);

  rtdm_event_destroy(&ctx->in_event);
  rtdm_event_destroy(&ctx->out_event);
  rtdm_event_destroy(&ctx->empty_event);

  rtdm_mutex_destroy(&ctx->in_lock);
  rtdm_mutex_destroy(&ctx->out_lock);

  return 0;
}

#else

static int pcan_release(struct inode *inode, struct file *filep)
{
  struct fileobj *fobj = (struct fileobj *)filep->private_data;
  struct pcandev *dev;

  DPRINTK(KERN_DEBUG "%s: pcan_release()\n", DEVICE_NAME);

  // free the associated irq and allocated memory
  if (fobj && fobj->dev)
  {
    dev = fobj->dev;

    pcan_release_path(dev);

    kfree(fobj);
  }
  return 0;
}

#endif

//----------------------------------------------------------------------------
// is called at user ioctl() with cmd = PCAN_READ_MSG
#ifdef XENOMAI
int pcan_ioctl_read_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx,
                              TPCANRdMsg *usr)
#else
static int pcan_ioctl_read(struct file *filep, struct pcandev *dev, TPCANRdMsg *usr)
#endif
{
  int err = 0;
  TPCANRdMsg msg;

#ifdef XENOMAI
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_READ_MSG)\n", DEVICE_NAME);

  dev = ctx->dev;

  // sleep until fifo is not empty
  err = rtdm_event_wait(&ctx->in_event);
#else
  // DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_READ_MSG)\n", DEVICE_NAME);

  // support nonblocking read if requested
  if ((filep->f_flags & O_NONBLOCK) && (!pcan_fifo_empty(&dev->readFifo)))
    return -EAGAIN;

  // sleep until data are available
  err = wait_event_interruptible(dev->read_queue, (pcan_fifo_empty(&dev->readFifo)));
#endif
  if (err)
    goto fail;

  // if the device is plugged out
  if (!dev->ucPhysicallyInstalled)
    return -ENODEV;

#ifdef XENOMAI
  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#endif

  // get data out of fifo
  err = pcan_fifo_get(&dev->readFifo, (void *)&msg);

#ifdef XENOMAI
  if (pcan_fifo_empty(&dev->readFifo))
    rtdm_event_signal(&ctx->in_event);
#endif

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif

  if (err)
    goto fail;

#ifdef XENOMAI
  if (user_info)
  {
    if (!rtdm_rw_user_ok(user_info, usr, sizeof(msg)) ||
         rtdm_copy_to_user(user_info, usr, &msg, sizeof(msg)))
      err = -EFAULT;
  }
  else
    memcpy(usr, &msg, sizeof(msg));
#else
  if (copy_to_user(usr, &msg, sizeof(*usr)))
    err = -EFAULT;
#endif

  fail:
  return err;
}

//----------------------------------------------------------------------------
// is called at user ioctl() with cmd = PCAN_WRITE_MSG 
#ifdef XENOMAI
int pcan_ioctl_write_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx,
                               TPCANMsg *usr)
#else
static int pcan_ioctl_write(struct file *filep, struct pcandev *dev, TPCANMsg *usr)
#endif
{
  int err = 0;
  TPCANMsg msg;

#ifdef XENOMAI
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_WRITE_MSG)\n", DEVICE_NAME);

  dev = ctx->dev;

  // sleep until space is available
  err = rtdm_event_wait(&ctx->out_event);
#else
  // DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_WRITE_MSG)\n", DEVICE_NAME);

  // support nonblocking write if requested
  if ((filep->f_flags & O_NONBLOCK) && (!pcan_fifo_near_full(&dev->writeFifo)) && (!atomic_read(&dev->DataSendReady)))
    return -EAGAIN;

  // sleep until space is available
  err = wait_event_interruptible(dev->write_queue,
             (pcan_fifo_near_full(&dev->writeFifo) || atomic_read(&dev->DataSendReady)));
#endif
  if (err)
    goto fail;

  // if the device is plugged out
  if (!dev->ucPhysicallyInstalled)
    return -ENODEV;

  // get from user space
#ifdef XENOMAI
  if (user_info)
  {
    if (!rtdm_read_user_ok(user_info, usr, sizeof(msg)) || 
         rtdm_copy_from_user(user_info, &msg, usr, sizeof(msg)))
    {
      err = -EFAULT;
      goto fail;
    }
  }
  else
    memcpy(&msg, usr, sizeof(msg));
#else
  if (copy_from_user(&msg, usr, sizeof(msg)))
  {
    err = -EFAULT;
    goto fail;
  }
#endif

  // filter extended data if initialized to standard only
  if (!(dev->bExtended) && ((msg.MSGTYPE & MSGTYPE_EXTENDED) || (msg.ID > 2047)))
  {
    err = -EINVAL;
    goto fail;
  }

#ifdef XENOMAI
  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#endif

  // put data into fifo
  err = pcan_fifo_put(&dev->writeFifo, &msg);

#ifdef XENOMAI
  if (pcan_fifo_near_full(&dev->writeFifo) || atomic_read(&dev->DataSendReady))
    rtdm_event_signal(&ctx->out_event);
#endif

  if (!err)
  {
    // pull new transmission
    if (atomic_read(&dev->DataSendReady))
    {
      atomic_set(&dev->DataSendReady, 0);
      if ((err = dev->device_write(dev)))
        atomic_set(&dev->DataSendReady, 1);
    }
    else
    {
      // DPRINTK(KERN_DEBUG "%s: pushed %d item into Fifo\n", DEVICE_NAME, dev->writeFifo.nStored);
    }
  }

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif

  fail:
  return err;
}

//----------------------------------------------------------------------------
// is called at user ioctl() with cmd = PCAN_GET_EXT_STATUS 
#ifdef XENOMAI
int pcan_ioctl_extended_status_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx,
                         TPEXTENDEDSTATUS *status)
#else
static int pcan_ioctl_extended_status(struct pcandev *dev, TPEXTENDEDSTATUS *status)
#endif
{
  int err = 0;
  TPEXTENDEDSTATUS local;

#ifdef XENOMAI
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_GET_EXT_STATUS)\n", DEVICE_NAME);

  dev = ctx->dev;

  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#else
  DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_GET_EXT_STATUS)\n", DEVICE_NAME);
#endif

  local.wErrorFlag = dev->wCANStatus;

  local.nPendingReads = dev->readFifo.nStored;

  // get infos for friends of polling operation
  if (!pcan_fifo_empty(&dev->readFifo))
    local.wErrorFlag |= CAN_ERR_QRCVEMPTY;

  local.nPendingWrites = (dev->writeFifo.nStored + ((atomic_read(&dev->DataSendReady)) ? 0 : 1));

  if (!pcan_fifo_near_full(&dev->writeFifo))
    local.wErrorFlag |= CAN_ERR_QXMTFULL;

  local.nLastError = dev->nLastError;

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);

  if (user_info)
  {
    if (!rtdm_rw_user_ok(user_info, status, sizeof(local)) || 
         rtdm_copy_to_user(user_info, status, &local, sizeof(local)))
    {
      err = -EFAULT;
      goto fail;
    }
  }
  else
    memcpy(status, &local, sizeof(local));
#else
  if (copy_to_user(status, &local, sizeof(local)))
  {
    err = -EFAULT;
    goto fail;
  }
#endif

  dev->wCANStatus = 0;
  dev->nLastError = 0;

  fail:
  return err;
}

//----------------------------------------------------------------------------
// is called at user ioctl() with cmd = PCAN_GET_STATUS
#ifdef XENOMAI
int pcan_ioctl_status_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx,
                  TPSTATUS *status)
#else
static int pcan_ioctl_status(struct pcandev *dev, TPSTATUS *status)
#endif
{
  int err = 0;
  TPSTATUS local;

#ifdef XENOMAI
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_GET_STATUS)\n", DEVICE_NAME);

  dev = ctx->dev;

  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#else
  DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_GET_STATUS)\n", DEVICE_NAME);
#endif

  local.wErrorFlag = dev->wCANStatus;

  // get infos for friends of polling operation
  if (!pcan_fifo_empty(&dev->readFifo))
    local.wErrorFlag |= CAN_ERR_QRCVEMPTY;

  if (!pcan_fifo_near_full(&dev->writeFifo))
    local.wErrorFlag |= CAN_ERR_QXMTFULL;

  local.nLastError = dev->nLastError;

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);

  if (user_info)
  {
    if ( !rtdm_rw_user_ok(user_info, status, sizeof(local)) ||
          rtdm_copy_to_user(user_info, status, &local, sizeof(local)))
    {
      err = -EFAULT;
      goto fail;
    }
  }
  else
    memcpy(status, &local, sizeof(local));
#else
  if (copy_to_user(status, &local, sizeof(local)))
  {
    err = -EFAULT;
    goto fail;
  }
#endif

  dev->wCANStatus = 0;
  dev->nLastError = 0;

  fail:
  return err;
}

//----------------------------------------------------------------------------
// is called at user ioctl() with cmd = PCAN_DIAG 
#ifdef XENOMAI
int pcan_ioctl_diag_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx,
                              TPDIAG *diag)
#else
static int pcan_ioctl_diag(struct pcandev *dev, TPDIAG *diag)
#endif
{
  int err = 0;
  TPDIAG local;

#ifdef XENOMAI
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_DIAG)\n", DEVICE_NAME);

  dev = ctx->dev;

  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#else
  DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_DIAG)\n", DEVICE_NAME);
#endif

  local.wType           = dev->wType;
  switch (dev->wType)
  {
    case HW_ISA_SJA:
      local.dwBase      = dev->port.isa.dwPort;
      local.wIrqLevel   = dev->port.isa.wIrq;
      break;
    case HW_DONGLE_SJA:
    case HW_DONGLE_SJA_EPP:
      local.dwBase      = dev->port.dng.dwPort;
      local.wIrqLevel   = dev->port.dng.wIrq;
      break;
    case HW_PCI:
      local.dwBase      = dev->port.pci.dwPort;
      local.wIrqLevel   = dev->port.pci.wIrq;
      break;
    case HW_USB:
      #ifdef USB_SUPPORT
      local.dwBase    = dev->port.usb.dwSerialNumber;
      local.wIrqLevel = dev->port.usb.ucHardcodedDevNr;
      #endif
      break;
    case HW_PCCARD:
      #ifdef PCCARD_SUPPORT
      local.dwBase      = dev->port.pccard.dwPort;
      local.wIrqLevel   = dev->port.pccard.wIrq;
      #endif
      break;
  }
  local.dwReadCounter   = dev->readFifo.dwTotal;
  local.dwWriteCounter  = dev->writeFifo.dwTotal;
  local.dwIRQcounter    = dev->dwInterruptCounter;
  local.dwErrorCounter  = dev->dwErrorCounter;
  local.wErrorFlag      = dev->wCANStatus;

  // get infos for friends of polling operation
  if (!pcan_fifo_empty(&dev->readFifo))
    local.wErrorFlag |= CAN_ERR_QRCVEMPTY;

  if (!pcan_fifo_near_full(&dev->writeFifo))
    local.wErrorFlag |= CAN_ERR_QXMTFULL;

  local.nLastError      = dev->nLastError;
  local.nOpenPaths      = dev->nOpenPaths;

  strncpy(local.szVersionString, pcan_drv.szVersionString, VERSIONSTRING_LEN);

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);

  if (user_info)
  {
    if (!rtdm_rw_user_ok(user_info, diag, sizeof(local)) || 
         rtdm_copy_to_user(user_info, diag, &local, sizeof(local)))
      err = -EFAULT;
  }
  else
    memcpy(diag, &local, sizeof(local));
#else
  if (copy_to_user(diag, &local, sizeof(local)))
    err = -EFAULT;
#endif

  return err;
}

//----------------------------------------------------------------------------
// is called at user ioctl() with cmd = PCAN_INIT 
#ifdef XENOMAI
int pcan_ioctl_init_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx, TPCANInit *Init)
#else
int pcan_ioctl_init(struct pcandev *dev, TPCANInit *Init)
#endif
{
  int err = 0;
  TPCANInit local;

#ifdef XENOMAI
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_INIT)\n", DEVICE_NAME);

  dev = ctx->dev;

  if (user_info)
  {
    if (!rtdm_read_user_ok(user_info, Init, sizeof(local)) ||
         rtdm_copy_from_user(user_info, &local, Init, sizeof(local)))
    {
      err = -EFAULT;
      goto fail;
    }
  }
  else
    memcpy(&local, Init, sizeof(local));

  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);

  if (pcan_fifo_empty(&dev->writeFifo))
    rtdm_event_clear(&ctx->empty_event);

  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);

  rtdm_event_wait(&ctx->empty_event);
#else
  DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_INIT)\n", DEVICE_NAME);

  if (copy_from_user(&local, Init, sizeof(*Init)))
  {
    err = -EFAULT;
    goto fail;
  }
#endif

  // flush fifo contents
#ifdef XENOMAI
  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#endif
  err = pcan_fifo_reset(&dev->writeFifo);
#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif
  if (err)
    goto fail;

#ifdef XENOMAI
  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#endif
  err = pcan_fifo_reset(&dev->readFifo);
#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif
  if (err)
    goto fail;

  // wait until fifo is empty or MAX_WAIT_UNTIL_CLOSE time is elapsed
  wait_until_fifo_empty(dev, MAX_WAIT_UNTIL_CLOSE);

#ifdef XENOMAI
  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#endif

  // release the device
  dev->device_release(dev);

  // init again
  err = dev->device_open(dev, local.wBTR0BTR1, local.ucCANMsgType, local.ucListenOnly);

  if (!err)
  {
    dev->wBTR0BTR1    = local.wBTR0BTR1;
    dev->ucCANMsgType = local.ucCANMsgType;
    dev->ucListenOnly = local.ucListenOnly;
  }

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif

  fail:
  return err;
}

//----------------------------------------------------------------------------
// get BTR0BTR1 init values
#ifdef XENOMAI
int pcan_ioctl_BTR0BTR1_rt(rtdm_user_info_t *user_info, struct pcanctx_rt *ctx,
                    TPBTR0BTR1 *BTR0BTR1)
#else
static int pcan_ioctl_BTR0BTR1(struct pcandev *dev, TPBTR0BTR1 *BTR0BTR1)
#endif
{
  int err = 0;
  TPBTR0BTR1 local;

#ifdef XENOMAI
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(PCAN_BTR0BTR1)\n", DEVICE_NAME);

  if (user_info)
  {
    if (!rtdm_read_user_ok(user_info, BTR0BTR1, sizeof(local)) ||
         rtdm_copy_from_user(user_info, &local, BTR0BTR1, sizeof(local)))
    {
      err = -EFAULT;
      goto fail;
    }
  }
  else
    memcpy(&local, BTR0BTR1, sizeof(local));

  rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#else
  DPRINTK(KERN_DEBUG "%s: pcan_ioctl(PCAN_BTR0BTR1)\n", DEVICE_NAME);

  if (copy_from_user(&local, BTR0BTR1, sizeof(local)))
  {
    err = -EFAULT;
    goto fail;
  }
#endif

  // this does not influence hardware settings, only BTR0BTR1 values are calculated
  local.wBTR0BTR1 = sja1000_bitrate(local.dwBitRate);

#ifdef XENOMAI
  rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif

  if (!local.wBTR0BTR1)
  {
    err = -EFAULT;
    goto fail;
  }

#ifdef XENOMAI
  if (user_info)
  {
    if (!rtdm_rw_user_ok(user_info, BTR0BTR1, sizeof(local)) ||
         rtdm_copy_to_user(user_info, BTR0BTR1, &local, sizeof(local)))
      err = -EFAULT;
  }
  else
    memcpy(BTR0BTR1, &local, sizeof(local));
#else
  if (copy_to_user(BTR0BTR1, &local, sizeof(*BTR0BTR1)))
    err = -EFAULT;
#endif

  fail:
  return err;
}

//----------------------------------------------------------------------------
// is called at user ioctl() call
#ifdef XENOMAI
int pcan_ioctl_rt(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, int request, void *arg)
{
  int err;
  struct pcanctx_rt *ctx;
  struct pcandev *dev;

  ctx = (struct pcanctx_rt *)context->dev_private;
  dev = ctx->dev;

  // if the device is plugged out
  if (!dev->ucPhysicallyInstalled)
    return -ENODEV;

  switch(request)
  {
    case PCAN_READ_MSG:
      err = pcan_ioctl_read_rt(user_info, ctx, (TPCANRdMsg *)arg); // support blocking and nonblocking IO
      break;
    case PCAN_WRITE_MSG:
      err = pcan_ioctl_write_rt(user_info, ctx, (TPCANMsg *)arg);  // support blocking and nonblocking IO
      break;
    case PCAN_GET_EXT_STATUS:
      err = pcan_ioctl_extended_status_rt(user_info, ctx, (TPEXTENDEDSTATUS *)arg);
     break;
    case PCAN_GET_STATUS:
      err = pcan_ioctl_status_rt(user_info, ctx, (TPSTATUS *)arg);
      break;
    case PCAN_DIAG:
      err = pcan_ioctl_diag_rt(user_info, ctx, (TPDIAG *)arg);
      break;
    case PCAN_INIT: 
      err = pcan_ioctl_init_rt(user_info, ctx, (TPCANInit *)arg);
      break;
    case PCAN_BTR0BTR1:
      err = pcan_ioctl_BTR0BTR1_rt(user_info, ctx, (TPBTR0BTR1 *)arg);
      break;

    default:
      DPRINTK(KERN_DEBUG "%s: pcan_ioctl_rt(%d)\n", DEVICE_NAME, request);
      err = -ENOTTY;
      break;
  }
  
  DPRINTK(KERN_DEBUG "%s: pcan_ioctl()_rt = %d\n", DEVICE_NAME, err);
  return err;
}

#else // XENOMAI

static int pcan_ioctl(struct inode *inode, struct file *filep, unsigned int cmd, unsigned long arg)
{
  int err;
  struct fileobj *fobj = (struct fileobj *)filep->private_data;
  struct pcandev *dev  = fobj->dev;

  // if the device is plugged out
  if (!dev->ucPhysicallyInstalled)
    return -ENODEV;

  switch(cmd)
  {
    case PCAN_READ_MSG:
      err = pcan_ioctl_read(filep, dev, (TPCANRdMsg *)arg); // support blocking and nonblocking IO
      break;
    case PCAN_WRITE_MSG:
      err = pcan_ioctl_write(filep, dev, (TPCANMsg *)arg);  // support blocking and nonblocking IO
      break;
    case PCAN_GET_EXT_STATUS:
      err = pcan_ioctl_extended_status(dev, (TPEXTENDEDSTATUS *)arg);
      break;
    case PCAN_GET_STATUS:
      err = pcan_ioctl_status(dev, (TPSTATUS *)arg);
      break;
    case PCAN_DIAG:
      err = pcan_ioctl_diag(dev, (TPDIAG *)arg);
      break;
    case PCAN_INIT: 
      err = pcan_ioctl_init(dev, (TPCANInit *)arg);
      break;
    case PCAN_BTR0BTR1:
      err = pcan_ioctl_BTR0BTR1(dev, (TPBTR0BTR1 *)arg);
      break;

    default:
      DPRINTK(KERN_DEBUG "%s: pcan_ioctl(%d)\n", DEVICE_NAME, cmd);  
      err = -ENOTTY;
      break;
  }

  // DPRINTK(KERN_DEBUG "%s: pcan_ioctl() = %d\n", DEVICE_NAME, err);
  return err;
}
#endif // XENOMAI

//----------------------------------------------------------------------------
// is called when read from the path
#ifdef XENOMAI
ssize_t pcan_read_rt(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, void *buf, size_t count)
#else
static ssize_t pcan_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
#endif
{
  int    err;
  int    len = 0;
  TPCANRdMsg msg;
#ifdef XENOMAI
  struct pcanctx_rt  *ctx;
  struct pcandev *dev;
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_read_rt().\n", DEVICE_NAME);

  ctx = (struct pcanctx_rt *)context->dev_private;

  dev = ctx->dev;

  err = rtdm_mutex_lock(&ctx->in_lock);
  if (err)
    return err;
#else
  struct fileobj *fobj = (struct fileobj *)filep->private_data;
  struct pcandev *dev  = fobj->dev; 

  // DPRINTK(KERN_DEBUG "%s: pcan_read().\n", DEVICE_NAME);
#endif

  // if the device is plugged out
  if (!dev->ucPhysicallyInstalled)
    return -ENODEV;

#ifdef XENOMAI
  if (ctx->nReadRest <= 0)
  {
    if (!pcan_fifo_empty(&dev->readFifo))
    {
      err = rtdm_event_wait(&ctx->in_event);
      if (err)
        return err;
    }
#else
  if (fobj->nReadRest <= 0)
  {
    // support nonblocking read if requested
    if ((filep->f_flags & O_NONBLOCK) && (!pcan_fifo_empty(&dev->readFifo)))
      return -EAGAIN;

    // sleep until data are available
    err = wait_event_interruptible(dev->read_queue, (pcan_fifo_empty(&dev->readFifo)));
    if (err)
      return err;
#endif

    // if the device is plugged out
    if (!dev->ucPhysicallyInstalled)
      return -ENODEV;

#ifdef XENOMAI
    rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
#endif

    // get read data out of FIFO
    err = pcan_fifo_get(&dev->readFifo, &msg);

#ifdef XENOMAI
    rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
#endif

    if (err)
      return err;

#ifdef XENOMAI
    ctx->nReadRest = pcan_make_output(ctx->pcReadBuffer, &msg);
    ctx->pcReadPointer = ctx->pcReadBuffer;
  }

  // give the data to the user
  if (count > ctx->nReadRest)
  {
    // put all data to user
    len = ctx->nReadRest;
    ctx->nReadRest = 0;
    if (user_info)
    {
      if (!rtdm_rw_user_ok(user_info, buf, len) ||
           rtdm_copy_to_user(user_info, buf, ctx->pcReadPointer, len))
        return -EFAULT;
    }
    else
      memcpy(buf, ctx->pcReadPointer, len);

    ctx->pcReadPointer = ctx->pcReadBuffer;
  }
  else
  {
    // put only partial data to user
    len = count;
    ctx->nReadRest -= count;
    if (user_info)
    {
      if (!rtdm_rw_user_ok(user_info, buf, len) || rtdm_copy_to_user(user_info, buf, ctx->pcReadPointer, len))
        return -EFAULT;
    }
    else
      memcpy(buf, ctx->pcReadPointer, len);
    ctx->pcReadPointer = (u8 *)((u8*)ctx->pcReadPointer + len);
  }

  ctx->nTotalReadCount += len;

  rtdm_mutex_unlock(&ctx->in_lock);

  DPRINTK(KERN_DEBUG "%s: pcan_read_rt() is OK\n", DEVICE_NAME);
#else
    fobj->nReadRest = pcan_make_output(fobj->pcReadBuffer, &msg);
    fobj->pcReadPointer = fobj->pcReadBuffer;
  }

  // give the data to the user
  if (count > fobj->nReadRest)
  {
    // put all data to user
    len = fobj->nReadRest;
    fobj->nReadRest = 0;
    if (copy_to_user(buf, fobj->pcReadPointer, len))
      return -EFAULT;
    fobj->pcReadPointer = fobj->pcReadBuffer; 
  }
  else
  {
    // put only partial data to user
    len = count;
    fobj->nReadRest -= count;
    if (copy_to_user(buf, fobj->pcReadPointer, len))
      return -EFAULT;
    fobj->pcReadPointer = (u8 *)((u8*)fobj->pcReadPointer + len);
  }

  *f_pos += len;
  fobj->nTotalReadCount += len;

  // DPRINTK(KERN_DEBUG "%s: pcan_read() is OK\n", DEVICE_NAME);
#endif

  return len;
}

//----------------------------------------------------------------------------
// is called when written to the path
#ifdef XENOMAI
ssize_t pcan_write_rt(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, const void *buf, size_t count)
#else
static ssize_t pcan_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
#endif
{
  int err = 0; 
  u32 dwRest;
  u8 *ptr;

#ifdef XENOMAI
  struct pcanctx_rt *ctx;
  struct pcandev *dev; 
  rtdm_lockctx_t lock_ctx;

  DPRINTK(KERN_DEBUG "%s: pcan_write_rt().\n", DEVICE_NAME);

  ctx = (struct pcanctx_rt *)context->dev_private;

  dev = ctx->dev;

  err = rtdm_mutex_lock(&ctx->out_lock);
  if (err)
    return err;
#else
  struct fileobj *fobj = (struct fileobj *)filep->private_data;
  struct pcandev *dev  = fobj->dev;

  // DPRINTK(KERN_DEBUG "%s: pcan_write().\n", DEVICE_NAME);
#endif

  // if the device is plugged out
  if (!dev->ucPhysicallyInstalled)
    return -ENODEV;

  // calculate remaining buffer space
#ifdef XENOMAI
  dwRest = WRITEBUFFER_SIZE - (ctx->pcWritePointer - ctx->pcWriteBuffer); // nRest > 0!
  count = (count > dwRest) ? dwRest : count;

  if (user_info)
  {
    if (!rtdm_read_user_ok(user_info, buf, count) ||
         rtdm_copy_from_user(user_info, ctx->pcWritePointer, buf, count))
      return -EFAULT;
  }
  else
    memcpy(ctx->pcWritePointer, buf, count);

  // adjust working pointer to end
  ctx->pcWritePointer += count;
#else
  dwRest = WRITEBUFFER_SIZE - (fobj->pcWritePointer - fobj->pcWriteBuffer); // nRest > 0!
  count  = (count > dwRest) ? dwRest : count;

  if (copy_from_user(fobj->pcWritePointer, buf, count))
    return -EFAULT;

  // adjust working pointer to end
  fobj->pcWritePointer += count;
#endif

  // iterate search blocks ending with '\n'
  while (1)
  {
    #ifdef XENOMAI
    // search first '\n' from begin of buffer
    ptr = ctx->pcWriteBuffer; 
    while ((*ptr != '\n') && (ptr < ctx->pcWritePointer))
      ptr++;

    // parse input when a CR was found
    if ((*ptr == '\n') && (ptr < ctx->pcWritePointer))
    {
      u32 amount = (u32)(ctx->pcWritePointer - ptr - 1);
      u32 offset = (u32)(ptr - ctx->pcWriteBuffer + 1);
    #else
    // search first '\n' from begin of buffer
    ptr = fobj->pcWriteBuffer; 
    while ((*ptr != '\n') && (ptr < fobj->pcWritePointer))
      ptr++;

    // parse input when a CR was found
    if ((*ptr == '\n') && (ptr < fobj->pcWritePointer))
    {
      u32 amount = (u32)(fobj->pcWritePointer - ptr - 1);
      u32 offset = (u32)(ptr - fobj->pcWriteBuffer + 1);
    #endif
      if ((amount > WRITEBUFFER_SIZE) || (offset > WRITEBUFFER_SIZE))
      {
        #ifdef __LP64__
        #warning "Compiling for __LP64__"
        printk(KERN_ERR "%s: fault in pcan_write() %ld %u, %u: \n", DEVICE_NAME, count, amount, offset);
        #else
        printk(KERN_ERR "%s: fault in pcan_write() %d %u, %u: \n", DEVICE_NAME, count, amount, offset);
        #endif
        return -EFAULT;
      }

      #ifdef XENOMAI
      if (pcan_parse_input_idle(ctx->pcWriteBuffer))
      {
        TPCANMsg msg;

        if (pcan_parse_input_message(ctx->pcWriteBuffer, &msg))
        {
          TPCANInit Init;

          if ((err = pcan_parse_input_init(ctx->pcWriteBuffer, &Init)))
      #else
      if (pcan_parse_input_idle(fobj->pcWriteBuffer))
      {
        TPCANMsg msg;

        if (pcan_parse_input_message(fobj->pcWriteBuffer, &msg))
        {
          TPCANInit Init;

          if ((err = pcan_parse_input_init(fobj->pcWriteBuffer, &Init)))
      #endif
            return err;
          else
          {
          #if 0
            DPRINTK(KERN_DEBUG "%s: ***** Init 0x%04x 0x%02x 0x%02x\n", DEVICE_NAME,
              Init.wBTR0BTR1, Init.ucCANMsgType, Init.ucListenOnly);
          #endif

            // init the associated chip and the fifos again with new parameters
            err = dev->device_open(dev, Init.wBTR0BTR1, Init.ucCANMsgType, Init.ucListenOnly);
            if (err)
              return err;
            else
            {
               dev->wBTR0BTR1    = Init.wBTR0BTR1;
               dev->ucCANMsgType = Init.ucCANMsgType;
               dev->ucListenOnly = Init.ucListenOnly;
            }

            #ifdef XENOMAI
            rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
            #endif
            err = pcan_fifo_reset(&dev->writeFifo);
            #ifdef XENOMAI
            rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
            #endif
            if (err)
              return err;

            #ifdef XENOMAI
            rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
            #endif
            err = pcan_fifo_reset(&dev->readFifo);
            #ifdef XENOMAI
            rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
            #endif
            if (err)
              return err;
          }
        }
        else
        {
          #if 0 // ------- print out message, begin -----------
          int i = 0;

          DPRINTK(KERN_DEBUG "%s: *** 0x%08x 0x%02x %d . ",
            DEVICE_NAME, msg.ID, Message.MSGTYPE, msg.LEN);

          while (i++ < Message.LEN)
            DPRINTK(KERN_DEBUG "0x%02x ", msg.DATA[i]);

          DPRINTK(KERN_DEBUG " ***\n");
          #endif // ------- print out message, end ------------

          #ifdef XENOMAI
          // sleep until space is available
          if (!pcan_fifo_near_full(&dev->writeFifo) && !atomic_read(&dev->DataSendReady))
          {
             err = rtdm_event_wait(&ctx->out_event);
            if (err)
              return err;
          }
          #else
          // support nonblocking write if requested
          if ((filep->f_flags & O_NONBLOCK) && (!pcan_fifo_near_full(&dev->writeFifo)) && (!atomic_read(&dev->DataSendReady)))
            return -EAGAIN;

          // sleep until space is available
          err = wait_event_interruptible(dev->write_queue,
                     (pcan_fifo_near_full(&dev->writeFifo) || atomic_read(&dev->DataSendReady)));
          #endif
          if (err)
            return err;

          // if the device is plugged out
          if (!dev->ucPhysicallyInstalled)
            return -ENODEV;

          // filter extended data if initialized to standard only
          if (!(dev->bExtended) && ((msg.MSGTYPE & MSGTYPE_EXTENDED) || (msg.ID > 2047)))
            return -EINVAL;

          #ifdef XENOMAI
          rtdm_lock_get_irqsave(&ctx->lock, lock_ctx);
          #endif
          err = pcan_fifo_put(&dev->writeFifo, &msg);
          #ifdef XENOMAI
          rtdm_lock_put_irqrestore(&ctx->lock, lock_ctx);
          #endif
          if (err)
            return err;

          // pull new transmission
          if (atomic_read(&dev->DataSendReady))
          {
            atomic_set(&dev->DataSendReady, 0);
            if ((err = dev->device_write(dev)))
            {
              atomic_set(&dev->DataSendReady, 1);
              return err;
            }
          }
          else
          {
            // DPRINTK(KERN_DEBUG "%s: pushed %d item into Fifo\n", DEVICE_NAME, dev->writeFifo.nStored);
          }
        }
      }

      // move rest of amount data in buffer offset steps to left
      #ifdef XENOMAI
      memmove(ctx->pcWriteBuffer, ptr + 1, amount);
      ctx->pcWritePointer -= offset;
      #else
      memmove(fobj->pcWriteBuffer, ptr + 1, amount);
      fobj->pcWritePointer -= offset;
      #endif
    }
    else
      break; // no CR found
  }

#ifdef XENOMAI
  if (ctx->pcWritePointer >= (ctx->pcWriteBuffer + WRITEBUFFER_SIZE))
  {
    ctx->pcWritePointer = ctx->pcWriteBuffer; // reject all
    return -EFAULT;
  }

  rtdm_mutex_unlock(&ctx->out_lock);

  DPRINTK(KERN_DEBUG "%s: pcan_write_rt() is OK\n", DEVICE_NAME);

  return count;
#else
  if (fobj->pcWritePointer >= (fobj->pcWriteBuffer + WRITEBUFFER_SIZE))
  {
    fobj->pcWritePointer = fobj->pcWriteBuffer; // reject all
    return -EFAULT;
  }

  // DPRINTK(KERN_DEBUG "%s: pcan_write() is OK\n", DEVICE_NAME);

  return count;
#endif
}

#ifndef XENOMAI
//----------------------------------------------------------------------------
// is called at poll or select
static unsigned int pcan_poll(struct file *filep, poll_table *wait)
{
  struct fileobj *fobj = (struct fileobj *)filep->private_data;
  struct pcandev *dev  = fobj->dev;
  unsigned int mask = 0;

  poll_wait(filep, &dev->read_queue,  wait);
  poll_wait(filep, &dev->write_queue, wait);

  // return on ops that could be performed without blocking
  if (pcan_fifo_empty(&dev->readFifo))      mask |= (POLLIN  | POLLRDNORM);
  if (pcan_fifo_near_full(&dev->writeFifo)) mask |= (POLLOUT | POLLWRNORM);

  return mask;
}
#endif

//****************************************************************************
// GLOBALS

//----------------------------------------------------------------------------
// this structure is used in init_module(void)
#ifdef XENOMAI
struct rtdm_device pcandev_rt =
{
    struct_version:     RTDM_DEVICE_STRUCT_VER,

    device_flags:       RTDM_NAMED_DEVICE,
    context_size:       sizeof(struct pcanctx_rt),
    device_name:        "",

    open_rt:            NULL,
    open_nrt:           pcan_open_rt,

    ops: {
        close_rt:       NULL,
        close_nrt:      pcan_close_rt,

        ioctl_rt:       pcan_ioctl_rt,
        ioctl_nrt:      pcan_ioctl_rt,

        read_rt:        pcan_read_rt,
        read_nrt:       NULL,

        write_rt:       pcan_write_rt,
        write_nrt:      NULL,

        recvmsg_rt:     NULL,
        recvmsg_nrt:    NULL,

        sendmsg_rt:     NULL,
        sendmsg_nrt:    NULL,
    },

    device_class:       RTDM_CLASS_CAN,
    driver_name:        "pcan_driver",
    provider_name:      "PEAK System-Technik GmbH",
    proc_name:          pcandev_rt.device_name,
};
#else
struct file_operations pcan_fops =
{
  // marrs:  added owner, which is used to implement a use count that disallows
  //         rmmod calls when the driver is still in use (as suggested by
  //         Duncan Sands on the linux-kernel mailinglist)

  owner:      THIS_MODULE,
  open:       pcan_open,
  release:    pcan_release,
  read:       pcan_read,
  write:      pcan_write,
  ioctl:      pcan_ioctl,
  poll:       pcan_poll,
};
#endif

// end of file
