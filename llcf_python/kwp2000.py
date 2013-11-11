import struct


class KWP2000Error(Exception):
    """ General KWP2000 error
    """
    def __init__(self, msg = None):
        self.message = msg
        
    def __str__(self):
        return "KWP2000Error: " + self.message    

class KWP2000FrameError(KWP2000Error):
    """ KWP2000 Frame error
    """
    def __str__(self):
        return "KWP2000FrameError: " + self.message    
            
class SIDFormatError(KWP2000Error):
    """ Format Error of Diagnostic Service ID
    """
    def __str__(self):
        return "SIDFormatError: " + self.message    
    
class MWBFormatError(KWP2000Error):
    """ Format error in raw MWB (Measurement block) data
    """
    def __str__(self):
        return "MWBFormatError: " + self.message    
    
class MWBFormulaError(KWP2000Error):
    """ Measurement block formula Error
    """
    def __str__(self):
        return "MWBFormulaError: " + self.message

class DTCCountError(KWP2000Error):
    """ Invalid count of DTCs Error
    """
    def __str__(self):
        return "DTCCountError: " + self.message
    
    
class parsedKWPFrame:
    """ Container object for parsed KWP2000 frame data. Contains SID, length of payload (without SID)
        and payload itself
    """
    def __init__(self, sid = 0x00, length = 0x00, payload = []):
        self.sid = sid
        self.length = length
        self.payload = payload
    
    def setSID(self, sid):
        """ Set Service ID
        """
        self.sid = sid
         
    def getSID(self):
        """ Return KWP2000 Service ID
        """
        return self.sid
    
    def setLength(self, length):
        """ Set length of data payload
        """
        self.length = length
        
    def getLength(self):
        """ Return length of payload in KWP2000 frame (without SID)
        """
        return self.length
    
    def setPayload(self, payload):
        """ Set payload of frame
        """
        self.payload = payload
        
    def getPayload(self):
        """ Return list object of payload in KWP2000 frame (without SID)
        """
        return self.payload

class dtcEntry:
    """ Container class for DTC entries in Failure memory
    """
    def __init__(self, dtc = 0x0000, status = 0x0):
        self.dtc = dtc
        self.status = dtcStatus(status)

    def __str__(self):
        text = 'DTC = 0x%04X, %s' % (self.dtc, self.status.__str__())
        return text
            
    def setDtc(self, dtc):
        self.dtc = dtc
        
    def getDtc(self):
        return self.dtc
    
    def getStatus(self):
        return self.status
        
            
class dtcStatus:
    """ Container class for DTC Stati of DTCs
    """
    def __init__(self, wlState = 0x0, storState = 0x0, symptom = 0x0, tstCompl = 0x0):
        self.warningLampState = wlState
        self.dtcStorageState = storState
        self.dtcTestComplete = tstCompl
        self.dtcFaultSymptom = symptom
        self._init_dicts()
        
    def __init__(self, bitmask = 0x0):
        self.warningLampState = ((bitmask & 0x80)>>7)
        self.dtcStorageState = ((bitmask & 0x60)>>5)
        self.dtcTestComplete = ((bitmask & 0x10)>>4)
        self.dtcFaultSymptom = (bitmask & 0x0F)        
        self._init_dicts()
            
    def _init_dicts(self):
        self.dict_Symptom = {0x0:'Not shown', 0x1:'Upper limit exceeded', 0x2:'Lower limit underrun', 0x3:'Mechanical error',
                            0x4:'No communication', 0x5:'No or wrong adaption', 0x6:'Shortcut to Vcc', 0x7:'Shortcut to Ground',
                            0x8:'Invalid signal', 0x9:'Break/Shortcut to Ground', 0xA:'Break/Shortcut to Vcc', 0xB:'Break',
                            0xC:'Electrical failure', 0xD:'Read failure memory', 0xE:'Defect', 0xF:'Not testable'}
                            
        self.dict_StorState = {0x0:'Not stored', 0x1:'sporadic', 0x2:'short time failure', 0x3:'permanent'}
        
        self.dict_LampState = {0x0:'Warning light Off', 0x1:'Warning light On'}            
        
    def __str__(self):
        text = 'Symptom = %s, Occurrence = %s, %s' % (self.getTextSymptom(),self.getTextStorageState(), self.getTextLampState())
        return text           
        
    def setWarnLampState(self, state):
        self.warningLampState = state & 0x01
        
    def getWarnLampState(self):
        return self.warningLampState
    
    def setStorageState(self, state):
        self.dtcStorageState = state & 0x03
        
    def getStorageState(self):
        return self.dtcStorageState
    
    def setFaultSymptom(self, symptom):
        self.dtcFaultSymptom = symptom & 0x0F
        
    def getFaultSymptom(self):
        return self.dtcFaultSymptom
    
    def setTestComplete(self, tst):
        self.dtcTestComplete = tst & 0x01
    
    def getTestComplete(self):
        return self.dtcTestComplete

    def getTextSymptom(self):
        return self.dict_Symptom[self.dtcFaultSymptom]
    
    def getTextStorageState(self):
        return self.dict_StorState[self.dtcStorageState]
    
    def getTextLampState(self):
        return self.dict_LampState[self.warningLampState]
    
