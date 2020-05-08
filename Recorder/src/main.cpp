#include <iostream>
#include <string>
#include <cstdio>
#include "recorderUI.h"
#include "config.h"

using namespace std;

int main()
{
	freopen("recorder.log","w", stdout);
	freopen("recorder_err.log","w", stderr);

	Config *config = &Config::get();
	config->setFile("recorder.conf");

    RecorderUI ui(config);


	// Recorder recorder(config);

	// recorder.startRecording(config->getParam("cam51"));
	// recorder.startRecording(config->getParam("cam52"));
	// recorder.startRecording(config->getParam("cam53"));
	// recorder.startRecording(config->getParam("cam54"));


	// Player player;
	// player.playStream(config->getParam("cam51"));


	// for (auto &rec : recorder.getRunningRecorders())
	// {
	// 	int status;
	// 	while(waitpid(rec.second, &status, 0) == -1);
	// }
}
