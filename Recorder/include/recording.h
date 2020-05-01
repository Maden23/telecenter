#include <string>
#include <gst/gst.h>
#include <iostream>
#include <vector>

using namespace std;

enum status_t
{
    RUNNING, RELINKING, STOPPING, STOPPED
};

/*!
 * \brief The Recording class
 * @ingroup recorder
 */
class Recording
{
public:
    Recording(string uri, string folder, string camName, long timeout, long videoTimeLimit);
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
    long timeout, videoTimeLimit;
	int freeze_check_id;
	bool streamLinked;

    GstElement *pipeline, *src, *testsrc, *depay, *parse, *streamcapsfilter, *mux, *sink;
    GstElement *enc, *fakesink, *testcapsfilter;
	GstClock *clock;
	GstClockTime lastBufferTime; // time of last played data buffer in nanoseconds

	bool buildPipeline();

	// Dynamic source linking
	static void pad_added_handler (GstElement * src, GstPad * new_pad, Recording *recording);

	// Handelling bus messages
	static GstBusSyncReply busSyncHandler (GstBus *bus, GstMessage *message, Recording *recording);

	// Probes for dynamic pipeline change
    static GstPadProbeReturn probe_block_stream(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn probe_eos_in_stream(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn probe_idle_relink(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    gint in_idle_probe; // to make sure idle probe is called in one thread

    // Callbacks to catch a freeze
	static GstPadProbeReturn data_probe (GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
	static gboolean freeze_check(gpointer user_data);

    // Relinkingg elements
    static bool elementSrcLinked(GstElement *elem);
    static bool elementSinkLinked(GstElement *elem);
    static bool relinkElements(GstElement *wrong_src, GstElement *right_src, GstElement* sink);

};