class kwp2000:
    """ Helper class for KWP2000 data on TP 2.0 
    """
    def __init__(self):
        pass
    
    def getKWPPayload(self, data):
        """ returns a list of KWP2000 service data and
            Diagnostic header bytes (simply length bytes)
        """
        length = len(data)
        lHigh = length & 0xF00
        lLow = length & 0xFF
        dlist = list(data)
        payload = [lHigh, lLow] + dlist[:]
        return payload

    def getKWPPayload(self, sid, data):
        """ returns a list of KWP2000 service ID, data and
            Diagnostic header bytes (simply length bytes)
        """
        if sid > 255:
            raise SIDFormatError("Diagnostic SID out of range")
        length = len(data) + 1
        lHigh = length & 0xF00
        lLow = length & 0xFF
        dlist = list(data)
        payload = [lHigh, lLow] + [sid] + dlist[:]
        return payload

    def parseKWPData(self,data):
        """ Parses the KWP2000 data of an diagnostic response for example
        """
        if len(data) > 2:
            payloadLength = ((data[0] & 0xF)<<8) + data[1] - 1
            #print "Payload length assigned = %d, 0x%x,0x%x" % (payloadLength, data[0],data[1])
            sid = data[2]
            if len(data) > 3:
                payload = data[3:]
            else:
                payload = []
            #print "Payload length measured = %d" % len(payload)                
            if payloadLength != len(payload):
                raise KWP2000FrameError('Length information in KWP2000 Frame wrong')
            return parsedKWPFrame(sid, payloadLength, payload)
        else:
            raise KWP2000FrameError('Not enough data in KWP2000 Frame')
            return None
    
    def parseDTCData(self, rawdata):
        if len(rawdata) > 0:
            dtcs = []
            numOfDtc = rawdata[0]
            if len(rawdata) == (numOfDtc*3+1):
                for i in range(numOfDtc):
                    dtc = dtcEntry(((rawdata[(i*3)+1]<<8) + rawdata[(i*3)+2]),rawdata[(i*3)+3])
                    dtcs.append(dtc)    
                return dtcs
            else:
                raise DTCCountError('Number of anounced DTCs not transmitted')
            
    def parseMWBData(self, rawdata):
        """ Parse MWB data and return processed MWB data. Function returns
            a list of tuples ((value, unit),...). Values can be numbers or strings,
            units are always strings.
        """
        mwblen = len(rawdata)
        rptr = 0
        mvals = []
        if  mwblen < 3:
            raise MWBFormatError("Not enough data for Measurement block")
        elif mwblen >= 3:
            while rptr < mwblen:
                formula = rawdata[rptr]
                if len(rawdata) > rptr+2:
                    if formula == 0x06: #MW*NW*0,01V
                        unit = 'V'
                        value = rawdata[rptr+2]*rawdata[rptr+1]*0.001
                        mvals.append((value,unit)) 
                        rptr += 3
                    elif formula == 0x25: #Text
                        unit = 'Text'
                        if (rawdata[rptr+1] == 0x00) and (rawdata[rptr+2] == 0x00):
                            value = ''
                        elif (rawdata[rptr+1] == 0x01):
                            if rawdata[rptr+2] == 135:
                                value = 'Off'
                            elif rawdata[rptr+2] == 136:
                                value = 'On'
                            elif rawdata[rptr+2] == 208:
                                value = 'Detected'
                            elif rawdata[rptr+2] == 86:
                                value = 'Not detected'
                        else:
                            value = 'unknown'
                        mvals.append((value,unit))                    
                        rptr +=3     
                    elif formula == 0x75:
                        unit = 'Celsius'
                        value = (rawdata[rptr+2]-64)*rawdata[rptr+1]*0.01
                        mvals.append((value,unit)) 
                        rptr += 3                        
                    elif formula == 0xA1:
                        unit = 'Binary'
                        value = (((rawdata[rptr+1])<<8) + rawdata[rptr+2]) & 0xFFFF
                        mvals.append((value,unit))
                        rptr += 3                   
                    elif formula == 0x5F: #Langtext
                        unit = 'Long Text'
                        stringlen = rawdata[rptr+1]
                        #TODO: Remove the hack for wrong MWB 7 datalength below, when fixed in MDI
                        if len(rawdata) > (rptr+1+stringlen)-6:
                            fmt = '%dB' % (stringlen-6)
                            value = struct.pack(fmt, *rawdata[rptr+2:(rptr+1+stringlen)])
                            mvals.append((value,unit))
                        else:
                            print "String length assigned = %d, bytes found = %d" % (len(rawdata),(rptr+1+stringlen))
                            raise MWBFormatError("Not enough data for Measurement block")
                        rptr += 1+stringlen
                    else:
                        text = 'Unknown MWB formula 0x%X' % formula
                        raise MWBFormulaError(text)
                else:
                    raise MWBFormatError("Not enough data for Measurement block")
                    rptr += 3
            return mvals