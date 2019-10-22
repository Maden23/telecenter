#ifndef UI_H
#define UI_H

#include <iostream>
#include <string>
#include <unistd.h>
#include <gtk/gtk.h>
#include "config.h"
#include "player.h"


using namespace std;

struct display_player_data
{
	GtkWidget* playerWidget;
	GtkWidget* playerLabel;
	GtkWidget* grid;
	Player *player;
	string cam_id;
};

struct hide_player_data
{
	GtkWidget* playerWidget;
	GtkWidget* playerLabel;
};

class UI
{
public:
	UI(Config *config);
	~UI();
	void displayRecordingStatus(string cam_id, bool status);
private:
	Config *config;
	Player *player;

	GtkBuilder *menuBuilder, *playerBuilder;
	GtkWidget *menuWindow, *playerWindow;
	GtkWidget *playerWidget, *playerLabel;

	// Pairs <cam_id, widget>
	map<string, GtkWidget*> buttons;  
	map<string, GtkWidget*> recImages;

	int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
	void initPlayerWidgets();
	void initMenuWidgets();

};

	void displayPlayer(GtkWidget* widget, gpointer *data);
	void hidePlayer(GtkWidget* widget, gpointer *data);


#endif