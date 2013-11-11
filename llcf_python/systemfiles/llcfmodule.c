/*
 * llcfmodule.c
 *
 * Copyright (c) 2002-2005 Schefenacker Innovation GmBH, Germany
 * Licensed under GPL. Free to use, redistribute, modify with the 
 * condition that copyright information stays.
 *
 * Send feedback to <vikas.agrawal@web.de>
 
****************************************************************************
 * llcfmodule.c - Extension Module to LLCF drivers for using python APIs
 *
 * $Id: llcfmodule.c  2006-08-28 ( yy-mm-dd) agrawal $
****************************************************************************
 *
 * This is an extension module to call the LLCF APIS from Python
 *
 * $Log: llcfmodule.c,v $
 * Revision 1.0  2006/08/28 22:03:29  agrawal
 * Initial Revision
 *
*****************************************************************************/

#include "Python.h"

#include <netinet/in.h>
#include <sys/un.h>
#include "can.h"
#include "tp20.h"
#include "raw.h"
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "bcm.h"

typedef int SOCKET_T;
#ifndef SOCKETCLOSE
#define SOCKETCLOSE close
#endif

#define LOCAL_TP_ID  0
#define REMOTE_TP_ID 0x54
#define LOCAL_TP_RX_ID 0x300
#define LOCAL_TP_TX_ID 0x123
#undef ASSIGN_LOCAL_TP_TX_ID
#define APPLICATION_PROTO 0x01


/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */
static PyObject *PySocket_Error;
static PyObject *socket_timeout;
/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */
static PyObject *
PySocket_Err(void)
{
        return PyErr_SetFromErrno(PySocket_Error);
}

/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
        PyObject_HEAD
        SOCKET_T sock_fd;       /* Socket file descriptor */
        int sock_family;        /* Address family, e.g., PF_CAN */
        int sock_type;          /* Socket type, e.g., SOCK_STREAM */
        int sock_proto;         /* Protocol type, usually 0 */
        union sock_addr {
                struct sockaddr_can can;
        } sock_addr;
} PySocketSockObject;

 
/* A forward reference to the Socktype type object.
   The Socktype variable contains pointers to various functions,
   some of which call newsockobject(), which uses Socktype, so
   there has to be a circular reference. */
staticforward PyTypeObject PySocketSock_Type;

/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
PySocketSock_New(SOCKET_T fd, int family, int type, int proto)
{
        PySocketSockObject *s;
        PySocketSock_Type.ob_type = &PyType_Type;
        s = PyObject_New(PySocketSockObject, &PySocketSock_Type);
        if (s != NULL) {
                s->sock_fd = fd;
                s->sock_family = family;
                s->sock_type = type;
                s->sock_proto = proto;
        }
        return s;
}

/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

static int
getsockaddrarg(PySocketSockObject *s, struct sockaddr **addr_ret, int *len_ret)
{
        switch (s->sock_family) {

        case PF_CAN:
        {
                struct sockaddr_can* localaddr;         
                localaddr=(struct sockaddr_can*)&(s->sock_addr).can;
                *addr_ret = (struct sockaddr *) localaddr;
                *len_ret = sizeof *localaddr;
                return 1;
        }
        
        default:
                PyErr_SetString(PySocket_Error, "getsockaddrarg: bad family");
                return 0;

        }
}


/* s.bind(sockaddr) method */

static PyObject *
PySocketSock_bind(PySocketSockObject *s, PyObject *args)
{
        struct sockaddr *addr;
        struct sockaddr_can *canaddr;
        int addrlen;
        int res;
        if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;
        canaddr = (struct sockaddr_can *)addr;
        canaddr->can_family = AF_CAN;
        canaddr->can_ifindex = 0;
        Py_BEGIN_ALLOW_THREADS
        res = bind(s->sock_fd, (struct sockaddr *)canaddr, addrlen);
        Py_END_ALLOW_THREADS
        if (res < 0)
                return PySocket_Err();
        Py_INCREF(Py_None);
        return Py_None;
       
}

