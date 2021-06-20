#ifndef PLAYER_H
#define PLAYER_H

#include <gtk/gtk.h>

#if defined (GDK_WINDOWING_X11)  // for GDK_WINDOW_XID
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WIN32
#include &lt;gdk/gdkwin32.h&gt;  // for GDK_WINDOW_HWND
#endif

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <iostream>
using namespace std;

struct PadData
{
	GstElement *src;
	GstElement *depay;
};

/*!
 * \brief The Player class
 *
 * @ingroup grid
 */
class Player
{
public:
    Player(GtkWidget* videoWindow, string platform);
    Player(GtkWidget* videoWindow, string platform, string uri, string camName);
    ~Player();

    void changeStream(string camName, string uri);
    void playStream();
    void stopStream();

    bool isPlaying() {return playing;}
	GstElement *pipeline, *src, *depay, *parse, *dec, *scale, *sink, *queue;


private:
    bool playing;
    bool restarting;
    string uri, camName;
	GtkWidget *videoWindow;
	GstBus *bus;
    string platform;


    void init();
    GstElement* createSource();
	void buildPipeline();

	// Video rendering using GTK
	guintptr videoWindowHandle = 0;
    static void videoWidgetRealize_cb (GtkWidget *widget, Player *player);
	static gboolean videoWidgetDraw_cb (GtkWidget *widget, cairo_t *cr, gpointer user_data);

	// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
    void sendCustomMessage(gchar *name);

    static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player);

    // For restarting pipeline on error
    void initRestart();
    void doRestart();
    static GstPadProbeReturn src_block_probe(GstPad * pad, GstPadProbeInfo * info, gpointer user_data);
       
	// Dynamic source linking
    static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player);
};


#endif // PLAYER_H


