from PyQt5 import QtWidgets, uic
from PyQt5.QtCore import QTimer, QRunnable, QThreadPool, pyqtSlot
import sys

import pyqtgraph as pg
import numpy as np
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
    def __init__(self, cam):
        super(Pinger, self).__init__()
        self.cam = cam

    # Function for QThreadPool to run in separate thread
    @pyqtSlot()
    def run(self):
        responseList = ping(self.cam.ip, verbose = False, count = 1)
        # print(cam.ip, responseList)
        for r in responseList:
            response = r
            if response.error_message:
                time = 0
            else:
                time = response.time_elapsed_ms

            self.cam.setPing(response.error_message, time)
            


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
        self.plot.setTitle(cam)
        self.curve = plot.plot()



class Camera():
    """Containes all camera information and ui objects"""
    def __init__(self, threadpool, ip, button, label, grapher):
        self.threadpool = threadpool
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

    def updatePing(self):
        pinger = Pinger(self)
        threadpool.start(pinger)

    # @pyqtSlot()
    def drawMe(self):
        grapher.changeCam(self.ip)

         
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

# Add graph drawer 
widget = graph.findChild(pg.PlotWidget, "plotwidget")
plot = widget.getPlotItem()
grapher = Grapher(plot)

# Create threadpool to update ping values in a separate thread
threadpool = QThreadPool()

# Attach buttons and ping labels to cameras
cams = []
i = 0
for c in camList:
    button = menu.findChild(QtWidgets.QPushButton, "b" + str(i))
    button.setText(c)
    label = menu.findChild(QtWidgets.QLabel, "l" + str(i))
    # Pass threadpool, ip and ui controls
    camera = Camera(threadpool, camList[c], button, label, grapher)
    cams.append(camera)
    # Draw graph of this camera on click
    button.clicked.connect(camera.drawMe)

    i+=1

# Update cam information periodically in separate thread
pingTimer = QTimer()
for cam in cams:
    pingTimer.timeout.connect(cam.updatePing)
pingTimer.start(1000)

# Start Qt app
sys.exit(app.exec())
