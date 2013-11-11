"""Use the module functions to call the kernel level function calls."""

#  *
#  * llcf.py
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
#  * $Id: llcf.py  2006-08-28 ( yy-mm-dd) agrawal $
# ****************************************************************************
#  *
#  * Python script to create and close socket, send and recieve messages
#  * $Log: llcf.py,v $
#  * Revision 1.0  2006/08/28 22:03:29  agrawal
#  * Initial Revision
#  *
# *****************************************************************************

import _llcf

family_dict = {
        "PF_CAN":30
        }

proto_dict = { 
        "CAN_TP20" :2, "CAN_RAW":3 ,"CAN_TP16" :1,"CAN_BCM" :4,"CAN_MCNET" :5,"CAN_ISOTP" :6,"CAN_BAP" :7,"CAN_MAX" :8
        }
        
type_dict = {
        "SOCK_SEQPACKET" :5, "SOCK_RAW" :3,"SOCK_DGRAM" :2 
        }


class FRAME:
        """FRAME Class, the default parameters are <<< 0x11, (0x22,0x33) >>>     """
        def __init__(self,id=0x11,data=(0x22,0x33)):
                """
		It checks for the data if the user has correctly entered ( id and the datas ) 
                """                          
      		self.id = id
      		self.data = data
      		if (self.id<1) or (self.id>2**11):
         		raise ValueError("Id needs to be between 1 and 2^11. Try Again")
         
         	datalen = len(self.data)
         	if (datalen<1) or (datalen > 8):
            		raise ValueError("Data Length needs to be between 0 and 8, try again")
            
         	if not(filter(lambda x: x<0 or x>255, self.data) == ()):                                   
            		raise ValueError('check the data should be in the range of 1 byte. try again')                
                                 

        def __del__(self):
                        pass
			#print   'Closing a FRAME instance'

class PAYLOAD:
        """FRAME Class, the default parameters are <<< 0x11, (0x22,0x33) >>>     """
        def __init__(self,payload=(0x02,0x03,0x12,0x13,0x22,0x23,0x32,0x33,0x42,0x43,0x52,0x53,0x62,0x63,0x72,0x73)):
                """
		It checks for the data if the user has correctly entered ( id and the datas ) 
                """                          
      		self.payload = payload
                                

        def __del__(self):
			#print   'Closing a FRAME instance'
                        pass


class SOCKET:
        """SOCKET class, the default parameters are <<< "PF_CAN", "CAN_RAW","SOCK_RAW" >>>"""
        def __init__(self, family = "PF_CAN", type = "SOCK_SEQPACKET", proto = "CAN_TP20"):
                """
                
                """
                self.family = family
                self.proto = proto
                self.type = type
                
		try:
                        self.newsocket = _llcf.socket(family_dict[self.family], type_dict[self.type], proto_dict[self.proto] )
                        
# The bind() system call assigns an address to an unbound socket. When a socket is created with socket(), it exists in an address space (address family) but has no address assigned. bind() causes the socket whose descriptor is s to become bound to the address specified in the socket address structure pointed to by addr.
		except:
			raise KeyError  ('Only used can be \'PF_CAN\', \'CAN_RAW\', \'SOCK_RAW\'  OR \nDo not enter any parameters' )
                else:
                        pass
                        #print '%s bound successfully to all interfaces...' %self.newsocket                        


        def bind(self):
		"""
		"""
		self.newsocket.bind()               
		
        def setblock(self, block=0):
		"""
		"""
		self.newsocket.setblocking(block)  	
	
	def send(self, FRAME, device="vcan2"):
                """
        
                """	
		self.device=device
                self.newsocket.sendto(FRAME.id, FRAME.data, device)		      
        
	def startupTP20(self,tuple1, tuple2, device):
                """

                """
                x1 = self.newsocket.configureStart_TP20(tuple1, tuple2, device)
                return x1      
	
	def streamwrite(self,stringy, device):
                """

                """
                x1 = self.newsocket.writeStream(stringy, device)
                return x1           
	
        def writeBCM(self, tuppy, tuplen, device = 'can0'):
                """

                """
                x1 = self.newsocket.writeTupleBCM(tuppy, tuplen, device)
                return x1
        
	def recv(self):
                """

                """
                x1 = self.newsocket.recvfrom()
                return x1
                
        def write1(self, PAYLOAD, device="vcan2"):
                """
                """	
                self.newsocket.write(PAYLOAD.payload,  device)	
     
        def read(self):
                """

                """
                buffer1 = self.newsocket.read()
                return buffer1
    
        def listen(self):
		"""
		
		"""
		self.newsocket.listen(5)
		
	def accept(self):
		"""
		"""
		newsock, addr = self.newsocket.accept()
		return SOCKET(), addr
				
	def connect(self, device = "vcan2", tupleconn = (0,0)):
		"""
		"""
		self.device = device
		self.newsocket.connect(tupleconn, device)
# the 2nd parameter is the struct sockaddr containing the destination port and IP address, in our case there is no destport and IP address.		
        def show(self):
                """
                """
                x1 = self.newsocket.recvfrom()
                return x1
                        	
	def close(self):
                """

                """
                self.newsocket.close()
                #print self.newsocket

        def __del__(self):
                try:
                        self.newsocket.close()   
                        #print ' Closed a socket'                     
                except:
                        print ' Nothing to close'
			raise	