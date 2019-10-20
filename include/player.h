#include <gtk/gtk.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#endif

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include "config.h"

class Player
{
public:
	Player(GtkWidget* window, GtkWidget* videoWindow, GtkWidget* camLabel, Config *config);
	~Player();
	void playStream(string cam_id);


private:
	Config *config;

	GtkWidget *window, *videoWindow, *camLabel;
	GstElement *pipeline, *src, *sink;
	GstBus *bus;

	

};

/* Not in class bacause of g_signal_connect */
// video window handle
static guintptr videoWindowHandle = 0;
static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, gpointer user_data);
static void videoWidgetRealize_cb (GtkWidget *widget, GtkWidget *videoWindow);


