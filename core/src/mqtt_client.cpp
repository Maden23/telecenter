#include "mqtt_client.h"

MqttClient::MqttClient(string broker, string client_id, vector<string> topics)
{
	this->topics = topics;

	mqtt::connect_options connOpts;
	connOpts.set_keep_alive_interval(20);
	connOpts.set_clean_session(true);

	client = new mqtt::async_client(broker, client_id);
	callback cb(*client, connOpts, topics);
	client->set_callback(cb);

	// Start the connection.
	// When completed, the callback will subscribe to topics.
	try {
		std::cout << "Connecting to the MQTT server..." << std::flush;
		client->connect(connOpts, nullptr, cb);
	}
	catch (const mqtt::exception&) {
		std::cerr << "\nERROR: Unable to connect to MQTT server: '"
			<< broker << "'" << std::endl;
	}

}

MqttClient::~MqttClient()
{
	// Disconnect
	try {
		std::cout << "\nDisconnecting from the MQTT server..." << std::flush;
		client->disconnect()->wait();
		std::cout << "OK" << std::endl;
	}
	catch (const mqtt::exception& exc) {
		std::cerr << exc.what() << std::endl;
	}

	delete client;
}