/*
 *  mcnet.h
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

#ifndef MCNET_H
#define MCNET_H

#ifdef __KERNEL__
#include "version.h"
RCSID("$Id: mcnet.h,v 1.6 2005/12/14 10:21:15 ethuerm Exp $");
#endif

#include "tp_conf.h"

#define MCNET_BS_DEFAULT 15   /* blocksize 15 possible */
#define MCNET_TD_DEFAULT 10   /* 10ms transmit delay */

#define MCNET_CONFIG_MASK (USE_CONNECTIONTEST |\
                         ENABLE_OOB_DATA | TRACE_MODE |\
                         RELOAD_DEFAULTS)

#define MCNET_CONFIG_DEFAULT 0 /* (USE_CONNECTIONTEST | ENABLE_OOB_DATA) */

#define SOL_CAN_MCNET (SOL_CAN_BASE + CAN_MCNET)
#define CAN_MCNET_OPT 1

struct can_mcnet_options {

  unsigned char blocksize; /* max number of unacknowledged DT TPDU's (1..15) */
  struct timeval td;       /* transmit delay for DT TPDU's */
  unsigned int  config;    /* analogue tp_user_data.conf see tp_gen.h */

};

#endif /* MCNET_H */
