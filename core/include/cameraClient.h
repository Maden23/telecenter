#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


using namespace std;

enum cameraCommand_t {STOP, LEFT, RIGHT, UP, DOWN};

class CameraClient
{
public:
    CameraClient(string server_ip, int server_port);
    ~CameraClient();
    bool connectToCamera(string ip, int port, string login, string password);
    bool commandToCamera(cameraCommand_t command, float velocity = 1.0, float timeout = 0.5);
    void disconnectFromCamera();
private:
    int sock = 0;
    bool connected;
};

