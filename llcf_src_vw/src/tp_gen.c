/*
 *  tp_gen.c
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

#define EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/uio.h>
#include <linux/wait.h>
#include <net/sock.h>

#include "af_can.h"
#include "version.h"
#include "tp_gen.h"
#include "tp_conf.h"

RCSID("$Id: tp_gen.c,v 1.56 2006/04/13 07:47:36 hartko Exp $");

#define NAME "TP Generic driver for LLCF"
static __initdata const char banner[] = BANNER(NAME);

MODULE_DESCRIPTION(NAME);
MODULE_LICENSE("Volkswagen Group closed source");
MODULE_AUTHOR("Oliver Hartkopp <oliver.hartkopp@volkswagen.de>");

#ifdef DEBUG
MODULE_PARM(debug, "1i");
static int debug = 0;
#define DBG(args...)       (debug & 1 ? \
	                       (printk(KERN_DEBUG "TP %s: ", __func__), \
			        printk(args)) : 0)
#define DBG_FRAME(args...) (debug & 2 ? can_debug_cframe(args) : 0)
#define DBG_SKB(skb)       (debug & 4 ? can_debug_skb(skb) : 0)
#else
#define DBG(args...)
#define DBG_FRAME(args...)
#define DBG_SKB(skb)
#endif

MODULE_PARM(printstats, "1i");
static int printstats = 0;

/* tp generic private functions */

static void tp_rx_store_data(struct tp_rx_statem *mystate,
			     unsigned char len, unsigned char *data);
static void tp_tx_timer(unsigned long data);
static void tp_tx_sendtpdata(struct tp_tx_statem *mystate);
static int tp_tx_adjust_sn(struct tp_tx_statem *mystate, unsigned char tpci);
static void tp_xmitDT(struct tp_user_data *ud, unsigned char tpci1,
		      unsigned char *data, unsigned char len);

/* module start/stop section */

static int __init tp_gen_init(void)
{
    printk(banner);

    return 0;
}

static void __exit tp_gen_exit(void)
{
    return;
}

/* implementation section */

int tp_create_ud(struct sock *sk)
{
    struct tp_user_data *ud;

    if (!(ud = kmalloc(sizeof(struct tp_user_data), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL)))
      return -ENOMEM;

    /* init all variables and pointers to NULL */
    memset(ud, 0, sizeof(struct tp_user_data));

    sk->user_data = ud; /* bind to my sock structure */
    ud->sk = sk;        /* back reference */

    DBG("ud (%p) belongs to socket [%p]\n", ud, sk->socket);

    init_waitqueue_head(&ud->wait);

    /* back reference to my sock structure */
    ud->txstate.ud    = ud;
    ud->txstateoob.ud = ud;
    ud->rxstate.ud    = ud;
    ud->rxstateoob.ud = ud;
    ud->constate.ud   = ud;
    ud->cteststate.ud = ud;

    /* init timer structures */
    init_timer(&ud->txstate.timer);
    init_timer(&ud->txstateoob.timer);
    init_timer(&ud->constate.timer);
    init_timer(&ud->cteststate.timer);

    return 0;
}


void tp_remove_ud(struct tp_user_data *ud)
{
  if (ud) {

    /* delete timers */
    del_timer(&ud->txstate.timer);
    del_timer(&ud->txstateoob.timer);
    del_timer(&ud->constate.timer);
    del_timer(&ud->cteststate.timer);
    
    /* remove txstate tx_buf queue if available */
    if (ud->txstate.tx_queue) {
      DBG("removing tx_queue for ud (%p)\n", ud);
      tp_remove_tx_queue(ud->txstate.tx_queue);
      /* setting ud->txstate.tx_queue = NULL is obsolete here */
    }
    
    /* remove txstateoob tx_buf queue if available */
    if (ud->txstateoob.tx_queue) {
      DBG("removing tx_queue(oob) for ud (%p)\n", ud);
      tp_remove_tx_queue(ud->txstateoob.tx_queue);
      /* setting ud->txstateoob.tx_queue = NULL is obsolete here */
    }
    
    if (ud->rxstate.rx_buf) {
      DBG("ud (%p) rx_buf (%p) size %d\n",
	  ud, ud->rxstate.rx_buf, ud->rxstate.rx_buf_size);
      kfree(ud->rxstate.rx_buf);
    }

    if (ud->rxstateoob.rx_buf) {
      DBG("oob: ud (%p) rx_buf (%p) size %d\n",
	  ud, ud->rxstateoob.rx_buf, ud->rxstateoob.rx_buf_size);
      kfree(ud->rxstateoob.rx_buf);
    }

    kfree(ud);
  }
}


