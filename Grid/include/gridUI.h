#ifndef UI_H
#define UI_H

#include "config.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "room.h"
#include "player.h"
#include "mqtt_client.h"

/* For finding IP */
#include <cstdlib>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

/*!
 * \brief The UI class
 * @ingroup grid
 */
class GridUI
{
public:
    GridUI(Config *config);
    ~GridUI();
    Config *config;
	
private:
    vector<Room*> *rooms;
    map<Camera*, Player*> players;
    GtkBuilder *menuBuilder;
    GtkWidget *menuWindow;

	int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
	void initMenuWidgets();
    static void pageSwitched(GtkWidget* widget, GParamSpec* property, vector<Room *> *rooms);
    void initCamWidgets(int room_n, vector<Camera*> *cams);
	void initRoomTab(int room_n, string room_name);

	/* For status bar information */
	string findIP();

    /* Sending an MQTT message on player click */
    struct on_player_click_data
	{
        Camera *cam;
        MqttClient *mqtt;
	};
    static void onPlayerClick(GtkWidget* widget, gpointer data);
    MqttClient *mqtt;


};


#endif
