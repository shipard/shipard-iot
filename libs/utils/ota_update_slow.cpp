
ShpOTAUpdateSlow::ShpOTAUpdateSlow() :
                                      m_phase(OTA_SLOW_PHASE_NONE),
                                      m_fwSize(0),
                                      m_incomingPackets(0),
                                      m_incomingBytes(0),
                                      m_writeBufferPos(0),
                                      m_blockNumberToDownload(0),
                                      m_blockNumber(-1),
                                      m_packetInBlockNumber(0)
{
}

uint8_t ShpOTAUpdateSlow::setUpdateInfo(String payload)
{
	m_fwSize = 0;
  m_fwUrl = "";
  m_phase = OTA_SLOW_PHASE_NONE;

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

  begin();
  m_phase = OTA_SLOW_PHASE_INITIALIZED;
  return 1;
}

void ShpOTAUpdateSlow::begin()
{
  bool canBegin = Update.begin(m_fwSize);
}

void ShpOTAUpdateSlow::end()
{
  if (Update.end())
  {
    app->log(shpllStatus, "[OTA UPGRADE] done!");
    //Serial.println("OTA done!");
    if (Update.isFinished())
    {
      app->log(shpllStatus, "[OTA UPGRADE] Update successfully completed. Rebooting...");
      //Serial.println("Update successfully completed. Rebooting.");
      ESP.restart();
    }
    else
    {
      app->log(shpllError, "[OTA UPGRADE] Update not finished? Something went wrong!");
      //Serial.println("Update not finished? Something went wrong!");
    }
  }
  else
  {
    app->log(shpllError, "[OTA UPGRADE] Error #%d: %s", Update.getError(), Update.errorString());
  }
}

void ShpOTAUpdateSlow::setBlockNumber(uint8_t blockNumber)
{
  m_blockNumberToDownloadSize = OTA_SLOW_WRITE_BUFFER_SIZE;

	int blockBegin = blockNumber * OTA_SLOW_WRITE_BUFFER_SIZE;
  int blockEnd = blockBegin + OTA_SLOW_WRITE_BUFFER_SIZE - 1;

  if (blockEnd >= m_fwSize)
  {
    blockEnd = m_fwSize - 1;
    m_blockNumberToDownloadSize = blockEnd - blockBegin + 1;
  }

  //Serial.printf("Block size: %d (%d/%d)\n", m_blockNumberToDownloadSize, blockBegin, blockEnd);
}

void ShpOTAUpdateSlow::addPacket(int dataLen, uint8_t *packetData)
{
  uint8_t blockNumber = packetData[1];
  uint16_t packetInBlockNumber = makeWord(packetData[3], packetData[2]);

  int bytes = dataLen - 4;

  memcpy(m_writeBuffer + m_writeBufferPos, packetData + 4, bytes);
  m_writeBufferPos += bytes;

  m_incomingPackets++;

  if (m_writeBufferPos == m_blockNumberToDownloadSize)
  {
    m_phase = OTA_SLOW_PHASE_NEED_WRITE_BLOCK;
    //return 1;
  }
}

uint8_t ShpOTAUpdateSlow::checkUpdateProgress()
{
  if (m_phase == OTA_SLOW_PHASE_NEED_WRITE_BLOCK)
  {
    writeBlock();

    if (m_blockNumberToDownload == 108)
    {
      Serial.printf("TEST-BLOCK-108: %d / %d\n", m_incomingBytes, m_fwSize);
    }

    if (m_incomingBytes == m_fwSize)
    {
      Serial.println("FW DOWNLOADED!");
      Serial.printf("TOTAL %d packets, %d bytes\n", m_incomingPackets, m_incomingBytes);
      m_phase = OTA_SLOW_PHASE_WAIT_FOR_REBOOT;
      end();

      return OTA_SLOW_NEXT_OP_NONE;
    }

    m_phase = OTA_SLOW_PHASE_INCOMING_PACKETS;
    return OTA_SLOW_NEXT_OP_NEXT_BLOCK;
  }

  return OTA_SLOW_NEXT_OP_NONE;
}

void ShpOTAUpdateSlow::writeBlock()
{
  m_incomingBytes += m_writeBufferPos;

  //long write_start = millis();
  //Serial.printf("### write: %d bytes (%d/%d), ", m_writeBufferPos, m_incomingBytes, m_fwSize);
  int res = Update.write(m_writeBuffer, m_writeBufferPos);
  //long write_end = millis();
  //Serial.printf("%d ms\n", write_end - write_start);

  if (res != m_writeBufferPos)
  {
    Serial.printf("--- WRITE ERROR %d  / `%d`\n", m_writeBufferPos, res);
  }

  m_writeBufferPos = 0;
  m_blockNumberToDownload++;
}
