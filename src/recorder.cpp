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

bool Recorder::startRecording(string uri, string fileName)
{
    /* Check if recording is already running */
    if (runningRecordings.find(uri) != runningRecordings.end())
    {
        return true;
    }

    /* Create file name if not specified */
    if (fileName == "")
    {
        string stream = uri.substr(uri.find("@") + 1);
        char datetime[18];
        time_t t = time(nullptr);
        strftime(datetime, sizeof(datetime), "%d-%m-%Y_%H:%M", localtime(&t));
        fileName = stream + "_" + string(datetime) + ".mp4"; 
    }

    long timeout_ms = config->getParamInt("videoTimeout");
    if (timeout_ms == -1)
    {
        cerr << "Timeout not found" << endl << endl;
        return false;
    }
    long timeout_ns = timeout_ms * 1000000;
    Recording *recording = new Recording(uri, config->getParam("saveToFolder"), fileName, timeout_ns);
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

bool Recorder::uploadVideo(string uri, string fileName)
{
    string command = "python3 src/video-upload.py";
    command += " " + fileName;
    command += " " + config->getParam("saveToFolder");
    cout << "Running " << command << endl << endl;
    system(command.c_str());
    // execl("python3", "src/video-upload.py", "fileName", config->getParam("saveToFolder"));
    return true;

}

void Recorder::checkIfRecStopped()  
{
    for (auto it = runningRecordings.cbegin(); it != runningRecordings.end();)
    {
        // cout << it->first << " status: " << it->second->getStatus() << endl;
        /* If any recording has this status, it needs to be deleted, and it's video -- uploaded*/
        if (it->second->getStatus() == STOPPED)
        {

            string fileName = it->second->getFileName();

            delete it->second;
            // cout << "HIIIIIIIIIIIIIIIIIIIII";
                // runningRecordings.erase(uri)];
            it = runningRecordings.erase(it);

            cout << "stopped." << endl;
            cout << "File name: " << fileName << endl << endl;

            // pid_t pid = fork();
            // if (pid == -1)   
            // {
            //     cerr << "Failed to fork" << endl;
            //     cout << "Failed to start" << endl;
            //     return false;
            // }
            // if (pid == 0)
            // {
            //     // Child process
            //     uploadVideo(uri, fileName);
            // }   
            // else
            // {
            //     cout << "Called a python script" << endl;
            // } 
        }
        else
        { 
            it++;
        }
    }
}
