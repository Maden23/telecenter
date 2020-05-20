#include "mqtt_client.h"

MqttClient::MqttClient(string broker, string client_id, vector<string> topics)
{
	this->topics = topics;

	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(20);
	connOpts.set_clean_session(true);
        client = new mqtt::client(broker, "");


	// Start the connection.
	// When completed, the callback will subscribe to topics.
	try {
                cout << "Connecting to the MQTT server..." << flush;
                client->connect(connOpts);
                cout << "OK" << endl;
	}
	catch (const mqtt::exception&) {
                cerr << "\nERROR: Unable to connect to MQTT server: '"
                        << broker << "'" << endl;
	}

        for (auto t : topics)
        {
            client->subscribe(t);
            cout << "Subscribed to topic: " << t << endl;
        }

}

MqttClient::~MqttClient()
{
	// Disconnect
	try {
                cout << "\nDisconnecting from the MQTT server..." << flush;
                client->disconnect();
                cout << "OK" << endl;
	}
	catch (const mqtt::exception& exc) {
                cerr << exc.what() << endl;
	}

}

void MqttClient::publish(string topic, string message)
{
    if (!client->is_connected())
    {
        cout << "Lost MQTT connection. Attempting reconnect" << endl;
        if (tryReconnect(client))
        {
            for (auto t : topics)
            {
                client->subscribe(t, 1); //topic, qos
            }
            cout << "Reconnected MQTT" << endl;
        }
        else
        {
             cout << "MQTT Reconnect failed." << endl;
             return;
        }
    }
    cout << "Sending MQTT message to " << topic << ":" << endl
         << message << endl << endl;
    client->publish(mqtt::message(topic.c_str(), message.c_str(), message.length()+1));
}

bool MqttClient::tryReconnect(mqtt::client* cli)
{
    constexpr int N_ATTEMPT = 30;

    for (int i=0; i<N_ATTEMPT && !cli->is_connected(); ++i) {
        try {
            cli->reconnect();
            return true;
        }
        catch (const mqtt::exception&) {
            this_thread::sleep_for(chrono::seconds(1));
        }
    }
    return false;
}


void MqttClient::passMessagesToQueue(GAsyncQueue *q)
{
    if (!q)
    {
        cerr << "Queue not provided for MQTT messages" << endl << endl;
        return;
    }

    // Receive messages
    while (true)
    {/*
        // Reconnecting if client disconnected
        if (!client->is_connected())
        {
            cout << "Lost MQTT connection. Attempting reconnect" << endl;
            if (tryReconnect(client))
            {
                for (auto t : topics)
                {
                    client->subscribe(t, 1); //topic, qos
                }
                cout << "Reconnected MQTT" << endl;
                continue;
            }
            else
            {
                 cout << "MQTT Reconnect failed." << endl;
            }
        }*/

        // Wait till message received
        auto msg = client->consume_message();
        if (!msg)
        {

            // Reconnecting if client disconnected
            if (!client->is_connected())
            {
                cout << "Lost MQTT connection. Attempting reconnect" << endl;
                if (tryReconnect(client))
                {
                    for (auto t : topics)
                    {
                        client->subscribe(t, 1); //topic, qos
                    }
                    cout << "Reconnected MQTT" << endl;
                    continue;
                }
                else
                {
                     cout << "MQTT Reconnect failed." << endl;
                }
            }
            else
            {
               cout << "An error occurred retrieving messages." << endl;
            }
            break;
        }

        // post message to queue
        auto msg_data = new QueueMQTTMessage;
        msg_data->topic = msg->get_topic(), // topic
        msg_data->message = msg->to_string();  // message
        g_async_queue_push(q, msg_data);
    }
}
