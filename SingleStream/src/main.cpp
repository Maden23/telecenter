#include <iostream>
#include <string>
#include <cstdio>
#include "singleStreamUI.h"


using namespace std;

int main()
{
	freopen("single_stream.log","w", stdout);
    freopen("single_stream_err.log","w", stderr);


    SingleStreamUI *ui = new SingleStreamUI();
    delete ui; //after gtk_main_quit
}
