""" 


    $Date: 2006-09-01 18:20:08 +0200 (Fr, 01 Sep 2006) $
    $Rev: 851 $
    $Author: agrawal $
    $Email: vikas.agrawal@web.de $

    Copyright (c) 2006 S1nn GmbH & Co. KG.
    All Rights Reserved.

"""

_svn_id = "$Id: BapXmlReader 851 2006-08-27 16:20:08Z agrawal $"
_svn_rev = "$Rev: 851 $"
__version__ = _svn_rev[6:-2]


import elementtree.ElementTree as ET
import BapMessage

class XmlError(Exception):
    """General CAN Error"""
    def __str__(self):
        return self.__doc__

class ErrorFunctionId(XmlError):
    """ FunctionId not found """

class ErrorOpCode(XmlError):
    """ Opcode not found """

class ErrorDataLength(XmlError):
    """ No. of data incorrect """


opcode_dict = {'Reset' : 0, 'Set' : 0, 'SetArray' : 0, 'Start' : 0,
               'Get' : 1, 'GetArray' : 1, 'AbortResult' : 1, 'GetAll' : 1,
               'SetGet' : 2, 'SetGetArray' : 2 , 'StartResult' : 2,
               'HeartBeatStatus' : 3, 'ChangedArray' : 3, 'Processing' : 3,
               'Status' : 4, 'StatusArray' : 4, 'StatusAll' : 4,
               'Error' : 7,
               }

issegmenteddict  = {'CanNoneSegInterface' : 0,
                    'CanSegInterface' : 1
                    }
datatype_dict = {'byteSequence' : 0,
                 'fixedByteSequence' : 1,
                 'unit8' : 2
                 }

class BapXmlReader(object):
        configinfo = []
        functionlistitem = []
        fctidlist = []
        fctidelemlist = []
        def __init__(self, dokument='BAPSW.xml'):
            self.tree = ET.parse(dokument)
            self.root = self.tree.getroot()

            for elem in self.tree.getiterator('Fct'):
                self.fctidlist.append(elem.get('id'))
                self.fctidelemlist.append(elem)

        def GetDocInfo(self):
            elem = self.root
            # use Comments tool from elementtree
            


        def GetConfigInfo(self):
            """ Basic Configuration Information """
            elem = self.root
            list = elem.items()
            for x in range(len(list)-1):
                print list[x][0] + '=' + list[x][1]

                                                            
        def GetMessage(self, desiredcanid, desiredopcode, desiredfctid, desireddata):
            """ Returns Errors in string mode, Input desiredcanid, Desiredopcode, desiredfctid and desireddata to get the
            message and ready to send to the PCAN driver. Assumption : data will not be segmented when datatype = uint8
            or """
            desiredfctid = str(desiredfctid)
            retval = desiredfctid in self.fctidlist
            if (retval == False):            
                return 'FunctionIdError'

            else:    
                fctid = desiredfctid
                self.desiredfctidelement = self.fctidelemlist[self.fctidlist.index(desiredfctid)]
                opcodecombined = self.desiredfctidelement.getchildren()[0].find('TX').get('opCode')
                retval = desiredopcode in opcodecombined.split('-')
                if (retval == False ):
                    return 'OpcodeError'

                else:
                    opcode = opcode_dict[desiredopcode]
                    lsgid = self.root.find('FLsg').get('id')
                    issegmentedelement = self.desiredfctidelement.getchildren()[0].find('TX').getchildren()[0] 
                    self.issegment = ET.tostring(issegmentedelement).split(' ')[0][1:]
                    self.datatype = self.desiredfctidelement.getchildren()[0].find('TX').get('dataType')
                    if self.datatype == 'byteSequence':
                        self.length = int(self.desiredfctidelement.getchildren()[0].find('TX').get('length'))
                        if len(desireddata) >= self.length:
                            return 'DataLengthError'
                        else:
                            maxlength = self.length
                    elif self.datatype == 'fixedByteSequence':
                        self.length = int(self.desiredfctidelement.getchildren()[0].find('TX').get('length'))
                        if len(desireddata) != self.length:
                            return 'DataLengthError'
                        else:
                            maxlength = self.length

                    elif self.datatype == 'uint8':       
                        self.length = 1  
                        if len(desireddata) >= 1:
                            return 'DataLengthError'
                        maxlength = 1
            data = desireddata
            canid = desiredcanid
            bapmessage = BapMessage.BapMessage(canid, lsgid, fctid, opcode, issegmenteddict[self.issegment], maxlength, data)
            a = bapmessage.getCanMessages()
            return a

        
        def ShowParametersTX(self, desiredfctid):
            """" no return value, shows the parameters related to the specific Fct id . User must interact """
            desiredfctid = str(desiredfctid)
            retval = desiredfctid in self.fctidlist
            if (retval == False):            
                return 'FunctionIdError'
            else:    
                made = []
                print 'TX for FunctionID ' + desiredfctid 
                self.desiredfctidelement = self.fctidelemlist[self.fctidlist.index(desiredfctid)]
                list = self.desiredfctidelement.getiterator('TX')
                for cc in range(len(list)):
                    made.append(list[cc].items())
                    if not made[cc]:
                        pass
                    else:
                        for xx in range(len(made[cc])):       
                            print made[cc][xx][0] + '=' + made[cc][xx][1]
                       
                            
        def ShowParametersRX(self, desiredfctid):
            """" no return value, shows the parameters related to the specific Fct id . User must interact """
            desiredfctid = str(desiredfctid)
            retval = desiredfctid in self.fctidlist
            if (retval == False):            
                return 'FunctionIdError'
            else:    
                made = []
                print 'RX for FunctionID ' + desiredfctid
                self.desiredfctidelement = self.fctidelemlist[self.fctidlist.index(desiredfctid)]
                list = self.desiredfctidelement.getiterator('RX')
                for cc in range(len(list)):
                    made.append(list[cc].items())
                    if not made[cc]:
                        pass
                    else:
                        for xx in range(len(made[cc])):       
                            print made[cc][xx][0] + '=' + made[cc][xx][1]
