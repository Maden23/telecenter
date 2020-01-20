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


using namespace std;

struct display_player_data
{
	GtkWidget* playerWidget;
	GtkWidget* playerLabel;
	GtkWidget* grid;
	Player *player;
	string cam_id;
	string *playingCam;
};

struct hide_player_data
{
	GtkWidget* playerWidget;
	GtkWidget* playerLabel;
	string *playingCam;
};

class UI
{
public:
	UI(Config *config);
	~UI();
	void displayRecordingStatus(string cam_id, bool status);
	Config *config;
	Recorder *recorder;
	string playingCam = ""; // empty if none playing
	map<string, GtkWidget*> recImages;
	
private:
	Player *player;
	GtkBuilder *menuBuilder, *playerBuilder;
	GtkWidget *menuWindow, *playerWindow;
	GtkWidget *playerWidget, *playerLabel;

	// Pairs <cam_id, widget>
	map<string, GtkWidget*> buttons;  

	int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
	void initPlayerWidgets();
	void initMenuWidgets();

};

void displayPlayer(GtkWidget* widget, gpointer *data);
void hidePlayer(GtkWidget* widget, gpointer *data);
gboolean keyPress(GtkWidget* widget, GdkEventKey *event, UI *ui);

#endif
