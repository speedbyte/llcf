class dutObj:
    def __init__(self):
        self.remTpId = 0x00
           
    def getRemoteTpId(self):
        """ Return TP id of DUT
        """
        return self.remTpId
    
    def setRemoteTpId(self, id):
        """ set TP id of DUT
        """
        self.remTpId = id