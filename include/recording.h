#include <string>
#include <vector>
#include <gst/gst.h>
#include <iostream>

using namespace std;

enum status_t
{
	RUNNING, RESTARTING, STOPPING, STOPPED
};

class Recording
{
public:
	Recording(string uri, string folder, long timeout);
	~Recording();

	vector<string> getFiles() { return files; }
	status_t getStatus() { return status; }

	bool start();
	bool stop();
	void restart();

	status_t status;
	
	string uri, fileName, folder;
	vector<string> files;

	GstElement *pipeline, *src, *depay, *parse, *mux, *sink;
	int freeze_check_id;

private:
	long timeout;

	GstClock *clock;
	GstClockTime lastBufferTime; // time of last played data buffer in nanoseconds

	bool buildPipeline();

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording);

	// Handelling bus messages
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording);

	// Probes for dynamic pipeline change
	static GstPadProbeReturn eos_drop_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static GstPadProbeReturn block_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

	// Callbacks to catch a freeze
	static GstPadProbeReturn data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static gboolean freeze_check(gpointer user_data);

};