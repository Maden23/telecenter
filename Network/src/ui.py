from PyQt5 import QtWidgets, uic
from PyQt5.QtCore import QTimer, QRunnable, QThreadPool, pyqtSlot
import pyqtgraph as pg
import sys

from pythonping import ping


class UIMenu(QtWidgets.QMainWindow):
    def __init__(self):
        super(UIMenu, self).__init__() # Call the inherited classes __init__ method
        uic.loadUi('../ui/menu.ui', self) # Load the .ui file
        self.show() # Show the GUI

class UIGraph(QtWidgets.QWidget):
    def __init__(self):
        super(UIGraph, self).__init__() # Call the inherited classes __init__ method
        uic.loadUi('../ui/graph.ui', self) # Load the .ui file
        self.show() # Show the GUI

class Pinger(QRunnable):
    """ Updates ping information """
    def __init__(self, ip, button, label):
        super(Pinger, self).__init__()
        self.ip = ip
        self.button = button
        self.label = label
    
    # Function for QThreadPool to run in separate thread
    @pyqtSlot()
    def run(self):
        responseList = ping(self.ip, verbose = False, count = 1)
        for r in responseList:
            response = r
            
        if response.error_message:
            self.label.setText(response.error_message)
            self.label.setStyleSheet("color: white;")
            # print(response.error_message)
        else:
            time = response.time_elapsed_ms
            self.label.setText(str(time) + " ms")
            if time < 50:
                self.label.setStyleSheet("color: green;")
            elif time >= 50 and time < 200:
                self.label.setStyleSheet("color: yellow;")
            elif time >= 200:
                self.label.setStyleSheet("color: red;")
            # print(response.time_elapsed_ms)


class Camera():
    """Containes all camera information and ui objects"""
    def __init__(self, threadpool, ip, button, label):
        self.threadpool = threadpool
        self.ip = ip
        self.button = button
        self.label = label

    def update(self):
        pinger = Pinger(self.ip, self.button, self.label)
        threadpool.start(pinger)

         
camList = {
    '51': '172.18.200.51',
    '52': '172.18.200.52',
    '53': '172.18.200.53',
    '54': '172.18.200.54',
    '55': '172.18.200.55',
    '56': '172.18.200.56',
    '30': '172.18.199.30',
    '32': '172.18.199.32',
    '42': '192.168.15.42',   
}



app = QtWidgets.QApplication([])

# Init menu window and apply styles
menu = UIMenu()
stylesheet = open("../ui/stylesheet.css", "r")
menu.setStyleSheet(stylesheet.read())
menu.resize(540, 360)

# Init graph window and apply styles
graph = UIGraph()
graph.setStyleSheet(stylesheet.read())
graph.resize(540, 360)

# Create threadpool to update ping values in a separate thread
threadpool = QThreadPool()

# Attach buttons and ping labels to cameras
cams = []
i = 0
for cam in camList:
    button = menu.findChild(QtWidgets.QPushButton, "b" + str(i))
    button.setText(cam)
    label = menu.findChild(QtWidgets.QLabel, "l" + str(i))
    # Pass threadpool, ip and ui objects
    cams.append(Camera(threadpool, camList[cam], button, label))
    i+=1

# Update cam information periodically in separate thread
pingTimer = QTimer()
for cam in cams:
    pingTimer.timeout.connect(cam.update)
pingTimer.start(1000)

# Start Qt app
sys.exit(app.exec())