static char bind_doc[] =
"bind(address)\n\
\n\
Bind the socket to a local address.";


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
PySocketSock_close(PySocketSockObject *s, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":close"))
                return NULL;
        if (s->sock_fd != -1) {
                printf ("Closing socket %d\n",s->sock_fd);
                Py_BEGIN_ALLOW_THREADS
                (void) SOCKETCLOSE(s->sock_fd);  // #define SOCKETCLOSE close
                Py_END_ALLOW_THREADS
        }
        s->sock_fd = -1;
        Py_INCREF(Py_None);
        return Py_None;
}

static char close_doc[] =
"close()\n\
\n\
Close the socket.  It cannot be used after this call.";

/* s.configureStart_TP20(data, [flags,] sockaddr) method */

static PyObject *
PySocketSock_configureStart_TP20(PySocketSockObject *s, PyObject *args)
{
	struct {
		struct bcm_msg_head msg_head;
		struct can_frame frame;
	} msg;
	
	PyObject *Tuple1_msghead, *Tuple2_frame; // new reference
	char *dev;
        int dlc; 
        int i,res;
        int ifindex;
        struct ifreq ifr;
        struct sockaddr_can *canaddr;
        struct sockaddr *addr;
        int addrlen, n, flags;
	int length;
	unsigned char *jump;
        flags = 0;
	memset(&msg, 0xFF, sizeof(msg));
	
	if (!PyArg_ParseTuple(args, "OOs:configureStart_TP20", &Tuple1_msghead, &Tuple2_frame, &dev ))
        {
        	PyErr_SetString(PyExc_TypeError,"send invalid parameter" ); 
	        return NULL; 
        }
        if (!PyTuple_Check(Tuple1_msghead))
        {
                printf(" Object is Not a Tuple, please supply a tuple");
                return NULL;
        }
        if (!PyTuple_Check(Tuple2_frame))
        {
                printf(" Object is Not a Tuple, please supply a tuple");
                return NULL;
        }
	if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;    
	strcpy(ifr.ifr_name, dev);
        ioctl(s->sock_fd, SIOCGIFINDEX, &ifr);
        ifindex = ifr.ifr_ifindex;
	dlc = PyTuple_Size(Tuple1_msghead);
              
	canaddr = (struct sockaddr_can *)addr;
        canaddr->can_family = AF_CAN;
        canaddr->can_ifindex = ifindex;
	
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, (struct sockaddr *)canaddr, addrlen);
	Py_END_ALLOW_THREADS
	printf("connect return = %d\n", res);
	/* set filter for response from TP2.0 Server */
	msg.msg_head.opcode	 = RX_SETUP;
	msg.msg_head.can_id	 = 0x200 + 0x54; //REMOTE_TP_ID = 1;
	msg.msg_head.flags	 = RX_FILTER_ID; /* special case can have nframes = 0 */
	/* nframes and all the rest are set to zero due to memset() above */
	length = sizeof(msg);
	printf("AAAAAAAAAAAa %d\n", length);
	Py_BEGIN_ALLOW_THREADS
	n = write(s->sock_fd, &msg, sizeof(msg));
	Py_END_ALLOW_THREADS
        if (n < 0)
                return PySocket_Err();              
	/* fill CAN frame */
	msg.frame.can_id  = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 0));
	msg.frame.can_dlc = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 1));
	msg.frame.data[0] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 2));
	msg.frame.data[1] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 3));
	msg.frame.data[2] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 4));
	msg.frame.data[3] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 5));
	msg.frame.data[4] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 6));
	msg.frame.data[5] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 7));
	msg.frame.data[6] = PyInt_AsLong(PyTuple_GetItem(Tuple2_frame, 8));	
	
	msg.msg_head.opcode	  = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 0));
	msg.msg_head.can_id	  = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 1));
	msg.msg_head.flags	  = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 2));
	msg.msg_head.nframes      = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 3));
	msg.msg_head.count        = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 4)); 
	msg.msg_head.ival1.tv_sec = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 5));;
	msg.msg_head.ival1.tv_usec= PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 6));; /* 50 ms retry */
	msg.msg_head.ival2.tv_sec = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 7));;
	msg.msg_head.ival2.tv_usec= PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, 8));
	jump = (unsigned char *)&msg;
	for (i = 0; i < 56 ; i++){
		printf("%02x\n", *(jump+i));	
	}
	printf("--------------\n");	
	Py_BEGIN_ALLOW_THREADS
	n = write(s->sock_fd, &msg, sizeof(msg));
	Py_END_ALLOW_THREADS
        if (n < 0)
                return PySocket_Err();   
    /* read channel setup reply */
	/*Py_BEGIN_ALLOW_THREADS
	n = read(s->sock_fd, &msg, sizeof(msg));
	Py_END_ALLOW_THREADS
	
	len = n;
        if (n <= 0)
        {        //return PySocket_Err();
                return PyInt_FromLong((long)n); // a non blocking socket
        }
        else
        {
	        MessageObject_recv = PyList_New(len);
		for (i = 0; i < len ; i++)
        	        PyList_SetItem(MessageObject_recv, i, PyInt_FromLong(buffer[i]));	
		MessageObject_recv = Py_BuildValue("O", msg);
	}
	return MessageObject_recv; */
	return PyInt_FromLong((long)n); 
}

