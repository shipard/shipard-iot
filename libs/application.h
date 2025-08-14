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

#define bcifUnknown 0
#define bcifNotStoredInFlash 1
#define bcifStoredInFlash 2

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

#define hbLEDMode_NONE 0
#define hbLEDMode_BINARY 1
#define hbLEDMode_PWM 2
#define hbLEDMode_RGB 3

#define hbLEDStatus_Initializing 				0
#define hbLEDStatus_NetworkInitialized 	1
#define hbLEDStatus_NetworkAddressReady 2
#define hbLEDStatus_WaitForCfg					3
#define hbLEDStatus_Running							4
#define hbLEDStatus_Unconfigured				5

#define hbLEDMode_BINARY_STEPS 					2
#define hbLEDMode_RGB_STEPS 						2

class ShpClientUART;

class Application {

	public:

		Application();

		virtual void checks();
		virtual void checkAutoSleep();

		virtual void init();
		void doIncomingMessage(const char* topic, byte* payload, unsigned int length, uint8_t shortMode = 0);
		virtual void doFwUpgradeRequest(String payload);

		void initIOPorts();
		virtual void init2IOPorts();
		void addIOPort(ShpIOPort *ioPort, JsonVariant portCfg);
		ShpIOPort *ioPort(const char *portId);

		virtual void setup();
		virtual void loadBoxConfig();
		virtual void loop();
		virtual void setIotBoxCfg(String data);
		bool setIotBoxFromStoredCfg();

		void saveIotBoxCfg(String cfg, bool rebootNow = false);
		void saveIotBoxCfgWiFi(String cfg, bool rebootNow = false);
		void saveIotBoxCfgServers(String cfg, bool rebootNow = false);

		virtual void subscribeIOPortTopic (uint8_t ioPortIndex, const char *topic);

		void addCmdQueueItemFromMessage(const char* commandId, byte* payload, unsigned int length);
		void addCmdQueueItem(const char *commandId, String payload, unsigned long startAfter);
		void runCmdQueueItem(int i);
		void doSet(byte* payload, unsigned int length);

		void setChargingState(bool charging);


		void doReboot();
		void doSleep();
		virtual void checkBeforeSleep();

		void setLogLevel(const char *logLevel);

		void log(uint8_t msgClass, const char* format, ...);
		void log(const char *msg, uint8_t level);
		void iotBoxInfo();
		void getIotBoxCfg();

		virtual boolean publish(const char *payload, const char *topic = NULL);
		void setValue(const char *key, const char *value, uint8_t sendMode);
		void setValue(const char *key, const int value, uint8_t sendMode);
		void setValue(const char *key, const float value, uint8_t sendMode);
		virtual void publishData(uint8_t sendMode, const char *payload = NULL);
		void publishAction(const char *key, const char *value);
		void publishAction(const char *key, const int value);
		void publishAction(const char *key, const float value);

		virtual int getDeviceCfg(uint8_t *hwId, String& data) {return -1;}

		void doHBLed();
		void setHBLedStatus(uint8_t status);

		void setDSWakeupTimer(int seconds);

	public:

		uint8_t m_logLevel;

		uint8_t m_hbLEDMode;
		uint8_t m_hbLEDPin;
		uint8_t m_hbLedStatus;
		uint8_t m_hbLedStep;
		unsigned long m_hbLedNextCheck;

		#ifdef SHP_GSM
		ShpModemGSM *m_modem;
		#endif

		String macHostName;

		String m_deviceTopic;
		String m_actionTopic;
		//String m_deviceSubTopic;

		StaticJsonDocument<16384> m_boxConfig;
		StaticJsonDocument<2048> m_serversConfig;
		boolean m_boxConfigLoaded;
		uint8_t m_boxConfigInFlash;

		ShpIOPort **m_ioPorts;
		uint8_t m_countIOPorts;
		uint16_t m_routedTopicsCount;

	public:

		//uint8_t m_signalLedState;

		Preferences m_prefs;
		String m_deviceId;
		int16_t m_deviceNdx;
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

		StaticJsonDocument<8192> m_iotBoxInfo;
		bool m_publishDataOnNextLoop;

		struct RoutedTopicItem
		{
			uint8_t ioPortIndex;
			const char *topic;
		};
		RoutedTopicItem *m_routedTopics;

		Adafruit_NeoPixel *m_hbLed;

		uint8_t m_useSerialComm;
		ShpClientUART *m_clientUART;

		bool m_disableAutoSleep;
		bool m_wakeUpFromSleep;
		int m_dsWakeupTimer;

	public:

		boolean m_serverConnected;

		bool m_lowPowerDevice;
		bool m_lowPowerDeviceCharging;
		long m_TotalLoops;
		int m_iotBoxInfoCounter;

		long m_SendIotBoxInfoTimeout;
		long m_SendIotBoxInfoNextSend;

		bool m_autoSleepEnabled;
		bool m_doCheckAutoSleep;

};


#endif
