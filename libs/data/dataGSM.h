#ifndef SHP_DATA_GSM_H
#define SHP_DATA_GSM_H

#define MAX_DATA_GSM_SERIAL_BUF_LEN 200
#define MAX_DATA_GSM_SERIAL_RESPONSE_ROWS 20
#define MAX_PHONE_NUMBER_LEN 30
#define DATA_GSM_QUEUE_LEN 5

#define dgaInvalid 0
#define dgaSms 1
#define dgaCall 2


class ShpDataGSM : public ShpIOPort 
{
	public:

		ShpDataGSM();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

		virtual void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);
		void addQueueItem(int8_t action, const char *phoneNumber, String text);

	protected:

		bool sendCmd(const char* cmd, const char *expectedResult = NULL);
		bool readResponse();
		void printResponse();

		void initGSMModule();
		void checkModule();


		bool checkIncomingCall();
		bool checkIncomingSMS();
		bool checkOutgoingCall();

		void hangup();
		bool sendSMS (const char *number, const char *text);
		bool call (const char *number);

		bool searchPhoneNumber(const char *srcText, char *result);
		void runQueueItem(int i);


	private:

		int m_speed;
		uint32_t m_mode;
		int8_t m_rxPin;
		int8_t m_txPin;
		int m_incomingCallMaxRings;
		int m_outgoingCallMaxSecs;

		int m_checkInterval;
		unsigned long m_nextCheck;

		int m_checkModuleInterval;
		unsigned long m_nextModuleCheck;

		HardwareSerial *m_hwSerial;
		bool m_moduleInitialized;

		int m_incomingCallRingInProgress;
		char m_incomingCallPhoneNumber[MAX_PHONE_NUMBER_LEN];

		bool m_outgoingCallInProgress;
		unsigned long m_outgoingCallStartedAt;

		int m_responseRows;
		int m_responseCharPosition;
		char m_response[MAX_DATA_GSM_SERIAL_RESPONSE_ROWS][MAX_DATA_GSM_SERIAL_BUF_LEN];

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			int8_t action;
			char phoneNumber[MAX_PHONE_NUMBER_LEN];
			String text;
		};


		int m_queueRequests;
		QueueItem m_queue[DATA_GSM_QUEUE_LEN];
};


#endif
