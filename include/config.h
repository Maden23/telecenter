#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

/* Singleton class to store configuration. 
*  Init with:
*       Config *config = &Config::get();
*       config->parseFile("config.txt"); */

class Config
{
public:
    static Config& get()
    {
        static Config instance;
        return instance;
    }

    void parseFile(string configPath)
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
        for (auto &cam : cameras)
        {
            cout << "Known cameras: " << endl;
            cout << cam.first << " : " << cam.second << endl;
        }
    }


    
    string getParam(string name){ return configuration[name]; }
    string getCamUri(string cam) { return cameras[cam]; }

private:
    Config(){};
    Config(const Config&);
    Config& operator=(const Config&);   
    map<string, string> configuration;
    map<string, string> cameras;

};

#endif