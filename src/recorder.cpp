#include "recorder.h"

Recorder::Recorder(Config *config)
{
    this->config = config;
}

Recorder::~Recorder()
{
    for (auto &rec : runningRecorders)
    {
        cout << "Stopping recording of " << rec.first << endl;
        kill(rec.second, SIGINT);
        waitpid(rec.second, nullptr, 0);
    }
}

pid_t Recorder::startRecording(string uri, string fileName)
{
    if (runningRecorders.find(uri) != runningRecorders.end())
    {
        cout << uri << " is already recording. PID: " << runningRecorders[uri] << endl << endl;;
        return false;
    }
    pid_t pid = fork();
    
    if (pid == -1) {
        cerr << "Couldn't fork" << endl;
        perror("fork");
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
    fileNames[uri] = fileName;

    if (pid == 0) {
        // string logsFile = config->getParam("saveToFolder") + "logs/" + to_string(getpid()) + ".logs";

        // int fd = open(logsFile.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

        // if(fd == -1) {
        //     perror("logs");
        // }

        // dup2(fd, 1);
        // dup2(fd, 2);

        execlp("gst-launch-1.0", "-e", "--quiet", "rtspsrc", ("location=" + uri).c_str(),
       "protocols=tcp", "!", "rtph264depay", "name=vdepay", "!", "mpegtsmux", "name=mux", 
       "!", "filesink", ("location=" + config->getParam("saveToFolder") + fileName).c_str(), nullptr);

    }
    else
    {
        runningRecorders[uri] = pid;
        cout << "Recording of " << uri << " started. Proccess id: " << pid << endl << endl;
    }

    return pid;
}

bool Recorder::stopRecording(string uri)
{
    cout << "Trying to stop recording of " << uri << endl;
    if (runningRecorders.find(uri) == runningRecorders.end())
    {
        cout << "No recording found" << endl << endl;
        cerr << "No running recording process found for " << uri << endl << endl;
        return false;
    }
    kill(runningRecorders[uri], SIGINT);
    waitpid(runningRecorders[uri], nullptr, 0);
    cout << "Recording of " << uri << " stopped." << endl;
    cout << "File name: " << fileNames[uri] << endl << endl;

    runningRecorders.erase(uri);

    uploadVideo(uri);
    return true;
}

void Recorder::uploadVideo(string uri)
{
    string command = "python3 src/video-upload.py";
    command += " " + fileNames[uri];
    command += " " + config->getParam("saveToFolder"); 
    command += " &";

    fileNames.erase(uri);

    cout << "Running " << command << endl << endl;
    system(command.c_str());
}
