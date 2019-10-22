#include <gtk/gtk.h>

#if defined (GDK_WINDOWING_X11)  // for GDK_WINDOW_XID
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WIN32
#include &lt;gdk/gdkwin32.h&gt;  // for GDK_WINDOW_HWND
#endif

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include "config.h"

class Player
{
public:
	Player(GtkWidget* videoWindow, GtkWidget* camLabel, Config *config);
	~Player();
	void playStream(string cam_id);
	void stopStream();


private:
	Config *config;

	GtkWidget *videoWindow, *camLabel;
	GstElement *pipeline, *src, *depay, *parse, *dec, *sink;
	GstBus *bus;

	void buildPipeline();

};

/* Not in class bacause of g_signal_connect */
// video window handle
static guintptr videoWindowHandle = 0;

static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, gpointer user_data)
{
  // ignore anything but 'prepare-window-handle' element messages
  if (!gst_is_video_overlay_prepare_window_handle_message (message))
    return GST_BUS_PASS;
  if (videoWindowHandle != 0)
    {
      GstVideoOverlay *overlay;
      // GST_MESSAGE_SRC (message) will be the video sink element
      overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
      gst_video_overlay_set_window_handle (overlay, videoWindowHandle);
      // cout << "Set video handle" << endl;
    }
  else
    {
      g_warning ("Should have obtained videoWindowHandle by now!");
    }
}

static void videoWidgetRealize_cb (GtkWidget *widget,gpointer *data)
{
	#ifdef GDK_WINDOWING_X11
	  {
	    gulong xid = GDK_WINDOW_XID (gtk_widget_get_window (widget));
	    videoWindowHandle = xid;
	  }
	#endif

	 #ifdef GDK_WINDOWING_WIN32
	{
	 HWND wnd = GDK_WINDOW_HWND (gtk_widget_get_window (widget));
	 videoWindowHandle = (guintptr) wnd;
	}
	#endif
}


