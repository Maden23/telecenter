#include <cameraClient.h>
#include <errno.h>

CameraClient::CameraClient(string server_ip, int server_port)
{
    connected = false;

    // Creating a socket
    char recvBuff[1024];
    memset(recvBuff, '0',sizeof(recvBuff));
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "Error : Could not create socket " << endl << endl;
    }
    // Creating struct with address data
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if(inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr)<=0)
    {   
        cerr << "inet_pton error" << endl << endl;
    }

    // Connecting
    if( connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        int err = errno;
        cerr << "Error: Socket connection failed " << err << endl << endl;
    }
    else
    {
        connected = true;
    }
}

CameraClient::~CameraClient()
{
    disconnectFromCamera();
    close(sock);
}

bool CameraClient::connectToCamera(string ip, int port, string login, string password)
{
    char buff[1024];
    int len;
    if (!connected)
    {
        cerr << "Error while connecting to camera: Not connected to server" << endl << endl;
        return false;
    }

    string info = ip + " " + to_string(port) + " " + login + " " + password;
    unsigned int out_bytes = 0;
    int n;
    while(out_bytes < info.length())
    {
        n = write(sock, info.c_str(), info.length());
        if (n < 0)
        {
            cerr << "Error: Could not send data to camera comtrolling server" << endl << endl;
            return false;
        }
        else if (n == 0)
        {
            cerr << "Error: Camera controlling server has closed the connection" << endl << endl;
            close(sock);
            connected = false;
            return false;
        }
        else
        {
            out_bytes += n;
        }
    }
  
    if ((len = read(sock, &buff, sizeof(buff)-1)) > 0)
    {
        buff[len] = 0; // end of line
        if (0 == strcmp(buff, "You are connected to camera"))
        {   
            cout << "Connected to camera " << ip << " for sending commands" << endl << endl;
        }
    }
    if (len < 0)
    {
        cerr << "Socket reading error" << endl << endl;
        return false;
    }
    return true;
}

bool CameraClient::commandToCamera(cameraCommand_t command, float velocity, float timeout)
{
    if (!connected)
    {
        cerr << "Error while sending command to camera: Not connected to server" << endl << endl;
        return false;
    }
    if (command == STOP)
    {
        string s =  "stop";
        write(sock, s.c_str(), sizeof(s));
        return true;
    }

    char buff[2048];
    string command_s;
    switch (command)
    {
    case STOP:
        command_s = "stop";
        break;
    case UP:
        command_s = "up";
        break;
    case DOWN:
        command_s = "down";
        break;
    case LEFT:
        command_s = "left";
        break;
    case RIGHT:
        command_s = "right";
        break;
    default:
        cerr << "Camera command not supported" << endl << endl;
        return false;
    }

    // buff += " " + to_string(velocity) + " " + to_string(timeout);
    sprintf(buff, "%s %.2f %.2f", command_s.c_str(), velocity, timeout);
    write(sock, buff, strlen(buff));

    char readBuff[1024];
    int len;
    if ((len = read(sock, &readBuff, sizeof(readBuff)-1)) > 0)
    {
        readBuff[len] = 0;
        if(0 != strcmp(readBuff, "Success"))
        {
            cerr << "Error while sending command to camera: " << readBuff << endl << endl;
            return false;
        }
        cout << readBuff << endl;
    }
    return true;
}

void CameraClient::disconnectFromCamera()
{
    if (connected)
    {
        string command = "exit";
        write(sock, command.c_str(), sizeof(command));
    }
}