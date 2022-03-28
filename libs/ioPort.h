#ifndef SHP_IO_PORT_H
#define SHP_IO_PORT_H


#define SHP_IOPORT_SUBCMD_MAX_LEN 30

class ShpIOPort {
	public:
		
		ShpIOPort();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		void log(uint8_t level, const char* format, ...);
		virtual void loop();
		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);
		virtual void shutdown();

	public:

		const char *m_portId;
		String m_valueTopic;
		uint8_t m_appPortIndex;

		bool m_valid;
		bool m_paused;
		unsigned long m_pausedTo;
		
		uint8_t m_sendMode;
		uint8_t m_sendAsAction;
};


#endif
