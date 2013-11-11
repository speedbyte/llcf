"""python script to create a EM_check

#  *
#  * server-tp20.py
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
#  * $Id: server-tp20.py  2006-08-28 ( yy-mm-dd) agrawal $
# ****************************************************************************
#  *
#  * Python script for ping pong messages
#  * $Log: server-tp20.py,v $
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

              
try:
	a  = llcf.SOCKET()
except:
#a.listen()
#newid = a.accept()
## Upon successful completion, accept() returns a nonnegative integer which is a descriptor for the accepted socket.
#readbuf = a.read(newid[2])             # the accept returns the file descriptor of the new socket.
#print readbuf
#newstring = readbuf + 'with love from server'
#a.write(newid[2], newstring,'can0')
#time.sleep(1)
	print 'exception reached'
	a.close()
# try:
# 	a.bind()
# 	print "binding"
# except:
# 	print "bind exceptin reached"
# 	a.close()
#newid[2].close()
try:
	a.connect()
	print "connecting"
except:
	print "connect exceptin reached"
	a.close()
a.write()
print a.read()
print "closing"
a.close()
                        
                               
