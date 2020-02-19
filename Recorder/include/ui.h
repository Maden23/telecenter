#ifndef UI_H
#define UI_H

#include <iostream>
#include <string>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "config.h"
#include "player.h"
#include "recorder.h"

/* For finding IP */
#include <cstdlib>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* For disk space monitoring */
#include <sys/statvfs.h>
#include <iomanip>

using namespace std;


struct Camera
{
	string name;
	string uri;
	GtkWidget *button;
	GtkWidget *recImage;
};


class UI
{
public:
	UI(Config *config);
	~UI();
	void displayRecordingStatus(string cam_id, bool status);
	Config *config;
	Recorder *recorder;
	string playingCamName = ""; // empty if none playing
	// map<string, GtkWidget*> recImages;
	vector<struct Camera> camDataV; // stores data and ui objects assigned to cameras 
	vector<GtkWidget*> switchGridV;  // stores grid objects with switches for edit mode on them
	
private:
	Player *player;
	GtkBuilder *menuBuilder, *playerBuilder;
	GtkWidget *menuWindow, *playerWindow;
	GtkWidget *playerWidget, *playerLabel;
	GtkWidget *editButton;

	// Pairs <cam_id, widget>
	// map<string, GtkWidget*> buttons;  


	int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
	void initPlayerWidgets();
	void initMenuWidgets();
	void initCamWidgets(int room_n, map<string, string> cams);
	void initRoomTab(int room_n, string room_name);

	/* For status bar information */
	string findIP();
	struct gdrive_status_data
	{
		Recorder *recorder;
		GtkWidget *GDriveIcon;
	};
	static gboolean updateGDriveStatus(gpointer user_data);
};

struct display_player_data
{
	string camName;
	string uri;
	GtkWidget *playerLabel;
	Player *player;
	string *playingCamName;
};
void displayPlayer(GtkWidget* widget, gpointer *data);

gboolean keyPress(GtkWidget* widget, GdkEventKey *event, UI *ui);
void editButtonClicked(GtkWidget* widget, vector<GtkWidget*> *switchGridV);

#endif
