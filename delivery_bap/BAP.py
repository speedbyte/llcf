"""
reload(bap8)
msg = bap8.USERMESSAGE()
bap8.send(msg)
setFctId(value) setLsgId(value) setOpCode(value) .. the above creates a USERMESSAGE, extracts the various parameters,
and sends it on Peak driver after breaking it in segements.......
Change Log from bap7 = can change LSGID, FCTID; and opcode. functions added
"""

try:
    reload(pyCAN)
except:
    import pyCAN

class CANERROR(Exception):
    """General CAN Error"""
    def __str__(self):
        return self.__doc__

import lbits
import random


c = pyCAN.CanDriver()
c.open()


canidlength_dict = {
    "STD": 0,
    "EXT": 2
    }

class ERRORLENGTH(CANERROR):
    """Lange ist mehr als was BAP Protokol schicken kann"""

     
class USERMESSAGE(object):
    def __init__(self, elem = 0, BPL = (0x2b, 0xd8), BAL = (0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x0A, 0x0B, 0x0C, 0x0D)):
        self.BPL = BPL 
        if elem == 0:
            self.BAL = BAL
        else:
            self.BAL = []
            while elem:
                self.BAL.append(random.randint(0,255))
                elem = elem - 1
            self.BAL = tuple(self.BAL)
        self.DecodeBPLHeader()

    def setFctId(self, value):
        """ change the Function ID and rewrite the entire header """
        fctid_temp = self.convert_tobitstring(value, 6) # convert new value to 6 bits binary
        if fctid_temp > 0x3F:
                raise TypeError
        self.BPL = []
        #       opcode ( + + + + )   LSGID ( + + + + + + )   FCTID ( + + + + + + ) = 16 bits
        BPLstring = self.convert_tobitstring(self.BPLcombined[0],4) + self.convert_tobitstring(self.BPLcombined[1],6) + fctid_temp
        self.BPL.append(int(BPLstring[:8], 2))        # convert 1st byte  to binary
        self.BPL.append(int(BPLstring[8:], 2))        # convert 2nd byte  to binary
        self.BPL = tuple(self.BPL)                  # protect BPL from damage
        self.DecodeBPLHeader()
      
    def setLsgId(self, value):
        """ change the LSG ID and rewrite the entire header """
        lsgid_temp = self.convert_tobitstring(value, 6)
        if lsgid_temp > 0x3F:
                raise TypeError
        self.BPL = []
        #       opcode ( + + + + )   LSGID ( + + + + + + )   FCTID ( + + + + + + ) = 16 bits
        nochmal = self.convert_tobitstring(self.BPLcombined[0],4) + lsgid_temp + self.convert_tobitstring(self.BPLcombined[2],6)
        self.BPL.append(int(BPLstring[:8], 2))        # convert 1st byte  to binary
        self.BPL.append(int(BPLstring[8:], 2))        # convert 2nd byte  to binary
        self.BPL = tuple(self.BPL)                  # protect BPL from damage    
        self.DecodeBPLHeader()
        
    def setOpCde(self, value):
        """ change the Opcode and rewrite the entire header """
        opcode_temp = self.convert_tobitstring(value, 4)
        if opcode_temp > 0xF:
                raise TypeError
        self.BPL = []
        #       opcode ( + + + + )   LSGID ( + + + + + + )   FCTID ( + + + + + + ) = 16 bits
        nochmal = opcode_temp + self.convert_tobitstring(self.BPLcombined[1],6) + self.convert_tobitstring(self.BPLcombined[2],6) 
        self.BPL.append(int(BPLstring[:8], 2))        # convert 1st byte  to binary
        self.BPL.append(int(BPLstring[8:], 2))        # convert 2nd byte  to binary
        self.BPL = tuple(self.BPL)                  # protect BPL from damage    
        self.DecodeBPLHeader()  
              
    def segmentmessage(self):
        """ segment the messages , everytime start from new when the send is pressed"""
        self.segmentedmessages = []
        self.prehead = []
        if len(self.BAL) <= 6:
            self.segmentedmessages.append(self.BPL + self.BAL[:6])   # combine BPL and BAP in one tuple
            return self.segmentedmessages
        elif len(self.BAL) > 2**12:
            raise ERRORLENGTH
        else:
            ch_no = "00"
            length = Laenge = (len(self.BAL))
            lengthfield = self.convert_tobitstring(Laenge, 12)      # the length field is 12 bits long
            self.prehead.append(int("10" + ch_no + lengthfield[:4], 2))  # make the first BYTE ( 2bit + 2 bit + higher nibble of Lange )
            self.prehead.append(int(lengthfield[4:],2))   # make the second byte, the lower 8 bits of Lange
            self.segmentedmessages.append(tuple(self.prehead) + self.BPL + self.BAL[:4])
            length = length - 4          # first 4 bytes of BAL are send, remaining are (length - 4)
            seq_no = 0                  # start with seq no. 0, add 1 for each subsequent frame
            x = 4                       # go to the 4th element in BAL, 0 1 2 3 are already sent
            while length > 0:                # if length is still > 0 .i.e BAL is greater than 6 bytes
                """ repeat the step to create an tuple of tuples """
                self.nexthead = []
                seqnofield = self.convert_tobitstring(seq_no, 4)
                self.nexthead.append(int("11" + ch_no + seqnofield, 2))
                self.segmentedmessages.append(tuple(self.nexthead) + self.BAL[x:min(len(self.BAL),(x+7))])
                seq_no = seq_no + 1        # increase squence number
                x =x + 7                   # go to the x + 7th element in the BAL
                length = length - 7        # decrease the length subsequently        
            return self.segmentedmessages   # FINAL MESSAGE, completely segmented and ready to send


    def getFctId(self):
        return hex(self.FctId)

    def getLsgId(self):
        return hex(self.LsgId)

    def getOpCde(self):
        return hex(self.OpCde)

    def DecodeBPLHeader(self):
        total = ((self.BPL[0]<<8) + (self.BPL[1]))   # 2BD8
        print hex(total)
        self.FctId = total&(0x003F) # extract last 6 bits
        self.LsgId = total&(0x0FC0) # extract next 6 bits
        self.LsgId = self.LsgId >> 6
        self.OpCde = total&(0xF000) # extract next 3 bits
        self.OpCde = self.OpCde >> 12
        self.BPLcombined = (self.OpCde, self.LsgId, self.FctId)
        print 'Opcde = %s' %hex(self.OpCde)
        print 'LsgId = %s' %hex(self.LsgId)
        print 'FctId = %s' %hex(self.FctId)

    def convert_tobitstring(self,number, positions):
        """ Give any integer value and this function converts the integer into bits maximally positioned acording to
        positions, returns a string... eg convert_tobitstring(0 , 4 ) = >  '0000'"""
        x = lbits.LBits(number)
        y = x.__repr__()
        y = y[:-1]
        y = y[1:]
        length = len(y)
        total = ''
        for x in range(positions-length):
            total =  '0' + total
        total = total + y
        return total
 
          
def send(USERMESSAGE, can_id):
    """ parameters are USERMESSAGE instance and can_id"""
    tmp_message = USERMESSAGE.segmentmessage()
    for x in range(len(tmp_message)):
        length = len(tmp_message[x])
        message = pyCAN.CANMsg(can_id, canidlength_dict["STD"], length, tmp_message[x]) # id is dummy
        c.write(message)
        
def recv():
    x = c.read()
    return x

def closingsocket():
    c.close()
