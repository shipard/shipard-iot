#ifndef SHP_DISPLAY_NEXTION_H
#define SHP_DISPLAY_NEXTION_H

#define MAX_DISPLAY_NEXTION_BUF_LEN 500


class ShpDisplayNextion : public ShpDisplay
{
	public:
		ShpDisplayNextion();

		virtual void init(JsonVariant portCfg);
		virtual void loop();

		void initDisplay();

		void doCommand(const char *cmd);
		void doCommand_Set(const char *itemId, const char *text);
		void updateFirmware(const char *params);
		void setDimLevel(uint8_t level);


		void readCommandResult();
		void waitForCommandResult();
		void waitForConfirm();

		virtual void runQueueItem(int i);

		void addEscapedText(String &dst, const char *text);


	private:

		int m_speed;
		uint32_t m_mode;
		int8_t m_rxPin;
		int8_t m_txPin;

		uint8_t m_currentDimLevel;
		uint8_t m_defaultDimLevel;

		HardwareSerial *m_hwSerial;

		uint8_t m_buffer[MAX_DISPLAY_NEXTION_BUF_LEN];
		int m_sbCnt;
		int m_bufNeedSend;

		String m_displayType;
		String m_displaySN;
};


#endif
