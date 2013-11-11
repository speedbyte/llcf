/*
 *  mcnet.c
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/uio.h>
#include <linux/poll.h>
#include <net/sock.h>

#include "af_can.h"
#include "version.h"
#include "tp_gen.h"
#include "mcnet.h"

RCSID("$Id: mcnet.c,v 1.40 2006/04/10 09:37:39 ethuerm Exp $");

#ifdef DEBUG
MODULE_PARM(debug, "1i");
static int debug = 0;
#define DBG(args...)       (debug & 1 ? \
	                       (printk(KERN_DEBUG "MCNET %s: ", __func__), \
			        printk(args)) : 0)
#define DBG_FRAME(args...) (debug & 2 ? can_debug_cframe(args) : 0)
#define DBG_SKB(skb)       (debug & 4 ? can_debug_skb(skb) : 0)
#else
#define DBG(args...)
#define DBG_FRAME(args...)
#define DBG_SKB(skb)
#endif

#define NAME "MCNet for LLCF"
#define IDENT "mcnet"
static __initdata const char banner[] = BANNER(NAME);

MODULE_DESCRIPTION(NAME);
MODULE_LICENSE("Volkswagen Group closed source");
MODULE_AUTHOR("Oliver Hartkopp <oliver.hartkopp@volkswagen.de>, "
	      "Urs Thuermann <urs.thuermann@volkswagen.de>");

static void mcnet_notifier(unsigned long msg, void *data);
static int mcnet_release(struct socket *sock);
static int mcnet_bind   (struct socket *sock, struct sockaddr *uaddr, int len);
static int mcnet_connect(struct socket *sock, struct sockaddr *uaddr, int len, int flags);
static unsigned int mcnet_poll(struct file *file, struct socket *sock,
			     poll_table *wait);
static int mcnet_getname(struct socket *sock, struct sockaddr *uaddr, int *len, int peer);
static int mcnet_accept (struct socket *sock, struct socket *newsock, int flags);
static int mcnet_listen (struct socket *sock, int len);
static int mcnet_setsockopt(struct socket *sock, int level, int optname,
			   char *optval, int optlen);
static int mcnet_getsockopt(struct socket *sock, int level, int optname,
			   char *optval, int *optlen);
static int mcnet_sendmsg(struct socket *sock, struct msghdr *msg, int size,
		       struct scm_cookie *scm);
static int mcnet_recvmsg(struct socket *sock, struct msghdr *msg, int size,
		       int flags, struct scm_cookie *scm);
static void mcnet_rx_handler(struct sk_buff *skb, void *data);
static void mcnet_initdefaults(struct tp_user_data *ud);
static int mcnet_bind_dev(struct sock *sk, struct sockaddr *uaddr);
static void mcnet_con_data(struct tp_cx_statem *mystate, struct can_frame rxframe);
static void mcnet_ctest_data(struct tp_cx_statem *mystate, struct can_frame rxframe);
static void mcnet_con_timer(unsigned long data);
static void mcnet_ctest_timer(unsigned long data);
static void mcnet_start_engine(struct tp_user_data *ud);
static void mcnet_stop_engine(struct tp_user_data *ud);
static void mcnet_disconnect(struct tp_user_data *ud);
static void mcnet_create_and_send_cs(struct tp_cx_statem *mystate);
static void mcnet_create_and_send_ct(struct tp_cx_statem *mystate);

static struct proto_ops mcnet_ops = {
    .family        = PF_CAN,
    .release       = mcnet_release,
    .bind          = mcnet_bind,
    .connect       = mcnet_connect,
    .socketpair    = sock_no_socketpair,
    .accept        = mcnet_accept,
    .getname       = mcnet_getname,
    .poll          = mcnet_poll,
    .ioctl         = 0,
    .listen        = mcnet_listen,
    .shutdown      = sock_no_shutdown,
    .setsockopt    = mcnet_setsockopt,
    .getsockopt    = mcnet_getsockopt,
    .sendmsg       = mcnet_sendmsg,
    .recvmsg       = mcnet_recvmsg,
    .mmap          = sock_no_mmap,
    .sendpage      = sock_no_sendpage,
};

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* module specific stuff                                                     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int __init mcnet_init(void)
{
    printk(banner);

    can_proto_register(CAN_MCNET, &mcnet_ops);
    return 0;
}

static void __exit mcnet_exit(void)
{
    can_proto_unregister(CAN_MCNET);
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* socket specific stuff                                                     */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void mcnet_notifier(unsigned long msg, void *data)
{
    struct sock *sk = (struct sock *)data;

    DBG("called for sock %p\n", sk);

    switch (msg)
    {
    case NETDEV_UNREGISTER:
	sk->bound_dev_if = 0;
    case NETDEV_DOWN:
	sk->err = ENETDOWN;
	sk->error_report(sk);
    }
}

