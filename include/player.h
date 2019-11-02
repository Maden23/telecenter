#include <gtk/gtk.h>

// #define ON_JETSON

#if defined (GDK_WINDOWING_X11)  // for GDK_WINDOW_XID
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WIN32
#include &lt;gdk/gdkwin32.h&gt;  // for GDK_WINDOW_HWND
#endif

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include "config.h"

struct PadData
{
	GstElement *src;
	GstElement *depay;
};

class Player
{
public:
	Player(GtkWidget* videoWindow, GtkWidget* camLabel, Config *config);
	~Player();
	void playStream(string cam_id);
	void stopStream();
	GstElement *pipeline, *src, *depay, *parse, *dec, *sink;


private:
	Config *config;

	GtkWidget *videoWindow, *camLabel;
	GstBus *bus;

	void buildPipeline();

};

/* Not in class bacause of g_signal_connect */
// video window handle
static guintptr videoWindowHandle = 0;

static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, gpointer user_data)
{
	if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
	{
		GError *err;
		gchar *debug;

		gst_message_parse_error (message, &err, &debug);
		cerr << err->message << endl;
	}

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

static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player)
{
	GstPad *sink_pad = gst_element_get_static_pad (player->depay, "sink");
	GstPadLinkReturn ret;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	/* If our converter is already linked, we have nothing to do here */
  	if (gst_pad_is_linked (sink_pad))
  	{
    	return;
  	}
  	/* Check the new pad's type */
	new_pad_caps = gst_pad_get_current_caps (new_pad);
	new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
	new_pad_type = gst_structure_get_name (new_pad_struct);
	if (!g_str_has_prefix (new_pad_type, "application/x-rtp"))
	{
		cerr << "Wrong prefix" << endl;
		return;
	}

	/* Attempt the link */
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
		cerr << "Source link failed" << endl;
	}
	else 
		cerr << "Linked source" << endl;

}
