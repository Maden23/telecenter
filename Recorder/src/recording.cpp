#include "recording.h"

Recording::Recording(string uri, string folder, string camName, long videoTimeLimit)
{
	this->uri = uri;
	this->folder = folder;
    this->camName = camName;
    this->videoTimeLimit = videoTimeLimit;

    /* Create file name */
    if (fileNamePattern == "")
    {
        char datetime[18];
        time_t t = time(nullptr);
        strftime(datetime, sizeof(datetime), "%d-%m-%Y_%H:%M", localtime(&t));
        fileNamePattern = camName + "_" + string(datetime)+ "_%02d.mp4";
    }

}

Recording::~Recording()
{
    gst_object_unref(pipeline);
}

bool Recording::start()
{

	status = RUNNING;

	/* Initiallizing Gstreamer*/
	gst_init(nullptr, nullptr);

	if (!buildPipeline())
	{
		return false;
	}

	/* Get bus to handle messages*/
	GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, this, NULL);
	g_object_unref (bus);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	return true;
}

bool Recording::stop()
{
    if (status == STOPPING) return false;

	// send EOS
	status = STOPPING;
	gst_element_send_event(src, gst_event_new_eos());
    cerr << camName << ": Sent EOS to stream" << endl;

	gst_element_send_event(src, gst_event_new_eos());

	return true;
}


bool Recording::buildPipeline()
{
    /* Creating elements */
    src = gst_element_factory_make("rtspsrc", ("src " + camName).c_str());
    depay = gst_element_factory_make("rtph264depay", ("depay " + camName).c_str());
    parse = gst_element_factory_make("h264parse", ("parse " + camName).c_str());
    sink = gst_element_factory_make("splitmuxsink", ("sink " + camName).c_str());

    pipeline = gst_pipeline_new(("pipeline " + camName).c_str());

    if (!pipeline || !src || !depay || !parse ||  !sink )
    {
        cerr << camName << "Not all pipeline elements could be created" << endl << endl;
        return false;
    } 

    gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, sink, NULL);

    /* Link pipeline without source */
    if (!gst_element_link_many(depay, parse, sink, NULL))
    {
        cerr << "Stream linking error" << endl << endl;
    }
    // Set properties for stream source
    g_object_set (src,
            "location", uri.c_str(),
            "protocols", (1 << 2), // TCP,
            NULL);
    // Set properties for streaming time limit
    g_object_set(sink,
                 "location", (folder+fileNamePattern).c_str(),
                 "max-size-time", videoTimeLimit,
                 "muxer-factory", "mpegtsmux",
                 NULL);

    // Signal to handle new source pad (for delayed source linking)
    g_signal_connect(src, "pad-added", G_CALLBACK(pad_added_handler), this);

    return true;
}

void Recording::pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording)
{
	GstPad *sink_pad = gst_element_get_static_pad (recording->depay, "sink");
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
        cerr << recording->camName << ": Wrong stream prefix" << endl << endl;;
		return;
	}

	/* Attempt the link */
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
        cerr << recording->camName << ": Source pad link failed" << endl << endl;
	}
	else 
	{       
        cerr << recording->camName << ": Linked source pad" << endl << endl;
	}

}


// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
GstBusSyncReply Recording::busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording)
{
	switch (GST_MESSAGE_TYPE(message))
	{
		case GST_MESSAGE_ERROR:
		{
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
            cerr << "Recording " << recording->camName << endl
				 << "Bus: " << err->message  << endl 
				 << debug << endl << endl;
            recording->status = STOPPED;

			g_error_free (err);
     		g_free (debug);
            gst_object_unref(recording->pipeline);

			break;
		}
		case GST_MESSAGE_ELEMENT:
		{
			const GstStructure *s = gst_message_get_structure (message);
			const gchar *name = gst_structure_get_name (s);
            // Information from splitmuxsink that file is closed
            // https://lists.freedesktop.org/archives/gstreamer-commits/2015-October/089738.html
            if (0 == strcmp(name, "splitmuxsink-fragment-closed"))
            {
                const gchar *filename = gst_structure_get_string(s, "location");
                cout << "New file: " << filename << endl << endl;
                recording->files.push_back(filename);
            }
			break;
		}
		case GST_MESSAGE_EOS:
		{
            cerr << recording->camName << ": EOS in bus" << endl << endl;
            // This recording is finished if stop was initiated by user (= status was set to STOPPING), not by splitmuxsink
            if (recording->status == STOPPING)
                recording->status = STOPPED;
			break;
		}
		default:
			const GstStructure *s = gst_message_get_structure (message);
			const gchar *name = gst_structure_get_name (s);
			break;
	}
	return GST_BUS_PASS;
}
