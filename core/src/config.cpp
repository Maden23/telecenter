#include "config.h"

Config::Config()
{
    getCustomRooms();
    getGSuiteRooms();
    cout << "Known cameras: " << endl;
    for (auto room : rooms)
    {
        cout << "\t" << room->getName() << endl;
        for (auto &cam : *room->getCameras())
        {
            cout << cam.fullName << " (" << cam.name << ") " << " : " << cam.uri << endl;
        }
        cout << "Sound: " << endl
             << room->getAudioSource().name << " : "
             << room->getAudioSource().uri << endl;
    }
    cout << endl;
} 

void Config::setFile(string configPath)
{
    ifstream file(configPath);
    if (file.is_open())
    {
        string line;
        while(getline(file, line)){
            line.erase(remove_if(line.begin(), line.end(), ::isspace),
                                 line.end());
            if(line[0] == '#' || line.empty())
                continue;
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            /* Check if it is a camera (starts with cam) */
            // if (name.find("cam") == 0)
            // {
            //     string camName = name.substr(3); // delete "cam"
            //     replace(camName.begin(), camName.end(), '_', ' ');
            //     cameras[camName] = value;
            // }
            // else
            configuration[name] = value;
        }
        
    }
    else {
        cerr << "Couldn't open config file for reading.\n";
    }
}

string Config::getParam(string name)
{ 
    if (configuration.find(name) == configuration.end())
        return "";
    return configuration[name]; 
}

int Config::getParamInt(string name)
{
    if (configuration.find(name) == configuration.end())
        return -1;
    return stoi(configuration[name]); 
}

vector<Room *>* Config::getRooms()
{
    return &rooms;
}

void Config::getGSuiteRooms()
{
    /* Run script for fetching info from GSuite*/
    int res = system("python3 src/from-gsuite.py");
    if (res == -1) 
    {
        cerr << "Failed to get rooms from GSuite" << endl << endl;
    }

    auto GSuiteRooms = readRoomsFromFile("gsuite_rooms.json");
    for (auto &room : GSuiteRooms)
    {
        room->type = GSUITE;
    }
    rooms.insert(rooms.begin(), GSuiteRooms.begin(), GSuiteRooms.end());
}

void Config::getCustomRooms()
{
    auto customRooms = readRoomsFromFile("custom_cams.json");
    for (auto &room : customRooms)
    {
        room->type = CUSTOM;
    }
    rooms.insert(rooms.begin(), customRooms.begin(), customRooms.end()); // add custom cameras
}

vector<Room*> Config::readRoomsFromFile(string fileName)
{
    /* Check if file with rooms exists */

    FILE* pFile = fopen(fileName.c_str(), "rb");
    if (!pFile)
    {
        cerr << fileName << " not found" << endl << endl;
        return {};
    }

    /* Read data from JSON to DOM structure*/
    char buffer[65536];
    rapidjson::FileReadStream is(pFile, buffer, sizeof(buffer));
    rapidjson::Document d;
    d.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(is);

    /* Cycle through roooms in file and populate Room objects */
    vector<Room*> rooms;
    for (auto &room : d.GetObject())
    {
        string roomName = room.name.GetString();
        vector<Camera> roomCams;

        // Iterate through array of camera Objects in the room
        for (auto &camItem : room.value["cameras"].GetArray())
        {
            // for (auto &mem : camItem.GetObject())
            //     cout << mem.name.GetString() << endl;
            Camera cam;
            // Add name and rtsp-address of the camera
            cam.name = camItem["name"].GetString();
            cam.fullName = camItem["full_name"].GetString();
            cam.uri = camItem["address"].GetString();
            roomCams.push_back(cam);
        }

        // Get the first audio source of the array
        AudioSource roomAudio;
        auto audio = room.value["audio"].GetArray();
        if (!audio.Empty())
        {
            roomAudio.name = audio[0]["full_name"].GetString();
            roomAudio.uri = audio[0]["address"].GetString();
        }

        // Create new room and add it to vector
        rooms.push_back(new Room(roomName, roomCams, roomAudio));
    }
    return rooms;
}

