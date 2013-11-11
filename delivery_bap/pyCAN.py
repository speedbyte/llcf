""" 


    $Date: 2006-08-27 18:20:08 +0200 (So, 27 Aug 2006) $
    $Rev: 851 $
    $Author: henkelma $
    $URL: svn://voice.s1nn.int/pyCAN/trunk/pyCAN.py $

    Copyright (c) 2006 S1nn GmbH & Co. KG.
    All Rights Reserved.

"""

_svn_id = "$Id: pyCAN.py 851 2006-08-27 16:20:08Z henkelma $"
_svn_rev = "$Rev: 851 $"
__version__ = _svn_rev[6:-2]


import ctypes


# Constants definitions - Frame Type
#
CAN_INIT_TYPE_EX = 0x01	# Extended Frame
CAN_INIT_TYPE_ST = 0x00	# Standart Frame

# Constants definitions - ID
#
CAN_MAX_STANDARD_ID = 0x7ff
CAN_MAX_EXTENDED_ID = 0x1fffffff

# Constants definitions  - CAN message types
#
MSGTYPE_STANDARD = 0x00   # Standard Data frame (11-bit ID)
MSGTYPE_RTR = 0x01        # 1, if Remote Request frame
MSGTYPE_EXTENDED = 0x02   # 1, if Extended Data frame (CAN 2.0B, 29-bit ID)
MSGTYPE_STATUS = 0x80     # 1, if Status information
	
# Baud rate codes = BTR0/BTR1 register values for the CAN controller.
# You can define your own Baudrate with the BTROBTR1 register !!
# take a look at www.peak-system.com for our software BAUDTOOL to
# calculate the BTROBTR1 register for every baudrate and sample point.
#
class CAN_BAUD(object):
    """Container class for CAN BAUD rates"""
CAN_BAUD._1M = 0x0014  #   1 MBit/s
CAN_BAUD._500K = 0x001C  # 500 kBit/s
CAN_BAUD._250K = 0x011C  # 250 kBit/s
CAN_BAUD._125K = 0x031C  # 125 kBit/s
CAN_BAUD._100K = 0x432F  # 100 kBit/s
#CAN_BAUD._100K = 0x4914  # 100 kBit/s
CAN_BAUD._50K = 0x472F  #  50 kBit/s
CAN_BAUD._20K = 0x532F  #  20 kBit/s
CAN_BAUD._10K = 0x672F  #  10 kBit/s
CAN_BAUD._5K = 0x7F7F  #   5 kBit/s

# Error codes (bit code)
#
class CAN_ERR(object):
    """Container class for CAN Error codes"""
CAN_ERR.OK = 0x0000  # No error
CAN_ERR.XMTFULL = 0x0001  # Transmit buffer in CAN controller is full
CAN_ERR.OVERRUN = 0x0002  # CAN controller was read too late
CAN_ERR.BUSLIGHT = 0x0004  # Bus error: an error counter reached the 'light' limit
CAN_ERR.BUSHEAVY = 0x0008  # Bus error: an error counter reached the 'heavy' limit
CAN_ERR.BUSOFF = 0x0010  # Bus error: the CAN controller is in bus-off state
CAN_ERR.QRCVEMPTY = 0x0020  # Receive queue is empty
CAN_ERR.QOVERRUN = 0x0040  # Receive queue was read too late
CAN_ERR.QXMTFULL = 0x0080  # Transmit queue is full
CAN_ERR.REGTEST = 0x0100  # Test of the CAN controller hardware registers failed (no hardware found)
CAN_ERR.NOVXD = 0x0200  # Driver not loaded
CAN_ERR.HWINUSE = 0x0400  # The Hardware is alraedy occuped by a Net
CAN_ERR.NETINUSE = 0x0800  # A Client is already connected to the Net
CAN_ERR.ILLHW = 0x1400  # Hardware handle is invalid
CAN_ERR.ILLNET = 0x1800  # Net handle is invalid
CAN_ERR.ILLCLIENT = 0x1C00  # Client handle is invalid
CAN_ERR.RESOURCE = 0x2000  # Resource (FIFO, Client, timeout) cannot be created
CAN_ERR.ILLPARAMTYPE = 0x4000  # Invalid parameter
CAN_ERR.ILLPARAMVAL = 0x8000  # Invalid parameter value
CAN_ERR.UNKNOWN = 0x10000 # Unbekannter Fehler
CAN_ERR.MASK_ILLHANDLE = 0x1C00  # Unknown error
CAN_ERR.ANYBUSERR = (CAN_ERR.BUSLIGHT | CAN_ERR.BUSHEAVY | CAN_ERR.BUSOFF)


