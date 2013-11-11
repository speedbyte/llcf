""" 


    $Date: 2006-08-27 18:17:22 +0200 (So, 27 Aug 2006) $
    $Rev: 850 $
    $Author: henkelma $
    $URL: svn://voice.s1nn.int/pyCAN/trunk/BapMessage.py $

    Copyright (c) 2006 S1nn GmbH & Co. KG.
    All Rights Reserved.

"""

import pyCAN

_svn_id = "$Id: BapMessage.py 850 2006-08-27 16:17:22Z henkelma $"
_svn_rev = "$Rev: 850 $"
__version__ = _svn_rev[6:-2]




class BapMessage(object):
    """Container class for BAP Messages.

    """
    TotalMessagePayload = 8
    UnSegMessagePayload = TotalMessagePayload-2
    StartMessagePayload = TotalMessagePayload-4
    SequenceMessagePayload = TotalMessagePayload-1

    def __init__(self, CanId, LsgId, FunctionId, OpCode, IsSegmented, MaxLength, Data):
        """

        """
        self.setCanId(CanId)
        self.setLsgId(LsgId)
        self.setFunctionId(FunctionId)
        self.setOpCode(OpCode)
        self.setIsSegmented(IsSegmented)
        self.setMaxLength(MaxLength)
        self.setData(Data)

    def getCanMessages(self, LogicalChannel=0):
        """get a list of CAN messages which are representing the BAP message.

        Inputs:
            LogicalChannel - for the segmented message Transfer

        """
        if (LogicalChannel<0) or (LogicalChannel>3):
            raise ValueError, "Value for LogicalChannel needs to be between 0 and 3"

        #
        # Binary Header Format for an unsegmented BAP Message
        #
        #        0OOOLLLL LLFFFFFF
        #          byte0 | byte1
        #
        #         O - 3 Bit OpCode
        #         L - 6 Bit LsgId
        #         F - 6 Bit FunctionId
        #
        # this equals to Byte2 and Byte3 of Startmessage for segmented
        # transmission
        #

        # WARNING: The following will only work as long as all numbers
        #          are within the proper range
        byte0 = 0x00 | (self.OpCode<<4) | (self.LsgId>>2)
        byte1 = ((self.LsgId&0x03)<<6) | self.FunctionId

        result = []

        if not self.IsSegmented:
            can_data = [byte0,byte1]
            can_data.extend(self.Data)
            num_of_pad_bytes = self.MaxLength-len(self.Data)
            if num_of_pad_bytes > 0:
                can_data.extend(num_of_pad_bytes*[0])
            #print can_data
            result.append(
                pyCAN.makeCanMsg(self.CanId, can_data)
                )

        else:
            #
            # Binary Header Format for a segmented BAP Startmessage
            #
            #        10CCLLLL LLLLLLLL   byte0   byte1 (of unsgemented msg)
            #          byte0 | byte1     byte2 | byte3
            #
            #         C - 2 Bit LogicalChannel
            #         L - 12 Bit self.MaxLength
            #
            
            byte2 = byte0
            byte3 = byte1

            byte0 = 0x80 | (LogicalChannel<<4) | (self.MaxLength>>8)
            byte1 = self.MaxLength & 0xFF

            carryover = self.MaxLength   # num of bytes left to process
            data_start_index = 0

            can_data = [byte0,byte1,byte2,byte3]
            if carryover <= self.StartMessagePayload:
                can_data.extend(self.Data[data_start_index:])
                num_of_pad_bytes = self.StartMessagePayload-len(self.Data[data_start_index:])
                if num_of_pad_bytes > 0:
                    can_data.extend(num_of_pad_bytes*[0])
            else:
                if len(self.Data[data_start_index:])<self.StartMessagePayload:
                    can_data.extend(self.Data[data_start_index:])
                    num_of_pad_bytes = self.StartMessagePayload-len(self.Data[data_start_index:])
                    can_data.extend(num_of_pad_bytes*[0])
                else:
                    can_data.extend(
                        self.Data[data_start_index:data_start_index+self.StartMessagePayload]
                        )
            
            #print can_data
            result.append(
                pyCAN.makeCanMsg(self.CanId, can_data)
                )

            carryover -= self.StartMessagePayload
            data_start_index = self.StartMessagePayload


            #
            # Binary Header Format for a segmented BAP Sequencemessage
            #
            #        11CCSSSS 
            #          byte0
            #
            #         C - 2 Bit LogicalChannel
            #         S - 4 Bit SequenceNumber
            #
            byte0template = 0xc0 | (LogicalChannel<<4)
            sequence_number = 0
            if carryover>0:
                num_of_complete_seq_msgs = carryover/self.SequenceMessagePayload
                len_of_last_seq_msg = carryover%self.SequenceMessagePayload
                for seq_msg_index in range(num_of_complete_seq_msgs):
                    byte0=byte0template | sequence_number
                    can_data = [byte0]

                    if len(self.Data[data_start_index:])<self.SequenceMessagePayload:
                        can_data.extend(self.Data[data_start_index:])
                        num_of_pad_bytes = self.SequenceMessagePayload-len(self.Data[data_start_index:])
                        can_data.extend(num_of_pad_bytes*[0])
                    else:
                        can_data.extend(
                            self.Data[data_start_index:data_start_index+self.SequenceMessagePayload]
                            )

                    #print can_data
                    result.append(
                        pyCAN.makeCanMsg(self.CanId, can_data)
                        )
                    carryover -= self.SequenceMessagePayload
                    data_start_index += self.SequenceMessagePayload
                    if sequence_number<15:
                        sequence_number += 1
                    else:
                        sequence_number = 0
                if len_of_last_seq_msg>0:
                    byte0=byte0template | sequence_number
                    can_data = [byte0]

                    if len(self.Data[data_start_index:])<len_of_last_seq_msg:
                        can_data.extend(self.Data[data_start_index:])
                        num_of_pad_bytes = len_of_last_seq_msg-len(self.Data[data_start_index:])
                        can_data.extend(num_of_pad_bytes*[0])
                    else:
                        can_data.extend(
                            self.Data[data_start_index:data_start_index+self.SequenceMessagePayload]
                            )

                    
                    #print can_data
                    result.append(
                        pyCAN.makeCanMsg(self.CanId, can_data)
                        )

        return result

    def getCopy(self):
        """

        """
        raise NotImplementedError


    def setCanId(self, CanId):
        """See help in attached property."""
        CanId = int(CanId)
        if (CanId<0) or (CanId>0x7ff):
            raise ValueError, "CanID needs to be between 0 and 0x7FF"
        self._CanId = CanId

    def getCanId(self):
        """See help in attached property."""
        return self._CanId

    def setLsgId(self, LsgId):
        """See help in attached property."""
        LsgId = int(LsgId)
        self._LsgId = LsgId

    def getLsgId(self):
        """See help in attached property."""
        return self._LsgId

    def setFunctionId(self, FunctionId):
        """See help in attached property."""
        FunctionId = int(FunctionId)
        self._FunctionId = FunctionId

    def getFunctionId(self):
        """See help in attached property."""
        return self._FunctionId

    def setOpCode(self, OpCode):
        """See help in attached property."""
        self._OpCode = OpCode

    def getOpCode(self):
        """See help in attached property."""
        return self._OpCode

    def setIsSegmented(self, IsSegmented):
        """See help in attached property."""
        self._IsSegmented = bool(IsSegmented)

    def getIsSegmented(self):
        """See help in attached property."""
        return self._IsSegmented

    def setMaxLength(self, MaxLength):
        """See help in attached property."""
        self._MaxLength = MaxLength

    def getMaxLength(self):
        """See help in attached property."""
        return self._MaxLength

    def setData(self, Data):
        """See help in attached property."""
        self._Data = Data

    def getData(self):
        """See help in attached property."""
        return self._Data

    
    CanId = property(getCanId,setCanId, doc='CAN Identifier for the BAP message.')
    LsgId = property(getLsgId,setLsgId, doc='Logische Steuergeraete Id for the BAP message.')
    FunctionId = property(getFunctionId,setFunctionId, doc='Function Identifier for the BAP message.')
    OpCode = property(getOpCode,setOpCode, doc='')
    IsSegmented = property(getIsSegmented,setIsSegmented, doc='')
    MaxLength = property(getMaxLength,setMaxLength, doc='')
    Data = property(getData,setData, doc='')




