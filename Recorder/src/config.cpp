#include "config.h"

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
            if (name.find("cam") == 0)
            {
                string camName = name.substr(3); // delete "cam"
                replace(camName.begin(), camName.end(), '_', ' ');
                cameras[camName] = value;
            }
            else
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
    return cameras[cam];
}

int Config::getCamCount() 
{
    return cameras.size(); 
}

map<string, string> Config::getCams() 
{ 
    return cameras; 
}

map<string, map<string, string>> Config::getRooms()
{
    return rooms;
}


void Config::getGSuiteRooms()
{
    map<string, string> room1 = {
        {"51", "rtsp://admin:Supervisor@172.18.200.51:554"},
        {"52", "rtsp://admin:Supervisor@172.18.200.52:554"},
        {"53", "rtsp://admin:Supervisor@172.18.200.53:554"},
        {"54", "rtsp://admin:Supervisor@172.18.200.54:554"},
        {"55", "rtsp://admin:Supervisor@172.18.200.55:554"},
        {"56", "rtsp://admin:Supervisor@172.18.200.56:554"}
    };
    rooms.insert({"", cameras}); // add custom cameras
    rooms.insert({"Зал", room1});
    this->rooms = rooms;
    cout << "Known cameras: " << endl;
    for (auto room : rooms)
    {
        for (auto &cam : room.second)
        {
            cout << cam.first << " : " << cam.second << endl;
        }

    }

}