#include <iostream>
#include <string>
#include <cstdio>
#include "ui.h"
#include "config.h"
#include <signal.h>

using namespace std;

UI ui;

void intHandler (int dummy)
{
	cout << "Got SIGINT" << endl;
	ui.stop();
	exit(0);
}

int main()
{
	signal(SIGINT, intHandler);

	freopen("recorder.log","w", stdout);
	freopen("recorder_err.log","w", stderr);

	Config *config = &Config::get();
	config->parseFile("recorder.conf");

	ui.init(config);
}