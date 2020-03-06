#include "recorder.h"

Recorder::Recorder(Config *config)
{
    this->config = config;
    g_timeout_add(500, checkIfRecStopped, &runningRecordings);
}

Recorder::~Recorder()
{
    // for (auto &rec : runningRecorders)
    for (auto &rec : runningRecordings)
    {
        cerr << "Stopping recording of " << rec.first << endl;
        // kill(rec.second, SIGINT);
        // waitpid(rec.second, nullptr, 0);
        rec.second->stop();
        delete rec.second;
    }
}

// pid_t Recorder::startRecording(string uri, string fileName)
// {
//     if (runningRecorders.find(uri) != runningRecorders.end())
//     {
//         cout << uri << " is already recording. PID: " << runningRecorders[uri] << endl << endl;;
//         return false;
//     }
//     pid_t pid = fork();
    
//     if (pid == -1) {
//         cerr << "Couldn't fork" << endl;
//         perror("fork");
//     }

//     /* Create file name if not specified */
//     if (fileName == "")
//         {
//             string stream = uri.substr(uri.find("@") + 1);
//             char datetime[18];
//             time_t t = time(nullptr);
//             strftime(datetime, sizeof(datetime), "%d-%m-%Y_%H:%M", localtime(&t));
//             fileName = stream + "_" + string(datetime) + ".mp4"; 
//         }
//     fileNames[uri] = fileName;

//     if (pid == 0) {
//         // string logsFile = config->getParam("saveToFolder") + "logs/" + to_string(getpid()) + ".logs";

//         // int fd = open(logsFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

//         // if(fd == -1) {
//         //     perror("logs");
//         // }

//         // dup2(fd, 1);
//         // dup2(fd, 2);

//         execlp("gst-launch-1.0", "-e", "--quiet", "rtspsrc", ("location=" + uri).c_str(),
//        "protocols=tcp", "!", "rtph264depay", "name=vdepay", "!", "mpegtsmux", "name=mux", 
//        "!", "filesink", ("location=" + config->getParam("saveToFolder") + fileName).c_str(), nullptr);

//     }
//     else
//     {
//         runningRecorders[uri] = pid;
//         cout << "Recording of " << uri << " started. Proccess id: " << pid << endl << endl;
//     }

//     return pid;
// }

bool Recorder::startRecording(Camera *cam, string fileName)
{
    /* Check if recording is already running */
    if (runningRecordings.find(cam) != runningRecordings.end())
    {
        return true;
    }

        /* Create file name if not specified */
    if (fileName == "")
    {
        string stream = cam->uri.substr(cam->uri.find("@") + 1);
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
    Recording *recording = new Recording(cam->uri, config->getParam("saveToFolder"), fileName, timeout_ns);
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
    // TODO: Add rec image widget control here
    auto *runningRecordings = (map<Camera*, Recording*>*) data;
    for (auto it = runningRecordings->cbegin(); it != runningRecordings->end();)
    {
        /* If any recording has this status, it needs to be deleted, and it's video -- uploaded*/
        if (it->second->getStatus() == STOPPED)
        {
            cerr << "Recording of " << it->second->uri << " stopped." << endl;
            string fileName = it->second->getFileName();
            cerr << "File name: " << fileName << endl << endl;

            // Turn off recImage widget
            gtk_widget_hide(it->first->recImage);

            delete it->second;
            it = runningRecordings->erase(it);

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
    return true;
}
