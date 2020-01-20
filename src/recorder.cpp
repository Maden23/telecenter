#include "recorder.h"

Recorder::Recorder(Config *config)
{
    this->config = config;
}

Recorder::~Recorder()
{
    for (auto &rec : runningRecordings)
    {
        cout << "Stopping recording of " << rec.first << endl << endl;
        // kill(rec.second, SIGINT);
        // waitpid(rec.second, nullptr, 0);
        rec.second->stop();
        delete rec.second;
    }
}

bool Recorder::startRecording(string uri)
{
    /* Check if recording is already running */
    if (runningRecordings.find(uri) != runningRecordings.end())
    {
        return true;
    }

    long timeout_ms = config->getParamInt("videoTimeout");
    if (timeout_ms == -1)
    {
        cerr << "Timeout not found" << endl << endl;
        return false;
    }
    long timeout_ns = timeout_ms * 1000000;
    Recording *recording = new Recording(uri, config->getParam("saveToFolder"), timeout_ns);
    if (!recording->start())
    {
        cout << "Failed to start recording of " << uri << endl << endl;
        return false;
    }

    runningRecordings[uri] = recording;
    cout << "Started recording of " << uri << endl << endl;
    return true;
}

bool Recorder::stopRecording(string uri)
{
    cout << "Trying to stop recording of " << uri << "...";

    if (runningRecordings.find(uri) == runningRecordings.end())
    {
        cout << "failed" << endl;
        cout << "No recording found" << endl << endl;
        return false;
    }

    runningRecordings[uri]->stop();
      return true;
}

bool Recorder::uploadVideo(string fileName)
{
    string command = "python3 src/video-upload.py";
    command += " " + fileName;
    command += " " + config->getParam("saveToFolder"); 
    command += " &";
    cout << "Running " << command << endl;
    system(command.c_str());

    return true;

}

void Recorder::checkIfRecStopped()  
{
    for (auto it = runningRecordings.cbegin(); it != runningRecordings.end();)
    {      
        /* If any recording has this status, it needs to be deleted, and it's video -- uploaded*/
        if (it->second->getStatus() == STOPPED)
        {
            auto files = it->second->getFiles();

            delete it->second;
            it = runningRecordings.erase(it);

            cout << "stopped." << endl;
            for (auto fileName : files)
            {
                cout << "File name: " << fileName << endl;
                uploadVideo(fileName);
            }
            cout << endl;
        }
        else
        { 
            it++;
        }
    }
}
