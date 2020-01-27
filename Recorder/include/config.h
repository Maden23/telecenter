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
*       config->parseFile("config.txt"); */

class Config
{
public:
    static Config& get()
    {
        static Config instance;
        return instance;
    }
    void parseFile(string configPath);

    string getParam(string name);
    int getParamInt(string name);
    string getCamUri(string cam);
    int getCamCount();
    map<string, string> getCams();
    
private:
    Config(){};
    Config(const Config&);
    Config& operator=(const Config&);   
    map<string, string> configuration;
    map<string, string> cameras;

};

#endif