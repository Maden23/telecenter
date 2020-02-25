#ifndef CAMERA_H
#define CAMERA_H

#include <string>
#include <gtk/gtk.h>

using namespace std;

struct Camera
{
    string name;
    string uri;
    GtkWidget *button;
    GtkWidget *recImage;
    bool record;
};

#endif // CAMERA_H
