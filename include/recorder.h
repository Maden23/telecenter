#include <iostream>
#include <vector>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include "config.h"
#include <ctime>

#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <bits/stdc++.h>

using namespace std; 

/* Forks gstreamer pipelines for each recording
 * Needs a stream id to start a recording 
 * Stores pids of running recorder processes
 * Stops recordings by stream id
 * */
class Recorder
{
public:
    Recorder(Config *config);
    ~Recorder();

    map<string, pid_t> getRunningRecorders() { return runningRecorders; }

    pid_t startRecording(string uri, string fileName = "");
    bool stopRecording(string uri);
   
private:
    map<string, pid_t> runningRecorders; //stream uri, pid
    map<string, string> fileNames; //stream uri, filename
    Config *config;
    void uploadVideo(string uri);
};
