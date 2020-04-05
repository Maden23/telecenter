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

/* For finding IP */
#include <cstdlib>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

class UI
{
public:
	UI(Config *config);
	~UI();
	Config *config;
	string playingCamName = ""; // empty if none playing
	
private:
    vector<Room*> *rooms;
    map<Camera*, Player*> players;
    GtkBuilder *menuBuilder;
    GtkWidget *menuWindow;

    static int on_show(gpointer ui_ptr);
	int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
	void initMenuWidgets();
    static void pageSwitched(GtkWidget* widget, GParamSpec* property, vector<Room *> *rooms);
    void initCamWidgets(int room_n, vector<Camera> *cams);
	void initRoomTab(int room_n, string room_name);

	/* For status bar information */
	string findIP();

	struct display_player_data
	{
        Camera cam;
		string *playingCamName;
	};
	static void displayPlayer(GtkWidget* widget, gpointer data);

//    struct key_press_data
//    {
//        vector<Room*>* rooms;
//        Recorder *recorder;
//    };
    static gboolean keyPress(GtkWidget* widget, GdkEventKey *event, UI *ui);

};


#endif
