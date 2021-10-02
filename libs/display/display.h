#ifndef SHP_DISPLAY_H
#define SHP_DISPLAY_H

#define DISPLAY_QUEUE_LEN 8

class ShpDisplay : public ShpIOPort 
{
	public:
		ShpDisplay();

		virtual void loop();
		void onMessage(const char* topic, const char *subCmd, byte* payload, unsigned int length);

protected:

		struct QueueItem
		{
			int8_t qState;
			unsigned long startAfter;
			char subCmd[SHP_IOPORT_SUBCMD_MAX_LEN + 1];
			String payload;
		};


		int m_queueRequests;
		QueueItem m_queue[DISPLAY_QUEUE_LEN];

		void addQueueItem(const char *subCmd, byte* payload, unsigned int length, unsigned long startAfter);
		virtual void runQueueItem(int i);

};


#endif
