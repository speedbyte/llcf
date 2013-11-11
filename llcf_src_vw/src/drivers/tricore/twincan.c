/*
 *  Low Level CAN Framework
 *
 *  Infineon TC1920 TwinCAN network device driver
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

//preprocessor stuff
#ifndef __KERNEL__
  #define __KERNEL__
#endif

#ifndef MODULE
  #define MODULE
#endif

#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>     //printk()
#include <linux/slab.h>       //kmalloc()
#include <linux/fs.h>         //everything...
#include <linux/errno.h>      //error codes
#include <linux/types.h>      //size_t, u8,..
#include <linux/netdevice.h>  //struct net_device
#include <linux/skbuff.h>
#include <linux/ioport.h>
#include <asm/io.h>           //readl, writel, memcpy_toio ...
#include <asm/string.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/string.h>

#include <linux/spinlock.h>

#include "can.h"
#include "version.h"
#include "twincan.h"

RCSID("$Id: twincan.c,v 1.7 2006/02/03 14:56:00 hartko Exp $");

#define NAME "LLCF TwinCAN-Driver for Infineon TC1920"
static __initdata const char banner[] = BANNER(NAME);

MODULE_DESCRIPTION(NAME);
MODULE_AUTHOR("Sandro Anders and Fabian Godehardt");
MODULE_LICENSE("Volkswagen Group closed source");

//global Variables
u8 use_count = 0;

u32 base = 0xf0100000;
#define BASE_ADDRESS base

const char *drv_name = "twincan_tc1920";

static int loopback = 0;
MODULE_PARM(loopback, "i");

static int mo_count = 8; //Anzahl der Message Objekte pro Can-Knoten (nie >16)
MODULE_PARM(mo_count, "i");

//****************************************************************************//
//*    Name: twincan_devs[]
//* Purpose: Array which containes the devices
//*  Author: Fabian Godehardt
//****************************************************************************//
struct net_device twincan_devs[2] = {
  { init: twincan_init, },  //init, nichts weiter
  { init: twincan_init, }
};

//****************************************************************************//
//*    Name: InterruptPriority[]
//* Purpose: Array which containes the interrupt-channels
//*  Author: Sandro Anders
//****************************************************************************//
int InterruptPriority[8] = {
  0x00000010, /* CAN_SRC0 */
  0x00000011, /* CAN_SRC1 */
  0x00000012, /* CAN_SRC2 */
  0x00000013, /* CAN_SRC3 */
  0x00000014, /* CAN_SRC4 */
  0x00000015, /* CAN_SRC5 */
  0x00000016, /* CAN_SRC6 */
  0x00000017  /* CAN_SRC7 */
};

//****************************************************************************//
//*    Name: CanSrnRegAddr[]
//* Purpose: Array which containes the Can Service Request Node Register Adresses
//*  Author: Sandro Anders
//****************************************************************************//
int CanSrnRegAddr[8]={
  CAN_SRC0_ADDR,
  CAN_SRC1_ADDR,
  CAN_SRC2_ADDR,
  CAN_SRC3_ADDR,
  CAN_SRC4_ADDR,
  CAN_SRC5_ADDR,
  CAN_SRC6_ADDR,
  CAN_SRC7_ADDR
};

//****************************************************************************//
//*    Name: reg_read
//* Purpose: Read out the register contents
//*  Author: Sandro Anders
//****************************************************************************//
u32 reg_read(int offset_addr) {
  return readl(BASE_ADDRESS + offset_addr);
}
 
//****************************************************************************//
//*    Name: reg_write
//* Purpose: Write to the registers
//*  Author: Sandro Anders
//****************************************************************************//
void reg_write(u32 value,int offset_addr) {
  writel(value, BASE_ADDRESS + offset_addr);
}

