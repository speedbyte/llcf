import logging
import logging.handlers
import threading
import Queue
import os.path
from time import time, sleep

class loggingObj:
    """ Container class for device log entries
    """
    def __init__(self, message = '', devID = 0xFF, tstamp = time.asctime()):
        self.tstamp = tstamp
        self.devID = devID
        self.message = message      


class LoggingModule(threading.Thread):
    """ Logging module for test results of CAN communication tests
    """

    def __init__(self, devQueue, statQueue):
        threading.Thread.__init__()        
        self.dutLogger = []
        self.logBytes = 15000000
        self.logCount = 999999
        self.shutdownFlag = False

        i = 0

        # Assign queues for LogRecords
        self.devQueue = devQueue
        self.statQueue = statQueue

        # Create log directory
        cwd = os.path.abspath(os.path.curdir)
        logDir = 'C:/Trace' + '/logs/'
        print "LogDir = " + logDir
        if not (os.path.exists(logDir)):
            os.mkdir(logDir)
            
        statsPath = logDir + 'stats.log'        
        
        #Setting up statistics logger
        self.statLogger = logging.getLogger('statLogger')
        self.statHandler = logging.handlers.RotatingFileHandler(statsPath, 'a', self.logBytes, self.logCount)
        self.statFormatter = logging.Formatter('%(asctime)s %(message)s')
        self.statHandler.setFormatter(self.statFormatter)
        self.statLogger.addHandler(self.statHandler)
        self.statLogger.setLevel(logging.INFO)

        # Setting up device loggers
        devList = ('logMdi00', 'logMdi01', 'logMdi02', 'logMdi03', 'logMdi04',
                   'logMdi05', 'logMdi06', 'logMdi07', 'logMdi08', 'logMdi09')

        for dev in devList:
            self.dutLogger.append(logging.getLogger(dev))
            fname = logDir + dev + '_' + '.log'
            tmpHandler = logging.handlers.RotatingFileHandler(fname, 'a', self.logBytes, self.logCount)
            tmpHandler.setFormatter(logging.Formatter('%(asctime)s %(message)s'))            
            self.dutLogger[i].addHandler(tmpHandler)
            self.dutLogger[i].setLevel(logging.INFO)
            i += 1

    def run(self):
        while not (self.shutdownFlag):
            sleep(0.1)
            # read Log queues and write logs
            if not (self.devQueue.empty()):
                try:
                    logObj = self.devQueue.get(False)
                    self.writeDevLog(logObj.tstamp, logObj.devID, logObj.message)
                except Queue.Empty:
                    pass                  
            if not (self.statQueue.empty()):
                try:
                    logObj = self.statQueue.get(False)
                    self.writeStat
                except Queue.Empty:
                    pass
                
    def stop(self):
        self.shutdownFlag = True
            
    def writeDevLog(self, message, devID, tstamp):
        """ Function for writing device logs of specified DUT id
        """
        msg = tstamp + ' ' + message
        self.dutLogger[devID].info(message)

    def writeStatLog(self, message):    
        """ Function for writing statistics into general log
        """
        self.statLogger.info(message)
        

#Testing code
if __name__ == "__main__":

    # Opening queues for LogRecords
    devQueue = Queue.Queue(0)
    statQueue = Queue.Queue(0)
    # Create LogModule thread
    logMod = LoggingModule(devQueue, statQueue)
    logMod.start()    

    for j in range(100):
        for i in range(10):
            msg = "Log message for device No.%i" % i
            devQueue.put(msg)
        
        logMod.writeStatLog('Statistics message')

    logMod.stop()        

    
