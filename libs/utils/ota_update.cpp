extern Application *app;



ShpOTAUpdate::ShpOTAUpdate()
{
}

void ShpOTAUpdate::doFwUpgradeRun(int fwLenght, String fwUrl)
{
	clearUpgradeRequest();

  //Serial.printf("OTA URL: `%s`, fwLenght: `%d`\n", fwUrl.c_str(), fwLenght);

  String data;

  WiFiClient client;
  HTTPClient http;

	http.setTimeout(60);
  if (http.begin(client, fwUrl)) 
  {
    //Serial.print("[OTA] GET...");
		app->log(shpllStatus, "[OTA UPGRADE] download started");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      //Serial.printf("[OTA] GET... code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        //data = http.getString();
      } 
      else 
      {
				app->log(shpllError, "[OTA UPGRADE] download FAILED; error: `%s`", http.errorToString(httpCode).c_str());
				return;
      }
    } 
    else 
    {
			app->log(shpllError, "[OTA UPGRADE] download FAILED; unable to connect");
			return;
    }
	}
	else
	{
		app->log(shpllError, "[OTA UPGRADE] download FAILED; unable to GET");
		return;
	}
	

	bool canBegin = Update.begin(fwLenght);
	if (canBegin)
	{
		size_t fwLoadedSize = Update.writeStream(http.getStream());
		//Serial.printf(" DOWNLOADED: %i bytes \n", fwLoadedSize);
		app->log(shpllStatus, "[OTA UPGRADE] downloaded %i bytes", fwLoadedSize);
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
  else 
	{
		app->log(shpllError, "[OTA UPGRADE] Not enough space to begin OTA");
		client.flush();
	}

	http.end();
}

void ShpOTAUpdate::doFwUpgradeRequest(String payload)
{
	// String url = "http://" + app->cfgServerHostName + "/firmware/" /*macHostName*/ + "firmware.bin";

	int fwLenght = 0;
	String fwUrl = "";

	if (payload.length() != 0)
	{
		int spacePos = payload.indexOf(' ');
		if (spacePos == -1)
		{
			app->log(shpllError, "[UPGRADE] Invalid payload format; missing space");
			return;
		}

		String fwLenStr = payload.substring(0, spacePos);
		fwLenght = atoi(fwLenStr.c_str());
		fwUrl = payload.substring(spacePos + 1);

		if (fwLenght == 0)
		{
			app->log(shpllError, "[UPGRADE] Invalid firmware lenght");
			return;
		}
		if (fwUrl.length() == 0)
		{
			app->log(shpllError, "[UPGRADE] Blank firmware url");
			return;
		}
	}

	if (fwLenght == 0 || fwUrl.length() == 0)
		return;

	//Serial.printf("LEN: `%d`, URL: `%s`", fwLenght, fwUrl.c_str());

	app->m_prefs.begin("IotBox");
	app->m_prefs.putBool("doUpgrade", true);
	app->m_prefs.putInt("fwLength", fwLenght);
	app->m_prefs.putString("fwUrl", fwUrl);
	app->m_prefs.end();

	app->log(shpllStatus, "[UPGRADE] request done. Rebooting after 3 sec...");
	usleep(3000000);
	app->reboot();
}

void ShpOTAUpdate::clearUpgradeRequest()
{
	app->m_prefs.begin("IotBox");
	app->m_prefs.putBool("doUpgrade", false);
	app->m_prefs.putBool("fwLength", 0);
	app->m_prefs.putString("fwUrl", "");
	app->m_prefs.end();
}