//****************************************************************************//
//*    Name: display_LEC
//* Purpose: assign LEC-bitstring to messages
//*  Author: Sandro Anders
//****************************************************************************//
void display_LEC(struct net_device *dev, int LEC) {
  switch(LEC){
    case 0:  printk(KERN_WARNING "%s: no error\n", dev->name);
             break;
    case 1:  printk(KERN_WARNING "%s: stuff error\n", dev->name);
             break;
    case 2:  printk(KERN_WARNING "%s: form error\n", dev->name);
             break;
    case 3:  printk(KERN_WARNING "%s: achnowledge error\n", dev->name);
             break;
    case 4:  printk(KERN_WARNING "%s: bit 1 error\n", dev->name);
             break;
    case 5:  printk(KERN_WARNING "%s: bit 0 error\n", dev->name);
             break;
    case 6:  printk(KERN_WARNING "%s: CRC error\n", dev->name);
             break;
    default: printk(KERN_WARNING "%s: unknown error\n", dev->name);
             break;
  }
}

//****************************************************************************//
//*    Name: clean_TXIPND
//* Purpose: cleans Interrupt-Pending Flag of the TXIPND register
//*  Author: Sandro Anders
//****************************************************************************//
void clean_TXIPND(u8 node) {
  int mask = 0x00000001, i;
  KernelReg.TXIPND.Register = REG_READ(TXIPND_ADDR);
  for(i=0; i < (mo_count*(node+1)); i++){
    if(KernelReg.TXIPND.Register & mask){
      REG_WRITE(MSGCTR_REG_INTERRUPT_PENDING_RES, MSGCTR_ADDR + i*0x20);
      PDEBUG("Transmit IRQ on MO-Nr.: %d\n",i);
      //priv->status = -1;
      break;
    }
    mask = mask << 1;
  }
}

//****************************************************************************//
//*    Name: clean_RXIPND
//* Purpose: cleans Interrupt-Pending Flag of the RXIPND register
//*  Author: Sandro Anders
//****************************************************************************//
int clean_RXIPND(u8 node) {
  int mask = 0x00000001, i; //, result = -1;
  KernelReg.RXIPND.Register = REG_READ(RXIPND_ADDR);
  for(i=0; i < (mo_count*(node+1)); i++){
    if(KernelReg.RXIPND.Register & mask){
      REG_WRITE(MSGCTR_REG_INTERRUPT_PENDING_RES, MSGCTR_ADDR + i*0x20);
      PDEBUG("Receive IRQ on MO-Nr.: %d\n",i);
      //if(result < 0)result = i;
      //priv->status = -1;
      break;
    }
    mask = mask << 1;
  }
  return i;
}


//****************************************************************************//
//*    Name: twincan_stats
//* Purpose: Contains the stats of the TwinCAN Device
//*  Author: Sandro Anders
//****************************************************************************//
struct net_device_stats *twincan_stats(struct net_device *dev) {
  struct twincan_priv *priv = (struct twincan_priv *) dev->priv;
  return &priv->stats;
}

//****************************************************************************//
//*    Name: fill_KernelConstants
//* Purpose: Sets the Kernel Constants
//*  Author: Sandro Anders
//****************************************************************************//
void fill_KernelConstants() {
  KernelConstants.BitTimingNodeA =              BAUD_0100_000_WITH_MHZ_50;
  KernelConstants.BitTimingNodeB =              BAUD_0500_000_WITH_MHZ_50;
  KernelConstants.AnalysingModeNodeA =          ANALYSE_MODE_DISABLED;
  KernelConstants.AnalysingModeNodeB =          ANALYSE_MODE_DISABLED;
  KernelConstants.FrameCounterModeNodeA =       FCR_UPDATE_ON_RECEIVE_FRAME | FCR_INTRPT;
  KernelConstants.FrameCounterModeNodeB =       FCR_UPDATE_ON_RECEIVE_FRAME | FCR_INTRPT;
  KernelConstants.GlobalInterruptControlNodeA = STATUS_CHANGE_INTRPT|ERROR_INTRPT|LAST_ERROR_INTRPT;
  KernelConstants.GlobalInterruptControlNodeB = STATUS_CHANGE_INTRPT|ERROR_INTRPT|LAST_ERROR_INTRPT;
  KernelConstants.GlobalInterruptNodePointerA = ( GINP_REG_DEF_ERROR_NODE_POINTR_A | \
										GINP_REG_DEF_LAST_ERR_NODE_PTR_A | \
                                                  GINP_REG_DEF_TXRX_NODE_POINTR_A  | \
                                                  GINP_REG_DEF_CFC_NODE_POINTR_A);

  KernelConstants.GlobalInterruptNodePointerB = ( GINP_REG_DEF_ERROR_NODE_POINTR_B | \
  										GINP_REG_DEF_LAST_ERR_NODE_PTR_B | \
                                                  GINP_REG_DEF_TXRX_NODE_POINTR_B  | \
                                                  GINP_REG_DEF_CFC_NODE_POINTR_B);

  if(loopback == 1){
    PDEBUG("\n\nLoopback ein ....\n\n");
    KernelConstants.BitTimingNodeA |= BTR_REG_LOOP_BACK_MODE_MASK;
    KernelConstants.BitTimingNodeB |= BTR_REG_LOOP_BACK_MODE_MASK;
  }
  else { PDEBUG("\n\nLoopback aus ....\n\n"); }
}