static int mcnet_release(struct socket *sock)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);

  if (!ud) {
    DBG("no user_data available.\n");
    return 0;
  }

  if (ud->flags & SKREF) { /* currently two sockets in use? */

    ud->flags &= ~SKREF; /* only one socket refers to the user_data ow */

    if (sock != ud->lsock) { /* communication socket? */
      DBG("comm socket [%p]: ud is (%p)\n", sock, ud);
      mcnet_disconnect(ud); /* yes: disconnect this channel */

      if (ud->conf & RELOAD_DEFAULTS)
	mcnet_initdefaults(ud);

      sock_graft(sk, ud->lsock); /* bind sk-structure back to listen socket */
    }
    else
      DBG("listen socket [%p]: ud is (%p)\n", sock, ud);

  }
  else { /* shut down (last) socket */

    if ((sock != ud->lsock) || (ud->conf & ACTIVE)) { /* communication socket? */
      DBG("FINAL comm socket [%p]: ud is (%p)\n", sock, ud);
      mcnet_disconnect(ud); /* yes: disconnect this channel */
    }
    else
      DBG("FINAL listen socket [%p]: ud is (%p)\n", sock, ud);

    mcnet_stop_engine(ud); /* stop TP */

    tp_remove_ud(ud);

    if (sk->bound_dev_if) {
	struct net_device *dev = dev_get_by_index(sk->bound_dev_if);
	if (dev) {
	    can_dev_unregister(dev, mcnet_notifier, sk);
	    dev_put(dev);
	}
    } else
	DBG("socket not bound\n");

    sock_put(sk);
  }

  return 0;
}


static int mcnet_getname(struct socket *sock, struct sockaddr *uaddr, int *len, int peer)
{
  struct sockaddr_can *addr = (struct sockaddr_can*)uaddr;
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);

  if ((ud) && (sk->bound_dev_if)) {

    addr->can_family  = AF_CAN;
    addr->can_ifindex = sk->bound_dev_if;

    if (peer) {
      addr->can_addr.mcnet.tx_id = ud->can_rx_id;
      addr->can_addr.mcnet.rx_id = ud->can_tx_id;
    }
    else {
      addr->can_addr.mcnet.tx_id = ud->can_tx_id;
      addr->can_addr.mcnet.rx_id = ud->can_rx_id;
    }

    *len = sizeof(*addr);
  
    return 0;
  }
  else
    return -EOPNOTSUPP;
}

static struct tp_user_data *mcnet_create_ud(struct socket *sock, struct sock *sk)
{
  struct tp_user_data *ud;

  if (tp_create_ud(sk))
    return NULL;

  ud = tp_sk(sk);
  ud->lsock = sock;
  mcnet_initdefaults(ud);
  return ud;
}


/* ------------------------------------------------------------------------- */
/*                                                                           */
/* socket specific stuff (communication client functions)                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int mcnet_connect(struct socket *sock, struct sockaddr *uaddr, int len, int flags)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);

  int ret = 0;

  if (!ud && !(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  if (ud->flags & TP_ONLINE) { /* big statemachine already running? */
    if (ud->constate.state == CON_CON)
      return -EISCONN;
    else
      return -EALREADY;
  }

  DBG("starting engine in client mode\n");

  if (ret = mcnet_bind_dev(sk, uaddr))
    return ret;
    
  ud->conf |= ACTIVE; /* client mode: initiating CS */
    
  mcnet_start_engine(ud); /* willy go! */

  /* block process until connection established */
  if (wait_event_interruptible(ud->wait, ud->constate.state != CON_STARTUP))
    return -ERESTARTSYS;

  if (ud->constate.state != CON_CON) {
    DBG("connection timed out. ud is (%p)\n", ud);
    return -ETIMEDOUT;
  }

  DBG("CONNECTED. ud is (%p)\n", ud);
  sk->state = TCP_ESTABLISHED;

  return 0;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* socket specific stuff (communication server functions)                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int mcnet_bind(struct socket *sock, struct sockaddr *uaddr, int len)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);

  int ret = 0;

  if (ud)
    return -EADDRINUSE;

  if (!(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  if (ret = mcnet_bind_dev(sk, uaddr))
    return ret;

  ud->conf &= ~ACTIVE; /* server mode: wait for CS */
  
  /* the rest of the initialisation is done in mcnet_listen() */

  return 0;
}

static int mcnet_listen (struct socket *sock, int len)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);
  int ret = 0;

  if (!ud) { /* need to create initial user data ? */
    DBG("sorry: not bound.\n");
    return -EOPNOTSUPP;
  }

  if (ud->conf & ACTIVE) { /* client mode? */
    DBG("not in server mode\n");
    return -EISCONN;
  }
  
  if ((ud->flags & SKREF) || (sock != ud->lsock)) {
    DBG("already listening. socket [%p]!\n", sock);
    return -EOPNOTSUPP;
  }

  if (ud->flags & TP_ONLINE) { /* big statemachine already running? */
    if (ud->constate.state == CON_CON)
      return -EISCONN;
    else
      return -EALREADY;
  }
  else {
    DBG("starting engine in server mode\n");
    mcnet_start_engine(ud); /* willy go! */
  }
  return ret;
}