void tp_remove_tx_queue(struct tx_buf *tx_queue)
{
  struct tx_buf *p, *q;

  for (p=tx_queue; p ; ) {
      q = p;
      p = q->next;
      
      DBG("txb (%p) data (%p) size %d\n", q, q->data, q->size);
      
      if (q->data)
	  kfree(q->data);
      kfree(q);
  }
}


void tp_start_engine(struct tp_user_data *ud)
{
  
  ud->txstate.timer.function    = tp_tx_timer;
  ud->txstateoob.timer.function = tp_tx_timer;

  ud->txstate.timer.data    = (unsigned long) &ud->txstate;
  ud->txstateoob.timer.data = (unsigned long) &ud->txstateoob;

  ud->txstate.state         = TX_IDLE;
  ud->txstate.txsn          = 0;
  ud->txstate.arsn          = (ud->parm.bs - 1) % 16;
  ud->txstate.retrycnt      = 0;
  ud->txstate.blkerrcnt     = 0;
  ud->txstate.blkstart      = 1; /* prevent jitter in tp_tx_sendtpdata() */
  ud->txstate.tx_buf_ptr    = 0;
  ud->txstate.lasttxlen     = 0;

  ud->txstateoob.state      = TX_IDLE;
  ud->txstateoob.txsn       = 0;
  ud->txstateoob.arsn       = (ud->parm.bs - 1) % 16;
  ud->txstateoob.retrycnt   = 0;
  ud->txstateoob.blkerrcnt  = 0;
  ud->txstateoob.blkstart   = 1; /* prevent jitter in tp_tx_sendtpdata() */
  ud->txstateoob.tx_buf_ptr = 0;
  ud->txstateoob.lasttxlen  = 0;

  ud->rxstate.state    = RX_RECEIVE;
  ud->rxstate.rxsn     = 0;

  if (!(ud->rxstate.rx_buf)) {

    ud->rxstate.rx_buf_size = 1024; /* start size, which may hopefully be enough */

    if (!(ud->rxstate.rx_buf = kmalloc(ud->rxstate.rx_buf_size,
				       in_interrupt() ? GFP_ATOMIC : GFP_KERNEL))) {
      printk(KERN_ERR "TP: cannot allocate rx buffer memory\n");
      ud->rxstate.rx_buf_size = 0;
      return;
    }
    else {
      ud->rxstate.rx_buf_ptr = 0; /* write position */
      DBG("allocated rxstate.rx_buf at (%p) for ud (%p)\n",
	  ud->rxstate.rx_buf, ud);
    }
  }

  ud->rxstateoob.state    = RX_RECEIVE;
  ud->rxstateoob.rxsn     = 0;

  if (!(ud->rxstateoob.rx_buf)) {

    ud->rxstateoob.rx_buf_size = 1024; /* start size, which may hopefully be enough */

    if (!(ud->rxstateoob.rx_buf = kmalloc(ud->rxstateoob.rx_buf_size,
					  in_interrupt() ? GFP_ATOMIC : GFP_KERNEL))) {
      printk(KERN_ERR "TP: cannot allocate rx buffer memory\n");
      ud->rxstateoob.rx_buf_size = 0;
      return;
    }
    else {
      ud->rxstateoob.rx_buf_ptr = 0; /* write position */
      DBG("allocated rxstateoob.rx_buf at (%p) for ud (%p)\n",
	  ud->rxstateoob.rx_buf, ud);
    }
  }

}

