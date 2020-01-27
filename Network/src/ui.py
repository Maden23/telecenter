from PyQt5 import QtWidgets, uic
from PyQt5.QtGui import QFont
from PyQt5.QtCore import QTimer, QRunnable, QThreadPool, pyqtSlot
import sys

import pyqtgraph as pg
import numpy as np
import shlex  
from subprocess import Popen, PIPE, STDOUT


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
    def __init__(self, camList):
        super(Pinger, self).__init__()
        self.camList = camList

    def _runCmd(self, cmd, stderr=STDOUT):
        """ Run commend and get output """
        args = shlex.split(cmd)
        return Popen(args, stdout=PIPE, stderr=stderr, encoding='utf8').communicate()[0]

    @pyqtSlot()
    def run(self):
        """ Function for QThreadPool to run in separate thread """
        cmd = "fping "
        for cam in self.camList:
            cmd += cam.ip + " "
        cmd += "-C 1 -q"

        # Command output: "<ip> : <time>" OR "<ip> : - " if no response was received
        responseList = self._runCmd(cmd).strip().split('\n')
        for response in responseList:
            # Parse results for each camera
            ip, response = response.strip().split(" : ")
            if (response == '-'):
                error = 'No response'
                time = 0
            else:
                error = None
                time = float(response)
            # Save results in Camera object
            for cam in self.camList:
                if cam.ip == ip:
                    cam.setPing(error, time)
                    break
            

class Grapher():
    """Draws a graph on PlotWidget"""
    def __init__(self, plot):
        self.plot = plot

        self.curve = None
        self.activeCam = None      
        self.data = None  


    def setData(self, cam, data):
        # Data is accepted only from active cam
        if cam != self.activeCam:
            return
        self.data = data
        self.curve.setData(data)


    def changeCam(self, cam):
        """Set name or ip of camera, which data should be drawn"""
        self.activeCam = cam
        # Add plot line with title according to camera name
        self.plot.clear()  
        # Format graph title
        self.plot.setTitle(cam, size='25pt', color='w')
        
        # Format graph line
        self.curve = plot.plot()
        self.curve.setPen('w', width=3)


class Camera():
    """Containes all camera information and ui objects"""
    def __init__(self, ip, button, label, grapher):
        self.ip = ip
        self.button = button
        self.label = label

        self.grapher = grapher
        self.pingHistory = np.zeros(50)


    def setPing(self, error, time):
        if error:
            self.label.setText(error)
            self.label.setStyleSheet("color: white;")
        elif time and self.label:
            self.label.setText(str(time) + " ms")
            # print(self.ip)

            # Set style for labels
            if time < 50:
                self.label.setStyleSheet("color: green;")
            elif time >= 50 and time < 200:
                self.label.setStyleSheet("color: yellow;")
            elif time >= 200:
                self.label.setStyleSheet("color: red;")

        # Update history (add new ping to array front)
        self.pingHistory = np.roll(self.pingHistory, 1)
        self.pingHistory[0] = time
        # Reverse passed array (old - first, new - last)
        self.grapher.setData(self.ip, np.flip(self.pingHistory))


    # @pyqtSlot()
    def drawMe(self):
        grapher.changeCam(self.ip)

         
adresses = {
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
# Move to the first screen
# menu.resize(540, 360)
display0 = QtWidgets.QDesktopWidget().screenGeometry(0)
menu.move(display0.left(), display0.top())
menu.showFullScreen()

# Init graph window and apply styles
graph = UIGraph()
graph.setStyleSheet(stylesheet.read())
# Move to the second screen
# graph.resize(540, 360)
display1 = QtWidgets.QDesktopWidget().screenGeometry(1)
graph.move(display1.left(), display1.top())
graph.showFullScreen()

# Get pyqtgraph PlotItem and pass it to Grapher
widget = graph.findChild(pg.PlotWidget, "plotwidget")
plot = widget.getPlotItem()
# Format axis
plot.showAxis('right', True)
rightAxis = plot.getAxis('right')
leftAxis = plot.getAxis('left')
labelStyle = {'color': '#FFF', 'font-size': '20pt'}
leftAxis.setLabel('ping', units='ms', **labelStyle)
font = QFont()
font.setPixelSize(20)
rightAxis.tickFont = font
leftAxis.tickFont = font
leftAxis.setPen('w')
rightAxis.setPen('w')

# Add grid
plot.showGrid(x=True, y=True, alpha=1.0)
grapher = Grapher(plot)


# Attach buttons and ping labels to cameras
cams = []
i = 0
for c in adresses:
    button = menu.findChild(QtWidgets.QPushButton, "b" + str(i))
    button.setText(c)
    label = menu.findChild(QtWidgets.QLabel, "l" + str(i))
    # Pass threadpool, ip and ui controls
    camera = Camera(adresses[c], button, label, grapher)
    cams.append(camera)
    # Draw graph of this camera on click
    button.clicked.connect(camera.drawMe)

    i+=1

# Create threadpool to update ping values in a separate thread
threadpool = QThreadPool()

# Check ping regularly
pingTimer = QTimer()
pinger = Pinger(cams)
def runPing():
    threadpool.start(pinger)

pingTimer.timeout.connect(runPing)
pingTimer.start(1000)

# Start Qt app
sys.exit(app.exec())
