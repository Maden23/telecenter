#include "config.h"

Config::Config()
{
    getCustomRooms();
    getGSuiteRooms();
    cout << "Known cameras: " << endl;
    for (auto room : rooms)
    {
        cout << "\t" << room.first << endl;
        for (auto &cam : room.second)
        {
            cout << cam.first << " : " << cam.second << endl;
        }
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
    
    getGSuiteRooms();
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



string Config::getCamUri(string cam)
{
    for (auto room : rooms)
    {
        if (room.second.find(cam) != room.second.end())
            return room.second[cam];
    }
    return "";
}

map<string, map<string, string>> Config::getRooms()
{
    return rooms;
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
    rooms.insert(GSuiteRooms.begin(), GSuiteRooms.end());
}

void Config::getCustomRooms()
{
    auto cameras = readRoomsFromFile("custom_cams.json");
    rooms.insert(cameras.begin(), cameras.end()); // add custom cameras

}

map<string, map<string, string>> Config::readRoomsFromFile(string fileName)
{
    /* Check if file with rooms exists */
    ifstream file(fileName);
    if (!file.is_open())
    {
        cerr << fileName << " not found" << endl << endl;
        return {};
    }
    
    map<string, map<string, string>> rooms;

    /* Read data from JSON to DOM structure*/
    string content((istreambuf_iterator<char>(file)), (istreambuf_iterator<char>()));
    rapidjson::Document d;
    d.Parse(content.c_str());

    /* Cycle through roooms and pack cameras to maps */
    for (auto &val : d.GetObject())
    {
        string roomName = val.name.GetString();
        map <string, string> roomCams;
        // Iterating through array of camera Objects in the room 
        for (auto &camItem : val.value.GetArray())
        {
            // for (auto &mem : camItem.GetObject())
            //     cout << mem.name.GetString() << endl;

            // Add name and rtsp-address of the camera to the map
            string camName = camItem["name"].GetString();
            string camAddress = camItem["address"].GetString();
            roomCams[camName] = camAddress;
        }
        rooms.insert({roomName, roomCams});
    }
    return rooms;
}