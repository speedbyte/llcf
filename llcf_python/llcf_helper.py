import struct

class LLCFMsgHeader:
    """ Container class for LLCF struct msg_head.
        Creates and returns a tuple with all parameters
    """
    def __init__(self):
        self.opcode = 0x0000
        self.flags = 0x0000
        self.count = 0x0000
        self.iVal1Sec = 0x0000
        self.iVal1USec = 0x0000
        self.iVal2Sec = 0x0000
        self.iVal2USec = 0x0000
        self.canId = 0x0000
        self.nFrames = 0x0000
        
    def getMsgHead(self):
        bts = struct.pack('LLLLLLLLLxxxx',self.opcode,self.flags,self.count,self.iVal1Sec,
                   self.iVal1USec,self.iVal2Sec,self.iVal2USec,self.canId,self.nFrames)
        return bts

    def setOpcode(self,opcode):
        self.opcode = opcode
        
    def getOpcode(self):    
        return self.opcode

    def setCanId(self,canid):
        self.canId = canid
        
    def getCanId(self):    
        return self.canId

    def setFlags(self,flags):
        self.flags = flags
        
    def getFlags(self):    
        return self.flags

    def setNFrames(self, nframes):
        self.nFrames = nframes
        
    def getNFrames(self):    
        return self.nFrames

    def setCount(self,count):
        self.count = count
        
    def getCount(self):    
        return self.count

    def setIVal1Sec(self,sec):
        self.iVal1Sec = sec
        
    def getIVal1Sec(self):    
        return self.iVal1Sec    

    def setIVal1USec(self,usec):
        self.iVal1USec = usec
        
    def getIVal1USec(self):    
        return self.iVal1USec

    def setIVal2Sec(self,sec):
        self.iVal2Sec = sec
        
    def getIVal2Sec(self):    
        return self.iVal2Sec    

    def setIVal2USec(self,usec):
        self.iVal2USec = usec
        
    def getIVal2USec(self):    
        return self.iVal2USec


class LLCFFrame:
    """ Container class for LLCF struct can_frame.
        Creates and returns a tuple with all parameters
    """
    def __init__(self):
        self.canId = 0x0000
        self.canDlc = 0x0000
        self.data = [0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0]
       
    def getFrame(self):
        bts = struct.pack('LBxxxBBBBBBBB',self.canId,self.canDlc,self.data[0],self.data[1],
			  self.data[2],self.data[3],self.data[4],self.data[5],self.data[6],self.data[7])
        return bts

    def setCanId(self, canid):
        self.canId = canid
        
    def getCanId(self):    
        return self.canId

    def setCanDlc(self, candlc):
        self.canDlc = candlc
        
    def getCanDlc(self):    
        return self.canDlc

    def setData(self, data):
        for i in range(len(data)):
            self.data[i] = data[i]
        
    def getData(self):    
        return self.data
    
    
class LLCFMsg:
    """ Class for helper methods on LLCF/BCM message objects
    """
    def getTxRx(self, msg):
        rx = ((msg[51] & 0xF)<<8) + msg[50]
        tx = ((msg[53] & 0xF)<<8) + msg[52]
        return (tx,rx) 