static char configureStart_TP20_doc[] =
"sendto(data[, flags], address)\n\
\n\
";

static PyObject *
PySocketSock_writeStream(PySocketSockObject *s, PyObject *args)
{
	struct {
		struct bcm_msg_head msg_head;
		struct can_frame frame;
	} msg;
	PyObject *Tuple1_msghead; // new reference
	char *dev;
	char buffer[4096];
        int i,res;
        int ifindex;
        struct ifreq ifr;
        struct sockaddr_can *canaddr;
        struct sockaddr *addr;
        int addrlen, n, flags;
	int length;
	unsigned char *jump;
        flags = 0;
	memset(&msg, 0xFF, sizeof(msg));
	
	if (!PyArg_ParseTuple(args, "iOs:writeStream", &Tuple1_msghead, &dev ))
        {
        	PyErr_SetString(PyExc_TypeError,"send invalid parameter" ); 
	        return NULL; 
        }

	if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;    
	strcpy(ifr.ifr_name, dev);
        ioctl(s->sock_fd, SIOCGIFINDEX, &ifr);
        ifindex = ifr.ifr_ifindex;
              
	canaddr = (struct sockaddr_can *)addr;
        canaddr->can_family = AF_CAN;
        canaddr->can_ifindex = ifindex;
	
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, (struct sockaddr *)canaddr, addrlen);
	Py_END_ALLOW_THREADS
	printf("connect return = %d\n", res);
	/* set filter for response from TP2.0 Server */
	msg.msg_head.opcode	 = RX_SETUP;
	msg.msg_head.can_id	 = 0x200 + 0x54; //REMOTE_TP_ID = 1;
	msg.msg_head.flags	 = RX_FILTER_ID; /* special case can have nframes = 0 */
	/* nframes and all the rest are set to zero due to memset() above */
	length = sizeof(msg);
	printf("AAAAAAAAAAAa %d\n", length);
	Py_BEGIN_ALLOW_THREADS
	n = write(s->sock_fd, &msg, sizeof(msg));
	Py_END_ALLOW_THREADS
        if (n < 0)
                return PySocket_Err();              
	for (i=0;i<56;i++)
		buffer[i]  = PyInt_AsLong(PyTuple_GetItem(Tuple1_msghead, i));
	jump = (unsigned char *)&msg;
	for (i = 0; i < 56 ; i++){
		printf("%02x\n", *(jump+i));	
	}
	printf("--------------\n");	
	Py_BEGIN_ALLOW_THREADS
	n = write(s->sock_fd, buffer, 56);
	Py_END_ALLOW_THREADS
        if (n < 0)
                return PySocket_Err();   

	return PyInt_FromLong((long)n); 
}

static char writeStream_doc[] =
"sendto(data[, flags], address)\n\
\n\
";

