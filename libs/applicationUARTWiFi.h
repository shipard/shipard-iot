#ifndef SHP_APPLICATION_UART_WIFI_H
#define SHP_APPLICATION_UART_WIFI_H


class ApplicationUARTWiFi : public ApplicationLan
{
	public:

		ApplicationUARTWiFi();

		//virtual void init();
		virtual void checks();
		virtual void setup();
		virtual void loop();

  public:

    bool m_wifiInitialized;
    String m_wifiSSID;
    String m_wifiPassword;

};

#endif
