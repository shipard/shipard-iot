#ifndef SHP_APPLICATION_LAN_H
#define SHP_APPLICATION_LAN_H


class ApplicationLan : public Application
{
	public:

		ApplicationLan();

		virtual void init();
		virtual void checks();
		virtual void setup();
		virtual void loop();

		virtual boolean publish(const char *payload, const char *topic = NULL);
		virtual void publishData(uint8_t sendMode);

		virtual void loadBoxConfig();

		#ifdef SHP_MQTT
		void checkMqtt();
		void onMqttMessage(const char* topic, byte* payload, unsigned int length);
		#endif

		void initNetworkInfo();
		void IP_Got();
		void IP_Lost();

	public:

		WiFiClient lanClient;
		#ifdef SHP_MQTT
		PubSubClient *mqttClient;
		#endif

		boolean m_networkInfoInitialized;
		static bool eth_connected;
		IPAddress ipLocal;

};



#endif

