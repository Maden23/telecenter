#include "recording.h"
#include "config.h"
#include <iostream>
#include <vector>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
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

    map<string, Recording*> getRunningRecordings() { return runningRecordings; }
    bool startRecording(string uri, string fileName = "");
    bool stopRecording(string uri);
   
    void checkIfRecStopped();
private:
    map<string, Recording*> runningRecordings; //stream uri, Recording object
    // map<string, string> fileNames; //stream uri, filename
    Config *config;
    bool uploadVideo(string uri, string fileName);

    // Run in g_timeout_add to wait for EOS in recording
};
