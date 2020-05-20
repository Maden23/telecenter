#include "player.h"

// Player::videoWindowHandle = 0;

Player::Player (GtkWidget* videoWindow, string platform, string uri, string camName)
{
	this->videoWindow = videoWindow;
    this->platform = platform;
    this->uri = uri;
    this->camName = camName;
    this->playing = false;
	init();
}

Player::Player(GtkWidget* videoWindow, string platform)
{
	this->videoWindow = videoWindow;
    this->platform = platform;
	this->playing = false;

	init();
}

Player::~Player() 
{

}

void Player::init()
{
	/* Initiallizing Gstreamer*/
//    setenv("GST_DEBUG", "*:2", 1);
	gst_init(nullptr, nullptr);

	/* Prepare videoWindow for rendering*/
	gtk_widget_set_double_buffered (videoWindow, FALSE);
	// connect signal for rendering default background on startup
	g_signal_connect (videoWindow, "draw", G_CALLBACK(videoWidgetDraw_cb), NULL);
	// connect signal for video rendering
	g_signal_connect (videoWindow, "realize", G_CALLBACK (videoWidgetRealize_cb), this);
	// realize window now so that the video window gets created and we can
	// obtain its XID/HWND before the pipeline is started up and the videosink
	// asks for the XID/HWND of the window to render onto
	gtk_widget_realize (videoWindow);
	// we should have the XID/HWND now
	g_assert (videoWindowHandle != 0);

	/* Build pipeline */
	buildPipeline();

	/* Get bus to handle messages*/
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, this, NULL);
	gst_object_unref (bus);

}

void Player::buildPipeline()
{
    src = gst_element_factory_make("rtspsrc", ("src_" + camName).c_str());
    depay = gst_element_factory_make("rtph264depay", ("depay_" + camName).c_str());
    parse = gst_element_factory_make("h264parse", ("parse_" + camName).c_str());

    pipeline = gst_pipeline_new(("pipeline_" + camName).c_str());

    if (platform == "jetson")
	{
        dec = gst_element_factory_make("omxh264dec", ("dec_" + camName).c_str());
        sink = gst_element_factory_make("nveglglessink", ("sink_" + camName).c_str());

		if (!pipeline ||  !src || !depay || !parse || !dec || !sink)
		{
			cerr << "Not all pipeline elements could be created" << endl << endl;
		} 

		gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, dec, sink, NULL);

		if (!gst_element_link_many(depay, parse, dec, sink, NULL))
			cerr << "Pipeline linking error" << endl << endl;
	}

    else if (platform == "other")
	{
        dec = gst_element_factory_make("avdec_h264", ("dec_" + camName).c_str());
        scale = gst_element_factory_make("videoscale", ("scale_" + camName).c_str());
        sink = gst_element_factory_make("glimagesink", ("sink_" + camName).c_str());
		if (!pipeline ||  !src || !depay || !parse || !dec || !scale || !sink)
		{
			cerr << "Not all pipeline elements could be created" << endl << endl;
		} 

		gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, dec, scale, sink, NULL);

		if (!gst_element_link_many(depay, parse, dec, scale, sink, NULL))
			cerr << "Pipeline linking error" << endl << endl;
	}
	else
        cout << "Platform not specified" << endl << endl;
	
	/* Set latency */
	g_object_set (src, "latency", 0, NULL);
	g_object_set (src, "tcp-timeout", 200000, NULL);
	g_object_set (src, "timeout", 200000 , NULL);



//    int width = gtk_widget_get_allocated_width (videoWindow);
//    int height = gtk_widget_get_allocated_height (videoWindow);
//    g_object_set(scale,
//                 "width", width,
//                 "height", height,
//                 NULL);

	/* Signal to handle new source pad*/
	g_signal_connect(src, "pad-added", G_CALLBACK(pad_added_handler), this);

}

void Player::setCam(string camName, string uri)
{
    this->camName = camName;
    this->uri = uri;
    playStream();
}


void Player::playStream()
{
//    this->uri = uri;
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_element_unlink(src, depay);

    cout << "Playing " << uri << endl << endl;
    g_object_set (src, "location", (uri+"?width=640&height=480").c_str(), NULL);

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    playing = true;
}

void Player::stopStream()
{
    gst_element_set_state (pipeline, GST_STATE_NULL);
    playing = false;
}

GstBusSyncReply Player::busSyncHandler (GstBus *bus, GstMessage *message, Player *player)
{
	switch (GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_ERROR:
		{
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);

            cerr << "Player "  << player->camName << endl;
			cerr << err->message << endl;
			cerr << debug << endl << endl;
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

			cerr << "Player bus: EOS" << endl << endl;
			break;
		}
		default:
			break;
	}

	// ignore anything but 'prepare-window-handle' element messages
	if (!gst_is_video_overlay_prepare_window_handle_message (message))
		return GST_BUS_PASS;
	if (player->videoWindowHandle != 0)
	{
	  GstVideoOverlay *overlay;
	  // GST_MESSAGE_SRC (message) will be the video sink element
	  overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
	  gst_video_overlay_set_window_handle (overlay, player->videoWindowHandle);
	  // cout << "Set video handle" << endl;
	}
	else
	{
	  g_warning ("Should have obtained videoWindowHandle by now!");
	}
    return GST_BUS_PASS;
}

void Player::videoWidgetRealize_cb (GtkWidget *widget, Player *player)
{
	#ifdef GDK_WINDOWING_X11
	  {
		gulong xid = GDK_WINDOW_XID (gtk_widget_get_window (widget));
		player->videoWindowHandle = xid;
	  }
	#endif

	 #ifdef GDK_WINDOWING_WIN32
	{
	 HWND wnd = GDK_WINDOW_HWND (gtk_widget_get_window (widget));
	 player->videoWindowHandle = (guintptr) wnd;
	}
	#endif
}

gboolean Player::videoWidgetDraw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	guint width, height;
	GdkRGBA color;
//	GtkStyleContext *context;

//	context = gtk_widget_get_style_context (widget);

	width = gtk_widget_get_allocated_width (widget);
	height = gtk_widget_get_allocated_height (widget);

//	gtk_render_background (context, cr, 0, 0, width, height);

	cairo_rectangle (cr, 0, 0, width, height);

//	gtk_style_context_get_color (context,
//	                           gtk_style_context_get_state (context),
//	                           &color);
    gdk_rgba_parse(&color, "#444444");
	gdk_cairo_set_source_rgba (cr, &color);

	cairo_fill (cr);
}


void Player::pad_added_handler (GstElement * src, GstPad * new_pad, Player *player)
{
//    cerr << "Linking " << player->camName << endl;
	GstPad *sink_pad = gst_element_get_static_pad (player->depay, "sink");
	GstPadLinkReturn ret;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	/* If our converter is already linked, we have nothing to do here */
	if (gst_pad_is_linked (sink_pad))
	{
        cerr << player->camName << ": Pad is already linked" << endl << endl;
		return;
	}
	/* Check the new pad's type */
	new_pad_caps = gst_pad_get_current_caps (new_pad);
	new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
	new_pad_type = gst_structure_get_name (new_pad_struct);
	if (!g_str_has_prefix (new_pad_type, "application/x-rtp"))
	{
        cerr << player->camName << ": Wrong stream prefix" << endl;
		return;
	}

	/* Attempt the link */
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
        cerr << player->camName << ": Source link failed" << endl << endl;
	}
    else
    {
        cerr << player->camName << ": Source linked" << endl << endl;
    }

}

