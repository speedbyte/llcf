"""python script to create a CANDUMP

#  *
#  * candump.py
#  *
#  * Copyright (c) 2002-2005 Schefenacker Innovation GmBH, Germany
#  * All rights reserved.
#  *
#  *
#  * Send feedback to <vikas.agrawal@web.de>
#  
# ****************************************************************************
#  * llcf.py - Python script to create and close socket, send and recieve messages
#  *
#  * $Id: candump.py  2006-08-28 ( yy-mm-dd) agrawal $
# ****************************************************************************
#  *
#  * Python script to create and close socket, send and recieve messages
#  * $Log: candump.py,v $
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
                print " time not found at all "

file1 = open('LogFiles/dump.txt', 'w')
socket  = llcf.SOCKET()
try:
        while 1:               
                        messagesonbus = socket.show()
                        if(messagesonbus > 0):
                                dlc2 = len(messagesonbus[3])
                                b = []; y = []
                                y = list(messagesonbus)
                                y[0] = hex(messagesonbus[0])[2:]
                                y[3] = (map(hex, messagesonbus[3]))  
                                y[3] = [ element[ 2: ] for element in y[3] ]
                                total = y
                                for num in range(0,4):
                                        b.append(str(total[num])) 
                                        file1.write(b[num])
                                        file1.write('\t')
                                file1.write('\n')
                                print total
                        else:
                                pass


except KeyboardInterrupt:
        socket.close()   
        file1.close()                              