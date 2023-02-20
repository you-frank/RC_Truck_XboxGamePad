from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import Qt
from serial import Serial
from inputs import devices #DeviceManager
from math import floor, ceil

import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg
#from matplotlib.figure import Figure
import matplotlib.pyplot as plt
#import numpy as np

class BarGauge(QtWidgets.QWidget):
    VAL_MIN=0
    VAL_MAX=99
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        #self.resize(80, 360)
        '''
        self.setSizePolicy(
            QtWidgets.QSizePolicy.Fixed,
            QtWidgets.QSizePolicy.Fixed
        )
        '''
        self.setSizePolicy(
            QtWidgets.QSizePolicy.MinimumExpanding,
            QtWidgets.QSizePolicy.MinimumExpanding
        )
        self.val=0

    def sizeHint(self):
        #return QtCore.QSize(40,120)
        return QtCore.QSize(80,360)

    def paintEvent(self, e):
        painter = QtGui.QPainter(self)

        brush = QtGui.QBrush()
        brush.setColor(QtGui.QColor('black'))
        brush.setStyle(Qt.SolidPattern)
        rect = QtCore.QRect(0, 0, painter.device().width(), painter.device().height())
        painter.fillRect(rect, brush)

        
        # Get current state.
        '''
        dial = self.parent()._dial
        dialMin, dialMax = dial.minimum(), dial.maximum()
        dialValue = dial.value()
        '''
        padding = 5

        # Define our canvas.
        d_height = painter.device().height() - (padding * 2)
        d_width = painter.device().width() - (padding * 2)

        # Draw the bars.
        steps=20
        step_size = d_height / steps
        bar_height = step_size * 0.8
        bar_spacer = step_size * 0.1 # / 2

        #pc = (dialValue - dialMin) / (dialMax - dialMin)
        pc = (self.val - BarGauge.VAL_MIN) / (BarGauge.VAL_MAX - BarGauge.VAL_MIN)
        n_steps_to_draw = int(pc * steps)
        colorSteps = int(256/steps)
        
        for n in range(n_steps_to_draw):
            rect = QtCore.QRect(
                int(padding),
                int(padding + d_height - ((n+1) * step_size) + bar_spacer),
                int(d_width), int(bar_height) )
            # turn green to red
            brush.setColor(QtGui.QColor(colorSteps*n, 255-(colorSteps*n), 0, 255))
            painter.fillRect(rect, brush)

        painter.end()

    def _trigger_refresh(self):
        self.update()
    
    # Set Gauge Value and redraw BarGauge
    def setVal(self, nVal):
        if nVal>BarGauge.VAL_MAX:
            nVal=BarGauge.VAL_MAX
        elif nVal<BarGauge.VAL_MIN:
            nVal=BarGauge.VAL_MIN
        self.val=nVal
        self.update()
#############  end of _bar #############


class MplCanvas(FigureCanvasQTAgg):
    def __init__(self, parent=None, width=5, height=4, dpi=100):
        #fig = Figure(figsize=(width, height), dpi=dpi)
        self.fig=plt.figure(figsize=(width, height), dpi=dpi)
        #self.axes = fig.add_subplot(111)
        super(MplCanvas, self).__init__(self.fig)
        

