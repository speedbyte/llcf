/*
 *  tp_conf.h
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

#ifndef TP_CONF_H
#define TP_CONF_H

#ifdef __KERNEL__
#include "version.h"
RCSID("$Id: tp_conf.h,v 1.8 2005/12/14 10:37:20 ethuerm Exp $");
#endif

/* configuration from tp_user_data.conf */

#define USE_CONNECTIONTEST   0x0001 /* use connection test */
#define USE_DISCONNECT       0x0002 /* send DC when closing connection */
#define ENABLE_BREAK         0x0004 /* support TP2.0 break functionality */
#define ENABLE_OOB_DATA      0x0008 /* support MCNet expedited data */
#define HALF_DUPLEX          0x0010 /* half duplex mode (TP1.6) */
#define AUTO_DISCONNECT      0x0020 /* auto disconnect */
#define ACTIVE               0x0040 /* active = client that starts the CS */
#define TRACE_MODE           0x0080 /* trace & combine received frames on can_rx_id (read only) */
#define RELOAD_DEFAULTS      0x0100 /* reload default tp parameters when closing socket */


#endif /* TP_CONF_H */
