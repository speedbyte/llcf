/*
 *  tp_gen.h
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

#ifndef TP_GEN_H
#define TP_GEN_H

#include "version.h"
RCSID("$Id: tp_gen.h,v 1.34 2006/04/07 12:55:54 hartko Exp $");


/* macros provided by tp generic */

#define MS2JIFF(x) ((x)*HZ/1000)
#define JIFF2MS(x) ((x)*1000/HZ)

/* generic TP structures */

struct tp_parameters {

    /* these parameters are static but can be configured at start time */

    unsigned int bs;       /* blocksize. max number of unacknowledged DT TPDU's */
    unsigned int mybs;     /* % value announced to the communication partner */

    unsigned int mnt;      /* max number of transmissions DT TPDU */
    unsigned int mntc;     /* max number of transmissions CS TPDU */
    unsigned int mnct;     /* max number of transmissions CT TPDU */
    unsigned int mnbe;     /* max number of block errors. VAG: MNTB */

    /* values for timers already in jiffies */

    unsigned long ta;      /* ACK timeout for DT TPDU. VAG: T1 */
    unsigned long myta;    /* % usec value announced to communication partner */
    unsigned long tac;     /* ACK timeout for CS TPDU. VAG: T_E */
    unsigned long tat;     /* ACK timeout for CT TPDU */

    unsigned long tct;     /* time between consecutive CT TPDU's. VAG: T_CT */

    unsigned long td;      /* transmit delay for DT TPDU's. VAG: T3 */
    unsigned long mytd;    /* % usec value announced to communication partner */
    unsigned long tde;     /* transmit delay for DT TPDU's in RNR conditions */

    unsigned long tdc;     /* timeout for disconnect in inactive connection */
    unsigned long mytdc;   /* % usec value announced to communication partner */

    unsigned long tn;      /* bus latency. VAG: TN */

    /* The my* parameters reflect the local capabilities that are announced */
    /* to the communication partner. These parameters are not used in the   */
    /* state machines. The parameters 'bs', 'ta', 'td' and 'tdc' are set    */
    /* with default values at start time which can be modified by the       */
    /* communication partner at connection setup time. eg. for TP16 / TP20  */

};

struct tp_statistics {

    unsigned int msgs;     /* number of processed messages */
    unsigned int bytes;    /* number of processed bytes */
    unsigned int retries;  /* summarized retry counter */

};

struct tp_rx_statem {

    unsigned char     state;       /* current state of specific rx path */
    unsigned char     rxsn;        /* assumed next sequence number to receive */  
    unsigned int      rx_buf_size; /* current size of malloc'd rx_buf */ 
    unsigned char     *rx_buf;     /* pointer to receive buffer */
    unsigned int      rx_buf_ptr;  /* offset form start of rx_buf to write incoming data */
    struct tp_statistics stat;     /* statistics */
    struct tp_user_data *ud;       /* reference to user_data of this tp connection */

};

struct tp_tx_statem {

    unsigned char     state;      /* current state of specific tx path */
    unsigned char     txsn;       /* next sequence number to send */  
    unsigned char     arsn;       /* sequence number to send acknowledge request (bs>1) */  
    unsigned char     retrycnt;   /* retry counter */
    unsigned char     blkerrcnt;  /* block error counter */
    unsigned char     blkstart;   /* start of tx for current block (for jitter correction) */
    struct tx_buf     *tx_queue;  /* pointer to first (and current) element of our tx_queue */
    unsigned int      tx_buf_ptr; /* offset form start of tx_buf to read outgoing data */
    unsigned int      lasttxlen;  /* length of last transmitted datagram */
    struct timer_list timer;      /* timeout timer of specific tx path */
    struct tp_statistics stat;    /* statistics */
    struct tp_user_data *ud;      /* reference to user_data of this tp connection */

};

struct tp_cx_statem {

    unsigned int      state;    /* current state of specific cx path */
    unsigned char     retrycnt; /* retry counter */
    struct timer_list timer;    /* timeout timer of specific cx path */
    struct tp_statistics stat;  /* statistics */
    struct tp_user_data *ud;    /* reference to user_data of this tp connection */

};

