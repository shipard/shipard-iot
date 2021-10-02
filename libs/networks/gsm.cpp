extern Application *app;


#define UART_BAUD   9600
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_PIN     12


ShpModemGSM::ShpModemGSM()
{
	SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

	m_modem = new TinyGsm(SerialAT);

	pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);



	if (!m_modem->restart()) {
    Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = m_modem->getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);

  String modemInfo = m_modem->getModemInfo();
  delay(500);
  Serial.println("Modem Info: " + modemInfo);


Serial.println("\n\n\nWaiting for network...");
  if (!m_modem->waitForNetwork()) 
	{
    delay(1000);
		Serial.println("Waiting for network 2...");
  //  return;
  }

  if (m_modem->isNetworkConnected()) {
    Serial.println("Network connected");
  }


	const char apn[]  = "internet.t-mobile.cz";     //SET TO YOUR APN
	const char gprsUser[] = "";
	const char gprsPass[] = "";

  Serial.println("\n---Starting GPRS TEST---\n");
  Serial.println("Connecting to: " + String(apn));
  if (!m_modem->gprsConnect(apn, gprsUser, gprsPass)) {
    delay(6000);
		Serial.println("Connecting to 2: " + String(apn));
    //return;
  }	



	Serial.print("GPRS status: ");
  if (m_modem->isGprsConnected()) {
    Serial.println("connected");
  } else {
    Serial.println("not connected");
  }

  String ccid = m_modem->getSimCCID();
  Serial.println("CCID: " + ccid);

  String imei = m_modem->getIMEI();
  Serial.println("IMEI: " + imei);

  String cop = m_modem->getOperator();
  Serial.println("Operator: " + cop);

  IPAddress local = m_modem->localIP();
  Serial.println("Local IP: " + String(local));

}


void ShpModemGSM::loop()
{
	
}

void ShpModemGSM::turnOn()
{
}

void ShpModemGSM::turnOff()
{
}

