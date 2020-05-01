#include "recording.h"

Recording::Recording(string uri, string folder, string camName, long timeout, long videoTimeLimit)
{
	this->uri = uri;
	this->folder = folder;
    this->camName = camName;
	this->timeout = timeout;
    this->videoTimeLimit = videoTimeLimit;
    /* Create file name */
    if (fileNamePattern == "")
    {
//        string stream = cam->uri.substr(cam->uri.find("@") + 1);
        char datetime[18];
        time_t t = time(nullptr);
        strftime(datetime, sizeof(datetime), "%d-%m-%Y_%H:%M", localtime(&t));
        fileNamePattern = camName + "_" + string(datetime)+ "_%02d.mp4";
//        fileName = stream + "_" + string(datetime) + ".mp4";
    }

}

Recording::~Recording()
{
//	gst_element_set_state(pipeline, GST_STATE_NULL);
//    g_source_remove(freeze_check_id);
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

    // run freeze check every 200 milliseconds
//    freeze_check_id = g_timeout_add (200, freeze_check, this);

	return true;
}

bool Recording::stop()
{
    if (status == STOPPING) return false;

	/* Stop checking for freezes */
//    g_source_remove(freeze_check_id);

	// send EOS
	status = STOPPING;
	gst_element_send_event(src, gst_event_new_eos());
    cerr << camName << ": Sent EOS to stream" << endl;

	// GstPad* pad = gst_element_get_static_pad(testsrc, "src");
	// gst_pad_add_probe(pad, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),eos_drop_probe, NULL, NULL);

    if (streamLinked)
    {
		gst_element_send_event(src, gst_event_new_eos());
	}
    else
    {
        gst_element_send_event(testsrc, gst_event_new_eos());
    }

	return true;
}


bool Recording::buildPipeline()
{
    /* For RTSP stream */
    src = gst_element_factory_make("rtspsrc", ("src " + camName).c_str());
    depay = gst_element_factory_make("rtph264depay", ("depay " + camName).c_str());
    parse = gst_element_factory_make("h264parse", ("parse " + camName).c_str());
    streamcapsfilter = gst_element_factory_make("capsfilter", ("streamcaps " + camName).c_str());


    /* For test */
    testsrc = gst_element_factory_make("videotestsrc", ("test " + camName).c_str());
    testcapsfilter = gst_element_factory_make("capsfilter", ("testcaps " + camName).c_str());
    enc = gst_element_factory_make("x264enc", ("enc " + camName).c_str());

    /* Sinks */
//    mux = gst_element_factory_make("mpegtsmux", ("mux  " + camName).c_str());
//    sink = gst_element_factory_make("filesink", ("sink " + camName).c_str());
    sink = gst_element_factory_make("splitmuxsink", ("sink " + camName).c_str());
    fakesink = gst_element_factory_make("fakesink", ("fakesink " + camName).c_str());

    pipeline = gst_pipeline_new(("pipeline " + camName).c_str());

//    if (!pipeline || !src || !depay || !parse || !streamcapsfilter || !mux || !sink || !fakesink || !testsrc || !testcapsfilter || !enc)
    if (!pipeline || !src || !depay || !parse ||  !sink )
    {
        cerr << camName << "Not all pipeline elements could be created" << endl << endl;
        return false;
    } 

//    gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, streamcapsfilter, mux, sink, fakesink, testsrc, testcapsfilter, enc, NULL);
    gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, sink, NULL);

    // Caps for videotestsrc
    GstCaps* streamcaps = gst_caps_new_simple("video/x-h264",
                 "width", G_TYPE_INT, 1920,
                 "height", G_TYPE_INT, 1080,
                 "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
    g_object_set(streamcapsfilter, "caps", streamcaps, NULL);

    /* Link stream to fake sink (src will be linked in playing state) */
//    if (!gst_element_link_many(depay, parse, streamcapsfilter, fakesink, NULL))
//    {
//        cerr << "Stream linking error" << endl << endl;
//        return false;
//    }
    /* Link stream to real sink */
    if (!gst_element_link_many(depay, parse, sink, NULL))
    {
        cerr << "Stream linking error" << endl << endl;
    }
    // Set properties for stream source
    g_object_set (src,
            "location", uri.c_str(),
            "protocols", (1 << 2), // TCP,
//            "timeout", 1000,
            NULL);
    // Set properties for streaming time limit
    g_object_set(sink,
                 "location", (folder+fileNamePattern).c_str(),
                 "max-size-time", videoTimeLimit,
                 "muxer-factory", "mpegtsmux",
                 NULL);

    // Signal to handle new source pad
    g_signal_connect(src, "pad-added", G_CALLBACK(pad_added_handler), this);

    /* Link test to normal sink */
    // Caps for videotestsrc
//    GstCaps* caps = gst_caps_new_simple("video/x-raw",
//                 "width", G_TYPE_INT, 1920,
//                 "height", G_TYPE_INT, 1080,
//                 "framerate", GST_TYPE_FRACTION, 30, 1, NULL);
//    g_object_set(testcapsfilter, "caps", caps, NULL);
//    g_object_set(testsrc,
//    // "do-timestamp", true,
//    "pattern", 13,
//    "is-live", true,
//     NULL);

//    if (!gst_element_link_many(testsrc, testcapsfilter, enc, mux, sink, NULL))
//    {
//        cerr << "Test pipeline linking error" << endl << endl;
//        return false;
//    }


//    streamLinked = false;
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
		// Add data probe to remember the last received video buffer time
//         gst_pad_add_probe(new_pad, GST_PAD_PROBE_TYPE_BUFFER, data_probe, recording, NULL);
	}

}

