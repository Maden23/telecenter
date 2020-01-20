#include "recording.h"

Recording::Recording(string uri, string folder, long timeout)
{
	this->uri = uri;
	this->folder = folder;
	this->timeout = timeout;
}

Recording::~Recording()
{
	gst_element_set_state(pipeline, GST_STATE_NULL);
	g_source_remove(freeze_check_id);
	gst_object_unref(clock);
	gst_object_unref(pipeline);
}

bool Recording::start()
{
	status = RUNNING;

	/* Create file name */
	string stream = uri.substr(uri.find("@") + 1);
    char datetime[18];
    time_t t = time(nullptr);
    strftime(datetime, sizeof(datetime), "%d-%m-%Y_%H:%M", localtime(&t));
    fileName = stream + "_" + string(datetime); 

	/* Initiallize Gstreamer*/
	gst_init(nullptr, nullptr);

	if (!buildPipeline())
	{
		return false;
	}

	/* Initialize clock for freeze check */
	clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));

	/* Get bus to handle messages */
	GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, this, NULL);
	g_object_unref (bus);

	// lastBufferTime = gst_clock_get_time(clock);
	lastBufferTime = -1;

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	cerr << uri << ": Recording started" << endl << endl;

	/* Now the recording is started, so we can add file name to files vector */
	files.push_back(fileName);

	freeze_check_id = g_timeout_add (500, freeze_check, this); // run freeze check every x milliseconds	

	return true;
}

bool Recording::stop()
{
	// Stop checking data flow for freeze
	g_source_remove(freeze_check_id);
	// send EOS
	status = STOPPING;
	gst_element_send_event(src, gst_event_new_eos());
	cerr << uri << ": Sent EOS to stream" << endl << endl;;

	// while(status != STOPPED);
	return true;
}

void Recording::restart()
{
	cerr << uri << ": restarting" << endl << endl;
	// gst_element_set_state(pipeline, GST_STATE_READY);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	/* Restarting recording to a new file */

	/* Create new file name */
	string stream = uri.substr(uri.find("@") + 1);
    char datetime[18];
    time_t t = time(nullptr);
    strftime(datetime, sizeof(datetime), "%d-%m-%Y_%H:%M", localtime(&t));
    fileName = stream + "_" + string(datetime);
    files.push_back(fileName);

	string file = folder + fileName  + ".mp4";
    // g_object_set(sink, "location", file.c_str(), NULL);
	cout << uri << ": restarting recording. New file:" << file << endl << endl;

    // gst_element_set_state (pipeline, GST_STATE_PLAYING);
    // status = RUNNING;

   	// freeze_check_id = g_timeout_add (500, freeze_check, this); // restart freeze check every x milliseconds	
	start();

}

bool Recording::buildPipeline()
{
    src = gst_element_factory_make("rtspsrc", ("src " + fileName).c_str());
    depay = gst_element_factory_make("rtph264depay", ("depay " + fileName).c_str());  
    parse = gst_element_factory_make("h264parse", ("parse " + fileName).c_str());
    mux = gst_element_factory_make("mpegtsmux", ("mux  " + fileName).c_str());
    sink = gst_element_factory_make("filesink", ("sink " + fileName).c_str());

    pipeline = gst_pipeline_new(("pipeline " + fileName).c_str());

    if (!pipeline || !src || !depay || !parse || !mux || !sink)
    {
        cerr << "Not all pipeline elements could be created" << endl << endl;
        return false;
    } 

    gst_bin_add_many(GST_BIN(pipeline), src,  depay, parse, mux, sink, NULL);

    if (!gst_element_link_many(depay, parse, mux, sink, NULL))
    {
        cerr << "Pipeline linking error" << endl << endl;
        return false;
    }
    
    /* Set properties */
    // g_object_set (src, "tcp-timeout", 200000,	
    // 		"timeout", 200000,
    // 		"location", uri.c_str(),
    // 		NULL);

    g_object_set (src,
    		"location", uri.c_str(),
    		// "protocols", (1 << 2), // TCP,
    		// "timeout", 1000,
    		NULL);
    string file = folder+fileName + ".mp4";
    g_object_set(sink, "location", file.c_str(), NULL);

    /* Signal to handle new source pad*/
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
		cerr << recording->uri << ": Wrong stream prefix" << endl << endl;;
		return;
	}

	/* Attempt the link */
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
		cerr << recording->uri << ": Source pad link failed" << endl << endl;
	}
	else 
	{       
		cerr << recording->uri << ": Linked source pad" << endl << endl;
		// Add data probe to remember the last received video buffer time
		gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_BUFFER, data_probe, recording, NULL);

	}
}

GstPadProbeReturn Recording::data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	/* Updating the time last buffer eas sent */
	Recording* recording = (Recording*) user_data;
	recording->lastBufferTime = gst_clock_get_time(recording->clock);
	// cerr << recording->lastBufferTime << endl;
	return GST_PAD_PROBE_OK;
}


gboolean Recording::freeze_check(gpointer user_data)
{	
	Recording* recording = (Recording*) user_data;

	if (recording->lastBufferTime == -1) return true; // No need to check if no stream is playing

	// if (recording->status == STOPPING) return false; // Stop checking if streaming is stopping
	// current time
	GstClockTime current = gst_clock_get_time(recording->clock);

	// check difference between current time and last time data was received
	GstClockTimeDiff diff = GST_CLOCK_DIFF(recording->lastBufferTime, current);
	// cerr << diff << " " << recording->timeout << endl;
	if (diff > recording->timeout)
	{
		cerr << recording->uri << ": timeout reached. Restarting." << endl << endl;
		recording->lastBufferTime = current;
		gst_element_send_event(recording->src, gst_event_new_eos());
		recording->status = RESTARTING;
		return false; // Stop checking. Start checks when the stream is restarted
	}
	else 
	{

	}
	return true; // to contionue checking regularly
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
			cerr << "Recording " << recording->uri << endl 
				 << "Bus: " << err->message  << endl 
				 << debug << endl << endl;
			break;
		}
		case GST_MESSAGE_ELEMENT:
		{
			const GstStructure *s = gst_message_get_structure (message);
			const gchar *name = gst_structure_get_name (s);
			// cout << name << endl << endl;
			break;
		}
		case GST_MESSAGE_EOS:
		{
			cerr << recording->uri << ": EOS in bus" << endl << endl;
			if (recording->status == STOPPING) 
				recording->status = STOPPED;
			else
				gst_element_set_state(recording->pipeline, GST_STATE_NULL);
				// recording->start();
				// recording->restart();
			break;
		}
		default:
			const GstStructure *s = gst_message_get_structure (message);
			const gchar *name = gst_structure_get_name (s);
			// cout << name << endl << endl;
			break;
	}
	return GST_BUS_PASS;
}