static PyObject *
PySocketSock_writeTupleBCM(PySocketSockObject *s, PyObject *args)
{
	PyObject *Tuple;
        int tlen;
	char *dev;
	char buffer[4096];
        int i,res;
        int ifindex;
        struct ifreq ifr;
        struct sockaddr_can *canaddr;
        struct sockaddr *addr;
        int addrlen, n = 0;   

        printf("Entering writeBCM\n");        
                
	if (!PyArg_ParseTuple(args, "Ois:writeTupleBCM", &Tuple, &tlen, &dev ))
        {
        	PyErr_SetString(PyExc_TypeError,"send invalid parameter" ); 
	        return NULL; 
        }

	if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;    
	strcpy(ifr.ifr_name, dev);
        ioctl(s->sock_fd, SIOCGIFINDEX, &ifr);
        ifindex = ifr.ifr_ifindex;
        
        printf("Configure canaddr structure\n");      
	canaddr = (struct sockaddr_can *)addr;
        canaddr->can_family = AF_CAN;
        canaddr->can_ifindex = ifindex;
	
        printf("Connecting BCM socket\n");
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, (struct sockaddr *)canaddr, addrlen);
	Py_END_ALLOW_THREADS

        printf("Converting Python tuple\n");
        if (tlen > 4096) tlen = 4096;
	for (i = 0; i < tlen; i++) {
		buffer[i]  = PyInt_AsLong(PyTuple_GetItem(Tuple, i));
                //printf("0x%02X ",buffer[i]);
        }
        printf("\n");
	//Pmy_BEGIN_ALLOW_THREADS
        n = write(s->sock_fd, buffer, tlen);
        //Pmy_END_ALLOW_THREADS
        if (n < 0)
                return PySocket_Err();   
	return PyInt_FromLong((long)n); 
}

static char writeTupleBCM_doc[] =
"writeTupleBCM(Tuple, Tlength, CanDevice)\n\
\n\
";

/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
PySocketSock_sendto(PySocketSockObject *s, PyObject *args)
{
        PyObject *MessageObject_send; // new reference
        char *dev;
        int id, dlc; 
        int i;
        int ifindex;
        struct can_frame frame;
        struct ifreq ifr;
        struct sockaddr_can *canaddr;
        struct sockaddr *addr;
        int addrlen, len, n, flags;
        flags = 0;
    
        if (!PyArg_ParseTuple(args, "iOs:sendto", &id, &MessageObject_send, &dev))
        {
        	PyErr_SetString(PyExc_TypeError,"send invalid parameter" ); 
	        return NULL; 
        }
        if (!PyTuple_Check(MessageObject_send))
        {
                printf(" Object is Not a Tuple, please supply a tuple");
                return NULL;
        }
        if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;
	dlc = PyTuple_Size(MessageObject_send);
	strcpy(ifr.ifr_name, dev);
        ioctl(s->sock_fd, SIOCGIFINDEX, &ifr);
        ifindex = ifr.ifr_ifindex;
            /* fill CAN frame */
	frame.can_id  = id;
        frame.can_dlc = dlc;
        for(i = 0; i < dlc ;i++)
               frame.data[i] = PyInt_AsLong(PyTuple_GetItem(MessageObject_send, i));
        len = sizeof(struct can_frame);
	canaddr = (struct sockaddr_can *)addr;
        canaddr->can_family = AF_CAN;
        canaddr->can_ifindex = ifindex;
        fflush(stdout);
        Py_BEGIN_ALLOW_THREADS
        n = sendto(s->sock_fd, &frame, len, flags, (struct sockaddr *)canaddr, addrlen);
        Py_END_ALLOW_THREADS
        if (n < 0)
                return PySocket_Err();
        fflush(stdout);
	return PyInt_FromLong((long)n);
}

static char sendto_doc[] =
"sendto(data[, flags], address)\n\
\n\
";

/* s.write(data [,flags]) method */

static PyObject *
PySocketSock_write(PySocketSockObject *s, PyObject *args)
{
	PyObject *MessageObject_send; // new reference
	char buf[40];
	char *ptrbuf = buf;
	int i;
	char *dev;
	int len, n=0, flags = 0, total = 0;
	if (!PyArg_ParseTuple(args, "Os:write", &MessageObject_send, &dev))
		return NULL;
	len = PyTuple_Size(MessageObject_send);
        for(i = 0; i < len ;i++)
               buf[i] = PyInt_AsLong(PyTuple_GetItem(MessageObject_send, i));
	
	//printf("Writing buffer Size= %d\n", len);
	fflush(stdout);
	//write(s->sock_fd, buf, len);
	do {
	        Py_BEGIN_ALLOW_THREADS		
		n = send(s->sock_fd, ptrbuf, len, flags);
                Py_END_ALLOW_THREADS
		if (n < 0)
			break;
		total += n;
		ptrbuf += n;
		len -= n;
	} while (len > 0);
	if (n < 0)
		return PySocket_Err();
	Py_INCREF(Py_None);
	return Py_None;
}

