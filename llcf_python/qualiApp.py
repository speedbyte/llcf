
class DiagnosticTester:

    def __init__(self):
        pass

    def openTPSession(self, dut):
        pass
        return False

    def closeTPSession(self, dut):
        pass
        return False

    def testBoardVoltage(self, dut, timeout):
        pass
        return False

    def testBoardTemperature(self, dut, timeout):
        pass
        return False

    def testUCMediaVersion(self, dut, timeout):
        pass
        return False

    def readDTCMemory(self, dut, timeout):
        pass
        return False

    def runTests(self, dut, logModule):
        res1 = self.openTPSession(dut)
        res2 = self.testBoardVoltage(dut, 1)
        res3 = self.testBoardTemperature(dut, 1)
        res4 = self.testUCMediaVersion(dut, 1)
        res5 = self.readDTCMemory(dut, 1)
        res6 = self.closeTPSession(dut)
        if (res1 and res2 and res3 and res4 and res5 and res6):
            return True
        else:
            return False

class BAPTester:

    def __init__(self):
        pass

    def awaitHeartBeats(self, dut, timeout):
        pass
        return False

    def requestPlayStatus(self, dut, timeout):
        pass
        return False

    def runTests(self, dut):
        res1 = self.awaitHeartBeats(dut, 1)
        res2 = self.requestPlayStatus(dut, 1)
        if (res1) and (res2):
            return True
        else:
            return False


        