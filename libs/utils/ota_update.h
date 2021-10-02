#ifndef SHP_OTA_UPDATE_H
#define SHP_OTA_UPDATE_H


class ShpOTAUpdate : public ShpUtility 
{
	public:

		ShpOTAUpdate();

		void doFwUpgradeRequest(String payload);
		void doFwUpgradeRun(int fwLenght, String fwUrl);
		void clearUpgradeRequest();

};


#endif