//****************************************************************************//
//*    Name: show_register_contents
//* Purpose: Shows the Content of all TwinCan Registers
//*  Author: Sandro Anders and Fabian Godehardt
//****************************************************************************//
void show_register_contents(){
  int i;
  
  PDEBUG("\n=====[ Register Contents ]=====\n");

  PDEBUG("ACR  : %08x\n",REG_READ(ACR_ADDR));
  PDEBUG("ASR  : %08x\n",REG_READ(ASR_ADDR));
  PDEBUG("AIR  : %08x\n",REG_READ(AIR_ADDR));
  PDEBUG("ABTR : %08x\n",REG_READ(ABTR_ADDR));
  PDEBUG("AGINP: %08x\n",REG_READ(AGINP_ADDR));
  PDEBUG("AFCR : %08x\n",REG_READ(AFCR_ADDR));
  PDEBUG("AIMR0: %08x\n",REG_READ(AIMR0_ADDR));
  PDEBUG("AIMR4: %08x\n",REG_READ(AIMR4_ADDR));
  PDEBUG("AECNT: %08x\n",REG_READ(AECNT_ADDR));

  PDEBUG("BCR  : %08x\n",REG_READ(BCR_ADDR));
  PDEBUG("BSR  : %08x\n",REG_READ(BSR_ADDR));
  PDEBUG("BIR  : %08x\n",REG_READ(BIR_ADDR));
  PDEBUG("BBTR : %08x\n",REG_READ(BBTR_ADDR));
  PDEBUG("BGINP: %08x\n",REG_READ(BGINP_ADDR));
  PDEBUG("BFCR : %08x\n",REG_READ(BFCR_ADDR));
  PDEBUG("BIMR0: %08x\n",REG_READ(BIMR0_ADDR));
  PDEBUG("BIMR4: %08x\n",REG_READ(BIMR4_ADDR));
  PDEBUG("BECNT: %08x\n",REG_READ(BECNT_ADDR));

  for(i=0;i<mo_count*2;i++){
   	PDEBUG("MSGDR0  %i: %08x\n",i,REG_READ(MSGDR0_ADDR + i*0x20));
   	PDEBUG("MSGDR4  %i: %08x\n",i,REG_READ(MSGDR4_ADDR + i*0x20));
   	PDEBUG("MSGAR   %i: %08x\n",i,REG_READ(MSGAR_ADDR + i*0x20));
   	PDEBUG("MSGAMR  %i: %08x\n",i,REG_READ(MSGAMR_ADDR + i*0x20));
   	PDEBUG("MSGCTR  %i: %08x\n",i,REG_READ(MSGCTR_ADDR + i*0x20));
   	PDEBUG("MSGCFG  %i: %08x\n",i,REG_READ(MSGCFG_ADDR + i*0x20));
   	PDEBUG("MSGFGCR %i: %08x\n",i,REG_READ(MSGFGCR_ADDR + i*0x20));
  }

  PDEBUG("RXIPND: %08x\n",REG_READ(RXIPND_ADDR));
  PDEBUG("TXIPND: %08x\n",REG_READ(TXIPND_ADDR));

  PDEBUG("P2 Data Output       : %08x\n",readl(0xF0002A10)); //P2 Data Output
  PDEBUG("P2 Data Input        : %08x\n",readl(0xF0002A14)); //P2 Data Input
  PDEBUG("P2 Direction         : %08x\n",readl(0xF0002A18)); //P2 Direction
  PDEBUG("P2 Open Drain        : %08x\n",readl(0xF0002A1C)); //P2 Open Drain
  PDEBUG("P2 PullUp/Down Select: %08x\n",readl(0xF0002A28)); //P2 PullUp/Down Select
  PDEBUG("P2 PullUp/Down Enable: %08x\n",readl(0xF0002A2C)); //P2 PullUp/Down Enable
  PDEBUG("P2 Alternate Select 0: %08x\n",readl(0xF0002A44)); //P2 Alternate Select 0

  PDEBUG("\n===============================\n");
}

