#include "recording.h"

Recording::Recording(string uri, string folder, string fileName, long timeout)
{
	this->uri = uri;
	this->folder = folder;
	this->fileName = fileName;
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
	streamLinked = false;

	/* Initiallizing Gstreamer*/
	gst_init(nullptr, nullptr);

	if (!buildPipeline())
	{
		return false;
	}

	/* Initialize clock for freeze check */
	clock = gst_pipeline_get_clock(GST_PIPELINE(pipeline));

	/* Get bus to handle messages*/
	GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	// set up sync handler for setting the xid once the pipeline is started
	gst_bus_set_sync_handler (bus, (GstBusSyncHandler) busSyncHandler, this, NULL);
	g_object_unref (bus);

	lastBufferTime = gst_clock_get_time(clock);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	// gst_element_set_state(testsrc, GST_STATE_READY);

	freeze_check_id = g_timeout_add (500, freeze_check, this); // run freeze check every x milliseconds	

	return true;
}

bool Recording::stop()
{
	// send EOS
	status = STOPPING;
	gst_element_send_event(src, gst_event_new_eos());
	cerr << uri << ": Sent EOS to stream" << endl;

	// GstPad* pad = gst_element_get_static_pad(testsrc, "src");
	// gst_pad_add_probe(pad, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),eos_drop_probe, NULL, NULL);

	if (elementSrcLinked(parse))
	{
		cerr << "Stream is linked" << endl;
		gst_element_send_event(src, gst_event_new_eos());

	}
	else
	{
		cerr << "Stream is not linked" << endl;
	}
	if (elementSinkLinked(fakesink))
		cerr << "Test linked to fake" << endl;
	else
		cerr << "Test is linked to sink" << endl;
	gst_element_unlink(testsrc, enc);
	gst_element_send_event(mux, gst_event_new_eos());
	// gst_element_send_event(testsrc, gst_event_new_eos());
	// cerr << "SENT EOS to testsrc" << endl;

	// gst_element_send_event(pipeline, gst_event_new_eos());
	// cerr << "SENT EOS" << endl;
	// while(status != STOPPED);
	return true;
}

bool Recording::buildPipeline()
{
    src = gst_element_factory_make("rtspsrc", ("src " + fileName).c_str());
    testsrc = gst_element_factory_make("videotestsrc", ("testsrc " + fileName).c_str());
    depay = gst_element_factory_make("rtph264depay", ("depay " + fileName).c_str());  
    parse = gst_element_factory_make("h264parse", ("parse " + fileName).c_str());
    mux = gst_element_factory_make("mpegtsmux", ("mux  " + fileName).c_str());
    sink = gst_element_factory_make("filesink", ("sink " + fileName).c_str());
    capsfilter = gst_element_factory_make("capsfilter", ("caps " + fileName).c_str());
    enc = gst_element_factory_make("x264enc", ("enc " + fileName).c_str());
    fakesink = gst_element_factory_make("fakesink", ("fakesink " + fileName).c_str());

    pipeline = gst_pipeline_new(("pipeline " + fileName).c_str());

    if (!pipeline || !src || !testsrc || !depay || !parse || !mux || !sink || !capsfilter || !enc || !fakesink)
    {
        cerr << "Not all pipeline elements could be created" << endl << endl;
        return false;
    } 

    gst_bin_add_many(GST_BIN(pipeline), src, testsrc, depay, parse, mux, sink, capsfilter, enc, fakesink, NULL);
    // gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, mux, sink, NULL);

    /* Caps for videotestsrc */
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                 "width", G_TYPE_INT, 1280,
                 "height", G_TYPE_INT, 720,
                 "framerate", GST_TYPE_FRACTION, 25, 1, NULL);
    g_object_set(capsfilter, "caps", caps, NULL);

    if (!gst_element_link_many(testsrc, capsfilter, enc, fakesink, NULL))
    {
    	cerr << "Test pipeline linking error" << endl << endl;;
    	return false;
    }

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
    		"protocols", (1 << 2), // TCP,
    		"timeout", 1000,
    		NULL);

    g_object_set(testsrc, 
	// "do-timestamp", true,
	"pattern", 13,
	"is-live", true,
	 NULL);

    g_object_set(sink, "location", (folder+fileName).c_str(), NULL);

    /* Signal to handle new source pad*/
    g_signal_connect(src, "pad-added", G_CALLBACK(pad_added_handler), this);

    streamLinked = true;
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
	return GST_PAD_PROBE_OK;
}

