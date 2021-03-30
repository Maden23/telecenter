#include "recManager.h"

RecManager::RecManager(Config *config)
{
    this->config = config;
    // Verify path
    savePath = config->getParam("saveToFolder");
    if (savePath[savePath.length() - 1] != '/') 
    {
        savePath.append("/");
    }
    struct stat info;
    if(stat(savePath.c_str(), &info) != 0 )
    {
        cerr << "Cannot access " << savePath << endl;
        exit(1);
    }

    // Start regular check on recordings 
    g_timeout_add(500, handleRecordingStatus, this);
}

RecManager::~RecManager()
{
    for (auto &rec : runningRecordings)
    {
        cerr << "Stopping recording of " << rec.first->fullName << endl;

        rec.second->stop();
        delete rec.second;
    }
}


bool RecManager::startRecording(Source *source)
{
    /* Check if recording is already running */
    if (runningRecordings.find(source) != runningRecordings.end())
    {
        return true;
    }
   
    /* Get video files time limit from config file */
    long limit_s = config->getParamInt("timeLimitSeconds");
    if (limit_s == -1)
    {
        cerr << "timeLimitSeconds not found. Default = 0" << endl << endl;
        limit_s = 0;
    }
    long limit_ns = limit_s * 1000000000;

    /* Attempt to start recording */ 
    Recording *recording = new Recording(source->getType(), source->uri, savePath, source->fullName, limit_ns);
    if (!recording->start())
    {
        cerr << "Failed to start recording of " << source->uri << endl << endl;
        return false;
    }
    runningRecordings[source] = recording;
    cerr << "Started recording of " << source->uri << endl << endl;
    return true;
}

bool RecManager::stopRecording(Source *source)
{
    cerr << "Trying to stop recording of " << source->uri << "." << endl << endl;

    if (runningRecordings.find(source) == runningRecordings.end())
    {
        cerr << "failed" << endl;
        cerr << "No recording found" << endl << endl;
        return false;
    }

    runningRecordings[source]->stop();

    return true;
}


bool RecManager::stopAll()
{
    for (auto rec : runningRecordings)
    {
        stopRecording(rec.first);
    }
}

void* RecManager::uploadFileAsync(gpointer uploadFileAsyncData)
{
    auto data = (uploadFileAsyncData_t*) uploadFileAsyncData;
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

gboolean RecManager::handleRecordingStatus(gpointer recManager_ptr)
{
    auto *recManager = (RecManager*) recManager_ptr;
    // Using iterators to be able to delete items
    bool deleted_something;
    for (auto it = recManager->runningRecordings.cbegin(); it != recManager->runningRecordings.end();)
    {
        deleted_something = false;
        /* If any recording has this status, it needs to be deleted, and it's file -- uploaded*/
        if (it->second->getStatus() == STOPPED_MANUALLY)
        {
            cerr << "Recording of " << it->second->uri << " stopped." << endl;
            vector<string> files = it->second->getFiles();
            for (auto file : files)
            {
                cerr << "File name: " << file << endl << endl;
            }
            // Turn off recImage widget for video sources
            if (it->first->getType() == VIDEO)
            {
                Camera* cam = static_cast<Camera*>(it->first);
                gtk_widget_hide(cam->recImage);
            }

            // Upload files in new thread
            uploadFileAsyncData_t *data = new uploadFileAsyncData_t();
            *data = {.runningGDriveUploads = &(recManager->runningGDriveUploads),
                     .files = files};
            g_thread_new(it->first->name.c_str(), uploadFileAsync, data);

            // Delete Recording
            delete it->second;
            it = recManager->runningRecordings.erase(it);
            deleted_something = true;

        }
        else if (it->second->getStatus() == STOPPED_ERROR)
        {
            // Change recImage widget for video sources
            if (it->first->getType() == VIDEO)
            {
                Camera* cam = static_cast<Camera*>(it->first);
                gtk_image_set_from_icon_name(GTK_IMAGE(cam->recImage), "face-sad", GTK_ICON_SIZE_DIALOG);
                
            }
            it->second->requestRestart();               
        }
        else if (it->second->getStatus() == STARTING)
        {
            // Change recImage widget for video sources
            if (it->first->getType() == VIDEO)
            {
                Camera* cam = static_cast<Camera*>(it->first);
                gtk_image_set_from_icon_name(GTK_IMAGE(cam->recImage), "widget-gtk-spinner", GTK_ICON_SIZE_DIALOG);
                
                gtk_widget_show(cam->recImage);

            }
        }
        else if (it->second->getStatus() == RUNNING)
        {
            // Change recImage widget for video sources
            if (it->first->getType() == VIDEO)
            {
                Camera* cam = static_cast<Camera*>(it->first);
                gtk_image_set_from_icon_name(GTK_IMAGE(cam->recImage), "gtk-media-record", GTK_ICON_SIZE_DIALOG);
                
            }
        }
        if (!deleted_something)
        { 
            it++;
        }
    }
    return true;
}
