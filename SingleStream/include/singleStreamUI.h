#include "player.h"

#include <string>
#include <vector>
#include <fstream>

#include <gtk/gtk.h>
#include "mqtt_client.h"

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
    vector<GtkWidget*> hidableOverlayWidgets;

    Player *player;

    int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
    void initPlayerWidgets();
    void initOverlayWidgets();

    static gboolean widgetClicked(GtkWidget *widget, GdkEventButton *event);

    static gpointer runMqtt(gpointer data);
    MqttClient *mqtt;
    GAsyncQueue *mqttQueue;
    static gpointer parseQueue(gpointer data);

};

