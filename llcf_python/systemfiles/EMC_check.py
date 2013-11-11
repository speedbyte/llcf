"""python script to create a EM_check

#  *
#  * EMC_check.py
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
#  * $Id: EMC_check.py  2006-08-28 ( yy-mm-dd) agrawal $
# ****************************************************************************
#  *
#  * Python script for ping pong messages
#  * $Log: EM_check.py,v $
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

file1 = open('LogFiles/error_log.txt', 'w') 
               
a  = llcf.SOCKET()
f  = llcf.FRAME(0x431, (0x15, 0x01,0x00, 0x00,0x00, 0x00,0x00, 0x00))
f2 = llcf.FRAME(0x431, (0x11, 0x02,0x00, 0x00,0x00, 0x00,0x00, 0x00))

def send_raw():
                a.send(f2,'can0')
                try:
                        while 1:
                                a.send(f, 'can0')
                                for x in range(10):     
                                         if x == 9:
                                                print 'ERROR...'                                        
                                                file1.write("Error")
                                                file1.write('\n')
                                                file1.write(str(time.time()))
                                         time.sleep(0.01)
                                         recv_frame = a.recv()
                                         print recv_frame
                                         if recv_frame == -1:
                                                 continue
                                         elif recv_frame[0] == 0x435:
                                                 break                                      
                                time.sleep(0.05 )                                                                      
                
                except KeyboardInterrupt:
                        print ' Interrupting sending signals'
                        a.close()
                        file1.close()
                        
                               