static int mcnet_accept (struct socket *sock, struct socket *newsock, int flags)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);
  int ret = 0;

  if (!ud) {
    DBG("not listening / not bound.\n");
    return -EOPNOTSUPP;
  }

  if (ud->conf & ACTIVE) { /* client mode? */
    DBG("not in server mode\n");
    return -EISCONN;
  }
  
  if ((ud->flags & SKREF) || (sock != ud->lsock)) {
    DBG("we are already accepted. socket [%p]!\n", sock);
    return -EISCONN;
  }

  DBG("sock [%p/%p] newsock [%p/%p]\n", sock, sock->sk, newsock, newsock->sk);

  /* block process until connection established */
  if (ret = wait_event_interruptible(ud->wait, ud->constate.state != CON_WAIT_CS)) {
    DBG("wait_event_interruptible with errors %d\n", ret);
    return -ERESTARTSYS;
  }

  if (ud->constate.state == CON_CON) {
    DBG("CONNECTED. ud is (%p)\n", ud);

    sock_graft(sk, newsock); /* bind sk-structure (also) to new socket */
    ud->flags |= SKREF; /* indicate two sockets are using the same user_data */
    newsock->sk->state = TCP_ESTABLISHED;

    return 0;
  }
  else{
    DBG("connection timed out. ud is (%p)\n", ud);
    return -ETIMEDOUT;
  }
  return ret;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* socket specific stuff (socket option functions)                           */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int mcnet_setsockopt(struct socket *sock, int level, int optname,
			   char *optval, int optlen)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);

  if (level != SOL_CAN_MCNET) {
    DBG("wrong socket level.\n");
    return -EINVAL;
  }

  if (!ud && !(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  /* in the case of the server-mode the socket options may be set for either */
  /* the parent socket or the communication socket - so no distinction here */

  switch (optname) {

  case CAN_MCNET_OPT: {
    
    struct can_mcnet_options *opts = (struct can_mcnet_options *) optval;

    DBG("CAN_MCNET_OPT for ud (%p)\n", tp_sk(sk));

    ud->parm.bs = opts->blocksize & BS_MASK;

    if (!ud->parm.bs)
      ud->parm.bs = 1; /* fix wrong parameter */
    
    /* transmit delay for DT TPDU's */
    ud->parm.td = timeval2jiffies(&opts->td, 1);

    ud->conf &= ~MCNET_CONFIG_MASK; /* clear possible flags */
    ud->conf |= (opts->config & MCNET_CONFIG_MASK); /* set new values */

    DBG("TD=%dms BS=%d conf=0x%X\n",
	(int)JIFF2MS(ud->parm.td), ud->parm.bs, ud->conf);
    break;
  }
  
  default:
    break;
    
  } /* switch (optname) */

  return 0;
}

static int mcnet_getsockopt(struct socket *sock, int level, int optname,
			   char *optval, int *optlen)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);

  if (level != SOL_CAN_MCNET) {
    DBG("wrong socket level.\n");
    return -EINVAL;
  }

  if (!ud && !(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  /* in the case of the server-mode the socket options may be set for either */
  /* the parent socket or the communication socket - so no distinction here */

  DBG("(defunct) for ud (%p)\n", tp_sk(sk));

  return 0;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* socket specific stuff (get data from the user app)                        */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int mcnet_sendmsg(struct socket *sock, struct msghdr *msg, int size,
		       struct scm_cookie *scm)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);
  struct tx_buf *txb,*p;
  unsigned char *data;
  int err, ret = size;

  if (!ud && !(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  if ((ud->flags & SKREF) && (sock == ud->lsock)) {
    DBG("unusable listen socket [%p]!\n", sock);
    return -ENOTCONN;
  }

  if (ud->conf & TRACE_MODE) { /* in trace mode you can't send data */
    DBG("socket [%p] is in TRACE_MODE!\n", sock);
    return -EOPNOTSUPP;
  }

  if (ud->constate.state == CON_CON) { /* only when connected */
    
    if (!(txb = kmalloc(sizeof(struct tx_buf), GFP_KERNEL)))
      return -ENOMEM;
    
    if (!(data = kmalloc(size, GFP_KERNEL))) {
      kfree(txb);
      return -ENOMEM;
    }
    
    DBG("ud (%p) txb (%p) data (%p) size %d\n", tp_sk(sk), txb, data, size);

    if ((err = memcpy_fromiovec(data, msg->msg_iov, size)) < 0)
      return err;

    txb->data = data;
    txb->size = size;
    txb->next = NULL;

    /* connect to end of tx_buf */
    if (ud->txstate.tx_queue) {
	
      for (p=ud->txstate.tx_queue; p->next ; p=p->next)
	;

      p->next = txb; /* p->next == NULL => connect txb */
      
    }
    else
      ud->txstate.tx_queue=txb;

    tp_tx_wakeup(&ud->txstate); /* start or wake up transmission */
  }
  else {
    DBG("currently DISCONNECTED!\n");
    ret = -ENOTCONN;
  }
  
  return ret;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* socket specific stuff (bring data to the user app)                        */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int mcnet_recvmsg(struct socket *sock, struct msghdr *msg, int size,
		       int flags, struct scm_cookie *scm)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);
  struct sk_buff *skb;
  int error = 0;
  int noblock;

  if (!ud && !(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  if ((ud->flags & SKREF) && (sock == ud->lsock)) {
    DBG("unusable listen socket [%p]!\n", sock);
    return -ENOTCONN;
  }

  DBG("ud is (%p)\n", tp_sk(sk));

  noblock =  flags & MSG_DONTWAIT;
  flags   &= ~MSG_DONTWAIT;
  if (!(skb = skb_recv_datagram(sk, flags, noblock, &error))) {
    if (sk->shutdown & RCV_SHUTDOWN)
      return 0;
    return error;
  }
  
  DBG("delivering skbuff %p\n", skb);
  DBG_SKB(skb);

  if (skb->len < size)
    size = skb->len;
  if ((error = memcpy_toiovec(msg->msg_iov, skb->data, size)) < 0) {
    skb_queue_head(&sk->receive_queue, skb);
    return error;
  }
  
  sock_recv_timestamp(msg, sk, skb);

  DBG("freeing sock [%p], skbuff %p\n", sk, skb);
  skb_free_datagram(sk, skb);
  
  return size;
}

static unsigned int mcnet_poll(struct file *file, struct socket *sock,
			     poll_table *wait)
{
  struct sock *sk = sock->sk;
  struct tp_user_data *ud = tp_sk(sk);
  unsigned int mask = 0;

  if (!ud && !(ud = mcnet_create_ud(sock, sk)))
    return -ENOMEM;

  if ((ud->flags & SKREF) && (sock == ud->lsock)) {
    DBG("unusable listen socket [%p]!\n", sock);
    return -ENOTCONN;
  }

  mask = datagram_poll(file, sock, wait);
  return mask;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* mcnet start and stop functions                                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void mcnet_start_engine(struct tp_user_data *ud)
{
  struct net_device *dev;

  if (!ud->sk->bound_dev_if) {
      DBG("socket not bound\n");
      return;
  }

  /* register rx can_id from llcf */

  DBG("can_rx_register() for can_id <%03X>. ud is (%p)\n", ud->can_rx_id, ud);

  dev = dev_get_by_index(ud->sk->bound_dev_if);
  if (dev) {
    can_rx_register(dev, ud->can_rx_id, CAN_SFF_MASK, mcnet_rx_handler, ud, IDENT);
    dev_put(dev);
  }

  tp_start_engine(ud); /* init generic part */

  ud->constate.retrycnt   = 0;
  ud->cteststate.retrycnt = 0;

  ud->constate.timer.function   = mcnet_con_timer;
  ud->cteststate.timer.function = mcnet_ctest_timer;

  ud->constate.timer.data   = (unsigned long) &ud->constate;
  ud->cteststate.timer.data = (unsigned long) &ud->cteststate;

  if (ud->conf & TRACE_MODE) {

    /* in trace mode we are allways connected */
    ud->constate.state   = CON_CON;
    ud->cteststate.state = CT_DISABLED;

  }
  else {

    if (ud->conf & USE_CONNECTIONTEST)
      ud->cteststate.state = CT_WAIT_CON;
    else
      ud->cteststate.state = CT_DISABLED;

    if (ud->conf & ACTIVE) {
      ud->constate.state = CON_STARTUP;
      mcnet_create_and_send_cs(&ud->constate);
    }
    else
      ud->constate.state = CON_WAIT_CS;
  }

  ud->flags |= TP_ONLINE;

}

static void mcnet_stop_engine(struct tp_user_data *ud)
{

  DBG("can_rx_unregister() for can_id <%03X>. ud is (%p)\n",
      ud->can_rx_id, ud);

  if (ud->sk->bound_dev_if) {
    struct net_device *dev = dev_get_by_index(ud->sk->bound_dev_if);
    if (dev) {
      can_rx_unregister(dev, ud->can_rx_id, CAN_SFF_MASK, mcnet_rx_handler, ud);
      dev_put(dev);
    }
  } else
    DBG("socket not bound\n");

  tp_stop_engine(ud); /* shutdown generic part */

  /* anything mcnet specific to stop here ? */

  ud->flags &= ~TP_ONLINE;

}

static void mcnet_initdefaults(struct tp_user_data *ud)
{

  ud->tp_disconnect = mcnet_disconnect; /* reference for tp generic part */

  ud->conf  = MCNET_CONFIG_DEFAULT;
  ud->ident = IDENT;

  ud->parm.bs   = MCNET_BS_DEFAULT; /* blocksize. max number of unacknowledged DT TPDU's */
  ud->parm.mnt  = 10;               /* max number of transmissions DT TPDU */
  ud->parm.mntc = 10;               /* max number of transmissions CS TPDU */
  ud->parm.mnct = 10;               /* max number of transmissions CT TPDU */
  ud->parm.mnbe = 5;                /* max number of block errors */

  ud->parm.ta   = MS2JIFF(100);    /* ACK timeout for DT TPDU */
  ud->parm.tac  = MS2JIFF(1000);   /* ACK timeout for CS TPDU */
  ud->parm.tat  = MS2JIFF(100);    /* ACK timeout for CT TPDU */

  ud->parm.tct  = MS2JIFF(1000);   /* time between consecutive CT TPDU's */

  ud->parm.td   = MS2JIFF(MCNET_TD_DEFAULT); /* transmit delay for DT TPDU's */
  ud->parm.tde  = MS2JIFF(1000);   /* transmit delay for DT TPDU's in RNR conditions */

  ud->parm.tn   = MS2JIFF(0);      /* bus latency. unused in MCNet */

  DBG("BS=%d conf=0x%X\n", ud->parm.bs, ud->conf);
}

static void mcnet_disconnect(struct tp_user_data *ud)
{
  DBG("ud is (%p)\n", ud);

  ud->sk->shutdown |= RCV_SHUTDOWN;

  if (ud->constate.state == CON_CON)
    wake_up_interruptible(ud->sk->sleep); /* wakeup read() */
  else
    wake_up_interruptible(&ud->wait);     /* wakeup connect() or accept() */

  ud->constate.state = CON_DISC;
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* tp receive from CAN functions                                             */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void mcnet_rx_handler(struct sk_buff *skb, void *data)
{
  struct tp_user_data *ud = (struct tp_user_data*)data;
  struct can_frame rxframe;
  struct timeval tv;
  unsigned char tpci1;

  DBG("Called with ud (%p)\n", ud);

  if (skb->len == sizeof(rxframe)) {
    memcpy(&rxframe, skb->data, sizeof(rxframe));
    tv = skb->stamp;
    kfree_skb(skb);
    DBG("got can_frame with can_id <%03X>\n", rxframe.can_id);
  }
  else {
    DBG("Wrong skb->len = %d\n", skb->len);
    kfree_skb(skb);
    return;
  }

  DBG_FRAME("MCNET: mcnet_rx_handler: CAN frame", &rxframe);

  if (ud->can_rx_id != rxframe.can_id) {
    DBG("ERROR! Got wrong can_id!\n");
    return;
  }

  tpci1 = rxframe.data[0]; /* get the transfer control information byte */
    
  if ((ud->conf & TRACE_MODE) && (tpci1 & 0x80)) /* non data TPDU in trace mode? */
    return; /* do not confuse the statemachine ... */

  if (tpci1 & 0x80) { /* non data TPDU */

    if (tpci1 & 0x10) { /* ACK for tx path */

      if (tpci1 & 0x40) /* OOB */
	tp_tx_data(&ud->txstateoob, rxframe);
      else
	tp_tx_data(&ud->txstate, rxframe);
    }
    else{
      
      switch (tpci1 & 0x0F) {

      case 0x00: /* CS connection setup */
      case 0x03: /* CT connection test request */
      case 0x08: /* DC disconnect */
	mcnet_con_data(&ud->constate, rxframe);
	break;

      case 0x01: /* CA connection acknowledge */
	mcnet_ctest_data(&ud->cteststate, rxframe);
	mcnet_con_data(&ud->constate, rxframe);
	break;

      default:
	DBG("ERROR! Got wrong TPCI1 0x%02X!\n", tpci1);
	break;
      }
    }
  }
  else{ /* data TPDU for rx path */
    ud->stamp = tv;
    
    if (tpci1 & 0x40) /* OOB */
      tp_rx_data(&ud->rxstateoob, rxframe);
    else
      tp_rx_data(&ud->rxstate, rxframe);
  }
  
  return;
  
}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* mcnet send to CAN functions                                                */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void mcnet_create_and_send_ct(struct tp_cx_statem *mystate)
{
  struct tp_user_data *ud = mystate->ud;

  DBG("called\n");
  del_timer(&mystate->timer);
  tp_xmitCI(ud, TPCT);
  mystate->state = CT_WAIT_AK;
  mystate->timer.expires = jiffies + ud->parm.tct;
  add_timer(&mystate->timer);

}

static void mcnet_create_and_send_cs(struct tp_cx_statem *mystate)
{

  struct can_frame csframe;
  struct tp_user_data *ud = mystate->ud;

  DBG("called\n");
  del_timer(&mystate->timer);

  csframe.can_dlc = 3;
  csframe.data[0] = TPCS; /* currently no OOB implemented */
  csframe.data[1] = ud->parm.bs & 0x0F; /* blocksize */
  csframe.data[2] = 0; /* Zero Byte for TRX wakeup (see MCNet spec) */

  tp_xmit_frame(ud, &csframe);

  mystate->timer.expires = jiffies + ud->parm.tac + ud->parm.tn;
  add_timer(&mystate->timer);

}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* mcnet connection setup and connection test functions                       */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static void mcnet_con_data(struct tp_cx_statem *mystate, struct can_frame rxframe)
{

  struct tp_user_data *ud = mystate->ud;
  unsigned char tpci = rxframe.data[0]; /* get the transfer control information byte */

  switch (tpci & ~TPOOB) {

  case TPCS:
    del_timer(&mystate->timer);
    mystate->state = CON_CON;
    DBG("TPCS CONNECTED!\n");

    /* adjust connection parameters */
    if (rxframe.data[1] < ud->parm.bs){
      ud->parm.bs = rxframe.data[1];
      DBG("adjusted BS to %d.\n", ud->parm.bs);
    }
    
    /* send CA */
    
    rxframe.can_dlc = 2;
    rxframe.data[0] = TPCA; /* currently no OOB implemented */
    rxframe.data[1] = ud->parm.bs & 0x0F; /* blocksize */
    
    tp_xmit_frame(ud, &rxframe);
    
    if(ud->cteststate.state == CT_WAIT_CON) /* implies USE_CONNECTIONTEST set */
      mcnet_create_and_send_ct(&ud->cteststate);
    
    wake_up_interruptible(&ud->wait);
    
    break;
    
  case TPCA:
    if (mystate->state == CON_STARTUP) {
      del_timer(&mystate->timer);
      mystate->state = CON_CON;
      DBG("TPCA CONNECTED!\n");

      /* adjust connection parameters */
      if (rxframe.data[1] < ud->parm.bs){ /* blocksize */
	ud->parm.bs = rxframe.data[1];
	DBG("adjusted BS to %d.\n", ud->parm.bs);
      }

      if (ud->cteststate.state == CT_WAIT_CON) /* implies USE_CONNECTIONTEST set */
	mcnet_create_and_send_ct(&ud->cteststate);

      wake_up_interruptible(&ud->wait);

    }
    break;

  case TPCT:
    if (mystate->state == CON_CON) {

      /* send CA */

      rxframe.can_dlc = 2;
      rxframe.data[0] = TPCA; /* currently no OOB implemented */
      rxframe.data[1] = ud->parm.bs & 0x0F; /* blocksize */

      tp_xmit_frame(ud, &rxframe);

      DBG("TPCT CONNECTED!\n");

    }
    break;

  default:
    DBG("unknown tpci 0x%02X\n", tpci);
    break;
  }
}

static void mcnet_con_timer(unsigned long data)
{

  struct tp_cx_statem *stm = (struct tp_cx_statem *) data;
  struct tp_user_data *ud = stm->ud;

  switch (stm->state) {

  case CON_STARTUP:

    /* ACK timer expired for connection setup */

    if (stm->retrycnt < ud->parm.mntc) {
      stm->retrycnt++;
      DBG("CS timeout. sending number %d\n",stm->retrycnt);
      mcnet_create_and_send_cs(stm);
    }
    else {
      del_timer(&stm->timer);
      stm->retrycnt = 0; /* reload variables */
      mcnet_disconnect(ud); /* disconnect this channel */
      /* standby */
    }
    break;

  case CON_CON:

    /* disconnect timer expired for this connection */

    if (ud->conf & AUTO_DISCONNECT) {
      DBG("AUTO_DISCONNECT timeout for ud (%p)\n", ud);
      del_timer(&stm->timer);
      stm->retrycnt = 0; /* reload variables */
      mcnet_disconnect(ud); /* disconnect this channel */
      /* standby */
    }
    break;

  default:
    DBG("unknown state %d\n", stm->state);
    break;
  }
}

static void mcnet_ctest_data(struct tp_cx_statem *mystate, struct can_frame rxframe)
{
  /* we surely got a connection acknowledge,     */
  /* so we do not need any specific testing here */

  if(mystate->state == CT_WAIT_AK){
    mystate->state    = CT_WAIT_TCT; /* just make a transition */
    mystate->retrycnt = 0;           /* reset retry counter */
  }

}

static void mcnet_ctest_timer(unsigned long data)
{
  struct tp_cx_statem *stm = (struct tp_cx_statem *) data;

  if(stm->state == CT_WAIT_AK){

    /* ACK timer expired for connection test */

    if (stm->retrycnt < stm->ud->parm.mnct) {
      stm->retrycnt++;
      DBG("CT timeout. sending number %d\n", stm->retrycnt);
      mcnet_create_and_send_ct(stm);
    }
    else {
      stm->retrycnt = 0; /* reload variables */
      stm->state = CT_WAIT_CON;
      DBG("disconnecting!\n");
      mcnet_disconnect(stm->ud); /* disconnect this channel */
    }
  }
  else
    mcnet_create_and_send_ct(stm);

}

/* ------------------------------------------------------------------------- */
/*                                                                           */
/* mcnet utility functions                                                    */
/*                                                                           */
/* ------------------------------------------------------------------------- */

static int mcnet_bind_dev(struct sock *sk, struct sockaddr *uaddr)
{
  struct tp_user_data *ud = tp_sk(sk);
  struct sockaddr_can *addr = (struct sockaddr_can *) uaddr;
  struct net_device *dev = NULL;

  ud->can_tx_id = addr->can_addr.mcnet.tx_id;
  ud->can_rx_id = addr->can_addr.mcnet.rx_id;

  /* bind a device to this socket */
  if (!addr->can_ifindex)
    return -ENODEV;

  dev = dev_get_by_index(addr->can_ifindex);
  if (dev) {
    sk->bound_dev_if = dev->ifindex;
    can_dev_register(dev, mcnet_notifier, sk);
    dev_put(dev);
  }
  else {
    printk(KERN_DEBUG "could not find device %d\n", addr->can_ifindex);
    return -ENODEV;
  }
  
  DBG("socket [%p] to device idx %d\n", sk->socket, sk->bound_dev_if);
  
  return 0;
  
}


module_init(mcnet_init);
module_exit(mcnet_exit);
