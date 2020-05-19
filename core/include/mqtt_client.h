#include "mqtt_classes.h"

using namespace std;

class MqttClient
{
public:
    MqttClient(string broker, string client_id, vector<string> topics);
    ~MqttClient();

    void subscribe(string topic);
    void unsubscribe (string topic);
private:
    mqtt::async_client *client;
    vector<string> topics;
};
