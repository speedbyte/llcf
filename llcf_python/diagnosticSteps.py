import llcf
import time
import struct
import kwp2000
import bcm_helper
import dut_helper
import tp20_helper


def getMWB01(llcf, tp, kwp):
    """ Read MWB 01 over TP 2.0
    """
    mwb1 = llcf.PAYLOAD((0x00, 0x02, 0x21, 0x01))        
    tp.write1(mwb1,"can0")
    for i in range(100):
        time.sleep(0.05)        
        recv_frame = tp.read()
        if recv_frame == -1:
            continue
        else:
            #print "Diagnostic response = ", recv_frame
            resp = kwp.parseKWPData(recv_frame)
            if resp.getSID() == 0x61:
                pl = resp.getPayload()
                if pl is not None and (pl[0] is not 0x7F):
                    mwbs = kwp.parseMWBData(pl[1:])
                    for mwb in mwbs:
                        print "Measurement block = ",mwb  
                break
            else:
                continue
            
def getMWB05(llcf, tp, kwp):
    """ Read MWB 05 over TP 2.0 DAP Voltage
    """
    mwb1 = llcf.PAYLOAD((0x00, 0x02, 0x21, 0x05))        
    tp.write1(mwb1,"can0")
    for i in range(100):
        time.sleep(0.05)        
        recv_frame = tp.read()
        if recv_frame == -1:
            continue
        else:
            #print "Diagnostic response = ", recv_frame
            resp = kwp.parseKWPData(recv_frame)
            if resp.getSID() == 0x61:
                pl = resp.getPayload()
                if pl is not None and (pl[0] is not 0x7F):
                    mwbs = kwp.parseMWBData(pl[1:])
                    for mwb in mwbs:
                        print "Measurement block = ",mwb  
                break
            else:
                continue              

def getMWB06(llcf, tp, kwp):
    # Write some test data over TP 2.0
    mwb6 = llcf.PAYLOAD((0x00, 0x02, 0x21, 0x06))    
    tp.write1(mwb6,"can0")
    for i in range(100):
        time.sleep(0.05)        
        recv_frame = tp.read()
        if recv_frame == -1:
            continue
        else:
            #print "Diagnostic response = ", recv_frame
            resp = kwp.parseKWPData(recv_frame)
            if resp.getSID() == 0x61:
                pl = resp.getPayload()
                if pl is not None and (pl[0] is not 0x7F):
                    mwbs = kwp.parseMWBData(pl[1:])
                    for mwb in mwbs:
                        print "Measurement block = ",mwb  
                break
            else:
                continue    

def getMWB07(llcf, tp, kwp):
    # Read ucMedia SW version
    mwb7 = llcf.PAYLOAD((0x00, 0x02, 0x21, 0x07))     
    tp.write1(mwb7,"can0")
    for i in range(100):
        time.sleep(0.05)        
        recv_frame = tp.read()
        if recv_frame == -1:
            continue
        else:
            #print "Diagnostic response = ", recv_frame
            resp = kwp.parseKWPData(recv_frame)
            if resp.getSID() == 0x61:
                pl = resp.getPayload()
                if pl is not None and (pl[0] is not 0x7F):
                    mwbs = kwp.parseMWBData(pl[1:])
                    for mwb in mwbs:
                        print "Measurement block = ",mwb  
                break
            else:
                continue

def getDTC(llcf, tp, kwp):
    # Read Failure memory
    dtc1 = llcf.PAYLOAD((0x00, 0x04, 0x18, 0x02, 0xFF, 0x00))      
    tp.write1(dtc1,"can0")
    for i in range(100):
        time.sleep(0.05)        
        recv_frame = tp.read()
        if recv_frame == -1:
            continue
        else:
            #print "Diagnostic response = ", map(hex,recv_frame)
            resp = kwp.parseKWPData(recv_frame)
            if resp.getSID() == 0x58:
                pl = resp.getPayload()
                if pl is not None:
                    if (pl[0] is not 0x7F):
                        dtcs = kwp.parseDTCData(pl)
                        for dtc in dtcs:
                            print dtc  
                    elif pl[0] is 0x7F:
                        print "Negative response received."
                else:
                    print "No KWP2000 payload received." 
                break
            else:
                continue 

                        
if __name__ == "__main__":
     
    dut01 = dut_helper.dutObj()
    dut02 = dut_helper.dutObj()
    dut03 = dut_helper.dutObj()
    dut01.setRemoteTpId(0x54)
    dut02.setRemoteTpId(0x53)
    dut03.setRemoteTpId(0x55)        
    duts = [dut01, dut02, dut03]
    tp = None
    bcm = None
      
    try:
        #Create BCM socket for TP Dynamic Channel Setup and cyclic ignition message          
        bcm = llcf.SOCKET("PF_CAN", "SOCK_DGRAM", "CAN_BCM")
        kl15 = bcm_helper.bcm_helper()
        kl15.startKl15(bcm, 'can0')      
        
        while 1:
            try:
                for dut in duts:
                    tptest = tp20_helper.tp20_helper(dut)                
                    time.sleep(5)
                    tp = tptest.openTPChannel('can0')
                    if tp is not None:    
                        kwp = kwp2000.kwp2000()
                        getMWB01(llcf, tp, kwp)
                        getMWB05(llcf, tp, kwp)                        
                        getMWB06(llcf, tp, kwp)
                        getMWB07(llcf, tp, kwp)          
                        getDTC(llcf, tp, kwp)        
                        tp.close()
            except kwp2000.KWP2000Error,e:
                if tp is not None:
                    tp.close()
                print e
        bcm.close
    except:
        if tp is not None:
            tp.close()
        if bcm is not None:
            bcm.close()                       
        print "Received unknown Error type"
        raise