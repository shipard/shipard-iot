extern SHP_APP_CLASS *app;



ShpOTAUpdateSlowSender::ShpOTAUpdateSlowSender() : m_fwSize(0), m_packetSize(0),
                                                   m_thisBlockSize(0),
                                                   m_thisBlockPackets(0),
                                                   m_oneBlockPos(0),
                                                   m_packetsPerBlock(0),
                                                   m_blockNumber(0),
                                                   m_packetInBlockNumber(0),
                                                   m_sendedBytes(0)
{

}


uint8_t ShpOTAUpdateSlowSender::setUpdateInfo(String payload, uint16_t packetSize)
{
  m_packetsPerBlock = OTA_SLOW_SENDER_BLOCK_SIZE / packetSize;
  m_packetSize = packetSize;

	m_fwSize = 0;
  m_fwUrl = "";
  //m_phase = OTA_SLOW_PHASE_NONE;

  if (payload.length() == 0)
  {
    app->log(shpllError, "[UPGRADE] No payload");
    return 0;
  }

  int spacePos = payload.indexOf(' ');
  if (spacePos == -1)
  {
    app->log(shpllError, "[UPGRADE] Invalid payload format; missing space");
    return 0;
  }

  String fwLenStr = payload.substring(0, spacePos);
  m_fwSize = atoi(fwLenStr.c_str());
  m_fwUrl = payload.substring(spacePos + 1);

  if (m_fwSize == 0)
  {
    app->log(shpllError, "[UPGRADE] Invalid firmware lenght");
    return 0;
  }
  if (m_fwUrl.length() == 0)
  {
    app->log(shpllError, "[UPGRADE] Blank firmware url");
    return 0;
  }

  Serial.println("OTA SEND INIT SUCCESS");
  //m_phase = OTA_SLOW_PHASE_INITIALIZED;

  // getBlock(0);

  return 1;
}

int ShpOTAUpdateSlowSender::getBlock(int blockNumber)
{
  Serial.print("[");

  m_packetInBlockNumber = 0;
  m_blockNumber = blockNumber;

  int blockSize = m_packetsPerBlock * m_packetSize;//OTA_SLOW_SENDER_BLOCK_SIZE;
	int blockBegin = blockNumber * blockSize;

  if (blockBegin >= m_fwSize)
    return 0;

	int blockEnd = blockBegin + blockSize - 1;

  if (blockEnd >= m_fwSize)
  {
    blockEnd = m_fwSize - 1;
    blockSize = blockEnd - blockBegin + 1;
  }


  WiFiClient client;
  HTTPClient http;

	http.setTimeout(60);


	char rangeHeaderValue[64];
	sprintf(rangeHeaderValue, "bytes=%d-%d", blockBegin, blockEnd);

	http.begin(client, m_fwUrl);
	http.addHeader("range", rangeHeaderValue);

  int readedBytes = 0;

	int httpCode = http.GET();

	if (httpCode > 0)
	{
		//Serial.printf("[OTA] GET... code: %d\n", httpCode);

		if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_PARTIAL_CONTENT)
		{
      Serial.print("] ");
			readedBytes = client.read(m_oneBlock, blockSize);
      m_thisBlockSize = readedBytes;
			Serial.printf("%d: readed %d bytes: ", m_blockNumber, readedBytes);
      for (int i = 0; i < 10; i++)
      {
        Serial.printf("0x%02X ", m_oneBlock[i]);
      }
      Serial.println(rangeHeaderValue);
		}
		else
		{
			app->log(shpllError, "[OTA UPGRADE] download FAILED; error: `%s`", http.errorToString(httpCode).c_str());
			return -1;
		}
	}
	else
	{
		app->log(shpllError, "[OTA UPGRADE] download FAILED; unable to connect");
		return -1;
	}

	http.end();

	return readedBytes;
}

int ShpOTAUpdateSlowSender::getNextPacket(uint8_t *data)
{
  //if (m_sendedBytes >= m_fwSize)
  //  return 0;

  int posInBlock = m_packetInBlockNumber * m_packetSize;
  if (posInBlock >= /*OTA_SLOW_SENDER_BLOCK_SIZE*/ m_thisBlockSize)
  {
    return 0;
  }

  m_packetInBlockNumber++;
  m_sendedBytes += m_packetSize;
  memcpy(data, m_oneBlock + posInBlock, m_packetSize);


  if (m_packetInBlockNumber < 3)
  {
    Serial.printf("SP: bn: %d, pib: %d, pos: %d; sended: %d; ", m_blockNumber, m_packetInBlockNumber, posInBlock, m_sendedBytes);
    for (int i = 0; i < m_packetSize; i++)
    {
      Serial.printf("0x%02X ", data[i]);
    }
    Serial.println();
  }


  return 4;
}