//****************************************************************************//
//*    Name: twincan_interrupt
//* Purpose: TwinCAN Interrupt handling
//*  Author: Sandro Anders and Fabian Godehardt
//****************************************************************************//
void twincan_interrupt(int irq, void *dev_id, struct pt_regs *regs) {
  int i;

  struct net_device *dev = (struct net_device*)dev_id;
  struct twincan_priv *priv = (struct twincan_priv*)dev->priv;

  PDEBUG("Interrupt Handler - start -\n");

  spin_lock(&priv->lock);

  switch(irq){
    case 0x10 :
         //SRN for LEC,ERWN,BOFF,CFCOV,TXOK and RXOK on Node A
         PDEBUG("IRQ: Global Interrupt Node A\n");
         KernelReg.ASR.Register = REG_READ(ASR_ADDR);

         if(KernelReg.ASR.Bitfield.LEC != 0) { // Last Error Code
           display_LEC(dev, KernelReg.ASR.Bitfield.LEC);
           KernelReg.ASR.Bitfield.LEC = 0;
         }
         if(KernelReg.ASR.Bitfield.EWRN == 1 ) printk(KERN_WARNING "%s: EWRN\n", dev->name);
         if(KernelReg.ASR.Bitfield.BOFF == 1 ) printk(KERN_WARNING "%s: BOFF\n", dev->name);

         if(KernelReg.ASR.Bitfield.TXOK == 1){
           PDEBUG("IRQ: TXOK Node A\n");
           KernelReg.ASR.Bitfield.TXOK = 0;
           clean_TXIPND(0);
         }

         if(KernelReg.ASR.Bitfield.RXOK == 1){
           PDEBUG("IRQ: RXOK Node A\n");
           KernelReg.ASR.Bitfield.RXOK = 0;
           clean_RXIPND(0);
         }

         REG_WRITE(KernelReg.ASR.Register, ASR_ADDR);
         //delete pending IRQ
         REG_WRITE(IR_REG_HW_RESET_VALUE, AIR_ADDR);

         netif_wake_queue(dev);
         break;

    case 0x11 :
         //SRN for LEC,ERWN,BOFF,CFCOV,TXOK and RXOK on Node B
         PDEBUG("IRQ: Global Interrupt Node B\n");
         KernelReg.BSR.Register = REG_READ(BSR_ADDR);

         if(KernelReg.BSR.Bitfield.LEC != 0) { //Last Error Code
           display_LEC(dev, KernelReg.BSR.Bitfield.LEC);
           KernelReg.BSR.Bitfield.LEC = 0;
         }
         if(KernelReg.BSR.Bitfield.EWRN == 1 ) printk(KERN_WARNING "%s: EWRN\n", dev->name);
         if(KernelReg.BSR.Bitfield.BOFF == 1 ) printk(KERN_WARNING "%s: BOFF\n", dev->name);

         if(KernelReg.BSR.Bitfield.TXOK == 1) {
           PDEBUG("IRQ: TXOK Node B\n");
           KernelReg.BSR.Bitfield.TXOK = 0;
           clean_TXIPND(1);
         }

         if(KernelReg.BSR.Bitfield.RXOK == 1) {
           PDEBUG("IRQ: RXOK Node B\n");
           KernelReg.BSR.Bitfield.RXOK = 0;
           clean_RXIPND(1);
         }

         REG_WRITE(KernelReg.BSR.Register, BSR_ADDR);
         //delete pending IRQ
         REG_WRITE(IR_REG_HW_RESET_VALUE, BIR_ADDR);
         netif_wake_queue(dev);
         break;

    case 0x12 : //SRN for all Node A MO Tx Interrupts
         PDEBUG("Node A MO Tx Interrupt\n");
         clean_TXIPND(0);
         break;

    case 0x13 ://SRN for all Node B MO Tx Interrupts
         PDEBUG("Node B MO Tx Interrupt\n");
         clean_TXIPND(1);
         break;

    case 0x14 : //SRN for all Node A MO Rx Interrupts
         PDEBUG("Node A MO Rx Interrupt\n");
         i = clean_RXIPND(0);
         if(i >= 0) twincan_rx(dev, i);
         break;

    case 0x15 : //SRN for all Node B MO Rx Interrupts
         PDEBUG("Node B MO Rx Interrupt\n");
         i = clean_RXIPND(1);
         if(i >= 0) twincan_rx(dev, i);
         break;

    case 0x16 : //SRN for all Node A MO Rtr Interrupts
         PDEBUG("Node A MO Rtr Interrupt\n");
         break;

    case 0x17 : //SRN for all Node B MO Rtr Interrupts
         PDEBUG("Node B MO Rtr Interrupt\n");
         break;

    default:
         printk(KERN_WARNING "%s: %i is a unknown irq\n", dev->name, irq );
         break;
  }

  spin_unlock(&priv->lock);

  PDEBUG("Interrupt Handler - end -\n");
  return;
}

