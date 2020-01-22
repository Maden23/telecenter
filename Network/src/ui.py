from PyQt5 import QtWidgets, uic
import pyqtgraph as pg
import sys


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

app = QtWidgets.QApplication([])

menu = UIMenu()
menu.resize(540, 360)

graph = UIGraph()
graph.resize(540, 360)

sys.exit(app.exec())
