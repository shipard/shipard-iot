#ifndef SHP_OTA_UPDATE_H
#define SHP_OTA_UPDATE_H


class ShpOTAUpdate : public ShpUtility
{
	public:

		ShpOTAUpdate();

		void doFwUpgradeRequest(String payload);
		void clearUpgradeRequest();

		#if defined(SHP_ETH) || defined(SHP_WIFI)
		void doFwUpgradeRun(int fwLenght, String fwUrl);
		#endif

};


#endif