//****************************************************************************//
//*    Name: twincan_tx_timeout
//* Purpose: Called on timeout
//*  Author: Sandro Anders
//****************************************************************************//
void twincan_tx_timeout(struct net_device *dev){
  struct twincan_priv *priv = (struct twincan_priv *) dev->priv;
   
  printk(KERN_INFO "%s: Transmit timed out on %ld, latency %ld\n",
    dev->name, jiffies, jiffies - dev->trans_start);

  //hier Geraet auf sinnvollen Zustand zurueck setzen
  REG_WRITE(MSGCTR_REG_HW_RESET_VALUE, MSGCTR_ADDR + priv->current_MO*0x20); //reset
  priv->status = -1;
  priv->stats.tx_errors++;
  netif_wake_queue(dev);
}

//****************************************************************************//
//*    Name: twincan_start_xmit
//* Purpose: Called on transmission start
//*  Author: Sandro Anders
//****************************************************************************//
int twincan_start_xmit(struct sk_buff *skb, struct net_device *dev){
  u8 result = 0, node = 1;
  struct can_frame *cf = (struct can_frame *) skb->data;
  struct twincan_priv *priv = (struct twincan_priv *) dev->priv;

  PDEBUG("Transmit - start -\n");

  dev->trans_start = jiffies;

  if(strcmp(dev->name, "can0") == 0) node = 0; //node = 0 (can0) else node = 1 (can1)

  //Hardware specific Transmit
  result =  TwinCan_Transmit(dev,
                             node,
                             cf->can_id,
                             cf->can_dlc,
                             cf->data);

  dev_kfree_skb(skb);

  if(result == 0)priv->stats.tx_packets++;

  PDEBUG("Transmit - end - with %d\n", result);

  return result;
 }

