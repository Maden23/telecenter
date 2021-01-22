#include "player.h"

#include <string>
#include <vector>
#include <fstream>

#include <gtk/gtk.h>
#include "mqtt_client.h"
#include "cameraClient.h"

using namespace std;

class SingleStreamUI
{
public:
    SingleStreamUI();
    ~SingleStreamUI();
private:
    GtkBuilder *playerBuilder;
    GtkWidget *playerWindow;

    GtkWidget *playerWidget, *playerLabel;

    Player *player;

    int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
    void initPlayerWidgets();

    /* Widgets overlayed over video -- arrow direction buttons, zoom buttons. 
    Appear after user clicks on video, hide after some time, or on another click*/
    vector<GtkWidget*> hidableOverlayWidgets;
    void initOverlayWidgets();
    static gboolean showOverlayWidgets(gpointer ui_ptr);
    static gboolean hideOverlayWidgets(gpointer ui_ptr);
    static gboolean overlayWidgetClicked(GtkWidget *widget, GdkEventButton *event, gpointer ui_ptr);
    // Catch click on player
    static void playerClicked(SingleStreamUI *ui);
    guint hideWidgetsGSource = 0;
    

    static gpointer runMqtt(gpointer data);
    MqttClient *mqtt;
    GAsyncQueue *mqttQueue;
    static gpointer parseQueue(gpointer data);

    CameraClient *cameraClient; 

};