static char write_doc[] =
"write(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.";


static PyObject *
PySocketSock_recvfrom(PySocketSockObject *s, PyObject *args)
{
        PyObject *MessageObject_recv;
        int i;
        char *dev = "any";      // recieve from all sockets 
        struct can_frame frame;
        struct ifreq ifr;
        int ifindex;
        struct sockaddr_can *canaddr;
        struct sockaddr *addr;
        int addrlen,len, n, flags;
        flags = 0;

        if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;
        strcpy(ifr.ifr_name, dev);
        ioctl(s->sock_fd, SIOCGIFINDEX, &ifr);
        ifindex = ifr.ifr_ifindex;
        len     = sizeof(struct can_frame);
        canaddr = (struct sockaddr_can *)addr;
        canaddr->can_family = AF_CAN;
        canaddr->can_ifindex = ifindex;
        Py_BEGIN_ALLOW_THREADS       
                n = recvfrom(s->sock_fd, &frame, len, flags, (struct sockaddr *)canaddr, &addrlen);
        Py_END_ALLOW_THREADS
        if (n <= 0)
        {        //return PySocket_Err();
                return PyInt_FromLong((long)n); // a non blocking socket
        }
        else
        {
                ifr.ifr_ifindex = canaddr->can_ifindex;
                ioctl(s->sock_fd, SIOCGIFNAME, &ifr);
                MessageObject_recv = PyList_New(frame.can_dlc);
                for (i = 0; i < frame.can_dlc; i++)
                        PyList_SetItem(MessageObject_recv, i, PyInt_FromLong(frame.data[i]));
                MessageObject_recv = Py_BuildValue("iisO", frame.can_id, frame.can_dlc, ifr.ifr_name, MessageObject_recv);           
                return MessageObject_recv;
        }
}

static char recvfrom_doc[] =
"recvfrom(data[, flags], address)\n\
\n\
";              

/* s.recv(nbytes [,flags]) method */

static PyObject *
PySocketSock_read(PySocketSockObject *s, PyObject *args)
{
	PyObject *MessageObject_recv;
	char *dev = "any";
	int len, n, i, flags = 0, filedesc;
	unsigned char buffer[4096];

	Py_BEGIN_ALLOW_THREADS
 	n = read(s->sock_fd, buffer, 4096);
	//n = read(s->sock_fd, &msg, sizeof(msg));
	Py_END_ALLOW_THREADS
	len = n;
        if (n <= 0)
        {        //return PySocket_Err();
                return PyInt_FromLong((long)n); // a non blocking socket
        }
        else
        {
	        MessageObject_recv = PyList_New(len);
		for (i = 0; i < len ; i++){
			//printf("%02x\n", buffer[i]);
        	        PyList_SetItem(MessageObject_recv, i, PyInt_FromLong(buffer[i]));	}
		MessageObject_recv = Py_BuildValue("O", MessageObject_recv);
	} 
	return MessageObject_recv;
}

static char read_doc[] =
"read(buffersize[, flags]) -> data\n\
\n\
Receive up to buffersize bytes from the socket.  For the optional flags\n\
argument, see the Unix manual.  When no data is available, block until\n\
at least one byte is available or until the remote end is closed.  When\n\
the remote end is closed and all data is read, return the empty string.";


/* s.setblocking(1 | 0) method */

static PyObject *
PySocketSock_setblocking(PySocketSockObject *s, PyObject *arg)
{
	int block;
	int delay_flag;
        
	block = PyInt_AsLong(arg);
	if (block == -1 && PyErr_Occurred())
		return NULL;
	//Pmy_BEGIN_ALLOW_THREADS
	delay_flag = fcntl(s->sock_fd, F_GETFL, 0);
	//Pmy_END_ALLOW_THREADS        
	if (block)
		delay_flag &= (~O_NDELAY);
	else
		delay_flag |= O_NDELAY;
	//Pmy_BEGIN_ALLOW_THREADS	
        fcntl (s->sock_fd, F_SETFL, delay_flag);
	//Pmy_END_ALLOW_THREADS

	Py_INCREF(Py_None);     
	return Py_None;
}

static char setblocking_doc[] =
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
This uses the FIONBIO ioctl with the O_NDELAY flag.";


/* s.connect(sockaddr) method */

