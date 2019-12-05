#include "player.h"

// Player::videoWindowHandle = 0;

Player::Player (GtkWidget* videoWindow, GtkWidget* camLabel, Config *config)
{
	this->videoWindow = videoWindow;
	this->camLabel = camLabel;
	this->config = config;
	
	/* Initiallizing Gstreamer*/
	gst_init(nullptr, nullptr);

	/* Prepare videoWindow for rendering*/
	gtk_widget_set_double_buffered (videoWindow, FALSE);
	g_signal_connect (videoWindow, "realize", G_CALLBACK (videoWidgetRealize_cb), NULL);
	// realize window now so that the video window gets created and we can
	// obtain its XID/HWND before the pipeline is started up and the videosink
	// asks for the XID/HWND of the window to render onto
	gtk_widget_realize (videoWindow);
	// we should have the XID/HWND now
	g_assert (videoWindowHandle != 0);

	/* Build pipeline */
	buildPipeline();

	/* Initialize clock for freeze check */
	clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));

	/* Get bus to handle messages*/
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, this, NULL);
	gst_object_unref (bus);
}

Player::~Player() 
{

}

void Player::buildPipeline()
{
	src = gst_element_factory_make("rtspsrc", "src");
	testsrc = gst_element_factory_make("videotestsrc", "testsrc");
	depay = gst_element_factory_make("rtph264depay", "depay");  
	parse = gst_element_factory_make("h264parse", "parse");

	pipeline = gst_pipeline_new("pipeline");

	#ifdef ON_JETSON
		cout << "JETSON" << endl;
		dec = gst_element_factory_make("omxh264dec", "dec");
		sink = gst_element_factory_make("nveglglessink", "sink");

		if (!pipeline ||  !src || !testsrc || !depay || !parse || !dec || !sink)
		{
			cerr << "Not all pipeline elements could be created" << endl;
		} 

		gst_bin_add_many(GST_BIN(pipeline), src, testsrc, depay, parse, dec, sink, NULL);

		if (!gst_element_link_many(depay, parse, dec, sink, NULL))
			cerr << "Pipeline linking error" << endl;

	#else
		cout << "Not JETSON" << endl;
		dec = gst_element_factory_make("avdec_h264", "dec");
		scale = gst_element_factory_make("videoscale", "scale");
		sink = gst_element_factory_make("autovideosink", "sink");
		if (!pipeline ||  !src || !testsrc || !depay || !parse || !dec || !scale || !sink)
		{
			cerr << "Not all pipeline elements could be created" << endl;
		} 

		gst_bin_add_many(GST_BIN(pipeline), src, testsrc, depay, parse, dec, scale, sink, NULL);

		if (!gst_element_link_many(depay, parse, dec, scale, sink, NULL))
			cerr << "Pipeline linking error" << endl;
	#endif

	
	/* Set latency */
	g_object_set (src, "latency", 0, NULL);
	g_object_set (src, "tcp-timeout", 200000, NULL);
	g_object_set (src, "timeout", 200000 , NULL);


	/* Signal to handle new source pad*/
	g_signal_connect(src, "pad-added", G_CALLBACK(pad_added_handler), this);

}

void Player::playStream(string cam_id)
{
	/* Update label */
	// gtk_button_set_label(GTK_BUTTON(camLabel), cam_id.c_str());
	gtk_label_set_text(GTK_LABEL(camLabel), cam_id.c_str());

	gst_element_set_state (pipeline, GST_STATE_READY);

	cout << "Playing " << config->getCamUri(cam_id).c_str() << endl;
	// g_object_set (G_OBJECT (pipeline), "uri", config->getCamUri(cam_id).c_str(), NULL);
	g_object_set (src, "location", config->getCamUri(cam_id).c_str(), NULL);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);

}

void Player::stopStream()
{
	cout << "Paused";
	gst_element_set_state (pipeline, GST_STATE_PAUSED);
}

bool Player::showTest()
{
	cout << "Showing test" << endl;
	gst_element_set_state(pipeline, GST_STATE_READY);
	gst_element_unlink(dec, scale);
	
	// gst_element_set_state(src, GST_STATE_READY);
	// gst_element_set_state(src, GST_STATE_PLAYING);

	if (!gst_element_link(testsrc, scale))
	{
		cerr << "Failed to link testsrc" << endl;
		return false;
	}
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	return true;

}

/* Not in class bacause of g_signal_connect */

static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player)
{
	switch (GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_ERROR:
		{
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
			// cerr << "Bus: " << err->message << endl;
			break;
		}
		case GST_MESSAGE_ELEMENT:
		{
			const GstStructure *s = gst_message_get_structure (message);
			const gchar *name = gst_structure_get_name (s);
			break;
		}
		case GST_MESSAGE_EOS:
		{
			cerr << "Bus: EOS" << endl;
			break;
		}
		default:
			break;
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
		cerr << "Wrong stream prefix" << endl;
		return;
	}

	/* Attempt the link */
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
		cerr << "Source link failed" << endl;
	}
	else 
	{       
		cerr << "Linked source" << endl;
		// Add data probe to remember the last received video buffer time
		player->lastBufferTime = -1; 
		gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_BUFFER, data_probe, player, NULL);
		g_timeout_add (500, freeze_check, player); // run freeze check every x milliseconds

	}
}

GstPadProbeReturn data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	Player* player = (Player*) user_data;
	player->lastBufferTime = gst_clock_get_time(player->clock);
	cout << player->lastBufferTime << endl;

	return GST_PAD_PROBE_OK;
}


gboolean freeze_check(gpointer user_data)
{	
	Player* player = (Player*) user_data;

	if (player->lastBufferTime == -1) return true; // No need to check if no stream is playing

	// current time
	GstClockTime current = gst_clock_get_time(player->clock);

	// check difference between current time and last time data was received
	GstClockTimeDiff diff = GST_CLOCK_DIFF(player->lastBufferTime, current);
	int timeout = player->config->getParamInt("videoTimeout") * 1000000; // from ms to ns

	if (diff > timeout)
	{
		// cout << "Showing testsrc" << endl;
		if (!player->showTest())
			cout << "Failed to show test." << endl;
		else 
			return false; // Freeze caught, test is showing - stopping regular check
	}
	return true; // to contionue checking regularly
}
