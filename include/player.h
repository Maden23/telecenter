#include <gtk/gtk.h>

// #define ON_JETSON

#if defined (GDK_WINDOWING_X11)  // for GDK_WINDOW_XID
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WIN32
#include &lt;gdk/gdkwin32.h&gt;  // for GDK_WINDOW_HWND
#endif

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include "config.h"

struct PadData
{
	GstElement *src;
	GstElement *depay;
};

class Player
{
public:
	Player(GtkWidget* videoWindow, GtkWidget* camLabel, Config *config);
	~Player();

	void playStream(string cam_id);
	void stopStream();

	GstElement *pipeline, *src, *depay, *parse, *dec, *scale, *sink;
	GstClock *clock;
	GstClockTime lastBufferTime; // time of last played data buffer in nanoseconds

	Config *config;


private:

	GtkWidget *videoWindow, *camLabel;
	GstBus *bus;

	void buildPipeline();
};


// Video rendering using GTK
static guintptr videoWindowHandle = 0;
static void videoWidgetRealize_cb (GtkWidget *widget, gpointer *data);

// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player);

// Dynamic source linking
static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player);

// Functions to catch a freeze
GstPadProbeReturn data_probe (GstPad *pad, GstPadProbeInfo *info,  gpointer user_data);
gboolean freeze_check(gpointer user_data);
