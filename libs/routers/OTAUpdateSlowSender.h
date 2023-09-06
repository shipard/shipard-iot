#ifndef SHP_OTA_SLOW_SENDER_H
#define SHP_OTA_SLOW_SENDER_H


#define OTA_SLOW_SENDER_BLOCK_SIZE 3840


class ShpOTAUpdateSlowSender : public ShpUtility
{
	public:

		ShpOTAUpdateSlowSender();

		uint8_t setUpdateInfo(String payload, uint16_t packetSize);
		int getBlock(int blockNumber);
		int getNextPacket(uint8_t *data);

	protected:

		//uint8_t m_phase;
		int m_fwSize;
		String m_fwUrl;
		uint16_t m_packetSize;

		int m_thisBlockSize;
		int m_thisBlockPackets;
		uint16_t m_oneBlockPos;
		uint8_t	m_oneBlock[OTA_SLOW_SENDER_BLOCK_SIZE];

	public:

		int m_packetsPerBlock;
		uint16_t m_blockNumber;
		uint16_t m_packetInBlockNumber;
		int m_sendedBytes;

};

#endif
