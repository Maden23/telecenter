#include "player.h"

// Player::videoWindowHandle = 0;

Player::Player (GtkWidget* videoWindow, GtkWidget* camLabel, Config *config)
{
	this->videoWindow = videoWindow;
	this->camLabel = camLabel;
	this->config = config;
	
	/* Starting Snowmix in a separate thread */
	snowmixPid = startSnowmix();

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

	/* Build pipelines */
	buildInputPipeline();
	buildOutputPipeline();

	/* Get bus to handle messages */
	out_bus = gst_pipeline_get_bus (GST_PIPELINE (out_pipeline));

	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (out_bus, (GstBusSyncHandler) busSyncHandler, this, NULL);
	gst_object_unref (out_bus);

	gst_element_set_state (out_pipeline, GST_STATE_PLAYING);
}

Player::~Player() 
{	
	stopStream();
	stopSnowmix();
}

void Player::buildInputPipeline()
{
	in_src = gst_element_factory_make("rtspsrc", "in_src");
	in_depay = gst_element_factory_make("rtph264depay", "in_depay");  
	in_parse = gst_element_factory_make("h264parse", "in_parse");

	in_pipeline = gst_pipeline_new("in_pipeline");

	#ifdef ON_JETSON
		cout << "JETSON" << endl;
		in_dec = gst_element_factory_make("omxh264dec", "in_dec");
		
	#else
		cout << "Not JETSON" << endl;
		in_dec = gst_element_factory_make("avdec_h264", "in_dec");
		
	#endif

	in_convert = gst_element_factory_make("videoconvert", "in_convert");
	in_scale = gst_element_factory_make("videoscale", "in_scale");
	in_caps = gst_element_factory_make("capsfilter", "in_caps");
	in_sink = gst_element_factory_make("shmsink", "in_sink");

	if (!in_pipeline ||  !in_src || !in_depay || !in_parse || !in_dec || !in_convert || !in_scale || !in_caps || !in_sink)
	{
		cerr << "Not all input pipeline elements could be created" << endl;
	} 

	gst_bin_add_many(GST_BIN(in_pipeline), in_src, in_depay, in_parse, in_dec, in_convert, in_scale, in_caps, in_sink, NULL);

	// string scaps =  "video/x-raw, format=BGRA, pixel-aspect-ratio=1/1, interlace-mode=progressive, framerate=25/1, width="
	//  + config->getParam("windowWidth") + ", height=" + config->getParam("windowHeight");
	// g_object_set(in_caps, "caps", caps.c_str(), NULL);
	// cout << scaps << endl;
	
	/* Linking the rest of the pipeline */
	if (!gst_element_link_many(in_depay, in_parse, in_dec, in_convert, in_scale, NULL))
		cerr << "Input pipeline linking error" << endl;

	/* Linking to sink with caps*/
	GstCaps *caps;
	caps = gst_caps_new_simple ("video/x-raw",
	      "format", G_TYPE_STRING, "BGRA",
	      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
	      "interlace-mode", G_TYPE_STRING, "progressive",
	      "framerate", GST_TYPE_FRACTION, 25, 1,
	      "width", G_TYPE_INT, stoi(config->getParam("windowWidth")),
	      "height", G_TYPE_INT, stoi(config->getParam("windowHeight")),
	      NULL);
	// g_object_set(in_caps, "caps", caps, NULL);
	gboolean link_ok = gst_element_link_filtered (in_scale, in_sink, caps);
	gst_caps_unref(caps);
	if (!link_ok)
		cerr << "Input pipeline: caps linking error" << endl;

	/* Set socket properties */
	int size = stoi(config->getParam("windowWidth")) * stoi(config->getParam("windowHeight")) * 4 * 20;
	g_object_set(in_sink, "socket-path", "/tmp/feed1-control-pipe", "shm-size", size, NULL);
	
	/* Set latency */
	g_object_set (in_src, "latency", 0, NULL);
	g_object_set (in_src, "tcp-timeout", 200000, NULL);
	g_object_set (in_src, "timeout", 200000 , NULL);


	/* Signal to handle new source pad*/
	g_signal_connect(in_src, "pad-added", G_CALLBACK(pad_added_handler), this);
}

