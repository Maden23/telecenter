#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <gtk/gtk.h>
#include <player.h>

using namespace std;

enum room_t {CUSTOM, GSUITE};


struct Camera
{
    string name;
    string fullName;
    string uri;
    Player *player;
    GtkWidget *button;
    GtkWidget *drawingArea;
};

struct AudioSource
{
    string name;
    string uri;
};

class Room
{
public:
    Room(string name, vector<Camera> cameras, AudioSource audio)
    {
        this->name = name;
        this->cameras = cameras;
        this->audio = audio;
    }
    Room() {}

    room_t type;

    string getName() { return name; }
    void setName(string name) { this->name = name; }

    vector<Camera>* getCameras() { return &cameras; }
    void setCameras(vector <Camera> cams) { cameras = cams; }

    AudioSource getAudioSource() { return audio; }
    void setAudioSource(AudioSource audio) { this->audio = audio; }

private:
   string name;
   AudioSource audio;
   vector<Camera> cameras;
};


#endif // ROOM_H
