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

}