/* This is called when new data is available on the pad, which was blocked */
GstPadProbeReturn Recording::block_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	Recording* recording = (Recording*) user_data;
	cerr << recording->uri << ": New data in stream" << endl << endl;
	/* Relinking source */
	if (!recording->showStream())
		cerr << recording->uri << ": Failed to show stream" << endl << endl;


	return GST_PAD_PROBE_REMOVE; // removing the blocking probe
}

GstPadProbeReturn Recording::eos_drop_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	cerr << "EOS dropped by probe" << endl << endl;
	/* Removing this probe */
	gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID (info));
	/* Preventing EOS from moving further */
	return GST_PAD_PROBE_DROP;
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
		if (!recording->showTest())
			cerr << recording->uri << ": Failed to show test" << endl << endl;
		// else 
		// 	return false; // Freeze caught, test is showing - stopping regular check. It will start again with new source
	}
	else 
	{
		if (!recording->showStream())
			cerr << recording->uri << ": Failed to show stream" << endl << endl;
	}
	return true; // to contionue checking regularly
}

bool Recording::showTest()
{
	/* if fakesink is not linked to anything, then test is already linked to sink*/
	// if (!elementSinkLinked(fakesink))
	// 	return true;
	if (streamLinked == false)
		return true;

	cerr << uri << ": Preparing to link test" << endl << endl;

	/* Attach handler to parse src pad for cathing EOS */
	GstPad* parse_src = gst_element_get_static_pad(parse, "src");
	gst_pad_add_probe(parse_src, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM), eos_drop_probe, NULL, NULL);
	cerr << uri << ": EOS probe assigned to parse" << endl << endl;

	/* Send EOS to src */
	gst_element_send_event(src, gst_event_new_eos());
	cerr << uri << ": EOS sent to rtspsrc" << endl << endl;

	/* Block stream pad */
	gst_pad_add_probe(parse_src, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER), block_probe, this, NULL);
	gst_object_unref(parse_src);
	cerr << uri << ": Stream pad blocked" << endl << endl;

	// gst_element_set_state (pipeline, GST_STATE_READY);

	/* Link testsrc */
	// Unlink test from fakesink
	gst_element_unlink(enc, fakesink);
	// Link test to real sink
	if (relinkElements(parse, enc, mux))
	{
		cerr << uri << ": Test linked successfully" << endl << endl;
		streamLinked = false;
	}
	else
	{
		cerr << uri << ": Failed to link test" << endl << endl;
		return false;
	}
	// gst_element_set_state(pipeline, GST_STATE_PLAYING);
	return true;

}

bool Recording::showStream()
{
	/* Check if stream is already linked */
	// if (elementSrcLinked(parse))
	// {
	// 	return true;
	// }
	if (streamLinked)
		return true;

	cerr << uri << ": Preparing to link stream" << endl << endl;

	/* Attach handler to enc src pad for cathing EOS */
	// GstPad* enc_src = gst_element_get_static_pad(enc, "src");
	// gst_pad_add_probe(enc_src, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM), eos_drop_probe, NULL, NULL);
	// cerr << uri << ": EOS probe assigned to encoder" << endl << endl;

	// /* Send EOS to testsrc */
	// gst_element_send_event(testsrc, gst_event_new_eos());
	// cerr << uri << ": EOS sent to videotestsrc" << endl << endl;

	/* Relinling */
	// gst_element_set_state (pipeline, GST_STATE_READY);
		
	// Stream to real sink
	if (relinkElements(enc, parse, mux))
	{
		cerr << uri << ": Stream relinked successfully" << endl << endl;
		streamLinked = true;
	}
	else
	{
		cerr << uri << ": Failed to relink stream" << endl << endl;
	}

	// Test to fake sink
	if (!gst_element_link(enc, fakesink))
	{
		cerr << uri << ": Failed to link test to fakesink" << endl << endl;
	}

	// gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return true;

}

bool Recording::elementSrcLinked(GstElement *elem)
{
	GstPad *pad = gst_element_get_static_pad (elem, "src");
	return gst_pad_is_linked(pad);
}

bool Recording::elementSinkLinked(GstElement *elem)
{
	GstPad *pad = gst_element_get_static_pad (elem, "sink");
	return gst_pad_is_linked(pad);
}


bool Recording::relinkElements(GstElement *wrong_src, GstElement *right_src, GstElement* sink)
{
	/* Relinking pipeline */
	gst_element_unlink(wrong_src, sink);
	if(gst_element_link(right_src, sink))
		return true;
	else
	{
		// return to previous link state if new link didn't work
		gst_element_link(wrong_src, sink);
		return false;
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
			recording->status = STOPPED;
			break;
		}
		default:
			const GstStructure *s = gst_message_get_structure (message);
			const gchar *name = gst_structure_get_name (s);
			cout << name << endl << endl;
			break;
	}
	return GST_BUS_PASS;
}