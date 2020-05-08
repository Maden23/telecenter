#ifndef UI_H
#define UI_H

#include "config.h"
#include "room.h"
#include "player.h"
#include "recorder.h"

#include <iostream>
#include <string>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>


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

/*!
 * \brief The UI class
 *
 * @ingroup recorder
 */
class RecorderUI
{
public:
    RecorderUI(Config *config);
    ~RecorderUI();
    void displayRecordingStatus(string cam_id, bool status);
    Config *config;
    Recorder *recorder;
    string playingCamName = ""; // empty if none playing
    // map<string, GtkWidget*> recImages;
//	vector<struct Camera*> camDataV; // stores data and ui objects assigned to cameras
    vector<GtkWidget*> switchGridV;  // stores grid objects with switches for edit mode on them

private:
    vector<Room*> *rooms;
    Player *player;
    GtkBuilder *menuBuilder, *playerBuilder;
    GtkWidget *menuWindow, *playerWindow;
    GtkWidget *playerWidget, *playerLabel;
    GtkWidget *editButton;

	int initStyles();
	GtkWidget* windowInit(GtkBuilder** builder, string gladeFile, string windowName);
    void initPlayerWidgets();
    void initMenuWidgets();
    void initCamWidgets(int room_n, vector<Camera> *cams);
	void initRoomTab(int room_n, string room_name);

	/* For status bar information */
	string findIP();
    struct gdrive_status_data
    {
        Recorder *recorder;
        GtkWidget *GDriveIcon;
    };
    static gboolean updateGDriveStatus(gpointer user_data);

	struct display_player_data
	{
        Camera cam;
        GtkWidget *playerLabel;
        Player *player;
		string *playingCamName;
	};
	static void displayPlayer(GtkWidget* widget, gpointer data);

//    struct key_press_data
//    {
//        vector<Room*>* rooms;
//        Recorder *recorder;
//    };
    static gboolean keyPress(GtkWidget* widget, GdkEventKey *event, RecorderUI *ui);

    struct switch_state_changed_data
    {
        Camera *cam;
        Recorder *recorder;
    };
    static void switchStateChanged(GtkWidget* widget, gboolean state, gpointer user_data);

    static void editButtonClicked(GtkWidget* widget, vector<GtkWidget*> *switchGridV);
};


#endif
