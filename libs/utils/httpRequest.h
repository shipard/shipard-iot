#ifndef SHP_HTTP_REQUEST_H
#define SHP_HTTP_REQUEST_H


class ShpHttpRequest {
	public:

		ShpHttpRequest();

		bool begin(const char *url);
		void end();


	public:

		WiFiClient m_wifiClient;
  	HTTPClient m_httpClient;


};


#endif
