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
	pipeline = gst_element_factory_make ("playbin", "play");
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, NULL, NULL);
	gst_object_unref (bus);
}

Player::~Player() 
{
	
}

void Player::playStream(string cam_id)
{
	/* Update label */
	gtk_button_set_label(GTK_BUTTON(camLabel), cam_id.c_str());

	gst_element_set_state (pipeline, GST_STATE_READY);
	cout << "Playing " << config->getCamUri(cam_id).c_str() << endl;
	g_object_set (G_OBJECT (pipeline), "uri", config->getCamUri(cam_id).c_str(), NULL);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);

}

void Player::stopStream()
{
	cout << "Paused";
 	gst_element_set_state (pipeline, GST_STATE_PAUSED);
}

