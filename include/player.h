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
#include <wait.h>


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

	GstElement *out_pipeline, *out_src, *out_queue, *out_scale, *out_convert, *out_sink;
	GstElement *in_pipeline, *in_src, *in_depay, *in_parse, *in_dec, *in_convert, *in_scale, *in_caps, *in_sink;

	Config *config;


private:

	GtkWidget *videoWindow, *camLabel;
	GstBus *out_bus;

	void buildInputPipeline();
	void buildOutputPipeline();

	pid_t snowmixPid;
	pid_t startSnowmix();
	void stopSnowmix();
};


// Video rendering using GTK
static guintptr videoWindowHandle = 0;
static void videoWidgetRealize_cb (GtkWidget *widget, gpointer *data);

// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player);

// Dynamic source linking
static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player);
