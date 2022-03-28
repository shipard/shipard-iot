#ifdef ESP32
#include <esp_now.h>
#endif

#ifdef ESP8266
#include <espnow.h>
#endif


String shpMacToStr(const uint8_t* mac)
{
	char macAddr[13];
	macAddr[12] = 0;
	
	sprintf(macAddr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);		

	return String(macAddr);
}


void shpStrToMac(const char* str, uint8_t* mac)
{
	if(strlen(str) != 12)
		return;
		
	char buffer[2];
	
	for(int i = 0; i < 6; i++)
	{
		strncpy ( buffer, str+i*2, 2 );
		mac[i] = strtol(buffer, NULL, 16);
	}
}

ShpEspNow::ShpEspNow() : m_anyDataToSend(0), m_anyDataToReceive(0)
{
	for (int i = 0; i < SHP_ENSQ_LEN; i++)
	{
		m_sendQueue[i].state = SHP_ENSQS_NONE;
		m_sendQueue[i].nextPacketIdx = 0;
		m_sendQueue[i].totalPackets = 0;
	}
	for (int i = 0; i < SHP_ENRQ_LEN; i++)
	{
		m_recvQueue[i].state = SHP_ENSQS_NONE;
		m_recvQueue[i].lastPacketIndex = 0;
		m_recvQueue[i].totalPackets = 0;
	}
}

void ShpEspNow::sendData(String &data, const uint8_t packetType, const char *toAddress)
{
	for (int i = 0; i < SHP_ENSQ_LEN; i++)
	{
		if (m_sendQueue[i].state != SHP_ENSQS_NONE)
			continue;

		m_sendQueue[i].state = SHP_ENSQS_WAIT_FOR_START;
		m_sendQueue[i].data = data;
		m_sendQueue[i].packetType = packetType;
		m_sendQueue[i].nextPacketIdx = 0;
		m_sendQueue[i].totalPackets = data.length() / SHP_MAX_PACKET_DATA_LEN;
		if (data.length() % SHP_MAX_PACKET_DATA_LEN)
			m_sendQueue[i].totalPackets++;
		shpStrToMac (toAddress, m_sendQueue[i].macAddr);

		m_anyDataToSend++;
		return;
	}
}

void ShpEspNow::sendNextDataPacket()
{
	int doQueueItem = SHP_ENSQ_LEN + 1;
	for (int i = 0; i < SHP_ENSQ_LEN; i++)
	{
		if (m_sendQueue[i].state != SHP_ENSQS_IN_PROGRESS)
			continue;
		doQueueItem = i;
		break;
	}

	if (doQueueItem > SHP_ENSQ_LEN)
	{
		for (int i = 0; i < SHP_ENSQ_LEN; i++)
		{
			if (m_sendQueue[i].state != SHP_ENSQS_WAIT_FOR_START)
				continue;
			m_sendQueue[i].state = SHP_ENSQS_IN_PROGRESS;
			doQueueItem = i;
		}
	}

	if (doQueueItem > SHP_ENSQ_LEN)
		return;

	sendNextDataPacketItem(doQueueItem);
}

static esp_now_peer_info_t espNowPeerInfo;

