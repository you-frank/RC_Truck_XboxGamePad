#from inputs import get_gamepad
from math import pow, floor
import threading
from inputs import get_gamepad
from time import sleep

def ddm2dm(ddm):
    ddm_calc=ddm
    if ddm<0:
        ddm_calc = ddm*(-1)
        
    deg=floor(ddm_calc/100)
    minu=ddm_calc-deg*100
    ret = deg+minu/60
    if ddm<0:
        ret = ret*(-1)
    return ret


class XboxController(object):
    MAX_TRIG_VAL = pow(2, 8)
    MAX_JOY_VAL = pow(2, 15)

    def __init__(self):
        self.LeftJoystickY, self.LeftJoystickX = 0, 0
        self.RightJoystickY, self.RightJoystickX = 0, 0
        self.LeftTrigger, self.RightTrigger, self.LeftBumper, self.RightBumper = 0,0,0,0
        self.A, self.X, self.Y, self.B = 0,0,0,0
        self.LeftThumb, self.RightThumb, self.Back, self.Start = 0,0,0,0
        self.LeftDPad, self.RightDPad, self.UpDPad, self.DownDPad = 0,0,0,0
        
        self.b_found=True;
        
        self._monitor_thread = threading.Thread(target=self._monitor_controller, args=())
        self._monitor_thread.daemon = True
        self._monitor_thread.start()
        
    def rumble(self): # vibrate left, right for 1sec
        from inputs import devices
        try:
            devices.gamepads[0].set_vibration(1,1,1000)
        except Exception as err:
            pass
        
    # read xboxController return [L_Stick_X(int), LTrigger, RTrigger, btn(b0000abxy)
    def read(self): # return the buttons/triggers that you care about in this methode
        '''x = self.LeftJoystickX
        y = self.LeftJoystickY
        a = self.A
        b = self.X # b=1, x=2
        #rb = self.RightBumper
        lt = self.LeftTrigger
        rt = self.RightTrigger
        return [x, y, a, b, rb, rt]
        '''
        btn = 0
        if self.RightBumper:
            btn+=32# b 0010 0000
        if self.LeftBumper:
            btn+=16# b 0001 0000 
        if self.Y:
            btn+=8 # b 0000 1000
        if self.X:
            btn+=4 # b 0000 0100
        if self.B:
            btn+=2 # b 0000 0010
        if self.A: 
            btn+=1 # b 0000 0001
        btn=btn&0xff
        
        LJoyXVal=0
        if self.LeftJoystickX<0:
            LJoyXVal=int(100*(pow(self.LeftJoystickX,2)*(-1)+1))
        else:
            LJoyXVal=int(100*(pow(self.LeftJoystickX,2)+1))
        
        return [LJoyXVal,
                int(self.LeftTrigger*100),  #int(pow(self.LeftTrigger,2)*100),
                int(self.RightTrigger*100), #int(pow(self.RightTrigger,2)*100),
                int(btn)]
        '''
        return [int(self.LeftJoystickX*100)+100, # checksum때문에 unsigned로 처리해버는게 쉬움.
                int(self.LeftTrigger*100),
                int(self.RightTrigger*100), int(btn)]
        '''
    def _monitor_controller(self):
        while True:
          try:
            events = get_gamepad()
            
            self.b_found=True;
            
            for event in events:
                if event.code == 'ABS_X':
                    self.LeftJoystickX = event.state / XboxController.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'BTN_SOUTH':
                    self.A = event.state
                elif event.code == 'BTN_NORTH':
                    self.Y = event.state #previously switched with X
                elif event.code == 'BTN_WEST':
                    self.X = event.state #previously switched with Y
                elif event.code == 'BTN_EAST':
                    self.B = event.state
                elif event.code == 'ABS_Z':
                    self.LeftTrigger = event.state / XboxController.MAX_TRIG_VAL # normalize between 0 and 1
                elif event.code == 'ABS_RZ':
                    self.RightTrigger = event.state / XboxController.MAX_TRIG_VAL # normalize between 0 and 1
                elif event.code == 'BTN_TL':
                    self.LeftBumper = event.state
                elif event.code == 'BTN_TR':
                    self.RightBumper = event.state
                '''
                elif event.code == 'ABS_Y':
                    self.LeftJoystickY = event.state / XboxController.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'ABS_RY':
                    self.RightJoystickY = event.state / XboxController.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'ABS_RX':
                    self.RightJoystickX = event.state / XboxController.MAX_JOY_VAL # normalize between -1 and 1
                elif event.code == 'BTN_THUMBL':
                    self.LeftThumb = event.state
                elif event.code == 'BTN_THUMBR':
                    self.RightThumb = event.state
                elif event.code == 'BTN_SELECT':
                    self.Back = event.state
                elif event.code == 'BTN_START':
                    self.Start = event.state
                elif event.code == 'BTN_TRIGGER_HAPPY1':
                    self.LeftDPad = event.state
                elif event.code == 'BTN_TRIGGER_HAPPY2':
                    self.RightDPad = event.state
                elif event.code == 'BTN_TRIGGER_HAPPY3':
                    self.UpDPad = event.state
                elif event.code == 'BTN_TRIGGER_HAPPY4':
                    self.DownDPad = event.state
                '''
          except Exception as err:
            print(f'err {err}')
            self.b_found=False;
            sleep(1)
            #os.execl(sys.executable, os.path.abspath(__file__), *sys.argv)
            #os.execl(sys.executable, os.path.abspath('gpad_rf.py'), *sys.argv) 
            pass

