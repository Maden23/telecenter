#include "recorder.h"

Recorder::Recorder(Config *config)
{
    this->config = config;
    g_timeout_add(500, checkIfRecStopped, this);
}

Recorder::~Recorder()
{
    // for (auto &rec : runningRecorders)
    for (auto &rec : runningRecordings)
    {
        cerr << "Stopping recording of " << rec.first << endl;

        rec.second->stop();
        delete rec.second;
    }
}


bool Recorder::startRecording(Camera *cam)
{
    /* Check if recording is already running */
    if (runningRecordings.find(cam) != runningRecordings.end())
    {
        return true;
    }
    /* Get timeout for test showing */
    long timeout_ms = config->getParamInt("videoTimeout");
    if (timeout_ms == -1)
    {
        cerr << "Timeout not found" << endl << endl;
        return false;
    }
    long timeout_ns = timeout_ms * 1000000;

    /* Get video time limit */
    long videolimit_s = config->getParamInt("videoTimeLimitSeconds");
    if (videolimit_s == -1)
    {
        cerr << "videoTimeLimitSeconds not found. Default = 0" << endl << endl;
        videolimit_s = 0;
    }
    long videolimit_ns = videolimit_s * 1000000000;

    Recording *recording = new Recording(cam->uri, config->getParam("saveToFolder"), cam->name, timeout_ns, videolimit_ns);
    if (!recording->start())
    {
        cerr << "Failed to start recording of " << cam->uri << endl << endl;
        return false;
    }
    runningRecordings[cam] = recording;
    cerr << "Started recording of " << cam->uri << endl << endl;
    return true;
}

bool Recorder::stopRecording(Camera *cam)
{
    cerr << "Trying to stop recording of " << cam->uri << "." << endl << endl;

    if (runningRecordings.find(cam) == runningRecordings.end())
    {
        cerr << "failed" << endl;
        cerr << "No recording found" << endl << endl;
        return false;
    }

    // runningRecorders.erase(uri);

    // uploadVideo(uri);
    runningRecordings[cam]->stop();

    return true;
}

void Recorder::uploadVideo(string uri, string fileName)
{
    runningGDriveUploads++;
    string command = "python3 src/video-upload.py";
    command += " " + fileName;
    command += " " + config->getParam("saveToFolder"); 
    command += " &";

    // fileNames.erase(uri);

    cerr << "Running " << command << endl << endl;
    system(command.c_str());
    runningGDriveUploads--;
}

gboolean Recorder::checkIfRecStopped(gpointer data)  
{
    auto *recorder = (Recorder*) data;
    for (auto it = recorder->runningRecordings.cbegin(); it != recorder->runningRecordings.end();)
    {
        /* If any recording has this status, it needs to be deleted, and it's video -- uploaded*/
        if (it->second->getStatus() == STOPPED)
        {
            cerr << "Recording of " << it->second->uri << " stopped." << endl;
            vector<string> files = it->second->getFiles();
            for (auto file : files)
            {
                cerr << "File name: " << file << endl << endl;
            }
            // Turn off recImage widget
            gtk_widget_hide(it->first->recImage);

            delete it->second;
            it = recorder->runningRecordings.erase(it);

            for (auto file : files)
            {
                 pid_t pid = fork();
                 if (pid == -1)
                 {
                     cerr << "Failed to fork" << endl;
                     cout << "Failed to start" << endl;
                     return false;
                 }
                 if (pid == 0)
                 {
                     // Child process
                     recorder->uploadVideo(it->first->uri, file);
                 }
                 else
                 {
                     cout << "Called an upload script for " << file << endl;
                 }
             }
        }
        else
        { 
            it++;
        }
    }
    return true;
}
