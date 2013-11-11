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
//                Armin Bauer (armin.bauer@desscon.com)
//****************************************************************************

//****************************************************************************
//
// all parts to handle the interface specific parts of pcan-pci
// 
// $Id: pcan_pci.c 451 2007-02-02 10:09:50Z ohartkopp $
//
//****************************************************************************

//****************************************************************************
// INCLUDES
#include <src/pcan_common.h>     // must always be the 1st include
#include <linux/ioport.h>
#include <linux/pci.h>      // all about pci
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <src/pcan_pci.h>
#include <src/pcan_sja1000.h>

//****************************************************************************
// DEFINES
#define PCAN_PCI_MINOR_BASE 0        // the base of all pci device minors

// important PITA registers
#define PITA_ICR         0x00        // interrupt control register
#define PITA_GPIOICR     0x18        // general purpose IO interface control register
#define PITA_MISC        0x1C        // miscellanoes register

#define PCAN_PCI_VENDOR_ID   0x001C  // the PCI device and vendor IDs
#define PCAN_PCI_DEVICE_ID   0x0001

#define PCI_CONFIG_PORT_SIZE 0x1000  // size of the config io-memory
#define PCI_PORT_SIZE        0x0400  // size of a channel io-memory

//****************************************************************************
// GLOBALS
#ifdef UDEV_SUPPORT 
static struct pci_device_id pcan_pci_tbl[] =
{
  {PCAN_PCI_VENDOR_ID, PCAN_PCI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
  {0, }
};

static int pcan_pci_register_driver(struct pci_driver *p_pci_drv)
{
  p_pci_drv->name     = DEVICE_NAME;
  p_pci_drv->id_table = pcan_pci_tbl;
  
  return pci_register_driver(p_pci_drv);
}

static void pcan_pci_unregister_driver(struct pci_driver *p_pci_drv)
{
  pci_unregister_driver(p_pci_drv);
}

MODULE_DEVICE_TABLE(pci, pcan_pci_tbl);
#endif

//****************************************************************************
// LOCALS
static u16 _pci_devices = 0; // count the number of pci devices

//****************************************************************************
// CODE  
static u8 pcan_pci_readreg(struct pcandev *dev, u8 port) // read a register
{
  u32 lPort = port << 2;
  return readb(dev->port.pci.pvVirtPort + lPort);
}

static void pcan_pci_writereg(struct pcandev *dev, u8 port, u8 data) // write a register
{
  u32 lPort = port << 2;
  writeb(data, dev->port.pci.pvVirtPort + lPort);
}

// a special frame around the default irq handler
#ifdef XENOMAI
static int pcan_pci_irqhandler_rt(rtdm_irq_t *irq_context)
{
  struct pcanctx_rt *ctx;
  struct pcandev *dev;
#else
static irqreturn_t IRQHANDLER(pcan_pci_irqhandler, int irq, void *dev_id, struct pt_regs *regs)
{
  struct pcandev *dev = (struct pcandev *)dev_id;
#endif
  u16 PitaICRLow;
  int ret;

#ifdef XENOMAI
  ctx = rtdm_irq_get_arg(irq_context, struct pcanctx_rt);
  dev = ctx->dev;
  
  ret = sja1000_irqhandler_rt(irq_context);
#else
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
  ret = IRQHANDLER(sja1000_irqhandler, irq, dev_id, regs);
  #else
  IRQHANDLER(sja1000_irqhandler, irq, dev_id, regs);
  ret = 0; // supress warning about unused variable
  #endif
#endif

  // select and clear in Pita stored interrupt 
  PitaICRLow = readw(dev->port.pci.pvVirtConfigPort);  // PITA_ICR
  if (dev->props.ucMasterDevice == CHANNEL_SLAVE)
  {
    if (PitaICRLow & 0x0001)
      writew(0x0001, dev->port.pci.pvVirtConfigPort);
  } 
  else // it is CHANNEL_SINGLE or CHANNEL_MASTER
  {
    if (PitaICRLow & 0x0002)
      writew(0x0002, dev->port.pci.pvVirtConfigPort);
  }

  return IRQ_RETVAL(ret);
}

// all about interrupt handling
#ifdef XENOMAI
static int pcan_pci_req_irq(struct rtdm_dev_context *context)
{
  struct pcanctx_rt *ctx;
  struct pcandev *dev = (struct pcandev *)NULL;
#else
static int pcan_pci_req_irq(struct pcandev *dev)
{
#endif
  int err;
  u16 PitaICRHigh;
 
#ifdef XENOMAI
  ctx = (struct pcanctx_rt *)context->dev_private;
  dev = ctx->dev;
#endif

  if (dev->wInitStep == 5)
  {
    #ifdef XENOMAI
    if ((err = rtdm_irq_request(&ctx->irq_handle, ctx->irq, pcan_pci_irqhandler_rt,
            RTDM_IRQTYPE_SHARED | RTDM_IRQTYPE_EDGE, context->device->proc_name, ctx)))
      return err;
    #else
    if ((err = request_irq(dev->port.pci.wIrq, pcan_pci_irqhandler, SA_INTERRUPT | SA_SHIRQ, "pcan", dev)))
      return err;
    #endif
    
    // enable interrupt depending if SLAVE, MASTER, SINGLE
    PitaICRHigh = readw(dev->port.pci.pvVirtConfigPort + PITA_ICR + 2);
    if (dev->props.ucMasterDevice == CHANNEL_SLAVE)
      PitaICRHigh |= 0x0001;
    else
      PitaICRHigh |= 0x0002;  
    writew(PitaICRHigh, dev->port.pci.pvVirtConfigPort + PITA_ICR + 2); 
    
    dev->wInitStep++;
  }
  
  return 0;
}

static void pcan_pci_free_irq(struct pcandev *dev)
{
  u16 PitaICRHigh;
  
  if (dev->wInitStep == 6)
  {
    // disable interrupt, polarity 0
    PitaICRHigh = readw(dev->port.pci.pvVirtConfigPort + PITA_ICR + 2);
    if (dev->props.ucMasterDevice == CHANNEL_SLAVE)
      PitaICRHigh &= ~0x0001;
    else
      PitaICRHigh &= ~0x0002;  
    writew(PitaICRHigh, dev->port.pci.pvVirtConfigPort + PITA_ICR + 2); 
  
    #ifndef XENOMAI
    free_irq(dev->port.pci.wIrq, dev);  
    #endif
    dev->wInitStep--;
  }
}

// release and probe
static int pcan_pci_cleanup(struct pcandev *dev)
{
  DPRINTK(KERN_DEBUG "%s: pcan_pci_cleanup()\n", DEVICE_NAME);

  switch(dev->wInitStep)
  {
    case 6: pcan_pci_free_irq(dev);
    case 5: _pci_devices--; 
    case 4: iounmap(dev->port.pci.pvVirtPort);
    case 3: 
           release_mem_region(dev->port.pci.dwPort, PCI_PORT_SIZE);
    case 2: if (dev->props.ucMasterDevice != CHANNEL_SLAVE)
              iounmap(dev->port.pci.pvVirtConfigPort);  
    case 1: 
            if (dev->props.ucMasterDevice != CHANNEL_SLAVE)
              release_mem_region(dev->port.pci.dwConfigPort, PCI_CONFIG_PORT_SIZE);  
    case 0: dev->wInitStep = 0;
           #ifdef UDEV_SUPPORT
           if (_pci_devices == 0)
             pcan_pci_unregister_driver(&pcan_drv.pci_drv);
           #endif
  }
  
  return 0;
}

// interface depended open and close
static int pcan_pci_open(struct pcandev *dev)
{
  return 0;
}

static int pcan_pci_release(struct pcandev *dev)
{
  return 0;
}

static int  pcan_pci_init(struct pcandev *dev, u32 dwConfigPort, u32 dwPort, u16 wIrq, u8 ucMasterDevice, struct pcandev *master_dev)
{
  DPRINTK(KERN_DEBUG "%s: pcan_pci_init(), _pci_devices = %d\n", DEVICE_NAME, _pci_devices);

  dev->props.ucMasterDevice = ucMasterDevice;
    
  // init process wait queues
  init_waitqueue_head(&dev->read_queue);
  init_waitqueue_head(&dev->write_queue);
    
  // set this before any instructions, fill struct pcandev, part 1 
  dev->wInitStep   = 0;  
  dev->readreg     = pcan_pci_readreg;
  dev->writereg    = pcan_pci_writereg;
  dev->cleanup     = pcan_pci_cleanup;
  dev->req_irq     = pcan_pci_req_irq;
  dev->free_irq    = pcan_pci_free_irq;
  dev->open        = pcan_pci_open;
  dev->release     = pcan_pci_release;
  dev->nMinor      = PCAN_PCI_MINOR_BASE + _pci_devices; 
           
  // fill struct pcandev, part 1
  dev->port.pci.dwPort       = dwPort;
  dev->port.pci.dwConfigPort = dwConfigPort;
  dev->port.pci.wIrq         = wIrq;

  // reject illegal combination
  if (!dwPort || !wIrq)
    return -EINVAL;

  // do it only if the device is channel master
  if (dev->props.ucMasterDevice != CHANNEL_SLAVE)
  {
    if (check_mem_region(dev->port.pci.dwConfigPort, PCI_CONFIG_PORT_SIZE))
      return -EBUSY;
            
    request_mem_region(dev->port.pci.dwConfigPort, PCI_CONFIG_PORT_SIZE, "pcan");
    
    dev->wInitStep = 1;
    
    dev->port.pci.pvVirtConfigPort = ioremap(dwConfigPort, PCI_CONFIG_PORT_SIZE); 
    if (dev->port.pci.pvVirtConfigPort == NULL)
      return -ENODEV;
      
    dev->wInitStep = 2;
    
    // configuration of the PCI chip, part 2
    writew(0x0005, dev->port.pci.pvVirtConfigPort + PITA_GPIOICR + 2);  //set GPIO control register
    
    if (dev->props.ucMasterDevice == CHANNEL_MASTER)
      writeb(0x00, dev->port.pci.pvVirtConfigPort + PITA_GPIOICR); // enable both
    else
      writeb(0x04, dev->port.pci.pvVirtConfigPort + PITA_GPIOICR); // enable single
      
    writeb(0x05, dev->port.pci.pvVirtConfigPort + PITA_MISC + 3);  // toggle reset
    mdelay(5);
    writeb(0x04, dev->port.pci.pvVirtConfigPort + PITA_MISC + 3);  // leave parport mux mode
    wmb();
  }
  else
    dev->port.pci.pvVirtConfigPort = master_dev->port.pci.pvVirtConfigPort;
  
  if (check_mem_region(dev->port.pci.dwPort, PCI_PORT_SIZE))
    return -EBUSY;
            
  request_mem_region(dev->port.pci.dwPort, PCI_PORT_SIZE, "pcan");
  
  dev->wInitStep = 3;
  
  dev->port.pci.pvVirtPort = ioremap(dwPort, PCI_PORT_SIZE); 
    
  if (dev->port.pci.pvVirtPort == NULL)
    return -ENODEV;
  dev->wInitStep = 4;
  
  _pci_devices++;
  dev->wInitStep = 5; 
  
  printk(KERN_INFO "%s: pci device minor %d found\n", DEVICE_NAME, dev->nMinor);
    
  return 0;
}

//----------------------------------------------------------------------------
// search all pci based devices from peak
int pcan_search_and_create_pci_devices(void)
{
  int result = 0;
  struct pcandev *dev = NULL;
  
  // search pci devices
  DPRINTK(KERN_DEBUG "%s: search_and_create_pci_devices()\n", DEVICE_NAME);
  #ifdef LINUX_26
  if (CONFIG_PCI)
  #else
  if (pci_present())
  #endif
  {
    struct pci_dev *pciDev;
    
    struct pci_dev *from = NULL;
    do
    {
      pciDev = pci_find_device((unsigned int)PCAN_PCI_VENDOR_ID, (unsigned int)PCAN_PCI_DEVICE_ID, from);
      
      if (pciDev != NULL)
      {
        u16 wSubSysID;

        // a PCI device with PCAN_PCI_VENDOR_ID and PCAN_PCI_DEVICE_ID was found
        from = pciDev;
         
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
        if (pci_enable_device(pciDev))
          continue;  
        #endif

        // get the PCI Subsystem-ID
        result = pci_read_config_word(pciDev, 0x2E, &wSubSysID);
        if (result)
          goto fail;

        // configure the PCI chip, part 1
        result = pci_write_config_word(pciDev, 0x04, 2);
        if (result)
          goto fail;
          
        result = pci_write_config_word(pciDev, 0x44, 0);
        if (result)
          goto fail;
        wmb();
          
        // make the first device on board 
        if ((dev = (struct pcandev *)kmalloc(sizeof(struct pcandev), GFP_KERNEL)) == NULL)
          goto fail;
            
        pcan_soft_init(dev, "pci", HW_PCI);
        
        dev->device_open    = sja1000_open;
        dev->device_write   = sja1000_write;
        dev->device_release = sja1000_release;
        #ifdef NETDEV_SUPPORT
        dev->netdevice_write  = sja1000_write_frame;
        #endif
        
        dev->props.ucExternalClock = 1;
  
        result = pcan_pci_init(dev, (u32)pciDev->resource[0].start, (u32)pciDev->resource[1].start, 
                                            (u16)pciDev->irq, (wSubSysID > 3) ? CHANNEL_MASTER : CHANNEL_SINGLE, NULL);        
        
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

        if (wSubSysID > 3)
        {
          struct pcandev *master_dev = dev;
    
          // make the second device on board
          if ((dev = (struct pcandev *)kmalloc(sizeof(struct pcandev), GFP_KERNEL)) == NULL)
            goto fail;

          pcan_soft_init(dev, "pci", HW_PCI);
        
          dev->device_open    = sja1000_open;
          dev->device_write   = sja1000_write;
          dev->device_release = sja1000_release;
          #ifdef NETDEV_SUPPORT
          dev->netdevice_write  = sja1000_write_frame;
          #endif
          
          dev->props.ucExternalClock = 1;
        
          result = pcan_pci_init(dev, (u32)pciDev->resource[0].start, (u32)pciDev->resource[1].start + 0x400, 
                                              (u16)pciDev->irq, CHANNEL_SLAVE, master_dev);
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
        }
      }
    } while (pciDev != NULL);
    
    DPRINTK(KERN_DEBUG "%s: search_and_create_pci_devices() is OK\n", DEVICE_NAME);
  
    #ifdef UDEV_SUPPORT
    if (!result)
      pcan_pci_register_driver(&pcan_drv.pci_drv);
    #endif
    
    fail:
  
    if ((result) && (pciDev))
    {
     #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
      pci_disable_device (pciDev);
     #endif    
    }
  }  

  return result;  
}