if __name__=='__main__':
    
    #
    # Testcode:
    #
    
    myBapMessage1 = BapMessage(
        CanId=0x63C,         # BAP MDI
        LsgId=0x2F,          # MDI
        FunctionId=0x02,     # BAP Config
        OpCode=0x04,
        IsSegmented=False,
        MaxLength=6,
        Data=(0x03,0x00,0x00,0x00,0x03,0x00),
        )
    a = myBapMessage1.getCanMessages()
    print

    myBapMessage2 = BapMessage(
        CanId=0x63C,
        LsgId=0x01,
        FunctionId=0x01,
        OpCode=0x04,
        IsSegmented=True,
        MaxLength=4,
        Data=(1,2,3),
        )
    b = myBapMessage2.getCanMessages()
    print

    myBapMessage3 = BapMessage(
        CanId=0x63C,
        LsgId=0x01,
        FunctionId=0x01,
        OpCode=0x01,
        IsSegmented=True,
        MaxLength=12,
        Data=(1,2,3,4,5,6,7,8,9,10,11),
        )
    c = myBapMessage3.getCanMessages()
    print

    myBapMessage4 = BapMessage(
        CanId=0x63C,
        LsgId=0x01,
        FunctionId=0x01,
        OpCode=0x01,
        IsSegmented=True,
        MaxLength=5,
        Data=(1,2,3,4),
        )
    d = myBapMessage4.getCanMessages()
    print

    myBapMessage5 = BapMessage(
        CanId=0x63C,
        LsgId=0x01,
        FunctionId=0x01,
        OpCode=0x01,
        IsSegmented=True,
        MaxLength=12,
        Data=(1,2,3,4,5,6,7,8,9,10,11,12),
        )
    c = myBapMessage5.getCanMessages()
    print

    myBapMessage6 = BapMessage(
        CanId=0x63C,
        LsgId=0x01,
        FunctionId=0x01,
        OpCode=0x01,
        IsSegmented=True,
        MaxLength=5,
        Data=(1,2,3,4,5),
        )
    d = myBapMessage6.getCanMessages()
    print

    myBapMessage7 = BapMessage(
        CanId=0x63C,         # BAP MDI
        LsgId=0x2F,          # MDI
        FunctionId=0x01,     # BAP Config
        OpCode=0x04,
        IsSegmented=True,
        MaxLength=768,
        Data=(0x03,0x00,0x00,0x00,0x03,0x00),
        )
    e = myBapMessage7.getCanMessages()
    print

    myBapMessage8 = BapMessage(
        CanId=0x63C,         # BAP MDI
        LsgId=0x2F,          # MDI
        FunctionId=0x18,     # BAP Config
        OpCode=0x04,
        IsSegmented=False,
        MaxLength=4,
        Data=(0x00,0x0A,0x00,0x01),
        )
    f = myBapMessage8.getCanMessages()
    print


