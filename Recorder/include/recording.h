#include <string>
#include <gst/gst.h>
#include <iostream>
#include <vector>

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
    Recording(string uri, string folder, string camName, long videoTimeLimit);
	~Recording();

    string getFileNamePattern() { return fileNamePattern; }
    vector<string> getFiles() { return files; }
	status_t getStatus() { return status; }

	bool start();
	bool stop();

	status_t status;
	
    string uri, camName, fileNamePattern, folder;
private:
    vector<string> files;
    long videoTimeLimit;

    GstElement *pipeline, *src, *depay, *parse, *sink;

	bool buildPipeline();

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording);

	// Handelling bus messages
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording);

};
