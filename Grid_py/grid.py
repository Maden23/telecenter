import sys
from PySide2.QtCore import QUrl, QRunnable
from PySide2.QtWidgets import QApplication
from PySide2.QtQml import QQmlApplicationEngine
from PySide2.QtQuick import QQuickWindow, QQuickItem

import gi
gi.require_version("Gst", "1.0")
from gi.repository import Gst, GObject

import logging

class Runner(QRunnable):
    '''
    QRunnable that runs function 'fn' with arguments 'args'
    '''
    def __init__(self, fn, *args):
        super().__init__()
        self.fn = fn
        self.args = args

    def run(self):
        self.fn(self.args)


class Player:
    def __init__(self, name):
        self.name = name

        # Pipeline must be created before QML engine so that GstGLVideoItem (QML component) 
        # could load correctly from qmlglsink (GStreamer component)
        self._build_pipeline()

        self.engine = QQmlApplicationEngine()
        self.engine.load(QUrl('ui/player.qml'))

        
        self._bind_widget_to_pipeline()
        
    
    def _build_pipeline(self):
        # Create elements
        self.pipeline = Gst.Pipeline.new()
        self.src = Gst.ElementFactory.make("videotestsrc")
        self.glupload = Gst.ElementFactory.make("glupload")
        self.sink = Gst.ElementFactory.make("qmlglsink")

        # Add elements to pipeline
        self.pipeline.add(self.src)
        self.pipeline.add(self.glupload)
        self.pipeline.add(self.sink)

        # Link elements
        self.src.link(self.glupload)
        self.glupload.link(self.sink)
        
        # Handle bus messages
        bus = self.pipeline.get_bus()
        bus.set_sync_handler(self._on_message)

    def _on_message(self, bus, message):
        self.logger.debug("Player %s: %s", self.name, message.type)
        t = message.type
        if t == Gst.MessageType.EOS:
            # self.pipeline.set_state(Gst.State.NULL)
            self.pipeline.unref()
        elif t == Gst.MessageType.ERROR:
            # self.pipeline.set_state(Gst.State.NULL)
            err, debug = message.parse_error()
            self.logger.error("Player %s: %s %s", self.name, err, debug)
            self.pipeline.unref()
        return Gst.BusSyncReply.PASS
       

    def _start_pipeline(self):
        self.pipeline.set_state(Gst.State.PLAYING)

    def _bind_widget_to_pipeline(self):
        # Bind sink to player widget
        try:
            rootObject = self.engine.rootObjects()[0]
        except IndexError:
            logging.critical("Player %s: Could not find root object. Maybe QML file did not load properly", self.name)
            sys.exit(1)
        videoItem = rootObject.findChild(QQuickItem, 'videoItem')
        if not videoItem:
           logging.critical("Player %s: Could not found \"videoItem\" element in QML file", self.name)
        self.sink.set_property('widget', videoItem)

        # Schedule pipeline start
        starter = Runner(self._start_pipeline)
        rootObject.scheduleRenderJob(starter, QQuickWindow.BeforeSynchronizingStage)


if __name__ == '__main__':

    Gst.init()

    app = QApplication(sys.argv)

    player = Player('first')

    app.exec_()
    sys.exit()

# player = new QMediaPlayer;
# player->setMedia(QUrl("gst-pipeline: videotestsrc ! autovideosink"));
# player->play();
