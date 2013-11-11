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
// Contributions: John Privitera  (JohnPrivitera@dciautomation.com)
//****************************************************************************

//****************************************************************************
//
// pcan_fifo.c - manages the ring buffers for read and write data
//
// $Id: pcan_fifo.c 451 2007-02-02 10:09:50Z ohartkopp $
//
//****************************************************************************

//****************************************************************************
// INCLUDES
#include <src/pcan_common.h> 
#include <linux/types.h>
#include <linux/errno.h>    // error codes
#include <linux/string.h>   // memcpy
#include <linux/sched.h>
#include <asm/system.h>     // cli(), save_flags(), restore_flags()
#include <linux/spinlock.h>

#include <src/pcan_fifo.h> 

//****************************************************************************
// DEFINES

//****************************************************************************
// GLOBALS

//****************************************************************************
// LOCALS

//****************************************************************************
// CODE 
static inline int not_atomic(void)
{
#if defined(in_atomic)
  #if defined(irqs_disabled)
    return (!in_atomic() && !irqs_disabled());
  #else
    return (!in_atomic());
  #endif
#else
  #if defined(irqs_disabled)
    return (!irqs_disabled());
  #else
    return 1;
  #endif
#endif
}

#ifdef XENOMAI

static inline void init_lock(register FIFO_MANAGER *anchor){}
static inline void set_lock(register FIFO_MANAGER *anchor){}
static inline void release_lock(register FIFO_MANAGER *anchor){}

#else

// some helpers to sparsely set and release locks
static inline void init_lock(register FIFO_MANAGER *anchor)
{
  // need_irq_restore is global for that specific FIFO and local for the lock
  anchor->need_irqrestore = 0;
  spin_lock_init(&anchor->lock);
}
  
static inline void set_lock(register FIFO_MANAGER *anchor)
{
  // it s not necessary to lock the lock if you are inside uninterruptible code
  if (not_atomic())
  {
    spin_lock_irqsave(&anchor->lock, anchor->flags);
    anchor->need_irqrestore = 1;
  }
}
  
static inline void release_lock(register FIFO_MANAGER *anchor)
{
  // release the lock only if you locked it
  if (anchor->need_irqrestore)
  {
    anchor->need_irqrestore = 0;
    spin_unlock_irqrestore(&anchor->lock, anchor->flags);
  }
}

#endif

int pcan_fifo_reset(register FIFO_MANAGER *anchor)
{
  set_lock(anchor);
  
  anchor->dwTotal       = 0; 
  anchor->nStored       = 0; 
  anchor->r = anchor->w = anchor->bufferBegin; // nothing to read
  
  release_lock(anchor);

  // DPRINTK(KERN_DEBUG "%s: pcan_fifo_reset() %d %p %pd\n", DEVICE_NAME, anchor->nStored, anchor->r, anchor->w);

  return 0;
}

int pcan_fifo_init(register FIFO_MANAGER *anchor, void *bufferBegin, void *bufferEnd, int nCount, u16 wCopySize)
{
  anchor->wCopySize   = wCopySize;
  anchor->wStepSize   = (bufferBegin == bufferEnd) ? 0 : ((bufferEnd - bufferBegin) / (nCount - 1));
  anchor->nCount      = nCount;
  anchor->bufferBegin = bufferBegin;
  anchor->bufferEnd   = bufferEnd;
  anchor->flags       = 0;
  
  // check for fatal program errors
  if ((anchor->wStepSize < anchor->wCopySize) || (anchor->bufferBegin > anchor->bufferEnd) || (nCount <= 1))
    return -EINVAL;
  
  init_lock(anchor);

  return pcan_fifo_reset(anchor);
}

int pcan_fifo_put(register FIFO_MANAGER *anchor, void *pvPutData)
{
  int err = 0;
  
  // DPRINTK(KERN_DEBUG "%s: pcan_fifo_put() %d %p %p\n", DEVICE_NAME, anchor->nStored, anchor->r, anchor->w);

  set_lock(anchor);

  if (anchor->nStored < anchor->nCount)
  {
    memcpy(anchor->w, pvPutData, anchor->wCopySize);
    
    anchor->nStored++;
    anchor->dwTotal++;
    
    if (anchor->w < anchor->bufferEnd)
      anchor->w += anchor->wStepSize;   // increment to next
    else
      anchor->w = anchor->bufferBegin;  // start from begin
  }
  else
    err = -ENOSPC;
    
  release_lock(anchor); 
  
  return err;
}


int pcan_fifo_get(register FIFO_MANAGER *anchor, void *pvGetData)
{
  int err = 0;
  
  // DPRINTK(KERN_DEBUG "%s: pcan_fifo_get() %d %p %p\n", DEVICE_NAME, anchor->nStored, anchor->r, anchor->w);

  set_lock(anchor);

  if (anchor->nStored > 0)
  {
    memcpy(pvGetData, anchor->r, anchor->wCopySize);

    anchor->nStored--;
    if (anchor->r < anchor->bufferEnd)
      anchor->r += anchor->wStepSize;  // increment to next   
    else
      anchor->r = anchor->bufferBegin; // start from begin
  }
  else
    err = -ENODATA;
  
  release_lock(anchor);
  
  return err;
}


//----------------------------------------------------------------------------
// returns the current count of elements in fifo
int pcan_fifo_status(FIFO_MANAGER *anchor)
{
  return anchor->nStored;
}

//----------------------------------------------------------------------------
// returns 0 if the fifo is full
int pcan_fifo_near_full(FIFO_MANAGER *anchor)
{
  return (anchor->nStored < (anchor->nCount - 1));
} 

//----------------------------------------------------------------------------
// returns 0 if the fifo is empty
int pcan_fifo_empty(FIFO_MANAGER *anchor)
{
  return anchor->nStored;
}


