from math import pow, floor
from inputs import get_gamepad, devices
import threading
from time import sleep
from platform import platform

class GamePad(object):
    MAX_TRIG_VAL = pow(2, 8)
    MAX_JOY_VAL = pow(2, 15)

    def __init__(self):
        self.LeftJoystickY, self.LeftJoystickX = 0, 0
        self.RightJoystickY, self.RightJoystickX = 0, 0
        self.LeftTrigger, self.RightTrigger, self.LeftBumper, self.RightBumper = 0,0,0,0
        self.A, self.X, self.Y, self.B = 0,0,0,0
        self.LeftThumb, self.RightThumb, self.Menu, self.Start = 0,0,0,0
        self.LeftDPad, self.RightDPad, self.UpDPad, self.DownDPad = 0,0,0,0
        
        self.b_connected=False;
        self._os=platform()[0:3]  #'Win' or 'Lin' or something else.

        #self._monitor_thread = threading.Thread(target=self._monitor_controller, args=())
        #self._monitor_thread.daemon = True
        #self._monitor_thread.start()
#        self._monitor_thread = threading.Timer(0.1, self._monitor_controller)
#        self._monitor_thread.start()

    def checkConnect(self):
        if self._os=='Win':
            try:
                devices._detect_gamepads()  # Not working with Linux. Windows Only
            except Exception as err:
                print(err)
                pass

        if len(devices.gamepads)==0:
            self.b_connected=False
        else:
            self.b_connected=True
        return self.b_connected
    
            
    def rumble(self): # vibrate left, right for 1sec
        from inputs import devices
        try:
            devices.gamepads[0].set_vibration(1,1,1000)
        except Exception as err:
            pass
    
    
    # read xboxController return [L_Stick_X(int), LTrigger, RTrigger, btn(b0000abxy)
    def read(self): # return the buttons/triggers that you care about in this methode
        btn_set = 0
        if self.RightBumper:
            btn_set+=0x20# b 0010 0000
        if self.LeftBumper:
            btn_set+=0x10# b 0001 0000 
        if self.Y:  # windows 에서는 button X,Y가 바뀌어있음. 버그같음.
            if self._os=='Win':
                btn_set+=0x4 
            else:
                btn_set+=0x8 # b 0000 1000
        if self.X:
            if self._os=='Win':
                btn_set+=0x8 
            else:
                btn_set+=0x4 # b 0000 0100
        if self.B:
            btn_set+=0x2 # b 0000 0010
        if self.A: 
            btn_set+=0x1 # b 0000 0001
            
        if self.Menu:
            btn_set+=0x8000#b 1000 0000
        if self.Start:
            btn_set+=0x4000# b 0100 0000 
        if self.RightThumb:
            btn_set+=0x2000# b 0010 0000
        if self.LeftThumb:
            btn_set+=0x1000# b 0001 0000 
        if self.RightDPad:
            btn_set+=0x0800 # b 0000 1000
        if self.LeftDPad:
            btn_set+=0x0400 # b 0000 0100
        if self.DownDPad:
            btn_set+=0x0200 # b 0000 0010
        if self.UpDPad: 
            btn_set+=0x0100 # b 0000 0001
        btn_set=btn_set&0xffff
        
        LJoyXVal=0
        if self.LeftJoystickX<0:
            LJoyXVal=int(100*(pow(self.LeftJoystickX,2)*(-1)+1))
        else:
            LJoyXVal=int(100*(pow(self.LeftJoystickX,2)+1))
        return (int(btn_set), LJoyXVal,
                int(self.LeftTrigger*100),  #int(pow(self.LeftTrigger,2)*100),
                int(self.RightTrigger*100)) #int(pow(self.RightTrigger,2)*100),


    def _monitor_controller(self):
        try:
            events = get_gamepad() # 이게 계속 대기중이어서 프로그램 종료때 thread가 안끝남. 어쩔???
            self.b_connected=True;
            
            for event in events:
                if event.code == 'ABS_X':
                    self.LeftJoystickX = event.state / GamePad.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'BTN_SOUTH':
                    self.A = event.state
                elif event.code == 'BTN_WEST':
                    self.Y = event.state #previously switched with X
                elif event.code == 'BTN_NORTH':
                    self.X = event.state #previously switched with Y
                elif event.code == 'BTN_EAST':
                    self.B = event.state
                elif event.code == 'ABS_Z':
                    self.LeftTrigger = event.state / GamePad.MAX_TRIG_VAL # normalize between 0 and 1
                elif event.code == 'ABS_RZ':
                    self.RightTrigger = event.state / GamePad.MAX_TRIG_VAL # normalize between 0 and 1
                elif event.code == 'BTN_TL':
                    self.LeftBumper = event.state
                elif event.code == 'BTN_TR':
                    self.RightBumper = event.state
                # Set 2                    
                elif event.code == 'BTN_THUMBL':
                    self.LeftThumb = event.state
                    print('L-Thmb')
                elif event.code == 'BTN_THUMBR':
                    self.RightThumb = event.state
                    print('R-Thmb')
                elif event.code == 'BTN_SELECT':
                    self.Menu = event.state
                    print(f'Menu {event.state}')
                elif event.code == 'BTN_START':
                    self.Start = event.state
                    print(f'Start {event.state}')
                elif event.code == 'ABS_HAT0Y':  # 얘네들은 눌렸을때 한번만 날아가고 끝임.
                    if event.state==-1:			 # 떼일때 날아가는것 없음. 
                        self.UpDPad = 1
                        print('Up-pd')
                    elif event.state==1:
                        self.DownDPad = 1
                        print('Dn-pd')
                    else:
                        self.UpDPad=self.DownDPad=0
                elif event.code == 'ABS_HAT0X':  # 얘네들은 눌렸을때 한번만 날아가고 끝임.
                    if event.state==-1:           # 떼일때 날아가는것 없음. 
                        self.LeftDPad = 1
                        print(f'L-pd {event.state}')
                    elif event.state==1:
                        self.RightDPad = 1
                        print(f'R-pd {event.state}')
                    else:
                        self.LeftDPad=self.RightDPad=0
                '''        
                else:                    
                    print(event.code)
                    print(event.state)
                    print(event.ev_type)
                    
                elif event.code == 'ABS_Y':
                    self.LeftJoystickY = event.state / GamePad.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'ABS_RY':
                    self.RightJoystickY = event.state / GamePad.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'ABS_RX':
                    self.RightJoystickX = event.state / GamePad.MAX_JOY_VAL # normalize between -1 and 1
                '''
                
        except Exception as err:
            if self.b_connected != -1:
                print(f'err {err}')
                self.b_connected=-1;
            pass


