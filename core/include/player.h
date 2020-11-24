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

typedef void (*eventHandleFunction)(void*);
struct eventHandleFunctionData_t
{
    eventHandleFunction func_to_call;
    void* params;
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

    void setCam(string camName, string uri);


    // void setOnClickFunction(onClickFunctionData_t *func_data);
    eventHandleFunctionData_t onClick;


    void playStream();
    void stopStream();

    bool isPlaying() {return playing;}
	GstElement *pipeline, *src, *depay, *parse, *dec, *scale, *sink, *queue;


private:
    bool playing;
    string uri, camName;
	GtkWidget *videoWindow;
	GstBus *bus;
    string platform;

    GTimer *timer;

    void init();
	void buildPipeline();

	// Video rendering using GTK
	guintptr videoWindowHandle = 0;
    static void videoWidgetRealize_cb (GtkWidget *widget, Player *player);
	static gboolean videoWidgetDraw_cb (GtkWidget *widget, cairo_t *cr, gpointer user_data);

    // Handelling Navigation events
    static gboolean filterGstNavigationEvents (GstPad *pad, GstObject *parent, GstEvent *event);

	// Handelling bus messages (incuding 'prepare-window-handle' for rendering video)
    static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Player *player);

    // For restarting pipeline with g_idle_add
    static gboolean restart(gpointer user_data);
    guint restartID = 0;

    /**
     * @brief Для сохранения времени последнего перезапуска пайплайна с помощью g_timer_elapsed(timer, NULL)
     * 
     */
    gdouble lastRestartTime = -1;

    /**
     * @brief Количество недавних перезапусков пайплайна (следует обнулять при длительной паузе между перезапусками)
     * 
     */
    int restartCounter = 0;
    
	// Dynamic source linking
    static void pad_added_handler (GstElement * src, GstPad * new_pad, Player *player);
};


#endif // PLAYER_H


