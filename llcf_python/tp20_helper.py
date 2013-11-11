import llcf
import llcf_helper
import bcm_helper
import struct

class tp20_helper:
    """ Class for diagnostic tests on DUT. Config options of DUT 
        are given to constructor
    """
    def __init__(self, dut, localTpId = 0x00, localRxId = 0x300, appProto = 0x01):
        #TP 2.0 consts
        self.LOCAL_TP_ID = localTpId
        self.REMOTE_TP_ID = dut.getRemoteTpId()
        self.LOCAL_TP_RX_ID = localRxId
        self.APPLICATION_PROTO = appProto
        # BCM support class
        self.bcmSup = bcm_helper.bcm_helper()     

    def setTPFilter(self, socket, device):
        """ Set filter in BCM for expected TP CHA message 
            from TP 2.0 server
        """
        print "Setting Filter for CHA reception"
        msg_head = llcf_helper.LLCFMsgHeader()
	msg_head.setOpcode(self.bcmSup.RX_SETUP)
	msg_head.setCanId(0x200 + self.REMOTE_TP_ID)
	msg_head.setFlags(self.bcmSup.RX_FILTER_ID)
        msg_head.setNFrames(0)

        msg_frame = llcf_helper.LLCFFrame()
        
        msgStruct = msg_head.getMsgHead() + msg_frame.getFrame()
        msglen = "%dB" % len(msgStruct)
        tupling = struct.unpack(msglen, msgStruct)
        res = socket.writeBCM(tupling, len(tupling), device)        
        return res
        
    def setTPStartup(self, socket, device):
        """ Create the Message tuple to set up TP channel (CHS message)
        """       
        print "Sending TP 2.0 CHS message to DUT"
        msg_head = llcf_helper.LLCFMsgHeader()
        msg_head.setOpcode(self.bcmSup.TX_SETUP)
        msg_head.setCanId(0x200 + self.LOCAL_TP_ID) # Tester static CAN ID
        msg_head.setFlags(self.bcmSup.SETTIMER | self.bcmSup.STARTTIMER | self.bcmSup.TX_COUNTEVT)
        msg_head.setNFrames(1)
        msg_head.setCount(10)
        msg_head.setIVal1Sec(0)
        msg_head.setIVal1USec(50000)
        msg_head.setIVal2Sec(0)
        msg_head.setIVal2USec(0)

        # Configure frame tuple
        frame = llcf_helper.LLCFFrame()
        frame.setCanId(0x200 + self.LOCAL_TP_ID)
        frame.setCanDlc(7)
        myData = [0x00]*8
        myData[0] = self.REMOTE_TP_ID
        myData[1] = 0xC0
        myData[2] = 0x00
        myData[3] = 0x10
        myData[4] = (self.LOCAL_TP_RX_ID & 0xFF)
        myData[5] = (self.LOCAL_TP_RX_ID>>8 & 0x07)
        myData[6] = self.APPLICATION_PROTO
        frame.setData(myData)
        
        msgStruct = msg_head.getMsgHead() + frame.getFrame()
        msglen = "%dB" % len(msgStruct)
        tupling = struct.unpack(msglen, msgStruct);   
        res = socket.writeBCM(tupling, len(tupling), device)      
        return res
        
    def openTPChannel(self, device = 'can0'):
        """ Open a TP channel to DeviceUnderTest
        """      
        try:
            bcm = llcf.SOCKET("PF_CAN", "SOCK_DGRAM", "CAN_BCM")            
            bcm.setblock(1)
            self.setTPFilter(bcm, device)
            self.setTPStartup(bcm, device)
            #Get Tx and Rx ID for TP channel
            res = bcm.read()    
        finally:
            bcm.close()        
        # if response is not empty
        tp = None
        if res is not -1:
            if res[0] == self.bcmSup.TX_EXPIRED:
                print "Received TX_EXPIRED"
            elif res[0] == self.bcmSup.RX_CHANGED:
                print "Received RX_CHANGED"  
                tp = llcf.SOCKET("PF_CAN", "SOCK_SEQPACKET", "CAN_TP20")
                tp.setblock(0)
                try:
                    help = llcf_helper.LLCFMsg()
                    txrx = help.getTxRx(res)
                    #print "TxRx tuple for CS = 0x%X,0x%X" % (txrx[0],txrx[1])
                    tp.connect(device, txrx)
                except:
                    print "Got exception during TP startup"
                    tp.close()
                    raise            
            else:
                print "Unknown response. Check script."
        return tp
        