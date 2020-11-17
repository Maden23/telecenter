#include <string>
#include <gst/gst.h>
#include <iostream>
#include <vector>
#include "room.h"

using namespace std;

enum status_t
{
    RUNNING, STOPPING, STOPPED
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

	status_t status;
	sourceType_t sourceType; 
	
    string uri, sourceName, fileNamePattern, folder;
private:
    vector<string> files;
    long timeLimit;

    GstElement *pipeline, *src, *depay, *parse, *muxsink;

	bool buildPipeline();

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording);

	// Handelling bus messages
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording);

};
