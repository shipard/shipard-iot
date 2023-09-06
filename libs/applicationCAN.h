#ifndef SHP_APPLICATION_CAN_H
#define SHP_APPLICATION_CAN_H


class ApplicationCAN : public Application
{
	public:

		ApplicationCAN();

		virtual void checks();
		virtual void setup();
		virtual void loop();
		virtual void doFwUpgradeRequest(String payload);
		virtual boolean publish(const char *payload, const char *topic = NULL);
		virtual void publishData(uint8_t sendMode);

	public:

		ShpClientCAN *m_client;
		//bool m_wakeUpFromSleep;

};

#endif
