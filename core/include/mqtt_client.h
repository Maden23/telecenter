#include <mqtt/client.h>
#include <thread>
#include <chrono>
#include <vector>
#include <glib.h>

using namespace std;

struct QueueMQTTMessage
{
    string topic;
    string message;
};

class MqttClient
{
public:
    MqttClient(string broker, string client_id, vector<string> topics);
    ~MqttClient();

    // topics
    void subscribe(string topic);
    void unsubscribe (string topic);

    void publish(string topic, string message);

    void passMessagesToQueue(GAsyncQueue *q);
private:
    mqtt::client *client;
    vector<string> topics;
    bool tryReconnect(mqtt::client* cli);

};