void Player::buildOutputPipeline()
{
	out_pipeline = gst_pipeline_new("out_pipeline");
	out_src = gst_element_factory_make("shmsrc", "in_src");
	out_queue = gst_element_factory_make("queue", "out_queue");
	out_scale = gst_element_factory_make("videoscale", "out_scale");
	out_convert = gst_element_factory_make("videoconvert", "out_convert");

	#ifdef ON_JETSON
		out_sink = gst_element_factory_make("nveglglessink", "out_sink");
	#else
		out_sink = gst_element_factory_make("autovideosink", "out_sink");
	#endif

	if (!out_pipeline || !out_src || !out_queue || !out_scale || !out_convert || !out_sink)
		cerr << "Not all output pipeline elements could be created." << endl;

	gst_bin_add_many(GST_BIN(out_pipeline), out_src, out_queue, out_scale, out_convert, out_sink, NULL);

	/* Set properties  */
	g_object_set(out_src, 
		"socket-path", "/tmp/mixer1",
		"do-timestamp", true,
		"is-live", true,
		NULL);
	g_object_set(out_queue,
		"leaky", 2,
		"max-size-buffers", 2,
		NULL);

	/* Lining */
	if (!gst_element_link(out_src, out_queue))
		cerr << "Output pipeline: source linking error" << endl;

	GstCaps *caps;
	caps = gst_caps_new_simple ("video/x-raw",
	      "format", G_TYPE_STRING, "BGRA",
	      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
	      "interlace-mode", G_TYPE_STRING, "progressive",
	      "framerate", GST_TYPE_FRACTION, 25, 1,
	      "width", G_TYPE_INT, stoi(config->getParam("windowWidth")),
	      "height", G_TYPE_INT, stoi(config->getParam("windowHeight")),
	      NULL);
	if (!gst_element_link_filtered(out_queue, out_scale, caps))
		cerr << "Output pipeline: queue + scale linking error" << endl;
	gst_caps_unref(caps);

	caps = gst_caps_new_simple ("video/x-raw",
	      "format", G_TYPE_STRING, "BGRA",
	      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
	      "interlace-mode", G_TYPE_STRING, "progressive",
	      "width", G_TYPE_INT, stoi(config->getParam("windowWidth")),
	      "height", G_TYPE_INT, stoi(config->getParam("windowHeight")),
	      NULL);
	if (!gst_element_link_filtered(out_scale, out_convert, caps))
		cerr << "Output pipeline: scale + convert linking error" << endl;
	gst_caps_unref(caps);

	if (!gst_element_link(out_convert, out_sink))
		cerr << "Output pipeline: sink linking error" << endl;
}

pid_t Player::startSnowmix()
{
	pid_t pid = fork();

	if (pid == -1)
	{
		cerr << "Failed to fork for Snowmix start" << endl;
		perror("fork");
		return -1;
	}
	else if (pid == 0)
	{
		cout << "Starting snowmix." << endl;
		system("snowmix src/snowmix/player.ini");
		exit(0);
	}
	else
	{
		return pid;
	}
}

void Player::stopSnowmix()
{
	kill(snowmixPid, SIGINT);
    waitpid(snowmixPid, nullptr, 0);
    cout << "Snowmix in player stopped." << endl;
}

void Player::playStream(string cam_id)
{
	/* Update label */
	// gtk_button_set_label(GTK_BUTTON(camLabel), cam_id.c_str());
	gtk_label_set_text(GTK_LABEL(camLabel), cam_id.c_str());

	gst_element_set_state (in_pipeline, GST_STATE_READY);

	cout << "Playing " << config->getCamUri(cam_id).c_str() << endl;
	g_object_set (in_src, "location", config->getCamUri(cam_id).c_str(), NULL);

	gst_element_set_state (in_pipeline, GST_STATE_PLAYING);
}

void Player::stopStream()
{
    gst_element_set_state (in_pipeline, GST_STATE_NULL);
    gst_object_unref (in_pipeline);
	cout << "Streaming in player stopped" << endl;
}

/* Not in class bacause g_signal_connect does not accept in-class functions */

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
			// const GstStructure *s = gst_message_get_structure (message);
			// const gchar *name = gst_structure_get_name (s);
			//cerr << "Bus: " << name << endl;
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
	  // GST_MESSAGE_in_SRC (message) will be the video sink element
	  overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
	  gst_video_overlay_set_window_handle (overlay, videoWindowHandle);
	  // cout << "Set video handle" << endl;
	}
	else
	{
	  g_warning ("Should have obtained videoWindowHandle by now!");
	}
	return GST_BUS_PASS;
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

static void pad_added_handler (GstElement * in_src, GstPad * new_pad, Player *player)
{
	GstPad *sink_pad = gst_element_get_static_pad (player->in_depay, "sink");
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
	{       
		cerr << "Linked source" << endl;
	}
}

