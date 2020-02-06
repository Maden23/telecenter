#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <algorithm>

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
    int getCamCount();
    map<string, string> getCams();

    map<string, map<string, string>> getRooms();
    
private:
    Config(){};
    Config(const Config&);
    Config& operator=(const Config&);   

    void getGSuiteRooms();

    map<string, string> configuration;
    map<string, map<string, string>> rooms;
    map<string, string> cameras;


};

#endif