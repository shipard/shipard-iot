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
		virtual void onMessage(byte* payload, unsigned int length, const char* subCmd);
		virtual void routeMessage(const char* topic, byte* payload, unsigned int length) {};

		int32_t stateLoad(int32_t valueNotExist = -1);
		void stateSave(int32_t state);

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