void tp_stop_engine(struct tp_user_data *ud)
{

  DBG("for ud (%p)\n", ud);

  /* hm - what should i do here? */

  /* write some statistic information into the kernel log */
  if (printstats) {

    printk(KERN_INFO
	   "TP: %s-stats for ud (%p): tx_id <%03X> rx_id <%03X>\n",
	   ud->ident, ud, ud->can_tx_id, ud->can_rx_id);
    
    if ((ud->txstate.stat.msgs) || (ud->rxstate.stat.msgs))
      printk(KERN_INFO
	     "TP: stats ud (%p): TX: %d msgs (%d bytes) RX: %d msgs (%d bytes) TX-RETRIES: %d\n",
	     ud, ud->txstate.stat.msgs, ud->txstate.stat.bytes,
	     ud->rxstate.stat.msgs, ud->rxstate.stat.bytes, ud->txstate.stat.retries);
    
    if ((ud->txstateoob.stat.msgs) || (ud->rxstateoob.stat.msgs))
      printk(KERN_INFO
	     "TP: stats OOB ud (%p): TX: %d msgs (%d bytes) RX: %d msgs (%d bytes) TX-RETRIES: %d\n",
	     ud, ud->txstateoob.stat.msgs, ud->txstateoob.stat.bytes,
	     ud->rxstateoob.stat.msgs, ud->rxstateoob.stat.bytes, ud->txstateoob.stat.retries);
  }

}

