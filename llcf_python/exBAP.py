import llcf
import llcf_helper
import time
# BCM enum
NO_OP = 0
TX_SETUP = 1
TX_DELETE = 2
TX_READ = 3
TX_SEND = 4
RX_SETUP = 5
RX_DELETE = 6
RX_READ = 7
TX_STATUS = 8
TX_EXPIRED = 9
RX_STATUS = 10
RX_TIMEOUT = 11
RX_CHANGED = 12

# BCM Flags
SETTIMER = 0x0001
STARTTIMER = 0x0002
TX_COUNTEVT = 0x0004
TX_ANNOUNCE = 0x0008
TX_CP_CAN_ID = 0x0010
RX_FILTER_ID = 0x0020
RX_CHECK_DLC = 0x0040
RX_NO_AUTOTIMER = 0x0080
RX_ANNOUNCE_RESUME = 0x0100
TX_RESET_MULTI_IDX = 0x0200
RX_RTR_FRAME = 0x0400
CMD_ERROR = 0x8000


# Configure msg_head tuple
msg_head = llcf_helper.LLCFMsgHeader()
msg_head.opcode = TX_SETUP
msg_head.canId = 0x200 + LOCAL_TP_ID # Tester static CAN ID
msg_head.flags = SETTIMER | STARTTIMER | TX_COUNTEVT
msg_head.nFrames = 1
msg_head.count = 5
msg_head.iVal1Sec = 0
msg_head.iVal1USec = 50000
msg_head.iVal2Sec = 0
msg_head.iVal2USec = 0


# Configure frame tuple
frame = llcf_helper.LLCFFrame()
frame.canId = 0x200 + LOCAL_TP_ID
frame.canDlc = 7
frame.data = [REMOTE_TP_ID, 0xC0, 0x00, 0x10, (LOCAL_TP_RX_ID & 0xFF),
              (LOCAL_TP_RX_ID>>8 & 0x07), APPLICATION_PROTO]

print "Header = ", msg_head.getMsgHead()
print "Frame  = ", frame.getFrame()
#Create BCM socket and start TP Dynamic Channel Setup
bcm = llcf.SOCKET("PF_CAN", "SOCK_DGRAM", "CAN_BCM")
bcm.setblock(1)
try :
	res = bcm.startupTP20(tuple(msg_head.getMsgHead()), tuple(frame.getFrame()), "can0" )
	#time.sleep(0.015)
	res2 = bcm.read()
finally:
	bcm.close()
print "written character = ", res
print "read character = ", res2

if res2 == 0:
    print "No TX or RX id received"
else:
    tx_id = 0x4EC #res[0]
    rx_id = 0x300 #res[1]
    tp = llcf.SOCKET("PF_CAN", "SOCK_SEQPACKET", "CAN_TP20")
    tp.setblock(1)
    try:
        tp.connect("can0", (tx_id,rx_id))
    except:
        print "Got exception during TP startup"
        tp.close()
        raise
    p = llcf.PAYLOAD()
    p = llcf.PAYLOAD((0x00, 0x02, 0x21, 0x01))    
    # Write some test data over TP 2.0
    tp.write1(p,"can0")
    answer = tp.read()
#     if answer[0] == 0xb3
#     	print " acknolwdged"
    
    #recvdData = tp.read()
    tp.close()