#include "player.h"

// Player::videoWindowHandle = 0;

Player::Player (GtkWidget* window, GtkWidget* videoWindow, GtkWidget* camLabel, Config *config)
{
	this->window = window;
	this->videoWindow = videoWindow;
	this->camLabel = camLabel;
	this->config = config;
	

	gst_init(nullptr, nullptr);

	// g_signal_connect (videoWindow, "realize", G_CALLBACK (videoWidgetRealize_cb), NULL);
	g_signal_connect (videoWindow, "realize", G_CALLBACK (videoWidgetRealize_cb), videoWindow);

}

Player::~Player() 
{
	
}

void Player::playStream(string cam_id)
{
	/* Update label */

	/* Build pipeline */
	pipeline = gst_element_factory_make ("playbin", "play");
	g_object_set (G_OBJECT (pipeline), "uri", config->getCamUri(cam_id).c_str(), NULL);
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_element_set_state (pipeline, GST_STATE_READY);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	// set up sync handler for setting the xid once the pipeline is started
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, NULL, NULL);
	gst_object_unref (bus);

	gtk_widget_show_all(window);
	gtk_widget_realize (videoWindow);
}


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
    }
  else
    {
      g_warning ("Should have obtained videoWindowHandle by now!");
    }
}

static void videoWidgetRealize_cb (GtkWidget *widget, GtkWidget *videoWindow)
{
#ifdef GDK_WINDOWING_X11
  {
    gulong xid = GDK_WINDOW_XID (gtk_widget_get_window (videoWindow));
    videoWindowHandle = xid;
  }
#endif
}