//****************************************************************************//
//*    Name: twincan_rx
//* Purpose: Called when receiving a can-frame
//*  Author: Sandro Anders
//****************************************************************************//
void twincan_rx(struct net_device *dev, int mo_nr){
  int i = 0;

  struct twincan_priv *priv = (struct twincan_priv *) dev->priv;
  struct can_frame *cf;
  struct sk_buff *skb;

  canid_t id;
  uint8_t dlc;

  PDEBUG("Receive - start -\n");

  skb = dev_alloc_skb(sizeof(struct can_frame));
  if(skb == NULL){ return; }

  skb->dev = dev;
  skb->protocol = htons(ETH_P_CAN);

  MessageObject.MSGCTR.Register = REG_READ(MSGCTR_ADDR + mo_nr*0x20);
  MessageObject.MSGCTR.Bitfield.NEWDAT = MSGCTR_REG_BITFIELD_OFF; //reset NEWDAT
  REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR + mo_nr*0x20);

  // Read Registers
  MessageObject.MSGCFG.Register = REG_READ(MSGCFG_ADDR + mo_nr*0x20);
  MessageObject.MSGDR0.Register = REG_READ(MSGDR0_ADDR + mo_nr*0x20);
  MessageObject.MSGDR4.Register = REG_READ(MSGDR4_ADDR + mo_nr*0x20);

  id  = REG_READ(MSGAR_ADDR + mo_nr*0x20);
  dlc = MessageObject.MSGCFG.Bitfield.DLC;

  cf = (struct can_frame*)skb_put(skb, sizeof(struct can_frame));

  if(MessageObject.MSGCFG.Bitfield.XTD == 1){ //eXTenDed Identifier
    id |= CAN_EFF_FLAG;
    cf->can_id    = id; //htonl(id);
    PDEBUG("Extended Frame!\n");
  }
  else { // Standard
    cf->can_id    = id >> 18; //htonl(id >> 18);
    PDEBUG("Standard Frame!\n");
  }

  cf->can_dlc   = dlc;
  /* WHAT ABOUT RTR-FRAMES??? */
  
  for(i=0; i<8; i++) {
    if(i < dlc) {
      if(i < 4) {
        cf->data[i] = MessageObject.MSGDR0.DATA[i];
      }
      else {
        cf->data[i] = MessageObject.MSGDR4.DATA[i-4];
      }
    }
    else {
      cf->data[i] = 0;
    }
  }

  PDEBUG("ID   = %x\n",id >> 18);
  PDEBUG("DLC  = %i\n",cf->can_dlc);
  PDEBUG("DATA = %llx\n",*(u64*)cf->data);

  //reset INTPND and the corresponding RXIPND flag
  REG_WRITE(MSGCTR_REG_INTERRUPT_PENDING_RES, MSGCTR_ADDR + mo_nr*0x20);

  //to llcf
  netif_rx(skb);

  dev->last_rx = jiffies;
  priv->stats.rx_packets++;
  priv->stats.rx_bytes += cf->can_dlc;
  priv->status = MO_RECEIVE;

  PDEBUG("Receive - end -\n");
}

//****************************************************************************//
//*    Name: twincan_init
//* Purpose: Inits the twincan devices (called while loading the module)
//*  Author: Sandro Anders and Fabian Godehardt
//****************************************************************************//
int twincan_init(struct net_device *dev) {
  //struct twincan_priv *priv = (struct twincan_priv *) dev->priv;

  PDEBUG("Init and fill net_device structure - start -\n");
  
  ether_setup(dev); //automatisches setzen einiger Felder in der Struktur

  dev->irq             = 0;
  dev->base_addr       = BASE_ADDRESS;
  dev->open            = twincan_open;
  dev->stop            = twincan_release;
  dev->hard_start_xmit = twincan_start_xmit;
  dev->get_stats       = twincan_stats;
  dev->tx_timeout      = twincan_tx_timeout;
  dev->watchdog_timeo  = TX_TIMEOUT;

  SET_MODULE_OWNER(dev);

  //private structure alloc.
  dev->priv = kmalloc(sizeof(struct twincan_priv), GFP_KERNEL);
  if(dev->priv == NULL){
    printk(KERN_ERR "%s: out of memory", drv_name);
    return -ENOMEM;
  }

  memset(dev->priv, 0, sizeof(struct twincan_priv));

  spin_lock_init(& ((struct twincan_priv *) dev->priv)->lock);

  //priv->clock = TC_CLOCK;

  PDEBUG("Init and fill net_device structure - end -\n");

  return 0;
}

