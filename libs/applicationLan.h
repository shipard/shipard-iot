#ifndef SHP_APPLICATION_LAN_H
#define SHP_APPLICATION_LAN_H


class ApplicationLan : public Application
{
	public:

		ApplicationLan();

		virtual void init();
		virtual void init2IOPorts();
		virtual void checks();
		virtual void setup();
		virtual void loop();

		virtual void doFwUpgradeRequest(String payload);
		virtual boolean publish(const char *payload, const char *topic = NULL);
		virtual void publishData(uint8_t sendMode, const char *payload = NULL);

		virtual void loadBoxConfig();

		#ifdef SHP_MQTT
		void checkMqtt();
		void onMqttMessage(const char* topic, byte* payload, unsigned int length);
		#endif

		void initNetworkInfo();
		void IP_Got();
		void IP_Lost();

		virtual int getDeviceCfg(uint8_t *hwId, String& data);

		virtual void checkBeforeSleep();

	public:

		String m_cfgServerHostName;
		String m_mqttServerHostName;

		WiFiClient lanClient;
		#ifdef SHP_MQTT
		PubSubClient *mqttClient;
		#endif

		volatile boolean m_networkInfoInitialized;
		unsigned long m_mqttReconnectAttempAfter;
		unsigned long m_loadConfigAfter;
		static bool eth_connected;
		IPAddress ipLocal;

		#ifdef SHP_WIFI
		ShpWiFiConnector *m_wifiConnector;
		#endif


};



#endif

