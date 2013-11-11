/*
 *  tp16.h
 *
 *  Low Level CAN Framework
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

#ifndef TP16_H
#define TP16_H

#ifdef __KERNEL__
#include "version.h"
RCSID("$Id: tp16.h,v 1.10 2005/12/14 10:21:15 ethuerm Exp $");
#endif

#include "tp_conf.h"

#define TP16_T1_DEFAULT 1000 /* set 1000 ms timeout for my ACKs in opposite sender */
#define TP16_T2_DEFAULT 0    /* unused (NOT IMPLEMENTED) */
#define TP16_T3_DEFAULT 0    /* no transmit delay for opposite sender */
#define TP16_T4_DEFAULT 1000 /* set 1000 ms timeout before opposite sender disconnects */

#define TP16_BS_DEFAULT 15   /* blocksize 15 possible */

#define TP16_T1_DISABLED 6300000 /* leads to 0xFF on CAN which means 'disabled' */
#define TP16_T4_DISABLED 6300000 /* leads to 0xFF on CAN which means 'disabled' */

#define TP16BASE_100US 0x00   /* dynamic protocol parameters in CS message */
#define TP16BASE_1MS   0x40
#define TP16BASE_10MS  0x80
#define TP16BASE_100MS 0xC0

#define TP16_CONFIG_MASK (HALF_DUPLEX | USE_DISCONNECT |\
                         ENABLE_BREAK | TRACE_MODE |\
                         AUTO_DISCONNECT | RELOAD_DEFAULTS)

#define TP16_CONFIG_DEFAULT (USE_DISCONNECT | HALF_DUPLEX | AUTO_DISCONNECT | ENABLE_BREAK)

#define SOL_CAN_TP16 (SOL_CAN_BASE + CAN_TP16)
#define CAN_TP16_OPT 1

struct can_tp16_options {

  struct timeval t1;       /* ACK timeout for DT TPDU. VAG: T1 */
  struct timeval t2;       /* max. time between two DT TPDU's. VAG: T2 (NOT IMPLEMENTED!) */
  struct timeval t3;       /* transmit delay for DT TPDU's. VAG: T3 */
  struct timeval t4;       /* receive timeout for automatic disconnect. VAG: T4 */

  unsigned char blocksize; /* max number of unacknowledged DT TPDU's (1..15) */
  unsigned int  config;    /* analogue tp_user_data.conf see tp_gen.h */

};

#endif /* TP16_H */
