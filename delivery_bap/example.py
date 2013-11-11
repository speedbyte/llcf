import BapXmlReader
import pyCAN
import time
##desiredcanid, desiredopcode, desiredfctid, desireddata
##integer, string, string, tuple
##
   
error_dict = {
    'FunctionIdError' : 1,
    'OpCodeError' : 2,
    'DataLengthError' : 3
    }

c = pyCAN.CanDriver()
c.open()
fctid = 0
msg = BapXmlReader.BapXmlReader('BAPSW.xml')
while fctid < 5:  
        x = msg.GetMessage(0x123, 'Reset', str(fctid) ,  (0,1))
        if x == 'FunctionIdError' or x == 'OpCodeError' or x == 'DataLengthError' :
            print 'Function Id = ' + str(fctid) + ' ' + x
        else:
            print 'Function Id = ' + str(fctid) +' OK'
        if x == 'FunctionIdError':
            print 'Functionid ' + str(fctid) + ' not found'
        fctid = fctid + 1
print
print
msg.ShowParametersTX(1)
msg.ShowParametersRX(1)
print
print
x  = msg.GetMessage(0x3, 'Error', 0x2 , (6,1,2,3,4,5))
if (isinstance(x, list)):
    for p in range(len(x)):
        c.write(x[p])
else:
    print x

# opcode lsgid fctid :: 0111 1011 1100 0010  => 7B C2
#                        'Error'   2F    '2'


c.close()