void tp_rx_data(struct tp_rx_statem *mystate, struct can_frame rxframe)
{
  struct tp_user_data *ud = mystate->ud;
  unsigned char tpci = rxframe.data[0];
  unsigned char newtpci = mystate->rxsn;

  if ((ud->conf & AUTO_DISCONNECT) && (ud->constate.state == CON_CON)) {
    /* stop timer for auto disconnect */
    del_timer(&ud->constate.timer);
    DBG("AUTO_DISCONNECT: stopped timer for ud (%p)\n", ud);
  }

  tpci ^= TPAR; /* bit is in negative logic on CAN */

  if ((tpci & BS_MASK) == mystate->rxsn) { /* correct SN ? */
    DBG("received correct SN = 0x%X for ud (%p)\n", (tpci & BS_MASK), ud);

    if (mystate->state == RX_RECEIVE) {
      tp_rx_store_data(mystate, rxframe.can_dlc, &rxframe.data[1]);
      newtpci = mystate->rxsn; /* update value changed by tp_rx_store_data() */
      newtpci |= TPRS; /* receiver ready */
    }

    if (tpci & TPAR) { /* ACK request */
      newtpci |= TPAK;
      tp_xmitCI(ud, newtpci); /* send ACK */
    }

    if ((tpci & TPEOM) && (newtpci & TPRS)){ /* end of message and data stored succesfully */

      struct sk_buff *skb;
      int err;
      
      DBG("EOM - received complete msg for ud (%p) with %d bytes.\n",
	  ud, mystate->rx_buf_ptr);
      
      if (ud->sk) { /* only when sk exists at this point */
	skb = alloc_skb(mystate->rx_buf_ptr, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	    
	/* copy modified msg_head */
	memcpy(skb_put(skb, mystate->rx_buf_ptr), mystate->rx_buf, mystate->rx_buf_ptr);

	/* set timestamp to last received can data tpdu */
	skb->stamp = ud->stamp;

	/* put into queue to user */
	if ((err = sock_queue_rcv_skb(ud->sk, skb)) < 0) {
	  DBG("sock_queue_rcv_skb failed: %d. ud is (%p)\n", err, ud);
	  kfree_skb(skb);
	}
      }
      else
	DBG("no sk. received msg trashed for ud (%p).\n", ud);

      mystate->stat.msgs++; /* update statistics */
      mystate->stat.bytes += mystate->rx_buf_ptr;

      mystate->rx_buf_ptr = 0; /* clear receive buffer */

      /* unblock any tx path on complete received message? */

      if (ud->conf & HALF_DUPLEX) {
	if (ud->txstate.state == TX_BLOCKED) {
	  DBG("HALF_DUPLEX unblocking ud (%p)\n", ud);
	  ud->txstate.state = TX_IDLE;
	  tp_tx_wakeup(&ud->txstate); /* look for new data */
	}
	if (ud->conf & ENABLE_OOB_DATA) { /* oob tx path on duty? */
	  if (ud->txstateoob.state == TX_BLOCKED) {
	    DBG("HALF_DUPLEX unblocking oob ud (%p)\n", ud);
	    ud->txstateoob.state = TX_IDLE;
	    tp_tx_wakeup(&ud->txstateoob); /* look for new data */
	  }
	}
      }

      /* end of unblock any tx path on complete received message? */
    }

  }
  else { /* incorrect SN */
    DBG("received wrong SN = 0x%X (should be 0x%X) for ud (%p)\n",
	(tpci & BS_MASK), mystate->rxsn, ud);

    if (tpci & TPAR) { /* ACK request */

      /* send ACK with current SN */
      if (mystate->state == RX_RECEIVE)
	tpci = mystate->rxsn | TPRS | TPAK; /* SN + receiver ready */
      else
	tpci = mystate->rxsn | TPAK; /* SN + receiver not ready */
	
      tp_xmitCI(ud, tpci); /* send ACK */

    }
  }
}

static void tp_rx_store_data(struct tp_rx_statem *mystate, unsigned char len, unsigned char *data)
{

  if (len) {

    len--; /* length of payload */

    DBG("adding %d bytes for ud (%p)\n", len, mystate->ud);

    if (mystate->rx_buf_ptr + len >= mystate->rx_buf_size) {

      unsigned char *p;

      if (!(p = kmalloc(mystate->rx_buf_size*2, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL))) {
	printk(KERN_ERR "TP: cannot allocate rx buffer memory\n");
	return;
      }
      else {
	mystate->rx_buf_size *= 2;

	if (mystate->rx_buf)
	  kfree(mystate->rx_buf);

	mystate->rx_buf = p;

	DBG("allocated %d bytes for new rxstate.rx_buf at (%p) for ud (%p)\n",
	    mystate->rx_buf_size, mystate->rx_buf, mystate->ud);
      }
    }

    memcpy(mystate->rx_buf + mystate->rx_buf_ptr, data, len);
    mystate->rx_buf_ptr += len;
    mystate->rxsn++;
    mystate->rxsn &= BS_MASK;

  }
  else {
    DBG("ERROR: DLC is zero(!) for ud (%p)\n", mystate->ud);
  }

}

void tp_tx_wakeup(struct tp_tx_statem *mystate)
{

  struct tp_user_data *ud = mystate->ud;

  if (mystate->state == TX_IDLE) {
    DBG("init tx for ud (%p)\n",ud);
    
    if (ud->flags & TP_BREAK_MODE) {
      DBG("TP_BREAK_MODE: not checking tx_queue.\n");
      ud->flags &= ~TP_BREAK_MODE;
      return;
    }

    if (!(mystate->tx_queue)) {
      DBG("empty tx_queue!\n");
      return;
    }
    /* ok - then grab the first element and send it to the world */
    mystate->state      = TX_SENDING;
    mystate->arsn       = (mystate->txsn + ud->parm.bs - 1) % 16;
    mystate->retrycnt   = 0;
    mystate->blkerrcnt  = 0;
    mystate->tx_buf_ptr = 0;
    mystate->lasttxlen  = 0;
    mystate->blkstart   = 1; /* prevent jitter in tp_tx_sendtpdata() */
    
    tp_tx_sendtpdata(mystate); /* send and check AR, etc. */

  }
  else
    DBG("obsolete wakeup for ud (%p)\n", mystate->ud);

}

void tp_tx_data(struct tp_tx_statem *mystate, struct can_frame rxframe)
{
  /* get the transfer control information byte */
  unsigned char tpci = rxframe.data[0];
  struct tp_user_data *ud = mystate->ud;

  DBG("received tpci 0x%X for ud (%p) state is %d\n",
      tpci, ud, mystate->state);

  if (mystate->state != TX_WAIT_AK)
    return;

  del_timer(&mystate->timer);
  mystate->retrycnt = 0; /* clear retry counter */

  if (!(tpci & TPRS)) { /* receiver not ready */
    DBG("RNR => using timer 'tde' for ud (%p)\n", ud);

    /* wait for extra tx delay in error and RNR conditions */

    mystate->timer.expires = jiffies + mystate->ud->parm.tde;
    add_timer(&mystate->timer);

    tpci &= BS_MASK; /* SN for the rest of this function */

    /* wrong SN ? */
    if (tpci != mystate->txsn)
	tp_tx_adjust_sn(mystate, tpci);

    return;
  }

  tpci &= BS_MASK; /* SN for the rest of this function */

  if (tpci == mystate->txsn) {
    DBG("correct SN 0x%X for ud (%p)\n", tpci, ud);

    /* update pointer to next tx data */
    mystate->tx_buf_ptr += mystate->lasttxlen;

    /* update arsn */
    mystate->arsn = (mystate->txsn + ud->parm.bs - 1) % 16;
    mystate->blkerrcnt = 0;

    if (mystate->tx_buf_ptr >= mystate->tx_queue->size) {

      struct tx_buf *q = mystate->tx_queue;

      DBG("message complete for ud (%p)\n", ud);

      mystate->stat.msgs++; /* update statistics */
      mystate->stat.bytes += mystate->tx_buf_ptr;

      DBG("remove txb: ud (%p) txb (%p) data (%p) size %d\n",
	  ud, q, q->data, q->size);

      mystate->tx_queue = q->next;

      if (q->data)
	kfree(q->data);
      kfree(q);

      if (ud->conf & HALF_DUPLEX) {
	DBG("HALF_DUPLEX blocking ud (%p)\n", ud);
	mystate->state = TX_BLOCKED;
	/* is unblocked when next rx message is processed completely */
      }
      else {
	mystate->state = TX_IDLE;
	tp_tx_wakeup(mystate); /* look for new data */
      }

      if (ud->conf & AUTO_DISCONNECT) {
	if (ud->constate.state == CON_CON) {
	  /* start timer for auto disconnect */
	  /* timer is stopped on data traffic in tp_tx_sendtpdata() and tp_rx_data() */
	  /* this timer is consumed in <canprotocol>_con_timer(), e.g. mcnet_con_timer() */
	  if (ud->parm.tdc) {
	    ud->constate.timer.expires = jiffies + ud->parm.tdc;
	    add_timer(&ud->constate.timer);
	    DBG("AUTO_DISCONNECT: started timer for ud (%p)\n", ud);
	  }
	  else
	    DBG("AUTO_DISCONNECT: timer disabled. ud (%p)\n", ud);
	}
	else
	  DBG("AUTO_DISCONNECT: not connected! ud (%p)\n", ud);
      }

    }
    else
      tp_tx_sendtpdata(mystate);

  }
  else {
    DBG("incorrect SN 0x%X for ud (%p) should be 0x%X\n",
	tpci, mystate->ud, mystate->txsn);

    /* MCNet has a inconistency in the spec here, because it just restarts   */
    /* the ACK timer, which causes a resend in tp_tx_timer() when it gets a  */
    /* wrong SN in send-and-wait mode (BS=1). What if we get infinitely      */
    /* wrong ACKs always within the timeout? => So we simplify the send-and- */
    /* wait-handling and do the same as in the block-transfer-mode (BS>1).   */

    /* handle wrong SN always like this - even in send-and-wait mode */
    if (tp_tx_adjust_sn(mystate, tpci))
      tp_tx_sendtpdata(mystate);
  }
}

static int tp_tx_adjust_sn(struct tp_tx_statem *mystate, unsigned char tpci)
{
  struct tp_user_data *ud = mystate->ud;
  unsigned char lost_seg;
  int ret = 1; /* == OK */

  if (mystate->blkerrcnt >= ud->parm.mnbe){
    DBG("block error count exceeded for ud (%p)\n", ud);
    mystate->retrycnt = 0; /* reload variables */
    ud->tp_disconnect(ud); /* disconnect this channel */
    ret = 0;
  }
  else {
    DBG("Block Error. blkerrcnt = %d for ud (%p)\n", mystate->blkerrcnt, ud);

    if (mystate->txsn < tpci)
      lost_seg = 16 + mystate->txsn - tpci;
    else
      lost_seg = mystate->txsn - tpci;

    if (lost_seg < 1) {
      DBG("internal ERROR: lostseg < 1 for ud (%p)\n", ud);
      lost_seg = 1; /* uuh - bad adjust */
    }

    /* we did not increment the tx_buf_ptr due to the buffer end problem */
    lost_seg--; /* => do not count the last message */

    if (mystate->tx_buf_ptr >= (lost_seg * 7))
      mystate->tx_buf_ptr -= (lost_seg * 7);
    else {
      mystate->tx_buf_ptr = 0;  /* uuh - bad adjust */
      DBG("internal ERROR: tx_buf_ptr underrun for ud (%p)\n", ud);
    }

    mystate->txsn = tpci;
    mystate->arsn = (mystate->txsn + ud->parm.bs - 1) % 16;
    mystate->blkerrcnt++;
    mystate->blkstart = 1; /* prevent jitter in tp_tx_sendtpdata() */
  }

  return ret;
}

static void tp_tx_timer(unsigned long data)
{
  struct tp_tx_statem *stm = (struct tp_tx_statem *) data;

  switch (stm->state) {
  case TX_WAIT_AK:

    /* ACK timer expired for data telegram */

    if (stm->retrycnt < stm->ud->parm.mnt) {
      stm->retrycnt++;
      stm->stat.retries++;
      DBG("DT timeout. sending number %d\n", stm->retrycnt);

      if (!(stm->txsn)) /* had been increased by tp_tx_sendtpdata() */
	stm->txsn = 0x0F;
      else
	stm->txsn--;

      tp_tx_sendtpdata(stm);
    }
    else {
      del_timer(&stm->timer);
      stm->retrycnt = 0; /* reload variables */
      stm->ud->tp_disconnect(stm->ud); /* disconnect this channel */
      /* standby */
    }
    break;

  case TX_SENDING:
    /* this is also done in tp_tx_data() when receiving an ACK */
    DBG("tx delay is over. lasttxlen is %s\n",
	(stm->lasttxlen == 7)?"ok":"WRONG!");
    stm->tx_buf_ptr += 7; /* we are in surely block transfer mode */
    tp_tx_sendtpdata(stm);
    break;

  default:
    DBG("unknown state %d!\n", stm->state);
    break;
  }
}

static void tp_tx_sendtpdata(struct tp_tx_statem *mystate)
{
  struct tp_user_data *ud = mystate->ud;
  unsigned char *data = mystate->tx_queue->data + mystate->tx_buf_ptr;
  unsigned char tpci1 = (TPDT | (mystate->txsn & BS_MASK));
  
  if ((ud->conf & AUTO_DISCONNECT) && (ud->constate.state == CON_CON)) {
    /* stop timer for auto disconnect */
    del_timer(&ud->constate.timer);
    DBG("AUTO_DISCONNECT: stopped timer for ud (%p)\n", ud);
  }

  if (ud->flags & TP_BREAK_MODE) {
    DBG("BREAK: force last packet for of msg ud (%p)\n", ud);
    mystate->tx_buf_ptr = mystate->tx_queue->size;
  }

  if ((mystate->lasttxlen = mystate->tx_queue->size - mystate->tx_buf_ptr) < 8) {
    DBG("len = %d => EOM AR for ud (%p)\n", mystate->lasttxlen, ud);
    tpci1 |= (TPEOM | TPAR);
  }
  else {
    mystate->lasttxlen = 7;

    if (mystate->txsn == mystate->arsn) {
      DBG("AR for ud (%p)\n", ud);
      tpci1 |= TPAR;
    }
  }

  tp_xmitDT(ud, tpci1, data, mystate->lasttxlen);

  mystate->txsn++; /* to match SN in ACK */
  mystate->txsn &= BS_MASK;

  if (tpci1 & TPAR) { /* wait for ACK? */
    DBG("TX_WAIT_AK for ud (%p)\n", ud);
    mystate->state = TX_WAIT_AK;

    if (ud->parm.ta) { /* if ACK timeout enabled */
      mystate->timer.expires = jiffies + ud->parm.ta + ud->parm.tn;
      add_timer(&mystate->timer);
    }
    /* who the hell specified /disabled/ ACK timeouts ? */
  }
  else {
    DBG("TX_SENDING for ud (%p)\n", ud);

    /* wait for tx delay and send next tpdata (if needed) */
    mystate->timer.expires = jiffies + ud->parm.td;

    /* to prevent jitter at first timeout sending */
    if (mystate->blkstart) {
      DBG("preventing jitter for ud (%p)\n", ud);
      mystate->timer.expires++;
      mystate->blkstart = 0;
    }
    add_timer(&mystate->timer);
    mystate->state = TX_SENDING;
  }
}

static void tp_xmitDT(struct tp_user_data *ud, unsigned char tpci1,
		      unsigned char *data, unsigned char len)
{
  static struct can_frame cf;

  len &= 0x07; /* ensure len < 8 */

  cf.can_dlc = len + 1; /* data length + tpci length */

  cf.data[0] = tpci1^TPAR;  /* bit is in negative logic on CAN */
  memcpy(&cf.data[1], data, cf.can_dlc);
  tp_xmit_frame(ud, &cf);

}

void tp_xmitCI(struct tp_user_data *ud, unsigned char tpci1)
{
  static struct can_frame cf;

  cf.can_dlc = 1;
  cf.data[0] = tpci1;
  tp_xmit_frame(ud, &cf);

}

void tp_xmit_frame(struct tp_user_data *ud, struct can_frame *txframe)
{

  struct sk_buff *skb;
  struct net_device *dev;
  int ifindex = ud->sk->bound_dev_if;

  /* when are in trace mode, we have to be silent. This is done here. */
  if (ud->conf & TRACE_MODE) {
    DBG("omitted tx due to TRACE_MODE\n");
    return;
  }

  txframe->can_id = ud->can_tx_id;

  DBG_FRAME("TP: tp_xmit_frame: sending frame", txframe);

  skb = alloc_skb(sizeof(struct can_frame), in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);

  if (!skb)
    return;

  memcpy(skb_put(skb, sizeof(struct can_frame)), txframe, sizeof(struct can_frame));

  if (ifindex) {
    dev = dev_get_by_index(ifindex);

    if (dev) {
      skb->dev = dev;
      can_send(skb);
      dev_put(dev);
    }
  }
  else
    DBG("no bound device!\n");

}

module_init(tp_gen_init);
module_exit(tp_gen_exit);


#ifdef EXPORT_SYMTAB
EXPORT_SYMBOL(tp_create_ud);
EXPORT_SYMBOL(tp_remove_ud);
EXPORT_SYMBOL(tp_remove_tx_queue);
EXPORT_SYMBOL(tp_start_engine);
EXPORT_SYMBOL(tp_stop_engine);
EXPORT_SYMBOL(tp_xmit_frame);
EXPORT_SYMBOL(tp_xmitCI);
EXPORT_SYMBOL(tp_tx_wakeup);
EXPORT_SYMBOL(tp_tx_data);
EXPORT_SYMBOL(tp_rx_data);
#endif
