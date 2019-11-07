#include "player.h"

// Player::videoWindowHandle = 0;

Player::Player (GtkWidget* videoWindow, GtkWidget* camLabel, Config *config)
{
	this->videoWindow = videoWindow;
	this->camLabel = camLabel;
	this->config = config;
	

	gst_init(nullptr, nullptr);

	// g_signal_connect (videoWindow, "realize", G_CALLBACK (videoWidgetRealize_cb), NULL);
	gtk_widget_set_double_buffered (videoWindow, FALSE);
	g_signal_connect (videoWindow, "realize", G_CALLBACK (videoWidgetRealize_cb), NULL);

	// realize window now so that the video window gets created and we can
    // obtain its XID/HWND before the pipeline is started up and the videosink
    // asks for the XID/HWND of the window to render onto
	gtk_widget_realize (videoWindow);
	// we should have the XID/HWND now
    g_assert (videoWindowHandle != 0);

	/* Build pipeline */
	// pipeline = gst_element_factory_make ("playbin", "play");
	buildPipeline();
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, NULL, NULL);
	gst_object_unref (bus);
}

Player::~Player() 
{

}

void Player::buildPipeline()
{
	src = gst_element_factory_make("rtspsrc", "src");
	depay = gst_element_factory_make("rtph264depay", "depay");	
	parse = gst_element_factory_make("h264parse", "parse");

	pipeline = gst_pipeline_new("pipeline");

	#ifdef ON_JETSON
		cout << "JETSON" << endl;
		dec = gst_element_factory_make("omxh264dec", "dec");
		sink = gst_element_factory_make("nveglglessink", "sink");

		if (!pipeline ||  !src || !depay || !parse || !dec || !sink)
		{
			cerr << "Not all pipeline elements could be created" << endl;
		} 

		gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, dec, sink, NULL);

		if (!gst_element_link_many(depay, parse, dec, sink, NULL))
			cerr << "Pipeline linking error" << endl;

	#else
		cout << "Not JETSON" << endl;
		dec = gst_element_factory_make("avdec_h264", "dec");
		scale = gst_element_factory_make("videoscale", "scale");
		sink = gst_element_factory_make("autovideosink", "sink");
		if (!pipeline ||  !src || !depay || !parse || !dec || !scale || !sink)
		{
			cerr << "Not all pipeline elements could be created" << endl;
		} 

		gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, dec, scale, sink, NULL);

		if (!gst_element_link_many(depay, parse, dec, scale, sink, NULL))
			cerr << "Pipeline linking error" << endl;
	#endif

	
	/* Set latency */
	g_object_set (src, "latency", 100, NULL);

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