class NotOpenError(Exception):
    pass

class CanError(Exception):
    """General CAN Error"""
    def __str__(self):
        return self.__doc__

class XMTFULL_Error(CanError):
    """Transmit buffer in CAN controller is full"""
class OVERRUN_Error(CanError):
    """CAN controller was read too late"""
class BUSLIGHT_Error(CanError):
    """Bus error: an error counter reached the 'light' limit"""
class BUSHEAVY_Error(CanError):
    """Bus error: an error counter reached the 'heavy' limit"""
class BUSOFF_Error(CanError):
    """Bus error: the CAN controller is in bus-off state"""
class QRCVEMPTY_Error(CanError):
    """Receive queue is empty"""
class QOVERRUN_Error(CanError):
    """Receive queue was read too late"""
class QXMTFULL_Error(CanError):
    """Transmit queue is full"""
class REGTEST_Error(CanError):
    """Test of the CAN controller hardware registers failed (no hardware found)"""
class NOVXD_Error(CanError):
    """Driver not loaded"""
class HWINUSE_Error(CanError):
    """The Hardware is alraedy occuped by a Net"""
class NETINUSE_Error(CanError):
    """A Client is already connected to the Net"""
class ILLHW_Error(CanError):
    """Hardware handle is invalid"""
class ILLNET_Error(CanError):
    """Net handle is invalid"""
class ILLCLIENT_Error(CanError):
    """Client handle is invalid"""
class RESOURCE_Error(CanError):
    """Resource (FIFO, Client, timeout) cannot be created"""
class ILLPARAMTYPE_Error(CanError):
    """Invalid parameter"""
class ILLPARAMVAL_Error(CanError):
    """Invalid parameter value"""
class UNKNOWN_Error(CanError):
    """Unbekannter Fehler"""
class MASK_ILLHANDLE_Error(CanError):
    """Unknown error"""
class ANYBUSERR_Error(CanError):
    pass







## // CAN Message
## //
## typedef struct {
##     DWORD ID;        // 11/29 bit identifier
##     BYTE  MSGTYPE;   // Bits from MSGTYPE_*
##     BYTE  LEN;       // Data Length Code of the Msg (0..8)
##     BYTE  DATA[8];   // Data 0 .. 7
## } TPCANMsg;


## #ifdef NTVERSION
##    #pragma pack(push, 1)    // Byte aligment (MS Visual C++)
## #endif

## #ifdef NTVERSION
##    #pragma pack(pop)        // Default alignment again
## #endif


# By default, Structure and Union fields are aligned in the same way the
# C compiler does it. It is possible to override this behaviour be specifying
# a _pack_ class attribute in the subclass definition. This must be set to a
# positive integer and specifies the maximum alignment for the fields. This is
# what #pragma pack(n) also does in MSVC.

class CANMsg(ctypes.Structure):
    _fields_ = [
        ("ID", ctypes.c_uint),
        ("MSGTYPE", ctypes.c_ubyte),
        ("LEN", ctypes.c_ubyte),
        ("DATA", ctypes.c_ubyte*8),
        ]


def makeCanMsg(can_id, can_data, msgtype = MSGTYPE_STANDARD):
    """

    """
    cm=CANMsg()
    cm.MSGTYPE = msgtype
    cm.ID = can_id
    cm.LEN = len(can_data)
    for index in range(cm.LEN):
        cm.DATA[index]=can_data[index]

    return cm




