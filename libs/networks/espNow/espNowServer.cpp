extern SHP_APP_CLASS *app;


#ifdef ESP32
#include <WiFi.h>
#include <esp_now.h>
#endif



ShpEspNowServer *g_espServer = NULL;




/*
void espNowServerOnPaired(uint8_t *ga, String ad)
{
  Serial.println("EspNowConnection : Client '"+ad+"' paired! ");

  g_espNow->endPairing();
}

void espNowServerOnConnected(uint8_t *ga, String ad)
{
  Serial.println("OnConnected: '"+ad+"'");
}
*/

void espNowServerOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}



static esp_now_peer_info_t espNowServerPeerInfo;



/**
 * ShpEspNowServer
 *
 */

ShpEspNowServer::ShpEspNowServer() :
																			m_pairingStatus(SHP_ENSPS_NONE),
																			m_pairingCounter(0),
																			m_pairingNextTry(0),
																			m_sendDeviceCfg(false)
{
	g_espServer = this;
}

void ShpEspNowServer::init()
{
	WiFi.mode(WIFI_STA);
	WiFi.macAddress(m_serverMac);
	Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

	esp_now_register_send_cb(espNowServerOnDataSent);
	esp_now_register_recv_cb(ShpEspNowServer::espNowServerOnDataRecv);
}

void ShpEspNowServer::pairingStart()
{
	Serial.println("Pairing started...");
	m_pairingCounter = 0;
	m_pairingNextTry = millis();
	m_pairingStatus = SHP_ENSPS_IN_PROGRESS;
}

void ShpEspNowServer::pairingStop(bool successfuly)
{
	Serial.print((successfuly) ? "SUCCESS: " : "TIMEOUT: ");
	Serial.println("Pairing stop..");
	m_pairingStatus = SHP_ENSPS_NONE;
	m_pairingCounter = 0;
	m_pairingNextTry = 0;
}

void ShpEspNowServer::pairingNextTry()
{
	m_pairingCounter++;

	if (m_pairingCounter > 20)
	{
		pairingStop(false);
		return;
	}

	Serial.printf("Pairing try #%d\n", m_pairingCounter);

	memcpy(&espNowServerPeerInfo.peer_addr, m_pairingMac, 6);
	esp_now_add_peer(&espNowServerPeerInfo);

	String data = shpMacToStr(m_serverMac);
	sendSinglePacket(m_pairingMac, data.c_str(), data.length(), SHP_ENPT_PAIR_REQUEST);
	m_pairingNextTry = millis() + 3000;

	esp_now_del_peer(m_pairingMac);
}

void ShpEspNowServer::espNowServerOnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
	shp_en_packet_t packet;
  memcpy(&packet, incomingData, len);

  Serial.print("Incoming packet len: ");
  Serial.print(len);

	Serial.print(" from: ");

	String macStr = shpMacToStr(mac);

	Serial.print(macStr);


	Serial.print("; packetType: ");
	Serial.print(packet.type);
	Serial.print("; dataLen: ");
	Serial.print(packet.dataLen);


	String payloadStr;
	for (int i = 0; i < packet.dataLen; i++)
		payloadStr.concat(packet.data[i]);

	Serial.print(" `");
	Serial.print(payloadStr);
	Serial.println("`");

	if (packet.type == SHP_ENPT_PAIR_RESPONSE)
	{
		g_espServer->pairingStop(true);
		return;
	}
	if (packet.type == SHP_ENPT_CFG_REQUEST)
	{
		if (!g_espServer->m_sendDeviceCfg)
		{
			g_espServer->m_sendDeviceCfgId = payloadStr;
			g_espServer->m_sendDeviceMac = shpMacToStr(mac);
			g_espServer->m_sendDeviceCfg = true;
		}
		return;
	}

	g_espServer->receiveData(mac, &packet);
}

void ShpEspNowServer::sendSinglePacket(const uint8_t *peer_addr, const char *data, size_t len, uint8_t packetType /* = SHP_ENPT_ONE_DATA_PACKET */, uint16_t packetIndex /* = 0 */)
{
	static shp_en_packet_t packet;
	packet.type = packetType;
	packet.packetIndex = packetIndex;
	packet.dataLen = len;

	memcpy(&packet.data, data, packet.dataLen);
	esp_now_send(NULL, (uint8_t *) &packet, packet.dataLen + SHP_PACKET_HEADER_LEN);
}

void ShpEspNowServer::sendDeviceCfg()
{
	Serial.println("send device cfg start");

		// -- LOAD CFG FROM SERVER
  String url = "http://" + app->cfgServerHostName + "/cfg/" + m_sendDeviceCfgId + ".json";
  Serial.println("========== CFG URL: "+url);

  String data;

  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin");
	http.setTimeout(5);
  if (http.begin(client, url))
  {
    Serial.print("[HTTP] GET...");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      Serial.printf("[HTTP] GET... code: %d", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        data = http.getString();
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
      }
    }
    else
    {
      Serial.printf("[HTTP] Unable to connect");
    }
    http.end();

    Serial.println(data);
		Serial.println(data.length());

    if (data.length())
    {
			g_espServer->sendData(data, SHP_ENPT_CFG_RESPONSE, m_sendDeviceMac.c_str());
    }
  }

	m_sendDeviceCfgId = "";
	g_espServer->m_sendDeviceCfg = false;
}

void ShpEspNowServer::onReceiveData(uint8_t packetType, String data)
{
	//Serial.printf ("--- RECEIVE DATA TYPE %i ---\n", packetType);
	//Serial.println(data);

	if (packetType == SHP_ENPT_MESSAGE)
	{
		int newLinePos = data.indexOf('\n');
		if (newLinePos > 0)
		{
			String topic = data.substring(0, newLinePos);
			String payload = data.substring(newLinePos + 1);
			app->publish(payload.c_str(), topic.c_str());
		}
	}
}

void ShpEspNowServer::loop()
{
	unsigned long now = millis();
	if (m_pairingStatus == SHP_ENSPS_IN_PROGRESS)
	{
		if (now > m_pairingNextTry)
		{
			pairingNextTry();
		}
	}
	if (m_sendDeviceCfg)
	{
		sendDeviceCfg();
	}

	ShpEspNow::loop();
}


/**
 * ShpEspNowServerIOPort
 *
 */

ShpEspNowServerIOPort::ShpEspNowServerIOPort() : m_espNowServer(NULL)
{
}

void ShpEspNowServerIOPort::init(JsonVariant portCfg)
{
	ShpIOPort::init(portCfg);

	m_espNowServer = new ShpEspNowServer();
	m_espNowServer->init();
}

void ShpEspNowServerIOPort::onMessage(byte* payload, unsigned int length, const char* subCmd)
{
	String payloadStr;
	for (int i = 0; i < length; i++)
		payloadStr.concat((char)payload[i]);

	if (payloadStr == "pairStart")
	{
		m_espNowServer->pairingStart();
	}
}

void ShpEspNowServerIOPort::loop()
{
	if (m_espNowServer)
		m_espNowServer->loop();
}
