#ifndef SHP_APPLICATION_H
#define SHP_APPLICATION_H

#include "consts.h"
#include "versionId.h"
#include "ioPort.h"

#define APP_CMD_QUEUE_LEN 8
#define cqsFree 0
#define cqsLocked 1
#define cqsDoIt 2
#define cqsRunning 3

#define shpllNONE 0
#define shpllError 1
#define shpllStatus 2
#define shpllWarning 3
#define shpllInfo 4
#define shpllNotice 5
#define shpllDebug 6
#define shpllVerbose 7
#define shpllALL 7

static const char* const SHP_LOG_LEVEL_NAMES[shpllALL] = 
{
	"ERR", 				// shpllError 
	"STATUS", 		// shpllStatus
	"WARN", 			// shpllWarning
	"INFO", 			// shpllInfo
	"NOTICE", 		// shpllNotice
	"DEBUG", 			// shpllDebug
	"DEBUG-ALL" 	// shpllverbose
};

#define IOT_BOX_INFO_VALUES "_states"
#define SM_NONE 0
#define SM_NOW 1
#define SM_LOOP 2


#ifdef SHP_GSM
class ShpModemGSM;
#endif



class Application {

	public:

		Application();

		virtual void checks();

		virtual void init();

		void initIOPorts();
		void init2IOPorts();
		void addIOPort(ShpIOPort *ioPort, JsonVariant portCfg);
		ShpIOPort *ioPort(const char *portId);

		virtual void setup();
		virtual void loadBoxConfig();
		virtual void loop();
		virtual void setIotBoxCfg(String data);

		void addCmdQueueItemFromMessage(const char* commandId, byte* payload, unsigned int length);
		void addCmdQueueItem(const char *commandId, String payload, unsigned long startAfter);
		void runCmdQueueItem(int i);
		void doSet(byte* payload, unsigned int length);

		void reboot();
		void setLogLevel(const char *logLevel);

		void log(uint8_t msgClass, const char* format, ...);
		void log(const char *msg, uint8_t level);
		void iotBoxInfo();

		virtual boolean publish(const char *payload, const char *topic = NULL);
		void setValue(const char *key, const char *value, uint8_t sendMode);
		void setValue(const char *key, const int value, uint8_t sendMode);
		void setValue(const char *key, const float value, uint8_t sendMode);
		virtual void publishData(uint8_t sendMode);
		void publishAction(const char *key, const char *value);
		void publishAction(const char *key, const int value);
		void publishAction(const char *key, const float value);

		void signalLedOn();
		void signalLedOff();
		void signalLedBlink(int count);


	public:

		uint8_t m_logLevel;
		int m_pinLed;
		
		#ifdef SHP_GSM
		ShpModemGSM *m_modem;
		#endif

		String cfgServerHostName;
		String mqttServerHostName;
		String macHostName;

		String m_deviceTopic;
		String m_deviceSubTopic;

		StaticJsonDocument<16384> m_boxConfig;
		boolean m_boxConfigLoaded;

		ShpIOPort **m_ioPorts;
		uint8_t m_countIOPorts;

	public:

		uint8_t m_signalLedState;

		Preferences m_prefs;
		String m_deviceId;
		String m_logTopic;

		struct CmdQueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			String commandId;
			String payload;
		};

		int m_cmdQueueRequests;
		CmdQueueItem m_cmdQueue[APP_CMD_QUEUE_LEN];

protected:

		StaticJsonDocument<4096> m_iotBoxInfo;
		bool m_publishDataOnNextLoop;

};


#endif