static PyObject *
PySocketSock_connect(PySocketSockObject *s, PyObject *args)
{
        PyObject *MessageObject_send;
	struct sockaddr *addr;
        struct sockaddr_can *canaddr;
	struct ifreq ifr;
	int addrlen;
        int res;
	char *dev;
         
        if (!PyArg_ParseTuple(args, "Os:connect", &MessageObject_send, &dev))
        {
        	PyErr_SetString(PyExc_TypeError,"send invalid parameter" ); 
	        return NULL; 
        }
        //printf ( "device name %s\n", dev );
	if (!getsockaddrarg(s, &addr, &addrlen))
                return NULL;
    	canaddr = (struct sockaddr_can *)addr;
	strcpy(ifr.ifr_name, dev);
    	ioctl(s->sock_fd, SIOCGIFINDEX, &ifr);		
        canaddr->can_family = AF_CAN;
   	canaddr->can_ifindex = ifr.ifr_ifindex;
    	canaddr->can_addr.tp20.tx_id  = PyInt_AsLong(PyTuple_GetItem(MessageObject_send, 0));
    	canaddr->can_addr.tp20.rx_id  = PyInt_AsLong(PyTuple_GetItem(MessageObject_send, 1));	
	fflush(stdout);
	Py_BEGIN_ALLOW_THREADS
	res = connect(s->sock_fd, (struct sockaddr *)canaddr, addrlen);
	Py_END_ALLOW_THREADS
        
	return PyInt_FromLong((long) res);
}

static char connect_doc[] =
"connect(address)\n\
\n\
Connect the socket to a remote address.";



/* List of methods for socket objects */

static PyMethodDef PySocketSock_methods[] = {
	{"read",		(PyCFunction)PySocketSock_read,  METH_VARARGS,
				read_doc},
	{"write",		(PyCFunction)PySocketSock_write, METH_VARARGS,
				write_doc},	
	{"connect",	  	(PyCFunction)PySocketSock_connect, METH_VARARGS,
			  	connect_doc},				
        {"bind",                (PyCFunction)PySocketSock_bind, METH_VARARGS,
                                bind_doc},
        {"close",               (PyCFunction)PySocketSock_close, METH_VARARGS,
                                close_doc},
        {"sendto",              (PyCFunction)PySocketSock_sendto, METH_VARARGS,
                                sendto_doc}, 
        {"configureStart_TP20", (PyCFunction)PySocketSock_configureStart_TP20, METH_VARARGS,
                                configureStart_TP20_doc},    
        {"recvfrom",            (PyCFunction)PySocketSock_recvfrom, METH_VARARGS,
                                recvfrom_doc},
	{"setblocking",		(PyCFunction)PySocketSock_setblocking, METH_O,
				setblocking_doc},	
        {"writeStream", 	(PyCFunction)PySocketSock_writeStream, METH_VARARGS,
                                writeStream_doc},	
        {"writeTupleBCM", 	(PyCFunction)PySocketSock_writeTupleBCM, METH_VARARGS,
                                writeTupleBCM_doc},	
        {NULL,                  NULL}           /* sentinel */
};


/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
PySocketSock_dealloc(PySocketSockObject *s)
{
        if (s->sock_fd != -1)
                (void) SOCKETCLOSE(s->sock_fd);
        PyObject_Del(s);
}


/* Return a socket object's named attribute. */

static PyObject *
PySocketSock_getattr(PySocketSockObject *s, char *name)
{
        return Py_FindMethod(PySocketSock_methods, (PyObject *) s, name);
}


static PyObject *
PySocketSock_repr(PySocketSockObject *s)
{
        char buf[512];
        sprintf(buf, 
                "<socket object, fd=%ld, family=%d, type=%d, protocol=%d>", 
                (long)s->sock_fd, s->sock_family, s->sock_type, s->sock_proto);
        return PyString_FromString(buf);
}


/* Type object for socket objects. */

static PyTypeObject PySocketSock_Type = {
        PyObject_HEAD_INIT(0)   /* Must fill in type value later */
        0,
        "socket",
        sizeof(PySocketSockObject),
        0,
        (destructor)PySocketSock_dealloc, /*tp_dealloc*/
        0,              /*tp_print*/
        (getattrfunc)PySocketSock_getattr, /*tp_getattr*/
        0,              /*tp_setattr*/
        0,              /*tp_compare*/
        (reprfunc)PySocketSock_repr, /*tp_repr*/
        0,              /*tp_as_number*/
        0,              /*tp_as_sequence*/
        0,              /*tp_as_mapping*/
};


