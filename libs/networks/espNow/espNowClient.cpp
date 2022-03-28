#include <WiFi.h>

#ifdef ESP32
#include <esp_now.h>
#include <esp_wifi.h>
#endif

#ifdef ESP8266
#include <espnow.h>
#include <ESP8266WiFi.h>
#endif


extern SHP_APP_CLASS *app;
ShpEspNowClient *g_espClient = NULL;
static esp_now_peer_info_t espNowClientPeerInfo;


/*
void espClientOnSendError(uint8_t* ad)
{
  Serial.println("ERROR: Sending to '"+g_espClient->m_espNow->macToStr(ad)+"' was not possible!");  
}
*/

void espNowClientOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

ShpEspNowClient::ShpEspNowClient() : 
																			m_mode(SHP_ENS_UNINITIALIZED)
{
	g_espClient = this;
}

bool ShpEspNowClient::readServerAddress()
{
	app->m_prefs.begin("EspNow");
	m_serverAddress = app->m_prefs.getString("serverMac", "");
	app->m_prefs.end();

	if (m_serverAddress.length() == 12)
	{
		Serial.printf("Loaded MAC is %s\n", m_serverAddress.c_str());

		return true;
	}

	return false;
}

void ShpEspNowClient::writeServerAddress()
{
	app->m_prefs.begin("EspNow");
	app->m_prefs.putString("serverMac", m_serverAddress);
	app->m_prefs.end();

	Serial.printf("Stored MAC is %s\n", m_serverAddress.c_str());
}

void ShpEspNowClient::writeIotBoxConfig(String data)
{
	app->m_prefs.begin("IotBox");
	app->m_prefs.putString("config", data);
	app->m_prefs.end();
}

void ShpEspNowClient::doPair()
{
	Serial.println("Pairing mode ongoing...");

#if defined(ESP8266)
		wifi_set_macaddr(STATION_IF, &m_pairingMac[0]);
#elif defined(ESP32)	
		esp_wifi_set_mac(WIFI_IF_STA, &m_pairingMac[0]);
#endif

	Serial.println("MAC set to : "+WiFi.macAddress());  

	m_mode = SHP_WAIT_FOR_CLIENT_PAIR;
}

void ShpEspNowClient::init()
{
	WiFi.mode(WIFI_MODE_STA);

	WiFi.macAddress(m_clientMac);
	Serial.println("Client addresss is " + WiFi.macAddress());

	if (esp_now_init() != ESP_OK) 
	{
    Serial.println("Error initializing ESP-NOW");
    return;
  }

	esp_now_register_send_cb(espNowClientOnDataSent);
	esp_now_register_recv_cb(ShpEspNowClient::espNowClientOnDataRecv);

	if (readServerAddress())
	{
		setServerAddress(m_serverAddress.c_str());

		if (app->m_wakeUpFromSleep)
		{
			if (app->setIotBoxFromStoredCfg())
			{
				m_mode = SHP_ENS_IDLE;
				Serial.printf("####### CONFIGURED AFTER %ld millis\n", millis());
				return;
			}
		}

		m_mode = SHP_WAIT_FOR_CLIENT_CFG;
		Serial.println("SEND CFG REQUEST...");
		delay(100);
		sendSinglePacket(app->macHostName.c_str(), app->macHostName.length(), SHP_ENPT_CFG_REQUEST);
		delay(100);
	}
	else
	{
		doPair();
	}
}

void ShpEspNowClient::setServerAddress(const char *address)
{
	uint8_t serverAddress[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
	shpStrToMac(address, serverAddress);
	
	memcpy(espNowClientPeerInfo.peer_addr, serverAddress, 6);
  espNowClientPeerInfo.channel = 0;  
	espNowClientPeerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&espNowClientPeerInfo) != ESP_OK)
	{
    Serial.println("Failed to add peer");
    return;
  }

	Serial.println("server added...");
}

void ShpEspNowClient::sendSinglePacket(const char *data, size_t len, uint8_t packetType /* = SHP_ENPT_ONE_DATA_PACKET */, uint16_t packetIndex /* = 0 */)
{
	static shp_en_packet_t packet;
	packet.type = packetType;
	packet.packetIndex = packetIndex;
	packet.dataLen = len;

	//Serial.println(data);
	//Serial.println(len);

	memcpy(&packet.data, data, packet.dataLen);
	esp_now_send(NULL, (uint8_t *) &packet, packet.dataLen + SHP_PACKET_HEADER_LEN);
}

void ShpEspNowClient::espNowClientOnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
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


	if (packet.type == SHP_ENPT_PAIR_REQUEST)
	{
		g_espClient->doPairRequestInit(&packet);
		return;
	}

	g_espClient->receiveData(mac, &packet);
	return;
}

void ShpEspNowClient::doPairRequestInit(shp_en_packet_t *packet)
{
	Serial.println("PAIR REQUEST!");

	if (m_mode != SHP_WAIT_FOR_CLIENT_PAIR)
		return;

	m_serverAddress = "";
	for (int i = 0; i < packet->dataLen; i++)
		m_serverAddress.concat(packet->data[i]);

	m_mode = SHP_WAIT_DO_CLIENT_PAIR_REQUEST;
}

void ShpEspNowClient::doPairRequestRun()
{
	writeServerAddress();
	setServerAddress(m_serverAddress.c_str());

	String data = m_serverAddress;
	sendSinglePacket(data.c_str(), data.length(), SHP_ENPT_PAIR_RESPONSE);

	delay(1000);

	app->reboot();
	m_mode = SHP_ENS_IDLE;
}

void ShpEspNowClient::onReceiveData(uint8_t packetType, String data)
{
	//Serial.printf ("--- RECEIVE DATA TYPE %i ---\n", packetType);
	//Serial.println(data);

	if (packetType == SHP_ENPT_CFG_RESPONSE)
	{
		app->setIotBoxCfg(data);
		if (app->m_boxConfigLoaded)
		{
			writeIotBoxConfig(data);

			m_mode = SHP_ENS_IDLE;
		}	
	}
}

void ShpEspNowClient::loop()
{
	ShpEspNow::loop();

	if (m_mode == SHP_WAIT_FOR_CLIENT_PAIR)
	{
		Serial.println("wait for pairing...");
		delay(1000);
		return;
	}

	if (m_mode == SHP_WAIT_DO_CLIENT_PAIR_REQUEST)
	{
		Serial.println("pairing done?...");
		doPairRequestRun();
		return;
	}

	if (m_mode == SHP_WAIT_FOR_CLIENT_CFG)
	{
		Serial.println("wait for client cfg...");
		delay(1000);
		return;
	}
}