struct tx_buf {

    struct tx_buf *next;
    unsigned char *data;
    unsigned int  size; /* size of malloc'd *data */

};


struct tp_user_data {

    unsigned int         conf;       /* configuration */
    unsigned int         flags;      /* TP global flags */
    char                 *ident;     /* ident from tp_gen 'user' */

    canid_t              can_tx_id;  /* this TP channel */
    canid_t              can_rx_id;  /* this TP channel */

    struct tp_parameters parm;       /* generic TP parameters */
    struct tp_tx_statem  txstate;    /* the tx statemachine */
    struct tp_tx_statem  txstateoob; /* the tx statemachine (oob) */
    struct tp_rx_statem  rxstate;    /* the rx statemachine */
    struct tp_rx_statem  rxstateoob; /* the rx statemachine (oob) */
    struct tp_cx_statem  constate;   /* the connection statemachine */
    struct tp_cx_statem  cteststate; /* the connectiontest statemachine */

    struct sock          *sk;        /* reference to own sock-structure */
    struct socket        *lsock;     /* reference to listen socket in server-mode */
    wait_queue_head_t    wait;       /* wait queue for connect() and accept() */
    struct timeval       stamp;      /* timestamp of last can data tpdu */
    void (*tp_disconnect)(struct tp_user_data * ud); /* reference for tp generic part */

};

#define tp_sk(sk) ((struct tp_user_data *)(sk)->user_data)

/* flags from tp_user_data.flags */

#define TP_ONLINE     0x0001 /* the big engine is running */
#define TP_BREAK_MODE 0x0002 /* is set as long as communication is unacknowledged */
#define TP_BREAK_2USR 0x0004 /* is set while BREAK condition ist not communicated to the user */
#define SKREF         0x0008 /* sock reference. Two sockets are currently using same sk/ud */ 

/* stati for constate */
enum {CON_WAIT_CS, CON_STARTUP, CON_CON, CON_DISC};

/* stati for cteststate */
enum {CT_DISABLED, CT_WAIT_CON, CT_WAIT_AK, CT_WAIT_TCT};

/* stati for txstate */
enum {TX_IDLE, TX_WAIT_AK, TX_SENDING, TX_BLOCKED};

/* stati for rxstate */
enum {RX_RECEIVE, RX_BUSY};


/* values for tcpi1 */

#define TPDT  0x00 /* valid add-ons : TPOOB, TPAR, TPEOM and SN */
#define TPAK  0x90 /* valid add-ons : TPOOB, TPRS and SN */
#define TPCS  0xA0 /* valid add-ons : TPOOB */
#define TPCA  0xA1 /* valid add-ons : TPOOB */
#define TPCT  0xA3 /* valid add-ons : none */
#define TPBR  0xA4 /* valid add-ons : TPOOB (unused!) */
#define TPDC  0xA8 /* valid add-ons : TPOOB (unused!) */

#define TPOOB 0x40 /* out of band data (MCNet expedited) */
#define TPAR  0x20 /* acknowledge request */
#define TPEOM 0x10 /* end of message */
#define TPRS  0x20 /* receive status 'receive ready' */

/* the blocksize is limited to 15 */

#define BS_MASK 0x0F

/* functions provided by tp generic */

int  tp_create_ud(struct sock *sk);
void tp_remove_ud(struct tp_user_data *ud);
void tp_remove_tx_queue(struct tx_buf *tx_queue);
void tp_start_engine(struct tp_user_data *ud);
void tp_stop_engine(struct tp_user_data *ud);

void tp_xmit_frame(struct tp_user_data *ud, struct can_frame *txframe);
void tp_xmitCI(struct tp_user_data *ud, unsigned char tpci1);

void tp_tx_wakeup(struct tp_tx_statem *mystate);

void tp_tx_data(struct tp_tx_statem *mystate, struct can_frame rxframe);
void tp_rx_data(struct tp_rx_statem *mystate, struct can_frame rxframe);

#endif /* TP_GEN_H */
