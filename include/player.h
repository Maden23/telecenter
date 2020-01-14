#include <gtk/gtk.h>

//#define ON_JETSON

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

	bool showTest();
	bool showStream();
	bool elementSrcLinked(GstElement *elem);
	bool relinkElements(GstElement *wrong_src, GstElement *right_src, GstElement* sink);


	GstElement *pipeline, *src, *testsrc, *depay, *parse, *dec, *scale, *sink;
	GstClock *clock;
	GstClockTime lastBufferTime = -1; // time of last played data buffer in nanoseconds

	Config *config;


private:

	GtkWidget *videoWindow, *camLabel;
	GstBus *bus;

	bool streamLinked;

	void buildPipeline();

	// Video rendering using GTK
	guintptr videoWindowHandle = 0;
	static void videoWidgetRealize_cb (GtkWidget *widget, Player *player);

	// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player);

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player);

	// Functions to catch a freeze
	static GstPadProbeReturn data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static gboolean freeze_check(gpointer user_data);
};