void ShpEspNow::sendNextDataPacketItem(int i)
{
	int from = m_sendQueue[i].nextPacketIdx * SHP_MAX_PACKET_DATA_LEN;
	int len = SHP_MAX_PACKET_DATA_LEN;
	if ((m_sendQueue[i].data.length() - from) < len)
		len = m_sendQueue[i].data.length() - from;

	// -- send
	static shp_en_packet_t packet;
	packet.type = m_sendQueue[i].packetType;
	packet.packetIndex = m_sendQueue[i].nextPacketIdx;
	packet.totalPackets = m_sendQueue[i].totalPackets;
	packet.dataLen = len;

	//Serial.printf("--- send msg (len: %d) packet %d/%d, len=%d, from=%d ---\n", m_sendQueue[i].data.length(), packet.packetIndex, packet.totalPackets, packet.dataLen, from);
	



	memcpy(&packet.data, m_sendQueue[i].data.c_str() + from, packet.dataLen);


	memcpy(&espNowPeerInfo.peer_addr, m_sendQueue[i].macAddr, 6);
	esp_now_add_peer(&espNowPeerInfo);

	esp_now_send(m_sendQueue[i].macAddr, (uint8_t *) &packet, packet.dataLen + SHP_PACKET_HEADER_LEN);


	esp_now_del_peer(m_sendQueue[i].macAddr);
	delay(20);

	m_sendQueue[i].nextPacketIdx++;
	if (m_sendQueue[i].nextPacketIdx >= m_sendQueue[i].totalPackets)
	{
		//Serial.println("=== SEND MESSAGE DONE ===");
		m_sendQueue[i].state = SHP_ENSQS_NONE;
		m_sendQueue[i].data = "";
		m_sendQueue[i].nextPacketIdx = 0;
		m_sendQueue[i].totalPackets = 0;
		m_anyDataToSend--;
	}
}

void ShpEspNow::receiveData(const uint8_t *peer_addr, shp_en_packet_t *packet)
{
	//Serial.println("recv-ddata-1");
	int doQueueItem = SHP_ENRQ_LEN + 1;
	if (packet->packetIndex == 0)
	{
		for (int i = 0; i < SHP_ENRQ_LEN; i++)
		{
			if (m_recvQueue[i].state == SHP_ENSQR_NONE)
			{			
				doQueueItem = i;
				break;
			}
		}
	}
	else
	{
		for (int i = 0; i < SHP_ENRQ_LEN; i++)
		{
			if (m_recvQueue[i].state != SHP_ENSQR_IN_PROGRESS)
				continue;
			if (memcmp(peer_addr, &m_recvQueue[i].macAddr, 6) == 0)
			{
				doQueueItem = i;
				break;
			}	
		}
	}

	if (doQueueItem > SHP_ENRQ_LEN)
		return;

	//Serial.printf("recv-data-index %d \n", doQueueItem);

	m_recvQueue[doQueueItem].state = SHP_ENSQR_IN_PROGRESS;
	memcpy(&m_recvQueue[doQueueItem].macAddr, peer_addr, 6);

	for (int x = 0; x < packet->dataLen; x++)
		m_recvQueue[doQueueItem].data.concat(packet->data[x]);
	
	m_recvQueue[doQueueItem].type = packet->type;
	m_recvQueue[doQueueItem].lastPacketIndex = packet->packetIndex;
	m_recvQueue[doQueueItem].totalPackets = packet->totalPackets;

	if (packet->packetIndex == packet->totalPackets - 1)
	{
		//Serial.printf("recv-data-done\n");
		m_recvQueue[doQueueItem].state = SHP_ENSQR_WAIT_FOR_RECEIVE;
		m_anyDataToReceive++;
	}
}

void ShpEspNow::doReceiveData()
{
	for (int i = 0; i < SHP_ENRQ_LEN; i++)
	{
		if (m_recvQueue[i].state == SHP_ENSQR_WAIT_FOR_RECEIVE)
		{
			onReceiveData(m_recvQueue[i].type, m_recvQueue[i].data);

			m_recvQueue[i].data = "";
			m_recvQueue[i].type = 0;
			m_recvQueue[i].lastPacketIndex = 0;
			m_recvQueue[i].totalPackets = 0;

			m_recvQueue[i].state = SHP_ENSQR_NONE;
			m_anyDataToReceive--;
			return;
		}
	}
}

void ShpEspNow::onReceiveData(uint8_t packetType, String data)
{
	Serial.println ("### ShpEspNow::onReceiveData ###");
}

void ShpEspNow::loop()
{
	if (m_anyDataToSend)
		sendNextDataPacket();

	if (m_anyDataToReceive)
		doReceiveData();
}
