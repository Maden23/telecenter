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

#include "recording.h"
#include "room.h"

using namespace std; 

/*! \brief Управляет процессом записи
 *
 * Forks gstreamer pipelines for each recording
 * Needs a stream id to start a recording 
 * Stores pids of running recorder processes
 * Stops recordings by stream id
 *
 * @ingroup recorder
 * */
class Recorder
{
public:
    Recorder(Config *config);
    ~Recorder();

    map<Camera*, Recording*> getRunningRecordings() { return runningRecordings; }
    bool startRecording(Camera* cam);
    bool stopRecording(Camera* cam);

    bool isGDriveUploadActive() { return runningGDriveUploads > 0; }
   
private:
    map<Camera*, Recording*> runningRecordings;
    Config *config;

    int runningGDriveUploads = 0;
    struct uploadVideoAsyncData_t {
        int *runningGDriveUploads;
        vector<string> files;
    };
    static void *uploadVideoAsync(gpointer uploadVideoAsyncData);
    static gboolean checkIfRecStopped(gpointer recorder_ptr);
};
