from threading import Timer
from time import sleep
from struct import Struct, pack, unpack
#from serial import Serial
from math import pow, floor

class RepeatTimer(Timer):  
    def run(self):  
        while not self.finished.wait(self.interval):  
            self.function(*self.args,**self.kwargs)  
            #print(' ')  

# Convert ddm GPS Coordination to dm
def ddm2dm(ddm):
    ddm_calc=ddm
    if ddm<0:
        ddm_calc = ddm*(-1)
        
    deg=floor(ddm_calc/100)
    minu=ddm_calc-deg*100
    ret = round(deg+minu/60, 6)
    if ddm<0:
        ret = ret*(-1)
    return ret


# 0.05초 마다 반복되서 불리워서 gamepad 상태를 계속 읽어옴.
def GamePadMonitoring(evt, gpad, dash):
    print('start thread')
    while evt.is_set()==False:
        if gpad.b_connected!=-1: #연결되었다가 끊어지면 알아챌수가 없음. exception에서 -1로 만들고, 복구되길 기다리면 됨.
            gpad.checkConnect()			# 연결 확인함.

        if gpad.b_connected != dash.b_gamepad: # 연결상태 바뀌면 dashboard에 update 해줌.
            print(f'state change {dash.b_gamepad} {gpad.b_connected}')
            dash.b_gamepad = gpad.b_connected
            dash.UpdateGpad()
            
        if gpad.b_connected!=False:
            gpad._monitor_controller()  # joypad 입력 읽어서 내부 변수들 update.
        #sleep(0.1)
    print('exit thread')


# 0.1Sec 간격으로 읽어서 RF통해 전송.
# joypad 연결이 끊겨있더라도 default value를 계속 전송하고 있어야함.
def RF_daemon(evt, gpad, dash):
    send_struct = Struct('2H3B') # START_FRM, L_stick_X, L_Trigger, R_Trigger, btns
    rcv_struct  = Struct('2fB') # 수신용 Struct 일단 2xfloat(GPS la,lo)+1Byte(RPM)
    
    while evt.is_set()==False:
        ret=(100,0,0,0) # default return value. first 100 is middle position of L-R Left Joystick(0~199)
        if gpad.b_connected==True:
            ret=gpad.read()
            
        if dash.ser.is_open:
            gpadInfo = send_struct.pack(0xffff, *ret) # *ret를 쓰면 배열, 튜플은 모드 풀어서 전달함.
            chkSum=0
            for i in gpadInfo[2:]: # 0xffff skip하기 위해서 [2:]임.
                chkSum+=i
            #print(send_struct.unpack(gpadInfo), chkSum)
            chkSum = chkSum&0xff
            gpadInfo+=pack('B',chkSum) # add 1 byte checksum at the end
            try:
                dash.ser.write(gpadInfo)
            except Exception as err:
                print(f'Serial Write Error : [{err}]')
                
            try:
                if dash.ser.in_waiting>=9:
    #                print('clear over 9')
    #                dash.ser.reset_input_buffer()
    #            elif dash.ser.in_waiting==9: # la,lo(2xfloat(4byte)) + rpm(1byte)
                    rd_buf=dash.ser.read(9)
                    dash.ser.reset_input_buffer()
                    la, lo, rpm = rcv_struct.unpack(rd_buf)
                    #print(f'rcv: {rcv_struct.unpack(rd_buf)}')
                    dash.barGauge.setVal(rpm)
                    #print(f"rpm:{rpm}, la:{la}, lo:{lo}");
                    if la!=0 and lo!=0:
                        la=ddm2dm(la)
                        lo=ddm2dm(lo)
                        dash.addCoords(la, lo)
            except Exception as err:
                print(f'Serial Read Error88 : [{err}]')
                
        sleep(0.1)
        #print(ret)
        # send ret data to RF Here
        # Write data to RF
    
#    mon_timer = threading.Timer(tm_interval, GamePadMonitoring, [gpad, dash])
#    mon_timer.start()
#    return mon_timer
    