class MyDashBoard(QtWidgets.QWidget):
    """
    Custom Qt Widget to show a power bar and dial.
    Demonstrating compound and custom-drawn widget.
    """

    def __init__(self, steps=5, *args, **kwargs):
        super(MyDashBoard, self).__init__(*args, **kwargs)
        
        #self.inputs = DeviceManager()
        '''
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.secTimerUpdate)
        self.timer.setInterval(1000) # msec.  1000=1sec
        self.timer.start()#1000)
        '''
        self.serlalStr="" # ComboBox Selected Serial name String
        self.ser=Serial() # Serial dummy

        grid = QtWidgets.QGridLayout()
        grid.setSpacing(10)
        self.barGauge = BarGauge()
        grid.addWidget(self.barGauge, 0,0,9,3, Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignVCenter)
        '''
        self._dial = QtWidgets.QDial()
        self._dial.valueChanged.connect(
           self._bar._trigger_refresh
        )
        
        grid.addWidget(self._dial,9,0,3,3, Qt.AlignmentFlag.AlignBottom | Qt.AlignmentFlag.AlignVCenter)
        '''
        #중간 빈영역(colum 3~9) 채우기 위한 용도. 없으면 barGauge가 확 늘어서 빈영역 다 채움.
        #grid.addWidget(QtWidgets.QLabel(self),0, 3, 5, 6)#, 1, 1)
        #x=np.random.randint(10, size=(5))
        self.x=[]
        self.y=[]
        #y=np.random.randint(20, size=(5))
        self.figure = plt.figure()
        self.canvas = FigureCanvasQTAgg(self.figure)
        self.ax=self.figure.add_subplot(111)
        self.ax.set_xticks([])
        self.ax.set_yticks([])

        self.ax.plot(self.x, self.y)
        self.canvas.draw()
        #sc.axes.plot([0,1,2,3,4], [10,1,20,3,40])
        
        grid.addWidget(self.canvas,0, 3, 6, 6)#, 1, 1)
        
        self.serialLabel = QtWidgets.QLabel(self)
        self.serialLabel.setText('teset')
        grid.addWidget(self.serialLabel, 1, 9)#, 1, 1)
        
        self.serialCombo = QtWidgets.QComboBox(self)
        self.UpdateSerialCombo()
        self.serialCombo.activated[str].connect(self.onSerialComboChanged)
        self.serlalStr=str(self.serialCombo.currentText()) # ComboBox Selected Serial name String
        grid.addWidget(self.serialCombo, 3, 9)#, 1, 1)

        btn = QtWidgets.QPushButton("Refresh Serial")
        btn.clicked.connect(self.UpdateSerialCombo)
        grid.addWidget(btn, 4, 9)#, 1, 1)
        self.conn_btn = QtWidgets.QPushButton("Connect Serial")
        self.conn_btn.clicked.connect(self.ConnectSerial)
        grid.addWidget(self.conn_btn, 5, 9)#, 1, 1)

        label = QtWidgets.QLabel(self)
        label.setText('GamePad :')
        label.setFont(QtGui.QFont('Arial', 12))
        grid.addWidget(label, 7, 8, Qt.AlignmentFlag.AlignRight)
        
        self.gpadOk = QtWidgets.QLabel(self)
        self.gpadOk.setText('GamePad found')
        self.gpadOk.setPixmap(self.style().standardIcon(getattr(QtWidgets.QStyle, 'SP_DialogApplyButton')).pixmap(QtCore.QSize(32,32)))
        grid.addWidget(self.gpadOk, 7, 9, Qt.AlignmentFlag.AlignCenter) 
        self.gpadNotOk = QtWidgets.QLabel(self)
        self.gpadNotOk.setText('GamePad Not found')
        self.gpadNotOk.setPixmap(self.style().standardIcon(getattr(QtWidgets.QStyle, 'SP_DialogCancelButton')).pixmap(QtCore.QSize(32,32)))
        grid.addWidget(self.gpadNotOk, 7, 9, Qt.AlignmentFlag.AlignCenter)
        
        self.b_gamepad=False;
        self.UpdateGpad() # gamepad 처음엔 연결안된걸로 표시
        
        btn = QtWidgets.QPushButton("Quit")
        btn.clicked.connect(self.ExitApp)
        grid.addWidget(btn, 11, 9)

        self.setLayout(grid)
        self.setGeometry(1000, 600, 600, 500)
        #self.setFixedSize(600, 500)
        self.setWindowTitle('DashBoard')
        self.setWindowFlags(Qt.WindowTitleHint | Qt.CustomizeWindowHint |
                            Qt.MSWindowsFixedSizeDialogHint)
    def testChangeLabel(self, text):
        self.serialLabel.setText(text)
        
    def ExitApp(self, event):
        reply = QtWidgets.QMessageBox.question(
            self, "Quit",
            "Are you sure you want to quit?",
            QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel,
            QtWidgets.QMessageBox.Ok)

        if reply == QtWidgets.QMessageBox.Ok:
            self.close()
        else:
            pass
    
    def UpdateSerialCombo(self):
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        #print(ports)
        self.serialCombo.clear()
        
        i=serComboSel=0
        if len(ports)==0:
            QtWidgets.QMessageBox.question(
                self, "Warning",
                "No Serial Port Exist",
                QtWidgets.QMessageBox.Ok,
                QtWidgets.QMessageBox.Ok)
            return
        
        self.b_CH9102=False
        for port in sorted(ports):
            self.serialCombo.addItem(port.device)
            if port.description.find('CH9102')!=-1 or port.description.find('CP210')!=-1:
                serComboSel=i
                self.b_CH9102=True
                self.serlalStr=port.device
            print(f"{port.device}: {port.description}")
            i+=1
            
        self.serialCombo.setCurrentIndex(serComboSel)
        if len(self.serlalStr)==0:
            self.serlalStr=self.serialCombo.currentText()
        try:
            if self.ser.is_open:
                self.ser.close()
            self.ser=Serial(self.serlalStr, 9600, timeout=1)
        except Exception as err:
            print(f'Serial Error : [{self.serlalStr}] {err}')
            QtWidgets.QMessageBox.question(
            self, "Warning",
            "Serial Opening Error",
            QtWidgets.QMessageBox.Ok,
            QtWidgets.QMessageBox.Ok)
            
            
    def onSerialComboChanged(self, text):
        self.serlalStr=text
        try:
            if self.ser.is_open:
                self.ser.close()
            self.ser=Serial(self.serlalStr, 9600, timeout=1)
        except Exception as err:
            print(f'Serial Error : {err}')
            QtWidgets.QMessageBox.question(
            self, "Warning",
            "Serial Opening Error",
            QtWidgets.QMessageBox.Ok,
            QtWidgets.QMessageBox.Ok)        
        
    def ConnectSerial(self):
        self.serialLabel.setText(self.serlalStr)

    def UpdateGpad(self):
        #devices._detect_gamepads()
        #if len(devices.gamepads)==0:
        if self.b_gamepad==False or self.b_gamepad==-1:
            self.gpadOk.hide()
            self.gpadNotOk.show()
        else:
            #self.b_gamepad=True
            self.gpadOk.show()
            self.gpadNotOk.hide()
    '''
    def hex_to_RGB(hex_str):
        # #FFFFFF -> [255,255,255]
        #Pass 16 to the integer function for change of base
        return [int(hex_str[i:i+2], 16) for i in range(1,6,2)]
    def get_color_gradient(c1, c2, n):
        #Given two hex colors, returns a color gradient with n colors.

        c1_rgb = np.array(hex_to_RGB(c1))/255
        c2_rgb = np.array(hex_to_RGB(c2))/255
    
    def _get_color_gradient(self, n):
        import numpy as np
        assert n > 1
        color_start = [255,255,255]
        color_end = [48,48,48]
        
        c1_rgb = np.array(color_start)/255
        c2_rgb = np.array(color_end)/255
        
        mix_pcts = [x/(n-1) for x in range(n)]
        rgb_colors = [((1-mix)*c1_rgb + (mix*c2_rgb)) for mix in mix_pcts]
        return ["#" + "".join([format(int(round(val*255)), "02x") for val in item]) for item in rgb_colors]
    '''
    def addCoords(self, la, lo):
        
        if la<0:
            la=round( (1+la-ceil(la))*1000000 )  # -12.123 => 1-0.123 => 0.877  -값일때는 증가하는 방향이 반대임. 뒤집어놔야 그릴때 좌우가 안바뀜.
        else:
            la=round( (la-floor(la))*1000000 )

        if lo<0:
            lo=round( (1+lo-ceil(lo))*1000000 )  # -12.123 => 1-0.123 => 0.877  -값일때는 증가하는 방향이 반대임. 뒤집어놔야 그릴때 좌우가 안바뀜.
        else:
            lo=round( (lo-floor(lo))*1000000 )
        
        if len(self.x)>1:
            if abs(self.x[-1]-lo)+abs(self.y[-1]-la) < 20: # Skip when variation is too small
                return
        
        print(f"la{la}, lo{lo}")
        if len(self.x)>200:
            del(self.x[0])
        self.x.append(lo)

        if len(self.y)>200:
            del(self.y[0])
        self.y.append(la)
        
        self.ax.clear()
        self.ax.set_xticks([])
        self.ax.set_yticks([])
        self.ax.plot(self.x, self.y, color='#606060')
        self.canvas.draw()
        
    def secTimerUpdate(self):
        from random import random
        #x=np.random.randint(10, size=(5))
        #y=np.random.randint(20, size=(5))
        self.addCoords(round(random(),6), round(random(),6))
        return
    
        self.x.append(round(random(),6))
        self.y.append(round(random(),6))
        if len(self.x)<2:
            self.ax.plot(self.x, self.y)
            return

        #grad=self._get_color_gradient(len(self.x))
        #i=0;
        #for f_ax in self.ax.flatten():
        #    f_ax.plot(x[i], y[i], color=grad[i])
        #    i+=1
        #for i,line in enumerate(self.ax.lines):
        #for i,clr in enumerate(grad):
        #    self.ax.lines[i].set_color(clr)
        #    self.ax.plot(self.x[i], self.y[i], color=clr)
            #print(f"i{i}, c{clr}")
            
            #line.set_color(grad[i])
        self.ax.plot(self.x, self.y, color='#888888')
        self.figure.canval.draw()
        #self.ax.draw()
        self.canvas.draw()
        
