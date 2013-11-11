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

 #ifndef __KERNEL__
 #  define __KERNEL__
 #endif
 #ifndef MODULE
 #  define MODULE
 #endif
 
 #include <linux/config.h>
 #include <linux/module.h>

 #include <linux/kernel.h>     /* printk()       */
 #include <linux/slab.h>       /* kmalloc()      */
 #include <linux/fs.h>         /* everything...  */
 #include <linux/errno.h>      /* error codes    */
 #include <linux/types.h>      /* size_t, u8,..  */
 #include <linux/netdevice.h>  /* struct net_device  */
 #include <asm/io.h>           /* readl, writel, outl, inl */
 #include <linux/interrupt.h>
 #include <linux/types.h>

 #include "can.h"
 #include "version.h"
 #include "can_ioctl.h"
 #include "twincan.h"
 //#include "twincan_port.h"

RCSID("$Id: twincan_hw.c,v 1.6 2006/02/03 14:49:49 hartko Exp $");

 u8 used_mo, use_count_hw = 0;

 void unprotect_wdcon(void)
 { /* used to enable the access to the CAN_CLC register */
   u32    uiWDTCON0, uiWDTCON1;

   uiWDTCON0 = readl(_WDTCON0);
   uiWDTCON1 = readl(_WDTCON1);
   uiWDTCON0 = uiWDTCON0 & 0xFFFFFF03;
   uiWDTCON0 = uiWDTCON0 | 0xF0;
   uiWDTCON0 = uiWDTCON0 | (uiWDTCON1 & 0xc);
   uiWDTCON0 = uiWDTCON0 ^ 0x2;
   writel(uiWDTCON0, _WDTCON0);
   // modify access to wdtcon0
   uiWDTCON0 = uiWDTCON0 & 0xFFFFFFF0;
   uiWDTCON0 = uiWDTCON0 | 2;

   writel(uiWDTCON0, _WDTCON0);
   return;
}


