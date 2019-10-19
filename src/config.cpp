#include "config.h"

void Config::parseFile(string configPath)
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
                cameras[camName] = value;
            }
            else
            configuration[name] = value;
        }
        
    }
    else {
        cerr << "Couldn't open config file for reading.\n";
    }
    
    cout << "Known cameras: " << endl;
    for (auto &cam : cameras)
    {
        cout << cam.first << " : " << cam.second << endl;
    }
}