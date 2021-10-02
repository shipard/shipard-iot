#ifndef APPLICATION_H
#define APPLICATION_H

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


#ifdef SHP_GSM
class ShpModemGSM;
#endif



class Application {

	public:

		Application();

		void checks();
		void checkMqtt();

		void init();

		void initIOPorts();
		void init2IOPorts();
		void addIOPort(ShpIOPort *ioPort, JsonVariant portCfg);
		ShpIOPort *ioPort(const char *portId);

		void setup();
		void initNetworkInfo();
		void loadBoxConfig();
		void loop();
		void IP_Got();
		void IP_Lost();

		void onMqttMessage(const char* topic, byte* payload, unsigned int length);
		void addCmdQueueItemFromMessage(const char* commandId, byte* payload, unsigned int length);
		void addCmdQueueItem(const char *commandId, String payload, unsigned long startAfter);
		void runCmdQueueItem(int i);

		void reboot();
		void setLogLevel(const char *logLevel);

		void log(uint8_t msgClass, const char* format, ...);
		void log(const char *msg, uint8_t level);
		void iotBoxInfo();

		boolean publish(const char *payload, const char *topic = NULL);

		void signalLedOn();
		void signalLedOff();
		void signalLedBlink(int count);


	public:

		static bool eth_connected;

		uint8_t m_logLevel;
		int m_pinLed;
		
		#ifdef SHP_GSM
		ShpModemGSM *m_modem;
		#endif

		WiFiClient lanClient;
		#ifdef SHP_MQTT
		PubSubClient *mqttClient;
		#endif

		IPAddress ipLocal;


		boolean m_networkInfoInitialized;
		String cfgServerHostName;
		String mqttServerHostName;
		String macHostName;

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

};


#endif
