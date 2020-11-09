#include "recorder.h"

Recorder::Recorder(Config *config)
{
    this->config = config;
    // Start regular check on recordings 
    g_timeout_add(500, handleStoppedRecordings, this);
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
   
    /* Get video files time limit from config file */
    long videolimit_s = config->getParamInt("videoTimeLimitSeconds");
    if (videolimit_s == -1)
    {
        cerr << "videoTimeLimitSeconds not found. Default = 0" << endl << endl;
        videolimit_s = 0;
    }
    long videolimit_ns = videolimit_s * 1000000000;

    /* Attempt to start recording */ 
    Recording *recording = new Recording(cam->uri, config->getParam("saveToFolder"), cam->fullName, videolimit_ns);
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

    runningRecordings[cam]->stop();

    return true;
}

void* Recorder::uploadVideoAsync(gpointer uploadVideoAsyncData)
{
    auto data = (uploadVideoAsyncData_t*) uploadVideoAsyncData;
    for (auto fileName : data->files)
    {
        cerr << "Trying to upload " << fileName << endl << endl;
        *(data->runningGDriveUploads) = *data->runningGDriveUploads + 1;
        string command = "python3 src/video-upload.py";
        command += " \"" + fileName + "\"";

        // fileNames.erase(uri);

        system(command.c_str());
        cerr << "Completed upload of " << fileName << endl << endl;
        *(data->runningGDriveUploads) = *data->runningGDriveUploads - 1;
    }
}

gboolean Recorder::handleStoppedRecordings(gpointer recorder_ptr)
{
    auto *recorder = (Recorder*) recorder_ptr;
    // Using iterators to be able to delete items
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

            // Upload files in new thread
            uploadVideoAsyncData_t *data = new uploadVideoAsyncData_t();
            *data = {.runningGDriveUploads = &(recorder->runningGDriveUploads),
                     .files = files};
            g_thread_new(it->first->name.c_str(), uploadVideoAsync, data);

            // Delete Recording
            delete it->second;
            it = recorder->runningRecordings.erase(it);

        }
        else
        { 
            it++;
        }
    }
    return true;
}