/* Python interface to socket(family, type, proto).
   The third (protocol) argument is optional.
   Return a new socket object. */

/*ARGSUSED*/
static PyObject *
PySocket_socket(PyObject *self, PyObject *args)
{
        PySocketSockObject *s;
        SOCKET_T fd;
        int family, type, proto = 0;
        if (!PyArg_ParseTuple(args, "ii|i:socket", &family, &type, &proto))
                return NULL;
      
	Py_BEGIN_ALLOW_THREADS
        fd = socket(family, type, proto);
        //fcntl(fd, F_SETFL, O_NONBLOCK);  // make a non blocking socket for continous polling
        Py_END_ALLOW_THREADS
	if (fd < 0){
		printf("bad fd");
                return PySocket_Err();}
        
	s = PySocketSock_New(fd, family, type, proto);
        /* If the object can't be created, don't forget to close the
           file descriptor again! */
        if (s == NULL){
		printf("bad new S");
                (void) SOCKETCLOSE(fd);}
        /* From now on, ignore SIGPIPE and let the error checking
           do the work. */
#ifdef SIGPIPE      
        (void) signal(SIGPIPE, SIG_IGN);
#endif   
	return (PyObject *) s;
}

static char socket_doc[] =
"socket(family, type[, proto]) -> socket object\n\
\n\
Open a socket of the given type.  \n\
family          type                                            proto  \n\
PF_CAN          SOCK_DGRAM, SOCK_RAW, SOCK_SEQ_PACKET           CAN_BCM, CAN_RAW, CAN_MCNET, CAN_TP16, CAN_TP20\n\
\n\
";
          



/* List of functions exported by this module. */

static PyMethodDef PySocket_methods[] = {

        {"socket",              PySocket_socket, 
         METH_VARARGS, socket_doc},    
        {NULL,                  NULL}            /* Sentinel */
};


/* Initialize this module.
 *   This is called when the first 'import _llcf' is done,
 *
 */

static char module_doc[] =
"Implementation module for socket operations.  See the socket module\n\
for documentation.";

static char sockettype_doc[] =
"A socket represents one endpoint of a network connection.\n\
\n\
Methods:\n\
\n\
bind() -- bind the socket to a local address\n\
close() -- close the socket\n\
sendto() -- send data to a given address\n\
recvfrom() - recv datas from all sockets\n\
";

DL_EXPORT(void)
init_llcf(void)
{
        PyObject *m, *d;

        m = Py_InitModule3("_llcf", PySocket_methods, module_doc);
        d = PyModule_GetDict(m);	
        PySocket_Error = PyErr_NewException("socket.error", NULL, NULL);
        if (PySocket_Error == NULL)
                return;
        
        PyDict_SetItemString(d, "error", PySocket_Error);
        PySocketSock_Type.ob_type = &PyType_Type;
        PySocketSock_Type.tp_doc = sockettype_doc;
        Py_INCREF(&PySocketSock_Type);
        if (PyDict_SetItemString(d, "SocketType",
                                 (PyObject *)&PySocketSock_Type) != 0)
                return;
}

/*
METH_O
    Methods with a single object argument can be listed with the METH_O flag, instead of invoking PyArg_ParseTuple() with a "O" argument. They have the type PyCFunction, with the self parameter, and a PyObject* parameter representing the single argument.  */
    
/*
Various sending and receiving implemented in socketmodule.c - 
in llcfmodule.c = sendto, recvfrom, sendall ( write ) , recv ( read )
recv(buflen[, flags]) -- receive data\n\
recv_into(buffer[, nbytes[, flags]]) -- receive data (into a buffer)\n\
recvfrom(buflen[, flags]) -- receive data and sender\'s address\n\
recvfrom_into(buffer[, nbytes, [, flags])\n\
  -- receive data and sender\'s address (into a buffer)\n\
sendall(data[, flags]) -- send all data\n\
send(data[, flags]) -- send data, may not send all of it\n\
sendto(data[, flags], addr) -- send data to a given address\n\

*/
