#include "player.h"

#include <string>
#include <fstream>

#include <gtk/gtk.h>

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

};