GstPadProbeReturn Recording::data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    /* Updating the time last buffer was sent */
	Recording* recording = (Recording*) user_data;
	recording->lastBufferTime = gst_clock_get_time(recording->clock);
//    cout << recording->lastBufferTime << endl;
	return GST_PAD_PROBE_OK;
}

GstPadProbeReturn Recording::probe_block_stream(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    auto recording = (Recording*)user_data;
    /* remove the probe first */
    gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

//    /* Prevent calling this repeatedly */
//    if (recording->status == RELINKING)
//    {
//        cerr << "Relinking" << endl;
//        return GST_PAD_PROBE_OK;
//    }
//    else
//        recording->status = RELINKING;

    /* Attach handler to src pad of parse for cathing EOS */
    GstPad* parse_src = gst_element_get_static_pad(recording->parse, "src");
    gst_pad_add_probe(parse_src, GstPadProbeType(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
                         probe_eos_in_stream, recording, NULL);

    cerr << recording->camName << ": EOS probe assigned to parse" << endl << endl;

    /* Send EOS to sink pad of parse  */
//    GstPad* parse_sink = gst_element_get_static_pad(recording->parse, "sink");
    gst_element_send_event(recording->parse, gst_event_new_eos());
    cerr << recording->camName << ": EOS sent to rtspsrc" << endl << endl;
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn Recording::probe_eos_in_stream(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    auto recording = (Recording*)user_data;
	cerr << "EOS dropped by probe" << endl << endl;
	/* Removing this probe */
	gst_pad_remove_probe(pad, GST_PAD_PROBE_INFO_ID (info));

    /* Add idle probe on test pipeline (encoder) */
    recording->in_idle_probe = FALSE;
    GstPad* enc_src = gst_element_get_static_pad(recording->enc, "src");
    gst_pad_add_probe (enc_src, GST_PAD_PROBE_TYPE_IDLE, probe_idle_relink, recording, NULL);
	/* Preventing EOS from moving further */
	return GST_PAD_PROBE_DROP;
}

GstPadProbeReturn Recording::probe_idle_relink(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    auto recording = (Recording*)user_data;

    // Prevent calling this from multiple threads
    if (!g_atomic_int_compare_and_exchange (&recording->in_idle_probe, FALSE, TRUE))
        return GST_PAD_PROBE_OK;

    /* Relinling */
    if (recording->streamLinked)
    {
        // Link test to real sink
        //if (relinkElements(recording->parse, recording->enc, recording->mux))
        if (relinkElements(recording->streamcapsfilter, recording->enc, recording->mux))
        {
            cerr << recording->camName << ": Test linked successfully" << endl << endl;
            recording->streamLinked = false;
            recording->lastBufferTime = gst_clock_get_time(recording->clock);
        }
        else
        {
            cerr << recording->camName << ": Failed to link test" << endl << endl;
        }
    }
    else
    {
        // Link stream to real sink
        if (relinkElements(recording->enc, recording->streamcapsfilter, recording->mux))
        {
            cerr << recording->camName << ": Stream relinked successfully" << endl << endl;
            recording->streamLinked = true;
            recording->lastBufferTime = gst_clock_get_time(recording->clock);
        }
        else
        {
            cerr << recording->camName << ": Failed to relink stream" << endl << endl;
        }
//        cerr << "Dot droped" << endl;
//        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(recording->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
    }
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
	// cout << diff << " " << recording->timeout << endl;

    /* Prevent relinking repeatedly */
    if (recording->status == RELINKING)
    {
//        cerr << "Relinking" << endl;
        return true;
    }

    /* Linking test if timeout is exceeded */
	if (diff > recording->timeout)
	{
        // Needs relinking if stream is linked now
        if (recording->streamLinked)
        {
            cerr << recording->camName << ": Preparing to link test" << endl << endl;
            /* Add blocking pad on parse src pad */
            GstPad* parse_src = gst_element_get_static_pad(recording->parse, "src");
            gst_pad_add_probe (parse_src, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, probe_block_stream, recording, NULL);
            recording->status = RELINKING;
        }
	}
	else 
    /* Linking stream if timeout is NOT exceeded */
	{
        // Needs relinking if test is linked now
        if (!recording->streamLinked)
        {
            cerr << recording->camName << ": Preparing to link source" << endl << endl;
            /* Add blocking pad on parse src pad */
            GstPad* parse_src = gst_element_get_static_pad(recording->parse, "src");
            gst_pad_add_probe (parse_src, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, probe_block_stream, recording, NULL);
            recording->status = RELINKING;
        }
	}
	return true; // to contionue checking regularly
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
            cerr << "Recording " << recording->camName << endl
				 << "Bus: " << err->message  << endl 
				 << debug << endl << endl;
            recording->status = STOPPED;

			g_error_free (err);
     		g_free (debug);
			/* Free pipeline resourses */
//            gst_element_set_state(recording->pipeline, GST_STATE_NULL);
//            g_source_remove(recording->freeze_check_id);
            gst_object_unref(recording->clock);
            gst_object_unref(recording->pipeline);

			/* If error occured with linked rtspsrc, linking test */
			// if (recording->streamLinked)
			// {
			// 	recording->showTest();
			// }
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
            // This recording is finished if stop was initiated by user, not by splitmuxsink
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
