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
// Contributions: Philipp Baer (philipp.baer@informatik.uni-ulm.de)
//****************************************************************************

//****************************************************************************
//
// all parts of the isa hardware for pcan-isa devices
//
// $Id: pcan_isa.c 447 2007-01-28 14:05:50Z khitschler $
//
//****************************************************************************

//****************************************************************************
// INCLUDES
#include <src/pcan_common.h>     // must always be the 1st include
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <src/pcan_isa.h>
#include <src/pcan_sja1000.h>

//****************************************************************************
// DEFINES
#define PCAN_ISA_MINOR_BASE 8    // starting point of minors for ISA devices  
#define ISA_PORT_SIZE       0x20 // the address range of the isa-port, enough for PeliCAN mode
#define ISA_DEFAULT_COUNT   2    // count of defaults for init

//****************************************************************************
// GLOBALS

//****************************************************************************
// LOCALS
static u16 isa_ports[] = {0x300, 0x320};  // default values for pcan-isa
static u8  isa_irqs[]  = {10, 5};
static u16 isa_devices = 0;        // the number of accepted isa_devices

//****************************************************************************
// CODE  
static u8 pcan_isa_readreg(struct pcandev *dev, u8 port) // read a register
{
  return inb(dev->port.isa.dwPort + port);
}

static void pcan_isa_writereg(struct pcandev *dev, u8 port, u8 data) // write a register
{
  outb(data, dev->port.isa.dwPort + port);
}

#ifndef XENOMAI
//----------------------------------------------------------------------------
// init shared interrupt linked lists
static inline void init_same_irq_list(struct pcandev *dev)
{
  DPRINTK(KERN_DEBUG "%s: init_same_irq_list(%p)\n", DEVICE_NAME, dev);
  
  INIT_LIST_HEAD(&dev->port.isa.anchor.same_irq_items); // init to no content
  dev->port.isa.anchor.same_irq_count  = 0;             // no element in list
  dev->port.isa.anchor.same_irq_active = 0;             // no active element
  dev->port.isa.same.dev               = dev;           // points to me
  dev->port.isa.my_anchor              = NULL;          // point to nowhere
}  

