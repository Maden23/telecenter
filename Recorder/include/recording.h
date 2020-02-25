#include <string>
#include <gst/gst.h>
#include <iostream>

using namespace std;

enum status_t
{
	RUNNING, STOPPING, STOPPED
};

class Recording
{
public:
	Recording(string uri, string folder, string fileName, long timeout);
	~Recording();

	string getFileName() { return fileName; }
	status_t getStatus() { return status; }

	bool start();
	bool stop();

	bool showTest();
	bool showStream();
	bool elementSrcLinked(GstElement *elem);
	bool elementSinkLinked(GstElement *elem);
	bool relinkElements(GstElement *wrong_src, GstElement *right_src, GstElement* sink);
	status_t status;
	
	string uri, fileName, folder;
private:
	long timeout;
	int freeze_check_id;
	bool streamLinked;

	GstElement *pipeline, *src, *testsrc, *depay, *parse, *mux, *sink;
	GstElement *enc, *fakesink, *capsfilter;
	GstClock *clock;
	GstClockTime lastBufferTime; // time of last played data buffer in nanoseconds

	bool buildPipeline();

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording);

	// Handelling bus messages
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording);

	// Probes for dynamic pipeline change
	static GstPadProbeReturn eos_drop_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static GstPadProbeReturn block_and_show_stream_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static GstPadProbeReturn block_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

	// Callbacks to catch a freeze
	static GstPadProbeReturn data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static gboolean freeze_check(gpointer user_data);

};