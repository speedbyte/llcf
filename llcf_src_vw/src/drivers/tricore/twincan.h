/*
 *  $Id: twincan.h,v 1.4 2006/02/03 14:49:49 hartko Exp $
 */

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

 /*******************************************************************************/
 /* @Prototypes                                                                 */
 /*******************************************************************************/

 /* Linux specific */
 int twincan_init(struct net_device *dev);
 int twincan_open(struct net_device *dev);
 int twincan_release(struct net_device *dev);
 void twincan_tx_timeout(struct net_device *dev);
 int twincan_start_xmit(struct sk_buff *skb, struct net_device *dev);
 struct net_device_stats *twincan_stats(struct net_device *dev);
 void twincan_rx(struct net_device *dev, int mo_nr);
 //int twincan_init_module(void);


 /* Hardware specific */
 void protect_wdcon(void);
 void unprotect_wdcon(void);
 void TwinCanInitStart(int, u8);
 void TwinCanPortInit();
 void TwinCanKernelInit(int);
 void TwinCanMessageObjectInit(int nr, int node, int art, int ident, int format);
 void TwinCanInitEnd(int);
 int TwinCan_Transmit(struct net_device *dev,
                      int     node,
                      canid_t id,
                      u8      dlc,
                      u8      data[8]);
 void disable_all_SRN(u8);

 //little functions
 //u8 strcmp(const char *s1, const char *s2);
 u32 reg_read(int);
 void reg_write(u32, int);
 void display_LEC(struct net_device *dev, int);
 void clean_TXIPND(u8);
 int clean_RXIPND(u8);
 void fill_KernelConstants();

 #define REG_READ(offset)           reg_read(offset)
 #define REG_WRITE(value, offset)   reg_write(value, offset)

 /******************************************************************************/
 /* @Defines                                                                   */
 /******************************************************************************/
 #define TWINCAN_IO_LEN 0xBFF
 #define CAN_DEV_NAME "can%d"
 #define MAC_ADDR "\0SNUL0"
 #define TX_TIMEOUT (1 * HZ)
 #define MSGCTR_REG_BITFIELD_ON  2
 #define MSGCTR_REG_BITFIELD_OFF 1
 #define NODE_A 0
 #define NODE_B 1
 #define MO_RECEIVE  0
 #define MO_TRANSMIT 1
 #define MO_REMOTE   2

 /* Device Status */
 #define TWINCAN_TX 0
 #define TWINCAN_RX 1


 #define EXTENDED_FRAME_MASK 0xFFFFFFFF
 #define STANDARD_FRAME_MASK 0xFFFC0000
 #define SFF 0
 #define EFF 1

 /* Type of Service Control */
 #define TOS_ICU  0 /* CPU */
 #define TOS_PICU 2 /* PCP */

 #define _WDTCON0  0xF0000020
 #define _WDTCON1  0xF0000024

 //Debugging aus- und einschalten
 #ifdef TWINCAN_DEBUG
  #define PDEBUG(fmt, args...) printk(KERN_WARNING "TwinCAN: " fmt, ## args)
 #else
  #define PDEBUG(fmt, args...) // kein Debugging: nichts machen
 #endif

 #define TC_CLOCK 50000000

 /******************************************************************************/
 /* @Offset Addresses                                                          */
 /******************************************************************************/

  /* External Control Registers */
 #define CAN_CLC_ADDR 0x000
 #define CAN_ID_ADDR  0x008

  /* TwinCAN Kernel Registers */
  /* Node A */
 #define ACR_ADDR    0x200
 #define ASR_ADDR    0x204
 #define AIR_ADDR    0x208
 #define ABTR_ADDR   0x20C
 #define AGINP_ADDR  0x210
 #define AFCR_ADDR   0x214
 #define AIMR0_ADDR  0x218
 #define AIMR4_ADDR  0x21C
 #define AECNT_ADDR  0x220
  /* Node B */
 #define BCR_ADDR    0x240
 #define BSR_ADDR    0x244
 #define BIR_ADDR    0x248
 #define BBTR_ADDR   0x24C
 #define BGINP_ADDR  0x250
 #define BFCR_ADDR   0x254
 #define BIMR0_ADDR  0x258
 #define BIMR4_ADDR  0x25C
 #define BECNT_ADDR  0x260
  /* Control/Status Registers */
 #define RXIPND_ADDR 0x284
 #define TXIPND_ADDR 0x288

  /* Message Object Registers */
 #define MSGDR0_ADDR  0x300
 #define MSGDR4_ADDR  0x304
 #define MSGAR_ADDR   0x308
 #define MSGAMR_ADDR  0x30C
 #define MSGCTR_ADDR  0x310
 #define MSGCFG_ADDR  0x314
 #define MSGFGCR_ADDR 0x318

  /* Service Request Nodes Register */
 #define CAN_SRC7_ADDR 0xAE0
 #define CAN_SRC6_ADDR 0xAE4
 #define CAN_SRC5_ADDR 0xAE8
 #define CAN_SRC4_ADDR 0xAEC
 #define CAN_SRC3_ADDR 0xAF0
 #define CAN_SRC2_ADDR 0xAF4
 #define CAN_SRC1_ADDR 0xAF8
 #define CAN_SRC0_ADDR 0xAFC

 /* connect IRQ-Source with SRN */
 #define GLOBAL_SRN_NODE_A  0 /* SRN for LEC,ERWN,BOFF,CFCOV,TXOK and RXOK on Node A */
 #define GLOBAL_SRN_NODE_B  1 /* SRN for LEC,ERWN,BOFF,CFCOV,TXOK and RXOK on Node B */
 #define MO_TX_NODE_A       2 /* SRN for all Node A MO Tx Interrupts */
 #define MO_TX_NODE_B       3 /* SRN for all Node B MO Tx Interrupts */
 #define MO_RX_NODE_A       4 /* SRN for all Node A MO Rx Interrupts */
 #define MO_RX_NODE_B       5 /* SRN for all Node B MO Rx Interrupts */
 #define MO_RTR_NODE_A      6 /* SRN for all Node A MO Rtr Interrupts */
 #define MO_RTR_NODE_B      7 /* SRN for all Node B MO Rtr Interrupts */

 /***********************************************************************/
 /* ------- Port specific ------- 								  */
 /***********************************************************************/
 #define P0_BASE   (0xF0002800)
 #define P1_BASE   (0xF0002900)
 #define P2_BASE   (0xF0002A00)
 #define P3_BASE   (0xF0002B00)
 #define P5_BASE   (0xF0002C00)

 #define P2_ADDR_RANGE 0xff

 /* Offset Addresses */
 #define OUT_ADDR     0x010  /* Data Output          */
 #define IN_ADDR      0x014  /* Data Input           */
 #define DIR_ADDR     0x018  /* Direction            */
 #define OD_ADDR      0x01C  /* Open Drain Mode      */
 #define PUDSEL_ADDR  0x028  /* Pull-up/-down Select */
 #define PUDEN_ADDR   0x02C  /* Pull-up/-down Enable */
 #define ALTSEL0_ADDR 0x044  /* Alternate Select     */


 /* Access Mode:
 R  - Read-only register.
 32 - Only 32-bit word accesses are permitted to that register/address range.
 E  - Endinit protected register/address.
 PW - Password protected register/address.
 For more details refer to specification.
 */

 struct PORT
 {
        volatile u32 RESERVED0[4];   /* Reserved */
        volatile u32 OUT;            /* Port Output Data Register */
        volatile u32 INP;            /* Port Input Data Register */
        volatile u32 DIR;            /* Port Direction Control Register */
        volatile u32 RESERVED1[3];   /* Reserved */
        volatile u32 PUDSEL;         /* Port Pull Up / Down Select Register */
        volatile u32 PUDEN;          /* Port Pull Up / Down Enable Register */
        volatile u32 POCON0;         /* Port Output Characteristic Control Register 0 */
        volatile u32 POCON1;         /* Port Output Characteristic Control Register 1 */
        volatile u32 POCON2;         /* Port Output Characteristic Control Register 2 */
        volatile u32 POCON3;         /* Port Output Characteristic Control Register 3 */
        volatile u32 PICON;          /* Port Input Configuration Register */
        volatile u32 ALTSEL0;        /* Port Alternate Select Register 0 */
        volatile u32 ALTSEL1;        /* Port Alternate Select Register 1 */
 };

 struct PORT pstP2;

 #define PX0	0x00000001	/* Port Pin 0 */
 #define PX1	0x00000002	/* Port Pin 1 */
 #define PX2	0x00000004	/* Port Pin 2 */
 #define PX3	0x00000008	/* Port Pin 3 */
 #define PX4	0x00000010	/* Port Pin 4 */
 #define PX5	0x00000020	/* Port Pin 5 */
 #define PX6	0x00000040	/* Port Pin 6 */
 #define PX7	0x00000080	/* Port Pin 7 */
 #define PX8	0x00000100	/* Port Pin 8 */
 #define PX9	0x00000200	/* Port Pin 9 */
 #define PX10	0x00000400	/* Port Pin 10 */
 #define PX11	0x00000800	/* Port Pin 11 */
 #define PX12	0x00001000	/* Port Pin 12 */
 #define PX13	0x00002000	/* Port Pin 13 */
 #define PX14	0x00004000	/* Port Pin 14 */
 #define PX15	0x00008000	/* Port Pin 15 */
 #define PX_LO	0x00000000	/* Port Pin 0..15 */
 #define PX_HI	0x0000FFFF	/* Port Pin 0..15 */

 /* Port Output Characteristic Control Register 0 */
 #define PPIN0DC_MSK	0x00000003	/* Port Pin 0 Driver Characteristic Control */
 #define PPIN0EC_MSK	0x0000000C	/* Port Pin 0 Edge Characteristic Control */
 #define PPIN1DC_MSK	0x00000030	/* Port Pin 1 Driver Characteristic Control */
 #define PPIN1EC_MSK	0x000000C0	/* Port Pin 1 Edge Characteristic Control */
 #define PPIN2DC_MSK	0x00000300	/* Port Pin 2 Driver Characteristic Control */
 #define PPIN2EC_MSK	0x00000C00	/* Port Pin 2 Edge Characteristic Control */
 #define PPIN3DC_MSK	0x00003000	/* Port Pin 3 Driver Characteristic Control */
 #define PPIN3EC_MSK	0x0000C000	/* Port Pin 3 Edge Characteristic Control */

 /* Port Output Characteristic Control Register 1 */
 #define PPIN4DC_MSK	0x00000003	/* Port Pin 4 Driver Characteristic Control */
 #define PPIN4EC_MSK	0x0000000C	/* Port Pin 4 Edge Characteristic Control */
 #define PPIN5DC_MSK	0x00000030	/* Port Pin 5 Driver Characteristic Control */
 #define PPIN5EC_MSK	0x000000C0	/* Port Pin 5 Edge Characteristic Control */
 #define PPIN6DC_MSK	0x00000300	/* Port Pin 6 Driver Characteristic Control */
 #define PPIN6EC_MSK	0x00000C00	/* Port Pin 6 Edge Characteristic Control */
 #define PPIN7DC_MSK	0x00003000	/* Port Pin 7 Driver Characteristic Control */
 #define PPIN7EC_MSK	0x0000C000	/* Port Pin 7 Edge Characteristic Control */

 /* Port Output Characteristic Control Register 2 */
 #define PPIN8DC_MSK	0x00000003	/* Port Pin 8 Driver Characteristic Control */
 #define PPIN8EC_MSK	0x0000000C	/* Port Pin 8 Edge Characteristic Control */
 #define PPIN9DC_MSK	0x00000030	/* Port Pin 9 Driver Characteristic Control */
 #define PPIN9EC_MSK	0x000000C0	/* Port Pin 9 Edge Characteristic Control */

 #define PPIN10DC_MSK	0x00000300	/* Port Pin 10 Driver Characteristic Control */
 #define PPIN10EC_MSK	0x00000C00	/* Port Pin 10 Edge Characteristic Control */
 #define PPIN11DC_MSK	0x00003000	/* Port Pin 11 Driver Characteristic Control */
 #define PPIN11EC_MSK	0x0000C000	/* Port Pin 11 Edge Characteristic Control */

 /* Port Output Characteristic Control Register 3 */
 #define PPIN12DC_MSK	0x00000003	/* Port Pin 12 Driver Characteristic Control */
 #define PPIN12EC_MSK	0x0000000C	/* Port Pin 12 Edge Characteristic Control */
 #define PPIN13DC_MSK	0x00000030	/* Port Pin 13 Driver Characteristic Control */
 #define PPIN13EC_MSK	0x000000C0	/* Port Pin 13 Edge Characteristic Control */
 #define PPIN14DC_MSK	0x00000300	/* Port Pin 14 Driver Characteristic Control */
 #define PPIN14EC_MSK	0x00000C00	/* Port Pin 14 Edge Characteristic Control */
 #define PPIN15DC_MSK	0x00003000	/* Port Pin 15 Driver Characteristic Control */
 #define PPIN15EC_MSK	0x0000C000	/* Port Pin 15 Edge Characteristic Control */

 /* for all POCON Register */
 #define PPINDC_HIGH	0x00000000	/* high current mode */
 #define PPINDC_DYN 	0x00001111	/* dynamic current mode */
 #define PPINDC_LOW 	0x00002222	/* low current mode */
 #define PPINEC_NORM	0x00000000	/* normal timing */
 #define PPINEC_SLOW	0x00004444	/* slow timing */

 /***********************************************************************/
 /*               Structure of TwinCan Register Groups                  */
 /***********************************************************************/
 /*typedef volatile*/ struct TwinCanKernelRegister
 {
     union
     {
        u32 Register;
        struct
        {
               u32 DISR:1;       /* Bit0 */
               u32 DISS:1;       /* Bit1 */
               u32 SPEN:1;       /* Bit2 */
               u32 EDIS:1;       /* Bit3 */
               u32 SBWE:1;       /* Bit4 */
               u32 Reserved1:3;  /* Bit[5:7]   */
               u32 RMC :8;       /* Bit[8:15]  */
               u32 Reserved2:16; /* Bit[16:31] */
        } Bitfield;
     }CLC;  /* relative address: + 0x000 */

     union
        {
        u32 Register;
        struct
        {
               u32 REVISION:8;  /* Bit[0: 7]  */
               u32 ID :24;      /* Bit[8:32]  */
        } Bitfield;
     }ID;  /* relative address: + 0x008 */

     union
        {
        u32 Register;

        struct
           {
           u32 INIT:1;       /* Bit0      */
           u32 Reserved11:1; /* Bit1      */
           u32 SIE:1;        /* Bit2      */
           u32 EIE:1;        /* Bit3      */
           u32 LECIE:1;      /* Bit4      */
           u32 Reserved12:1; /* Bit5      */
           u32 CCE:1;        /* Bit6      */
           u32 CALM:1;       /* Bit7      */
           u32 Reserved13:24;/* Bit[8:31] */
           } Bitfield;

        } ACR; /* relative address: + 0x200 */

     union
        {
        u32 Register;

        struct
            {
            u32 LEC :3;       /* Bit[0:2]  */
            u32 TXOK:1;       /* Bit3      */
            u32 RXOK:1;       /* Bit4      */
            u32 Reserved14:1; /* Bit5      */
            u32 EWRN:1;       /* Bit6      */
            u32 BOFF:1;       /* Bit7      */
            u32 Reserved15:24;/* Bit[8:31] */
            } Bitfield;
        } ASR; /* relative address: + 0x204 */

     union
        {
        u32 Register;

        struct
            {
            u32 INTID :8;     /* Bit[0: 7] */
            u32 Reserved16:24;/* Bit[8:31] */
            } Bitfield;

        } AIR; /* relative address: + 0x208 */

     union
        {
        u32 Register;

        struct
            {
            u32 BRP  :6;      /* Bit[ 0: 5] */
            u32 SJW  :2;      /* Bit[ 6: 7] */
            u32 TSEG1:4;      /* Bit[ 8:11] */
            u32 TSEG2:3;      /* Bit[12:14] */
            u32 DIV8X:1;      /* Bit 15     */
            u32 LBM  :1;      /* Bit 16     */
            u32 Reserved17:15;/* Bit[17:31] */
            } Bitfield;

        } ABTR; /* relative address: + 0x20C */

     union
        {
        u32 Register;

        struct
            {
            u32 EINP  :3;     /* Bit[ 0: 2] */
            u32 Reserved18:1; /* Bit  3     */
            u32 LECINP:3;     /* Bit[ 4: 6] */
            u32 Reserved19:1; /* Bit  7     */
            u32 TRINP :3;     /* Bit[ 8:10] */
            u32 Reserved20:1; /* Bit 11     */
            u32 CFCINP:3;     /* Bit[12:14] */
            u32 Reserved21:1; /* Bit[15:31] */
            } Bitfield;

        } AGINP; /* relative address: + 0x210 */

     union
        {
        u32 Register;

        struct
            {
            u32 CFC   :16;    /* Bit[ 0:15] */
            u32 CFCMD :4;     /* Bit[16:19] */
            u32 Reserved22:2; /* Bit[20:21] */
            u32 CFCIE :1;     /* Bit 22     */
            u32 CFCOV :1;     /* Bit 23     */
            u32 Reserved23:8; /* Bit[24:31] */
            } Bitfield;

        } AFCR; /* relative address: + 0x214 */

     union
        {
        u32 Register;

        struct
            {
            u32 IMC0 :1;   /* Bit0  */
            u32 IMC1 :1;   /* Bit1  */
            u32 IMC2 :1;   /* Bit2  */
            u32 IMC3 :1;   /* Bit3  */
            u32 IMC4 :1;   /* Bit4  */
            u32 IMC5 :1;   /* Bit5  */
            u32 IMC6 :1;   /* Bit6  */
            u32 IMC7 :1;   /* Bit7  */
            u32 IMC8 :1;   /* Bit8  */
            u32 IMC9 :1;   /* Bit9  */
            u32 IMC10:1;   /* Bit10 */
            u32 IMC11:1;   /* Bit11 */
            u32 IMC12:1;   /* Bit12 */
            u32 IMC13:1;   /* Bit13 */
            u32 IMC14:1;   /* Bit14 */
            u32 IMC15:1;   /* Bit15 */
            u32 IMC16:1;   /* Bit16 */
            u32 IMC17:1;   /* Bit17 */
            u32 IMC18:1;   /* Bit18 */
            u32 IMC19:1;   /* Bit19 */
            u32 IMC20:1;   /* Bit20 */
            u32 IMC21:1;   /* Bit21 */
            u32 IMC22:1;   /* Bit22 */
            u32 IMC23:1;   /* Bit23 */
            u32 IMC24:1;   /* Bit24 */
            u32 IMC25:1;   /* Bit25 */
            u32 IMC26:1;   /* Bit26 */
            u32 IMC27:1;   /* Bit27 */
            u32 IMC28:1;   /* Bit28 */
            u32 IMC29:1;   /* Bit29 */
            u32 IMC30:1;   /* Bit30 */
            u32 IMC31:1;   /* Bit31 */
            } Bitfield;

        } AIMR0; /* relative address: + 0x218 */

     union
        {
        u32 Register;

        struct
            {
            u32 IMC32:1;       /* Bit1  */
            u32 IMC33:1;       /* Bit2  */
            u32 IMC34:1;       /* Bit3  */
            u32 Reserved24:29; /* Bit[3:31] */
            } Bitfield;

        } AIMR4;  /* relative address: + 0x21C */

     union
        {
        u32 Register;

        struct
            {
            u32 REC:8;         /* Bit[ 0: 7] */
            u32 TEC:8;         /* Bit[ 8:15] */
            u32 EWRNLVL:8;     /* Bit[16:23] */
            u32 LETD:1;        /* Bit 24     */
            u32 LEINC:1;       /* Bit 25     */
            u32 Reserved25:6;  /* Bit[26:31] */
            } Bitfield;

        } AECNT;  /* relative address: + 0x220 */

     union
        {
        u32 Register;

        struct
            {
            u32 INIT:1;       /* Bit0      */
            u32 Reserved31:1; /* Bit1      */
            u32 SIE:1;        /* Bit2      */
            u32 EIE:1;        /* Bit3      */
            u32 LECIE:1;      /* Bit4      */
            u32 Reserved32:1; /* Bit5      */
            u32 CCE:1;        /* Bit6      */
            u32 CALM:1;       /* Bit7      */
            u32 Reserved33:24;/* Bit[8:31] */
            } Bitfield;

        } BCR;  /* relative address: + 0x240 */

     union
        {
        u32 Register;

        struct
            {
            u32 LEC :3;       /* Bit[0:2]  */
            u32 TXOK:1;       /* Bit3      */
            u32 RXOK:1;       /* Bit4      */
            u32 Reserved34:1; /* Bit5      */
            u32 EWRN:1;       /* Bit6      */
            u32 BOFF:1;       /* Bit7      */
            u32 Reserved35:24;/* Bit[8:31] */
            } Bitfield;

        } BSR; /* relative address: + 0x244 */

     union
        {
        u32 Register;

        struct
            {
            u32 INTID :8;     /* Bit[0:7]  */
            u32 Reserved36:24;/* Bit[8:31] */
            } Bitfield;

        } BIR; /* relative address: + 0x248 */

     union
        {
        u32 Register;

        struct
            {
            u32 BRP  :6;      /* Bit[0:5]   */
            u32 SJW  :2;      /* Bit[6:7]   */
            u32 TSEG1:4;      /* Bit[8:11]  */
            u32 TSEG2:3;      /* Bit[12:14] */
            u32 DIV8X:1;      /* Bit 15     */
            u32 Reserved37:16;/* Bit[16:31] */
            } Bitfield;

        } BBTR; /* relative address: + 0x24C */

     union
        {
        u32 Register;

        struct
            {
            u32 EINP  :3;     /* Bit[0:2]   */
            u32 Reserved38:1; /* Bit3       */
            u32 LECINP:3;     /* Bit[4:6]   */
            u32 Reserved39:1; /* Bit7       */
            u32 TRINP :3;     /* Bit[8:10]  */
            u32 Reserved40:1; /* Bit11      */
            u32 CFCINP:3;     /* Bit[12:14] */
            u32 Reserved41:1; /* Bit[15:31] */
            } Bitfield;

        } BGINP; /* relative address: + 0x250 */

     union
        {
        u32 Register;

        struct
            {
            u32 CFC   :16;    /* Bit[ 0:15] */
            u32 CFCMD :4;     /* Bit[16:19] */
            u32 Reserved42:2; /* Bit[20:21] */
            u32 CFCIE :1;     /* Bit 22     */
            u32 CFCOV :1;     /* Bit 23     */
            u32 Reserved43:8; /* Bit[24:31] */
            } Bitfield;

        } BFCR; /* relative address: + 0x254 */

     union
        {
        u32 Register;

        struct
            {
            u32 IMC0 :1;   /* Bit0  */
            u32 IMC1 :1;   /* Bit1  */
            u32 IMC2 :1;   /* Bit2  */
            u32 IMC3 :1;   /* Bit3  */
            u32 IMC4 :1;   /* Bit4  */
            u32 IMC5 :1;   /* Bit5  */
            u32 IMC6 :1;   /* Bit6  */
            u32 IMC7 :1;   /* Bit7  */
            u32 IMC8 :1;   /* Bit8  */
            u32 IMC9 :1;   /* Bit9  */
            u32 IMC10:1;   /* Bit10 */
            u32 IMC11:1;   /* Bit11 */
            u32 IMC12:1;   /* Bit12 */
            u32 IMC13:1;   /* Bit13 */
            u32 IMC14:1;   /* Bit14 */
            u32 IMC15:1;   /* Bit15 */
            u32 IMC16:1;   /* Bit16 */
            u32 IMC17:1;   /* Bit17 */
            u32 IMC18:1;   /* Bit18 */
            u32 IMC19:1;   /* Bit19 */
            u32 IMC20:1;   /* Bit20 */
            u32 IMC21:1;   /* Bit21 */
            u32 IMC22:1;   /* Bit22 */
            u32 IMC23:1;   /* Bit23 */
            u32 IMC24:1;   /* Bit24 */
            u32 IMC25:1;   /* Bit25 */
            u32 IMC26:1;   /* Bit26 */
            u32 IMC27:1;   /* Bit27 */
            u32 IMC28:1;   /* Bit28 */
            u32 IMC29:1;   /* Bit29 */
            u32 IMC30:1;   /* Bit30 */
            u32 IMC31:1;   /* Bit31 */
            } Bitfield;

        } BIMR0; /* relative address: + 0x258 */

     union
        {
        u32 Register;

        struct
            {
            u32 IMC32:1;       /* Bit0  */
            u32 IMC33:1;       /* Bit1  */
            u32 IMC34:1;       /* Bit2  */
            u32 Reserved44:29; /* Bit[3:31] */
            } Bitfield;

        } BIMR4; /* relative address: + 0x25C */


     union
        {
        u32 Register;

        struct
            {
            u32 REC:8;         /* Bit[ 0: 7] */
            u32 TEC:8;         /* Bit[ 8:15] */
            u32 EWRNLVL:8;     /* Bit[16:23] */
            u32 LETD:1;        /* Bit 24     */
            u32 LEINC:1;       /* Bit 25     */
            u32 Reserved45:6;  /* Bit[26:31] */
            } Bitfield;

        } BECNT;  /* relative address: + 0x260 */

     union
        {
        u32 Register;

        struct
            {
            u32 RXIPND0 :1;   /* Bit0  */
            u32 RXIPND1 :1;   /* Bit1  */
            u32 RXIPND2 :1;   /* Bit2  */
            u32 RXIPND3 :1;   /* Bit3  */
            u32 RXIPND4 :1;   /* Bit4  */
            u32 RXIPND5 :1;   /* Bit5  */
            u32 RXIPND6 :1;   /* Bit6  */
            u32 RXIPND7 :1;   /* Bit7  */
            u32 RXIPND8 :1;   /* Bit8  */
            u32 RXIPND9 :1;   /* Bit9  */
            u32 RXIPND10:1;   /* Bit10 */
            u32 RXIPND11:1;   /* Bit11 */
            u32 RXIPND12:1;   /* Bit12 */
            u32 RXIPND13:1;   /* Bit13 */
            u32 RXIPND14:1;   /* Bit14 */
            u32 RXIPND15:1;   /* Bit15 */
            u32 RXIPND16:1;   /* Bit16 */
            u32 RXIPND17:1;   /* Bit17 */
            u32 RXIPND18:1;   /* Bit18 */
            u32 RXIPND19:1;   /* Bit19 */
            u32 RXIPND20:1;   /* Bit20 */
            u32 RXIPND21:1;   /* Bit21 */
            u32 RXIPND22:1;   /* Bit22 */
            u32 RXIPND23:1;   /* Bit23 */
            u32 RXIPND24:1;   /* Bit24 */
            u32 RXIPND25:1;   /* Bit25 */
            u32 RXIPND26:1;   /* Bit26 */
            u32 RXIPND27:1;   /* Bit27 */
            u32 RXIPND28:1;   /* Bit28 */
            u32 RXIPND29:1;   /* Bit29 */
            u32 RXIPND30:1;   /* Bit30 */
            u32 RXIPND31:1;   /* Bit31 */
            } Bitfield;

        } RXIPND; /* relative address: + 0x284 */

     union
        {
        u32 Register;

        struct
            {
            u32 TXIPND0 :1;   /* Bit0  */
            u32 TXIPND1 :1;   /* Bit1  */
            u32 TXIPND2 :1;   /* Bit2  */
            u32 TXIPND3 :1;   /* Bit3  */
            u32 TXIPND4 :1;   /* Bit4  */
            u32 TXIPND5 :1;   /* Bit5  */
            u32 TXIPND6 :1;   /* Bit6  */
            u32 TXIPND7 :1;   /* Bit7  */
            u32 TXIPND8 :1;   /* Bit8  */
            u32 TXIPND9 :1;   /* Bit9  */
            u32 TXIPND10:1;   /* Bit10 */
            u32 TXIPND11:1;   /* Bit11 */
            u32 TXIPND12:1;   /* Bit12 */
            u32 TXIPND13:1;   /* Bit13 */
            u32 TXIPND14:1;   /* Bit14 */
            u32 TXIPND15:1;   /* Bit15 */
            u32 TXIPND16:1;   /* Bit16 */
            u32 TXIPND17:1;   /* Bit17 */
            u32 TXIPND18:1;   /* Bit18 */
            u32 TXIPND19:1;   /* Bit19 */
            u32 TXIPND20:1;   /* Bit20 */
            u32 TXIPND21:1;   /* Bit21 */
            u32 TXIPND22:1;   /* Bit22 */
            u32 TXIPND23:1;   /* Bit23 */
            u32 TXIPND24:1;   /* Bit24 */
            u32 TXIPND25:1;   /* Bit25 */
            u32 TXIPND26:1;   /* Bit26 */
            u32 TXIPND27:1;   /* Bit27 */
            u32 TXIPND28:1;   /* Bit28 */
            u32 TXIPND29:1;   /* Bit29 */
            u32 TXIPND30:1;   /* Bit30 */
            u32 TXIPND31:1;   /* Bit31 */
            } Bitfield;

        } TXIPND; /* relative address: + 0x288 */

     };// TwinCanKernelRegister  ;

 /************************************************************************/
 /*            Structure of the Message Object Registers                 */
 /************************************************************************/
 /*typedef volatile*/ struct TwinCanMessageObjectRegister
     {
     union
        {
        u32 Register;        
        u8 DATA[4];
        } MSGDR0;



     union
        {
        u32 Register;
        u8 DATA[4];
        } MSGDR4;

     union
        {
        u32 Register;

        struct
            {
            u32 ID:29;        /* Bit[ 0:28]  */
            u32 Reserved51:3; /* Bit[29:31] */
            } Bitfield;

        } MSGAR;

     union
        {
        u32 Register;

        struct
            {
            u32 EFF_MASK:18;        /* Bit[ 0:17]  */
            u32 SFF_MASK:11;        /* Bit[ 18:28]  */
            u32 Reserved52:3; /* Bit[29:31] */
            } Bitfield;

        } MSGAMR;

     union
        {
        u32 Register;

        struct
            {
            u16 CTR; /* Bit[0:15]  => 8 * 2 Control Fields            */
            u16 CFC; /* Bit[16:31] => 16 Bit Frame Counter Value Copy */
            } u16Bitfield;
            
        struct
            {
            u32 INTPND :2; /* Bit[ 0: 1]  */
            u32 RXIE   :2; /* Bit[ 2: 3]  */
            u32 TXIE   :2; /* Bit[ 4: 5]  */
            u32 MSGVAL :2; /* Bit[ 6: 7]  */

            u32 NEWDAT :2; /* Bit[ 8: 9]  */
            u32 CPUMSG :2; /* Bit[10:11]  */
            u32 TXRQ   :2; /* Bit[12:13]  */
            u32 RMTPND :2; /* Bit[14:15]  */
            u32 CFCVAL :16;/* Bit[16:31]  */
            } Bitfield;

        } MSGCTR;

     union
        {
        u32 Register;

        struct
            {
            u32 RMM       :1; /* Bit0        */
            u32 NODE      :1; /* Bit1        */
            u32 XTD       :1; /* Bit2        */
            u32 DIR       :1; /* Bit3        */
            u32 DLC       :4; /* Bit[ 4: 7]  */
            u32 Reserved53:8; /* Bit[ 8:15]  */
            u32 RXINP     :3; /* Bit[16:18]  */
            u32 Reserved54:1; /* Bit19       */
            u32 TXINP     :3; /* Bit[20:22]  */
            u32 Reserved55:1; /* Bit[23:31]  */
            } Bitfield;

        } MSGCFG;

     union
        {
        u32 Register;

        struct
            {
            u32 FSIZE     :5; /* Bit[ 0: 4]  */
            u32 Reserved56:8; /* Bit[ 5:12]  */
            u32 FD        :1; /* Bit 13      */
            u32 SDT       :1; /* Bit 14      */
            u32 STT       :1; /* Bit 15      */
            u32 CANPTR    :5; /* Bit[16:20]  */
            u32 Reserved58:3; /* Bit[21:23]  */
            u32 MMC       :3; /* Bit[24:26]  */
            u32 Reserved59:5; /* Bit[27:31]  */
            } FifoBitfield;

        struct
            {
            u32 FSIZE     :5; /* Bit[ 0: 4]  */
            u32 Reserved60:3; /* Bit[ 5: 7]  */
            u32 GDFS      :1; /* Bit  8      */
            u32 SRREN     :1; /* Bit  9      */
            u32 IDC       :1; /* Bit 10      */
            u32 DLCC      :1; /* Bit 11      */
            u32 Reserved61:3; /* Bit[12:14]  */
            u32 STT       :1; /* Bit 15      */
            u32 CANPTR    :5; /* Bit[16:20]  */
            u32 Reserved62:3; /* Bit[21:23]  */
            u32 MMC       :3; /* Bit[24:26]  */
            u32 Reserved63:5; /* Bit[27:31]  */
            } GatewayBitfield;

        } MSGFGCR;

     u32  ReservedMemorySpaceFive;

     } ;//TwinCanMessageObjectRegister ;

 /****************************************************************/
 /*            TwinCan Service Request Node Register             */
 /****************************************************************/
 /*typedef volatile*/struct TwinCanServiceRequestNodeRegister
     {
     union
        {
        u32 Register;
        struct
            {
            u32 SRPN      :8; /* Bit[ 0: 7]  */
            u32 Reserved60:2; /* Bit[ 8: 9]  */
            u32 TOS       :2; /* Bit[10:11]  */
            u32 SRE       :1; /* Bit12       */
            u32 SRR       :1; /* Bit13       */
            u32 CLRR      :1; /* Bit14       */
            u32 SETR      :1; /* Bit15       */
            u32 Reserved61:16;/* Bit[16:31]  */
            } Bitfield;
        } SRC;
  };//TwinCanServiceRequestNodeRegister;


 /******************************************************************************/
 /* @Enums                                                                     */
 /******************************************************************************/

 enum TwinCanErrStatus{
     TWINCAN_LEC                = 0x00,
     TWINCAN_EWRN_STATE_ERROR   = 0x01,
     TWINCAN_BOFF_STATE_ERROR   = 0x02
 };

 enum
     {
     BAUD_1000_000_WITH_MHZ_10  = 0x000000A5,/*10 Quanta,SJW = 0,Sample at 80%*/
     BAUD_0500_000_WITH_MHZ_10  = 0x00003E40,
     BAUD_0250_000_WITH_MHZ_10  = 0x00003E41,
     BAUD_0125_000_WITH_MHZ_10  = 0x00003A44,
     BAUD_0100_000_WITH_MHZ_10  = 0x00003E5D,
     BAUD_0062_500_WITH_MHZ_10  = 0x00003A49,
     BAUD_0050_000_WITH_MHZ_10  = 0x00003E49,
     BAUD_0015_625_WITH_MHZ_10  = 0x00003A67,//163F

     BAUD_1000_000_WITH_MHZ_16  = 0x00002B00,/*16 Quanta,SJW = 0,Sample at 81%*/
     BAUD_0500_000_WITH_MHZ_16  = 0x00002B01,
     BAUD_0250_000_WITH_MHZ_16  = 0x00002B03,
     BAUD_0125_000_WITH_MHZ_16  = 0x00002B07,
     BAUD_0100_000_WITH_MHZ_16  = 0x00002B09,
     BAUD_0062_500_WITH_MHZ_16  = 0x00002B0F,
     BAUD_0050_000_WITH_MHZ_16  = 0x00002B13,
     BAUD_0015_625_WITH_MHZ_16  = 0x00002B3F,

     BAUD_1000_000_WITH_MHZ_20  = 0x00001601,/*10 Quanta,SJW = 0,Sample at 80%*/
     BAUD_0500_000_WITH_MHZ_20  = 0x00001603,
     BAUD_0250_000_WITH_MHZ_20  = 0x00001607,
     BAUD_0125_000_WITH_MHZ_20  = 0x0000160F,
     BAUD_0100_000_WITH_MHZ_20  = 0x00001613,
     BAUD_0062_500_WITH_MHZ_20  = 0x0000161F,
     BAUD_0050_000_WITH_MHZ_20  = 0x00001627,
     BAUD_0031_250_WITH_MHZ_20  = 0x0000163F,

     BAUD_1000_000_WITH_MHZ_24  = 0x00001801,/*12 Quanta,SJW = 0,Sample at 83%*/
     BAUD_0500_000_WITH_MHZ_24  = 0x00001803,
     BAUD_0250_000_WITH_MHZ_24  = 0x00001807,
     BAUD_0125_000_WITH_MHZ_24  = 0x0000180F,
     BAUD_0100_000_WITH_MHZ_24  = 0x00001813,
     BAUD_0062_500_WITH_MHZ_24  = 0x0000181F,
     BAUD_0050_000_WITH_MHZ_24  = 0x00001827,
     BAUD_0031_250_WITH_MHZ_24  = 0x0000183F,

     BAUD_1000_000_WITH_MHZ_30  = 0x00001602,/*10 Quanta,SJW = 0,Sample at 80%*/
     BAUD_0500_000_WITH_MHZ_30  = 0x00001605,
     BAUD_0250_000_WITH_MHZ_30  = 0x0000160B,
     BAUD_0125_000_WITH_MHZ_30  = 0x00001617,
     BAUD_0100_000_WITH_MHZ_30  = 0x0000161D,
     BAUD_0062_500_WITH_MHZ_30  = 0x0000162F,
     BAUD_0050_000_WITH_MHZ_30  = 0x0000163B,
     BAUD_0046_900_WITH_MHZ_30  = 0x0000163F,

     BAUD_1000_000_WITH_MHZ_32  = 0x00002B01,/*16 Quanta,SJW = 0,Sample at 81%*/
     BAUD_0500_000_WITH_MHZ_32  = 0x00002B03,
     BAUD_0250_000_WITH_MHZ_32  = 0x00002B07,
     BAUD_0125_000_WITH_MHZ_32  = 0x00002B0F,
     BAUD_0100_000_WITH_MHZ_32  = 0x00002B13,
     BAUD_0062_500_WITH_MHZ_32  = 0x00002B1F,
     BAUD_0050_000_WITH_MHZ_32  = 0x00002B27,
     BAUD_0031_300_WITH_MHZ_32  = 0x00002B3F,

     BAUD_1000_000_WITH_MHZ_40  = 0x00001603,/*10 Quanta,SJW = 0,Sample at 80%*/
     BAUD_0500_000_WITH_MHZ_40  = 0x00001607,
     BAUD_0250_000_WITH_MHZ_40  = 0x0000160F,
     BAUD_0125_000_WITH_MHZ_40  = 0x0000161F,
     BAUD_0100_000_WITH_MHZ_40  = 0x00001627,
     BAUD_0062_500_WITH_MHZ_40  = 0x0000163F,

     BAUD_1000_000_WITH_MHZ_48  = 0x00003A42,/*16 Quanta,SJW = 0,Sample at 81%*/
     BAUD_0500_000_WITH_MHZ_48  = 0x00003A45,
     BAUD_0250_000_WITH_MHZ_48  = 0x00003A4B,
     BAUD_0125_000_WITH_MHZ_48  = 0x00003A57,
     BAUD_0100_000_WITH_MHZ_48  = 0x00003A5D,
     BAUD_0062_500_WITH_MHZ_48  = 0x00003A6F,
     BAUD_0050_000_WITH_MHZ_48  = 0x00003A7B,
     BAUD_0046_900_WITH_MHZ_48  = 0x00003A7F,

     BAUD_1000_000_WITH_MHZ_50  = 0x00001604,
     BAUD_0500_000_WITH_MHZ_50  = 0x00003E44,
     BAUD_0250_000_WITH_MHZ_50  = 0x00003E49,
     BAUD_0125_000_WITH_MHZ_50  = 0x00003A58,
     BAUD_0100_000_WITH_MHZ_50  = 0x00003E58,
     BAUD_0062_500_WITH_MHZ_50  = 0x00003A71,
     BAUD_0050_000_WITH_MHZ_50  = 0x00003E71,
     BAUD_0046_900_WITH_MHZ_50  = 0x00003D77,
     BAUD_0031_250_WITH_MHZ_50  = 0x0000FF3F,
    };

   /***********************************************************************/
   /*                       Port Control Registers                        */
   /***********************************************************************/


 enum P2DirRegister
     {
       INPUT_TWINCAN_NODE_A_RXD         = 0xFFFC, //0xFFFE,
       OUTPUT_TWINCAN_NODE_A_TXD        = 0x0002,
       INPUT_TWINCAN_NODE_B_RXD         = 0xFFF3, //0xFFFB
       OUTPUT_TWINCAN_NODE_B_TXD        = 0x0008
     };


 enum P2Altsel0Register
     {
       FUNCTION_TWINCAN_NODE_A_RXD      = 0x0001,
       FUNCTION_TWINCAN_NODE_A_TXD      = 0x0002,
       FUNCTION_TWINCAN_NODE_B_RXD      = 0x0004,
       FUNCTION_TWINCAN_NODE_B_TXD      = 0x0008
     };


   /***********************************************************************/
   /*                    TwinCan Kernel Register                          */
   /***********************************************************************/

 enum TwinCanClcRegister
     {
     CLC_REG_HW_RESET_VALUE   = 0x00000002,
     CLC_REG_ENAB_WITH_CLK1   = 0x00000108
     };


 enum TsrRegister      /* TSR Register is "Read Only" */
     {
     TSR_REG_HW_RESET_VALUE           = 0x00000000,
     TSR_REG_TRANS_IN_PROGRESS_A_MASK = 0x00000001
     };

 enum RxipndRegister      /* RXIPND Register is "Read Only" */
     {
     RXIPND_REG_HW_RESET_VALUE        = 0x00000000,
     RXIPND_REG_RXI_MESSAGE_01_MASK   = 0x00000001
     };

 enum TxipndRegister      /* TSR Register is "Read Only" */
     {
     TXIPND_REG_HW_RESET_VALUE   = 0x00000000
     };

 enum KernelModi
     {
     ANALYSE_MODE_DISABLED         = 0x00,
     ANALYSE_MODE_ENABLED          = 0x80,
     FCR_UPDATE_ON_FOREIGN_FRAME   = 0x01,
     FCR_UPDATE_ON_RECEIVE_FRAME   = 0x02,
     FCR_UPDATE_ON_TRANSMIT_FRAME  = 0x04,
     FCR_INTRPT                    = 0x40,
     STATUS_CHANGE_INTRPT          = 0x04,
     ERROR_INTRPT                  = 0x08,
     LAST_ERROR_INTRPT             = 0x10
     };

   /******************************************************************************/
   /*           Node Specific Control / Status Register Enums                    */
   /******************************************************************************/

 enum CrRegister
     {
     CR_REG_HW_RESET_VALUE            = 0x00000001,
     CR_REG_INIT_BIT_SET              = 0x00000001,
     CR_REG_STATUS_CHANGE_INTRPT_ENAB = 0x00000004,
     CR_REG_ERROR_INTRPT_ENAB         = 0x00000008,
     CR_REG_LAST_ERROR_INTRPT_ENAB    = 0x00000010,
     CR_REG_CONFIGURATION_CHANGE_ENAB = 0x00000040,
     CR_REG_CAN_ANALYZER_MODE_ENAB    = 0x00000080,
     CR_REG_INIT_AND_CCE_BIT_RESET    = 0xFFFFFFBE
     };

     
 enum SrRegister
     {
     SR_REG_HW_RESET_VALUE            = 0x00000000,
     SR_REG_LAST_ERROR_CODE_MASK      = 0x00000007,
     SR_REG_TX_BIT_MASK               = 0x00000008,
     SR_REG_RX_BIT_MASK               = 0x00000010,
     SR_REG_ERROR_WARNING_STATUS_MASK = 0x00000040,
     SR_REG_RX_BUS_OFF_STATUS_MASK    = 0x00000080,
     SR_REG_LAST_ERROR_CODE_RESET     = 0xFFFFFFF8,
     SR_REG_TX_OK_RESET               = 0xFFFFFFF7,
     SR_REG_RX_OK_RESET               = 0xFFFFFFEF,
     SR_REG_RXOK_TXOK_RESET           = 0xFFFFFFE7
     };


 enum IrRegister
     {
     IR_REG_HW_RESET_VALUE            = 0x00000000,
     IR_REG_INTERRUPT_IDENTIFIER_MASK = 0x000000FF
     };


 enum BtrRegister
     {
     BTR_REG_HW_RESET_VALUE           = 0x00000000,
     BTR_REG_DEFAULT_CONFIGURATION    = 0x000016CF,
     BTR_REG_BAUDRATE_PRESCALER_MASK  = 0x0000003F,
     BTR_REG_SJW_TIME_QUANTA_MASK     = 0x000000C0,
     BTR_REG_TSEG1_TIME_QUANTA_MASK   = 0x00000F00,
     BTR_REG_TSEG2_TIME_QUANTA_MASK   = 0x00007000,
     BTR_REG_MODULE_CLK_DIVIDER_MASK  = 0x00008000,
     BTR_REG_LOOP_BACK_MODE_MASK      = 0x00010000
     };


 enum GinpRegister
     {
     GINP_REG_HW_RESET_VALUE          = 0x00000000,
     GINP_REG_DEF_ERROR_NODE_POINTR_A = 0x00000000,
     GINP_REG_DEF_LAST_ERR_NODE_PTR_A = 0x00000000,
     GINP_REG_DEF_TXRX_NODE_POINTR_A  = 0x00000000,
     GINP_REG_DEF_CFC_NODE_POINTR_A   = 0x00000000,
     GINP_REG_DEF_ERROR_NODE_POINTR_B = 0x00000001,
     GINP_REG_DEF_LAST_ERR_NODE_PTR_B = 0x00000010,
     GINP_REG_DEF_TXRX_NODE_POINTR_B  = 0x00000100,
     GINP_REG_DEF_CFC_NODE_POINTR_B   = 0x00001000
     };


 enum FcrRegister
     {
     FCR_REG_HW_RESET_VALUE           = 0x00000000,
     FCR_REG_FRAME_COUNTER_MASK       = 0x0000FFFF,
     FCR_REG_FOREIGN_FRAME_CNT_ENABLE = 0x00010000,
     FCR_REG_RECEIVE_FRAME_CNT_ENABLE = 0x00020000,
     FCR_REG_TRANSMT_FRAME_CNT_ENABLE = 0x00040000,
     FCR_REG_SOF_BIT_TIME_STAMP_ENAB  = 0x00080000,
     FCR_REG_EOF_BIT_TIME_STAMP_ENAB  = 0x00090000,
     FCR_REG_FRAME_COUNTER_INTRPT_ENA = 0x00400000,
     FCR_REG_FRAME_COUNTER_OVFL_MASK  = 0x00800000
     };

 enum Imr0Register
     {
     IMR0_REG_HW_RESET_VALUE          = 0x00000000,
     IMR0_REG_ALL_MO_INT_DISPLAYED    = 0xFFFFFFFF
     };


 enum Imr4Register
     {
     IMR4_REG_HW_RESET_VALUE          = 0x00000000,
     IMR4_REG_LAST_ERR_TXRX_ERR_ENAB  = 0x00000007
     };


 enum EcntRegister
     {
     ECNT_REG_HW_RESET_VALUE          = 0x00600000,
     ECNT_REG_RECEIVE_ERR_CNT_MASK    = 0x000000FF,
     ECNT_REG_TRANSMIT_ERR_CNT_MASK   = 0x0000FF00,
     ECNT_REG_EWRNLVL_CNT_MASK        = 0x00FF0000,
     ECNT_REG_LAST_ERR_DIRECT_MASK    = 0x01000000,
     ECNT_REG_LAST_ERR_INCREMENT_MASK = 0x02000000
     };

   /***********************************************************************/
   /*                  TwinCan Message Object Register                    */
   /***********************************************************************/

 enum Msgdr0Register
     {
     MSGDR0_REG_HW_RESET_VALUE        = 0x00000000
     };


 enum Msgdr4Register
     {
     MSGDR4_REG_HW_RESET_VALUE        = 0x00000000
     };


 enum MsgarRegister
     {
     MSGAR_REG_HW_RESET_VALUE         = 0x00000000
     };


 enum MsgamrRegister
     {
     MSGAMR_REG_HW_RESET_VALUE        = 0xFFFFFFFF
     };


 enum MsgcfgRegister
     {
     MSGCFG_REG_HW_RESET_VALUE        = 0x00000000,
     MSGCFG_REG_NODE_B_MO_SETUP       = 0x00000002,
     MSGCFG_REG_EXT_IDENT_MO_SETUP    = 0x00000004,
     MSGCFG_REG_STD_IDENT_MO_SETUP    = 0x00000000,
     MSGCFG_REG_TRANSMIT_MO_SETUP     = 0x00000008,
     MSGCFG_REG_RECEIVE_MO_SETUP      = 0xFFFFFFF7,
     MSGCFG_REG_DATA_LENGTH_CODE_0    = 0x00000000,
     MSGCFG_REG_DATA_LENGTH_CODE_1    = 0x00000010,
     MSGCFG_REG_DATA_LENGTH_CODE_2    = 0x00000020,
     MSGCFG_REG_DATA_LENGTH_CODE_3    = 0x00000030,
     MSGCFG_REG_DATA_LENGTH_CODE_4    = 0x00000040,
     MSGCFG_REG_DATA_LENGTH_CODE_5    = 0x00000050,
     MSGCFG_REG_DATA_LENGTH_CODE_6    = 0x00000060,
     MSGCFG_REG_DATA_LENGTH_CODE_7    = 0x00000070,
     MSGCFG_REG_DATA_LENGTH_CODE_8    = 0x00000080,
     MSGCFG_REG_RX_NODE_POINTER_A     = 0x00040000,
     MSGCFG_REG_RX_NODE_POINTER_B     = 0x00050000,
     MSGCFG_REG_RTR_RX_NODE_POINTER_A = 0x00060000,
     MSGCFG_REG_RTR_RX_NODE_POINTER_B = 0x00070000,
     MSGCFG_REG_TX_NODE_POINTER_A     = 0x00200000,
     MSGCFG_REG_TX_NODE_POINTER_B     = 0x00300000
     };

 enum MsgctrRegister
     {
     MSGCTR_REG_HW_RESET_VALUE        = 0x00005555,
     MSGCTR_REG_INTERRUPT_PENDING_RES = 0xFFFFFFFD,
     MSGCTR_REG_INTERRUPT_PENDING_SET = 0xFFFFFFFE,
     MSGCTR_REG_RX_INTERRUPT_DISABLE  = 0xFFFFFFF7,
     MSGCTR_REG_RX_INTERRUPT_ENABLE   = 0xFFFFFFFB,
     MSGCTR_REG_TX_INTERRUPT_DISABLE  = 0xFFFFFFDF,
     MSGCTR_REG_TX_INTERRUPT_ENABLE   = 0xFFFFFFEF,
     MSGCTR_REG_MSGVAL_RESET          = 0xFFFFFF7F,
     MSGCTR_REG_MSGVAL_SET            = 0xFFFFFFBF,
     MSGCTR_REG_NEWDAT_RESET          = 0xFFFFFDFF,
     MSGCTR_REG_NEWDAT_SET            = 0xFFFFFEFF,
     MSGCTR_REG_CPUUPD_RESET          = 0xFFFFF7FF,
     MSGCTR_REG_CPUUPD_SET            = 0xFFFFFBFF,
     MSGCTR_REG_MSGLST_RESET          = 0xFFFFF7FF,
     MSGCTR_REG_MSGLST_SET            = 0xFFFFFBFF,
     MSGCTR_REG_TXRQ_RESET            = 0xFFFFDFFF,
     MSGCTR_REG_TXRQ_SET              = 0xFFFFEFFF,
     MSGCTR_REG_RMTPND_RESET          = 0xFFFF7FFF,
     MSGCTR_REG_RMTPND_SET            = 0xFFFFBFFF,
     MSGCTR_REG_CFCVAL_RESET          = 0x0000FFFF
     };

 enum MsgctrRegisterCtrPart
     {
     MSGCTR_CTR_HW_RESET_VALUE        = 0x5555,
     MSGCTR_CTR_INTERRUPT_PENDING_RES = 0xFFFD,
     MSGCTR_CTR_INTERRUPT_PENDING_SET = 0xFFFE,
     MSGCTR_CTR_RX_INTERRUPT_DISABLE  = 0xFFF7,
     MSGCTR_CTR_RX_INTERRUPT_ENABLE   = 0xFFFB,
     MSGCTR_CTR_TX_INTERRUPT_DISABLE  = 0xFFDF,
     MSGCTR_CTR_TX_INTERRUPT_ENABLE   = 0xFFEF,
     MSGCTR_CTR_MSGVAL_RESET          = 0xFF7F,
     MSGCTR_CTR_MSGVAL_SET            = 0xFFBF,
     MSGCTR_CTR_NEWDAT_RESET          = 0xFDFF,
     MSGCTR_CTR_NEWDAT_SET            = 0xFEFF,
     MSGCTR_CTR_CPUUPD_RESET          = 0xF7FF,
     MSGCTR_CTR_CPUUPD_SET            = 0xFBFF,
     MSGCTR_CTR_MSGLST_RESET          = 0xF7FF,
     MSGCTR_CTR_MSGLST_SET            = 0xFBFF,
     MSGCTR_CTR_TXRQ_RESET            = 0xDFFF,
     MSGCTR_CTR_TXRQ_SET              = 0xEFFF,
     MSGCTR_CTR_RMTPND_RESET          = 0x7FFF,
     MSGCTR_CTR_RMTPND_SET            = 0xBFFF
     };

 enum MsgctrRegisterCfcPart
     {
     MSGCTR_CFC_HW_RESET_VALUE        = 0x0000,
     MSGCTR_CFC_OVERFLOW_VALUE        = 0xFFFF
     };

 enum MsgfgcrRegister
     {
     MSGFGCR_REG_HW_RESET_VALUE       = 0x00000000,
     MSGFGCR_REG_FSIZE_MASK           = 0x0000001F,
     MSGFGCR_REG_STT_MASK             = 0x00008000,
     MSGFGCR_REG_CANPTR_MASK          = 0x001F0000,
     MSGFGCR_REG_MMC_MASK             = 0x07000000,
     /*******************************************/
     /*          FiFo Configuration             */
     /*******************************************/
     MSGFGCR_REG_2_STAGE_FIFO_SET     = 0x00000001,
     MSGFGCR_REG_4_STAGE_FIFO_SET     = 0x00000003,
     MSGFGCR_REG_8_STAGE_FIFO_SET     = 0x00000007,
     MSGFGCR_REG_16_STAGE_FIFO_SET    = 0x0000000F,
     MSGFGCR_REG_32_STAGE_FIFO_SET    = 0x0000001F,
     MSGFGCR_REG_FD_SET               = 0x00002000,
     MSGFGCR_REG_SDT_SET              = 0x00004000,
     MSGFGCR_REG_MO00_AS_FIFO_BASE    = 0x02000000,
     MSGFGCR_REG_MO01_AS_FIFO_BASE    = 0x02010000,
     MSGFGCR_REG_MO02_AS_FIFO_BASE    = 0x02020000,
     MSGFGCR_REG_MO03_AS_FIFO_BASE    = 0x02030000,
     MSGFGCR_REG_MO04_AS_FIFO_BASE    = 0x02040000,
     MSGFGCR_REG_MO05_AS_FIFO_BASE    = 0x02050000,
     MSGFGCR_REG_MO06_AS_FIFO_BASE    = 0x02060000,
     MSGFGCR_REG_MO07_AS_FIFO_BASE    = 0x02070000,
     MSGFGCR_REG_MO08_AS_FIFO_BASE    = 0x02080000,
     MSGFGCR_REG_MO09_AS_FIFO_BASE    = 0x02090000,
     MSGFGCR_REG_MO10_AS_FIFO_BASE    = 0x020A0000,
     MSGFGCR_REG_MO11_AS_FIFO_BASE    = 0x020B0000,
     MSGFGCR_REG_MO12_AS_FIFO_BASE    = 0x020C0000,
     MSGFGCR_REG_MO13_AS_FIFO_BASE    = 0x020D0000,
     MSGFGCR_REG_MO14_AS_FIFO_BASE    = 0x020E0000,
     MSGFGCR_REG_MO15_AS_FIFO_BASE    = 0x020F0000,
     MSGFGCR_REG_MO16_AS_FIFO_BASE    = 0x02100000,
     MSGFGCR_REG_MO17_AS_FIFO_BASE    = 0x02110000,
     MSGFGCR_REG_MO18_AS_FIFO_BASE    = 0x02120000,
     MSGFGCR_REG_MO19_AS_FIFO_BASE    = 0x02130000,
     MSGFGCR_REG_MO20_AS_FIFO_BASE    = 0x02140000,
     MSGFGCR_REG_MO21_AS_FIFO_BASE    = 0x02150000,
     MSGFGCR_REG_MO22_AS_FIFO_BASE    = 0x02160000,
     MSGFGCR_REG_MO23_AS_FIFO_BASE    = 0x02170000,
     MSGFGCR_REG_MO24_AS_FIFO_BASE    = 0x02180000,
     MSGFGCR_REG_MO25_AS_FIFO_BASE    = 0x02190000,
     MSGFGCR_REG_MO26_AS_FIFO_BASE    = 0x021A0000,
     MSGFGCR_REG_MO27_AS_FIFO_BASE    = 0x021B0000,
     MSGFGCR_REG_MO28_AS_FIFO_BASE    = 0x021C0000,
     MSGFGCR_REG_MO29_AS_FIFO_BASE    = 0x021D0000,
     MSGFGCR_REG_MO30_AS_FIFO_BASE    = 0x021E0000,
     MSGFGCR_REG_MO31_AS_FIFO_BASE    = 0x021F0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO00   = 0x03000000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO01   = 0x03010000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO02   = 0x03020000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO03   = 0x03030000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO04   = 0x03040000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO05   = 0x03050000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO06   = 0x03060000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO07   = 0x03070000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO08   = 0x03080000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO09   = 0x03090000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO10   = 0x030A0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO11   = 0x030B0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO12   = 0x030C0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO13   = 0x030D0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO14   = 0x030E0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO15   = 0x030F0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO16   = 0x03100000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO17   = 0x03110000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO18   = 0x03120000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO19   = 0x03130000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO20   = 0x03140000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO21   = 0x03150000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO22   = 0x03160000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO23   = 0x03170000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO24   = 0x03180000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO25   = 0x03190000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO26   = 0x031A0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO27   = 0x031B0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO28   = 0x031C0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO29   = 0x031D0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO30   = 0x031E0000,
     MSGFGCR_REG_FIFO_SLAVE_TO_MO31   = 0x031F0000,
     /*******************************************/
     /*       Gateway Configuration             */
     /*******************************************/
     MSGFGCR_REG_GDFS_SET             = 0x00000100,
     MSGFGCR_REG_SRREN_SET            = 0x00000200,
     MSGFGCR_REG_IDC_SET              = 0x00000400,
     MSGFGCR_REG_DLCC_SET             = 0x00000800,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO00 = 0x04000000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO01 = 0x04010000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO02 = 0x04020000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO03 = 0x04030000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO04 = 0x04040000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO05 = 0x04050000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO06 = 0x04060000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO07 = 0x04070000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO08 = 0x04080000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO09 = 0x04090000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO10 = 0x040A0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO11 = 0x040B0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO12 = 0x040C0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO13 = 0x040D0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO14 = 0x040E0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO15 = 0x040F0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO16 = 0x04100000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO17 = 0x04110000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO18 = 0x04120000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO19 = 0x04130000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO20 = 0x04140000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO21 = 0x04150000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO22 = 0x04160000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO23 = 0x04170000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO24 = 0x04180000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO25 = 0x04190000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO26 = 0x041A0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO27 = 0x041B0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO28 = 0x041C0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO29 = 0x041D0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO30 = 0x041E0000,
     MSGFGCR_REG_NORM_GATEW_S_TO_MO31 = 0x041F0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO00 = 0x00000000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO01 = 0x00010000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO02 = 0x00020000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO03 = 0x00030000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO04 = 0x00040000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO05 = 0x00050000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO06 = 0x00060000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO07 = 0x00070000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO08 = 0x00080000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO09 = 0x00090000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO10 = 0x000A0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO11 = 0x000B0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO12 = 0x000C0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO13 = 0x000D0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO14 = 0x000E0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO15 = 0x000F0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO16 = 0x00100000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO17 = 0x00110000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO18 = 0x00120000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO19 = 0x00130000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO20 = 0x00140000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO21 = 0x00150000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO22 = 0x00160000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO23 = 0x00170000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO24 = 0x00180000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO25 = 0x00190000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO26 = 0x001A0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO27 = 0x001B0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO28 = 0x001C0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO29 = 0x001D0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO30 = 0x001E0000,
     MSGFGCR_REG_GATEW_D_SRR_FOR_MO31 = 0x001F0000,
     MSGFGCR_REG_MO00_AS_NORM_GATEW_D = 0x00000000,
     MSGFGCR_REG_MO01_AS_NORM_GATEW_D = 0x00010000,
     MSGFGCR_REG_MO02_AS_NORM_GATEW_D = 0x00020000,
     MSGFGCR_REG_MO03_AS_NORM_GATEW_D = 0x00030000,
     MSGFGCR_REG_MO04_AS_NORM_GATEW_D = 0x00040000,
     MSGFGCR_REG_MO05_AS_NORM_GATEW_D = 0x00050000,
     MSGFGCR_REG_MO06_AS_NORM_GATEW_D = 0x00060000,
     MSGFGCR_REG_MO07_AS_NORM_GATEW_D = 0x00070000,
     MSGFGCR_REG_MO08_AS_NORM_GATEW_D = 0x00080000,
     MSGFGCR_REG_MO09_AS_NORM_GATEW_D = 0x00090000,
     MSGFGCR_REG_MO10_AS_NORM_GATEW_D = 0x000A0000,
     MSGFGCR_REG_MO11_AS_NORM_GATEW_D = 0x000B0000,
     MSGFGCR_REG_MO12_AS_NORM_GATEW_D = 0x000C0000,
     MSGFGCR_REG_MO13_AS_NORM_GATEW_D = 0x000D0000,
     MSGFGCR_REG_MO14_AS_NORM_GATEW_D = 0x000E0000,
     MSGFGCR_REG_MO15_AS_NORM_GATEW_D = 0x000F0000,
     MSGFGCR_REG_MO16_AS_NORM_GATEW_D = 0x00100000,
     MSGFGCR_REG_MO17_AS_NORM_GATEW_D = 0x00110000,
     MSGFGCR_REG_MO18_AS_NORM_GATEW_D = 0x00120000,
     MSGFGCR_REG_MO19_AS_NORM_GATEW_D = 0x00130000,
     MSGFGCR_REG_MO20_AS_NORM_GATEW_D = 0x00140000,
     MSGFGCR_REG_MO21_AS_NORM_GATEW_D = 0x00150000,
     MSGFGCR_REG_MO22_AS_NORM_GATEW_D = 0x00160000,
     MSGFGCR_REG_MO23_AS_NORM_GATEW_D = 0x00170000,
     MSGFGCR_REG_MO24_AS_NORM_GATEW_D = 0x00180000,
     MSGFGCR_REG_MO25_AS_NORM_GATEW_D = 0x00190000,
     MSGFGCR_REG_MO26_AS_NORM_GATEW_D = 0x001A0000,
     MSGFGCR_REG_MO27_AS_NORM_GATEW_D = 0x001B0000,
     MSGFGCR_REG_MO28_AS_NORM_GATEW_D = 0x001C0000,
     MSGFGCR_REG_MO29_AS_NORM_GATEW_D = 0x001D0000,
     MSGFGCR_REG_MO30_AS_NORM_GATEW_D = 0x001E0000,
     MSGFGCR_REG_MO31_AS_NORM_GATEW_D = 0x001F0000,
     MSGFGCR_REG_MO00_AS_SHARED_GATEW = 0x05000000,
     MSGFGCR_REG_MO01_AS_SHARED_GATEW = 0x05010000,
     MSGFGCR_REG_MO02_AS_SHARED_GATEW = 0x05020000,
     MSGFGCR_REG_MO03_AS_SHARED_GATEW = 0x05030000,
     MSGFGCR_REG_MO04_AS_SHARED_GATEW = 0x05040000,
     MSGFGCR_REG_MO05_AS_SHARED_GATEW = 0x05050000,
     MSGFGCR_REG_MO06_AS_SHARED_GATEW = 0x05060000,
     MSGFGCR_REG_MO07_AS_SHARED_GATEW = 0x05070000,
     MSGFGCR_REG_MO08_AS_SHARED_GATEW = 0x05080000,
     MSGFGCR_REG_MO09_AS_SHARED_GATEW = 0x05090000,
     MSGFGCR_REG_MO10_AS_SHARED_GATEW = 0x050A0000,
     MSGFGCR_REG_MO11_AS_SHARED_GATEW = 0x050B0000,
     MSGFGCR_REG_MO12_AS_SHARED_GATEW = 0x050C0000,
     MSGFGCR_REG_MO13_AS_SHARED_GATEW = 0x050D0000,
     MSGFGCR_REG_MO14_AS_SHARED_GATEW = 0x050E0000,
     MSGFGCR_REG_MO15_AS_SHARED_GATEW = 0x050F0000,
     MSGFGCR_REG_MO16_AS_SHARED_GATEW = 0x05100000,
     MSGFGCR_REG_MO17_AS_SHARED_GATEW = 0x05110000,
     MSGFGCR_REG_MO18_AS_SHARED_GATEW = 0x05120000,
     MSGFGCR_REG_MO19_AS_SHARED_GATEW = 0x05130000,
     MSGFGCR_REG_MO20_AS_SHARED_GATEW = 0x05140000,
     MSGFGCR_REG_MO21_AS_SHARED_GATEW = 0x05150000,
     MSGFGCR_REG_MO22_AS_SHARED_GATEW = 0x05160000,
     MSGFGCR_REG_MO23_AS_SHARED_GATEW = 0x05170000,
     MSGFGCR_REG_MO24_AS_SHARED_GATEW = 0x05180000,
     MSGFGCR_REG_MO25_AS_SHARED_GATEW = 0x05190000,
     MSGFGCR_REG_MO26_AS_SHARED_GATEW = 0x051A0000,
     MSGFGCR_REG_MO27_AS_SHARED_GATEW = 0x051B0000,
     MSGFGCR_REG_MO28_AS_SHARED_GATEW = 0x051C0000,
     MSGFGCR_REG_MO29_AS_SHARED_GATEW = 0x051D0000,
     MSGFGCR_REG_MO30_AS_SHARED_GATEW = 0x051E0000,
     MSGFGCR_REG_MO31_AS_SHARED_GATEW = 0x051F0000
     };

   /***********************************************************************/
   /*             TwinCan Service Node Request Register                   */
   /***********************************************************************/
 enum TwinCanSrcRegisterValues
     {
     CAN_SRC_REG_HW_RESET_VALUE       = 0x00000000,
     CAN_SRC_SERV_REQUEST_ENAB        = 0x00001000,
     CAN_SRC_SERV_REQUEST_CLEAR       = 0x00004000
     };

 struct TwinCanKernel
 {
    volatile u32 BitTimingNodeA;/* BAUD_0100_000_WITH_MHZ_10 ... or MY_ABTR */
    volatile u32 BitTimingNodeB;/* BAUD_1000_000_WITH_MHZ_10 ... or MY_ABTR */
             u8  AnalysingModeNodeA;
             u8  AnalysingModeNodeB;
             u8  FrameCounterModeNodeA;
             u8  FrameCounterModeNodeB;
             u8  GlobalInterruptControlNodeA;
             u8  GlobalInterruptControlNodeB;
             u32 GlobalInterruptNodePointerA;
             u32 GlobalInterruptNodePointerB;
 };

 struct TwinCanMoInterruptMasks
     {
     u32 IntMoTxMaskNodeA;
     u32 IntMoTxMaskNodeB;
     u32 IntMoRxMaskNodeA;
     u32 IntMoRxMaskNodeB;
     u32 IntMoRxRtrMaskNodeA;
     u32 IntMoRxRtrMaskNodeB;
     u8  IntNodeMask; /* stores all enabled interrupt nodes */
 };

 struct twincan_priv {
   struct net_device_stats stats;
   long   open_time;
   int    clock;
   int    Baud_Rate;
   int    gap;
   int    status;
   int    current_MO;
   spinlock_t lock;
 };

 /******************************************************************************/
 /* @Global Definitions                                                        */
 /******************************************************************************/

 int InterruptPriority[8];
 int CanSrnRegAddr[8];

 struct TwinCanKernelRegister KernelReg;
 struct TwinCanKernel KernelConstants;
 struct TwinCanMessageObjectRegister MessageObject;
 struct TwinCanMoInterruptMasks InterruptMasks;
 struct TwinCanServiceRequestNodeRegister ServiceRequestNode;

