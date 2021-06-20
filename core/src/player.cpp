#include "player.h"

// Player::videoWindowHandle = 0;

Player::Player (GtkWidget* videoWindow, string platform, string uri, string camName)
{
    this->videoWindow = videoWindow;
    this->platform = platform;
    this->uri = uri;
    this->camName = camName;
    this->playing = false;
    this->restarting = false;
    init();
    playStream();
}

Player::Player(GtkWidget* videoWindow, string platform)
{
    this->videoWindow = videoWindow;
    this->platform = platform;
    this->playing = false;
    this->restarting = false;

    init();
}

Player::~Player() 
{
    gst_bus_remove_signal_watch(bus);
}

void Player::init()
{
    /* Initiallizing Gstreamer*/
//    setenv("GST_DEBUG", "*:4", 1);
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

GstElement* Player::createSource()
{
    GstElement *newSource = gst_element_factory_make("rtspsrc", ("src_" + camName).c_str());
    /* Set latency */
    g_object_set (newSource, "latency", 200, NULL);

    if (uri != "")
        g_object_set (src, "location", uri.c_str(), NULL);

    // stream data over TCP https://gstreamer.freedesktop.org/documentation/rtsplib/gstrtsptransport.html?gi-language=c#GstRTSPLowerTrans
    // g_object_set(newSource, "protocols", 4, NULL); 

    /* Signal to handle new source pad*/
    g_signal_connect(newSource, "pad-added", G_CALLBACK(pad_added_handler), this);


    return newSource;
}

void Player::buildPipeline()
{
    // Create source only if we have stream info
    if (uri != "")
        src = createSource();
    else 
        src = nullptr;
    depay = gst_element_factory_make("rtph264depay", ("depay_" + camName).c_str());
    parse = gst_element_factory_make("h264parse", ("parse_" + camName).c_str());

    pipeline = gst_pipeline_new(("pipeline_" + camName).c_str());

    if (platform == "jetson")
    {
        dec = gst_element_factory_make("omxh264dec", ("dec_" + camName).c_str());
        // dec = gst_element_factory_make("nvv4l2decoder", ("dec_" + camName).c_str());
        queue = gst_element_factory_make("queue", ("queue_" + camName).c_str());
        sink = gst_element_factory_make("nveglglessink", ("sink_" + camName).c_str());
        // sink = gst_element_factory_make("nv3dsink", ("sink_" + camName).c_str());

        if (!pipeline ||  !src || !depay || !parse || !dec || !queue || !sink)
        {
            cerr << "Not all pipeline elements could be created" << endl << endl;
        }

        gst_bin_add_many(GST_BIN(pipeline), src, depay, parse, dec, queue, sink, NULL);

        if (!gst_element_link_many(depay, parse, dec, queue, sink, NULL))
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

}


void Player::changeStream(string camName, string uri)
{
    this->camName = camName;
    this->uri = uri;
    sendCustomMessage("init-restart");
}

GstPadProbeReturn Player::src_block_probe(GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
    return GST_PAD_PROBE_DROP;
}

void Player::initRestart()
{
    if (!restarting)
    {
        restarting = true;
    }
    // Block dataflow on source element, if it exists
    if (src)
    {
        GstPad *srcpad = gst_element_get_static_pad(src, "src");
        gst_pad_add_probe(srcpad, GST_PAD_PROBE_TYPE_BLOCK, src_block_probe, NULL,  NULL);
        gst_object_unref(srcpad);
    }

    sendCustomMessage("do-restart");
}

void Player::doRestart()
{
    stopStream();
    playStream();
    
    restarting = false;
}

void Player::stopStream()
{
    // Get ready to add/remove elements
    gst_element_set_state(pipeline, GST_STATE_READY);

    // Remove old source.
    if (src)
    {
        gst_element_set_state(src, GST_STATE_NULL);
        // remove unlinks automatically
        gst_bin_remove(GST_BIN(pipeline), src);
    }
    
    playing = false;
    
}

void Player::playStream()
{
    if (!playing)
    {
        // Create new source
        src = createSource();
        gst_bin_add(GST_BIN(pipeline), src);
    }
    playing = true;
}

void Player::sendCustomMessage(gchar *name)
{
    GstStructure *s = gst_structure_new_empty(name);
    GstMessage *message = gst_message_new_application(NULL, s);
    gst_bus_post(bus, message);
}

GstBusSyncReply Player::busSyncHandler (GstBus *bus, GstMessage *message, Player *player)
{
    switch (GST_MESSAGE_TYPE(message))
    {
        case GST_MESSAGE_ERROR:
        {
            
            player->playing = false;
            GError *err;
            gchar *debug;

            gst_message_parse_error (message, &err, &debug);

            cerr << "Player "  << player->camName << endl;
            cerr << "Stream: " << player->uri << endl;
            cerr << err->message << endl;
            cerr << debug << endl << endl;
            
            // Restarting pipeline
            player->sendCustomMessage("init-restart");

            g_error_free(err);
            g_free(debug);
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
            player->playing = false;
            cerr << "Player "  << player->camName << endl;
            cerr << "Stream: " << player->uri << endl;
            cerr << "EOS" << endl << endl;
            // Restarting pipeline
            player->sendCustomMessage("init-restart");
            break;
        }
        case GST_MESSAGE_APPLICATION:
        {
            const GstStructure *s = gst_message_get_structure (message);
            const gchar *name = gst_structure_get_name (s);
            if (0 == g_strcmp0(name, "init-restart"))
            {
                player->initRestart();
            }
            else if (0 == g_strcmp0(name, "do-restart") && !player->restarting)
            {
                player->doRestart();
            }
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
        gst_object_unref(sink_pad);
        return;
    }
    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps (new_pad);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (!g_str_has_prefix (new_pad_type, "application/x-rtp"))
    {
        cerr << player->camName << ": Wrong stream prefix" << endl;
        gst_caps_unref(new_pad_caps);
        gst_object_unref(sink_pad);
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
    gst_caps_unref(new_pad_caps);
    gst_object_unref(sink_pad);
}

