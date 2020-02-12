#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include "rapidjson/document.h"

using namespace std;

/* Singleton class to store configuration. 
*  Init with:
*       Config *config = &Config::get();
*       config->setFile("recorder.conf");
*       config->parseCamsFile("cams.conf") 
*       config->getRooms() */

class Config
{
public:
    static Config& get()
    {
        static Config instance;
        return instance;
    }
    void setFile(string configPath);

    string getParam(string name);
    int getParamInt(string name);

    string getCamUri(string cam);

    map<string, map<string, string>> getRooms();
    
private:
    Config();
    Config(const Config&);
    Config& operator=(const Config&);   

    void getGSuiteRooms();
    void getCustomRooms();

    string makeGSuiteRequest();
    map<string, map<string, string>> readRoomsFromFile(string fileName);

    map<string, string> configuration;
    map<string, map<string, string>> rooms;

};

#endif