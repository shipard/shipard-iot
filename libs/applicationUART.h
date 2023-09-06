#ifndef SHP_APPLICATION_UART_H
#define SHP_APPLICATION_UART_H


class ApplicationUART : public Application
{
	public:

		ApplicationUART();

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

};

#endif