void protect_wdcon(void)
{ /* used to disable the access to the CAN_CLC register */

   u32    uiWDTCON0, uiWDTCON1;

   uiWDTCON0 = readl(_WDTCON0);
   uiWDTCON1 = readl(_WDTCON1);
   uiWDTCON0 &= 0xFFFFFF03;
   uiWDTCON0 |= 0xF0;
   uiWDTCON0 |= (uiWDTCON1 & 0xc);
   uiWDTCON0 ^= 0x2;
   writel(uiWDTCON0, _WDTCON0);
   // modify access to wdtcon0
   uiWDTCON0 &= 0xFFFFFFF0;
   uiWDTCON0 |= 3;
   writel(uiWDTCON0, _WDTCON0);
   return;
}

 /*###### Begin HW - Initialisation ===> ######*/

 void TwinCanInitStart( int node, u8 get_mo_count ){
   PDEBUG("InitStart =>\n");
   
   if(!use_count_hw) {
     /******************************************************************************/
     /* @Descript.: - Enables TwinCan module clock with clock division factor "1"  */
     /*             - Disables TwinCan node A and B                                */
     /******************************************************************************/
     use_count_hw++;

     unprotect_wdcon();

     used_mo = get_mo_count;

     KernelReg.CLC.Register = CLC_REG_HW_RESET_VALUE;
     KernelReg.CLC.Bitfield.RMC = 1; //n MHZ / RMC = f_can
     REG_WRITE(KernelReg.CLC.Register, CAN_CLC_ADDR);

     protect_wdcon();
   }

   switch (node) {
     case 0 :
       KernelReg.ACR.Register = REG_READ(ACR_ADDR);
       KernelReg.ACR.Register |= CR_REG_INIT_BIT_SET;
       REG_WRITE(KernelReg.ACR.Register, ACR_ADDR);
       PDEBUG("Node: %i\n", node);
       break;
       
     case 1 :
       KernelReg.BCR.Register = REG_READ(BCR_ADDR);
       KernelReg.BCR.Register |= CR_REG_INIT_BIT_SET;
       REG_WRITE(KernelReg.BCR.Register, BCR_ADDR);
       PDEBUG("Node: %i\n", node);
       break;

     default :
       PDEBUG("Error! Wrong Node/Nodes requested!\n");
   }
   
   PDEBUG("<= InitStart\n");
 }

 void TwinCanPortInit(){
   /******************************************************************************/
   /* @Descript.: - Initializes Port Control Registers supporting TwinCAN Nodes  */
   /******************************************************************************/
   u32 OD_REG, PUDSEL_REG, PUDEN_REG, DATA_OUT;

   PDEBUG("PortInit =>\n");

   pstP2.ALTSEL0 = readl(P2_BASE + ALTSEL0_ADDR);
   pstP2.DIR     = readl(P2_BASE + DIR_ADDR);
   OD_REG        = readl(P2_BASE + OD_ADDR);
   DATA_OUT      = readl(0xF0002A10);
   //Data Output
   DATA_OUT &= 0xFFFF0000;
   DATA_OUT |= 0x00000800;
   writel(DATA_OUT, 0xF0002A10);
   PDEBUG("DataInput: %08x\n", readl(0xF0002A14));

   // Alternate Select
   pstP2.ALTSEL0 &= 0xFFFF0000;
   pstP2.ALTSEL0 |= 0x00000C0F;
   writel(pstP2.ALTSEL0, P2_BASE + ALTSEL0_ADDR);

   // PIN Direction
   pstP2.DIR &= 0xFFFF0000;
   pstP2.DIR |= 0x00000800;
   writel(pstP2.DIR, P2_BASE + DIR_ADDR);

   //Open Drain
   OD_REG &= 0xFFFF0000; //OpenDrain abgeschaltet
   OD_REG |= 0x00000800;
   writel(OD_REG, P2_BASE + OD_ADDR);

   //PullUp-PullDown
   PUDEN_REG &= 0xFFFF0000;
   writel(OD_REG, P2_BASE + PUDEN_ADDR);
   PUDSEL_REG &= 0xFFFF0000;
   writel(OD_REG, P2_BASE + PUDSEL_ADDR);

   PDEBUG("<= PortInit\n");
 }

 void TwinCanKernelInit(int node){
   PDEBUG("KernelInit =>\n");
   /******************************************************************************/
   /* @Descript.: - Initializes Control/Status Registers of TwinCAN Nodes        */
   /******************************************************************************/
   switch (node) {
     case 0 :
       PDEBUG("Node: %i\n", node);
       /* Node A Control/Status Registers */
       REG_WRITE((CR_REG_HW_RESET_VALUE | CR_REG_CONFIGURATION_CHANGE_ENAB | KernelConstants.GlobalInterruptControlNodeA), ACR_ADDR);
       REG_WRITE(SR_REG_HW_RESET_VALUE, ASR_ADDR);
       REG_WRITE(IR_REG_HW_RESET_VALUE, AIR_ADDR);
       REG_WRITE(KernelConstants.BitTimingNodeA , ABTR_ADDR); //Baudrate
       KernelReg.AGINP.Register = GINP_REG_HW_RESET_VALUE;
 
       KernelReg.AGINP.Bitfield.EINP   = GLOBAL_SRN_NODE_A;
       KernelReg.AGINP.Bitfield.LECINP = GLOBAL_SRN_NODE_A;
       KernelReg.AGINP.Bitfield.TRINP  = GLOBAL_SRN_NODE_A;
       KernelReg.AGINP.Bitfield.CFCINP = GLOBAL_SRN_NODE_A;

       REG_WRITE(KernelReg.AGINP.Register, AGINP_ADDR);

       REG_WRITE(((u32)KernelConstants.FrameCounterModeNodeA) << 16, AFCR_ADDR);
       REG_WRITE(IMR0_REG_ALL_MO_INT_DISPLAYED, AIMR0_ADDR);
       REG_WRITE(IMR4_REG_LAST_ERR_TXRX_ERR_ENAB, AIMR4_ADDR);
       REG_WRITE(ECNT_REG_HW_RESET_VALUE, AECNT_ADDR);
       KernelReg.ACR.Register = REG_READ(ACR_ADDR);

       KernelReg.ACR.Register |= KernelConstants.AnalysingModeNodeA;
       REG_WRITE(KernelReg.ACR.Register, ACR_ADDR);
       break;
      
     case 1 :
       PDEBUG("Node: %i\n", node);
       /* Node B Control/Status Registers */
       REG_WRITE((CR_REG_HW_RESET_VALUE | CR_REG_CONFIGURATION_CHANGE_ENAB | KernelConstants.GlobalInterruptControlNodeB), BCR_ADDR);
       REG_WRITE(SR_REG_HW_RESET_VALUE, BSR_ADDR);
       REG_WRITE(IR_REG_HW_RESET_VALUE, BIR_ADDR);
       REG_WRITE(KernelConstants.BitTimingNodeB, BBTR_ADDR); /* Baudrate */

       KernelReg.BGINP.Register = GINP_REG_HW_RESET_VALUE;

       KernelReg.BGINP.Bitfield.EINP   = GLOBAL_SRN_NODE_B;
       KernelReg.BGINP.Bitfield.LECINP = GLOBAL_SRN_NODE_B;
       KernelReg.BGINP.Bitfield.TRINP  = GLOBAL_SRN_NODE_B;
       KernelReg.BGINP.Bitfield.CFCINP = GLOBAL_SRN_NODE_B;

       REG_WRITE(KernelReg.BGINP.Register, BGINP_ADDR);

       REG_WRITE(((u32)KernelConstants.FrameCounterModeNodeB) << 16, BFCR_ADDR);
       REG_WRITE(IMR0_REG_ALL_MO_INT_DISPLAYED, BIMR0_ADDR);
       REG_WRITE(IMR4_REG_LAST_ERR_TXRX_ERR_ENAB, BIMR4_ADDR);
       REG_WRITE(ECNT_REG_HW_RESET_VALUE, BECNT_ADDR);

       KernelReg.BCR.Register = REG_READ(BCR_ADDR);
       KernelReg.BCR.Register |= KernelConstants.AnalysingModeNodeB;
       REG_WRITE(KernelReg.BCR.Register, BCR_ADDR);
       break;
       
     default :
       PDEBUG("Error! Wrong Node/Nodes requested!\n");
   }
   PDEBUG("<= KernelInit\n");
 }

 void TwinCanMessageObjectInit(int nr, int node, int art, int ident, int format){

   PDEBUG("MessageObjectInit nr:%d =>\n", nr);
   /******************************************************************************/
   /* @Descript.: - Initializes the Message Objects                              */
   /******************************************************************************/

   MessageObject.MSGCTR.Register = MSGCTR_REG_HW_RESET_VALUE & MSGCTR_CTR_MSGVAL_RESET;
   REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR + nr*0x20);    /* disable MO   */

   MessageObject.MSGCFG.Register = MSGCFG_REG_HW_RESET_VALUE;
   MessageObject.MSGCFG.Bitfield.NODE = node;                          /* set NODE     */

   MessageObject.MSGCTR.Bitfield.INTPND = MSGCTR_REG_BITFIELD_OFF;     /* reset INTPND */
   MessageObject.MSGCTR.Bitfield.RMTPND = MSGCTR_REG_BITFIELD_OFF;     /* reset RMTPND */
   MessageObject.MSGCTR.Bitfield.TXRQ   = MSGCTR_REG_BITFIELD_OFF;     /* reset TXRQ   */
   MessageObject.MSGCTR.Bitfield.NEWDAT = MSGCTR_REG_BITFIELD_OFF;     /* reset NEWDAT */

   switch(art){

     case MO_RECEIVE:  MessageObject.MSGCFG.Bitfield.DIR = MO_RECEIVE;
                       MessageObject.MSGCTR.Bitfield.RXIE = MSGCTR_REG_BITFIELD_ON;
                       MessageObject.MSGCTR.Bitfield.TXIE = MSGCTR_REG_BITFIELD_OFF;
                       /******************/
                       /* Interrupt Node */
                       /******************/
                       if(node == NODE_A){
                         MessageObject.MSGCFG.Bitfield.RXINP = MO_RX_NODE_A;
                           }else{
                         MessageObject.MSGCFG.Bitfield.RXINP = MO_RX_NODE_B;
                       }
                       /****************/
                       /* Frame Format */
                       /****************/
                       MessageObject.MSGAMR.Register = REG_READ(MSGAMR_ADDR + nr*0x20);
                       if(format == SFF){/* Standard Frame Format */
                         MessageObject.MSGCFG.Bitfield.XTD = 0;//SFF;
                         if(ident != 0){/* filter specific id */
                           MessageObject.MSGAMR.Register = STANDARD_FRAME_MASK;
                           REG_WRITE((ident << 18), MSGAR_ADDR + nr*0x20);
                         }
                         else{ /* no filter */
                           MessageObject.MSGAMR.Bitfield.SFF_MASK = 0;
                         }
                       }
                       else{ /* Extended Frame Format */
                         MessageObject.MSGCFG.Bitfield.XTD = EFF;
                         if(ident != 0){/* filter specific id */
                           MessageObject.MSGAMR.Register = EXTENDED_FRAME_MASK;
                           REG_WRITE(ident, MSGAR_ADDR + nr*0x20);
                         }
                         else{ /* no filter */
                           MessageObject.MSGAMR.Bitfield.SFF_MASK = 0;
                           MessageObject.MSGAMR.Bitfield.EFF_MASK = 0;
                         }
                       }
                       REG_WRITE(MessageObject.MSGAMR.Register, MSGAMR_ADDR + nr*0x20);
                       MessageObject.MSGCTR.Bitfield.CPUMSG |= MSGCTR_REG_BITFIELD_OFF; /* reset MSGLST */
                       REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR + nr*0x20);
                       break;

     case MO_TRANSMIT: MessageObject.MSGCFG.Bitfield.DIR = MO_TRANSMIT;
                       MessageObject.MSGCTR.Bitfield.TXIE = MSGCTR_REG_BITFIELD_ON;
                       MessageObject.MSGCTR.Bitfield.RXIE = MSGCTR_REG_BITFIELD_OFF;

                       if(node == NODE_A){
                         MessageObject.MSGCFG.Bitfield.TXINP = MO_TX_NODE_A;
                       }else{
                         MessageObject.MSGCFG.Bitfield.TXINP = MO_TX_NODE_B;
                       }

                       REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR + nr*0x20);
                       REG_WRITE(MSGCTR_REG_CPUUPD_SET, MSGCTR_ADDR + nr*0x20); /* set   CPUUPD */
                       break;

     case MO_REMOTE:   MessageObject.MSGCFG.Bitfield.DIR = MO_TRANSMIT;
                       MessageObject.MSGCTR.Bitfield.RXIE = MSGCTR_REG_BITFIELD_ON;
                       if(node == NODE_A){
                         MessageObject.MSGCFG.Bitfield.RXINP = MO_RTR_NODE_A;
                         MessageObject.MSGCFG.Bitfield.TXINP = MO_RTR_NODE_A;
                       }else{
                         MessageObject.MSGCFG.Bitfield.RXINP = MO_RTR_NODE_B;
                         MessageObject.MSGCFG.Bitfield.TXINP = MO_RTR_NODE_B;
                       }

                       REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR + nr*0x20);
                       break;

     default:          printk(KERN_INFO "No valid Message direction\n");
                       break;
   }

   REG_WRITE(MessageObject.MSGCFG.Register, MSGCFG_ADDR + nr*0x20);

   MessageObject.MSGCTR.Bitfield.MSGVAL = MSGCTR_REG_BITFIELD_ON;
   REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR + nr*0x20); /* enable MO    */

   PDEBUG("<= MessageObjectInit nr:%d\n", nr);
 }

 void TwinCanRxTxMaskDefine(int node){

   int i=0, u_mo = used_mo;
   u32 MO_Mask = 0x00000001;

   PDEBUG("RxTxMaskDefine =>\n");

/////ACHTUNG, sollte eine ungleiche Anzahl an MO je Node implementiert werden muss hier geaendert werden!!!!!!!!!!
   if(node == 1) {
     u_mo = used_mo * 2;
     i = used_mo;

     //Verschieben der Maske! Nicht vergessen da ab used_mo gezaehlt wird ;)
     MO_Mask = MO_Mask << used_mo;

     //reset the masks
     InterruptMasks.IntMoTxMaskNodeB    = 0;
     InterruptMasks.IntMoRxMaskNodeB    = 0;
     InterruptMasks.IntMoRxRtrMaskNodeB = 0;
   }
   else {
     //reset the masks
     InterruptMasks.IntMoTxMaskNodeA    = 0;
     InterruptMasks.IntMoRxMaskNodeA    = 0;
     InterruptMasks.IntMoRxRtrMaskNodeA = 0;
   }

/////ACHTUNG!!!!!!!!!!!!

   for(; i<u_mo; i++){
      /* copy the register contends in the structure */
      MessageObject.MSGCTR.Register = REG_READ(MSGCTR_ADDR + i*0x20);
      MessageObject.MSGCFG.Register = REG_READ(MSGCFG_ADDR + i*0x20);

      if(MessageObject.MSGCTR.Bitfield.MSGVAL == MSGCTR_REG_BITFIELD_ON){ /* valid ? */
        if(MessageObject.MSGCFG.Bitfield.NODE == NODE_A){
          /* NODE A */
          if(MessageObject.MSGCFG.Bitfield.DIR == MO_RECEIVE){
            if(MessageObject.MSGCTR.Bitfield.RXIE == MSGCTR_REG_BITFIELD_ON){
              InterruptMasks.IntMoRxMaskNodeA |= MO_Mask; /* Receive Message Object */
              PDEBUG("Maske = Receive Node A\n");
            }
          }

          else{
            if(MessageObject.MSGCTR.Bitfield.TXIE == MSGCTR_REG_BITFIELD_ON){
              InterruptMasks.IntMoTxMaskNodeA |= MO_Mask;  /* Transmit Message Object */
              PDEBUG("Maske = Transmit Node A\n");
            }
            if(MessageObject.MSGCTR.Bitfield.RXIE == MSGCTR_REG_BITFIELD_ON){
              InterruptMasks.IntMoRxRtrMaskNodeA |= MO_Mask; /* Remote Transfer MO */
              PDEBUG("Maske = Remote Node A\n");
            }
          }
        }
        else{
          /* NODE B */

          if(MessageObject.MSGCFG.Bitfield.DIR == MO_RECEIVE){
            if(MessageObject.MSGCTR.Bitfield.RXIE == MSGCTR_REG_BITFIELD_ON){
              InterruptMasks.IntMoRxMaskNodeB |= MO_Mask; /* Receive Message Object */
              PDEBUG("Maske = Receive Node B\n");
            }
          }
          else{
            if(MessageObject.MSGCTR.Bitfield.TXIE == MSGCTR_REG_BITFIELD_ON){
              InterruptMasks.IntMoTxMaskNodeB |= MO_Mask;  /* Transmit Message Object */
              PDEBUG("Maske = Transmit Node B\n");
            }
            if(MessageObject.MSGCTR.Bitfield.RXIE == MSGCTR_REG_BITFIELD_ON){
              InterruptMasks.IntMoRxRtrMaskNodeB |= MO_Mask; /* Remote Transfer MO*/
              PDEBUG("Maske = Remote Node B\n");
            }
          }
        }
      }else{PDEBUG("MO not valid!\n");}/* end if Nodes*/
      MO_Mask = MO_Mask << 1;
   }/* end for */
   PDEBUG("<= RxTxMaskDefine\n");
 }
 void TwinCanInitEnd( int node ){
   /******************************************************************************/
   /* @Descript.: Finishes the TwinCan Device Initialization                     */
   /*             - Defines the Identification Masks for Node specific Interrupts*/
   /*             - Defines Interrupt Service Node Priorities                    */
   /*             - Resets all pending SRN specific Interrupt Request Flags      */
   /*             - Enables Service Nodes according structure "Interrupt Prio"   */
   /*             - Enables both TwinCan Nodes                                   */
   /******************************************************************************/
   int i;

   PDEBUG("InitEnd =>\n");

   TwinCanRxTxMaskDefine(node);

   /***********************************************************************/
   /*   Configuration of Service Request Node Priority and Enable State   */
   /***********************************************************************/
   if(node == 0) {
     for(i=0; i<8; i+=2) {
       ServiceRequestNode.SRC.Register = REG_READ(CanSrnRegAddr[i]);
       if(InterruptPriority[i] != 0x00){
         /* enables SRN */
         REG_WRITE(ServiceRequestNode.SRC.Register | CAN_SRC_SERV_REQUEST_CLEAR, CanSrnRegAddr[i]);
         ServiceRequestNode.SRC.Register     |= CAN_SRC_SERV_REQUEST_CLEAR;
         ServiceRequestNode.SRC.Bitfield.SRPN = InterruptPriority[i];       /* IRQ Number      */
         ServiceRequestNode.SRC.Bitfield.TOS  = TOS_ICU;                    /* IRQ use the CPU */
         ServiceRequestNode.SRC.Register     |= CAN_SRC_SERV_REQUEST_ENAB;  /* enables the IRQ */
         REG_WRITE(ServiceRequestNode.SRC.Register, CanSrnRegAddr[i]);
       }
       else {
         /* disables SRN */
         ServiceRequestNode.SRC.Register &= CAN_SRC_SERV_REQUEST_ENAB;
         ServiceRequestNode.SRC.Register |= CAN_SRC_SERV_REQUEST_CLEAR;
         REG_WRITE(ServiceRequestNode.SRC.Register, CanSrnRegAddr[i]);
       }
     }
   }
   else {
     for(i=1; i<8; i+=2) {
       ServiceRequestNode.SRC.Register = REG_READ(CanSrnRegAddr[i]);
       if(InterruptPriority[i] != 0x00){
         /* enables SRN */
         REG_WRITE(ServiceRequestNode.SRC.Register | CAN_SRC_SERV_REQUEST_CLEAR, CanSrnRegAddr[i]);
         ServiceRequestNode.SRC.Register     |= CAN_SRC_SERV_REQUEST_CLEAR;
         ServiceRequestNode.SRC.Bitfield.SRPN = InterruptPriority[i];       /* IRQ Number      */
         ServiceRequestNode.SRC.Bitfield.TOS  = TOS_ICU;                    /* IRQ use the CPU */
         ServiceRequestNode.SRC.Register     |= CAN_SRC_SERV_REQUEST_ENAB;  /* enables the IRQ */
         REG_WRITE(ServiceRequestNode.SRC.Register, CanSrnRegAddr[i]);
       }
       else {
         /* disables SRN */
         ServiceRequestNode.SRC.Register &= CAN_SRC_SERV_REQUEST_ENAB;
         ServiceRequestNode.SRC.Register |= CAN_SRC_SERV_REQUEST_CLEAR;
         REG_WRITE(ServiceRequestNode.SRC.Register, CanSrnRegAddr[i]);
       }
     }
   }

   /***********************************************************************/
   /*                    Enable of TwinCan Nodes A and B                  */
   /***********************************************************************/
   /* Disables access to TwinCan Node A/B Timing Register and starts operation */
   switch (node) {
     case 0 :
       KernelReg.ACR.Register = REG_READ(ACR_ADDR);
       KernelReg.ACR.Register &= 0xFFFFFFFE;//CR_REG_INIT_AND_CCE_BIT_RESET;
       REG_WRITE(KernelReg.ACR.Register, ACR_ADDR);
       PDEBUG("Node: %i\n", node);
       break;

     case 1 :
       KernelReg.BCR.Register = REG_READ(BCR_ADDR);
       KernelReg.BCR.Register &= CR_REG_INIT_AND_CCE_BIT_RESET;
       REG_WRITE(KernelReg.BCR.Register, BCR_ADDR);
       PDEBUG("Node: %i\n", node);
       break;

     default :
       PDEBUG("Error! Wrong Node/Nodes requested!\n");
   }
   
   PDEBUG("<= InitEnd\n");
 }

 /*###### <=== End HW - Initialisation ######*/

 /*****[ little power ]**************************************************/
 int pow(int basis, int exp){
   int erg = basis;
   int i;
   if(exp == 0) return 1;
   else{
      for(i=1; i<exp; i++)
        erg *= basis;
   }
   return erg;
 }

 /*****[ Find the used MO ]************************************************/
 int find_MO(int node, canid_t rtr) {

   int i=0, mo_nr=-1;

   if(node == NODE_A){
     do{
       if(rtr){
         if( pow(2,i) & InterruptMasks.IntMoRxRtrMaskNodeA ){
           mo_nr=i;
           PDEBUG("Remote Frame Node A Nr.: %d\n", i);
         }
       }
       else{
         if( pow(2,i) & InterruptMasks.IntMoTxMaskNodeA ){
           mo_nr=i;
           PDEBUG("Data Frame Node A Nr.: %d\n", i);
         }
       }
       i++;
     }while(i < used_mo && mo_nr < 0);
   }else{
     i += used_mo;
     do{
       if(rtr) {
         if( pow(2,i) & InterruptMasks.IntMoRxRtrMaskNodeB ){
           mo_nr=i;
           PDEBUG("Remote Frame Node B Nr.: %d\n", i);
         }
       }
       else{
         if( pow(2,i) & InterruptMasks.IntMoTxMaskNodeB ){
           mo_nr=i;
           PDEBUG("Data Frame Node B Nr.: %d\n", i);
         }
       }
       i++;
     }while((i < 16) && (mo_nr < 0));
   }
   return mo_nr;
 }


 /****************************************************************************/
 /*****[ start HW transmit ]**************************************************/
 /****************************************************************************/

 int TwinCan_Transmit(struct net_device *dev,
                      int     node,
                      canid_t id,
                      u8      dlc,
                      u8      data[8])
 {
   int i, mo_nr;
   canid_t rtr = (id & CAN_RTR_FLAG);
   u32 MessageAcceptanceMask;

   struct twincan_priv *priv = (struct twincan_priv*)dev->priv;

   priv->status = 0;

   PDEBUG("HW-Transmit - start -\n");

   /* find the used Message Object */
   mo_nr = find_MO(node, rtr);
   if(mo_nr < 0){
      PDEBUG("Kein MO verfuegbar\n");
      return -EBUSY;
   }
   PDEBUG("Sende auf Node: %d mit MO-Nr: %d\n", node, mo_nr);

   priv->current_MO = mo_nr;

   /* Read old Register Contents */
   MessageObject.MSGCTR.Register = REG_READ(MSGCTR_ADDR+mo_nr*0x20);
   MessageObject.MSGCFG.Register = REG_READ(MSGCFG_ADDR+mo_nr*0x20);

   /***********************************/
   /*             EFF/SFF             */
   /***********************************/
   if(id & CAN_EFF_FLAG){
     /* Extended Frame */
     id &= CAN_EFF_MASK;
     MessageAcceptanceMask = EXTENDED_FRAME_MASK;
     MessageObject.MSGCFG.Bitfield.XTD = EFF;
   }else{
     /* Standard Frame */
     id &= CAN_SFF_MASK;
     MessageAcceptanceMask = STANDARD_FRAME_MASK;
     MessageObject.MSGCFG.Bitfield.XTD = SFF;
     id = id << 18; /* bit [28:18] */
   }

   /***********************************/
   /* Set ID, DLC and Acceptance Mask */
   /***********************************/
   MessageObject.MSGCFG.Bitfield.DLC    = dlc;
   MessageObject.MSGAR.Register         = id;
   MessageObject.MSGAMR.Register        = MessageAcceptanceMask;

   if(rtr){
     /********************************/
     /* Remote Frame Transmit        */
     /********************************/

     PDEBUG("Remote Frame Transmit - start -\n");

     priv->status = MO_REMOTE;

     if(MessageObject.MSGCTR.Bitfield.NEWDAT == MSGCTR_REG_BITFIELD_ON){/* bitwert=10 */
       return -EBUSY; /* MO data not read since last data frame reception! */
     }
     MessageObject.MSGCTR.u16Bitfield.CTR = MSGCTR_CTR_MSGVAL_RESET & /* MSGVAL = OFF */
                                            MSGCTR_CTR_MSGLST_RESET;  /* CPUUPD = ON  */
     REG_WRITE(MessageObject.MSGCTR.Register, MSGCTR_ADDR+mo_nr*0x20);

     REG_WRITE(MessageObject.MSGCFG.Register, MSGCFG_ADDR+mo_nr*0x20);
     REG_WRITE(MessageObject.MSGAR.Register, MSGAR_ADDR+mo_nr*0x20);
     REG_WRITE(MessageObject.MSGAMR.Register, MSGAMR_ADDR+mo_nr*0x20);

     PDEBUG("Remote Frame Transmit - end -\n");
   }
   else{
     /********************************/
     /* Data Frame Transmit          */
     /********************************/

     PDEBUG("Data Frame Transmit - start -\n");

     priv->status = MO_TRANSMIT;

     if(dlc > 8) return -EMSGSIZE;

     /*************************/
     /* Update Data Partition */
     /*************************/

     /* start update */
     REG_WRITE((MSGCTR_REG_CPUUPD_SET & MSGCTR_REG_NEWDAT_SET), MSGCTR_ADDR+mo_nr*0x20);
     REG_WRITE(MessageObject.MSGCFG.Register, MSGCFG_ADDR+mo_nr*0x20); /* DLC */

     /* Write/Calculate message contents */

     for(i=0; i < 8; i++){
       if(i < dlc){
         if(i < 4) MessageObject.MSGDR0.DATA[i]   = (u8)data[i];
         else      MessageObject.MSGDR4.DATA[i-4] = (u8)data[i];
       }
     }

     REG_WRITE(MessageObject.MSGDR0.Register, MSGDR0_ADDR+mo_nr*0x20);
     REG_WRITE(MessageObject.MSGDR4.Register, MSGDR4_ADDR+mo_nr*0x20);

     REG_WRITE(MessageObject.MSGAMR.Register, MSGAMR_ADDR+mo_nr*0x20);
     REG_WRITE(MessageObject.MSGAR.Register, MSGAR_ADDR+mo_nr*0x20); /* ID */

     /* update end */

     REG_WRITE(MSGCTR_REG_CPUUPD_RESET, MSGCTR_ADDR+mo_nr*0x20);

     PDEBUG("Data Frame Transmit - end -\n");
   }

   /* send */
   REG_WRITE(MSGCTR_REG_TXRQ_SET, MSGCTR_ADDR+mo_nr*0x20);

   priv->stats.tx_bytes += dlc;

   PDEBUG("HW-Transmit - end -\n");

   return 0;
 }

 void disable_all_SRN(u8 node){
   int i;
   if(node == 0) {
     for(i=0; i<8; i+=2) {
       ServiceRequestNode.SRC.Register = REG_READ(CanSrnRegAddr[i]);
       ServiceRequestNode.SRC.Register &= CAN_SRC_SERV_REQUEST_ENAB;
       ServiceRequestNode.SRC.Register |= CAN_SRC_SERV_REQUEST_CLEAR;
       REG_WRITE(ServiceRequestNode.SRC.Register, CanSrnRegAddr[i]);
     }
   }
   else {
     for(i=1; i<8; i+=2) {
       ServiceRequestNode.SRC.Register = REG_READ(CanSrnRegAddr[i]);
       ServiceRequestNode.SRC.Register &= CAN_SRC_SERV_REQUEST_ENAB;
       ServiceRequestNode.SRC.Register |= CAN_SRC_SERV_REQUEST_CLEAR;
       REG_WRITE(ServiceRequestNode.SRC.Register, CanSrnRegAddr[i]);
     }
   }
 }