class CanDriver(object):
    """

    """
    def __init__(self):
        """

        """
        self.isOpen = False
        self.writeOps = 0

    def __del__(self):
        """

        """
        if self.isOpen:
            self.close()

    def open(self):
        """

        Possible Errors: NOVXD ILLHW REGTEST RESOURCE

        """
        if self.isOpen:
            return
        #  CAN_Init()
        #  This function make the following:
        #		- Activate a Hardware
        #		- Make a Register Test of 82C200/SJA1000
        #		- Allocate a Send buffer and a Hardware handle
        #		- Programs the configuration of the transmit/receive driver
        #		- Set the Baudrate register
        #		- Set the Controller in RESET condition	
        #		
        #  If CANMsgType=0  ---> ID 11Bit
        #  If CANMsgType=1  ---> ID 11/29Bit 
        #
        ##DWORD __stdcall CAN_Init(WORD wBTR0BTR1, int CANMsgType);        
        result = ctypes.windll.Pcan_usb.CAN_Init(CAN_BAUD._100K,CAN_INIT_TYPE_ST)
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result == CAN_ERR.ILLHW:
            raise ILLHW_Error
        elif result == CAN_ERR.REGTEST:
            raise REGTEST_Error
        elif result == CAN_ERR.RESOURCE:
            raise RESOURCE_Error
        elif result != CAN_ERR.OK:
            raise UNKNOWN_Error

        self.isOpen = True


    def close(self):
        """terminate and release the configured hardware and allocated resources.

        Possible exceptions: NOVXD_Error

        """
        if not self.isOpen:
            return

        result = ctypes.windll.Pcan_usb.CAN_Close()
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result == CAN_ERR.ILLNET:
            raise ILLNET_Error
        elif result != CAN_ERR.OK:
            print "Code: %s" % (result,)
            raise UNKNOWN_Error

        self.isOpen = False

    def getStatus(self):
        """return the current status of the hardware (b.e. BUS-OFF).

        Return values: 'BUSOFF' 'BUSHEAVY' 'OVERRUN'

        Possible exceptions: NOVXD_Error

        """
        if not self.isOpen:
            raise NotOpenError
        result = ctypes.windll.Pcan_usb.CAN_Status()
        print hex(result)
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result == CAN_ERR.BUSOFF:
            retval = "BUSOFF"
        elif result == CAN_ERR.BUSHEAVY:
            retval = "BUSHEAVY"
        elif result == CAN_ERR.OVERRUN:
            retval = "OVERRUN"
        elif result == CAN_ERR.OK:
            retval = "OK"
        elif result != CAN_ERR.OK:
            raise UNKNOWN_Error

        return retval

    def read(self):
        """

        """
        if not self.isOpen:
            raise NotOpenError
        #  CAN_Read()
        #  This function get the next message or the next error from the Receive Queue of 
        #  the CAN Hardware.
        #  REMARK:
        #		- Check always the type of the received Message (MSGTYPE_STANDARD,MSGTYPE_RTR,
        #		  MSGTYPE_EXTENDED,MSGTYPE_STATUS)
        #		- The function will return ERR_OK always that you receive a CAN message successfully
        #		  although if the messages is a MSGTYPE_STATUS message.
        #		- When a MSGTYPE_STATUS mesasge is got, the ID and Length information of the message
        #		  will be treated as indefined values. Actually information of the received message
        #		  should be interpreted using the first 4 data bytes as follow:
        #			*	Data0	Data1	Data2	Data3	Kind of Error
        #				0x00	0x00	0x00	0x02	CAN_ERR_OVERRUN		0x0002	CAN Controller was read to late
        #				0x00	0x00	0x00	0x04	CAN_ERR_BUSLIGHT	0x0004  Bus Error: An error counter limit reached (96)
        #				0x00	0x00	0x00	0x08	CAN_ERR_BUSHEAVY	0x0008	Bus Error: An error counter limit reached (128)
        #				0x00	0x00	0x00	0x10	CAN_ERR_BUSOFF		0x0010	Bus Error: Can Controller went "Bus-Off"
        #		- If a CAN_ERR_BUSOFF status message is received, the CAN Controller must to be
        #		  initialized again using the Init() function.  Otherwise, will be not possible
        #		  to send/receive more messages.
        #		- The message will be written to 'msgbuff'.
        #
        #  Possible Errors: NOVXD  QRCVEMPTY
        #
        ##DWORD __stdcall CAN_Read(CANMsg* pMsgBuff );
        cm=CANMsg()
        result = ctypes.windll.Pcan_usb.CAN_Read(ctypes.byref(cm))
        if result == CAN_ERR.OK:
            pass
        elif result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result == CAN_ERR.QRCVEMPTY:
            raise QRCVEMPTY_Error
        elif result != CAN_ERR.OK:
            print "Code: %s" % (result,)
            raise UNKNOWN_Error

        return cm

    def write(self, CANMsg):
        """place a CAN message into the Tranmit Queue of the CAN Hardware.

        """
        if not self.isOpen:
            raise NotOpenError
        #  CAN_Write()
        #  This function Place a CAN message into the Transmit Queue of
        #  the CAN Hardware
        #
        #  Possible Errors: NOVXD RESOURCE BUSOFF QXMTFULL
        #
        ##DWORD __stdcall CAN_Write( CANMsg* pMsgBuff);
        result = ctypes.windll.Pcan_usb.CAN_Write(ctypes.byref(CANMsg))
        if result == CAN_ERR.OK:
            pass
        elif result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result == CAN_ERR.ILLCLIENT:
            raise ILLCLIENT_Error
        elif result == CAN_ERR.QXMTFULL:
            raise QXMTFULL_Error
        elif result == CAN_ERR.BUSOFF:
            raise BUSOFF_Error
        elif result == CAN_ERR.RESOURCE:
            raise RESOURCE_Error
        elif result != CAN_ERR.OK:
            print "Code: %s" % (result,)
            raise UNKNOWN_Error

        self.writeOps += 1


    def getVersionInfo(self):
        """get the Version and Copyright of the hardware.

        Possible Errors:  NOVXD

        """
        s = ctypes.create_string_buffer('\000' * 255)
        result = ctypes.windll.Pcan_usb.CAN_VersionInfo(s)
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result != CAN_ERR.OK:
            raise UNKNOWN_Error
        return s.value

    def resetClient(self):
        """delete both queues (Tx,Rx) by resetting the CAN Controller.

        Possible Errors: ERR_ILLCLIENT ERR_NOVXD

        """
        result = ctypes.windll.Pcan_usb.CAN_ResetClient()
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result == CAN_ERR.ILLCLIENT:
            raise ILLCLIENT_Error
        elif result != CAN_ERR.OK:
            raise UNKNOWN_Error

    def setMsgFilter(self,FromID,ToID,Type=CAN_INIT_TYPE_ST):
        """set the receive message filter of the CAN Controller.

        REMARK:
            - A quick register of all messages is possible using the parameters FromID and ToID = 0
            - Every call of this function maybe cause an extention of the receive filter of the 
              CAN controller, which one can go briefly to RESET
            - New in Ver 2.x:
               * Standard frames will be put it down in the acc_mask/code as Bits 28..13
               * Hardware driver for 82C200 must to be moved to Bits 10..0 again!
        WARNING: 
            It is not guaranteed to receive ONLY the registered messages.

        Possible Errors: NOVXD ILLCLIENT ILLNET REGTEST

        """
        result = ctypes.windll.Pcan_usb.CAN_MsgFilter(FromID,ToID,Type)
        # TODO: finalize error handling
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result != CAN_ERR.OK:
            raise UNKNOWN_Error


    def resetMsgFilter(self):
        """completely close the Message Filter of the Hardware.

        There will be no more messages received.

        Possible Errors: NOVXD

        """
        result = ctypes.windll.Pcan_usb.CAN_ResetFilter()
        if result == CAN_ERR.NOVXD:
            raise NOVXD_Error
        elif result != CAN_ERR.OK:
            raise UNKNOWN_Error


