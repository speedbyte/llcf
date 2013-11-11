"""python script to create a EM_check

#  *
#  * client-tp20.py
#  *
#  * Copyright (c) 2002-2005 Schefenacker Innovation GmBH, Germany
#  * All rights reserved.
#  *
#  *
#  * Send feedback to <vikas.agrawal@web.de>
#  
# ****************************************************************************
#  * llcf.py - Python script to ping pong messages
#  *
#  * $Id: client-tp20.py  2006-08-28 ( yy-mm-dd) agrawal $
# ****************************************************************************
#  *
#  * Python script for ping pong messages
#  * $Log: client-tp20.py,v $
#  * Revision 1.0  2006/08/28 22:03:29  agrawal
#  * Initial Revision
#  *
# *****************************************************************************
"""

try:
        reload(llcf)
except:
        try:
                print('llcf reload unssucces')
                import llcf
        except:
                print " llcf not found at all "
try:
        reload(time)
except:
        try:
                print('time reload unssucces')
                import time
        except:
                print " llcf not found at all "

# raw = llcf.SOCKET("PF_CAN",  "SOCK_RAW", "CAN_RAW")              
# frame = llcf.FRAME(0x200, (0x54, 0xc0, 0x00, 0x10, 0x00, 0x03, 0x01 ))
# raw.send(frame, "can0")
raw = llcf.SOCKET("PF_CAN", "SOCK_RAW", "CAN_RAW")              
frame = llcf.FRAME(0x200, (0x54, 0xc0, 0x00, 0x10, 0x00, 0x03, 0x01 ))
raw.send(frame, "can0")

time.sleep(0.015)
raw.close()

a  = llcf.SOCKET()
p  = llcf.PAYLOAD()
try:
	a.connect("can0",(0x4ec,0x300))
except:
	a.close
else:
	a.write1(p, "can0")	
	print "closing"
	a.close()
           
                        
                               
