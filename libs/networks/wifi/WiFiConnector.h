#ifndef SHP_WIFI_CONNECTOR_H
#define SHP_WIFI_CONNECTOR_H


class ShpWiFiConnector
{
	public:

		ShpWiFiConnector();

		virtual void start();
		virtual void stop();
		virtual void loop();

		void setWiFiSSID(uint8_t index);

	protected:

		StaticJsonDocument<2048> m_wifiConfig;

};


#endif