//****************************************************************************//
//*    Name: twincan_open
//* Purpose: Opens the twincan devices (called on "ifconfig ... up")
//*  Author: Sandro Anders and Fabian Godehardt
//****************************************************************************//
int twincan_open(struct net_device *dev){
  int i;

  struct twincan_priv *priv = (struct twincan_priv *) dev->priv;

  MOD_INC_USE_COUNT;

  memcpy(dev->dev_addr, MAC_ADDR, MAX_ADDR_LEN);

  PDEBUG("Device open - start - Node: %s\n", dev->name);

  use_count++;

  if(strcmp(dev->name, "can0")== 0) priv->Baud_Rate = KernelConstants.BitTimingNodeA;
    else priv->Baud_Rate = KernelConstants.BitTimingNodeB;

  if(use_count == 1) { //ifconfig das erste mal aufgerufen (weder can0 noch can1 aktiv)
    //IO-Memory alloc.
    if(check_mem_region(dev->base_addr, TWINCAN_IO_LEN+1)) {
      PDEBUG("Error on alloc I/O Mem-Region\n");
      return -ENOMEM;
    }
    request_mem_region(dev->base_addr, TWINCAN_IO_LEN+1, drv_name);

    //IO-Ports alloc.
    if(check_mem_region(P2_BASE, P2_ADDR_RANGE+1)) {
      PDEBUG("Error on alloc IO-Ports\n");
      PDEBUG("sizeof(PORT) = %d\n", P2_ADDR_RANGE+1);
      release_mem_region(dev->base_addr, TWINCAN_IO_LEN+1); /* io-mem release */
      return -EBUSY;
    }
    request_mem_region(P2_BASE, P2_ADDR_RANGE+1, drv_name);
     
    TwinCanPortInit();
  } else {
    if(strcmp(dev->name,"can0") == 0)
      PDEBUG("can1 was active, skip IO-memory alloc.\n");
    else PDEBUG("can0 was active, skip IO-memory alloc.\n");
  }
   
  //Init HW
  PDEBUG("Init-HW start\n");

  if(strcmp(dev->name, "can0") == 0) { //Node A, Lowspeed
    TwinCanInitStart(0, mo_count); //TwinCanInitStart(node, mo_count);

    TwinCanKernelInit(0); //TwinCanKernelInit(node);

    TwinCanMessageObjectInit(0, NODE_A, MO_TRANSMIT, 0, 0);
    TwinCanMessageObjectInit(1, NODE_A, MO_RECEIVE, 0x100, SFF); //specific id
    TwinCanMessageObjectInit(2, NODE_A, MO_RECEIVE, 0, SFF); //no filter
    TwinCanMessageObjectInit(3, NODE_A, MO_REMOTE, 0, 0);
    TwinCanMessageObjectInit(4, NODE_A, MO_TRANSMIT, 0, 0);
    TwinCanMessageObjectInit(5, NODE_A, MO_RECEIVE, 0x101, EFF); //specific id
    TwinCanMessageObjectInit(6, NODE_A, MO_RECEIVE, 0, SFF); //no filter
    TwinCanMessageObjectInit(7, NODE_A, MO_REMOTE, 0, 0);

    TwinCanInitEnd(0);

    PDEBUG("MO_INIT: Node A initialized\n");
  }
  else { //Node B, Highspeed
    TwinCanInitStart(1, mo_count);
  
    TwinCanKernelInit(1);

    TwinCanMessageObjectInit(8, NODE_B, MO_TRANSMIT, 0, 0);
    TwinCanMessageObjectInit(9, NODE_B, MO_RECEIVE, 0x100, SFF); //specific id
    TwinCanMessageObjectInit(10, NODE_B, MO_RECEIVE, 0, SFF); //no filter
    TwinCanMessageObjectInit(11, NODE_B, MO_REMOTE, 0, 0);
    TwinCanMessageObjectInit(12, NODE_B, MO_TRANSMIT, 0, 0);
    TwinCanMessageObjectInit(13, NODE_B, MO_RECEIVE, 0x101, EFF);// specific id*/
    TwinCanMessageObjectInit(14, NODE_B, MO_RECEIVE, 0, EFF); // no filter */
    TwinCanMessageObjectInit(15, NODE_B, MO_REMOTE, 0, 0);
    
    TwinCanInitEnd(1);
    
    PDEBUG("MO_INIT: Node B initialized\n");
  }

  PDEBUG("Init-HW end\n");

  PDEBUG("Register Interrupts\n");
   
  //register interrupt handler
  if(strcmp(dev->name, "can0") == 0) { //can0
    PDEBUG("Register can0 Interrupts...\n");
    for(i=0; i<8; i+=2)
      if(request_irq(InterruptPriority[i], twincan_interrupt, SA_SHIRQ,
        drv_name, (void*)dev) ) { return -EAGAIN; }
  }
  else { //can1
    PDEBUG("Register can1 Interrupts...\n");
    for(i=1; i<8; i+=2)
      if(request_irq(InterruptPriority[i], twincan_interrupt, SA_SHIRQ,
        drv_name, (void*)dev) ) { return -EAGAIN; }
  }

  priv->open_time = jiffies;

  netif_start_queue(dev);

  PDEBUG("Device open - end -\n");

  return 0;
}

