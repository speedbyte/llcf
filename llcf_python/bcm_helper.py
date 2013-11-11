import struct
import llcf_helper

class bcm_helper:
    """ Support functions for BCM sockets and tests like sending of Kl.15
    """
    def __init__(self):
        # BCM enum
        self.NO_OP = 0
        self.TX_SETUP = 1
        self.TX_DELETE = 2
        self.TX_READ = 3
        self.TX_SEND = 4
        self.RX_SETUP = 5
        self.RX_DELETE = 6
        self.RX_READ = 7
        self.TX_STATUS = 8
        self.TX_EXPIRED = 9
        self.RX_STATUS = 10
        self.RX_TIMEOUT = 11
        self.RX_CHANGED = 12

        # BCM Flags
        self.SETTIMER = 0x0001
        self.STARTTIMER = 0x0002
        self.TX_COUNTEVT = 0x0004
        self.TX_ANNOUNCE = 0x0008
        self.TX_CP_CAN_ID = 0x0010
        self.RX_FILTER_ID = 0x0020
        self.RX_CHECK_DLC = 0x0040
        self.RX_NO_AUTOTIMER = 0x0080
        self.RX_ANNOUNCE_RESUME = 0x0100
        self.TX_RESET_MULTI_IDX = 0x0200
        self.RX_RTR_FRAME = 0x0400
        self.CMD_ERROR = 0x8000   
    
    def startKl15(self, socket, device = 'can0'):
        """
        """
        # mBSG3 message structure
        mbsg3_head = llcf_helper.LLCFMsgHeader()        
        mbsg3_head.setOpcode(self.TX_SETUP)
        mbsg3_head.setCanId(0x575) # mBSG_3 message
        mbsg3_head.setFlags(self.SETTIMER | self.STARTTIMER | self.TX_COUNTEVT)
        mbsg3_head.setNFrames(1)
        mbsg3_head.setCount(0)
        mbsg3_head.setIVal1Sec(0)
        mbsg3_head.setIVal1USec(0)
        mbsg3_head.setIVal2Sec(0)
        mbsg3_head.setIVal2USec(200000)

        # Configure mBSG3 frame tuple
        mbsg3_frame = llcf_helper.LLCFFrame()
        mbsg3_frame.setCanId(0x575)
        mbsg3_frame.setCanDlc(4)
        myData = [0x00]*8
        myData[0] = 0x03
        myData[1] = 0x00
        myData[2] = 0x00
        myData[3] = 0x00
        mbsg3_frame.setData(myData)

        # mZAS_Status message structure
        mzas_head = llcf_helper.LLCFMsgHeader()        
        mzas_head.setOpcode(self.TX_SETUP)
        mzas_head.setCanId(0x2c3) # mZAS_Status message
        mzas_head.setFlags(self.SETTIMER | self.STARTTIMER | self.TX_COUNTEVT)
        mzas_head.setNFrames(1)
        mzas_head.setCount(0)
        mzas_head.setIVal1Sec(0)
        mzas_head.setIVal1USec(0)
        mzas_head.setIVal2Sec(0)
        mzas_head.setIVal2USec(100000)

        # Configure mZAS_Status frame tuple
        mzas_frame = llcf_helper.LLCFFrame()
        mzas_frame.setCanId(0x2c3)
        mzas_frame.setCanDlc(1)
        myData = [0x00]*8
        myData[0] = 0x03
        mzas_frame.setData(myData)
                
        # Send mBSG3 message to BCM        
        msgStruct = mbsg3_head.getMsgHead() + mbsg3_frame.getFrame()
        msglen = "%dB" % len(msgStruct)
        tupling = struct.unpack(msglen, msgStruct);   
        res = socket.writeBCM(tupling, len(tupling), device)

        # Send mZAS_Status message to BCM        
        msgStruct = mzas_head.getMsgHead() + mzas_frame.getFrame()
        msglen = "%dB" % len(msgStruct)
        tupling = struct.unpack(msglen, msgStruct);   
        res = socket.writeBCM(tupling, len(tupling), device)                