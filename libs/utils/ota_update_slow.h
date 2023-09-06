#ifndef SHP_OTA_UPDATE_SLOW_H
#define SHP_OTA_UPDATE_SLOW_H




#define OTA_SLOW_WRITE_BUFFER_SIZE 3840

#define OTA_SLOW_PHASE_NONE								0
#define OTA_SLOW_PHASE_INITIALIZED				1
#define OTA_SLOW_PHASE_INCOMING_PACKETS		5
#define OTA_SLOW_PHASE_NEED_WRITE_BLOCK		7
#define OTA_SLOW_PHASE_WAIT_FOR_REBOOT		9


#define OTA_SLOW_NEXT_OP_NONE							0
#define OTA_SLOW_NEXT_OP_NEXT_BLOCK				1

class ShpOTAUpdateSlow : public ShpUtility
{
	public:

		ShpOTAUpdateSlow();

		uint8_t setUpdateInfo(String payload);
		virtual void begin();
		virtual void end();

		void setBlockNumber(uint8_t blockNumber);
		void addPacket(int dataLen, uint8_t *packetData);
		uint8_t checkUpdateProgress();

	public:

		uint8_t m_blockNumberToDownload;
		int m_blockNumberToDownloadSize;
		uint8_t m_phase;

	protected:

		void writeBlock();

	protected:

		int m_fwSize;
		int m_incomingPackets;
		int m_incomingBytes;

		uint8_t m_blockNumber;
		uint16_t m_packetInBlockNumber;

		String m_fwUrl;
		int m_writeBufferPos;
		uint8_t	m_writeBuffer[OTA_SLOW_WRITE_BUFFER_SIZE];

};


#endif
