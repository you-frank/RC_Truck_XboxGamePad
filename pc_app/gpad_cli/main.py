from utils import XboxController, ddm2dm
from struct import Struct, pack, unpack
from serial import Serial
from time import sleep
import sys

RFserialPort="COM38"
#RFserialPort="/dev/ttyACM0"

if __name__ == '__main__':
    
    try:
        ser= Serial(RFserialPort, 9600, timeout=1) # timeout : 수신용 timeout 1sec. 1초에 한번씩 GPS받을것임. 2xfloat
    except Exception as err:
        print(f'Serial Error : {err}')
        sys.exit(0)
        
    ser.flushInput()
    #gpadInfo_struct = Struct('5H') # START_FRM, L_stick_X, L_Trigger, R_Trigger, btns
    gpadInfo_struct = Struct('H4B') # START_FRM, L_stick_X, L_Trigger, R_Trigger, btns
    rcv_struct = Struct('2fB'); # 수신용 Struct 일단 2xfloat(GPS la,lo)+1Byte(RPM)
    
    joy = XboxController()

    while True:
        if joy.b_found==False:
            print('GamePad Error!!')
            ser.close()
            #os.execl(sys.executable, os.path.abspath(__file__), *sys.argv)
            #os.system(f'python {os.path.abspath(__file__)}')
            sys.exit(0)
            
        gpad_rd=joy.read()
#        print(gpad_rd) # print gpad info
        gpadInfo = gpadInfo_struct.pack(0xffff, gpad_rd[0], gpad_rd[1], gpad_rd[2], gpad_rd[3] )
        chkSum=0
        for i in gpad_rd:
            chkSum+=i
        #print(gpadInfo_struct.unpack(gpadInfo), chkSum)
        chkSum = chkSum&0xff
        #gpadInfo+=pack('H',chkSum) # add 1 byte checksum at the end
        gpadInfo+=pack('B',chkSum) # add 1 byte checksum at the end
        ser.write(gpadInfo)
#        print(f'rcv {ser.in_waiting}')
        if ser.in_waiting>=9: # la,lo(2xfloat(4byte)) + rpm(1byte)
            rd_buf=ser.read(9)
            #print(f'rcv: {rcv_struct.unpack(rd_buf)}')
            la, lo, rpm = rcv_struct.unpack(rd_buf)
            la=ddm2dm(la)
            lo=ddm2dm(lo)
            print(f"rpm:{rpm}, la:{la}, lo:{lo}");
        sleep(0.1)
