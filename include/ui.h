#ifndef UI_H
#define UI_H

#include <iostream>
#include <string>
#include <unistd.h>
#include <gtk/gtk.h>
#include "config.h"
#include "player.h"


using namespace std;

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

	// static int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile);
	void initPlayerWidgets();
	void initMenuWidgets();
};

#endif