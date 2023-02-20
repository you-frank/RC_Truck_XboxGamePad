from PyQt5 import QtWidgets
from threading import Thread, Event
from time import sleep
from MyWidgets import MyDashBoard
from XboxController import GamePad
from utils import RepeatTimer, GamePadMonitoring, RF_daemon
#import sys

if __name__ == '__main__':
    app = QtWidgets.QApplication([])
    myDash = MyDashBoard()
    myDash.show()
    gpad=GamePad()

    #gpad_mon=RepeatTimer(0.05, GamePadMonitoring, [gpad, myDash])
    evt = Event()
    gpad_mon = Thread(target=GamePadMonitoring, args=(evt, gpad, myDash))
    gpad_mon.start()
    
    #gpad_send=RepeatTimer(0.1, GamePadRFSend, (gpad, myDash))
    rf_mon = Thread(target=RF_daemon, args=(evt, gpad, myDash))
    rf_mon.start()
    
    app.exec_()
    print('exit')
    evt.set()
    #sleep(0.15)
    gpad_mon.join()
    #gpad_send.cancel()
    rf_mon.join()
    app.quit()