//----------------------------------------------------------------------------
// create lists of devices with the same irq - base for shared irq handling
void pcan_create_isa_shared_irq_lists(void)
{
  struct pcandev *outer_dev;
  struct pcandev *inner_dev;
  struct list_head *outer_ptr, *inner_ptr;
  
  DPRINTK(KERN_DEBUG "%s: pcan_create_isa_shared_irq_lists()\n", DEVICE_NAME);
  
  // loop over all devices for a ISA port with same irq level and not myself
  for (outer_ptr = pcan_drv.devices.next; outer_ptr != &pcan_drv.devices; outer_ptr = outer_ptr->next)
  {
    outer_dev = (struct pcandev *)outer_ptr;  
    
    // if it is a ISA device and still not associated
    if ((outer_dev->wType == HW_ISA_SJA) && (outer_dev->port.isa.my_anchor == NULL))
    {
      outer_dev->port.isa.my_anchor = &outer_dev->port.isa.anchor; // then it is the root of a new list
      outer_dev->port.isa.my_anchor->same_irq_count++;             // I'm the first and - maybe - the only one
      list_add_tail(&outer_dev->port.isa.same.item, &outer_dev->port.isa.my_anchor->same_irq_items); // add to list
      
      DPRINTK(KERN_DEBUG "%s: main Irq=%d, dev=%p, count=%d\n", DEVICE_NAME, 
                         outer_dev->port.isa.wIrq, outer_dev, outer_dev->port.isa.my_anchor->same_irq_count);
          
      // now search for other devices with the same irq
      for (inner_ptr = outer_ptr->next; inner_ptr != &pcan_drv.devices; inner_ptr = inner_ptr->next)
      {
        inner_dev = (struct pcandev *)inner_ptr;  
  
        // if it is a ISA device and the irq level is the same and it is still not associated
        if ((inner_dev->wType == HW_ISA_SJA) && (inner_dev->port.isa.my_anchor == NULL) && (inner_dev->port.isa.wIrq == outer_dev->port.isa.wIrq))
        {
          inner_dev->port.isa.my_anchor = outer_dev->port.isa.my_anchor; // point and associate to the first with the same irq level
          inner_dev->port.isa.my_anchor->same_irq_count++;               // no - there are more
          list_add_tail(&inner_dev->port.isa.same.item, &inner_dev->port.isa.my_anchor->same_irq_items); // add to list
          
          DPRINTK(KERN_DEBUG "%s: sub  Irq=%d, dev=%p, count=%d\n", DEVICE_NAME, 
                             inner_dev->port.isa.wIrq, inner_dev, inner_dev->port.isa.my_anchor->same_irq_count);        
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// only one irq-handler per irq level for ISA shared interrupts
static irqreturn_t IRQHANDLER(pcan_isa_irqhandler, int irq, void *dev_id, struct pt_regs *regs)
{
  // loop the list of irq-handlers for all devices with the same 
  // irq-level until at least all devices are one time idle.
  
  SAME_IRQ_LIST *my_anchor = (SAME_IRQ_LIST *)dev_id;
  struct list_head *ptr;
  struct pcandev *dev;
  int ret        = 0;
  u16 loop_count = 0;
  u16 stop_count = 100;

  // DPRINTK(KERN_DEBUG "%s: pcan_isa_irqhandler(%p)\n", DEVICE_NAME, my_anchor);
  
  // loop over all ISA devices with same irq level
  for (ptr = my_anchor->same_irq_items.next; loop_count < my_anchor->same_irq_count; ptr = ptr->next)
  {
    if (ptr != &my_anchor->same_irq_items)
    {
      dev = ((SAME_IRQ_ITEM *)ptr)->dev;
      
      // DPRINTK(KERN_DEBUG "%s: dev=%p\n", DEVICE_NAME, dev);
      
      if (!IRQHANDLER(sja1000_base_irqhandler, irq, dev, regs))
        loop_count++;
      else
      {
        ret = 1;
        loop_count = 0; // reset, I need at least my_anchor->same_irq_count loops without a pending request
      }
        
      if (!stop_count--)
      {
        printk(KERN_ERR "%s: Too much ISA interrupt load, processing halted!\n", DEVICE_NAME);        
        break;
      }
    }
  }
  
  return IRQ_RETVAL(ret);
}
#endif

#ifdef XENOMAI
static int pcan_isa_req_irq(struct rtdm_dev_context *context)
{
  struct pcanctx_rt *ctx;
  struct pcandev *dev = (struct pcandev *)NULL;
#else
static int pcan_isa_req_irq(struct pcandev *dev)
{
#endif
  int err;
  
  DPRINTK(KERN_DEBUG "%s: pcan_isa_req_irq(%p)\n", DEVICE_NAME, dev);
  
#ifdef XENOMAI
  ctx = (struct pcanctx_rt *)context->dev_private;
  dev = ctx->dev;
#endif
  
  if (dev->wInitStep == 3) // init has finished completly 
  {
    #ifdef XENOMAI
    if ((err = rtdm_irq_request(&ctx->irq_handle, ctx->irq, sja1000_irqhandler_rt,
            0, context->device->proc_name, ctx)))
      return err;
    #else
    dev->port.isa.my_anchor->same_irq_active++;
    
    if (dev->port.isa.my_anchor->same_irq_active == 1) // the first device
    {
      if ((err = request_irq(dev->port.isa.wIrq, pcan_isa_irqhandler, SA_INTERRUPT, "pcan", dev->port.isa.my_anchor)))
        return err;
    }
    #endif
  
    dev->wInitStep++;
  }
    
  return 0;
}

static void pcan_isa_free_irq(struct pcandev *dev)
{
  DPRINTK(KERN_DEBUG "%s: pcan_isa_free_irq(%p)\n", DEVICE_NAME, dev);
  
  if (dev->wInitStep == 4) // irq was installed
  {
    #ifndef XENOMAI
    dev->port.isa.my_anchor->same_irq_active--;
    
    if (!dev->port.isa.my_anchor->same_irq_active) // the last device
      free_irq(dev->port.isa.wIrq, dev->port.isa.my_anchor); 
      
    #endif
    dev->wInitStep--;
  }
}

// release and probe function
static int pcan_isa_cleanup(struct pcandev *dev)
{
  DPRINTK(KERN_DEBUG "%s: pcan_isa_cleanup()\n", DEVICE_NAME);

  switch(dev->wInitStep)
  {
    case 4: pcan_isa_free_irq(dev);
    case 3: isa_devices--;
    case 2:
    case 1: release_region(dev->port.isa.dwPort, ISA_PORT_SIZE);
    case 0: dev->wInitStep = 0;
  }
  
  return 0;
}

// interface depended open and close
static int pcan_isa_open(struct pcandev *dev)
{
  return 0;
}

static int pcan_isa_release(struct pcandev *dev)
{
  return 0;
}

static int  pcan_isa_init(struct pcandev *dev, u32 dwPort, u16 wIrq)
{
  int err = 0;
  
  DPRINTK(KERN_DEBUG "%s: pcan_isa_init(), isa_devices = %d\n", DEVICE_NAME, isa_devices);

  // init process wait queues
  init_waitqueue_head(&dev->read_queue);
  init_waitqueue_head(&dev->write_queue);
  
  // set this before any instructions, fill struct pcandev, part 1  
  dev->wInitStep   = 0;           
  dev->readreg     = pcan_isa_readreg;
  dev->writereg    = pcan_isa_writereg;
  dev->cleanup     = pcan_isa_cleanup;
  dev->req_irq     = pcan_isa_req_irq;
  dev->free_irq    = pcan_isa_free_irq;
  dev->open        = pcan_isa_open;
  dev->release     = pcan_isa_release;
  
  // reject illegal combination
  if ((!dwPort && wIrq) || (dwPort && !wIrq))
    return -EINVAL;

  // a default is requested
  if (!dwPort)
  {
    // there's no default available
    if (isa_devices >= ISA_DEFAULT_COUNT)
      return -ENODEV;
    
    dev->port.isa.dwPort = isa_ports[isa_devices];
    dev->port.isa.wIrq   = isa_irqs[isa_devices];    
  }
  else
  {
    dev->port.isa.dwPort = dwPort;
    dev->port.isa.wIrq   = wIrq;    
  } 
   
  dev->nMinor      = PCAN_ISA_MINOR_BASE + isa_devices;
   
  // request address range reservation
  err = ___request_region(dev->port.isa.dwPort, ISA_PORT_SIZE, DEVICE_NAME);
  if (err)
    return err;

  dev->wInitStep = 1;
  
  dev->wInitStep = 2; 
#ifndef XENOMAI  
  // addition to handle interrupt sharing
  init_same_irq_list(dev);
#endif  
  isa_devices++; 
  dev->wInitStep = 3;
  
  printk(KERN_INFO "%s: isa device minor %d found (io=0x%04x,irq=%d)\n", DEVICE_NAME, dev->nMinor, dev->port.isa.dwPort, dev->port.isa.wIrq);
    
  return 0;
}
 
//----------------------------------------------------------------------------
// create all declared isa devices
int pcan_create_isa_devices(char* type, u32 io, u16 irq)
{
  int    result = 0;
  struct pcandev *dev = NULL;

  // create isa devices
  DPRINTK(KERN_DEBUG "%s: pcan_create_isa_devices(0x%x, %d)\n", DEVICE_NAME, io, irq);
  
  if ((dev = (struct pcandev *)kmalloc(sizeof(struct pcandev), GFP_KERNEL)) == NULL)
     goto fail;

  pcan_soft_init(dev, type, HW_ISA_SJA);
  
  dev->device_open    = sja1000_open;
  dev->device_write   = sja1000_write;
  dev->device_release = sja1000_release;
  #ifdef NETDEV_SUPPORT
  dev->netdevice_write  = sja1000_write_frame;
  #endif

  result = pcan_isa_init(dev, io, irq);
  if (!result)
    result = sja1000_probe(dev);

  if (result)
  {
    dev->cleanup(dev);
    kfree(dev);
    goto fail;
  }
  else
  {
    dev->ucPhysicallyInstalled = 1;
    list_add_tail(&dev->list, &pcan_drv.devices);  // add this device to the list
    pcan_drv.wDeviceCount++;
  }        
  
  fail:
  if (result)
    DPRINTK(KERN_DEBUG "%s: pcan_create_isa_devices() failed!\n", DEVICE_NAME);
  else
    DPRINTK(KERN_DEBUG "%s: pcan_create_isa_devices() is OK\n", DEVICE_NAME);

  return result;
}