//****************************************************************************//
//*    Name: twincan_release
//* Purpose: Stop/Release the twincan module (called on "ifconfig ... down")
//*  Author: Sandro Anders and Fabian Godehardt
//****************************************************************************//
int twincan_release(struct net_device *dev) {
  int i;

  struct twincan_priv *priv = (struct twincan_priv *) dev->priv;

  PDEBUG("Device (%s) close - start -\n", dev->name);

  use_count--;

  if(use_count == 0) {
    //IO-Mem freigeben
    release_mem_region(dev->base_addr, TWINCAN_IO_LEN+1);
    PDEBUG("=> free I/O Memory\n");

    //IO-Ports freigeben
    release_mem_region(P2_BASE, P2_ADDR_RANGE+1);
    PDEBUG("=> free I/O Ports\n");
  }

  if(strcmp(dev->name, "can0") == 0) { //irq freigeben
    PDEBUG("=> free IRQs\n");
    KernelReg.AIR.Register = REG_READ(AIR_ADDR);

    //delete pending irq's
    if(KernelReg.AIR.Bitfield.INTID != 0){
      if(KernelReg.AIR.Bitfield.INTID == 1) {
        PDEBUG("Global IRQ on Node A is Pending!\n"); }
        else{ PDEBUG("TX / RX IRQ on Node A is Pending!\n"); }
      REG_WRITE(IR_REG_HW_RESET_VALUE, AIR_ADDR);
    }

    //disable SRN's
    disable_all_SRN(0);

    //disable IRQ's
    for(i=0; i<8; i+=2)
      free_irq(InterruptPriority[i], (void*)dev);
  }
  else {
    KernelReg.BIR.Register = REG_READ(BIR_ADDR);

    //delete pending irq's
    if(KernelReg.BIR.Bitfield.INTID != 0){
      if(KernelReg.BIR.Bitfield.INTID == 1) {
        PDEBUG("Global IRQ on Node B is Pending!\n"); }
        else{ PDEBUG("TX / RX IRQ on Node B is Pending!\n"); }
      REG_WRITE(IR_REG_HW_RESET_VALUE, BIR_ADDR);
    }

    //disable SRN's
    disable_all_SRN(1);

    //disable IRQ's
    for(i=1; i<8; i+=2)
      free_irq(InterruptPriority[i], (void*)dev);
  }

  if(use_count == 0) {
    priv->open_time = 0;

    netif_stop_queue(dev); //kein Senden mehr moeglich
  }

  MOD_DEC_USE_COUNT;

  PDEBUG("Device(%s) close - end -\n", dev->name);

  return 0;
}

//****************************************************************************//
//*    Name: twincan_init_module
//* Purpose: Init the twincan module (called on "insmod ...")
//*  Author: Fabian Godehardt
//****************************************************************************//
int init_module(void) {
  int result, i, device_present = 0;

  PDEBUG("Module load start\n");

  printk(banner);

  fill_KernelConstants();

  strcpy(twincan_devs[0].name, "can0");
  strcpy(twincan_devs[1].name, "can1");

  for (i=0; i<2;  i++) {
    result = register_netdev(twincan_devs + i);
    if (result)
      PDEBUG("error %i registering device %s\n", result, twincan_devs[i].name);
    else device_present++;
  }
  
  PDEBUG("Module load end\n");
   
  return device_present ? 0 : -ENODEV;
}

//****************************************************************************//
//*    Name: twincan_cleanup
//* Purpose: Cleanup the twincan module (called on "rmmod ...")
//*  Author: Fabian Godehardt
//****************************************************************************//
void cleanup_module(void) {
  int i;
   
  for (i=0; i<2;  i++) {
    kfree(twincan_devs[i].priv);
    unregister_netdev(twincan_devs + i);
  }

  return;
}

//module_init(twincan_init_module);
//module_exit(twincan_cleanup);