def SetUSBDeviceNr(deviceNr):
    """set the device number of a USB CAN Hardware.

    Possible Errors: NOVXD ILLHW ILLPARAMTYPE ILLPARAMVAL REGTEST

    """
    result = ctypes.windll.Pcan_usb.SetUSBDeviceNr(deviceNr)
    if result == CAN_ERR.NOVXD:
        raise NOVXD_Error
    elif result == CAN_ERR.ILLHW:
        raise ILLHW_Error
    elif result == CAN_ERR.ILLPARAMTYPE:
        raise ILLPARAMTYPE_Error
    elif result == CAN_ERR.REGTEST:
        raise REGTEST_Error
    elif result == CAN_ERR.ILLPARAMVAL:
        raise ILLPARAMTYPE_Error
    elif result != CAN_ERR.OK:
        raise UNKNOWN_Error



def GetUSBDeviceNr():
    """read the device number of a USB CAN Hardware.

    Possible Errors: NOVXD ILLHW ILLPARAMTYPE

    """
    deviceNr = ctypes.c_long()
    result = ctypes.windll.Pcan_usb.GetUSBDeviceNr(ctypes.byref(deviceNr))
    if result == CAN_ERR.NOVXD:
        raise NOVXD_Error
    elif result == CAN_ERR.ILLHW:
        raise ILLHW_Error
    elif result == CAN_ERR.ILLPARAMTYPE:
        raise ILLPARAMTYPE_Error
    elif result != CAN_ERR.OK:
        raise UNKNOWN_Error

    return deviceNr.value


