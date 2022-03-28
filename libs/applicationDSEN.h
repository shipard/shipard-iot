#ifndef SHP_APPLICATION_DSEN_H
#define SHP_APPLICATION_DSEN_H


class ApplicationDSEN : public Application
{
	public:

		ApplicationDSEN();

		//virtual void init();
		virtual void checks();
		virtual void setup();
		virtual void loop();
		virtual boolean publish(const char *payload, const char *topic = NULL);
		virtual void publishData(uint8_t sendMode);

		bool setIotBoxFromStoredCfg();

		//virtual boolean publish(const char *payload, const char *topic = NULL);
		//virtual void publishData(uint8_t sendMode);

		//virtual void loadBoxConfig();

	public:

		ShpEspNowClient *m_espClient;
		bool m_wakeUpFromSleep;

};

#endif
