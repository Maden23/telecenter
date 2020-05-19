#include <iostream>
#include <string>
#include <cstdio>
#include "singleStreamUI.h"
#include "mqtt_client.h"


using namespace std;

int main()
{
	freopen("single_stream.log","w", stdout);
	freopen("single_stream_err.log","w", stderr);
    try
    {
        /* code */
        MqttClient mqtt("tcp://localhost:1883", "singlestream", {"test", "cams"});
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    

    SingleStreamUI *ui = new SingleStreamUI();
    delete ui; //after gtk_main_quit
}
