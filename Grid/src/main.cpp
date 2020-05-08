#include "config.h"
#include <iostream>
#include <string>
#include <cstdio>
#include "gridUI.h"

using namespace std;

int main()
{
    freopen("grid.log","w", stdout);
    freopen("grid_err.log","w", stderr);

	Config *config = &Config::get();
    config->setFile("grid.conf");

	GridUI ui(config);
}
