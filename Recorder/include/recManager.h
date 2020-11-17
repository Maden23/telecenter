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
 * Stores pids of running recording processes
 * Stops recordings by stream id
 *
 * @ingroup recorder
 * */
class RecManager
{
public:
    RecManager(Config *config);
    ~RecManager();

    map<Source*, Recording*> getRunningRecordings() { return runningRecordings; }

    bool startRecording(Source* source);
    bool stopRecording(Source* source);

    bool stopAll();

    bool isGDriveUploadActive() { return runningGDriveUploads > 0; }
   
private:
    map<Source*, Recording*> runningRecordings;
    Config *config;

    int runningGDriveUploads = 0;
    struct uploadFileAsyncData_t {
        int *runningGDriveUploads;
        vector<string> files;
    };
    static void *uploadFileAsync(gpointer uploadFileAsyncData);
    static gboolean handleStoppedRecordings(gpointer 
    );
};
