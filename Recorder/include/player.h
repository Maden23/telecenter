#include <gtk/gtk.h>

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
	Player(GtkWidget* videoWindow, Config *config);
	~Player();

	void playStream(string uri);

	GstElement *pipeline, *src, *depay, *parse, *dec, *scale, *sink;

	Config *config;


private:

	GtkWidget *videoWindow;
	GstBus *bus;

	void buildPipeline();

	// Video rendering using GTK
	guintptr videoWindowHandle = 0;
	static void videoWidgetRealize_cb (GtkWidget *widget, Player *player);
	static gboolean videoWidgetDraw_cb (GtkWidget *widget, cairo_t *cr, gpointer user_data);


	// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player);

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player);
};



