#include <string>
#include <gst/gst.h>
#include <iostream>
#include <vector>
#include "room.h"

using namespace std;

enum status_t
{
    NEW, STARTING, RUNNING, STOPPING_MANUALLY, STOPPED_ERROR, STOPPED_MANUALLY, 
};

/*!
 * \brief The Recording class
 * @ingroup recorder
 */
class Recording
{
public:
    Recording(sourceType_t sourceType, string uri, string folder, string sourceName, long timeLimit);
	~Recording();

    string getFileNamePattern() { return fileNamePattern; }
    vector<string> getFiles() { return files; }
	status_t getStatus() { return status; }

	bool start();
	bool stop();
	// Restart recording. It will not restart if it was just restarted resently.
	void requestRestart();

	status_t status;
	sourceType_t sourceType; 
	
    string uri, sourceName, fileNamePattern, folder;
private:
    vector<string> files;
    long timeLimit;

	gint fileIndex = 0; 	    
	
	GTimer *timer;
	gdouble lastRestartTime = -1;

    GstElement *pipeline, *src, *depay, *parse, *muxsink;

	bool buildPipeline();

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording);

	// Handelling bus messages
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording);

};
